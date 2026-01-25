#include <stdarg.h>
#include <cstring>
#include "log.h"

#pragma warning(disable : 4996)

CbpLogger::CbpLogger(const char *fname) 
    : handle(nullptr), loggingEnabled(true) {
    handle = fopen(fname, "w");  // Changed from "a" to "w" for truncate
    if (handle) {
        fprintf(handle, "CBP Log initialized\n");
        fflush(handle);
    }
}

CbpLogger::~CbpLogger() {
    if (handle) {
        fflush(handle);
        fclose(handle);
        handle = nullptr;
    }
}

void CbpLogger::SetLoggingEnabled(bool enabled) {
    loggingEnabled = enabled;
}

void CbpLogger::Write(LogLevel level, const char *fmt, va_list args) {
    if (!loggingEnabled || !handle) return;

    // Format the message into a temporary buffer
    char buffer[4096];
    int written = vsnprintf(buffer, sizeof(buffer), fmt, args);
    
    if (written < 0) return;  // Formatting error

    // Check for duplicate message
    if (lastMessage == buffer) {
        return;
    }

    lastMessage = buffer;

	// Write level tag and message -- ERROR as [NOTICE]
    const char* levelStr = (level == LogLevel::Error) ? "[NOTICE] " : "[INFO] ";
    fprintf(handle, "%s%s", levelStr, buffer);
    fflush(handle);
}

void CbpLogger::Info(const char *fmt...) {
    va_list argptr;
    va_start(argptr, fmt);
    Write(LogLevel::Info, fmt, argptr);
    va_end(argptr);
}

void CbpLogger::Error(const char *fmt...) {
    va_list argptr;
    va_start(argptr, fmt);
    Write(LogLevel::Error, fmt, argptr);
    va_end(argptr);
}

CbpLogger logger("Data\\F4SE\\Plugins\\cbp.log");
