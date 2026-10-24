// Upstream SmartClock firmware by B. van Weerd, with SugarCube changes.
// Copyright (c) 2026 B. van Weerd. Upstream code in this file is MIT. See LICENSE.
// Copyright (c) 2026 Peter Štieber. SugarCube changes are PolyForm Noncommercial 1.0.0. See LICENSE-SUGARCUBE.
// Required Notice: Copyright (c) 2026 Peter Štieber. SugarCube changes may not be sold or used commercially.

#ifndef FEEDS_H
#define FEEDS_H

#include <Arduino.h>
#include "dashboard.h"

#define FEEDS_CONFIG_PATH "/feeds-config.json"
#define FEEDS_CONFIG_VERSION 2
enum WeatherFeedSource : uint8_t {
    WEATHER_FEED_DISABLED = 0,
    WEATHER_FEED_MANUAL,
    WEATHER_FEED_OPEN_METEO
};

struct WeatherFeedConfig {
    uint8_t source;
    char query[40];
    char label[24];
    float latitude;
    float longitude;
    uint16_t refreshMinutes;
    bool useFahrenheit;
};

constexpr uint8_t kNightscoutHistoryMax = 12;

struct NightscoutConfig {
    bool enabled;
    char baseUrl[128];       // https://my-site.example.com (no trailing slash)
    char token[64];          // optional read-only access token
    bool useMmol;            // display unit only; the API always returns mg/dL
    bool showIob;            // also fetch insulin-on-board from devicestatus
    uint16_t refreshMinutes; // legacy minutes field kept in saved JSON
    uint16_t refreshSeconds; // how often to ask Nightscout for a new reading
    uint16_t urgentLowMgdl;
    uint16_t lowMgdl;
    uint16_t highMgdl;
    uint16_t urgentHighMgdl;
};

struct FeedConfig {
    uint16_t version;
    WeatherFeedConfig weather;
    NightscoutConfig nightscout;
};

struct WeatherFeedRuntime {
    bool hasData;
    bool syncing;
    uint32_t lastAttemptMs;
    uint32_t lastSuccessMs;
    char lastError[64];
    WeatherData data;
};

struct NightscoutRuntime {
    bool hasData;
    bool syncing;
    uint32_t lastAttemptMs;
    uint32_t lastSuccessMs;
    char lastError[64];
    uint16_t sgv;            // mg/dL
    char direction[20];      // Nightscout trend name, e.g. "Flat", "SingleUp"
    uint32_t readingEpoch;   // unix seconds of the CGM reading, 0 if unknown
    uint16_t history[kNightscoutHistoryMax];  // mg/dL, newest first
    uint8_t historyCount;
    bool iobValid;
    float iob;               // units of insulin on board
};

struct FeedRuntimeState {
    WeatherFeedRuntime weather;
    NightscoutRuntime nightscout;
};

extern FeedConfig feedConfig;
extern FeedRuntimeState feedRuntime;

void feedsInit();
void feedsLoop();
void feedsResetToDefaults();
bool feedsLoadConfig();
bool feedsSaveConfig();
bool feedsPreviewConfigJson(const String &json, String *error = nullptr);
bool feedsApplyConfigJson(const String &json, String *error = nullptr);
void feedsDiscardDraftChanges();
bool feedsHasDraftChanges();
void feedsBuildStateJson(String &json);
bool feedsSyncNow(const char *scope = "all", String *error = nullptr);
bool feedsBusy();
void feedsCancelJobs();
enum FeedSearchStatus : uint8_t { FEED_SEARCH_IDLE, FEED_SEARCH_PENDING, FEED_SEARCH_READY, FEED_SEARCH_FAILED };
bool feedsQueueWeatherSearch(const String &query, String *error = nullptr);
FeedSearchStatus feedsWeatherSearchResult(String &json, String *error = nullptr);
bool feedsSearchWeatherLocations(const String &query, String &json, String *error = nullptr);

uint8_t feedsWeatherSource();
bool feedsNightscoutConfigured();
bool feedsWeatherConfigured();
bool feedsHasWeatherData();
bool feedsWeatherUsesFahrenheit();
const WeatherData* feedsWeatherData();

#endif
