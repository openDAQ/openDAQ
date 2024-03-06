/*
 * Copyright 2022-2023 Blueberry d.o.o.
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

#include "opcuatms/converters/variant_converter.h"
#include "opcuatms/converters/selection_converter.h"
#include "opendaq/data_descriptor_ptr.h"
#include "open62541/daqbt_nodeids.h"
#include "open62541/daqbsp_nodeids.h"
#include "open62541/nodeids.h"
#include "opendaq/function_block_type_ptr.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

namespace converters
{
    static OpcUaVariant convertToVariant(IntfID interfaceId,
                                         const BaseObjectPtr& object,
                                         const UA_DataType* targetType,
                                         const ContextPtr& context);
    static OpcUaVariant convertToArrayVariant(IntfID elementId,
                                              const ListPtr<IBaseObject>& list,
                                              const UA_DataType* targetType,
                                              const ContextPtr& context);
    static BaseObjectPtr convertToDaqObject(const OpcUaVariant& variant, const ContextPtr& context);
    static BaseObjectPtr convertToDaqList(const OpcUaVariant& variant, const ContextPtr& context);

    static std::unordered_map<IntfID, std::function<OpcUaVariant(const BaseObjectPtr&, const UA_DataType*, const ContextPtr&)>>
        idToVariantMap{{INumber::Id,
                        [](const BaseObjectPtr& object, const UA_DataType* targetType, const ContextPtr& ctx)
                        { return VariantConverter<INumber>::ToVariant(object, targetType); }},
                       {IString::Id,
                        [](const BaseObjectPtr& object, const UA_DataType* targetType, const ContextPtr& ctx)
                        { return VariantConverter<IString>::ToVariant(object, targetType, ctx); }},
                       {IUnit::Id,
                        [](const BaseObjectPtr& object, const UA_DataType* targetType, const ContextPtr& ctx)
                        { return VariantConverter<IUnit>::ToVariant(object, targetType, ctx); }},
                       {IRatio::Id,
                        [](const BaseObjectPtr& object, const UA_DataType* targetType, const ContextPtr& ctx)
                        { return VariantConverter<IRatio>::ToVariant(object, targetType, ctx); }},
                       {IBoolean::Id,
                        [](const BaseObjectPtr& object, const UA_DataType* targetType, const ContextPtr& ctx)
                        { return VariantConverter<IBoolean>::ToVariant(object, targetType, ctx); }},
                       {IInteger::Id,
                        [](const BaseObjectPtr& object, const UA_DataType* targetType, const ContextPtr& ctx)
                        { return VariantConverter<IInteger>::ToVariant(object, targetType, ctx); }},
                       {IFloat::Id,
                        [](const BaseObjectPtr& object, const UA_DataType* targetType, const ContextPtr& ctx)
                        { return VariantConverter<IFloat>::ToVariant(object, targetType, ctx); }},
                       {IDict::Id,
                        [](const BaseObjectPtr& object, const UA_DataType* targetType, const ContextPtr& ctx)
                        { return VariantConverter<IDict>::ToVariant(object, targetType, ctx); }},
                       {IDataDescriptor::Id,
                        [](const BaseObjectPtr& object, const UA_DataType* targetType, const ContextPtr& ctx)
                        { return VariantConverter<IDataDescriptor>::ToVariant(object, targetType, ctx); }},
                       {IDimension::Id,
                        [](const BaseObjectPtr& object, const UA_DataType* targetType, const ContextPtr& ctx)
                        { return VariantConverter<IDimension>::ToVariant(object, targetType, ctx); }},
                       {IDimensionRule::Id,
                        [](const BaseObjectPtr& object, const UA_DataType* targetType, const ContextPtr& ctx)
                        { return VariantConverter<IDimensionRule>::ToVariant(object, targetType, ctx); }},
                       {IDataRule::Id,
                        [](const BaseObjectPtr& object, const UA_DataType* targetType, const ContextPtr& ctx)
                        { return VariantConverter<IDataRule>::ToVariant(object, targetType, ctx); }},
                       {IFunctionBlockType::Id,
                        [](const BaseObjectPtr& object, const UA_DataType* targetType, const ContextPtr& ctx)
                        { return VariantConverter<IFunctionBlockType>::ToVariant(object, targetType, ctx); }},
                       {IScaling::Id,
                        [](const BaseObjectPtr& object, const UA_DataType* targetType, const ContextPtr& ctx)
                        { return VariantConverter<IScaling>::ToVariant(object, targetType, ctx); }},
                       {IArgumentInfo::Id,
                        [](const BaseObjectPtr& object, const UA_DataType* targetType, const ContextPtr& ctx)
                        { return VariantConverter<IArgumentInfo>::ToVariant(object, targetType, ctx); }},
                       {IRange::Id,
                        [](const BaseObjectPtr& object, const UA_DataType* targetType, const ContextPtr& ctx)
                        { return VariantConverter<IRange>::ToVariant(object, targetType, ctx); }},
                       {IComplexNumber::Id,
                        [](const BaseObjectPtr& object, const UA_DataType* targetType, const ContextPtr& ctx)
                        { return VariantConverter<IComplexNumber>::ToVariant(object, targetType, ctx); }},
                       {IStruct::Id,
                        [](const BaseObjectPtr& object, const UA_DataType* targetType, const ContextPtr& ctx)
                        { return VariantConverter<IStruct>::ToVariant(object, targetType, ctx); }},
                       {IList::Id, [](const BaseObjectPtr& object, const UA_DataType* targetType, const ContextPtr& ctx) {
                            return VariantConverter<IBaseObject>::ToArrayVariant(object, targetType, ctx);
                        }}};

    static std::unordered_map<IntfID, std::function<OpcUaVariant(const ListPtr<IBaseObject>&, const UA_DataType*, const ContextPtr&)>>
        idToArrayVariantMap{{INumber::Id,
                             [](const ListPtr<IBaseObject>& object, const UA_DataType* targetType, const ContextPtr& ctx)
                             { return VariantConverter<INumber>::ToArrayVariant(object, targetType, ctx); }},
                            {IString::Id,
                             [](const ListPtr<IBaseObject>& object, const UA_DataType* targetType, const ContextPtr& ctx)
                             { return VariantConverter<IString>::ToArrayVariant(object, targetType, ctx); }},
                            {IUnit::Id,
                             [](const ListPtr<IBaseObject>& object, const UA_DataType* targetType, const ContextPtr& ctx)
                             { return VariantConverter<IUnit>::ToArrayVariant(object, targetType, ctx); }},
                            {IRatio::Id,
                             [](const ListPtr<IBaseObject>& object, const UA_DataType* targetType, const ContextPtr& ctx)
                             { return VariantConverter<IRatio>::ToArrayVariant(object, targetType, ctx); }},
                            {IBoolean::Id,
                             [](const ListPtr<IBaseObject>& object, const UA_DataType* targetType, const ContextPtr& ctx)
                             { return VariantConverter<IBoolean>::ToArrayVariant(object, targetType, ctx); }},
                            {IInteger::Id,
                             [](const ListPtr<IBaseObject>& object, const UA_DataType* targetType, const ContextPtr& ctx)
                             { return VariantConverter<IInteger>::ToArrayVariant(object, targetType, ctx); }},
                            {IFloat::Id,
                             [](const ListPtr<IBaseObject>& object, const UA_DataType* targetType, const ContextPtr& ctx)
                             { return VariantConverter<IFloat>::ToArrayVariant(object, targetType, ctx); }},
                            {IDataDescriptor::Id,
                             [](const ListPtr<IBaseObject>& object, const UA_DataType* targetType, const ContextPtr& ctx)
                             { return VariantConverter<IDataDescriptor>::ToArrayVariant(object, targetType, ctx); }},
                            {IDimension::Id,
                             [](const BaseObjectPtr& object, const UA_DataType* targetType, const ContextPtr& ctx)
                             { return VariantConverter<IDimension>::ToArrayVariant(object, targetType, ctx); }},
                            {IDimensionRule::Id,
                             [](const BaseObjectPtr& object, const UA_DataType* targetType, const ContextPtr& ctx)
                             { return VariantConverter<IDimensionRule>::ToArrayVariant(object, targetType, ctx); }},
                            {IDataRule::Id,
                             [](const BaseObjectPtr& object, const UA_DataType* targetType, const ContextPtr& ctx)
                             { return VariantConverter<IDataRule>::ToArrayVariant(object, targetType, ctx); }},
                            {IFunctionBlockType::Id,
                             [](const BaseObjectPtr& object, const UA_DataType* targetType, const ContextPtr& ctx)
                             { return VariantConverter<IFunctionBlockType>::ToArrayVariant(object, targetType, ctx); }},
                            {IScaling::Id,
                             [](const BaseObjectPtr& object, const UA_DataType* targetType, const ContextPtr& ctx)
                             { return VariantConverter<IScaling>::ToArrayVariant(object, targetType, ctx); }},
                            {IRange::Id,
                             [](const BaseObjectPtr& object, const UA_DataType* targetType, const ContextPtr& ctx)
                             { return VariantConverter<IRange>::ToArrayVariant(object, targetType, ctx); }},
                            {IComplexNumber::Id,
                             [](const BaseObjectPtr& object, const UA_DataType* targetType, const ContextPtr& ctx)
                             { return VariantConverter<IComplexNumber>::ToArrayVariant(object, targetType, ctx); }},
                            {IStruct::Id,
                             [](const BaseObjectPtr& object, const UA_DataType* targetType, const ContextPtr& ctx)
                             { return VariantConverter<IStruct>::ToArrayVariant(object, targetType, ctx); }},
                            {IArgumentInfo::Id, [](const BaseObjectPtr& object, const UA_DataType* targetType, const ContextPtr& ctx) {
                                 return VariantConverter<IArgumentInfo>::ToArrayVariant(object, targetType, ctx);
                             }}};

    // Does not include DataRule and DimensionRule due to ambiguity issues.
    // Does not include generic struct converter.
    static std::unordered_map<OpcUaNodeId, std::function<BaseObjectPtr(const OpcUaVariant&, const ContextPtr& context)>> uaTypeToDaqObject{
        {OpcUaNodeId(0, UA_NS0ID_BOOLEAN),
         [](const OpcUaVariant& var, const ContextPtr& context) { return VariantConverter<IBoolean>::ToDaqObject(var, context); }},
        {OpcUaNodeId(0, UA_NS0ID_FLOAT),
         [](const OpcUaVariant& var, const ContextPtr& context) { return VariantConverter<IFloat>::ToDaqObject(var, context); }},
        {OpcUaNodeId(0, UA_NS0ID_DOUBLE),
         [](const OpcUaVariant& var, const ContextPtr& context) { return VariantConverter<IFloat>::ToDaqObject(var, context); }},
        {OpcUaNodeId(0, UA_NS0ID_SBYTE),
         [](const OpcUaVariant& var, const ContextPtr& context) { return VariantConverter<IInteger>::ToDaqObject(var, context); }},
        {OpcUaNodeId(0, UA_NS0ID_BYTE),
         [](const OpcUaVariant& var, const ContextPtr& context) { return VariantConverter<IInteger>::ToDaqObject(var, context); }},
        {OpcUaNodeId(0, UA_NS0ID_INT16),
         [](const OpcUaVariant& var, const ContextPtr& context) { return VariantConverter<IInteger>::ToDaqObject(var, context); }},
        {OpcUaNodeId(0, UA_NS0ID_UINT16),
         [](const OpcUaVariant& var, const ContextPtr& context) { return VariantConverter<IInteger>::ToDaqObject(var, context); }},
        {OpcUaNodeId(0, UA_NS0ID_INT32),
         [](const OpcUaVariant& var, const ContextPtr& context) { return VariantConverter<IInteger>::ToDaqObject(var, context); }},
        {OpcUaNodeId(0, UA_NS0ID_UINT32),
         [](const OpcUaVariant& var, const ContextPtr& context) { return VariantConverter<IInteger>::ToDaqObject(var, context); }},
        {OpcUaNodeId(0, UA_NS0ID_INT64),
         [](const OpcUaVariant& var, const ContextPtr& context) { return VariantConverter<IInteger>::ToDaqObject(var, context); }},
        {OpcUaNodeId(0, UA_NS0ID_UINT64),
         [](const OpcUaVariant& var, const ContextPtr& context) { return VariantConverter<IInteger>::ToDaqObject(var, context); }},
        {OpcUaNodeId(0, UA_NS0ID_STRING),
         [](const OpcUaVariant& var, const ContextPtr& context) { return VariantConverter<IString>::ToDaqObject(var, context); }},
        {OpcUaNodeId(0, UA_NS0ID_LOCALIZEDTEXT),
         [](const OpcUaVariant& var, const ContextPtr& context) { return VariantConverter<IString>::ToDaqObject(var, context); }},
        {OpcUaNodeId(0, UA_NS0ID_QUALIFIEDNAME),
         [](const OpcUaVariant& var, const ContextPtr& context) { return VariantConverter<IString>::ToDaqObject(var, context); }},
        {OpcUaNodeId(NAMESPACE_DAQBT, UA_DAQBTID_EUINFORMATIONWITHQUANTITY),
         [](const OpcUaVariant& var, const ContextPtr& context) { return VariantConverter<IUnit>::ToDaqObject(var, context); }},
        {OpcUaNodeId(0, UA_NS0ID_EUINFORMATION),
         [](const OpcUaVariant& var, const ContextPtr& context) { return VariantConverter<IUnit>::ToDaqObject(var, context); }},
        {OpcUaNodeId(0, UA_NS0ID_RATIONALNUMBER),
         [](const OpcUaVariant& var, const ContextPtr& context) { return VariantConverter<IRatio>::ToDaqObject(var, context); }},
        {OpcUaNodeId(NAMESPACE_DAQBT, UA_DAQBTID_RATIONALNUMBER64),
         [](const OpcUaVariant& var, const ContextPtr& context) { return VariantConverter<IRatio>::ToDaqObject(var, context); }},
        {OpcUaNodeId(NAMESPACE_DAQBSP, UA_DAQBSPID_DATADESCRIPTORSTRUCTURE),
         [](const OpcUaVariant& var, const ContextPtr& context) { return VariantConverter<IDataDescriptor>::ToDaqObject(var, context); }},
        {OpcUaNodeId(NAMESPACE_DAQBSP, UA_DAQBSPID_STRUCTDESCRIPTORSTRUCTURE),
         [](const OpcUaVariant& var, const ContextPtr& context) { return VariantConverter<IDataDescriptor>::ToDaqObject(var, context); }},
        {OpcUaNodeId(NAMESPACE_DAQBSP, UA_DAQBSPID_DIMENSIONDESCRIPTORSTRUCTURE),
         [](const OpcUaVariant& var, const ContextPtr& context) { return VariantConverter<IDimension>::ToDaqObject(var, context); }},
        {OpcUaNodeId(NAMESPACE_DAQBSP, UA_DAQBSPID_FUNCTIONBLOCKINFOSTRUCTURE),
         [](const OpcUaVariant& var, const ContextPtr& context)
         { return VariantConverter<IFunctionBlockType>::ToDaqObject(var, context); }},
        {OpcUaNodeId(NAMESPACE_DAQBSP, UA_DAQBSPID_POSTSCALINGSTRUCTURE),
         [](const OpcUaVariant& var, const ContextPtr& context) { return VariantConverter<IScaling>::ToDaqObject(var, context); }},
        {OpcUaNodeId(NAMESPACE_DAQBSP, UA_DAQBSPID_LINEARSCALINGDESCRIPTIONSTRUCTURE),
         [](const OpcUaVariant& var, const ContextPtr& context) { return VariantConverter<IScaling>::ToDaqObject(var, context); }},
        {OpcUaNodeId(0, UA_NS0ID_ARGUMENT),
         [](const OpcUaVariant& var, const ContextPtr& context) { return VariantConverter<IArgumentInfo>::ToDaqObject(var, context); }},
        {OpcUaNodeId(0, UA_NS0ID_RANGE),
         [](const OpcUaVariant& var, const ContextPtr& context) { return VariantConverter<IRange>::ToDaqObject(var, context); }},
        {OpcUaNodeId(0, UA_NS0ID_COMPLEXNUMBERTYPE),
         [](const OpcUaVariant& var, const ContextPtr& context) { return VariantConverter<IComplexNumber>::ToDaqObject(var, context); }},
        {OpcUaNodeId(0, UA_NS0ID_DOUBLECOMPLEXNUMBERTYPE),
         [](const OpcUaVariant& var, const ContextPtr& context) { return VariantConverter<IComplexNumber>::ToDaqObject(var, context); }}};

    static std::unordered_map<OpcUaNodeId, std::function<ListPtr<IBaseObject>(const OpcUaVariant&, const ContextPtr& context)>>
        uaTypeToDaqList{
            {OpcUaNodeId(0, UA_NS0ID_BOOLEAN),
             [](const OpcUaVariant& var, const ContextPtr& context) { return VariantConverter<IBoolean>::ToDaqList(var, context); }},
            {OpcUaNodeId(0, UA_NS0ID_FLOAT),
             [](const OpcUaVariant& var, const ContextPtr& context) { return VariantConverter<IFloat>::ToDaqList(var, context); }},
            {OpcUaNodeId(0, UA_NS0ID_DOUBLE),
             [](const OpcUaVariant& var, const ContextPtr& context) { return VariantConverter<IFloat>::ToDaqList(var, context); }},
            {OpcUaNodeId(0, UA_NS0ID_SBYTE),
             [](const OpcUaVariant& var, const ContextPtr& context) { return VariantConverter<IInteger>::ToDaqList(var, context); }},
            {OpcUaNodeId(0, UA_NS0ID_BYTE),
             [](const OpcUaVariant& var, const ContextPtr& context) { return VariantConverter<IInteger>::ToDaqList(var, context); }},
            {OpcUaNodeId(0, UA_NS0ID_INT16),
             [](const OpcUaVariant& var, const ContextPtr& context) { return VariantConverter<IInteger>::ToDaqList(var, context); }},
            {OpcUaNodeId(0, UA_NS0ID_UINT16),
             [](const OpcUaVariant& var, const ContextPtr& context) { return VariantConverter<IInteger>::ToDaqList(var, context); }},
            {OpcUaNodeId(0, UA_NS0ID_INT32),
             [](const OpcUaVariant& var, const ContextPtr& context) { return VariantConverter<IInteger>::ToDaqList(var, context); }},
            {OpcUaNodeId(0, UA_NS0ID_UINT32),
             [](const OpcUaVariant& var, const ContextPtr& context) { return VariantConverter<IInteger>::ToDaqList(var, context); }},
            {OpcUaNodeId(0, UA_NS0ID_INT64),
             [](const OpcUaVariant& var, const ContextPtr& context) { return VariantConverter<IInteger>::ToDaqList(var, context); }},
            {OpcUaNodeId(0, UA_NS0ID_UINT64),
             [](const OpcUaVariant& var, const ContextPtr& context) { return VariantConverter<IInteger>::ToDaqList(var, context); }},
            {OpcUaNodeId(0, UA_NS0ID_STRING),
             [](const OpcUaVariant& var, const ContextPtr& context) { return VariantConverter<IString>::ToDaqList(var, context); }},
            {OpcUaNodeId(0, UA_NS0ID_LOCALIZEDTEXT),
             [](const OpcUaVariant& var, const ContextPtr& context) { return VariantConverter<IString>::ToDaqList(var, context); }},
            {OpcUaNodeId(0, UA_NS0ID_QUALIFIEDNAME),
             [](const OpcUaVariant& var, const ContextPtr& context) { return VariantConverter<IString>::ToDaqList(var, context); }},
            {OpcUaNodeId(NAMESPACE_DAQBT, UA_TYPES_DAQBT_EUINFORMATIONWITHQUANTITY),
             [](const OpcUaVariant& var, const ContextPtr& context) { return VariantConverter<IUnit>::ToDaqList(var, context); }},
            {OpcUaNodeId(0, UA_TYPES_EUINFORMATION),
             [](const OpcUaVariant& var, const ContextPtr& context) { return VariantConverter<IUnit>::ToDaqList(var, context); }},
            {OpcUaNodeId(0, UA_NS0ID_RATIONALNUMBER),
             [](const OpcUaVariant& var, const ContextPtr& context) { return VariantConverter<IRatio>::ToDaqList(var, context); }},
            {OpcUaNodeId(NAMESPACE_DAQBT, UA_DAQBTID_RATIONALNUMBER64),
             [](const OpcUaVariant& var, const ContextPtr& context) { return VariantConverter<IRatio>::ToDaqList(var, context); }},
            {OpcUaNodeId(NAMESPACE_DAQBSP, UA_DAQBSPID_DATADESCRIPTORSTRUCTURE),
             [](const OpcUaVariant& var, const ContextPtr& context) { return VariantConverter<IDataDescriptor>::ToDaqList(var, context); }},
            {OpcUaNodeId(NAMESPACE_DAQBSP, UA_DAQBSPID_STRUCTDESCRIPTORSTRUCTURE),
             [](const OpcUaVariant& var, const ContextPtr& context) { return VariantConverter<IDataDescriptor>::ToDaqList(var, context); }},
            {OpcUaNodeId(NAMESPACE_DAQBSP, UA_DAQBSPID_DIMENSIONDESCRIPTORSTRUCTURE),
             [](const OpcUaVariant& var, const ContextPtr& context) { return VariantConverter<IDimension>::ToDaqList(var, context); }},
            {OpcUaNodeId(NAMESPACE_DAQBSP, UA_DAQBSPID_FUNCTIONBLOCKINFOSTRUCTURE),
             [](const OpcUaVariant& var, const ContextPtr& context) { return VariantConverter<IFunctionBlockType>::ToDaqList(var, context); }},
            {OpcUaNodeId(NAMESPACE_DAQBSP, UA_DAQBSPID_POSTSCALINGSTRUCTURE),
             [](const OpcUaVariant& var, const ContextPtr& context) { return VariantConverter<IScaling>::ToDaqList(var, context); }},
            {OpcUaNodeId(NAMESPACE_DAQBSP, UA_DAQBSPID_LINEARSCALINGDESCRIPTIONSTRUCTURE),
             [](const OpcUaVariant& var, const ContextPtr& context) { return VariantConverter<IScaling>::ToDaqList(var, context); }},
            {OpcUaNodeId(0, UA_NS0ID_ARGUMENT),
             [](const OpcUaVariant& var, const ContextPtr& context) { return VariantConverter<IArgumentInfo>::ToDaqList(var, context); }},
            {OpcUaNodeId(0, UA_NS0ID_RANGE),
             [](const OpcUaVariant& var, const ContextPtr& context) { return VariantConverter<IRange>::ToDaqList(var, context); }},
            {OpcUaNodeId(0, UA_NS0ID_COMPLEXNUMBERTYPE),
             [](const OpcUaVariant& var, const ContextPtr& context) { return VariantConverter<IComplexNumber>::ToDaqList(var, context); }},
            {OpcUaNodeId(0, UA_NS0ID_DOUBLECOMPLEXNUMBERTYPE),
             [](const OpcUaVariant& var, const ContextPtr& context) { return VariantConverter<IComplexNumber>::ToDaqList(var, context); }}};

    static OpcUaVariant convertToVariant(IntfID interfaceId,
                                         const BaseObjectPtr& object,
                                         const UA_DataType* targetType,
                                         const ContextPtr& context)
    {
        if (const auto it = idToVariantMap.find(interfaceId); it != idToVariantMap.cend())
            return it->second(object, targetType, context);

        return {};
    }

    static OpcUaVariant convertToArrayVariant(IntfID elementId,
                                              const ListPtr<IBaseObject>& list,
                                              const UA_DataType* targetType,
                                              const ContextPtr& context)
    {
        if (const auto it = idToArrayVariantMap.find(elementId); it != idToArrayVariantMap.cend())
            return it->second(list, targetType, context);

        return {};
    }

    static BaseObjectPtr convertToDaqObject(const OpcUaVariant& variant, const ContextPtr& context)
    {
        if (variant.isNull())
            return nullptr;

        const auto typeId = variant.getValue().type->typeId;
        if (const auto it = uaTypeToDaqObject.find(OpcUaNodeId(typeId)); it != uaTypeToDaqObject.cend())
            return it->second(variant, context);

        return nullptr;
    }

    static BaseObjectPtr convertToDaqList(const OpcUaVariant& variant, const ContextPtr& context)
    {
        if (variant.isNull()) 
            return nullptr;

        const auto typeId = variant.getValue().type->typeId;
        if (const auto it = uaTypeToDaqList.find(OpcUaNodeId(typeId)); it != uaTypeToDaqList.cend())
            return it->second(variant, context);

        return nullptr;
    }
}

END_NAMESPACE_OPENDAQ_OPCUA_TMS
