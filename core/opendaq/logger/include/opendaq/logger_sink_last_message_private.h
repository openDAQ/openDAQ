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
#include <coretypes/common.h>
#include <coretypes/baseobject.h>
#include <coretypes/string_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

DECLARE_OPENDAQ_INTERFACE(ILastMessageLoggerSinkPrivate, IBaseObject)
{

    /*!
     * @brief Get the last log message
     * @param[out] lastMessage The last log message
     */
    virtual ErrCode INTERFACE_FUNC getLastMessage(IString** lastMessage) = 0;

    /*!
     * @brief Wait for receiving a new log message
     * @param timeoutMs The timeout in milliseconds until which wait for a new log message. If timeout set as 0 ms waiting will be skipepd.
     * @param[out] success The success will return true if there was a new unread log message before waiting
     * or sink got a new logger message before timeout. False if timeout was reached.
     */
    virtual ErrCode INTERFACE_FUNC waitForMessage(SizeT timeoutMs, Bool* success) = 0;
};

END_NAMESPACE_OPENDAQ
