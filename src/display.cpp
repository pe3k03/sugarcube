// Upstream SmartClock firmware by B. van Weerd, with SugarCube changes.
// Copyright (c) 2026 B. van Weerd. Upstream code in this file is MIT. See LICENSE.
// Copyright (c) 2026 Peter Štieber. SugarCube changes are PolyForm Noncommercial 1.0.0. See LICENSE-SUGARCUBE.
// Required Notice: Copyright (c) 2026 Peter Štieber. SugarCube changes may not be sold or used commercially.

#include "display.h"

#include "auth.h"
#include "config.h"
#include "feeds.h"
#include "logger.h"
#include "pixelfont.h"
#include "settings.h"
#include <ESP8266WiFi.h>
#include <time.h>
#include <math.h>
#include <vector>

#define FONT_INFO 1
#define FONT_LABEL 2
#define FONT_BODY 4
#define FONT_TITLE 6
#define FONT_HUGE 7

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite clockDynamicSprite = TFT_eSprite(&tft);
DisplayState displayState;
int scrollPos = 240;

namespace {

constexpr uint16_t rgb565(uint8_t red, uint8_t green, uint8_t blue) {
    return ((red & 0xF8) << 8) | ((green & 0xFC) << 3) | (blue >> 3);
}

constexpr int kPageMargin = 4;
constexpr int kPageWidth = DISPLAY_WIDTH - (kPageMargin * 2);
constexpr int kHeaderTitleY = 10;
constexpr int kHeaderSubtitleY = 12;
constexpr int kHeaderRuleY = 32;
constexpr int kContentTopY = 40;
constexpr int kClockDynamicRegionX = 12;
constexpr int kClockDynamicRegionY = 44;
constexpr int kClockDynamicRegionWidth = DISPLAY_WIDTH - (kClockDynamicRegionX * 2);
constexpr int kClockDynamicRegionHeight = 64;
constexpr int kClockTimeY = 54;
constexpr int kClockMetaY = 14;
constexpr int kClockFooterY = 148;
constexpr int kClockFooterHeight = 78;
// The 1 bpp buffer takes about 1.7 KB; keep at least 8 KB for request handling.
constexpr uint32_t kClockSpriteMinFreeHeapBytes = 10000UL;
constexpr uint32_t kClockSpriteRetryIntervalMs = 5000UL;
constexpr uint8_t kPageTransitionFadeDownSteps = 2;
constexpr uint8_t kPageTransitionFadeUpSteps = 3;
constexpr uint16_t kPageTransitionFadeStepDelayMs = 12;
constexpr uint8_t kPageTransitionDimPercent = 82;
constexpr uint8_t kPageTransitionMinimumBrightness = 18;
constexpr size_t kTemporaryMessageBufferSize = 96;

struct ThemePalette {
    uint16_t background;
    uint16_t surface;
    uint16_t surfaceAlt;
    uint16_t accent;
    uint16_t accentSoft;
    uint16_t text;
    uint16_t muted;
    uint16_t positive;
    uint16_t negative;
    uint16_t warning;
};

struct Rgb888 {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
};

struct ThemeSelection {
    uint8_t theme;
    bool customThemeEnabled;
    const char *background;
    const char *surface;
    const char *accent;
    const char *text;
};

#include "device_theme_palettes.h"

int savedBrightness = 100;
int appliedBrightness = -1;
bool backlightOn = true;

enum DisplayRenderMode : uint8_t {
    DISPLAY_MODE_DASHBOARD = 0,
    DISPLAY_MODE_TEMP_MESSAGE,
    DISPLAY_MODE_AP,
};

struct RenderCache {
    bool valid;
    uint8_t mode;
    uint8_t page;
    uint8_t theme;
    uint32_t staticHash;
    uint32_t dynamicHash;
};

RenderCache renderCache = {false, DISPLAY_MODE_DASHBOARD, 0, 0, 0, 0};

struct ClockMetaCache {
    bool valid;
    char metaLine[40];
};

ClockMetaCache clockMetaCache = {false, {0}};
char footerClockCache[16] = {0};
bool clockDynamicSpriteReady = false;
bool clockDynamicSpriteAttempted = false;
uint32_t clockDynamicSpriteLastAttemptMs = 0;
bool clockDynamicSpriteAllowed = false;
bool resumeClockSpriteAfterDynamicSuspend = false;
uint8_t dynamicResourceSuspendDepth = 0;

struct TemporaryMessageState {
    bool active;
    unsigned long expiresAtMs;
    char message[kTemporaryMessageBufferSize];
};

TemporaryMessageState temporaryMessage = {false, 0, {0}};

constexpr uint32_t kFnvOffset = 2166136261UL;
constexpr uint32_t kFnvPrime = 16777619UL;

void hashBytes(uint32_t &hash, const uint8_t *data, size_t length) {
    for (size_t index = 0; index < length; ++index) {
        hash ^= data[index];
        hash *= kFnvPrime;
    }
}

template <typename TValue>
void hashValue(uint32_t &hash, const TValue &value) {
    hashBytes(hash, reinterpret_cast<const uint8_t*>(&value), sizeof(TValue));
}

void hashCString(uint32_t &hash, const char *value) {
    if (value == nullptr) {
        const uint8_t zero = 0;
        hashBytes(hash, &zero, sizeof(zero));
        return;
    }

    hashBytes(hash, reinterpret_cast<const uint8_t*>(value), strlen(value));
    const uint8_t terminator = 0;
    hashBytes(hash, &terminator, sizeof(terminator));
}

void invalidateRenderCache() {
    renderCache.valid = false;
    clockMetaCache.valid = false;
    footerClockCache[0] = '\0';
    clockMetaCache.metaLine[0] = '\0';
}

void clearTemporaryMessage() {
    temporaryMessage.active = false;
    temporaryMessage.expiresAtMs = 0;
    temporaryMessage.message[0] = '\0';
}

bool temporaryMessageExpired() {
    return temporaryMessage.active &&
           static_cast<long>(millis() - temporaryMessage.expiresAtMs) >= 0;
}

bool temporaryMessageVisible() {
    return temporaryMessage.active &&
           temporaryMessage.message[0] != '\0' &&
           !temporaryMessageExpired();
}

Rgb888 rgb565ToRgb888(uint16_t color) {
    Rgb888 rgb;
    rgb.red = static_cast<uint8_t>(((color >> 11) & 0x1F) * 255 / 31);
    rgb.green = static_cast<uint8_t>(((color >> 5) & 0x3F) * 255 / 63);
    rgb.blue = static_cast<uint8_t>((color & 0x1F) * 255 / 31);
    return rgb;
}

uint16_t rgb888ToRgb565(const Rgb888 &rgb) {
    return rgb565(rgb.red, rgb.green, rgb.blue);
}

bool parseHexColor888(const char *value, Rgb888 &rgb) {
    if (value == nullptr) {
        return false;
    }

    const char *source = value[0] == '#' ? value + 1 : value;
    if (strlen(source) != 6) {
        return false;
    }

    char *end = nullptr;
    unsigned long raw = strtoul(source, &end, 16);
    if (end == nullptr || *end != '\0') {
        return false;
    }

    rgb.red = static_cast<uint8_t>((raw >> 16) & 0xFF);
    rgb.green = static_cast<uint8_t>((raw >> 8) & 0xFF);
    rgb.blue = static_cast<uint8_t>(raw & 0xFF);
    return true;
}

Rgb888 blendRgb(const Rgb888 &base, const Rgb888 &mix, uint8_t mixWeightPercent) {
    uint16_t baseWeight = 100U - constrain(mixWeightPercent, 0, 100);
    uint16_t mixWeight = constrain(mixWeightPercent, 0, 100);

    Rgb888 result;
    result.red = static_cast<uint8_t>((base.red * baseWeight + mix.red * mixWeight) / 100U);
    result.green = static_cast<uint8_t>((base.green * baseWeight + mix.green * mixWeight) / 100U);
    result.blue = static_cast<uint8_t>((base.blue * baseWeight + mix.blue * mixWeight) / 100U);
    return result;
}

ThemeSelection effectiveThemeSelection() {
    if (dashboardNightModeActive() && dashboardConfig.nightThemeEnabled) {
        return {
            dashboardConfig.nightTheme,
            dashboardConfig.nightCustomThemeEnabled,
            dashboardConfig.nightCustomBackground,
            dashboardConfig.nightCustomSurface,
            dashboardConfig.nightCustomAccent,
            dashboardConfig.nightCustomText
        };
    }

    return {
        dashboardConfig.theme,
        dashboardConfig.customThemeEnabled,
        dashboardConfig.customBackground,
        dashboardConfig.customSurface,
        dashboardConfig.customAccent,
        dashboardConfig.customText
    };
}

uint8_t activeThemeIdValue() {
    ThemeSelection selection = effectiveThemeSelection();
    return constrain(selection.theme, 0, DASHBOARD_THEME_COUNT - 1);
}

float themeLuminance(uint16_t color) {
    Rgb888 rgb = rgb565ToRgb888(color);
    const uint8_t channels[] = {rgb.red, rgb.green, rgb.blue};
    const float weights[] = {0.2126f, 0.7152f, 0.0722f};
    float luminance = 0;
    for (uint8_t index = 0; index < 3; ++index) {
        float value = channels[index] / 255.0f;
        luminance += weights[index] * (value <= 0.04045f ? value / 12.92f : powf((value + 0.055f) / 1.055f, 2.4f));
    }
    return luminance;
}

uint16_t readableDerivedColor(uint16_t color, uint16_t background) {
    float backgroundLuminance = themeLuminance(background);
    Rgb888 original = rgb565ToRgb888(color);
    // Pick the endpoint with the strongest contrast, then retain as much hue as possible.
    Rgb888 endpoint = backgroundLuminance < 0.179f ? Rgb888{255, 255, 255} : Rgb888{0, 0, 0};
    for (uint8_t weight = 0; weight <= 100; weight += 4) {
        uint16_t candidate = rgb888ToRgb565(blendRgb(original, endpoint, weight));
        float luminance = themeLuminance(candidate);
        float contrast = (max(luminance, backgroundLuminance) + 0.05f) / (min(luminance, backgroundLuminance) + 0.05f);
        if (contrast >= 4.5f) return candidate;
    }
    return rgb888ToRgb565(endpoint);
}

ThemePalette buildCustomTheme(const ThemePalette &base,
                              const char *backgroundColor,
                              const char *surfaceColor,
                              const char *accentColor,
                              const char *textColor) {
    Rgb888 background = rgb565ToRgb888(base.background);
    Rgb888 surface = rgb565ToRgb888(base.surface);
    Rgb888 accent = rgb565ToRgb888(base.accent);
    Rgb888 text = rgb565ToRgb888(base.text);

    parseHexColor888(backgroundColor, background);
    parseHexColor888(surfaceColor, surface);
    parseHexColor888(accentColor, accent);
    parseHexColor888(textColor, text);

    ThemePalette theme = {};
    theme.background = rgb888ToRgb565(background);
    theme.surface = rgb888ToRgb565(surface);
    theme.surfaceAlt = rgb888ToRgb565(blendRgb(surface, background, 22));
    theme.accent = rgb888ToRgb565(accent);
    theme.accentSoft = rgb888ToRgb565(blendRgb(surface, accent, 28));
    theme.text = rgb888ToRgb565(text);
    theme.muted = rgb888ToRgb565(blendRgb(text, background, 52));
    theme.muted = readableDerivedColor(theme.muted, theme.background);
    theme.positive = readableDerivedColor(base.positive, theme.background);
    theme.negative = readableDerivedColor(base.negative, theme.background);
    theme.warning = readableDerivedColor(base.warning, theme.background);
    return theme;
}

void hashThemeConfig(uint32_t &hash) {
    ThemeSelection selection = effectiveThemeSelection();
    hashValue(hash, selection.theme);
    hashValue(hash, selection.customThemeEnabled);
    if (!selection.customThemeEnabled) {
        return;
    }

    hashCString(hash, selection.background);
    hashCString(hash, selection.surface);
    hashCString(hash, selection.accent);
    hashCString(hash, selection.text);
}



const ThemePalette& activeTheme() {
    static ThemePalette customTheme;
    static uint32_t customThemeHash = 0;
    static bool customThemeValid = false;
    ThemeSelection selection = effectiveThemeSelection();
    uint8_t themeIndex = constrain(selection.theme, 0, DASHBOARD_THEME_COUNT - 1);
    if (!selection.customThemeEnabled) {
        return kThemes[themeIndex];
    }

    uint32_t hash = kFnvOffset;
    hashThemeConfig(hash);
    if (customThemeValid && hash == customThemeHash) return customTheme;

    customTheme = buildCustomTheme(kThemes[themeIndex],
                                   selection.background,
                                   selection.surface,
                                   selection.accent,
                                   selection.text);
    customThemeHash = hash;
    customThemeValid = true;
    return customTheme;
}

String safeCString(const char *value) {
    return (value != nullptr) ? String(value) : String("");
}

bool hasTimeSync() {
    return dashboardCurrentEpoch() != 0;
}

void formatLocalClock(char *buffer, size_t bufferSize, bool showSeconds, bool use24Hour) {
    if (bufferSize == 0) {
        return;
    }
    if (!hasTimeSync()) {
        strncpy(buffer, showSeconds ? "--:--:--" : "--:--", bufferSize - 1);
        buffer[bufferSize - 1] = '\0';
        return;
    }

    time_t now = time(nullptr);
    tm localTimeInfo;
    localtime_r(&now, &localTimeInfo);
    const char *format = use24Hour
                             ? (showSeconds ? "%H:%M:%S" : "%H:%M")
                             : (showSeconds ? "%I:%M:%S" : "%I:%M");
    if (strftime(buffer, bufferSize, format, &localTimeInfo) == 0) {
        strncpy(buffer, showSeconds ? "--:--:--" : "--:--", bufferSize - 1);
        buffer[bufferSize - 1] = '\0';
    }
}

String formatTimeForTm(const tm &timeInfo, bool showSeconds, bool use24Hour, bool *isPm = nullptr) {
    char buffer[16];
    const char *format = nullptr;

    if (use24Hour) {
        format = showSeconds ? "%H:%M:%S" : "%H:%M";
    } else {
        format = showSeconds ? "%I:%M:%S" : "%I:%M";
        if (isPm != nullptr) {
            *isPm = timeInfo.tm_hour >= 12;
        }
    }

    strftime(buffer, sizeof(buffer), format, &timeInfo);
    return String(buffer);
}

String formatLocalTime(bool showSeconds, bool use24Hour, bool *isPm = nullptr) {
    if (!hasTimeSync()) {
        if (isPm != nullptr) {
            *isPm = false;
        }
        return showSeconds ? "--:--:--" : "--:--";
    }

    time_t now = time(nullptr);
    tm localTimeInfo;
    localtime_r(&now, &localTimeInfo);
    return formatTimeForTm(localTimeInfo, showSeconds, use24Hour, isPm);
}

String formatLocalDate() {
    if (!hasTimeSync()) {
        return "Waiting for time";
    }

    char buffer[32];
    time_t now = time(nullptr);
    tm localTimeInfo;
    localtime_r(&now, &localTimeInfo);
    strftime(buffer, sizeof(buffer), "%a %d %b %Y", &localTimeInfo);
    return String(buffer);
}

String formatClockMetaLine(bool isPm) {
    String metaLine = formatLocalDate();
    if (!dashboardConfig.use24Hour && hasTimeSync()) {
        metaLine += isPm ? "  PM" : "  AM";
    }
    return metaLine;
}

void updateClockMetaCache(const String &metaLine) {
    strncpy(clockMetaCache.metaLine, metaLine.c_str(), sizeof(clockMetaCache.metaLine) - 1);
    clockMetaCache.metaLine[sizeof(clockMetaCache.metaLine) - 1] = '\0';
    clockMetaCache.valid = true;
}

bool fontSupportsText(int font, const String &text) {
    if (font != FONT_TITLE && font != FONT_HUGE) {
        return true;
    }
    for (size_t index = 0; index < text.length(); ++index) {
        char character = text[index];
        if ((character >= '0' && character <= '9') ||
            character == ' ' || character == ':' || character == '.' || character == '-') {
            continue;
        }
        if (font == FONT_TITLE &&
            (character == 'a' || character == 'p' || character == 'm' || character == '|')) {
            continue;
        }
        return false;
    }
    return true;
}

template <typename TCanvas>
int chooseCanvasFittingFont(TCanvas &canvas,
                            const String &text,
                            int maxWidth,
                            const int *fontCandidates,
                            size_t fontCandidateCount,
                            int maxHeight = DISPLAY_HEIGHT) {
    if (fontCandidates == nullptr || fontCandidateCount == 0) {
        return FONT_INFO;
    }

    for (size_t index = 0; index < fontCandidateCount; ++index) {
        int font = fontCandidates[index];
        if (fontSupportsText(font, text) && canvas.textWidth(text, font) <= maxWidth &&
            canvas.fontHeight(font) <= maxHeight) {
            return font;
        }
    }

    return FONT_INFO;
}

template <typename TCanvas>
void drawClockTime(TCanvas &canvas,
                   const String &timeText,
                   bool showSeconds,
                   uint16_t primaryColor,
                   uint16_t secondaryColor,
                   uint16_t backgroundColor,
                   int centerX,
                   int topY) {
    (void)secondaryColor;
    const int timeFonts[] = {FONT_HUGE, FONT_TITLE, FONT_BODY};
    int maxWidth = max(0, (centerX * 2) - (showSeconds ? 8 : 20));
    int timeFont = chooseCanvasFittingFont(canvas,
                                           timeText,
                                           maxWidth,
                                           timeFonts,
                                           sizeof(timeFonts) / sizeof(timeFonts[0]));

    canvas.setTextDatum(TL_DATUM);
    canvas.setTextFont(FONT_HUGE);
    int referenceHeight = canvas.fontHeight();

    canvas.setTextColor(primaryColor, backgroundColor);
    canvas.setTextFont(timeFont);
    int textWidth = canvas.textWidth(timeText, timeFont);
    int textHeight = canvas.fontHeight();
    int adjustedTopY = topY + max(0, (referenceHeight - textHeight) / 2);
    int startX = centerX - (textWidth / 2);
    canvas.drawString(timeText, startX, adjustedTopY, timeFont);
}

String headerSubtitle() {
    if (displayState.apMode && displayState.ipInfo[0] != '\0') {
        return safeCString(displayState.ipInfo);
    }

    if (!dashboardConfig.showIp) {
        return "";
    }

    if (displayState.ipInfo[0] != '\0') {
        return safeCString(displayState.ipInfo);
    }

    return "";
}

std::vector<String> wrapText(const String &text, int font, int maxWidth) {
    std::vector<String> lines;

    if (text.isEmpty()) {
        lines.push_back("");
        return lines;
    }

    tft.setTextFont(font);
    String currentLine;
    String currentWord;
    currentLine.reserve(text.length() + 4);
    currentWord.reserve(text.length() + 4);

    auto flushWord = [&]() {
        if (currentWord.isEmpty()) {
            return;
        }

        String candidate = currentLine;
        if (!candidate.isEmpty()) {
            candidate += " ";
        }
        candidate += currentWord;

        if (tft.textWidth(candidate) > maxWidth) {
            if (!currentLine.isEmpty()) {
                lines.push_back(currentLine);
            }
            currentLine = "";
            for (size_t index = 0; index < currentWord.length(); ++index) {
                char character = currentWord.charAt(index);
                if (!currentLine.isEmpty() && tft.textWidth(currentLine + character) > maxWidth) {
                    lines.push_back(currentLine);
                    currentLine = "";
                }
                currentLine += character;
            }
        } else {
            currentLine = candidate;
        }

        currentWord = "";
    };

    for (size_t index = 0; index < text.length(); ++index) {
        char character = text.charAt(index);

        if (character == '\n') {
            flushWord();
            lines.push_back(currentLine);
            currentLine = "";
        } else if (character == ' ') {
            flushWord();
        } else {
            currentWord += character;
        }
    }

    flushWord();
    if (!currentLine.isEmpty()) {
        lines.push_back(currentLine);
    }

    if (lines.empty()) {
        lines.push_back("");
    }

    return lines;
}

void drawRoundedPanel(int x, int y, int width, int height, uint16_t fillColor, uint16_t borderColor) {
    tft.fillRoundRect(x, y, width, height, 18, fillColor);
    tft.drawRoundRect(x, y, width, height, 18, borderColor);
}

void drawBadge(int x, int y, int width, int height, const String &label, uint16_t fillColor, uint16_t textColor) {
    tft.fillRoundRect(x, y, width, height, height / 2, fillColor);
    tft.setTextDatum(MC_DATUM);
    tft.setTextFont(FONT_LABEL);
    tft.setTextColor(textColor, fillColor);
    tft.drawString(label, x + (width / 2), y + (height / 2), FONT_LABEL);
}

void drawWrappedCenteredText(const String &text,
                             int centerX,
                             int startY,
                             int maxWidth,
                             int font,
                             uint16_t textColor,
                             uint16_t backgroundColor,
                             int lineSpacing = 4,
                             int maxHeight = DISPLAY_HEIGHT) {
    std::vector<String> lines = wrapText(text, font, maxWidth);
    tft.setTextDatum(TC_DATUM);
    tft.setTextFont(font);
    tft.setTextColor(textColor, backgroundColor);
    int fontHeight = tft.fontHeight();
    int lineHeight = fontHeight + lineSpacing;
    int availableHeight = min(maxHeight, DISPLAY_HEIGHT - startY);
    int lineCount = availableHeight < fontHeight ? 0 : 1 + (availableHeight - fontHeight) / lineHeight;
    size_t visibleLines = min(lines.size(), static_cast<size_t>(lineCount));
    if (visibleLines < lines.size() && visibleLines > 0) {
        String &lastLine = lines[visibleLines - 1];
        while (!lastLine.isEmpty() && tft.textWidth(lastLine + "...") > maxWidth) {
            lastLine.remove(lastLine.length() - 1);
        }
        lastLine += "...";
    }

    for (size_t lineIndex = 0; lineIndex < visibleLines; ++lineIndex) {
        tft.drawString(lines[lineIndex], centerX, startY + (lineIndex * lineHeight), font);
    }
}

int chooseFittingFont(const String &text, int maxWidth, const int *fontCandidates, size_t fontCandidateCount,
                      int maxHeight = DISPLAY_HEIGHT) {
    return chooseCanvasFittingFont(tft, text, maxWidth, fontCandidates, fontCandidateCount, maxHeight);
}

void drawAdaptiveText(const String &text,
                      int x,
                      int y,
                      int maxWidth,
                      uint8_t datum,
                      const int *fontCandidates,
                      size_t fontCandidateCount,
                      uint16_t textColor,
                      uint16_t backgroundColor) {
    int font = chooseFittingFont(text, maxWidth, fontCandidates, fontCandidateCount);
    tft.setTextDatum(datum);
    tft.setTextFont(font);
    tft.setTextColor(textColor, backgroundColor);
    tft.drawString(text, x, y, font);
}

String fitDisplayLabel(const String &text, int width, int font = FONT_LABEL) {
    String result = text;
    if (tft.textWidth(result, font) <= width) return result;
    while (!result.isEmpty() && tft.textWidth(result + "...", font) > width) {
        result.remove(result.length() - 1);
    }
    return result + "...";
}

void drawDisplayLabel(const String &text, int x, int y, int width, uint8_t datum,
                      uint16_t color, int font = FONT_LABEL) {
    tft.setTextDatum(datum);
    tft.setTextFont(font);
    tft.setTextColor(color, activeTheme().background);
    tft.drawString(fitDisplayLabel(text, width, font), x, y, font);
}

void drawHeroText(const String &text, int topY, int height, uint16_t color,
                  uint8_t maximumSize = 2) {
    int size = maximumSize;
    while (size > 1) {
        tft.setTextSize(size);
        if (tft.textWidth(text, FONT_BODY) <= 224 && tft.fontHeight(FONT_BODY) <= height) break;
        --size;
    }
    tft.setTextSize(size);
    String visible = fitDisplayLabel(text, 224, FONT_BODY);
    tft.setTextDatum(TC_DATUM);
    tft.setTextFont(FONT_BODY);
    tft.setTextColor(color, activeTheme().background);
    tft.drawString(visible, 120, topY + max(0, (height - tft.fontHeight()) / 2), FONT_BODY);
    tft.setTextSize(1);
}

String footerClockText() {
    bool isPm = false;
    String text = formatLocalTime(false, dashboardConfig.use24Hour, &isPm);
    if (!dashboardConfig.use24Hour && hasTimeSync()) text += isPm ? " PM" : " AM";
    return text;
}

void updateFooterClock() {
    if ((displayState.currentPage == DASHBOARD_PAGE_CLOCK || displayState.currentPage == DASHBOARD_PAGE_NIGHTSCOUT) &&
        !temporaryMessageVisible()) return;
    String text = footerClockText();
    if (strcmp(footerClockCache, text.c_str()) == 0) return;
    const ThemePalette &theme = activeTheme();
    tft.setTextSize(1);
    const int fonts[] = {FONT_BODY, FONT_LABEL};
    int font = chooseFittingFont(text, 84, fonts, 2, 26);
    tft.fillRect(8, 212, 84, 28, theme.background);
    int y = 212 + max(0, (26 - tft.fontHeight(font)) / 2);
    drawDisplayLabel(text, 8, y, 84, TL_DATUM, theme.text, font);
    strncpy(footerClockCache, text.c_str(), sizeof(footerClockCache) - 1);
    footerClockCache[sizeof(footerClockCache) - 1] = '\0';
}

void drawScreenChrome(const String &title) {
    const ThemePalette &theme = activeTheme();
    tft.setTextSize(1);
    tft.fillScreen(theme.background);
    footerClockCache[0] = '\0';
    if (displayState.currentPage == DASHBOARD_PAGE_CLOCK && !temporaryMessageVisible()) {
        drawDisplayLabel(title, 8, 220, 84, TL_DATUM, theme.accent);
    } else {
        updateFooterClock();
    }
    String address = headerSubtitle();
    if (!address.isEmpty()) {
        drawDisplayLabel(address, 232, 220, 132, TR_DATUM, theme.muted);
    }
}

void drawPlaceholder(const String &title, const String &message) {
    const ThemePalette &theme = activeTheme();
    drawScreenChrome(title);
    drawWrappedCenteredText("Nothing here yet", 120, 48, 224, FONT_BODY,
                            theme.text, theme.background, 4, 60);
    drawWrappedCenteredText(message, 120, 126, 224, FONT_LABEL,
                            theme.muted, theme.background, 4, 70);
}

const WeatherData* effectiveWeatherData() {
    return feedsWeatherSource() == WEATHER_FEED_OPEN_METEO ? feedsWeatherData() : nullptr;
}

bool weatherWaitingForSync() {
    return feedsWeatherSource() == WEATHER_FEED_OPEN_METEO &&
           feedsWeatherConfigured() &&
           !feedsHasWeatherData();
}

char weatherUnitSymbol() {
    return feedsWeatherUsesFahrenheit() ? 'F' : 'C';
}

String formatWeatherTemperature(int value) {
    return String(value) + weatherUnitSymbol();
}

bool hasWeatherContent() {
    const WeatherData *weather = effectiveWeatherData();
    return weather != nullptr &&
           (weather->location[0] != '\0' ||
            weather->condition[0] != '\0' ||
            feedsHasWeatherData());
}

const char* activeClockMessage() {
    if (displayState.line2[0] == '\0') {
        return nullptr;
    }

    if (strcmp(displayState.line2, "Dashboard preview ready") == 0) {
        return nullptr;
    }

    return displayState.line2;
}

bool dashboardPageHasRenderableContent(uint8_t pageId) {
    switch (pageId) {
        case DASHBOARD_PAGE_CLOCK:
            return true;
        case DASHBOARD_PAGE_NIGHTSCOUT:
            return true;
        case DASHBOARD_PAGE_WEATHER:
            return hasWeatherContent() || weatherWaitingForSync();
        default:
            return false;
    }
}

bool dashboardPageAvailable(uint8_t pageId) {
    return dashboardPageEnabled(pageId) && dashboardPageHasRenderableContent(pageId);
}

uint8_t firstAvailableDashboardPage() {
    for (uint8_t pageId = 0; pageId < DASHBOARD_PAGE_COUNT; ++pageId) {
        if (dashboardPageAvailable(pageId)) {
            return pageId;
        }
    }

    return dashboardFirstEnabledPage();
}

uint8_t nextAvailableDashboardPage(uint8_t currentPage) {
    for (uint8_t offset = 1; offset <= DASHBOARD_PAGE_COUNT; ++offset) {
        uint8_t pageId = (currentPage + offset) % DASHBOARD_PAGE_COUNT;
        if (dashboardPageAvailable(pageId)) {
            return pageId;
        }
    }

    if (dashboardPageEnabled(currentPage)) {
        return currentPage;
    }

    return dashboardFirstEnabledPage();
}

void hashWeatherData(uint32_t &hash) {
    hashValue(hash, feedsWeatherSource());
    hashValue(hash, feedsWeatherUsesFahrenheit());
    hashValue(hash, weatherWaitingForSync());

    const WeatherData *weather = effectiveWeatherData();
    bool hasData = weather != nullptr;
    hashValue(hash, hasData);
    if (!hasData) {
        return;
    }

    hashCString(hash, weather->location);
    hashCString(hash, weather->condition);
    hashValue(hash, weather->temperature);
    hashValue(hash, weather->high);
    hashValue(hash, weather->low);
    hashValue(hash, weather->rainChance);
}

int nightscoutAgeMinutes(const NightscoutRuntime &ns) {
    uint32_t nowEpoch = dashboardCurrentEpoch();
    if (ns.readingEpoch != 0 && nowEpoch != 0) {
        return nowEpoch > ns.readingEpoch ? static_cast<int>((nowEpoch - ns.readingEpoch) / 60UL) : 0;
    }
    return static_cast<int>((millis() - ns.lastSuccessMs) / 60000UL);
}

const char *const kWaitingPuns[] = {
    "Sweetening the data...",
    "Pinging pancreas...",
    "Blaming pizza...",
    "Sniffing carbs...",
    "Counting cubes...",
};

int waitingPunIndex() {
    constexpr int count = static_cast<int>(sizeof(kWaitingPuns) / sizeof(kWaitingPuns[0]));
    static int current = 0;
    static uint32_t slot = 0;
    static bool started = false;
    const uint32_t nowSlot = millis() / 12000UL;
    if (!started || nowSlot != slot) {
        int next = random(count);
        if (started && next == current) next = (next + 1) % count;
        current = next;
        slot = nowSlot;
        started = true;
    }
    return current;
}

void hashNightscoutData(uint32_t &hash) {
    const NightscoutRuntime &ns = feedRuntime.nightscout;
    const NightscoutConfig &cfg = feedConfig.nightscout;
    hashValue(hash, feedsNightscoutConfigured());
    hashValue(hash, ns.hasData);
    hashValue(hash, ns.sgv);
    hashCString(hash, ns.direction);
    hashCString(hash, ns.lastError);
    hashValue(hash, cfg.useMmol);
    hashValue(hash, cfg.urgentLowMgdl);
    hashValue(hash, cfg.lowMgdl);
    hashValue(hash, cfg.highMgdl);
    hashValue(hash, cfg.urgentHighMgdl);
    hashValue(hash, cfg.showIob);
    hashValue(hash, ns.iobValid);
    hashValue(hash, static_cast<int>(lroundf(ns.iob * 100.0f)));
    hashValue(hash, ns.historyCount);
    for (uint8_t i = 0; i < ns.historyCount; ++i) hashValue(hash, ns.history[i]);
    if (ns.hasData) {
        hashValue(hash, nightscoutAgeMinutes(ns));
        hashValue(hash, dashboardConfig.showSeconds);
        hashValue(hash, dashboardConfig.showIp);
        char clockText[16];
        formatLocalClock(clockText, sizeof(clockText), dashboardConfig.showSeconds, dashboardConfig.use24Hour);
        hashCString(hash, clockText);
        if (dashboardConfig.showIp) {
            hashCString(hash, displayState.ipInfo);
        }
    } else if (ns.lastError[0] == '\0' && feedsNightscoutConfigured()) {
        hashValue(hash, waitingPunIndex());
    }
}

void hashCommonPageState(uint32_t &hash, uint8_t pageId) {
    hashValue(hash, pageId);
    hashThemeConfig(hash);
    hashValue(hash, dashboardConfig.showIp);
    hashCString(hash, displayState.ipInfo);
}

uint32_t dashboardStaticHash(uint8_t pageId) {
    uint32_t hash = kFnvOffset;
    hashCommonPageState(hash, pageId);

    switch (pageId) {
        case DASHBOARD_PAGE_CLOCK:
            hashValue(hash, dashboardConfig.use24Hour);
            hashValue(hash, dashboardConfig.showSeconds);
            hashValue(hash, hasWeatherContent());
            hashValue(hash, weatherWaitingForSync());
            hashValue(hash, activeClockMessage() != nullptr);
            if (hasWeatherContent()) {
                hashWeatherData(hash);
            } else if (activeClockMessage() != nullptr) {
                hashCString(hash, activeClockMessage());
            }
            break;
        case DASHBOARD_PAGE_NIGHTSCOUT:
            hashNightscoutData(hash);
            break;
        case DASHBOARD_PAGE_WEATHER:
            hashWeatherData(hash);
            break;
        default:
            break;
    }

    return hash;
}

uint32_t dashboardDynamicHash(uint8_t pageId) {
    uint32_t hash = kFnvOffset;
    hashValue(hash, pageId);

    switch (pageId) {
        case DASHBOARD_PAGE_CLOCK: {
            bool isPm = false;
            String timeText = formatLocalTime(dashboardConfig.showSeconds, dashboardConfig.use24Hour, &isPm);
            String metaLine = formatClockMetaLine(isPm);
            hashCString(hash, timeText.c_str());
            hashCString(hash, metaLine.c_str());
            hashValue(hash, isPm);
            break;
        }
        default:
            break;
    }

    return hash;
}

uint32_t apScreenHash() {
    uint32_t hash = kFnvOffset;
    hashThemeConfig(hash);
    hashCString(hash, displayState.ipInfo);
    hashCString(hash, displayState.apSSID);
    hashCString(hash, displayState.apPassword);
    hashValue(hash, authCanRevealPassword());
    hashCString(hash, authProvisionedPassword());
    return hash;
}

uint32_t temporaryMessageHash() {
    uint32_t hash = kFnvOffset;
    hashThemeConfig(hash);
    hashValue(hash, displayState.apMode);
    hashValue(hash, dashboardConfig.showIp);
    hashCString(hash, displayState.ipInfo);
    hashCString(hash, temporaryMessage.message);
    return hash;
}

bool pageUsesDynamicRefresh(uint8_t pageId) {
    return pageId == DASHBOARD_PAGE_CLOCK;
}

bool ensureClockDynamicSprite() {
    if (!clockDynamicSpriteAllowed) {
        return false;
    }

    if (clockDynamicSpriteReady) {
        return true;
    }

    uint32_t now = millis();
    if (clockDynamicSpriteAttempted &&
        now - clockDynamicSpriteLastAttemptMs < kClockSpriteRetryIntervalMs) {
        return false;
    }

    clockDynamicSpriteAttempted = true;
    clockDynamicSpriteLastAttemptMs = now;
    uint32_t freeHeap = ESP.getFreeHeap();
    if (freeHeap < kClockSpriteMinFreeHeapBytes) {
        logPrintf(F("Clock sprite skipped, free heap too low: %u"), freeHeap);
        return false;
    }

    clockDynamicSprite.deleteSprite();
    clockDynamicSprite.setColorDepth(1);
    clockDynamicSpriteReady =
        clockDynamicSprite.createSprite(kClockDynamicRegionWidth, kClockDynamicRegionHeight) != nullptr;

    if (clockDynamicSpriteReady) {
        logPrintf(F("Clock sprite ready (%dx%d), free heap now %u"),
                  kClockDynamicRegionWidth,
                  kClockDynamicRegionHeight,
                  ESP.getFreeHeap());
    } else {
        logPrintf(F("Clock sprite allocation failed, free heap was %u"), freeHeap);
    }

    return clockDynamicSpriteReady;
}

void updateClockDynamicArea() {
    const ThemePalette &theme = activeTheme();
    bool isPm = false;
    String timeText = formatLocalTime(dashboardConfig.showSeconds, dashboardConfig.use24Hour, &isPm);
    String metaLine = formatClockMetaLine(isPm);
    uint16_t dynamicBackground = theme.background;
    int localTimeTopY = kClockTimeY - kClockDynamicRegionY;

    if (ensureClockDynamicSprite()) {
        clockDynamicSprite.setBitmapColor(theme.text, dynamicBackground);
        clockDynamicSprite.fillSprite(0);
        drawClockTime(clockDynamicSprite,
                      timeText,
                      dashboardConfig.showSeconds,
                      1,
                      1,
                      0,
                      kClockDynamicRegionWidth / 2,
                      localTimeTopY);
        clockDynamicSprite.pushSprite(kClockDynamicRegionX, kClockDynamicRegionY);
    } else {
        tft.fillRect(kClockDynamicRegionX,
                     kClockDynamicRegionY,
                     kClockDynamicRegionWidth,
                     kClockDynamicRegionHeight,
                     dynamicBackground);
        drawClockTime(tft,
                      timeText,
                      dashboardConfig.showSeconds,
                      theme.text,
                      theme.muted,
                      dynamicBackground,
                      120,
                      kClockTimeY);
    }

    if (!clockMetaCache.valid || strcmp(clockMetaCache.metaLine, metaLine.c_str()) != 0) {
        tft.setTextFont(FONT_LABEL);
        int metaWidth = tft.textWidth(metaLine) + 12;
        if (clockMetaCache.valid) {
            metaWidth = max(metaWidth, tft.textWidth(clockMetaCache.metaLine) + 12);
        }

        tft.setTextDatum(TC_DATUM);
        tft.setTextColor(theme.muted, dynamicBackground);
        tft.setTextPadding(metaWidth);
        tft.drawString(metaLine, 120, kClockMetaY, FONT_LABEL);
        tft.setTextPadding(0);
        updateClockMetaCache(metaLine);
    }
}

void renderClockPage() {
    const ThemePalette &theme = activeTheme();
    drawScreenChrome("Clock");
    clockMetaCache.valid = false;
    updateClockDynamicArea();
    const char *message = activeClockMessage();
    if (hasWeatherContent()) {
        const WeatherData *weather = effectiveWeatherData();
        drawHeroText(formatWeatherTemperature(weather->temperature), 132, 58, theme.accent);
        drawDisplayLabel(weather->condition[0] != '\0' ? weather->condition : "Weather",
                         120, 194, 224, TC_DATUM, theme.text);
    } else if (weatherWaitingForSync()) {
        drawDisplayLabel("Syncing weather...", 120, 166, 224, TC_DATUM, theme.muted);
    } else if (message != nullptr) {
        drawWrappedCenteredText(message, 120, 136, 224, FONT_BODY,
                                theme.text, theme.background, 4, 68);
    }
}

void renderWeatherPage() {
    const ThemePalette &theme = activeTheme();
    if (!hasWeatherContent()) {
        drawPlaceholder("Weather", weatherWaitingForSync() ? "Syncing live data." : "Add weather data.");
        return;
    }
    const WeatherData *weather = effectiveWeatherData();
    drawScreenChrome("Weather");
    drawDisplayLabel(weather->location[0] != '\0' ? weather->location : "Weather",
                     120, 8, 224, TC_DATUM, theme.text);
    drawHeroText(formatWeatherTemperature(weather->temperature), 42, 96, theme.text, 3);
    drawDisplayLabel(weather->condition[0] != '\0' ? weather->condition : "Updated",
                     120, 144, 224, TC_DATUM, theme.accent);
    const int valueFonts[] = {FONT_BODY, FONT_LABEL};
    const String values[] = {formatWeatherTemperature(weather->high),
                             formatWeatherTemperature(weather->low), String(weather->rainChance) + "%"};
    const char *labels[] = {"High", "Low", "Rain"};
    for (uint8_t index = 0; index < 3; ++index) {
        int centerX = 40 + index * 80;
        drawDisplayLabel(labels[index], centerX, 166, 72, TC_DATUM, theme.muted);
        drawAdaptiveText(values[index], centerX, 184, 72, TC_DATUM,
                         valueFonts, sizeof(valueFonts) / sizeof(valueFonts[0]), theme.text, theme.background);
    }
}

// ---------------------------------------------------------------------------
// Glucose page. Visitor TT1 square-pixel glyphs from pixelfont.h.
// ---------------------------------------------------------------------------
constexpr uint16_t kNsGreen = 0x47E8;   // neon green
constexpr uint16_t kNsCyan = 0x07FF;
constexpr uint16_t kNsOrange = 0xFC60;
constexpr uint16_t kNsRed = 0xF800;
constexpr uint16_t kNsWhite = 0xFFFF;
constexpr uint16_t kNsGrey = 0x7BEF;
constexpr bool kNsColourByRange = true;  // false = value always neon green

int pixelGlyphWidth(char c) {
    if (c < 32 || c > 126) c = '?';
    return pgm_read_byte(&PIXEL_GLYPH_WIDTH[static_cast<uint8_t>(c) - 32]);
}

int pixelTextWidth(const char *text, int scale) {
    int pixels = 0;
    if (text == nullptr) return 0;
    for (const char *cursor = text; *cursor != '\0'; ++cursor) {
        pixels += pixelGlyphWidth(*cursor);
    }
    return pixels * scale;
}

void drawPixelChar(int x, int y, char c, int scale, uint16_t color, int firstRow, int rowCount) {
    if (c < 32 || c > 126) c = '?';
    const int index = static_cast<uint8_t>(c) - 32;
    const int width = pgm_read_byte(&PIXEL_GLYPH_WIDTH[index]);
    const int lastRow = firstRow + rowCount < PIXEL_FONT_HEIGHT
                            ? firstRow + rowCount
                            : PIXEL_FONT_HEIGHT;
    for (int row = firstRow; row < lastRow; ++row) {
        const uint8_t bits = pgm_read_byte(&PIXEL_FONT_BITMAP[index][row]);
        int col = 0;
        while (col < width) {
            if (bits & (0x80 >> col)) {
                const int start = col;
                while (col < width && (bits & (0x80 >> col))) ++col;
                tft.fillRect(x + start * scale,
                             y + (row - firstRow) * scale,
                             (col - start) * scale,
                             scale,
                             color);
            } else {
                ++col;
            }
        }
    }
}

// align: 0 = left edge at x, 1 = centred on x, 2 = right edge at x
void drawPixelText(int x, int y, const char *text, int scale, uint16_t color, int align = 0,
                   int firstRow = 0, int rowCount = PIXEL_FONT_HEIGHT) {
    if (text == nullptr) return;
    int left = x;
    const int width = pixelTextWidth(text, scale);
    if (align == 1) left = x - width / 2;
    else if (align == 2) left = x - width;
    for (const char *cursor = text; *cursor != '\0'; ++cursor) {
        drawPixelChar(left, y, *cursor, scale, color, firstRow, rowCount);
        left += pixelGlyphWidth(*cursor) * scale;
    }
}

// Classic open arrowheads. Scaled so the whole arrow is as tall as the glucose digits.
constexpr int kArrowCols = 9;
constexpr int kArrowRows = 9;
const char *const kArrowEast[kArrowRows] = {
    ".........", ".....#...", "......#..", ".......#.", "#########",
    ".......#.", "......#..", ".....#...", "........."};
const char *const kArrowSouth[kArrowRows] = {
    "....#....", "....#....", "....#....", "....#....", "....#....",
    ".#..#..#.", "..#.#.#..", "...###...", "....#...."};
const char *const kArrowNorth[kArrowRows] = {
    "....#....", "...###...", "..#.#.#..", ".#..#..#.", "....#....",
    "....#....", "....#....", "....#....", "....#...."};
const char *const kArrowSouthEast[kArrowRows] = {
    "#........", ".#.......", "..#......", "...#....#", "....#...#",
    ".....#..#", "......#.#", ".......##", "...######"};
const char *const kArrowNorthEast[kArrowRows] = {
    "...######", ".......##", "......#.#", ".....#..#", "....#...#",
    "...#....#", "..#......", ".#.......", "#........"};

int arrowPixelSize(int scale) {
    // Ink of the flat arrow is 7 rows inside the 9-row cell. One step under the digits.
    int pixel = (PIXEL_FONT_DIGIT_HEIGHT * scale) / 7;
    if (pixel > 2) --pixel;
    return pixel > 0 ? pixel : 1;
}

void drawArrowBitmap(const char *const *art, int x, int y, int pixel, uint16_t color) {
    for (int row = 0; row < kArrowRows; ++row)
        for (int col = 0; col < kArrowCols; ++col)
            if (art[row][col] == '#') tft.fillRect(x + col * pixel, y + row * pixel, pixel, pixel, color);
}

const char *const *nightscoutTrendArt(const char *direction, bool *twin) {
    *twin = false;
    if (strcmp(direction, "Flat") == 0) return kArrowEast;
    if (strcmp(direction, "FortyFiveUp") == 0) return kArrowNorthEast;
    if (strcmp(direction, "FortyFiveDown") == 0) return kArrowSouthEast;
    if (strcmp(direction, "SingleUp") == 0) return kArrowNorth;
    if (strcmp(direction, "SingleDown") == 0) return kArrowSouth;
    if (strcmp(direction, "DoubleUp") == 0) { *twin = true; return kArrowNorth; }
    if (strcmp(direction, "DoubleDown") == 0) { *twin = true; return kArrowSouth; }
    return nullptr;
}

void drawNightscoutTrend(const char *direction, int x, int y, int scale, uint16_t color) {
    bool twin = false;
    const char *const *art = nightscoutTrendArt(direction, &twin);
    if (art == nullptr) return;
    const int pixel = arrowPixelSize(scale);
    const int top = y + (PIXEL_FONT_DIGIT_HEIGHT * scale - kArrowRows * pixel) / 2;
    const int gap = scale >= 8 ? 6 : 4;
    drawArrowBitmap(art, x, top, pixel, color);
    if (twin) drawArrowBitmap(art, x + kArrowCols * pixel + gap, top, pixel, color);
}

uint16_t nightscoutValueColor(uint16_t mgdl) {
    if (!kNsColourByRange) return kNsGreen;
    const NightscoutConfig &cfg = feedConfig.nightscout;
    if (mgdl <= cfg.urgentLowMgdl || mgdl >= cfg.urgentHighMgdl) return kNsRed;
    if (mgdl < cfg.lowMgdl || mgdl > cfg.highMgdl) return kNsOrange;
    return kNsGreen;
}

void drawNightscoutChart(const NightscoutRuntime &ns, int top, int bottom) {
    const int count = ns.historyCount;
    if (count < 1) return;
    const int left = 12, right = 228;
    uint16_t vmin = 1000, vmax = 0;
    for (int i = 0; i < count; ++i) {
        vmin = min(vmin, ns.history[i]);
        vmax = max(vmax, ns.history[i]);
    }
    float lo = vmin, hi = vmax;
    if (hi - lo < 54.0f) {  // keep tiny wiggles from looking dramatic
        const float mid = (hi + lo) / 2.0f;
        lo = mid - 27.0f;
        hi = mid + 27.0f;
    }
    const float pad = (hi - lo) * 0.08f;
    lo -= pad;
    hi += pad;

    int px[kNightscoutHistoryMax], py[kNightscoutHistoryMax];
    for (int i = 0; i < count; ++i) {
        const int idx = count - 1 - i;  // oldest on the left
        px[i] = count > 1 ? left + (right - left) * i / (count - 1) : (left + right) / 2;
        py[i] = bottom - static_cast<int>((ns.history[idx] - lo) / (hi - lo) * (bottom - top));
        if (py[i] < top) py[i] = top;
        if (py[i] > bottom) py[i] = bottom;
    }
    for (int i = 1; i < count; ++i) {
        tft.drawLine(px[i - 1], py[i - 1], px[i], py[i], kNsWhite);
        tft.drawLine(px[i - 1], py[i - 1] + 1, px[i], py[i] + 1, kNsWhite);
    }
    for (int i = 0; i < count; ++i) tft.fillCircle(px[i], py[i], 3, kNsWhite);
}

void nightscoutDeltaText(const NightscoutRuntime &ns, bool useMmol, char *buffer, size_t bufferSize) {
    const int delta = static_cast<int>(ns.history[0]) - static_cast<int>(ns.history[1]);
    if (!useMmol) {
        snprintf(buffer, bufferSize, "%+d", delta);
        if (strcmp(buffer, "+0") == 0 || strcmp(buffer, "-0") == 0) {
            strncpy(buffer, "0", bufferSize - 1);
            buffer[bufferSize - 1] = '\0';
        }
        return;
    }
    const float mmol = delta / 18.0182f;
    snprintf(buffer, bufferSize, "%+.1f", mmol);
    if (strcmp(buffer, "+0.0") == 0 || strcmp(buffer, "-0.0") == 0) {
        strncpy(buffer, "0.0", bufferSize - 1);
        buffer[bufferSize - 1] = '\0';
    }
}

struct NsRegion {
    bool drawn;
    uint32_t hash;
    int x;
    int y;
    int w;
    int h;
};

struct NsPaint {
    NsRegion clock;
    NsRegion age;
    NsRegion hero;
    NsRegion iob;
    NsRegion delta;
    NsRegion chart;
    NsRegion footer;
};

NsPaint nsPaint = {};

void pixelTextRect(int x, int y, const char *text, int scale, int align, int rowCount,
                   int &rx, int &ry, int &rw, int &rh) {
    const int width = pixelTextWidth(text, scale);
    int left = x;
    if (align == 1) left = x - width / 2;
    else if (align == 2) left = x - width;
    rx = left;
    ry = y;
    rw = width > 0 ? width : 1;
    rh = rowCount * scale;
}

void unionRect(int &x, int &y, int &w, int &h, int x2, int y2, int w2, int h2) {
    const int right = x + w > x2 + w2 ? x + w : x2 + w2;
    const int bottom = y + h > y2 + h2 ? y + h : y2 + h2;
    if (x2 < x) x = x2;
    if (y2 < y) y = y2;
    w = right - x;
    h = bottom - y;
}

void eraseScreenRect(int x, int y, int w, int h, bool keepHeader) {
    if (w < 1 || h < 1) return;
    if (keepHeader && y < 38) {
        const int cut = 38 - y;
        y = 38;
        h -= cut;
    }
    if (w < 1 || h < 1 || y >= 240) return;
    tft.fillRect(x, y, w, h, TFT_BLACK);
}

bool beginRegion(NsRegion &region, bool incremental, uint32_t hash, int x, int y, int w, int h,
                 bool keepHeader = true) {
    if (w < 1) w = 1;
    if (h < 1) h = 1;
    if (x < 0) {
        w += x;
        x = 0;
    }
    if (y < 0) {
        h += y;
        y = 0;
    }
    if (incremental && region.drawn && region.hash == hash) return false;
    if (incremental && region.drawn) eraseScreenRect(region.x, region.y, region.w, region.h, keepHeader);
    if (incremental) eraseScreenRect(x, y, w, h, keepHeader);
    region.x = x;
    region.y = y;
    region.w = w;
    region.h = h;
    region.hash = hash;
    region.drawn = true;
    return true;
}

void retireRegion(NsRegion &region, bool incremental) {
    if (incremental && region.drawn) eraseScreenRect(region.x, region.y, region.w, region.h, true);
    region.drawn = false;
    region.hash = 0;
}

void drawVerticallyCenteredText(const String &text, int centerX, int areaTop, int areaHeight,
                                 int maxWidth, int font, uint16_t color, uint16_t background) {
    std::vector<String> lines = wrapText(text, font, maxWidth);
    tft.setTextFont(font);
    const int fontHeight = tft.fontHeight();
    const int lineHeight = fontHeight + 6;
    const int count = lines.empty() ? 1 : static_cast<int>(lines.size());
    const int blockHeight = fontHeight + (count - 1) * lineHeight;
    int startY = areaTop + (areaHeight - blockHeight) / 2;
    if (startY < areaTop) startY = areaTop;
    tft.setTextDatum(TC_DATUM);
    tft.setTextPadding(0);
    tft.setTextColor(color, background);
    for (int line = 0; line < count; ++line) {
        tft.drawString(lines[line], centerX, startY + line * lineHeight, font);
    }
}

void renderNightscoutPage() {
    const NightscoutConfig &cfg = feedConfig.nightscout;
    const NightscoutRuntime &ns = feedRuntime.nightscout;
    tft.setTextSize(1);
    const bool incremental = nsPaint.clock.drawn &&
                             renderCache.valid &&
                             renderCache.mode == DISPLAY_MODE_DASHBOARD &&
                             renderCache.page == DASHBOARD_PAGE_NIGHTSCOUT;
    if (!incremental) {
        tft.fillScreen(TFT_BLACK);
        nsPaint = {};
    }
    footerClockCache[0] = '\0';

    if (!ns.hasData) {
        if (incremental) tft.fillScreen(TFT_BLACK);
        nsPaint = {};
        const bool failed = ns.lastError[0] != '\0';
        const bool privateSite = failed && strstr(ns.lastError, "private") != nullptr;
        if (privateSite) {
            drawVerticallyCenteredText(ns.lastError, 120, 36, 168, 216, FONT_BODY, kNsOrange, TFT_BLACK);
        } else if (failed) {
            drawPixelText(120, 70, "ERROR", 3, kNsOrange, 1);
        } else if (!feedsNightscoutConfigured()) {
            drawVerticallyCenteredText("Add your Nightscout site and token in the dashboard.",
                                       120, 36, 168, 216, FONT_BODY, kNsCyan, TFT_BLACK);
        } else {
            drawVerticallyCenteredText(kWaitingPuns[waitingPunIndex()], 120, 36, 168, 216,
                                       FONT_BODY, kNsCyan, TFT_BLACK);
        }
        if (failed && !privateSite) {
            const char *message = ns.lastError;
            for (int line = 0; line < 4 && message[0] != '\0'; ++line) {
                char row[29];
                size_t count = 0;
                while (message[count] != '\0' && count < 28) ++count;
                memcpy(row, message, count);
                row[count] = '\0';
                drawPixelText(120, 120 + line * 12, row, 1, kNsGrey, 1);
                message += count;
            }
        }
        return;
    }

    const int age = nightscoutAgeMinutes(ns);
    const bool stale = age > 15;
    const uint16_t valueColor = stale ? kNsGrey : nightscoutValueColor(ns.sgv);

    // Top row: local time (left) and age of the reading (right)
    char clockText[16];
    formatLocalClock(clockText, sizeof(clockText), dashboardConfig.showSeconds, dashboardConfig.use24Hour);
    char ageText[12];
    if (age < 1) strncpy(ageText, "now", sizeof(ageText));
    else if (age < 100) snprintf(ageText, sizeof(ageText), "%dm ago", age);
    else strncpy(ageText, "old", sizeof(ageText));
    ageText[sizeof(ageText) - 1] = '\0';
    int detailScale = 3;
    if (pixelTextWidth(clockText, detailScale) + pixelTextWidth(ageText, detailScale) > 212) {
        detailScale = 2;
    }
    int clockX = 0, clockY = 0, clockW = 0, clockH = 0;
    pixelTextRect(8, 4, clockText, detailScale, 0, PIXEL_FONT_HEIGHT, clockX, clockY, clockW, clockH);
    uint32_t clockHash = kFnvOffset;
    hashCString(clockHash, clockText);
    hashValue(clockHash, detailScale);
    int ageX = 0, ageY = 0, ageW = 0, ageH = 0;
    pixelTextRect(232, 4, ageText, detailScale, 2, PIXEL_FONT_HEIGHT, ageX, ageY, ageW, ageH);
    uint32_t ageHash = kFnvOffset;
    hashCString(ageHash, ageText);
    hashValue(ageHash, detailScale);

    // Value stays in the middle, with the trend arrow tucked against its right side.
    // Green inside the limits, orange outside, red past the urgent limits.
    char valueBuf[12];
    if (cfg.useMmol) snprintf(valueBuf, sizeof(valueBuf), "%.1f", ns.sgv / 18.0182f);
    else snprintf(valueBuf, sizeof(valueBuf), "%u", static_cast<unsigned>(ns.sgv));
    bool twinTrend = false;
    const bool hasArrow = nightscoutTrendArt(ns.direction, &twinTrend) != nullptr;
    int scale = 10;
    int numberWidth = 0;
    int arrowGap = 0;
    int arrowsWidth = 0;
    while (true) {
        numberWidth = pixelTextWidth(valueBuf, scale);
        arrowGap = hasArrow ? (scale >= 8 ? 6 : 4) : 0;
        const int oneArrow = hasArrow ? kArrowCols * arrowPixelSize(scale) : 0;
        arrowsWidth = twinTrend ? oneArrow * 2 + arrowGap : oneArrow;
        if (numberWidth + arrowGap + arrowsWidth <= 228 || scale <= 6) break;
        --scale;
    }
    const int groupWidth = numberWidth + arrowGap + arrowsWidth;
    int numberLeft = (DISPLAY_WIDTH - groupWidth) / 2;
    if (numberLeft < 2) numberLeft = 2;
    const int valueTop = 50;
    int heroX = numberLeft;
    int heroY = valueTop;
    int heroW = numberWidth > 0 ? numberWidth : 1;
    int heroH = PIXEL_FONT_DIGIT_HEIGHT * scale;
    if (hasArrow) {
        const int pixel = arrowPixelSize(scale);
        const int arrowTop = valueTop + (heroH - kArrowRows * pixel) / 2;
        const int arrowX = numberLeft + numberWidth + arrowGap;
        unionRect(heroX, heroY, heroW, heroH, arrowX, arrowTop, arrowsWidth, kArrowRows * pixel);
    }
    uint32_t heroHash = kFnvOffset;
    hashCString(heroHash, valueBuf);
    hashCString(heroHash, ns.direction);
    hashValue(heroHash, valueColor);
    hashValue(heroHash, scale);
    hashValue(heroHash, numberLeft);
    if (beginRegion(nsPaint.hero, incremental, heroHash, heroX, heroY, heroW, heroH)) {
        drawPixelText(numberLeft, valueTop, valueBuf, scale, valueColor, 0,
                      PIXEL_FONT_DIGIT_ROW, PIXEL_FONT_DIGIT_HEIGHT);
        if (hasArrow) {
            drawNightscoutTrend(ns.direction, numberLeft + numberWidth + arrowGap, valueTop, scale, kNsWhite);
        }
    }

    // IOB on the left. Delta, in mmol/l, on the right. The unit drops to the next
    // line when the two would meet.
    const int detailTop = valueTop + PIXEL_FONT_DIGIT_HEIGHT * scale + 16;
    const int rowScale = 3;
    char iobText[16];
    iobText[0] = '\0';
    if (cfg.showIob) {
        if (ns.iobValid) snprintf(iobText, sizeof(iobText), "%.2fU", ns.iob);
        else strncpy(iobText, "--U", sizeof(iobText));
        iobText[sizeof(iobText) - 1] = '\0';
        int iobX = 0, iobY = 0, iobW = 0, iobH = 0;
        pixelTextRect(8, detailTop, iobText, rowScale, 0, PIXEL_FONT_HEIGHT, iobX, iobY, iobW, iobH);
        uint32_t iobHash = kFnvOffset;
        hashCString(iobHash, iobText);
        hashValue(iobHash, detailTop);
        if (beginRegion(nsPaint.iob, incremental, iobHash, iobX, iobY, iobW, iobH)) {
            drawPixelText(8, detailTop, iobText, rowScale, kNsOrange, 0);
        }
    } else {
        retireRegion(nsPaint.iob, incremental);
    }
    int chartTop = detailTop + 6;
    if (ns.historyCount >= 2) {
        char deltaValue[12];
        nightscoutDeltaText(ns, cfg.useMmol, deltaValue, sizeof(deltaValue));
        const char *unitLabel = cfg.useMmol ? "mmol/l" : "mg/dl";
        const int unitGap = 8;
        const int valueWidth = pixelTextWidth(deltaValue, rowScale);
        const int unitWidth = pixelTextWidth(unitLabel, rowScale);
        const int iobWidth = cfg.showIob ? pixelTextWidth(iobText, rowScale) : 0;
        const int lineWidth = valueWidth + unitGap + unitWidth;
        const bool wrapUnit = iobWidth > 0 && iobWidth + 8 + lineWidth > 224;
        const int delta = static_cast<int>(ns.history[0]) - static_cast<int>(ns.history[1]);
        const uint16_t deltaColor = delta > 0 ? kNsGreen : (delta < 0 ? kNsRed : kNsWhite);
        int deltaX = 0, deltaY = 0, deltaW = 0, deltaH = 0;
        const int unitTop = detailTop + 6 * rowScale;
        if (wrapUnit) {
            chartTop = unitTop + PIXEL_FONT_HEIGHT * rowScale + 4;
            pixelTextRect(232, detailTop, deltaValue, rowScale, 2, PIXEL_FONT_HEIGHT, deltaX, deltaY, deltaW, deltaH);
            int unitX = 0, unitY = 0, unitW = 0, unitH = 0;
            pixelTextRect(232, unitTop, unitLabel, rowScale, 2, PIXEL_FONT_HEIGHT, unitX, unitY, unitW, unitH);
            unionRect(deltaX, deltaY, deltaW, deltaH, unitX, unitY, unitW, unitH);
        } else {
            chartTop = detailTop + PIXEL_FONT_HEIGHT * rowScale + 6;
            deltaX = 232 - lineWidth;
            deltaY = detailTop;
            deltaW = lineWidth;
            deltaH = PIXEL_FONT_HEIGHT * rowScale;
        }
        uint32_t deltaHash = kFnvOffset;
        hashCString(deltaHash, deltaValue);
        hashCString(deltaHash, unitLabel);
        hashValue(deltaHash, wrapUnit);
        hashValue(deltaHash, deltaColor);
        hashValue(deltaHash, detailTop);
        if (beginRegion(nsPaint.delta, incremental, deltaHash, deltaX, deltaY, deltaW, deltaH)) {
            if (wrapUnit) {
                drawPixelText(232, detailTop, deltaValue, rowScale, deltaColor, 2);
                drawPixelText(232, unitTop, unitLabel, rowScale, kNsWhite, 2);
            } else {
                drawPixelText(232 - unitWidth - unitGap, detailTop, deltaValue, rowScale, deltaColor, 2);
                drawPixelText(232, detailTop, unitLabel, rowScale, kNsWhite, 2);
            }
        }
    } else {
        retireRegion(nsPaint.delta, incremental);
        if (cfg.showIob) chartTop = detailTop + PIXEL_FONT_HEIGHT * rowScale + 6;
    }
    if (chartTop > 196) chartTop = 196;

    // Chart + footer. Unchanged areas stay on screen, so a glucose read does not black the display.
    int chartX = 8;
    int chartY = chartTop > 4 ? chartTop - 4 : 0;
    int chartW = 224;
    int chartH = 214 - chartY;
    uint32_t chartHash = kFnvOffset;
    hashValue(chartHash, chartTop);
    hashValue(chartHash, ns.historyCount);
    for (uint8_t i = 0; i < ns.historyCount; ++i) hashValue(chartHash, ns.history[i]);
    if (beginRegion(nsPaint.chart, incremental, chartHash, chartX, chartY, chartW, chartH)) {
        drawNightscoutChart(ns, chartTop, 210);
    }
    char footerText[24];
    footerText[0] = '\0';
    uint16_t footerColor = kNsCyan;
    int footerY = 220;
    if (ns.lastError[0] != '\0') {
        strncpy(footerText, "SYNC ERROR", sizeof(footerText) - 1);
        footerColor = kNsOrange;
        footerY = 218;
    } else if (dashboardConfig.showIp && displayState.ipInfo[0] != '\0') {
        strncpy(footerText, displayState.ipInfo, sizeof(footerText) - 1);
    }
    if (footerText[0] != '\0') {
        int footerX = 0, footerTop = 0, footerW = 0, footerH = 0;
        pixelTextRect(120, footerY, footerText, 1, 1, PIXEL_FONT_HEIGHT, footerX, footerTop, footerW, footerH);
        uint32_t footerHash = kFnvOffset;
        hashCString(footerHash, footerText);
        hashValue(footerHash, footerColor);
        if (beginRegion(nsPaint.footer, incremental, footerHash, footerX, footerTop, footerW, footerH)) {
            drawPixelText(120, footerY, footerText, 1, footerColor, 1);
        }
    } else {
        retireRegion(nsPaint.footer, incremental);
    }

    // Time and age are painted last so a glucose or chart clear cannot leave
    // the corner blank. Unchanged text is drawn again on the existing pixels.
    beginRegion(nsPaint.clock, incremental, clockHash, clockX, clockY, clockW, clockH, false);
    beginRegion(nsPaint.age, incremental, ageHash, ageX, ageY, ageW, ageH, false);
    drawPixelText(8, 4, clockText, detailScale, kNsWhite, 0);
    drawPixelText(232, 4, ageText, detailScale, kNsWhite, 2);
}

void renderDashboardPage() {
    if (!dashboardPageAvailable(displayState.currentPage)) {
        displayState.currentPage = firstAvailableDashboardPage();
    }

    switch (displayState.currentPage) {
        case DASHBOARD_PAGE_CLOCK:
            renderClockPage();
            break;
        case DASHBOARD_PAGE_NIGHTSCOUT:
            renderNightscoutPage();
            break;
        case DASHBOARD_PAGE_WEATHER:
            renderWeatherPage();
            break;
        default:
            renderClockPage();
            break;
    }
}

void updateDashboardDynamicArea(uint8_t pageId) {
    switch (pageId) {
        case DASHBOARD_PAGE_CLOCK:
            updateClockDynamicArea();
            break;
        default:
            break;
    }
}

void renderDashboardPageCached() {
    if (!dashboardPageAvailable(displayState.currentPage)) {
        displayState.currentPage = firstAvailableDashboardPage();
    }

    uint8_t pageId = displayState.currentPage;
    uint8_t themeId = activeThemeIdValue();
    uint32_t staticHash = dashboardStaticHash(pageId);
    uint32_t dynamicHash = dashboardDynamicHash(pageId);

    bool requiresFullRender = !renderCache.valid ||
                              renderCache.mode != DISPLAY_MODE_DASHBOARD ||
                              renderCache.page != pageId ||
                              renderCache.theme != themeId ||
                              renderCache.staticHash != staticHash;

    if (requiresFullRender) {
        renderDashboardPage();
        renderCache.valid = true;
        renderCache.mode = DISPLAY_MODE_DASHBOARD;
        renderCache.page = pageId;
        renderCache.theme = themeId;
        renderCache.staticHash = staticHash;
        renderCache.dynamicHash = dynamicHash;
        return;
    }

    if (pageUsesDynamicRefresh(pageId) && renderCache.dynamicHash != dynamicHash) {
        updateDashboardDynamicArea(pageId);
        renderCache.dynamicHash = dynamicHash;
    }
    updateFooterClock();
}

}  // namespace

