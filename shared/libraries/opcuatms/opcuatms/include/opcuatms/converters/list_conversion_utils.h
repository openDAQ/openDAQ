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
#include "opcuashared/opcuavariant.h"
#include "opcuatms/opcuatms.h"
#include "opcuatms/extension_object.h"
#include "opcuatms/converters/variant_converter.h"
#include "opcuatms/converters/struct_converter.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

template <>
OpcUaVariant VariantConverter<IBaseObject>::ToVariant(const BaseObjectPtr& object,
                                                      const UA_DataType* targetType,
                                                      const ContextPtr& context);

class ListConversionUtils
{
public:
    template <typename BlueberryType, typename TmsType>
    static OpcUaVariant ToArrayVariant(const ListPtr<BlueberryType>& list, const ContextPtr& context = nullptr);
    template <typename BlueberryType, typename TmsType>
    static ListPtr<BlueberryType> VariantToList(const OpcUaVariant& variant, const ContextPtr& context = nullptr);

    template <typename BlueberryType>
    static OpcUaVariant ToExtensionObjectArrayVariant(const ListPtr<BlueberryType>& list, const ContextPtr& context = nullptr);
    template <typename BlueberryType>
    static ListPtr<BlueberryType> ExtensionObjectVariantToList(const OpcUaVariant& variant, const ContextPtr& context = nullptr);

    static OpcUaVariant ToVariantTypeArrayVariant(const ListPtr<IBaseObject>& list, const ContextPtr& context = nullptr);
    static ListPtr<IBaseObject> VariantTypeArrayToList(const OpcUaVariant& variant, const ContextPtr& context = nullptr);
};

inline OpcUaVariant ListConversionUtils::ToVariantTypeArrayVariant(const ListPtr<IBaseObject>& list, const ContextPtr& context)
{
    constexpr auto type = GetUaDataType<UA_Variant>();
    const auto arr = static_cast<UA_Variant*>(UA_Array_new(list.getCount(), type));

    for (SizeT i = 0; i < list.getCount(); i++)
    {
        auto tmsStruct = VariantConverter<IBaseObject>::ToVariant(list.getItemAt(i), nullptr, context);
        arr[i] = tmsStruct.getDetachedValue();
    }

    auto variant = OpcUaVariant();
    UA_Variant_setArray(variant.get(), arr, list.getCount(), type);
    return variant;
}

inline ListPtr<IBaseObject> ListConversionUtils::VariantTypeArrayToList(const OpcUaVariant& variant, const ContextPtr& context)
{
    if (!variant.isType<UA_Variant>())
        throw ConversionFailedException{};

    auto list = List<IBaseObject>();
    const auto data = static_cast<UA_Variant*>(variant->data);

    for (size_t i = 0; i < variant->arrayLength; i++)
    {
        OpcUaVariant arrayField = OpcUaVariant(data[i]);
        const auto obj = VariantConverter<IBaseObject>::ToDaqObject(arrayField, context);
        list.pushBack(obj);
    }

    return list;
}

template <typename BlueberryType, typename TmsType>
OpcUaVariant ListConversionUtils::ToArrayVariant(const ListPtr<BlueberryType>& list, const ContextPtr& context)
{
    constexpr auto type = GetUaDataType<TmsType>();
    auto arr = static_cast<TmsType*>(UA_Array_new(list.getCount(), type));
    try
    {
        for (SizeT i = 0; i < list.getCount(); i++)
        {
            auto tmsStruct = StructConverter<BlueberryType, TmsType>::ToTmsType(list.getItemAt(i), context);
            arr[i] = tmsStruct.getDetachedValue();
        }
    }
    catch (...)
    {
        UA_Array_delete(arr, list.getCount(), type);
        throw;
    }
    
    auto variant = OpcUaVariant();
    UA_Variant_setArray(variant.get(), arr, list.getCount(), type);
    return variant;
}

template <typename BlueberryType, typename TmsType>
ListPtr<BlueberryType> ListConversionUtils::VariantToList(const OpcUaVariant& variant, const ContextPtr& context)
{
    if (!variant.isType<TmsType>())
        throw ConversionFailedException();

    auto data = static_cast<TmsType*>(variant->data);
    auto list = List<BlueberryType>();

    for (size_t i = 0; i < variant->arrayLength; i++)
    {
        const auto obj = StructConverter<BlueberryType, TmsType>::ToDaqObject(data[i], context);
        list.pushBack(obj);
    }

    return list;
}

template <typename BlueberryType>
OpcUaVariant ListConversionUtils::ToExtensionObjectArrayVariant(const ListPtr<BlueberryType>& list, const ContextPtr& context)
{
    constexpr auto type = GetUaDataType<UA_ExtensionObject>();
    const auto arr = static_cast<UA_ExtensionObject*>(UA_Array_new(list.getCount(), type));

    try
    {
        for (SizeT i = 0; i < list.getCount(); i++)
        {
            auto variant = VariantConverter<BlueberryType>::ToVariant(list.getItemAt(i), nullptr, context);
            auto extensionObject = ExtensionObject(variant);
            arr[i] = extensionObject.getDetachedValue();
        }
    }
    catch(...)
    {
        UA_Array_delete(arr, list.getCount(), type);
        throw;
    }

    auto variant = OpcUaVariant();
    UA_Variant_setArray(variant.get(), static_cast<void*>(arr), list.getCount(), type);
    return variant;
}

template <typename BlueberryType>
ListPtr<BlueberryType> ListConversionUtils::ExtensionObjectVariantToList(const OpcUaVariant& variant, const ContextPtr& context)
{
    if (!variant.isType<UA_ExtensionObject>())
        throw ConversionFailedException();

    const auto data = static_cast<UA_ExtensionObject*>(variant->data);
    auto list = List<BlueberryType>();

    for (size_t i = 0; i < variant->arrayLength; i++)
    {
        auto extensionObject = ExtensionObject(data[i]);
        BaseObjectPtr object = nullptr;
        if (extensionObject.isDecoded())
            object = VariantConverter<BlueberryType>::ToDaqObject(extensionObject.getAsVariant(), context);

        list.pushBack(object);
    }

    return list;
}


END_NAMESPACE_OPENDAQ_OPCUA_TMS
