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

/**
 * @file    anonymous_variable.h
 * @author  Martin Kraner
 * @date    04/03/2019
 * @version 1.0
 *
 * @brief Unique variable name generator.
 *        Each call returns a different identifier.
 *
 */
#pragma once

#define CONCATENATE_IMPL(s1, s2) s1##s2
#define CONCATENATE(s1, s2) CONCATENATE_IMPL(s1, s2)

#ifdef __COUNTER__
    #define ANONYMOUS_VARIABLE(str) \
        CONCATENATE(str, __COUNTER__)
#else
    #define ANONYMOUS_VARIABLE(str) \
        CONCATENATE(str, __LINE__)
#endif
