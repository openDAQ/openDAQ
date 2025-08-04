#include <opcuatms/core_types_utils.h>
#include <opcuatms/extension_object.h>
#include <opcuatms/converters/struct_converter.h>
#include <opcuatms/converters/variant_converter.h>
#include <opcuatms/converters/list_conversion_utils.h>
#include <coretypes/struct_factory.h>
#include <coretypes/struct_type_factory.h>
#include <coretypes/simple_type_factory.h>
#include <opendaq/custom_log.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

namespace detail
{
    static TypePtr createType(const BaseObjectPtr& obj)
    {
        const auto ct = obj.getCoreType();
        if (ct == ctStruct)
            return obj.asPtr<IStruct>().getStructType();
        else if (ct == ctEnumeration)
            return obj.asPtr<IEnumeration>().getEnumerationType();
        else
            return SimpleType(ct);
    }
}

template <>
StructPtr VariantConverter<IStruct>::ToDaqObject(const OpcUaVariant& variant, const ContextPtr& context)
{
    if (variant.isNull())
        return nullptr;

    if (!context.assigned() || !context.getTypeManager().assigned())
        DAQ_THROW_EXCEPTION(ConversionFailedException, "Generic struct conversion requires the TypeManager.");

    const auto typeManager = context.getTypeManager();

    const auto type = variant->type;

    const UA_DataTypeMember* members = type->members;
    const UA_UInt32 membersSize = type->membersSize;
    auto src = reinterpret_cast<uintptr_t>(variant->data);

    DictPtr<IString, IBaseObject> daqMembers = Dict<IString, IBaseObject>();
    ListPtr<IType> memberTypes = List<IType>();

    for (SizeT i = 0; i < membersSize; ++i)
    {
        const UA_DataTypeMember* member = &members[i];
        const UA_DataType* memberType = member->memberType;
        src += member->padding;

        // TODO: Refactor this
        if (!member->isOptional)
        {
            if (!member->isArray)
            {
                OpcUaVariant memberVariant{};
                UA_Variant_setScalarCopy(&memberVariant.getValue(), reinterpret_cast<void*>(src), memberType);
                const auto obj = VariantConverter<IBaseObject>::ToDaqObject(memberVariant, context);
                daqMembers.set(member->memberName, obj);
                src += memberType->memSize;
                memberTypes.pushBack(detail::createType(obj));
            }
            else
            {
                const size_t size = *reinterpret_cast<const size_t*>(src);
                src += sizeof(size_t);
                OpcUaVariant memberVariant{};
                UA_Variant_setArrayCopy(&memberVariant.getValue(), *reinterpret_cast<void**>(src), size, memberType);

                const auto obj = VariantConverter<IBaseObject>::ToDaqObject(memberVariant, context);
                daqMembers.set(member->memberName, obj);
                memberTypes.pushBack(detail::createType(obj));
                src += sizeof(void*);
            }
        }
        else
        {
            if (!member->isArray)
            {
                if(*reinterpret_cast<void* const*>(src) != nullptr)
                {
                    OpcUaVariant memberVariant{};
                    UA_Variant_setScalarCopy(&memberVariant.getValue(), *reinterpret_cast<void**>(src), memberType);

                    const auto obj = VariantConverter<IBaseObject>::ToDaqObject(memberVariant, context);
                    daqMembers.set(member->memberName, obj);
                    memberTypes.pushBack(detail::createType(obj));
                }
                else
                {
                    daqMembers.set(member->memberName, nullptr);
                    // TODO: Set appropriate type
                    memberTypes.pushBack(SimpleType(ctUndefined));
                }
            }
            else
            {
                if(*reinterpret_cast<void* const*>(src + sizeof(size_t)) != nullptr)
                {
                    const size_t size = *reinterpret_cast<const size_t*>(src);
                    src += sizeof(size_t);
                    OpcUaVariant memberVariant{};
                    UA_Variant_setArrayCopy(&memberVariant.getValue(), reinterpret_cast<void* const*>(src), size, memberType);

                    const auto obj = VariantConverter<IBaseObject>::ToDaqObject(memberVariant, context);
                    daqMembers.set(member->memberName, obj);
                    memberTypes.pushBack(detail::createType(obj));
                }
                else
                {
                    daqMembers.set(member->memberName, nullptr);
                    src += sizeof(size_t*);
                    memberTypes.pushBack(SimpleType(ctUndefined));
                }
            }
            src += sizeof(void*);
        }
    }

    try
    {
        typeManager.addType(StructType(type->typeName, daqMembers.getKeyList(), memberTypes));
    }
    catch (const std::exception& e)
    {
        const auto loggerComponent = context.getLogger().getOrAddComponent("GenericStructConverter");
        LOG_W("Couldn't add type {} to type manager: {}", type->typeName, e.what());
    }
    catch (...)
    {
        const auto loggerComponent = context.getLogger().getOrAddComponent("GenericStructConverter");
        LOG_W("Couldn't add type {} to type manager!", type->typeName);
    }

    return Struct(type->typeName, daqMembers, typeManager);
}