void setBacklightLevel(int brightness, bool rememberPreference, bool shouldLog = true) {
    int boundedBrightness = constrain(brightness, 0, 100);
    if (rememberPreference) {
        savedBrightness = boundedBrightness;
    }

    if (appliedBrightness == boundedBrightness) {
        backlightOn = boundedBrightness > 0;
        return;
    }

    int pwmValue = 0;
    if (boundedBrightness == 0) {
        pwmValue = 1023;
    } else if (boundedBrightness == 100) {
        pwmValue = 0;
    } else {
        pwmValue = map(boundedBrightness, 0, 100, 1023, 0);
    }

    analogWrite(PIN_BACKLIGHT, pwmValue);
    appliedBrightness = boundedBrightness;
    backlightOn = boundedBrightness > 0;
    if (shouldLog) {
        logPrintf(F("Brightness: %d%%, PWM: %d"), boundedBrightness, pwmValue);
    }
}

void animateBacklightRamp(int fromBrightness,
                          int toBrightness,
                          uint8_t steps,
                          uint16_t stepDelayMs) {
    if (steps == 0) {
        setBacklightLevel(toBrightness, false, false);
        return;
    }

    int start = constrain(fromBrightness, 0, 100);
    int target = constrain(toBrightness, 0, 100);
    if (start == target) {
        return;
    }

    for (uint8_t step = 1; step <= steps; ++step) {
        int interpolated = start + ((target - start) * static_cast<int>(step)) / static_cast<int>(steps);
        setBacklightLevel(interpolated, false, false);
        if (stepDelayMs > 0) {
            delay(stepDelayMs);
            yield();
        }
    }
}

