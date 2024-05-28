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
#include <gtest/gtest.h>
#include <opendaq/opendaq.h>

using namespace daq;

namespace
{
// TODO: opcua nd ns lt
// daq::StringPtr protocol = "nd";
daq::StringPtr protocol = getenv("protocol");
daq::StringPtr connectionString = "daq." + protocol + "://127.0.0.1";
}

// TODO: properly use the following classes

class Version
{
private:
    int major;
    int minor;
    int patch;

public:
    Version(int maj, int min, int pat)
    {
        major = maj;
        minor = min;
        patch = pat;
    }
    Version()
    {
        major = 3;
        minor = 0;
        patch = 0;
    }
};

class RegressionTest : public testing::Test
{
protected:
    std::vector<StringPtr> protocols;
    Version minVersion;

    RegressionTest(std::vector<StringPtr> protos, Version min)
    {
        protocols = protos;
        minVersion = min;
    }

    RegressionTest()
    {
        protocols = {"opcua", "nd", "ns", " lt"};
        minVersion = Version();
    }

    bool hasProtocol(StringPtr protocol)
    {
        return std::count(protocols.begin(), protocols.end(), protocol) > 0;
    }
};
