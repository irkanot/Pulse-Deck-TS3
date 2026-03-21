// src/ts3log.h
//----------------------------------
// Pulse Deck TS3 Source Code
// Copyright (c) 2015 Davide (Irkanot)
// All rights reserved
// Contact: support@pulse-deck-ts3.local
//----------------------------------

#pragma once

#include "common.h"

void logMessage(const char* msg, LogLevel level, ...);

#define logError(msg, ...) logMessage(msg, LogLevel_ERROR, ##__VA_ARGS__)
#define logInfo(msg, ...) logMessage(msg, LogLevel_INFO, ##__VA_ARGS__)
#define logWarning(msg, ...) logMessage(msg, LogLevel_WARNING, ##__VA_ARGS__)
#define logDebug(msg, ...) logMessage(msg, LogLevel_DEBUG, ##__VA_ARGS__)
#define logCritical(msg, ...) logMessage(msg, LogLevel_CRITICAL, ##__VA_ARGS__)


UINT checkError(UINT code, const char* msg, ...);

