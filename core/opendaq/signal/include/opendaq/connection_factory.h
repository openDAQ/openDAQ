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
#include <opendaq/connection_ptr.h>
#include <opendaq/signal_ptr.h>
#include <opendaq/input_port_ptr.h>
#include <opendaq/context_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_connection
 * @addtogroup opendaq_connection_factories Factories
 * @{
 */

/*!
 * @brief Creates a Connection object that acts as a packet queue between the provided Input port and Signal.
 * @param inputPort The input port to which the connection leads.
 * @param signal The signal that is to be connected to an input port.
 * @param context The Context. Most often provided by the Instance.
 */
inline ConnectionPtr Connection(InputPortPtr inputPort, SignalPtr signal, ContextPtr context)
{
    ConnectionPtr obj(Connection_Create(inputPort, signal, context));
    return obj;
}
/*!@}*/

END_NAMESPACE_OPENDAQ