int beginPageTransition(int targetBrightness) {
    int startBrightness = constrain(targetBrightness, 0, 100);
    if (startBrightness < kPageTransitionMinimumBrightness) {
        return startBrightness;
    }

    int dimBrightness = max(static_cast<int>(kPageTransitionMinimumBrightness),
                            (startBrightness * static_cast<int>(kPageTransitionDimPercent)) / 100);
    if (dimBrightness >= startBrightness) {
        return startBrightness;
    }

    animateBacklightRamp(startBrightness,
                         dimBrightness,
                         kPageTransitionFadeDownSteps,
                         kPageTransitionFadeStepDelayMs);
    return dimBrightness;
}

void endPageTransition(int transitionBrightness, int targetBrightness) {
    int startBrightness = constrain(transitionBrightness, 0, 100);
    int endBrightness = constrain(targetBrightness, 0, 100);
    if (startBrightness == endBrightness) {
        return;
    }

    delay(8);
    yield();
    animateBacklightRamp(startBrightness,
                         endBrightness,
                         kPageTransitionFadeUpSteps,
                         kPageTransitionFadeStepDelayMs);
}

void displayInit() {
    logPrint(F("Display init..."));

    tft.init();
    tft.setRotation(0);
    tft.invertDisplay(true);
    tft.fillScreen(TFT_BLACK);


    displayState.line2[0] = '\0';
    displayState.ipInfo[0] = '\0';
    displayState.apMode = false;
    displayState.apSSID[0] = '\0';
    displayState.apPassword[0] = '\0';
    displayState.currentPage = DASHBOARD_PAGE_NIGHTSCOUT;
    clearTemporaryMessage();

    clockDynamicSprite.deleteSprite();
    clockDynamicSpriteReady = false;
    clockDynamicSpriteAttempted = false;
    clockDynamicSpriteAllowed = false;

    pinMode(PIN_BACKLIGHT, OUTPUT);
    analogWriteFreq(1000);
    analogWriteRange(1023);

    logPrint(F("Display init complete"));
}

