#include <stdarg.h>
#include <cstring>
#include "log.h"
#include "version.h"

#pragma warning(disable : 4996)

CbpLogger::CbpLogger(const char *fname) 
    : handle(nullptr), loggingEnabled(true), consolidationEnabled(true), debugEnabled(false) {
    handle = fopen(fname, "w");  // Changed from "a" to "w" for truncate
    if (handle) {
        fprintf(handle, "OpenCBP version %s Log initialized\n", OCBP_VERSION_STR);
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

void CbpLogger::SetDebugEnabled(bool enabled) {
    debugEnabled = enabled;
}

void CbpLogger::SetConsolidationEnabled(bool enabled) {
    consolidationEnabled = enabled;
    if (!consolidationEnabled) {
        lastMessage.clear();
    }
}

void CbpLogger::Write(LogLevel level, const char *fmt, va_list args) {
    if (!loggingEnabled || !handle) return;

    // Respect debug filter: skip debug messages when disabled
    if (level == LogLevel::Debug && !debugEnabled) return;

	// Format the message into a temporary buffer. copy the args list so multiple messages can be handled at once.
    char buffer[4096]; 
    va_list argsCopy;
    va_copy(argsCopy, args);

    int written = vsnprintf(buffer, sizeof(buffer), fmt, argsCopy);

    va_end(argsCopy);  

    if (written < 0) return;  // Formatting error

    // Check for duplicate message if consolidation is enabled
    if (consolidationEnabled && lastMessage == buffer) {
        return;
    }

    lastMessage = buffer;

    // Write level tag and message
    const char* levelStr = "[INFO] ";
    if (level == LogLevel::Error) levelStr = "[NOTICE] ";
    else if (level == LogLevel::Debug) levelStr = "[DEBUG] ";
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

void CbpLogger::Debug(const char *fmt...) {
    va_list argptr;
    va_start(argptr, fmt);
    Write(LogLevel::Debug, fmt, argptr);
    va_end(argptr);
}

// Initialize logger and set debug enabled based on config default
CbpLogger logger("Data\\F4SE\\Plugins\\cbp.log");

// Default debug state will be set by config::LoadConfig (or defaults). Keep debug off by default.
