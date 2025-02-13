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
#include <opendaq/scheduler_factory.h>
#include <gtest/gtest.h>
#include <coretypes/common.h>
#include <opendaq/task_factory.h>

#include "testutils/ut_logging.h"

#include <mutex>
#include <random>
#include <chrono>
#include <thread>
#include <opendaq/logger_factory.h>

using ExclusiveLock = std::lock_guard<std::mutex>;

class SchedulerTest : public testing::Test
{
public:
    inline static bool RandomTaskSleep = false;

    SchedulerTest(daq::SizeT numWorkers = 0)
        : scheduler(daq::Scheduler(daq::Logger(), numWorkers))
        , dist(0, 1)
    {
        using namespace std::placeholders;
        using namespace daq;

        a = Task([this](daq::IBaseObject* obj)
        {
            task0A(obj);
        }, "a");
        b = Task([this](daq::IBaseObject* obj)
        {
            task1B(obj);
        }, "b");
        c = Task([this](daq::IBaseObject* obj)
        {
            task2C(obj);
        }, "c");
        d = Task([this](daq::IBaseObject* obj)
        {
            task3D(obj);
        }, "d");
        e = Task([this](daq::IBaseObject* obj)
        {
            task4E(obj);
        }, "e");
        f = Task([this](daq::IBaseObject* obj)
        {
            task5F(obj);
        }, "f");
    }

protected:
    void TearDown() override
    {
        using namespace std::chrono_literals;

        scheduler.release();
        std::this_thread::sleep_for(100ms);
    }

    daq::SchedulerPtr scheduler;
    daq::TaskPtr a;  // 0
    daq::TaskPtr b;  // 1
    daq::TaskPtr c;  // 2
    daq::TaskPtr d;  // 3
    daq::TaskPtr e;  // 4
    daq::TaskPtr f;  // 5

    std::mutex m;
    std::random_device random;
    std::uniform_int_distribution<int> dist;

    std::vector<char> order;

    void randomSleep()
    {
        if (!RandomTaskSleep)
        {
            return;
        }

        int seconds = dist(random);
        std::this_thread::sleep_for(std::chrono::seconds(seconds));
    }

    daq::ErrCode task0A(daq::IBaseObject*)
    {
        LOG("0 [" << std::this_thread::get_id() << "] a");

        randomSleep();

        ExclusiveLock lock(m);
        order.push_back('a');

        return OPENDAQ_SUCCESS;
    }

    daq::ErrCode task1B(daq::IBaseObject*)
    {
        // using namespace std::literals;
        // std::this_thread::sleep_for(500ms);

        LOG("1 [" << std::this_thread::get_id() << "] b");

        randomSleep();

        ExclusiveLock lock(m);
        order.push_back('b');

        return OPENDAQ_SUCCESS;
    }

    daq::ErrCode task2C(daq::IBaseObject*)
    {
        LOG("2 [" << std::this_thread::get_id() << "] c");

        randomSleep();

        ExclusiveLock lock(m);
        order.push_back('c');

        return OPENDAQ_SUCCESS;
    }

    daq::ErrCode task3D(daq::IBaseObject*)
    {
        LOG("3 [" << std::this_thread::get_id() << "] d");

        randomSleep();

        ExclusiveLock lock(m);
        order.push_back('d');

        return OPENDAQ_SUCCESS;
    }

    daq::ErrCode task4E(daq::IBaseObject*)
    {
        LOG("4 [" << std::this_thread::get_id() << "] e");

        randomSleep();

        ExclusiveLock lock(m);
        order.push_back('e');

        return OPENDAQ_SUCCESS;
    }

    daq::ErrCode task5F(daq::IBaseObject*)
    {
        LOG("5 [" << std::this_thread::get_id() << "] f");

        randomSleep();

        ExclusiveLock lock(m);
        order.push_back('f');

        return OPENDAQ_SUCCESS;
    }
};
