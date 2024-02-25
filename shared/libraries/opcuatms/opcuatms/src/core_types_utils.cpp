#include <opcuatms/core_types_utils.h>

#include "opcuashared/opcuadatatypearraylist.h"
#include "opcuatms/extension_object.h"
#include "open62541/nodeids.h"
#include "open62541/daqbt_nodeids.h"
#include "open62541/types_di_generated.h"
#include "open62541/types_daqesp_generated.h"
#include "open62541/types_daqhbk_generated.h"

using namespace daq::opcua;
using namespace daq;
using namespace daq::opcua;

BEGIN_NAMESPACE_OPENDAQ_OPCUA

namespace details
{
    static std::unordered_map<OpcUaNodeId, CoreType> nodeIdToCoreTypeMap = {
        {OpcUaNodeId(0, UA_NS0ID_BOOLEAN), ctBool},
        {OpcUaNodeId(0, UA_NS0ID_FLOAT), ctFloat},
        {OpcUaNodeId(0, UA_NS0ID_DOUBLE), ctFloat},
        {OpcUaNodeId(0, UA_NS0ID_SBYTE), ctInt},
        {OpcUaNodeId(0, UA_NS0ID_BYTE), ctInt},
        {OpcUaNodeId(0, UA_NS0ID_INT16), ctInt},
        {OpcUaNodeId(0, UA_NS0ID_UINT16), ctInt},
        {OpcUaNodeId(0, UA_NS0ID_INT32), ctInt},
        {OpcUaNodeId(0, UA_NS0ID_UINT32), ctInt},
        {OpcUaNodeId(0, UA_NS0ID_INT64), ctInt},
        {OpcUaNodeId(0, UA_NS0ID_UINT64), ctInt},
        {OpcUaNodeId(0, UA_NS0ID_STRING), ctString},
        {OpcUaNodeId(0, UA_NS0ID_RATIONALNUMBER), ctRatio},
        {OpcUaNodeId(NAMESPACE_DAQBT, UA_DAQBTID_RATIONALNUMBER64), ctRatio},
        {OpcUaNodeId(0, UA_DAQBTID_RATIONALNUMBER64), ctRatio}
    };
}

StringPtr ConvertToDaqCoreString(const UA_String& uaString)
{
    if (uaString.length == 0 && uaString.data == nullptr)
        return nullptr;
    return String(reinterpret_cast<const char*>(uaString.data), uaString.length);
}

OpcUaObject<UA_String> ConvertToOpcUaString(const StringPtr& str)
{
    if (str.assigned())
        return OpcUaObject<UA_String>(UA_STRING_ALLOC(str.getCharPtr()));
    return {};
}

BinaryDataPtr CreateCoreBinaryDataFromUaByteString(const UA_ByteString& uaByteString)
{
    if (uaByteString.length == 0 && uaByteString.data == nullptr)
        return nullptr;
    return BinaryData(reinterpret_cast<char*>(uaByteString.data), uaByteString.length);
}

OpcUaObject<UA_ByteString> CreateUaByteStringFromCoreBinaryData(const BinaryDataPtr& binaryData)
{
    OpcUaObject<UA_ByteString> byteString;
    byteString->data = (uint8_t*) UA_malloc(binaryData.getSize());
    byteString->length = binaryData.getSize();
    memcpy(byteString->data, binaryData.getAddress(), binaryData.getSize());
    return OpcUaObject<UA_ByteString>(byteString);
}

SampleType SampleTypeFromTmsEnum(UA_SampleTypeEnumeration tmsEnum)
{
    switch (tmsEnum)
    {
        case UA_SAMPLETYPEENUMERATION_INVALID:
            return SampleType::Invalid;
        case UA_SAMPLETYPEENUMERATION_FLOAT32:
            return SampleType::Float32;
        case UA_SAMPLETYPEENUMERATION_FLOAT64:
            return SampleType::Float64;
        case UA_SAMPLETYPEENUMERATION_UINT8:
            return SampleType::UInt8;
        case UA_SAMPLETYPEENUMERATION_INT8:
            return SampleType::Int8;
        case UA_SAMPLETYPEENUMERATION_UINT16:
            return SampleType::UInt16;
        case UA_SAMPLETYPEENUMERATION_INT16:
            return SampleType::Int16;
        case UA_SAMPLETYPEENUMERATION_UINT32:
            return SampleType::UInt32;
        case UA_SAMPLETYPEENUMERATION_INT32:
            return SampleType::Int32;
        case UA_SAMPLETYPEENUMERATION_UINT64:
            return SampleType::UInt64;
        case UA_SAMPLETYPEENUMERATION_INT64:
            return SampleType::Int64;
        case UA_SAMPLETYPEENUMERATION_COMPLEXFLOAT32:
            return SampleType::ComplexFloat32;
        case UA_SAMPLETYPEENUMERATION_COMPLEXFLOAT64:
            return SampleType::ComplexFloat64;
        case UA_SAMPLETYPEENUMERATION_BINARY:
            return SampleType::Binary;
        case UA_SAMPLETYPEENUMERATION_STRING:
            return SampleType::String;
        case UA_SAMPLETYPEENUMERATION_RANGEINT64:
            return SampleType::RangeInt64;
        default:
            throw ConversionFailedException();
    }
}

