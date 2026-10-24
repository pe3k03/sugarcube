// Upstream SmartClock firmware by B. van Weerd.
// Copyright (c) 2026 B. van Weerd. MIT License. See LICENSE.
// The code below is unchanged from that project. See ATTRIBUTION.md.

#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <ESP8266WebServer.h>
#include "display.h" // Include for display functions

void webserverInit();
void webserverHandle();
void webserverProcessPendingActions();
bool webserverHasPendingNetworkAction();
void webserverApplyEffectiveBrightness(bool syncPreference = false);
void handleRoot();

extern ESP8266WebServer server;
extern int currentBrightness;
extern int currentTheme;
extern String apPassword; // Random AP password generated at runtime
extern bool wifiFailsafeMode; // WiFi failsafe mode flag

#endif
