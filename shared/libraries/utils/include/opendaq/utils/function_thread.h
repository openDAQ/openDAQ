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

/**
 * @file    function_thread.h
 * @author  Anže Škerjanc
 * @date    15/10/2019
 * @version 1.0
 *
 * @brief Provides a mechanism for executing a method in a thread.
 */

#pragma once

#include <opendaq/utils/thread_ex.h>
#include <functional>

BEGIN_NAMESPACE_UTILS

/**
 * @brief Provides a mechanism for executing a method in a thread.
 */
class FunctionThread : public ThreadEx
{
public:
    using CallbackFunction = std::function<void(void)>;

    /**
     * @brief Initializes a new instance of the FunctionThread class.
     * @param callback a callback function you want the FunctionThread to execute.
     */
    FunctionThread(CallbackFunction callback = nullptr);

    /**
     * @brief Get callback function.
     * @return Callback function.
     */
    const CallbackFunction& getCallback() const;

    /**
     * @brief Set callback function.
     * @param value A function that will be called.
     */
    void setCallback(const CallbackFunction& value);

protected:
    void execute() override;

private:
    CallbackFunction callback;
};

END_NAMESPACE_UTILS
