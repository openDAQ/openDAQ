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
#include <coretypes/coretypes.h>
#include "channel_buffer.h"

BEGIN_NAMESPACE_DEWESOFT_RT_CORE

/*#
 * [templated(IChannelBuffer)]
 * [interfaceSmartPtr(IChannelBuffer, ChannelBufferPtr)]
 * [interfaceNamespace(IAsyncChannelBuffer, "Dewesoft::RT::Core::")]
 * [interfaceNamespace(IChannelBuffer, "Dewesoft::RT::Core::")]
 */
DECLARE_RT_INTERFACE(IAsyncChannelBuffer, IChannelBuffer)
{
    DEFINE_INTFID("IAsyncChannelBuffer")

    virtual ErrCode INTERFACE_FUNC addSample(void* value, Float timestamp) = 0;

    virtual ErrCode INTERFACE_FUNC getTimestampData(Float** data) = 0;
    virtual ErrCode INTERFACE_FUNC getTimestamp(SizeT position, Float* sample) = 0;
    virtual ErrCode INTERFACE_FUNC getTimestamp(SizeT position, IChannelBuffer*** sample) = 0;
};

//////////////////////////////////////////

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE()
OPENDAQ_DECLARE_CLASS_FACTORY(LIBRARY_FACTORY, Dict)
OPENDAQ_DECLARE_CLASS_FACTORY(LIBRARY_FACTORY, Data)
OPENDAQ_DECLARE_CLASS_FACTORY(LIBRARY_FACTORY, Property)

OPENDAQ_DECLARE_CLASS_FACTORY(LIBRARY_FACTORY, ModuleManager, IModuleDriverEnumerator*, enumerator)

OPENDAQ_DECLARE_CLASS_FACTORY(LIBRARY_FACTORY, EvalValue, IString*, eval)
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, EvalValueArgs, IEvalValue, IString*, eval, IList*, args)

OPENDAQ_DECLARE_CLASS_FACTORY(LIBRARY_FACTORY, AsyncChannel, IString*, nodeId, SampleDataType, type, Float, bufferSizeInSec)
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, AsyncVectorChannel, IAsyncChannel, IString*, nodeId, SampleDataType, type, SizeT, dimension, Float, bufferSizeSec)

OPENDAQ_DECLARE_CLASS_FACTORY(LIBRARY_FACTORY, SyncChannel, IString*, nodeId, SampleDataType, type, Float, bufferSizeSec)
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, SyncVectorChannel, ISyncChannel, IString*, nodeId, SampleDataType, type, SizeT, dimension, Float, bufferSizeSec)

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, ClockProviderHelper, IClockProviderHelper, IClockProvider*, clockProvider)

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY,
    PropertyObjectClassWithName,
    IPropertyObjectClass,
    createProperyObjectClassWithName,
    IString*,
    name
)

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(LIBRARY_FACTORY, FuncObject3, IFuncObject, createFuncObject3, FuncCall3, value)

OPENDAQ_DECLARE_CLASS_FACTORY(LIBRARY_FACTORY, String, ConstCharPtr, str)
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(LIBRARY_FACTORY, StringN, IString, createStringN, ConstCharPtr, str, SizeT, length)

OPENDAQ_DECLARE_CLASS_FACTORY(LIBRARY_FACTORY, Logger, IString*, name, LogLevel, level)

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY,
    LoggerWithSinks,
    ILogger,
    LoggerWithSinks_Create,
    IString*,
    name,
    IList*,
    sinks,
    LogLevel,
    level);

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(LIBRARY_FACTORY, StdErrLoggerSink, ILoggerSink, StdErrLoggerSink_Create)
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(LIBRARY_FACTORY, StdOutLoggerSink, ILoggerSink, StdOutLoggerSink_Create)

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(LIBRARY_FACTORY,
    RotatingFileLoggerSink,
    ILoggerSink,
    RotatingFileLoggerSink_Create,
    IString*,
    fileName,
    SizeT,
    maxFileByteSize,
    SizeT,
    maxFiles)

#ifdef _WIN32

    OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(LIBRARY_FACTORY,
        WinDebugLoggerSink,
        ILoggerSink,
        WinDebugLoggerSink_Create)

#endif

OPENDAQ_DECLARE_CLASS_FACTORY(LIBRARY_FACTORY, InputGroupType, IString*, type, Int, maxCount, SizeT, count)

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(LIBRARY_FACTORY,
    InputGroupTypeWithCallback,
    IInputGroupType,
    InputGroupTypeWithCallback_Create,
    IString*,
    type,
    IFuncObject*,
    onGetCount,
    Int,
    maxCount);

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(LIBRARY_FACTORY, DemoModuleDriver, Core::IModuleDriver, createDemoModuleDriver)

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(INTERNAL_FACTORY, DemoModuleFactory, Core::IModuleFactory, createDemoModuleFactory)

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(INTERNAL_FACTORY,
    ChannelsModuleFactory,
    Core::IModuleFactory,
    createChannelsModuleFactory)

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(LIBRARY_FACTORY, SumModuleDriver, Core::IModuleDriver, createSumModuleDriver)

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(INTERNAL_FACTORY, SumModuleFactory, Core::IModuleFactory, createSumModuleFactory)

///////////////////////////////////////

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, AsyncChannelBufferInt32, IAsyncChannelBuffer)
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, AsyncChannelBufferInt16, IAsyncChannelBuffer)
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, AsyncChannelBufferDouble, IAsyncChannelBuffer)

END_NAMESPACE_DEWESOFT_RT_CORE
