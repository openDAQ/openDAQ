/*
 * Copyright 2022-2024 Blueberry d.o.o.
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
#include <opendaq/input_port_ptr.h>
#include <opendaq/instance_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

class AppInputPort
{
public:
    static bool processCommand(BaseObjectPtr& port, const std::vector<std::string>& command, const InstancePtr& instance);

private:
    static bool connect(const InputPortPtr& port, const std::vector<std::string>& command, const InstancePtr& instance);
    static bool disconnect(const InputPortPtr& port);
    static bool print(const InputPortPtr& port, const std::vector<std::string>& command);
    static bool select(BaseObjectPtr& port, const std::vector<std::string>& command);
    static bool help();
};

END_NAMESPACE_OPENDAQ