void displaySetBrightness(int brightness) {
    setBacklightLevel(brightness, true);
}

void displayApplyBrightness(int brightness) {
    setBacklightLevel(brightness, false);
}

void displaySetClockSpriteAllowed(bool allowed) {
    if (clockDynamicSpriteAllowed == allowed &&
        (!clockDynamicSpriteReady || allowed)) {
        return;
    }

    clockDynamicSpriteAllowed = allowed;
    clockDynamicSpriteAttempted = false;

    if (!allowed) {
        clockDynamicSprite.deleteSprite();
        clockDynamicSpriteReady = false;
    }
}

void displaySuspendDynamicResources() {
    if (dynamicResourceSuspendDepth == 0) {
        resumeClockSpriteAfterDynamicSuspend = clockDynamicSpriteAllowed;
        if (clockDynamicSpriteReady || clockDynamicSpriteAllowed) {
            clockDynamicSprite.deleteSprite();
            clockDynamicSpriteReady = false;
            clockDynamicSpriteAttempted = false;
            clockDynamicSpriteAllowed = false;
            logPrintf(F("Display resources suspended, free heap now %u"), ESP.getFreeHeap());
        }
    }

    if (dynamicResourceSuspendDepth < 255) {
        ++dynamicResourceSuspendDepth;
    }
}

