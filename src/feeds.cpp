// Upstream SmartClock firmware by B. van Weerd, with SugarCube changes.
// Copyright (c) 2026 B. van Weerd. Upstream code in this file is MIT. See LICENSE.
// Copyright (c) 2026 Peter Štieber. SugarCube changes are PolyForm Noncommercial 1.0.0. See LICENSE-SUGARCUBE.
// Required Notice: Copyright (c) 2026 Peter Štieber. SugarCube changes may not be sold or used commercially.

#include "feeds.h"

#include "display.h"
#include "logger.h"
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <LittleFS.h>
#include <WiFiClientSecureBearSSL.h>
#include <ctype.h>
#include <cont.h>
#include <lwip/tcp.h>
#include <include/ClientContext.h>
#include <limits.h>
#include <functional>
#include <math.h>
#include <umm_malloc/umm_heap_select.h>

FeedConfig feedConfig;
FeedRuntimeState feedRuntime;

namespace {

constexpr uint16_t kWeatherMinRefreshMinutes = 5;
constexpr uint16_t kWeatherMaxRefreshMinutes = 240;
constexpr uint16_t kNightscoutMinRefreshMinutes = 1;
constexpr uint16_t kNightscoutMaxRefreshMinutes = 30;
constexpr uint16_t kNightscoutMinRefreshSeconds = 15;
constexpr uint16_t kNightscoutMaxRefreshSeconds = 3600;
constexpr uint16_t kNightscoutDefaultRefreshSeconds = 30;
constexpr uint32_t kHttpTimeoutMs = 12000;
constexpr uint32_t kFeedStartupGracePeriodMs = 15000UL;
constexpr uint32_t kHttpsMinFreeHeapBytes = 28000UL;
constexpr uint32_t kHttpsMinDramHeapBytes = 20000UL;
// The insecure handshake keeps its record buffers in IRAM. The DRAM block it
// needs is a few KB, and after a sync the largest hole often sits near 10 KB.
constexpr uint32_t kHttpsMinDramBlockBytes = 9600UL;
constexpr uint32_t kHttpsHeapWaitMs = 2000UL;
constexpr uint16_t kHttpsDefaultRecvBufferBytes = 4096;
constexpr uint16_t kHttpsCompactXmitBufferBytes = 512;
constexpr uint32_t kRequestDeadlineMs = 24000UL;

FeedConfig savedFeedConfig;
bool feedDraftActive = false;
uint32_t feedsStartupReadyAtMs = 0;

// HTTPClient clones the public wrapper, sharing its underlying TLS context.
// Abort only the context's TCP socket while the feed worker is suspended.
class FeedSocketAccess : public WiFiClient {
public:
    static void abortTransport(WiFiClient *client) {
        if (!client) return;
        ClientContext *socket = client->*(&FeedSocketAccess::_client);
        if (socket) socket->abort();
    }
};
class FeedTlsClient : public BearSSL::WiFiClientSecure {
public:
    void abortTransport() { FeedSocketAccess::abortTransport(_owned); }
};
class FeedHttpClient : public HTTPClient {
public:
    WiFiClient *transportClient() const { return _client.get(); }
    void abortTransport() { FeedSocketAccess::abortTransport(transportClient()); }
};
FeedTlsClient *activeTlsClient = nullptr;
std::unique_ptr<cont_t> feedContinuation;
uint16_t pendingSyncMask = 0;
uint16_t activeSyncMask = 0;
bool feedJobRunning = false;
bool feedJobComplete = false;
bool feedBatchActive = false;
bool feedJobsCancelled = false;
bool lastSyncSuccess = false;
String lastSyncError;
FeedHttpClient *activeHttpClient = nullptr;
uint32_t activeRequestStartedMs = 0;
bool activeRequestExpired = false;
FeedSearchStatus searchStatus = FEED_SEARCH_IDLE;
String searchQuery, searchJson, searchError;


bool feedConfigEquals(const FeedConfig &left, const FeedConfig &right);
void updateDraftState();
void copyString(char *destination, size_t destinationSize, const char *source);

bool buildTempPath(const char *path, char *tempPath, size_t tempPathSize) {
    int tempPathLength = snprintf(tempPath, tempPathSize, "%s.tmp", path);
    return tempPathLength > 0 && static_cast<size_t>(tempPathLength) < tempPathSize;
}

void copyString(char *destination, size_t destinationSize, const char *source) {
    if (destinationSize == 0) {
        return;
    }

    if (source == nullptr) {
        destination[0] = '\0';
        return;
    }

    strncpy(destination, source, destinationSize - 1);
    destination[destinationSize - 1] = '\0';
}

uint16_t clampWeatherRefresh(uint16_t minutes) {
    return constrain(minutes, kWeatherMinRefreshMinutes, kWeatherMaxRefreshMinutes);
}

uint16_t clampNightscoutRefresh(uint16_t seconds) {
    if (seconds == 0) {
        seconds = kNightscoutDefaultRefreshSeconds;
    }
    if (seconds < kNightscoutMinRefreshSeconds) return kNightscoutMinRefreshSeconds;
    if (seconds > kNightscoutMaxRefreshSeconds) return kNightscoutMaxRefreshSeconds;
    return seconds;
}

bool weatherCoordinatesValid(float latitude, float longitude) {
    return isfinite(latitude) &&
           isfinite(longitude) &&
           latitude >= -90.0f &&
           latitude <= 90.0f &&
           longitude >= -180.0f &&
           longitude <= 180.0f &&
           (fabsf(latitude) > 0.0001f || fabsf(longitude) > 0.0001f);
}

const char* weatherSourceToString(uint8_t source) {
    switch (source) {
        case WEATHER_FEED_OPEN_METEO:
            return "open-meteo";
        case WEATHER_FEED_DISABLED:
        default:
            return "disabled";
    }
}

uint8_t weatherSourceFromVariant(JsonVariantConst value) {
    if (value.isNull()) {
        return feedConfig.weather.source;
    }

    if (value.is<const char*>()) {
        String source = value.as<const char*>();
        source.toLowerCase();

        if (source == "open-meteo" || source == "openmeteo" || source == "meteo") {
            return WEATHER_FEED_OPEN_METEO;
        }
        return WEATHER_FEED_DISABLED;
    }

    return static_cast<uint8_t>(value.as<int>());
}

String urlEncode(const String &value) {
    static const char hex[] = "0123456789ABCDEF";
    String encoded;
    encoded.reserve(value.length() * 3);

    for (size_t index = 0; index < value.length(); ++index) {
        uint8_t character = static_cast<uint8_t>(value.charAt(index));
        if (isalnum(character) || character == '-' || character == '_' || character == '.' || character == '~') {
            encoded += static_cast<char>(character);
            continue;
        }

        encoded += '%';
        encoded += hex[(character >> 4) & 0x0F];
        encoded += hex[character & 0x0F];
    }

    return encoded;
}

String trimStringCopy(const char *value) {
    String normalized = value != nullptr ? String(value) : String("");
    normalized.trim();
    return normalized;
}

void trimAndCopyString(char *destination, size_t destinationSize, const char *source) {
    String normalized = trimStringCopy(source);
    copyString(destination, destinationSize, normalized.c_str());
}

bool baseUrlValid(const String &baseUrl) {
    return baseUrl.startsWith("http://") || baseUrl.startsWith("https://");
}

void normalizeBaseUrl(char *value, size_t valueSize) {
    if (value == nullptr || valueSize == 0) {
        return;
    }

    String normalized = trimStringCopy(value);
    while (normalized.endsWith("/")) {
        normalized.remove(normalized.length() - 1);
    }
    copyString(value, valueSize, normalized.c_str());
}

String buildWeatherLocationLabel(JsonObjectConst item) {
    String label = item["name"].isNull() ? String("") : String(item["name"].as<const char*>());
    if (!item["admin1"].isNull() && String(item["admin1"].as<const char*>()).length() > 0) {
        label += ", " + String(item["admin1"].as<const char*>());
    }
    if (!item["country"].isNull() && String(item["country"].as<const char*>()).length() > 0) {
        label += ", " + String(item["country"].as<const char*>());
    }
    return label;
}

void buildWeatherSearchFilter(JsonDocument &filter) {
    JsonArray results = filter["results"].to<JsonArray>();
    JsonObject item = results.add<JsonObject>();
    item["name"] = true;
    item["country"] = true;
    item["admin1"] = true;
    item["latitude"] = true;
    item["longitude"] = true;
    item["timezone"] = true;
}

void buildWeatherForecastFilter(JsonDocument &filter) {
    JsonObject current = filter["current"].to<JsonObject>();
    current["temperature_2m"] = true;
    current["weather_code"] = true;

    JsonObject daily = filter["daily"].to<JsonObject>();
    daily["temperature_2m_max"] = true;
    daily["temperature_2m_min"] = true;
    daily["precipitation_probability_max"] = true;
}

void buildNightscoutFilter(JsonDocument &filter) {
    // The endpoint returns an array; a filter with one element applies to every item.
    filter[0]["sgv"] = true;
    filter[0]["direction"] = true;
    filter[0]["date"] = true;
}

void setConfigDefaults() {
    memset(&feedConfig, 0, sizeof(feedConfig));
    feedConfig.version = FEEDS_CONFIG_VERSION;

    feedConfig.weather.source = WEATHER_FEED_DISABLED;
    feedConfig.weather.refreshMinutes = 30;
    feedConfig.weather.useFahrenheit = false;

    feedConfig.nightscout.enabled = false;
    feedConfig.nightscout.useMmol = false;
    feedConfig.nightscout.showIob = true;
    feedConfig.nightscout.refreshMinutes = 1;
    feedConfig.nightscout.refreshSeconds = kNightscoutDefaultRefreshSeconds;
    feedConfig.nightscout.urgentLowMgdl = 54;
    feedConfig.nightscout.lowMgdl = 70;
    feedConfig.nightscout.highMgdl = 180;
    feedConfig.nightscout.urgentHighMgdl = 250;
}

void clearWeatherRuntime() {
    memset(&feedRuntime.weather, 0, sizeof(feedRuntime.weather));
}

void clearNightscoutRuntime() {
    memset(&feedRuntime.nightscout, 0, sizeof(feedRuntime.nightscout));
}

void clearAllRuntime() {
    clearWeatherRuntime();
    clearNightscoutRuntime();
}

void normalizeConfig() {
    feedConfig.version = FEEDS_CONFIG_VERSION;

    // Weather is no longer shown or fetched. Old config entries stay on disk and are ignored.
    feedConfig.weather.source = WEATHER_FEED_DISABLED;
    feedConfig.weather.query[sizeof(feedConfig.weather.query) - 1] = '\0';
    feedConfig.weather.label[sizeof(feedConfig.weather.label) - 1] = '\0';
    feedConfig.weather.refreshMinutes = clampWeatherRefresh(feedConfig.weather.refreshMinutes);
    if (!isfinite(feedConfig.weather.latitude) || feedConfig.weather.latitude < -90.0f || feedConfig.weather.latitude > 90.0f) {
        feedConfig.weather.latitude = 0.0f;
    }
    if (!isfinite(feedConfig.weather.longitude) || feedConfig.weather.longitude < -180.0f || feedConfig.weather.longitude > 180.0f) {
        feedConfig.weather.longitude = 0.0f;
    }

    NightscoutConfig &nightscout = feedConfig.nightscout;
    normalizeBaseUrl(nightscout.baseUrl, sizeof(nightscout.baseUrl));
    trimAndCopyString(nightscout.token, sizeof(nightscout.token), nightscout.token);
    nightscout.refreshSeconds = clampNightscoutRefresh(nightscout.refreshSeconds);
    uint16_t legacyMinutes = static_cast<uint16_t>((static_cast<uint32_t>(nightscout.refreshSeconds) + 59U) / 60U);
    if (legacyMinutes < kNightscoutMinRefreshMinutes) legacyMinutes = kNightscoutMinRefreshMinutes;
    if (legacyMinutes > kNightscoutMaxRefreshMinutes) legacyMinutes = kNightscoutMaxRefreshMinutes;
    nightscout.refreshMinutes = legacyMinutes;
    auto clampU16 = [](uint16_t value, uint16_t low, uint16_t high) -> uint16_t {
        return value < low ? low : (value > high ? high : value);
    };
    nightscout.urgentLowMgdl = clampU16(nightscout.urgentLowMgdl, 30, 120);
    nightscout.lowMgdl = clampU16(nightscout.lowMgdl, 40, 200);
    nightscout.highMgdl = clampU16(nightscout.highMgdl, 100, 400);
    nightscout.urgentHighMgdl = clampU16(nightscout.urgentHighMgdl, 120, 600);
    if (nightscout.lowMgdl < nightscout.urgentLowMgdl) nightscout.lowMgdl = nightscout.urgentLowMgdl;
    if (nightscout.highMgdl < nightscout.lowMgdl) nightscout.highMgdl = nightscout.lowMgdl;
    if (nightscout.urgentHighMgdl < nightscout.highMgdl) nightscout.urgentHighMgdl = nightscout.highMgdl;
}

void clearRuntimeForInactiveSources() {
    if (feedConfig.weather.source != WEATHER_FEED_OPEN_METEO || !feedsWeatherConfigured()) {
        clearWeatherRuntime();
    }

    if (!feedsNightscoutConfigured()) {
        clearNightscoutRuntime();
    }
}

bool weatherIdentityChanged(const FeedConfig &before, const FeedConfig &after) {
    return before.weather.source != after.weather.source ||
           strcmp(before.weather.query, after.weather.query) != 0 ||
           strcmp(before.weather.label, after.weather.label) != 0 ||
           fabsf(before.weather.latitude - after.weather.latitude) > 0.0001f ||
           fabsf(before.weather.longitude - after.weather.longitude) > 0.0001f ||
           before.weather.useFahrenheit != after.weather.useFahrenheit;
}

bool nightscoutIdentityChanged(const NightscoutConfig &before, const NightscoutConfig &after) {
    return before.enabled != after.enabled ||
           strcmp(before.baseUrl, after.baseUrl) != 0 ||
           strcmp(before.token, after.token) != 0;
}

bool nightscoutConfigEquals(const NightscoutConfig &left, const NightscoutConfig &right) {
    return left.enabled == right.enabled &&
           strcmp(left.baseUrl, right.baseUrl) == 0 &&
           strcmp(left.token, right.token) == 0 &&
           left.useMmol == right.useMmol &&
           left.showIob == right.showIob &&
           left.refreshMinutes == right.refreshMinutes &&
           left.refreshSeconds == right.refreshSeconds &&
           left.urgentLowMgdl == right.urgentLowMgdl &&
           left.lowMgdl == right.lowMgdl &&
           left.highMgdl == right.highMgdl &&
           left.urgentHighMgdl == right.urgentHighMgdl;
}

bool weatherConfigEquals(const WeatherFeedConfig &left, const WeatherFeedConfig &right) {
    return left.source == right.source &&
           strcmp(left.query, right.query) == 0 &&
           strcmp(left.label, right.label) == 0 &&
           fabsf(left.latitude - right.latitude) <= 0.0001f &&
           fabsf(left.longitude - right.longitude) <= 0.0001f &&
           left.refreshMinutes == right.refreshMinutes &&
           left.useFahrenheit == right.useFahrenheit;
}

bool feedConfigEquals(const FeedConfig &left, const FeedConfig &right) {
    if (left.version != right.version ||
        !weatherConfigEquals(left.weather, right.weather) ||
        !nightscoutConfigEquals(left.nightscout, right.nightscout)) {
        return false;
    }

    return true;
}

void updateDraftState() {
    feedDraftActive = !feedConfigEquals(feedConfig, savedFeedConfig);
}

void captureSavedConfig() {
    memcpy(&savedFeedConfig, &feedConfig, sizeof(savedFeedConfig));
    feedDraftActive = false;
}

void fillWeatherDataJson(JsonObject root, const WeatherData &weather) {
    root["location"] = weather.location;
    root["condition"] = weather.condition;
    root["temperature"] = weather.temperature;
    root["high"] = weather.high;
    root["low"] = weather.low;
    root["rainChance"] = weather.rainChance;
}

void fillWeatherStatusJson(JsonObject root, const WeatherFeedRuntime &runtime) {
    root["syncing"] = runtime.syncing;
    root["hasData"] = runtime.hasData;
    root["lastError"] = runtime.lastError;
    root["lastAttemptAgeSec"] = runtime.lastAttemptMs > 0 ? static_cast<long>((millis() - runtime.lastAttemptMs) / 1000UL) : -1;
    root["lastSuccessAgeSec"] = runtime.lastSuccessMs > 0 ? static_cast<long>((millis() - runtime.lastSuccessMs) / 1000UL) : -1;

    JsonObject data = root["data"].to<JsonObject>();
    if (runtime.hasData) {
        fillWeatherDataJson(data, runtime.data);
    }
}

void fillNightscoutStatusJson(JsonObject root, const NightscoutRuntime &runtime) {
    root["syncing"] = runtime.syncing;
    root["hasData"] = runtime.hasData;
    root["lastError"] = runtime.lastError;
    root["sgv"] = runtime.sgv;
    root["direction"] = runtime.direction;
    root["lastAttemptAgeSec"] = runtime.lastAttemptMs > 0 ? static_cast<long>((millis() - runtime.lastAttemptMs) / 1000UL) : -1;
    root["lastSuccessAgeSec"] = runtime.lastSuccessMs > 0 ? static_cast<long>((millis() - runtime.lastSuccessMs) / 1000UL) : -1;
}

void fillConfigJson(JsonObject root) {
    root["version"] = feedConfig.version;

    JsonObject weather = root["weather"].to<JsonObject>();
    weather["source"] = weatherSourceToString(feedConfig.weather.source);
    weather["query"] = feedConfig.weather.query;
    weather["label"] = feedConfig.weather.label;
    weather["latitude"] = feedConfig.weather.latitude;
    weather["longitude"] = feedConfig.weather.longitude;
    weather["refreshMinutes"] = feedConfig.weather.refreshMinutes;
    weather["useFahrenheit"] = feedConfig.weather.useFahrenheit;

    JsonObject nightscout = root["nightscout"].to<JsonObject>();
    nightscout["enabled"] = feedConfig.nightscout.enabled;
    nightscout["baseUrl"] = feedConfig.nightscout.baseUrl;
    nightscout["token"] = feedConfig.nightscout.token;
    nightscout["useMmol"] = feedConfig.nightscout.useMmol;
    nightscout["showIob"] = feedConfig.nightscout.showIob;
    nightscout["refreshMinutes"] = feedConfig.nightscout.refreshMinutes;
    nightscout["refreshSeconds"] = feedConfig.nightscout.refreshSeconds;
    nightscout["urgentLowMgdl"] = feedConfig.nightscout.urgentLowMgdl;
    nightscout["lowMgdl"] = feedConfig.nightscout.lowMgdl;
    nightscout["highMgdl"] = feedConfig.nightscout.highMgdl;
    nightscout["urgentHighMgdl"] = feedConfig.nightscout.urgentHighMgdl;
}

void fillStatusJson(JsonObject root) {
    JsonObject weather = root["weather"].to<JsonObject>();
    fillWeatherStatusJson(weather, feedRuntime.weather);

    JsonObject nightscout = root["nightscout"].to<JsonObject>();
    fillNightscoutStatusJson(nightscout, feedRuntime.nightscout);
}

bool writeJsonToFile(const char *path, JsonDocument &doc) {
    char tempPath[64];
    if (!buildTempPath(path, tempPath, sizeof(tempPath))) {
        return false;
    }

    LittleFS.remove(tempPath);

    File file = LittleFS.open(tempPath, "w");
    if (!file) {
        return false;
    }

    size_t expectedBytes = measureJson(doc);
    bool success = !doc.overflowed() && serializeJson(doc, file) == expectedBytes;
    file.flush();
    file.close();
    if (!success) {
        LittleFS.remove(tempPath);
        return false;
    }

    if (!LittleFS.rename(tempPath, path)) {
        return false;
    }

    return true;
}

template <typename TObject>
bool loadJsonFile(const char *path, TObject object) {
    auto tryLoad = [&](const char *candidatePath) {
        File file = LittleFS.open(candidatePath, "r");
        if (!file) {
            return false;
        }

        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, file);
        file.close();
        if (error) {
            return false;
        }

        object(doc.as<JsonObjectConst>());
        return true;
    };

