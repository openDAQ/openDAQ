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
#include <gtest/gtest.h>
#include <opendaq/opendaq.h>

using namespace daq;

namespace
{

// To run the tests locally you can set the protocol here manually,
// instead of setting and reading the environment variable "protocol"

// Note: make sure this uses getenv in production

// StringPtr protocol = "nd";  // opcua nd ns lt
StringPtr protocol = getenv("protocol");

StringPtr connectionString = "daq." + protocol + "://127.0.0.1";
}

// Macro that ensures that we only continue for listed protocols
#define PROTOCOLS(...)                                                     \
    std::set<std::string> protocols = {__VA_ARGS__};                       \
    if (protocol == "opcua" && protocols.find("opcua") == protocols.end()) \
        return;                                                            \
    else if (protocol == "nd" && protocols.find("nd") == protocols.end())  \
        return;                                                            \
    else if (protocol == "ns" && protocols.find("ns") == protocols.end())  \
        return;                                                            \
    else if (protocol == "lt" && protocols.find("lt") == protocols.end())  \
        return;
