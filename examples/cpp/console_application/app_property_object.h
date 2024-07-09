/*
 * Copyright 2022-2024 openDAQ d. o. o.
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
#include <coreobjects/property_object_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

class AppFunctionBlock;
class AppDevice;

class AppPropertyObject
{
public:
    static bool processCommand(BaseObjectPtr& propObj, const std::vector<std::string>& command);

private:
    static bool set(const PropertyObjectPtr& propObj, const std::vector<std::string>& command);
    static bool get(const PropertyObjectPtr& propObj, const std::vector<std::string>& command);
    static bool list(const PropertyObjectPtr& propObj, const std::vector<std::string>& command);
    static bool help();

    static void printSingleInfo(const std::string& type, const std::string& info);
    static void printProperty(const PropertyPtr& info);
    static std::string coreTypeToString(CoreType type);

    friend AppFunctionBlock;
    friend AppDevice;
};

END_NAMESPACE_OPENDAQ