template <>
OpcUaVariant VariantConverter<IStruct>::ToVariant(const StructPtr& object, const UA_DataType* /*targetType*/, const ContextPtr& context)
{
    const auto type = GetUAStructureDataTypeByName(object.getStructType().getName());
    if (type == nullptr)
        DAQ_THROW_EXCEPTION(ConversionFailedException);

    const UA_DataTypeMember* members = type->members;
    const UA_UInt32 membersSize = type->membersSize;

    void* data = UA_new(type);
    const auto daqMembers = object.getAsDictionary();

    if (membersSize != daqMembers.getCount())
        DAQ_THROW_EXCEPTION(ConversionFailedException);

    auto dst = reinterpret_cast<uintptr_t>(data);
    for (SizeT i = 0; i < membersSize; ++i)
    {
        const UA_DataTypeMember* member = &members[i];
        const UA_DataType* memberType = member->memberType;

        if (!daqMembers.hasKey(member->memberName))
            DAQ_THROW_EXCEPTION(ConversionFailedException);

        auto daqMember = daqMembers.get(member->memberName);
        dst += member->padding;

        if (!daqMember.assigned())
        {
            if (member->isOptional)
            {
                if (member->isArray)
                    dst += sizeof(size_t*);
                dst += sizeof(void*);
                continue;
            }

            DAQ_THROW_EXCEPTION(ConversionFailedException);
        }

        OpcUaVariant variant = VariantConverter<IBaseObject>::ToVariant(daqMember, memberType, context);
        if (variant->type != memberType && !(variant->data == UA_EMPTY_ARRAY_SENTINEL && variant->arrayLength == 0))
            DAQ_THROW_EXCEPTION(ConversionFailedException);

        void* src = variant->data;

        if (!member->isArray)
        {
            if (!member->isOptional)
            {
                UA_copy(src, reinterpret_cast<void*>(dst), memberType);
                dst += memberType->memSize;
            }
            else
            {
                [[maybe_unused]]
                const UA_StatusCode retval = UA_Array_copy(src, 1, reinterpret_cast<void**>(dst), memberType);
                dst += sizeof(void*);
            }
        }
        else
        {
            auto *dst_size = reinterpret_cast<size_t*>(dst);
            const size_t size = variant->arrayLength;
            dst += sizeof(size_t);
            const UA_StatusCode retval = UA_Array_copy(src, size, reinterpret_cast<void**>(dst), memberType);
            if(retval == UA_STATUSCODE_GOOD)
                *dst_size = size;
            else
                *dst_size = 0;
            dst += sizeof(void*);
        }
    }
    
    OpcUaVariant variant{};
    UA_Variant_setScalar(&variant.getValue(), data, type);

    return variant;
}

template <>
ListPtr<IStruct> VariantConverter<IStruct>::ToDaqList(const OpcUaVariant& variant, const ContextPtr& context)
{
    auto list = List<IStruct>();
    auto src = reinterpret_cast<uintptr_t>(variant->data);

    for (size_t i = 0; i < variant->arrayLength; i++)
    {
        OpcUaVariant varObj;
        UA_Variant_setScalarCopy(&varObj.getValue(), reinterpret_cast<void*>(src), variant->type);

        const auto obj = ToDaqObject(varObj, context);
        list.pushBack(obj);
        src += variant->type->memSize;
    }

    return list;
}

template <>
OpcUaVariant VariantConverter<IStruct>::ToArrayVariant(const ListPtr<IStruct>& list,
                                                       const UA_DataType* /*targetType*/,
                                                       const ContextPtr& context)
{
    if (list.empty())
    {
        auto varObj = OpcUaVariant();
        varObj->data = UA_EMPTY_ARRAY_SENTINEL;
        return varObj;
    }
    
    auto firstConvertedStruct = ToVariant(list[0], nullptr, context);
    auto type = firstConvertedStruct->type;

    auto arr = UA_Array_new(list.getCount(), type);
    auto dst = reinterpret_cast<uintptr_t>(arr);

    UA_copy(firstConvertedStruct->data, reinterpret_cast<void*>(dst), firstConvertedStruct->type);
    dst += firstConvertedStruct->type->memSize;
    
    for (SizeT i = 1; i < list.getCount(); i++)
    {
        auto convertedStruct = ToVariant(list[i], nullptr, context);
        if (convertedStruct->type != type)
            DAQ_THROW_EXCEPTION(ConversionFailedException);

        UA_copy(convertedStruct->data, reinterpret_cast<void*>(dst), convertedStruct->type);
        dst += convertedStruct->type->memSize;
    }

    auto variant = OpcUaVariant();
    UA_Variant_setArray(variant.get(), arr, list.getCount(), type);
    return variant;
}

END_NAMESPACE_OPENDAQ_OPCUA_TMS