    if (tryLoad(path)) {
        return true;
    }

    char tempPath[64];
    if (!buildTempPath(path, tempPath, sizeof(tempPath))) {
        return false;
    }

    if (!LittleFS.exists(tempPath) || !tryLoad(tempPath)) {
        return false;
    }

    LittleFS.remove(path);
    LittleFS.rename(tempPath, path);
    return true;
}

const char* weatherCodeToText(int code) {
    switch (code) {
        case 0:
            return "Clear";
        case 1:
            return "Mostly clear";
        case 2:
            return "Partly cloudy";
        case 3:
            return "Overcast";
        case 45:
        case 48:
            return "Fog";
        case 51:
        case 53:
        case 55:
        case 56:
        case 57:
            return "Drizzle";
        case 61:
        case 63:
        case 65:
        case 66:
        case 67:
            return "Rain";
        case 71:
        case 73:
        case 75:
        case 77:
            return "Snow";
        case 80:
        case 81:
        case 82:
            return "Showers";
        case 85:
        case 86:
            return "Snow showers";
        case 95:
            return "Thunderstorm";
        case 96:
        case 99:
            return "Storm";
        default:
            return "Weather";
    }
}

// Bound parser work as well as network waits; filtered JSON still consumes the
// entire response. Yield every 256 bytes and reject unexpectedly large bodies.
class FeedJsonStream : public Stream {
public:
    explicit FeedJsonStream(Stream &source) : source_(source) { setTimeout(kHttpTimeoutMs); }
    int available() override { return exhausted() ? 0 : source_.available(); }
    int peek() override { return exhausted() ? -1 : source_.peek(); }
    int read() override {
        if (exhausted()) return -1;
        int value = source_.read();
        if (value >= 0 && (++consumed_ % 256) == 0) yield();
        return value;
    }
    size_t readBytes(char *buffer, size_t length) override {
        size_t received = 0;
        uint32_t lastProgress = millis();
        while (received < length && !exhausted()) {
            int value = read();
            if (value >= 0) {
                buffer[received++] = static_cast<char>(value);
                lastProgress = millis();
            } else {
                if (millis() - lastProgress >= kHttpTimeoutMs) break;
                yield();
            }
        }
        return received;
    }
    size_t write(uint8_t) override { return 0; }
    bool exhausted() const { return consumed_ >= 131072UL || activeRequestExpired; }
private:
    Stream &source_;
    uint32_t consumed_ = 0;
};

