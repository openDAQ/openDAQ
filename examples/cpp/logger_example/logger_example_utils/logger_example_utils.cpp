#pragma once
#include <opendaq/custom_log.h>
#include <opendaq/opendaq.h>
#include <chrono>
#include <iostream>


using namespace daq;

void logAtLevel(LogLevel level, const LoggerPtr& logger)
{
    auto loggerComponent = logger.getOrAddComponent("ExampleComponent");
    switch (level)
    {
    case LogLevel::Trace:
        LOG_T("Logging at trace level");
        break;
    case LogLevel::Debug:
        LOG_D("Logging at debug level");
        break;
    case LogLevel::Info:
        LOG_I("Logging at info level");
        break;
    case LogLevel::Warn:
        LOG_W("Logging at warning level");
        break;
    case LogLevel::Error:
        LOG_E("Logging at error level");
        break;
    case LogLevel::Critical:
        LOG_C("Logging at critical level");
        break;
    default:
        LOG_D("Logging at debug level");
    }
}
