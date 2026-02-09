#include <stdarg.h>
#include <cstring>
#include "log.h"
#include "version.h"
#include <shlobj.h>
#include <filesystem>

#pragma warning(disable : 4996)

CbpLogger::CbpLogger(const char *fname)
    : handle(nullptr), loggingEnabled(false), consolidationEnabled(true), debugEnabled(false) {
    char path[MAX_PATH];
    HRESULT hr = SHGetFolderPathA(NULL, CSIDL_MYDOCUMENTS | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, path);
    std::string fullPath;

    if (SUCCEEDED(hr)) {
        fullPath = path;
        // Ensure a separator between Documents path and supplied relative path
        if (!fullPath.empty() && fullPath.back() != '\\' && fullPath.back() != '/')
            fullPath += '\\';
        fullPath += fname;

        try {
            std::filesystem::path parent = std::filesystem::path(fullPath).parent_path();
            if (!parent.empty())
                std::filesystem::create_directories(parent);
        } catch (...) {
            // ignore filesystem errors and attempt to fopen anyway
        }

        handle = fopen(fullPath.c_str(), "w");
    }

    // Fallback: try opening the supplied path directly
    if (!handle) {
        handle = fopen(fname, "w");
    }

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
    if (loggingEnabled == enabled)
        return;

    loggingEnabled = true;
    logger.Info("LOG: loggingEnabled=%s\n", enabled ? "true" : "false");
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
    if ( !handle) return;

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
    if (!loggingEnabled ) {
        return;
	}
    va_start(argptr, fmt);
    Write(LogLevel::Info, fmt, argptr);
    va_end(argptr);
}

void CbpLogger::Error(const char *fmt...) {
    va_list argptr;
    if (!loggingEnabled) {
        return;
    }
    va_start(argptr, fmt);
    Write(LogLevel::Error, fmt, argptr);
    va_end(argptr);
}

void CbpLogger::Debug(const char *fmt...) {
    va_list argptr;
    if (!loggingEnabled) {
        return;
    }
    va_start(argptr, fmt);
    Write(LogLevel::Debug, fmt, argptr);
    va_end(argptr);
}

// Initialize logger and set debug enabled based on config default

CbpLogger logger("My Games\\Fallout4\\F4SE\\cbp.log");

// Default debug state will be set by config::LoadConfig (or defaults). Keep debug off by default.
