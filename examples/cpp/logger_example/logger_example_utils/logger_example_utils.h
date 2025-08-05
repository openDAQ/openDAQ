#pragma once
#include <opendaq/custom_log.h>
#include <opendaq/opendaq.h>
#include <chrono>
#include <iostream>

using namespace daq;

void logAtLevel(LogLevel level, const LoggerPtr& logger);
