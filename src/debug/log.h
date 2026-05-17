#pragma once
// Requires Arduino.h already included in the translation unit (Serial, millis)

// Log levels — set LOG_LEVEL before including or in build flags.
// 0 = off, 1 = ERROR, 2 = WARN, 3 = INFO, 4 = DEBUG
#ifndef LOG_LEVEL
#define LOG_LEVEL 3
#endif

#if LOG_LEVEL > 0
#define _LOG(label, fmt, ...) \
    Serial.printf("[%7lu] [" label "] " fmt "\r\n", millis(), ##__VA_ARGS__)
#else
#define _LOG(label, fmt, ...) do {} while (0)
#endif

#if LOG_LEVEL >= 1
#define LOG_E(fmt, ...) _LOG("ERR ", fmt, ##__VA_ARGS__)
#else
#define LOG_E(fmt, ...) do {} while (0)
#endif

#if LOG_LEVEL >= 2
#define LOG_W(fmt, ...) _LOG("WARN", fmt, ##__VA_ARGS__)
#else
#define LOG_W(fmt, ...) do {} while (0)
#endif

#if LOG_LEVEL >= 3
#define LOG_I(fmt, ...) _LOG("INFO", fmt, ##__VA_ARGS__)
#else
#define LOG_I(fmt, ...) do {} while (0)
#endif

#if LOG_LEVEL >= 4
#define LOG_D(fmt, ...) _LOG("DBG ", fmt, ##__VA_ARGS__)
#else
#define LOG_D(fmt, ...) do {} while (0)
#endif
