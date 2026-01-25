#pragma once
#include <stdio.h>
#include <string>

enum class LogLevel {
    Info,
    Error
};

class CbpLogger {
public:
    CbpLogger(const char* fname);
    ~CbpLogger();
    
    void SetLoggingEnabled(bool enabled);
    void SetConsolidationEnabled(bool enabled);
    void Info(const char* fmt...);
    void Error(const char* fmt...);

private:
    void Write(LogLevel level, const char* fmt, va_list args);
    
    FILE* handle;
    std::string lastMessage;
    bool loggingEnabled;
    bool consolidationEnabled;
};

extern CbpLogger logger;