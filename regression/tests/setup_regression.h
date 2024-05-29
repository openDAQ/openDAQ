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
// StringPtr protocol = "nd";
StringPtr protocol = getenv("protocol");
StringPtr connectionString = "daq." + protocol + "://127.0.0.1";
}

// TODO: properly use the following classes and macros (until end of file)

class Version
{
private:
    int major;
    int minor;
    int patch;

public:
    Version(int MAJOR, int MINOR, int PATCH)
    {
        major = MAJOR;
        minor = MINOR;
        patch = PATCH;
    }
    Version()
    {
        major = 3;
        minor = 0;
        patch = 0;
    }
};

class Protocols
{
private:
    bool opcua;
    bool nd;
    bool ns;
    bool lt;

public:
    Protocols(bool OPCUA, bool ND, bool NS, bool LT)
    {
        opcua = OPCUA;
        nd = ND;
        ns = NS;
        lt = LT;
    }
    Protocols()
    {
        opcua = true;
        nd = true;
        ns = true;
        lt = true;
    }
};

class RegressionTest : public testing::Test
{
protected:
    Protocols protocols;
    Version minVersion;

    RegressionTest(Protocols protos, Version min)
    {
        protocols = protos;
        minVersion = min;
    }

    RegressionTest()
    {
        protocols = Protocols();
        minVersion = Version();
    }
};

/*
namespace
{
Version version;
Protocols protocols;
}

#define TEST_R(test_fixture, test_name, ver, protos) \
    version = ver;                                   \
    protocols = protos;                              \
    TEST_F(test_fixture, test_name)

 */
