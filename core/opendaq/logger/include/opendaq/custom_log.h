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
#include <opendaq/log.h>

#if !defined(LOG_T)
    #define LOG_T(message, ...) DAQLOGF_T(loggerComponent, message, ##__VA_ARGS__)
    #define LOG_D(message, ...) DAQLOGF_D(loggerComponent, message, ##__VA_ARGS__)
    #define LOG_I(message, ...) DAQLOGF_I(loggerComponent, message, ##__VA_ARGS__)
    #define LOG_W(message, ...) DAQLOGF_W(loggerComponent, message, ##__VA_ARGS__)
    #define LOG_E(message, ...) DAQLOGF_E(loggerComponent, message, ##__VA_ARGS__)
    #define LOG_C(message, ...) DAQLOGF_C(loggerComponent, message, ##__VA_ARGS__)

    #define LOGP_T(message) DAQLOG_T(loggerComponent, message)
    #define LOGP_D(message) DAQLOG_D(loggerComponent, message)
    #define LOGP_I(message) DAQLOG_I(loggerComponent, message)
    #define LOGP_W(message) DAQLOG_W(loggerComponent, message)
    #define LOGP_E(message) DAQLOG_E(loggerComponent, message)
    #define LOGP_C(message) DAQLOG_C(loggerComponent, message)
#endif