bool tlsHeapReady() {
    uint32_t freeHeap = 0;
    uint32_t largestBlock = 0;
    ESP.getHeapStats(&freeHeap, &largestBlock);
    // The crypto stack, SSL engine and certificate validator use primary DRAM.
    // Spare IRAM for record buffers cannot replace their allocation headroom.
    if (freeHeap < kHttpsMinDramHeapBytes || largestBlock < kHttpsMinDramBlockBytes) {
        return false;
    }
#ifdef UMM_HEAP_IRAM
    // BearSSL prefers the secondary heap for its receive/transmit buffers.
    {
        HeapSelectIram selected;
        uint32_t iramFree = 0;
        uint32_t iramLargest = 0;
        ESP.getHeapStats(&iramFree, &iramLargest);
        freeHeap += iramFree;
        largestBlock = max(largestBlock, iramLargest);
    }
#endif
    logPrintf(F("TLS heap: free=%u max=%u"), freeHeap, largestBlock);
    // BearSSL adds 325 bytes of TLS record overhead to the receive buffer.
    return freeHeap >= kHttpsMinFreeHeapBytes &&
           largestBlock >= static_cast<uint32_t>(kHttpsDefaultRecvBufferBytes) + 325UL;
}


bool waitForTlsHeap(String *error) {
    uint32_t started = millis();
    while (true) {
        if (feedJobsCancelled) {
            if (error) *error = F("Feed job cancelled");
            return false;
        }
        if (tlsHeapReady()) return true;
        if (millis() - started >= kHttpsHeapWaitMs) {
            uint32_t freeHeap = 0, largestBlock = 0;
            ESP.getHeapStats(&freeHeap, &largestBlock);
            logPrintf(F("TLS deferred: DRAM free=%u max=%u"), freeHeap, largestBlock);
            if (error) *error = F("Low heap, retrying later");
            return false;
        }
        // Let closed API/provider sockets release their buffers before the next TLS request.
        delay(50);
    }
}