void displayResumeDynamicResources() {
    if (dynamicResourceSuspendDepth == 0) {
        return;
    }

    --dynamicResourceSuspendDepth;
    if (dynamicResourceSuspendDepth != 0) {
        return;
    }

    if (resumeClockSpriteAfterDynamicSuspend) {
        clockDynamicSpriteAllowed = true;
        clockDynamicSpriteAttempted = false;
        resumeClockSpriteAfterDynamicSuspend = false;
    }
}

bool holdSplashScreen = false;
uint32_t splashShownAtMs = 0;

void displayUpdate() {
    if (holdSplashScreen) return;
    if (temporaryMessageVisible()) {
        uint32_t currentHash = temporaryMessageHash();
        uint8_t themeId = activeThemeIdValue();
        if (renderCache.valid &&
            renderCache.mode == DISPLAY_MODE_TEMP_MESSAGE &&
            renderCache.theme == themeId &&
            renderCache.staticHash == currentHash) {
            updateFooterClock();
            return;
        }

        const ThemePalette &theme = activeTheme();
        drawScreenChrome("SugarCube");
        drawWrappedCenteredText(temporaryMessage.message, 120, 60, 224, FONT_BODY, theme.text, theme.background, 4, 144);

        renderCache.valid = true;
        renderCache.mode = DISPLAY_MODE_TEMP_MESSAGE;
        renderCache.page = 0;
        renderCache.theme = themeId;
        renderCache.staticHash = currentHash;
        renderCache.dynamicHash = 0;
        return;
    }

    if (temporaryMessageExpired()) {
        clearTemporaryMessage();
        invalidateRenderCache();
    }



    if (displayState.apMode) {
        uint32_t currentHash = apScreenHash();
        uint8_t themeId = activeThemeIdValue();
        if (renderCache.valid &&
            renderCache.mode == DISPLAY_MODE_AP &&
            renderCache.theme == themeId &&
            renderCache.staticHash == currentHash) {
            return;
        }

        displayRenderAPMode();
        renderCache.valid = true;
        renderCache.mode = DISPLAY_MODE_AP;
        renderCache.page = 0;
        renderCache.theme = themeId;
        renderCache.staticHash = currentHash;
        renderCache.dynamicHash = 0;
        return;
    }

    renderDashboardPageCached();
}

