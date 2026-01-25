#include <stdarg.h>
#include <cstring>
#include "log.h"

#pragma warning(disable : 4996)

CbpLogger::CbpLogger(const char *fname) 
    : handle(nullptr), loggingEnabled(true), consolidationEnabled(true) {
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

void CbpLogger::SetConsolidationEnabled(bool enabled) {
    consolidationEnabled = enabled;
    if (!consolidationEnabled) {
        lastMessage.clear();
    }
}

void CbpLogger::Write(LogLevel level, const char *fmt, va_list args) {
    if (!loggingEnabled || !handle) return;

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
