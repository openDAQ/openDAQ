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
#include <coretypes/baseobject.h>
#include <opendaq/data_descriptor.h>
#include <coretypes/number.h>

BEGIN_NAMESPACE_OPENDAQ

/*#
 * [interfaceLibrary(INumber, CoreTypes)]
 */

/*!
 * @ingroup opendaq_packets
 * @addtogroup opendaq_reusable_data_packet Reusable Data packet
 * @{
 */

DECLARE_OPENDAQ_INTERFACE(IReusableDataPacket, IBaseObject)
{
    virtual ErrCode INTERFACE_FUNC reuse(IDataDescriptor* newDescriptor, SizeT newSampleCount, INumber* newOffset, IDataPacket* newDomainPacket, Bool canReallocMemory, Bool* success) = 0;
};

/*!@}*/

END_NAMESPACE_OPENDAQ
