// Upstream SmartClock firmware by B. van Weerd.
// Copyright (c) 2026 B. van Weerd. MIT License. See LICENSE.
// The code below is unchanged from that project. See ATTRIBUTION.md.

#ifndef AUTH_H
#define AUTH_H

#include <Arduino.h>

#define AUTH_CONFIG_PATH "/auth.json"
#define AUTH_CONFIG_VERSION 2
#define AUTH_MIN_PASSWORD_LENGTH 8
#define AUTH_MAX_PASSWORD_LENGTH 32

void authInit();
bool authVerifyPassword(const String &password);
bool authUpdatePassword(const String &currentPassword, const String &newPassword, String *error = nullptr);
bool authValidatePasswordPolicy(const String &password, String *error = nullptr);
const char* authUsername();
const char* authOtaPasswordHash();
bool authWasProvisionedThisBoot();
bool authCanRevealPassword();
const char* authProvisionedPassword();

#endif