void displayRenderClock() {
    renderClockPage();
}

void displayRenderAPMode() {
    const ThemePalette &theme = activeTheme();
    bool canRevealAdminPassword = authCanRevealPassword();
    const char *adminPassword = authProvisionedPassword();
    String setupIp = displayState.ipInfo[0] != '\0' ? safeCString(displayState.ipInfo) : String("192.168.4.1");
    const int kWideFonts[] = {FONT_BODY, FONT_LABEL, FONT_INFO};
    const int kIpFonts[] = {FONT_INFO};
    const int kCompactFonts[] = {FONT_LABEL, FONT_INFO};

    tft.fillScreen(theme.background);

    tft.fillRoundRect(10, 10, 220, 24, 12, theme.surfaceAlt);
    tft.fillCircle(24, 22, 4, theme.accent);
    tft.setTextDatum(TL_DATUM);
    tft.setTextFont(FONT_INFO);
    tft.setTextColor(theme.text, theme.surfaceAlt);
    tft.drawString("Setup mode", 36, 16, FONT_INFO);

    drawRoundedPanel(10, 42, 220, 60, theme.accentSoft, theme.accentSoft);
    tft.setTextDatum(TC_DATUM);
    tft.setTextFont(FONT_INFO);
    tft.setTextColor(theme.accent, theme.accentSoft);
    tft.drawString("SETUP URL", 120, 52, FONT_INFO);
    drawAdaptiveText(setupIp,
                     120,
                     68,
                     192,
                     TC_DATUM,
                     kIpFonts,
                     sizeof(kIpFonts) / sizeof(kIpFonts[0]),
                     theme.text,
                     theme.accentSoft);
    tft.setTextFont(FONT_INFO);
    tft.setTextColor(theme.muted, theme.accentSoft);
    tft.drawString("Open this in browser", 120, 86, FONT_INFO);

    drawRoundedPanel(10, 108, 134, 50, theme.surface, theme.surfaceAlt);
    tft.setTextDatum(TL_DATUM);
    tft.setTextFont(FONT_INFO);
    tft.setTextColor(theme.muted, theme.surface);
    tft.drawString("WI-FI", 20, 118, FONT_INFO);
    drawAdaptiveText(safeCString(displayState.apSSID),
                     77,
                     136,
                     106,
                     TC_DATUM,
                     kCompactFonts,
                     sizeof(kCompactFonts) / sizeof(kCompactFonts[0]),
                     theme.text,
                     theme.surface);

    drawRoundedPanel(150, 108, 80, 50, theme.surface, theme.surfaceAlt);
    tft.setTextDatum(TC_DATUM);
    tft.setTextFont(FONT_INFO);
    tft.setTextColor(theme.muted, theme.surface);
    tft.drawString("PASS", 190, 118, FONT_INFO);
    drawAdaptiveText(safeCString(displayState.apPassword),
                     190,
                     136,
                     58,
                     TC_DATUM,
                     kWideFonts,
                     sizeof(kWideFonts) / sizeof(kWideFonts[0]),
                     theme.text,
                     theme.surface);

    drawRoundedPanel(10, 168, 220, 62, theme.surfaceAlt, theme.surfaceAlt);
    tft.setTextDatum(TL_DATUM);
    tft.setTextFont(FONT_INFO);
    tft.setTextColor(canRevealAdminPassword ? theme.accent : theme.muted, theme.surfaceAlt);
    tft.drawString("ADMIN LOGIN", 20, 178, FONT_INFO);

    if (canRevealAdminPassword && adminPassword != nullptr && adminPassword[0] != '\0') {
        drawBadge(20, 193, 48, 18, "admin", theme.accentSoft, theme.accent);
        drawAdaptiveText(safeCString(adminPassword),
                         220,
                         191,
                         138,
                         TR_DATUM,
                         kWideFonts,
                         sizeof(kWideFonts) / sizeof(kWideFonts[0]),
                         theme.text,
                         theme.surfaceAlt);

        tft.setTextDatum(TL_DATUM);
        tft.setTextFont(FONT_INFO);
        tft.setTextColor(theme.muted, theme.surfaceAlt);
        tft.drawString("Use this password on the dashboard.", 20, 212, FONT_INFO);
        return;
    }

    drawWrappedCenteredText("Use your saved admin password after Wi-Fi setup.",
                            120,
                            194,
                            192,
                            FONT_INFO,
                            theme.muted,
                            theme.surfaceAlt,
                            2, 34);
}

