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
#include <gtest/gtest.h>

class BaseTestListener : public ::testing::EmptyTestEventListener
{
protected:
    void OnTestStart(const testing::TestInfo& info) override
    {
#       ifdef OPENDAQ_SKIP_UNSTABLE_TESTS
        const auto testFullName = std::string(info.name());
        const auto unstableTestPrefix = std::string("UNSTABLE_SKIPPED_");
        if (testFullName.find(unstableTestPrefix) == 0)
        {
            ::testing::Test::RecordProperty("Unstable test skipped", true);
            GTEST_SKIP() << "Skipping unstable test: " << info.test_suite_name() << "." << testFullName.substr(unstableTestPrefix.length());
        }
#       endif
    }

    void OnTestEnd(const testing::TestInfo& /*info*/) override
    {
    }
};