bool httpGetJson(const String &url,
                 JsonDocument &doc,
                 String *error,
                 JsonDocument *filter = nullptr) {
    if (feedJobsCancelled) {
        if (error) *error = F("Feed job cancelled");
        return false;
    }
    if (filter != nullptr) filter->shrinkToFit();
    struct RequestGuard {
        RequestGuard() { activeRequestStartedMs = millis(); activeRequestExpired = false; }
        ~RequestGuard() { activeHttpClient = nullptr; }
    } requestGuard;
    struct ActiveClientGuard {
        explicit ActiveClientGuard(FeedHttpClient *client) { activeHttpClient = client; }
        ~ActiveClientGuard() { activeHttpClient = nullptr; activeTlsClient = nullptr; }
    };
    auto finishJsonRequest = [&](HTTPClient &http, const char *transportLabel) {
        http.addHeader("Accept", "application/json");
        int responseCode = http.GET();
        if (responseCode != HTTP_CODE_OK) {
            String httpError = responseCode < 0 ? HTTPClient::errorToString(responseCode) : "";
            if (error != nullptr) {
                if (feedJobsCancelled) *error = F("Feed job cancelled");
                else if (activeRequestExpired) *error = F("Request deadline exceeded");
                else *error = responseCode < 0
                             ? ("HTTP " + String(responseCode) + " " + httpError)
                             : ("HTTP " + String(responseCode));
            }
            if (responseCode < 0 && httpError.length() > 0) {
                logPrintf(F("%s GET failed: %d %s (%s)"),
                          transportLabel,
                          responseCode,
                          httpError.c_str(),
                          "feed request");
            } else {
                logPrintf(F("%s GET failed: %d (%s)"), transportLabel, responseCode, "feed request");
            }
            if (responseCode < 0 && activeTlsClient != nullptr) {
                char tlsMessage[80] = {};
                int tlsError = activeTlsClient->getLastSSLError(tlsMessage, sizeof(tlsMessage));
                logPrintf(F("TLS error: %d %s"), tlsError, tlsMessage);
                if (tlsError != 0 && error != nullptr && !feedJobsCancelled && !activeRequestExpired) {
                    *error = String(F("TLS ")) + String(tlsError) + " " + tlsMessage;
                }
            }
            http.end();
            return false;
        }

        if (http.getSize() > 131072) {
            if (error) *error = F("Response too large");
            http.end();
            return false;
        }
        doc.clear();
        FeedJsonStream body(http.getStream());
        DeserializationError jsonError =
            filter == nullptr
                ? deserializeJson(doc, body)
                : deserializeJson(doc,
                                  body,
                                  DeserializationOption::Filter(filter->as<JsonVariantConst>()));
        http.end();

        if (jsonError || body.exhausted()) {
            if (error != nullptr) {
                *error = body.exhausted() ? (activeRequestExpired ? "Request deadline exceeded" : "Response too large") : jsonError == DeserializationError::NoMemory
                             ? "JSON memory exhausted"
                             : String(jsonError.c_str());
            }
            logPrintf(F("%s JSON parse failed: %s (%s)"), transportLabel, jsonError.c_str(), "feed request");
            return false;
        }

        if (activeRequestExpired) {
            if (error) *error = F("Request deadline exceeded");
            return false;
        }
        return true;
    };

    FeedHttpClient http;
    http.setTimeout(kHttpTimeoutMs);
    http.useHTTP10(true);
    http.setUserAgent(F("SugarCube/1.0"));

    if (url.startsWith("http://")) {
        WiFiClient client;
        if (!http.begin(client, url)) {
            if (error != nullptr) {
                *error = F("HTTP begin failed");
            }
            return false;
        }
        ActiveClientGuard activeGuard(&http);

        return finishJsonRequest(http, "HTTP");
    }

    if (!url.startsWith("https://")) {
        if (error != nullptr) {
            *error = F("Unsupported URL scheme");
        }
        return false;
    }

    struct DisplayHeapGuard {
        DisplayHeapGuard() {
            displaySuspendDynamicResources();
        }

        ~DisplayHeapGuard() {
            displayResumeDynamicResources();
        }
    } displayHeapGuard;

    if (!waitForTlsHeap(error)) return false;
    std::unique_ptr<FeedTlsClient> client(new (std::nothrow) FeedTlsClient());
    if (!client) {
        if (error) *error = F("TLS client allocation failed");
        return false;
    }
    client->setInsecure();
    client->setBufferSizes(kHttpsDefaultRecvBufferBytes, kHttpsCompactXmitBufferBytes);
    if (!http.begin(*client, url)) {
        if (error != nullptr) {
            *error = F("HTTP begin failed");
        }
        return false;
    }
    ActiveClientGuard activeGuard(&http);
    activeTlsClient = client.get();

    return finishJsonRequest(http, "HTTPS");
}