UA_SampleTypeEnumeration SampleTypeToTmsEnum(SampleType daqEnum)
{
    switch (daqEnum)
    {
        case SampleType::Invalid:
            return UA_SAMPLETYPEENUMERATION_INVALID;
        case SampleType::Float32:
            return UA_SAMPLETYPEENUMERATION_FLOAT32;
        case SampleType::Float64:
            return UA_SAMPLETYPEENUMERATION_FLOAT64;
        case SampleType::UInt8:
            return UA_SAMPLETYPEENUMERATION_UINT8;
        case SampleType::Int8:
            return UA_SAMPLETYPEENUMERATION_INT8;
        case SampleType::UInt16:
            return UA_SAMPLETYPEENUMERATION_UINT16;
        case SampleType::Int16:
            return UA_SAMPLETYPEENUMERATION_INT16;
        case SampleType::UInt32:
            return UA_SAMPLETYPEENUMERATION_UINT32;
        case SampleType::Int32:
            return UA_SAMPLETYPEENUMERATION_INT32;
        case SampleType::UInt64:
            return UA_SAMPLETYPEENUMERATION_UINT64;
        case SampleType::Int64:
            return UA_SAMPLETYPEENUMERATION_INT64;
        case SampleType::ComplexFloat32:
            return UA_SAMPLETYPEENUMERATION_COMPLEXFLOAT32;
        case SampleType::ComplexFloat64:
            return UA_SAMPLETYPEENUMERATION_COMPLEXFLOAT64;
        case SampleType::Binary:
            return UA_SAMPLETYPEENUMERATION_BINARY;
        case SampleType::String:
            return UA_SAMPLETYPEENUMERATION_STRING;
        case SampleType::RangeInt64:
            return UA_SAMPLETYPEENUMERATION_RANGEINT64;
        case SampleType::Struct:
            return UA_SAMPLETYPEENUMERATION_INVALID;
        default:
            throw ConversionFailedException();
    }
}


ScaledSampleType ScaledSampleTypeFromTmsEnum(UA_SampleTypeEnumeration tmsEnum)
{
    switch (tmsEnum)
    {
        case UA_SAMPLETYPEENUMERATION_FLOAT32:
            return ScaledSampleType::Float32;
        case UA_SAMPLETYPEENUMERATION_FLOAT64:
            return ScaledSampleType::Float64;
        default:
            throw ConversionFailedException();
    }
}

UA_SampleTypeEnumeration ScaledSampleTypeToTmsEnum(ScaledSampleType daqEnum)
{
    switch (daqEnum)
    {
        case ScaledSampleType::Float32:
            return UA_SAMPLETYPEENUMERATION_FLOAT32;
        case ScaledSampleType::Float64:
            return UA_SAMPLETYPEENUMERATION_FLOAT64;
        default:
            throw ConversionFailedException();
    }
}

OpcUaNodeId CoreTypeToUANodeID(CoreType type)
{
    switch(type)
    {
        case ctBool:
            return OpcUaNodeId(0, UA_NS0ID_BOOLEAN);
        case ctInt:
            return OpcUaNodeId(0, UA_NS0ID_INT64);
        case ctFloat:
            return OpcUaNodeId(0, UA_NS0ID_DOUBLE);
        case ctString:
            return OpcUaNodeId(0, UA_NS0ID_STRING);
        case ctRatio:
            return OpcUaNodeId(NAMESPACE_DAQBT, UA_DAQBTID_RATIONALNUMBER64);
        case ctProc:
        case ctList:
        case ctDict:
        case ctObject:
        case ctBinaryData:
        case ctFunc:
        case ctComplexNumber:
        case ctUndefined:
        default:
            throw ConversionFailedException{"Mapping between core type and node id is not available"};
    }
}

CoreType UANodeIdToCoreType(OpcUaNodeId nodeId)
{
    if (const auto it = details::nodeIdToCoreTypeMap.find(nodeId); it != details::nodeIdToCoreTypeMap.cend())
        return it->second;
    throw ConversionFailedException{"Mapping between node id and core type is not available."};
}

OpcUaVariant DecodeIfExtensionObject(const OpcUaVariant& variant)
{
    if (variant.isType<UA_ExtensionObject>())
    {
        const auto data = (UA_ExtensionObject*) variant->data;
        auto extensionObject = tms::ExtensionObject(data[0]);
        if (extensionObject.isDecoded())
            return extensionObject.getAsVariant();

        throw ConversionFailedException();
    }

    return variant;
}

OpcUaVariant UnwrapIfVariant(const OpcUaVariant& variant)
{
    if (variant.isType<UA_Variant>())
    {
        const auto data = (UA_Variant*) variant->data;
        return OpcUaVariant(data[0]);
    }

    return variant;
}

const UA_DataType* GetUAStructureDataTypeByName(const std::string& structName)
{
    // TODO: Create static list, add any custom types added automatically.
    OpcUaDataTypeArrayList typeArr;
    typeArr.add(UA_TYPES_COUNT, UA_TYPES);
    typeArr.add(UA_TYPES_DI_COUNT, UA_TYPES_DI);
    typeArr.add(UA_TYPES_DAQBT_COUNT, UA_TYPES_DAQBT);
    typeArr.add(UA_TYPES_DAQBSP_COUNT, UA_TYPES_DAQBSP);
    typeArr.add(UA_TYPES_DAQDEVICE_COUNT, UA_TYPES_DAQDEVICE);
    typeArr.add(UA_TYPES_DAQESP_COUNT, UA_TYPES_DAQESP);
    typeArr.add(UA_TYPES_DAQHBK_COUNT, UA_TYPES_DAQHBK);
    
    const UA_DataTypeArray* dataType = typeArr.getCustomDataTypes();
    while(dataType)
    {
        for(size_t i = 0; i < dataType->typesSize; ++i)
        {
            if (dataType->types[i].typeName == structName)
            {
                const auto typeKind = dataType->types[i].typeKind;
                if (typeKind == UA_DATATYPEKIND_STRUCTURE || typeKind == UA_DATATYPEKIND_OPTSTRUCT)
                    return &dataType->types[i];
            }
        }
        dataType = dataType->next;
    }

    return nullptr;
}

END_NAMESPACE_OPENDAQ_OPCUA