void displayShowDashboard() {
    holdSplashScreen = false;
    clearTemporaryMessage();
    displayState.apMode = false;
    invalidateRenderCache();
    displayUpdate();
}



void displayServiceTemporaryMessage() {
    if (temporaryMessageExpired()) {
        displayUpdate();
    }
}





void displayShowMessage(const String &msg) {
    holdSplashScreen = false;
    clearTemporaryMessage();
    const ThemePalette &theme = activeTheme();
    drawScreenChrome("SugarCube");
    drawWrappedCenteredText(msg, 120, 60, 224, FONT_BODY, theme.text, theme.background, 4, 144);
    invalidateRenderCache();
}

void displayShowSplash(const String &message) {
    holdSplashScreen = true;
    splashShownAtMs = millis();
    clearTemporaryMessage();
    const ThemePalette &theme = activeTheme();
    tft.fillScreen(theme.background);
    tft.setTextSize(1);
    footerClockCache[0] = '\0';
    drawWrappedCenteredText("SugarCube", 120, 28, 224, FONT_BODY, theme.text, theme.background, 4, 40);

    tft.setTextFont(FONT_BODY);
    const int bodyHeight = tft.fontHeight();
    const int messageTop = (DISPLAY_HEIGHT - bodyHeight) / 2;
    drawWrappedCenteredText(message, 120, messageTop, 224, FONT_BODY, theme.text, theme.background, 4, bodyHeight + 4);

    tft.setTextFont(FONT_LABEL);
    const int versionHeight = tft.fontHeight();
    drawWrappedCenteredText(FIRMWARE_VERSION_STRING, 120, DISPLAY_HEIGHT - versionHeight - 14, 220,
                            FONT_LABEL, theme.muted, theme.background, 0, versionHeight + 2);
    invalidateRenderCache();
}