bool fetchWeatherSearchJson(const String &query, JsonDocument &providerDoc, String *error) {
    String trimmedQuery = query;
    trimmedQuery.trim();

    if (trimmedQuery.length() < 2) {
        if (error != nullptr) {
            *error = F("Enter at least 2 characters");
        }
        return false;
    }

    String url = "http://geocoding-api.open-meteo.com/v1/search?name=" + urlEncode(trimmedQuery) +
                 "&count=6&language=en&format=json";
    JsonDocument filter;
    buildWeatherSearchFilter(filter);
    return httpGetJson(url, providerDoc, error, &filter);
}

bool resolveWeatherLocationFromQuery(String *error) {
    JsonDocument providerDoc;
    String requestError;
    if (!fetchWeatherSearchJson(feedConfig.weather.query, providerDoc, &requestError)) {
        if (error != nullptr) {
            *error = requestError;
        }
        return false;
    }

    JsonArrayConst providerResults = providerDoc["results"].as<JsonArrayConst>();
    if (providerResults.isNull() || providerResults.size() == 0) {
        if (error != nullptr) {
            *error = F("City not found");
        }
        return false;
    }

    JsonObjectConst item = providerResults[0].as<JsonObjectConst>();
    if (item.isNull() || item["latitude"].isNull() || item["longitude"].isNull()) {
        if (error != nullptr) {
            *error = F("Location payload incomplete");
        }
        return false;
    }

    feedConfig.weather.latitude = item["latitude"].as<float>();
    feedConfig.weather.longitude = item["longitude"].as<float>();

    String resolvedLabel = buildWeatherLocationLabel(item);
    if (!resolvedLabel.isEmpty()) {
        copyString(feedConfig.weather.label, sizeof(feedConfig.weather.label), resolvedLabel.c_str());
    }

    updateDraftState();
    return true;
}

bool syncWeather(String *error) {
    if (WiFi.status() != WL_CONNECTED) {
        if (error != nullptr) {
            *error = F("WiFi disconnected");
        }
        return false;
    }

    if (feedConfig.weather.source != WEATHER_FEED_OPEN_METEO || !feedsWeatherConfigured()) {
        if (error != nullptr) {
            *error = F("Weather feed not configured");
        }
        return false;
    }

    WeatherFeedRuntime &runtime = feedRuntime.weather;
    runtime.syncing = true;
    runtime.lastAttemptMs = millis();
    runtime.lastError[0] = '\0';

    if (!weatherCoordinatesValid(feedConfig.weather.latitude, feedConfig.weather.longitude)) {
        String locationError;
        if (!resolveWeatherLocationFromQuery(&locationError)) {
            runtime.syncing = false;
            copyString(runtime.lastError, sizeof(runtime.lastError), locationError.c_str());
            if (error != nullptr) {
                *error = locationError;
            }
            return false;
        }
    }

    String url = "http://api.open-meteo.com/v1/forecast?latitude=" + String(feedConfig.weather.latitude, 4) +
                 "&longitude=" + String(feedConfig.weather.longitude, 4) +
                 "&current=temperature_2m,weather_code" +
                 "&daily=temperature_2m_max,temperature_2m_min,precipitation_probability_max" +
                 "&forecast_days=1&timezone=auto";
    if (feedConfig.weather.useFahrenheit) {
        url += "&temperature_unit=fahrenheit";
    } else {
        url += "&temperature_unit=celsius";
    }

    JsonDocument doc;
    String requestError;
    JsonDocument filter;
    buildWeatherForecastFilter(filter);
    if (!httpGetJson(url, doc, &requestError, &filter)) {
        runtime.syncing = false;
        copyString(runtime.lastError, sizeof(runtime.lastError), requestError.c_str());
        if (error != nullptr) {
            *error = requestError;
        }
        return false;
    }

    JsonObjectConst current = doc["current"].as<JsonObjectConst>();
    JsonObjectConst daily = doc["daily"].as<JsonObjectConst>();
    JsonArrayConst highs = daily["temperature_2m_max"].as<JsonArrayConst>();
    JsonArrayConst lows = daily["temperature_2m_min"].as<JsonArrayConst>();
    JsonArrayConst rain = daily["precipitation_probability_max"].as<JsonArrayConst>();

    if (current.isNull() || current["temperature_2m"].isNull()) {
        runtime.syncing = false;
        copyString(runtime.lastError, sizeof(runtime.lastError), "Weather payload incomplete");
        if (error != nullptr) {
            *error = runtime.lastError;
        }
        return false;
    }

    memset(&runtime.data, 0, sizeof(runtime.data));
    const char *locationLabel = feedConfig.weather.label[0] != '\0' ? feedConfig.weather.label : feedConfig.weather.query;
    copyString(runtime.data.location, sizeof(runtime.data.location), locationLabel);
    copyString(runtime.data.condition, sizeof(runtime.data.condition), weatherCodeToText(current["weather_code"] | 0));
    runtime.data.temperature = static_cast<int>(roundf(current["temperature_2m"].as<float>()));
    runtime.data.high = highs.isNull() || highs.size() == 0
                            ? runtime.data.temperature
                            : static_cast<int>(roundf(highs[0].as<float>()));
    runtime.data.low = lows.isNull() || lows.size() == 0
                           ? runtime.data.temperature
                           : static_cast<int>(roundf(lows[0].as<float>()));
    runtime.data.rainChance = rain.isNull() || rain.size() == 0
                                  ? 0
                                  : constrain(static_cast<int>(roundf(rain[0].as<float>())), 0, 100);

    runtime.hasData = true;
    runtime.syncing = false;
    runtime.lastSuccessMs = millis();
    runtime.lastError[0] = '\0';
    logPrintf(F("Weather sync OK: %s %d"), runtime.data.location, runtime.data.temperature);
    return true;
}

bool nightscoutIobNumber(JsonVariantConst value, float &out) {
    if (value.isNull() || value.is<JsonObjectConst>() || value.is<JsonArrayConst>() || value.is<const char *>()) {
        return false;
    }
    const float number = value.as<float>();
    if (!isfinite(number)) return false;
    out = number;
    return true;
}

bool nightscoutStatusIob(JsonVariantConst status, float &out) {
    if (nightscoutIobNumber(status["loop"]["iob"]["iob"], out)) return true;
    if (nightscoutIobNumber(status["loop"]["iob"], out)) return true;
    JsonVariantConst openapsIob = status["openaps"]["iob"];
    if (openapsIob.is<JsonArrayConst>()) openapsIob = openapsIob[0];
    if (nightscoutIobNumber(openapsIob["iob"], out)) return true;
    if (nightscoutIobNumber(openapsIob, out)) return true;
    if (nightscoutIobNumber(status["iob"], out)) return true;
    if (nightscoutIobNumber(status["pump"]["iob"], out)) return true;
    return false;
}

// One TLS session for glucose and insulin. A second handshake does not fit in the
// DRAM left after the first Nightscout response.
struct NightscoutTlsSession {
    FeedHttpClient http;
    std::unique_ptr<FeedTlsClient> client;
    bool suspended = false;

    ~NightscoutTlsSession() { close(); }

    void close() {
        // Abort instead of a graceful close so the socket does not stay in
        // TIME_WAIT and split the DRAM block the next handshake needs.
        if (client) client->abortTransport();
        http.setReuse(false);
        http.end();
        client.reset();
        if (activeTlsClient) activeTlsClient = nullptr;
        if (activeHttpClient == &http) activeHttpClient = nullptr;
        if (suspended) {
            displayResumeDynamicResources();
            suspended = false;
        }
    }

