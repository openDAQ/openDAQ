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
#include <opendaq/function_block.h>
#include <opendaq/input_port_config.h>
#include <opendaq/signal_config.h>
#include <opendaq/data_descriptor.h>
#include <coreobjects/permissions.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @brief Interface for function blocks implemented in Python.
 *
 * Extends IFunctionBlock with methods for creating and removing
 * input ports and signals from within a Python-implemented function block.
 */
DECLARE_OPENDAQ_INTERFACE(IPythonFunctionBlock, IFunctionBlock)
{
    virtual ErrCode INTERFACE_FUNC createAndAddInputPort(IInputPortConfig** port,
                                                         IString* localId,
                                                         PacketReadyNotification notificationMethod,
                                                         IBaseObject* customData,
                                                         Bool requestGapPackets,
                                                         IPermissions* permissions) = 0;

    virtual ErrCode INTERFACE_FUNC createAndAddSignal(ISignalConfig** signal,
                                                       IString* localId,
                                                       IDataDescriptor* descriptor,
                                                       Bool visible,
                                                       Bool isPublic,
                                                       IPermissions* permissions) = 0;

    virtual ErrCode INTERFACE_FUNC removeInputPort(IInputPortConfig* port) = 0;

    virtual ErrCode INTERFACE_FUNC removeSignal(ISignalConfig* signal) = 0;
};

END_NAMESPACE_OPENDAQ
