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
#include <opendaq/opendaq.h>

BEGIN_NAMESPACE_OPENDAQ

class ConsoleApplication
{
public:
    ConsoleApplication(const InstancePtr& instance);
    void start();

private:
    std::string getTypeString(const BaseObjectPtr& obj);
    std::string getPathString();
    bool processCommand(const std::vector<std::string>& command);

    InstancePtr instance;
    std::vector<BaseObjectPtr> parents;
    BaseObjectPtr currentObject;
};

END_NAMESPACE_OPENDAQ
