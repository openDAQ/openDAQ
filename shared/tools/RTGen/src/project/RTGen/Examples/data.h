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
#include <corestructure/node.h>
#include <corestructure/clock_provider_helper.h>
#include <corestructure/abs_time.h>
#include <corestructure/acquisition_info.h>

BEGIN_NAMESPACE_DEWESOFT_RT_CORE

enum class OperationMode : EnumType
{
    Configuration,
    Operation,
    Realtime
};

/*#
 * [propertyClass(NodeImpl)]
 * [propertyClassCtorArgs(const StringPtr& className, const StringPtr& nodeId)]
 * [interfaceSmartPtr(INode, NodePtr)]
 * [interfaceSmartPtr(IClockProviderHelper, ClockProviderHelperPtr)]
 * [addProperty(RestartAcquisitionOnStartup, Bool, False)]
 * [addEnumProperty(OperationMode, OperationMode, 1, List("Configuration", "Operation", "Realtime"), CreateUpdate)]
 */
DECLARE_RT_INTERFACE(IData, INode)
{
    // [elementType(value, IDataModule), property(defaultValue: List(), staticConfAction: Skip)]
    virtual ErrCode INTERFACE_FUNC getModules(IList** value) = 0;

    virtual ErrCode INTERFACE_FUNC getStartAbsTime(Int* value) = 0;

    virtual ErrCode INTERFACE_FUNC getClockProvider(IClockProviderHelper** value) = 0;

    // [property(staticConfAction: Skip)]
    virtual ErrCode INTERFACE_FUNC getAbsTimeProvider(IAbsTime** value) = 0;

    virtual ErrCode INTERFACE_FUNC initialize() = 0;
    virtual ErrCode INTERFACE_FUNC initializeAcquisition() = 0;

    virtual ErrCode INTERFACE_FUNC startAcquisition() = 0;
    virtual ErrCode INTERFACE_FUNC stopAcquisition() = 0;

    virtual ErrCode INTERFACE_FUNC finalizeAcquisition() = 0;

    virtual ErrCode INTERFACE_FUNC getAcquisitionInfo(IAcquisitionInfo** value) = 0;
};

OPENDAQ_DECLARE_CLASS_FACTORY(LIBRARY_FACTORY, Data)

END_NAMESPACE_DEWESOFT_RT_CORE
