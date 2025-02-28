
#pragma once 

#include <iostream>
#include <string>

enum LogSeverity
{
    LOG_SEVERITY_INFO,
    LOG_SEVERITY_WARN,
    LOG_SEVERITY_DEBUG,
    LOG_SEVERITY_ERROR,
    LOG_SEVERITY_FATAL
};

void _log(LogSeverity severity, std::string msg);

#define INFO(msg) _log(LOG_SEVERITY_INFO, msg);
#define WARN(msg) _log(LOG_SEVERITY_WARN, msg);
#define ERROR(msg) _log(LOG_SEVERITY_ERROR, msg);
#define FATAL(msg) _log(LOG_SEVERITY_FATAL, msg);

#if !defined(NDEBUG) || defined(_DEBUG)
#define DEBUG(msg) _log(LOG_SEVERITY_DEBUG, msg);
#endif
