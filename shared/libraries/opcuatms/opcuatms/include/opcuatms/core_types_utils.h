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

#include <coretypes/coretypes.h>
#include <opcuashared/node/opcuatype.h>
#include <opcuatms/opcuatms.h>
#include <opendaq/sample_type.h>
#include "opcuashared/opcuavariant.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA

StringPtr ConvertToDaqCoreString(const UA_String& uaString);
OpcUaObject<UA_String> ConvertToOpcUaString(const StringPtr& str);
BinaryDataPtr CreateCoreBinaryDataFromUaByteString(const UA_ByteString& uaByteString);
OpcUaObject<UA_ByteString> CreateUaByteStringFromCoreBinaryData(const BinaryDataPtr& binaryData);
SampleType SampleTypeFromTmsEnum(UA_SampleTypeEnumeration tmsEnum);
UA_SampleTypeEnumeration SampleTypeToTmsEnum(SampleType daqEnum);
ScaledSampleType ScaledSampleTypeFromTmsEnum(UA_SampleTypeEnumeration tmsEnum);
UA_SampleTypeEnumeration ScaledSampleTypeToTmsEnum(ScaledSampleType daqEnum);
OpcUaNodeId CoreTypeToUANodeID(CoreType type);
CoreType UANodeIdToCoreType(OpcUaNodeId nodeId);
OpcUaVariant DecodeIfExtensionObject(const OpcUaVariant& variant);
OpcUaVariant UnwrapIfVariant(const OpcUaVariant& variant);
const UA_DataType* GetUAStructureDataTypeByName(const std::string& structName);
const UA_DataType* GetUAEnumerationDataTypeByName(const std::string& enumerationName);
const std::string GetUATypeName(UA_UInt16 namespaceIndex, UA_UInt32 identifierNumeric);
bool nativeStructConversionSupported(const std::string& structName);

END_NAMESPACE_OPENDAQ_OPCUA