bool displaySplashBusy() {
    return holdSplashScreen && static_cast<int32_t>(millis() - splashShownAtMs) < 900;
}

void displayShowTemporaryMessage(const String &msg, uint32_t durationMs) {
    strncpy(temporaryMessage.message, msg.c_str(), sizeof(temporaryMessage.message) - 1);
    temporaryMessage.message[sizeof(temporaryMessage.message) - 1] = '\0';
    temporaryMessage.active = temporaryMessage.message[0] != '\0';
    temporaryMessage.expiresAtMs = millis() + (durationMs > 0 ? durationMs : 1000UL);
    invalidateRenderCache();
    displayUpdate();
}

void displayShowAPScreen(const char *ssid, const char *password, const char *ip) {
    holdSplashScreen = false;
    displayState.apMode = true;
    strncpy(displayState.apSSID, ssid, sizeof(displayState.apSSID) - 1);
    displayState.apSSID[sizeof(displayState.apSSID) - 1] = '\0';
    strncpy(displayState.apPassword, password, sizeof(displayState.apPassword) - 1);
    displayState.apPassword[sizeof(displayState.apPassword) - 1] = '\0';
    strncpy(displayState.ipInfo, ip, sizeof(displayState.ipInfo) - 1);
    displayState.ipInfo[sizeof(displayState.ipInfo) - 1] = '\0';
    invalidateRenderCache();
    displayUpdate();
}

void displayCycleNextPage(bool smoothTransition) {
    if (temporaryMessageVisible()) {
        logPrint(F("Page cycling disabled while temporary message is active"));
        return;
    }

    if (displayState.apMode) {
        logPrint(F("Page cycling disabled in AP mode"));
        return;
    }



    uint8_t previousPage = displayState.currentPage;
    if (!dashboardPageAvailable(displayState.currentPage)) {
        displayState.currentPage = firstAvailableDashboardPage();
    } else {
        displayState.currentPage = nextAvailableDashboardPage(displayState.currentPage);
    }

    if (displayState.currentPage == previousPage) {
        return;
    }

    int targetBrightness = appliedBrightness;
    int transitionBrightness = targetBrightness;
    if (smoothTransition) {
        transitionBrightness = beginPageTransition(targetBrightness);
    }

    invalidateRenderCache();
    displayUpdate();
    if (smoothTransition) {
        endPageTransition(transitionBrightness, targetBrightness);
    }
}

void displayToggleBacklight() {
    if (backlightOn) {
        logPrint(F("Backlight OFF"));
        displayApplyBrightness(0);
    } else {
        int resumeBrightness = dashboardEffectiveBrightness(savedBrightness > 0 ? savedBrightness : 100);
        logPrintf(F("Backlight ON (brightness: %d)"), resumeBrightness);
        displayApplyBrightness(resumeBrightness);
    }
}
