/*
 * Copyright 2022-2025 openDAQ d.o.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once
#include <audio_device_module/common.h>
#include <audio_device_module/miniaudio_utils.h>
#include <opendaq/module_info_factory.h>
#include <opendaq/logger_component_ptr.h>
#include <opendaq/logger_ptr.h>
#include <coreobjects/unit_ptr.h>

BEGIN_NAMESPACE_AUDIO_DEVICE_MODULE

namespace utils
{
    LoggerComponentPtr getLoggerComponent(const LoggerPtr& logger);

    ma_device_id getIdFromConnectionString(const std::string& connectionString);
    std::string getConnectionStringFromId(ma_backend backend, ma_device_id id);
}

END_NAMESPACE_AUDIO_DEVICE_MODULE