    bool open(String *error) {
        if (client) return true;
        displaySuspendDynamicResources();
        suspended = true;
        if (!waitForTlsHeap(error)) return false;
        client.reset(new (std::nothrow) FeedTlsClient());
        if (!client) {
            if (error) *error = F("TLS client allocation failed");
            return false;
        }
        client->setInsecure();
        client->setBufferSizes(kHttpsDefaultRecvBufferBytes, kHttpsCompactXmitBufferBytes);
        http.setTimeout(kHttpTimeoutMs);
        http.useHTTP10(false);
        http.setReuse(true);
        http.setUserAgent(F("SugarCube/1.0"));
        return true;
    }

    bool getJson(const String &url, JsonDocument &doc, String *error, JsonDocument *filter) {
        if (!open(error)) return false;
        activeRequestStartedMs = millis();
        activeRequestExpired = false;
        activeHttpClient = &http;
        activeTlsClient = client.get();
        if (!http.begin(*client, url)) {
            if (error) *error = F("HTTP begin failed");
            return false;
        }
        http.addHeader("Accept", "application/json");
        const int responseCode = http.GET();
        if (responseCode != HTTP_CODE_OK) {
            String httpError = responseCode < 0 ? HTTPClient::errorToString(responseCode) : "";
            if (error) {
                if (responseCode == 401) {
                    *error = F("This site is private. Add your token in the dashboard.");
                } else {
                    *error = responseCode < 0
                                 ? ("HTTP " + String(responseCode) + " " + httpError)
                                 : ("HTTP " + String(responseCode));
                }
            }
            http.setReuse(false);
            http.end();
            http.setReuse(true);
            return false;
        }
        doc.clear();
        FeedJsonStream body(http.getStream());
        DeserializationError jsonError =
            filter == nullptr
                ? deserializeJson(doc, body)
                : deserializeJson(doc, body, DeserializationOption::Filter(filter->as<JsonVariantConst>()));
        http.end();
        if (jsonError || body.exhausted()) {
            if (error) {
                *error = body.exhausted()
                             ? (activeRequestExpired ? String(F("Request deadline exceeded")) : String(F("Response too large")))
                             : String(jsonError.c_str());
            }
            return false;
        }
        return true;
    }
};

void fetchNightscoutIob(NightscoutTlsSession &session, JsonDocument &doc, const String &tokenSuffix) {
    NightscoutRuntime &runtime = feedRuntime.nightscout;
    if (!feedConfig.nightscout.showIob) {
        runtime.iobValid = false;
        return;
    }

    // One matching row only. A dozen full status documents are larger than the reader allows,
    // and the very newest row is often a pump upload with no insulin field.
    String url = String(feedConfig.nightscout.baseUrl) +
                 "/api/v1/devicestatus.json?count=1" +
                 "&find[$or][0][loop.iob.iob][$exists]=true" +
                 "&find[$or][1][openaps.iob.iob][$exists]=true" +
                 "&find[$or][2][iob][$exists]=true" +
                 tokenSuffix;
    JsonDocument filter;
    filter[0]["loop"]["iob"]["iob"] = true;
    filter[0]["loop"]["iob"] = true;
    filter[0]["openaps"]["iob"] = true;
    filter[0]["iob"] = true;
    filter[0]["pump"]["iob"] = true;
    doc.clear();
    String requestError;
    if (!session.getJson(url, doc, &requestError, &filter)) {
        logPrintf(F("Nightscout IOB request failed: %s"), requestError.c_str());
        return;
    }

    for (JsonVariantConst status : doc.as<JsonArrayConst>()) {
        float iob = 0.0f;
        if (!nightscoutStatusIob(status, iob)) continue;
        runtime.iob = iob;
        runtime.iobValid = true;
        return;
    }
    logPrintf(F("Nightscout IOB not in the latest status"));
}

bool syncNightscout(String *error) {
    if (WiFi.status() != WL_CONNECTED) {
        if (error != nullptr) {
            *error = F("WiFi disconnected");
        }
        return false;
    }

    if (!feedsNightscoutConfigured()) {
        if (error != nullptr) {
            *error = F("Nightscout not configured");
        }
        return false;
    }

    NightscoutRuntime &runtime = feedRuntime.nightscout;
    runtime.syncing = true;
    runtime.lastAttemptMs = millis();

    auto fail = [&](const String &message) {
        runtime.syncing = false;
        copyString(runtime.lastError, sizeof(runtime.lastError), message.c_str());
        if (error != nullptr) {
            *error = message;
        }
        return false;
    };

    String tokenSuffix;
    if (feedConfig.nightscout.token[0] != '\0') {
        tokenSuffix = "&token=" + urlEncode(String(feedConfig.nightscout.token));
    }
    String url = String(feedConfig.nightscout.baseUrl) + "/api/v1/entries/sgv.json?count=" +
                 String(kNightscoutHistoryMax) + tokenSuffix;

    JsonDocument doc;
    JsonDocument filter;
    buildNightscoutFilter(filter);
    String requestError;
    NightscoutTlsSession session;
    if (!session.getJson(url, doc, &requestError, &filter)) {
        return fail(requestError);
    }

    JsonArrayConst entries = doc.as<JsonArrayConst>();
    if (entries.isNull() || entries.size() == 0) {
        return fail(F("No readings returned"));
    }

    uint16_t history[kNightscoutHistoryMax] = {};
    uint8_t historyCount = 0;
    for (JsonVariantConst item : entries) {
        if (historyCount >= kNightscoutHistoryMax) {
            break;
        }
        JsonObjectConst entry = item.as<JsonObjectConst>();
        if (entry.isNull() || entry["sgv"].isNull()) {
            continue;
        }
        float value = entry["sgv"].as<float>();
        if (!isfinite(value) || value < 20.0f || value > 700.0f) {
            continue;
        }
        history[historyCount++] = static_cast<uint16_t>(roundf(value));
    }
    if (historyCount == 0) {
        return fail(F("Reading out of range"));
    }

    JsonObjectConst latest = entries[0].as<JsonObjectConst>();
    double dateMs = latest["date"].as<double>();
    uint32_t readingEpoch = dateMs > 100000000000.0 ? static_cast<uint32_t>(dateMs / 1000.0) : 0;

    runtime.sgv = history[0];
    memcpy(runtime.history, history, sizeof(runtime.history));
    runtime.historyCount = historyCount;
    copyString(runtime.direction, sizeof(runtime.direction), latest["direction"] | "");
    runtime.readingEpoch = readingEpoch;
    runtime.hasData = true;
    runtime.lastSuccessMs = millis();
    runtime.lastError[0] = '\0';

    fetchNightscoutIob(session, doc, tokenSuffix);  // same TLS session; never fails the glucose sync
    runtime.syncing = false;
    logPrintf(F("Nightscout sync OK: %u (%u readings)"),
              static_cast<unsigned>(runtime.sgv), static_cast<unsigned>(historyCount));
    return true;
}

void applyWeatherObject(JsonObjectConst weather) {
    if (weather.isNull()) {
        return;
    }

    if (!weather["source"].isNull()) {
        feedConfig.weather.source = weatherSourceFromVariant(weather["source"]);
    }
    if (!weather["query"].isNull()) {
        copyString(feedConfig.weather.query, sizeof(feedConfig.weather.query), weather["query"]);
    }
    if (!weather["label"].isNull()) {
        copyString(feedConfig.weather.label, sizeof(feedConfig.weather.label), weather["label"]);
    }
    if (!weather["latitude"].isNull()) {
        feedConfig.weather.latitude = weather["latitude"].as<float>();
    }
    if (!weather["longitude"].isNull()) {
        feedConfig.weather.longitude = weather["longitude"].as<float>();
    }
    if (!weather["refreshMinutes"].isNull()) {
        feedConfig.weather.refreshMinutes = weather["refreshMinutes"].as<uint16_t>();
    }
    if (!weather["useFahrenheit"].isNull()) {
        feedConfig.weather.useFahrenheit = weather["useFahrenheit"].as<bool>();
    }
}

