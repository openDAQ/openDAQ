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
#include <testutils/memcheck_listener.h>
#include <gtest/gtest.h>

class DaqMemCheckListener : public MemCheckListener
{
protected:
    void OnTestStart(const testing::TestInfo& info) override;
    void OnTestEnd(const testing::TestInfo& info) override;

private:
#ifndef NDEBUG
    size_t objCount = 0;
#endif
};
