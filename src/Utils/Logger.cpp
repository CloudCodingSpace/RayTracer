#include "Logger.h"

void _log(LogSeverity severity, std::string msg)
{
    std::string message = "[GLTracer] ";
    switch(severity) {
        case LOG_SEVERITY_INFO:
            printf("\033[0;32m");
            message += "INFO: ";
            break;
        case LOG_SEVERITY_WARN:
            printf("\033[0;33m");
            message += "WARN: ";
            break;
#if !defined(NDEBUG) || defined(_DEBUG)
        case LOG_SEVERITY_DEBUG:
            printf("\033[0;34m");
            message += "DEBUG: ";
            break;
#endif
        case LOG_SEVERITY_ERROR:
            printf("\033[1;31m");
            message += "ERROR: ";
            break;
        case LOG_SEVERITY_FATAL:
            printf("\033[0;31m");
            message += "FATAL: ";
            break;
    }

    message += msg;
    printf("%s\n", message.c_str());
    printf("\033[0;0m"); // Resetting the terminal color
}