void applyNightscoutObject(JsonObjectConst nightscout) {
    if (nightscout.isNull()) {
        return;
    }

    NightscoutConfig &config = feedConfig.nightscout;
    if (!nightscout["enabled"].isNull()) {
        config.enabled = nightscout["enabled"].as<bool>();
    }
    if (!nightscout["baseUrl"].isNull()) {
        copyString(config.baseUrl, sizeof(config.baseUrl), nightscout["baseUrl"]);
    }
    if (!nightscout["token"].isNull()) {
        copyString(config.token, sizeof(config.token), nightscout["token"]);
    }
    if (!nightscout["useMmol"].isNull()) {
        config.useMmol = nightscout["useMmol"].as<bool>();
    }
    if (!nightscout["showIob"].isNull()) {
        config.showIob = nightscout["showIob"].as<bool>();
    }
    if (!nightscout["refreshMinutes"].isNull()) {
        config.refreshMinutes = nightscout["refreshMinutes"].as<uint16_t>();
    }
    if (!nightscout["refreshSeconds"].isNull()) {
        config.refreshSeconds = clampNightscoutRefresh(nightscout["refreshSeconds"].as<uint16_t>());
    }
    if (!nightscout["urgentLowMgdl"].isNull()) {
        config.urgentLowMgdl = nightscout["urgentLowMgdl"].as<uint16_t>();
    }
    if (!nightscout["lowMgdl"].isNull()) {
        config.lowMgdl = nightscout["lowMgdl"].as<uint16_t>();
    }
    if (!nightscout["highMgdl"].isNull()) {
        config.highMgdl = nightscout["highMgdl"].as<uint16_t>();
    }
    if (!nightscout["urgentHighMgdl"].isNull()) {
        config.urgentHighMgdl = nightscout["urgentHighMgdl"].as<uint16_t>();
    }
}

void applyConfigObject(JsonObjectConst root) {
    applyWeatherObject(root["weather"].as<JsonObjectConst>());
    applyNightscoutObject(root["nightscout"].as<JsonObjectConst>());
    normalizeConfig();
}

constexpr uint16_t kWeatherJob = 1;
constexpr uint16_t kNightscoutJob = 2;

void runFeedJob() {
    if (searchStatus == FEED_SEARCH_PENDING && activeSyncMask == 0) {
        bool success = feedsSearchWeatherLocations(searchQuery, searchJson, &searchError);
        searchStatus = success ? FEED_SEARCH_READY : FEED_SEARCH_FAILED;
    } else {
        bool anySuccess = false;
        String firstError;
        auto record = [&](bool success, const String &error) {
            anySuccess = anySuccess || success;
            if (!success && firstError.isEmpty()) firstError = error;
        };
        String error;
        if (activeSyncMask & kWeatherJob) {
            record(syncWeather(&error), error);
            yield();
        }
        if (activeSyncMask & kNightscoutJob) {
            error = "";
            record(syncNightscout(&error), error);
            yield();
        }
        lastSyncSuccess = lastSyncSuccess && anySuccess && firstError.isEmpty();
        if (lastSyncError.isEmpty()) lastSyncError = firstError;
    }
    if (feedJobsCancelled) {
        lastSyncSuccess = false;
        lastSyncError = "Feed job cancelled";
        if (searchStatus == FEED_SEARCH_READY || searchStatus == FEED_SEARCH_PENDING) {
            searchStatus = FEED_SEARCH_FAILED;
            searchError = lastSyncError;
        }
    }
    feedJobComplete = true;
}

void pumpFeedJob() {
    if (!feedJobRunning) {
        if (pendingSyncMask == 0 && searchStatus != FEED_SEARCH_PENDING) return;
        if (pendingSyncMask && !feedBatchActive) {
            feedBatchActive = true;
            lastSyncSuccess = true;
            lastSyncError = "";
        }
        feedContinuation.reset(new (std::nothrow) cont_t);
        if (!feedContinuation) {
            lastSyncSuccess = false;
            lastSyncError = "Feed worker allocation failed";
            pendingSyncMask = 0;
            feedBatchActive = false;
            if (searchStatus == FEED_SEARCH_PENDING) {
                searchStatus = FEED_SEARCH_FAILED;
                searchError = lastSyncError;
            }
            return;
        }
        cont_init(feedContinuation.get());
        activeSyncMask = 0;
        if (pendingSyncMask) {
            activeSyncMask = pendingSyncMask & (0U - pendingSyncMask);  // lowest pending job first
            pendingSyncMask &= ~activeSyncMask;
        }
        feedJobRunning = true;
        feedJobComplete = false;
    }
    if (activeHttpClient && (feedJobsCancelled || millis() - activeRequestStartedMs >= kRequestDeadlineMs)) {
        activeRequestExpired = true;
        if (activeTlsClient) activeTlsClient->abortTransport();
        else activeHttpClient->abortTransport();
    }
    // Every core yield suspends this worker to this call site. The main
    // continuation must be restored before returning to application code.
    struct ContinuationGuard {
        cont_t *previous;
        explicit ContinuationGuard(cont_t *worker) : previous(g_pcont) { g_pcont = worker; }
        ~ContinuationGuard() { g_pcont = previous; }
    };
    {
        ContinuationGuard guard(feedContinuation.get());
        cont_run(feedContinuation.get(), runFeedJob);
        cont_check(feedContinuation.get());
    }
    if (feedJobComplete) {
        feedJobRunning = false;
        activeSyncMask = 0;
        // Release the 4 KB worker stack between feeds so free DRAM can coalesce.
        feedContinuation.reset();
        if (feedBatchActive && pendingSyncMask == 0) {
            if (!lastSyncSuccess) logPrintf(F("Feed sync failed: %s"), lastSyncError.c_str());
            feedBatchActive = false;
        }
    }
}

}  // namespace

void feedsResetToDefaults() {
    setConfigDefaults();
    clearAllRuntime();
    feedsStartupReadyAtMs = 0;
}

void feedsInit() {
    feedsResetToDefaults();
    if (!feedsLoadConfig()) {
        feedsSaveConfig();
    }
}

bool feedsLoadConfig() {
    setConfigDefaults();
    clearAllRuntime();

    bool success = loadJsonFile(FEEDS_CONFIG_PATH, [](JsonObjectConst root) {
        applyConfigObject(root);
    });

    normalizeConfig();
    clearRuntimeForInactiveSources();
    captureSavedConfig();
    return success;
}

bool feedsSaveConfig() {
    if (feedsBusy()) return false;
    normalizeConfig();

    JsonDocument doc;
    fillConfigJson(doc.to<JsonObject>());
    bool success = writeJsonToFile(FEEDS_CONFIG_PATH, doc);
    if (success) {
        captureSavedConfig();
    }
    return success;
}

