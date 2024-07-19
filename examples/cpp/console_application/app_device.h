/*
 * Copyright 2022-2024 openDAQ d.o.o.
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
#include <opendaq/device_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

class AppDevice
{
public:
    static bool processCommand(BaseObjectPtr& device, const std::vector<std::string>& command);

private:
    static bool list(const DevicePtr& device, const std::vector<std::string>& command);
    static bool select(BaseObjectPtr& device, const std::vector<std::string>& command);
    static bool listAvailable(const DevicePtr& device, const std::vector<std::string>& command);
    static bool add(const DevicePtr& device, const std::vector<std::string>& command);
    static bool remove(const DevicePtr& device, const std::vector<std::string>& command);
    static bool print(const DevicePtr& device, const std::vector<std::string>& command);
    static bool help();
};

END_NAMESPACE_OPENDAQ
