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

/**
 * @file    finally.h
 * @author  Anže Škerjanc
 * @date    29/05/2019
 * @version 1.0
 *
 * @brief Memory management based on RAII
 *
 */

#pragma once

#include <functional>
#include <opendaq/utils/utils.h>

BEGIN_NAMESPACE_UTILS

/**
 * @brief The class Finnaly calls cleanup function on destructor
 */
class Finally
{
public:
    Finally() = delete;
    Finally(const Finally& other) = delete;
    Finally& operator=(const Finally&) = delete;

    /**
     * @brief Create finally object
     * @param finalizer a noexcept function called in destructor.
     */
    explicit Finally(std::function<void()> finalizer)
        : finalizer(std::move(finalizer))
    {
    }

    ~Finally()
    {
        if (finalizer)
            finalizer();
    }

private:
    std::function<void()> finalizer;
};

END_NAMESPACE_UTILS