bool feedsPreviewConfigJson(const String &json, String *error) {
    if (feedsBusy()) {
        if (error) *error = F("Feed job already running");
        return false;
    }
    JsonDocument doc;
    DeserializationError deserializeError = deserializeJson(doc, json);
    if (deserializeError) {
        if (error != nullptr) {
            *error = deserializeError.c_str();
        }
        return false;
    }

    FeedConfig previousConfig = feedConfig;
    memcpy(&feedConfig, &savedFeedConfig, sizeof(feedConfig));
    applyConfigObject(doc.as<JsonObjectConst>());
    clearRuntimeForInactiveSources();

    if (weatherIdentityChanged(previousConfig, feedConfig)) {
        clearWeatherRuntime();
    }

    if (nightscoutIdentityChanged(previousConfig.nightscout, feedConfig.nightscout)) {
        clearNightscoutRuntime();
    }

    feedDraftActive = !feedConfigEquals(feedConfig, savedFeedConfig);
    return true;
}

bool feedsApplyConfigJson(const String &json, String *error) {
    if (!feedsPreviewConfigJson(json, error)) {
        return false;
    }

    if (!feedsSaveConfig()) {
        if (error != nullptr) {
            *error = F("Failed to write feed config");
        }
        return false;
    }

    return true;
}

void feedsDiscardDraftChanges() {
    if (feedsBusy()) return;
    FeedConfig previousConfig = feedConfig;
    memcpy(&feedConfig, &savedFeedConfig, sizeof(feedConfig));
    clearRuntimeForInactiveSources();

    if (weatherIdentityChanged(previousConfig, feedConfig)) {
        clearWeatherRuntime();
    }

    if (nightscoutIdentityChanged(previousConfig.nightscout, feedConfig.nightscout)) {
        clearNightscoutRuntime();
    }

    feedDraftActive = false;
}

bool feedsHasDraftChanges() {
    return feedDraftActive;
}

void feedsBuildStateJson(String &json) {
    JsonDocument doc;
    fillConfigJson(doc["config"].to<JsonObject>());
    fillStatusJson(doc["status"].to<JsonObject>());
    JsonObject meta = doc["meta"].to<JsonObject>();
    meta["hasDraft"] = feedDraftActive;
    meta["syncPending"] = feedsBusy();
    meta["lastSyncSuccess"] = lastSyncSuccess;
    meta["lastSyncError"] = lastSyncError;
    json = "";
    serializeJson(doc, json);
}

bool feedsBusy() {
    return feedJobRunning || pendingSyncMask != 0 || searchStatus == FEED_SEARCH_PENDING;
}

void feedsCancelJobs() {
    feedJobsCancelled = true;
    pendingSyncMask = 0;
    if (!feedJobRunning && feedBatchActive) {
        feedBatchActive = false;
        lastSyncSuccess = false;
        lastSyncError = F("Feed job cancelled");
    }
    if (!feedJobRunning && searchStatus == FEED_SEARCH_PENDING) {
        searchStatus = FEED_SEARCH_FAILED;
        searchError = "Feed job cancelled";
    }
    // Socket-only cancellation retains BearSSL engine and worker locals until
    // the suspended network call returns and its normal destructors unwind.
    if (activeHttpClient) {
        activeRequestExpired = true;
        if (activeTlsClient) activeTlsClient->abortTransport();
        else activeHttpClient->abortTransport();
    }
}

bool feedsSyncNow(const char *scope, String *error) {
    if (feedsBusy()) {
        if (error) *error = F("Feed job already running");
        return false;
    }
    String value = scope ? scope : "all";
    value.toLowerCase();
    if (value != "all" && value != "nightscout") {
        if (error) *error = F("Invalid feed scope");
        return false;
    }
    uint16_t jobs = 0;
    if ((value == "all" || value == "nightscout") && feedsNightscoutConfigured()) jobs |= kNightscoutJob;
    if (!jobs) {
        if (error) *error = F("No live feeds configured");
        return false;
    }
    feedJobsCancelled = false;
    pendingSyncMask = jobs;
    lastSyncError = "";
    return true;
}

bool feedsQueueWeatherSearch(const String &query, String *error) {
    String trimmed = query;
    trimmed.trim();
    if (trimmed.length() < 2 || trimmed.length() > 80 || feedsBusy()) {
        if (error) *error = feedsBusy() ? "Feed job already running" : "Search must contain 2 to 80 characters";
        return false;
    }
    feedJobsCancelled = false;
    searchQuery = trimmed;
    searchJson = "";
    searchError = "";
    searchStatus = FEED_SEARCH_PENDING;
    return true;
}

FeedSearchStatus feedsWeatherSearchResult(String &json, String *error) {
    if (searchStatus == FEED_SEARCH_READY) json = searchJson;
    if (searchStatus == FEED_SEARCH_FAILED && error) *error = searchError;
    return searchStatus;
}

bool feedsSearchWeatherLocations(const String &query, String &json, String *error) {
    JsonDocument providerDoc;
    if (!fetchWeatherSearchJson(query, providerDoc, error)) {
        return false;
    }

    JsonDocument resultDoc;
    JsonArray results = resultDoc["results"].to<JsonArray>();
    JsonArrayConst providerResults = providerDoc["results"].as<JsonArrayConst>();

    for (JsonObjectConst item : providerResults) {
        JsonObject result = results.add<JsonObject>();
        String label = buildWeatherLocationLabel(item);

        result["name"] = item["name"] | "";
        result["label"] = label;
        result["country"] = item["country"] | "";
        result["admin1"] = item["admin1"] | "";
        result["latitude"] = item["latitude"] | 0.0f;
        result["longitude"] = item["longitude"] | 0.0f;
        result["timezone"] = item["timezone"] | "";
    }

    json = "";
    serializeJson(resultDoc, json);
    return true;
}

void feedsLoop() {
    uint32_t now = millis();
    if (feedsBusy()) {
        pumpFeedJob();
        return;
    }
    if (WiFi.status() != WL_CONNECTED) return;
    if (feedsStartupReadyAtMs == 0) {
        feedsStartupReadyAtMs = now + kFeedStartupGracePeriodMs;
        return;
    }
    if (static_cast<int32_t>(now - feedsStartupReadyAtMs) < 0) return;
    const uint32_t refreshMs = static_cast<uint32_t>(feedConfig.nightscout.refreshSeconds) * 1000UL;
    if (feedsNightscoutConfigured() &&
        (feedRuntime.nightscout.lastAttemptMs == 0 ||
         now - feedRuntime.nightscout.lastAttemptMs >= refreshMs)) {
        pendingSyncMask |= kNightscoutJob;
    }
    if (pendingSyncMask) feedJobsCancelled = false;
    pumpFeedJob();
}

uint8_t feedsWeatherSource() {
    return feedConfig.weather.source;
}

bool feedsNightscoutConfigured() {
    return feedConfig.nightscout.enabled && baseUrlValid(String(feedConfig.nightscout.baseUrl));
}

bool feedsWeatherConfigured() {
    if (feedConfig.weather.source != WEATHER_FEED_OPEN_METEO) {
        return false;
    }

    if (weatherCoordinatesValid(feedConfig.weather.latitude, feedConfig.weather.longitude)) {
        return true;
    }

    String query = feedConfig.weather.query;
    query.trim();
    return query.length() >= 2;
}

bool feedsHasWeatherData() {
    return feedRuntime.weather.hasData;
}

bool feedsWeatherUsesFahrenheit() {
    return feedConfig.weather.source == WEATHER_FEED_OPEN_METEO && feedConfig.weather.useFahrenheit;
}

const WeatherData* feedsWeatherData() {
    return feedRuntime.weather.hasData ? &feedRuntime.weather.data : nullptr;
}

