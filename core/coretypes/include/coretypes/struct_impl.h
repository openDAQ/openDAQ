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
#include <coretypes/struct.h>
#include <coretypes/coretype.h>
#include <coretypes/intfs.h>
#include <coretypes/string_ptr.h>
#include <coretypes/dictobject_factory.h>
#include <coretypes/baseobject_factory.h>
#include <coretypes/struct_ptr.h>
#include <coretypes/type_manager_ptr.h>
#include <coretypes/struct_type_factory.h>
#include <coretypes/stringobject_factory.h>
#include <coretypes/simple_type_factory.h>
#include <coretypes/struct_builder_ptr.h>
#include <coretypes/deserializer.h>

BEGIN_NAMESPACE_OPENDAQ

template <class StructInterface, class... Interfaces>
class GenericStructImpl : public ImplementationOf<StructInterface, ICoreType, ISerializable, Interfaces...>
{
public:
    explicit GenericStructImpl(const StringPtr& name, DictPtr<IString, IBaseObject> fields, const TypeManagerPtr& typeManager);
    explicit GenericStructImpl(const StructBuilderPtr& builder);

    ErrCode INTERFACE_FUNC equals(IBaseObject* other, Bool* equal) const override;

    ErrCode INTERFACE_FUNC getStructType(IStructType** type) override;
    ErrCode INTERFACE_FUNC getFieldNames(IList** names) override;
    ErrCode INTERFACE_FUNC getFieldValues(IList** values) override;
    ErrCode INTERFACE_FUNC get(IString* name, IBaseObject** field) override;
    ErrCode INTERFACE_FUNC getAsDictionary(IDict** dictionary) override;
    ErrCode INTERFACE_FUNC hasField(IString* name, Bool* hasField) override;

    ErrCode INTERFACE_FUNC getCoreType(CoreType* coreType) override;

    ErrCode INTERFACE_FUNC serialize(ISerializer* serializer) override;
    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override;

    ErrCode INTERFACE_FUNC toString(CharPtr* str) override;
    
    static ConstCharPtr SerializeId();

    static ErrCode Deserialize(ISerializedObject* ser, IBaseObject* context, IBaseObject** obj);

protected:
    explicit GenericStructImpl(StructTypePtr type, DictPtr<IString, IBaseObject> fields);

    StructTypePtr structType;
    DictPtr<IString, IBaseObject> fields;
};

using StructImpl = GenericStructImpl<IStruct>;


template <class StructInterface, class... Interfaces>
GenericStructImpl<StructInterface, Interfaces...>::GenericStructImpl (StructTypePtr type, DictPtr<IString, IBaseObject> fields)
    : structType(std::move(type))
    , fields(std::move(fields))
{
    this->fields.freeze();
}

template <class StructInterface, class... Interfaces>
GenericStructImpl<StructInterface, Interfaces...>::GenericStructImpl(const StringPtr& name,
                                                                     DictPtr<IString, IBaseObject> fields,
                                                                     const TypeManagerPtr& typeManager)
{
    if (!typeManager.assigned())
        throw InvalidParameterException{"Type manager must be assigned on custom Struct creation."};

    if (typeManager.hasType(name))
    {
        structType = typeManager.getType(name);
        const auto types = structType.getFieldTypes();
        const auto defaultValues = structType.getFieldDefaultValues();
        const auto names = structType.getFieldNames();

        std::unordered_set<std::string> namesSet;
        for (const auto& fieldName : names)
            namesSet.insert(fieldName);

        if (!fields.assigned())
            fields = Dict<IString, IBaseObject>();

        for (const auto& [key, val] : fields)
            if (!namesSet.count(key))
                throw InvalidParameterException{fmt::format(R"(Struct field "{}" is not part of the Struct type)", key.toStdString())};

        for (SizeT i = 0; i < types.getCount(); ++i)
        {
            auto fieldName = names[i];
            if (!fields.hasKey(fieldName))
            {
                if (defaultValues.assigned())
                    fields.set(fieldName, defaultValues[i]);
                else
                    fields.set(fieldName, nullptr);
            }
            else
            {
                auto fieldType = types[i];
                const auto field = fields.get(fieldName);
                if (!field.assigned())
                    continue;

                if (fieldType.getCoreType() == ctStruct && fieldType.template asPtrOrNull<IStructType>().assigned())
                {
                    StructPtr structObj = field;
                    if (structObj.getStructType() != fieldType)
                        throw InvalidParameterException{fmt::format(R"(Struct field "{}" type mismatch.)", fieldName.toStdString())};
                }
                else
                {
                    auto coreType = field.getCoreType();
                    auto fieldCoreType = fieldType.getCoreType();
                    if (fieldCoreType != ctUndefined && coreType != fieldCoreType)
                        throw InvalidParameterException{fmt::format(R"(Struct field "{}" type mismatch.)", fieldName.toStdString())};
                }
            }
        }
    }
    else
    {
        if (!fields.assigned())
            throw InvalidParameterException{"Fields dictionary is missing."};

        auto types = List<IType>();
        auto fieldValues = fields.getValueList();
        for (SizeT i = 0; i < fieldValues.getCount(); ++i)
        {
            const auto val = fieldValues[i];
            if (!val.assigned())
            {
                types.pushBack(SimpleType(ctUndefined));
                continue;
            }
            const auto ct = val.getCoreType();
            if (ct == ctStruct)
                types.pushBack(val.asPtr<IStruct>().getStructType());
            else
                types.pushBack(SimpleType(ct));
        }

        structType = StructType(name, fields.getKeyList(), types);
        typeManager.addType(structType);
    }

    this->fields = std::move(fields);
    this->fields.freeze();
}

template <class StructInterface, class... Interfaces>
GenericStructImpl<StructInterface, Interfaces...>::GenericStructImpl (const StructBuilderPtr& builder)
{
    structType = builder.getStructType();

    const auto types = structType.getFieldTypes();
    const auto defaultValues = structType.getFieldDefaultValues();
    const auto names = structType.getFieldNames();

    std::unordered_set<std::string> namesSet;
    for (const auto& fieldName : names)
        namesSet.insert(fieldName);
    
    auto fields = builder.getAsDictionary();
    if (!fields.assigned())
        fields = Dict<IString, IBaseObject>();

    for (const auto& [key, val] : fields)
        if (!namesSet.count(key))
            throw InvalidParameterException{fmt::format(R"(Struct field "{}" is not part of the Struct type)", key.toStdString())};

    for (SizeT i = 0; i < types.getCount(); ++i)
    {
        auto fieldName = names[i];
        if (!fields.hasKey(fieldName))
        {
            if (defaultValues.assigned())
                fields.set(fieldName, defaultValues[i]);
            else
                fields.set(fieldName, nullptr);
        }
        else
        {
            auto fieldType = types[i];
            const auto field = fields.get(fieldName);
            if (!field.assigned())
                continue;

            if (fieldType.getCoreType() == ctStruct && fieldType.template asPtrOrNull<IStructType>().assigned())
            {
                StructPtr structObj = field;
                if (structObj.getStructType() != fieldType)
                    throw InvalidParameterException{fmt::format(R"(Struct field "{}" type mismatch.)", fieldName.toStdString())};
            }
            else
            {
                auto coreType = field.getCoreType();
                auto fieldCoreType = fieldType.getCoreType();
                if (fieldCoreType != ctUndefined && coreType != fieldCoreType)
                    throw InvalidParameterException{fmt::format(R"(Struct field "{}" type mismatch.)", fieldName.toStdString())};
            }
        }
    }
    
    this->fields = std::move(fields);
    this->fields.freeze();
}


template <class StructInterface, class... Interfaces>
ErrCode GenericStructImpl<StructInterface, Interfaces...>::equals(IBaseObject* other, Bool* equal) const
{
    if (equal == nullptr)
        return this->makeErrorInfo(OPENDAQ_ERR_ARGUMENT_NULL, "Equals out-parameter must not be null");

    *equal = false;
    if (other == nullptr)
        return OPENDAQ_SUCCESS;

    const StructPtr structOther = BaseObjectPtr::Borrow(other).asPtrOrNull<IStruct>();
    if (structOther == nullptr)
        return OPENDAQ_SUCCESS;

    *equal = structOther.getFieldValues() == fields.getValueList() && structOther.getFieldNames() == fields.getKeyList() && structOther.getStructType() == structType;
    return OPENDAQ_SUCCESS;
}

template <class StructInterface, class... Interfaces>
ErrCode GenericStructImpl<StructInterface, Interfaces...>::getStructType(IStructType** type)
{
    if (!type)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *type = this->structType.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

template <class StructInterface, class... Interfaces>
ErrCode GenericStructImpl <StructInterface, Interfaces...>::getFieldNames(IList** names)
{
    if (!names)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *names = this->fields.getKeyList().addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

template <class StructInterface, class ... Interfaces>
ErrCode GenericStructImpl<StructInterface, Interfaces...>::getFieldValues(IList** values)
{
    if (!values)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *values = this->fields.getValueList().addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

template <class StructInterface, class... Interfaces>
ErrCode GenericStructImpl <StructInterface, Interfaces...>::get(IString* name, IBaseObject** field)
{
    if (!name)
    {
        *field = nullptr;
        return OPENDAQ_SUCCESS;
    }

    if (!field)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    const auto nameObj = StringPtr::Borrow(name);
    if (this->fields.hasKey(name))
        *field = this->fields.get(name).addRefAndReturn();
    else
        *field = nullptr;

    return OPENDAQ_SUCCESS;
}

template <class StructInterface, class ... Interfaces>
ErrCode GenericStructImpl<StructInterface, Interfaces...>::getAsDictionary(IDict** dictionary)
{
    if (!dictionary)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *dictionary = this->fields.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

template <class StructInterface, class... Interfaces>
ErrCode GenericStructImpl<StructInterface, Interfaces...>::hasField(IString* name, Bool* hasField)
{
    if (!hasField)
        return OPENDAQ_ERR_ARGUMENT_NULL;
    
    *hasField = false;
    if (!name)
        return OPENDAQ_SUCCESS;

    const auto nameObj = StringPtr::Borrow(name);
    if (this->fields.hasKey(name))
        *hasField = true;

    return OPENDAQ_SUCCESS;
}

template <class StructInterface, class... Interfaces>
ErrCode GenericStructImpl<StructInterface, Interfaces...>::getCoreType(CoreType* coreType)
{
    if (!coreType)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *coreType = ctStruct;
    return OPENDAQ_SUCCESS;
}


template <class StructInterface, class ... Interfaces>
ErrCode GenericStructImpl<StructInterface, Interfaces...>::serialize(ISerializer* serializer)
{
    serializer->startTaggedObject(this);

    const StringPtr typeName = this->structType.getName();
    serializer->key("typeName");
    serializer->writeString(typeName.getCharPtr(), typeName.getLength());

    serializer->key("fields");
    ISerializable* serializableFields;
    ErrCode errCode = this->fields->borrowInterface(ISerializable::Id, reinterpret_cast<void**>(&serializableFields));

    if (errCode == OPENDAQ_ERR_NOINTERFACE)
        return OPENDAQ_ERR_NOT_SERIALIZABLE;

    if (OPENDAQ_FAILED(errCode))
        return errCode;

    errCode = serializableFields->serialize(serializer);

    if (OPENDAQ_FAILED(errCode))
        return errCode;

    serializer->endObject();

    return OPENDAQ_SUCCESS;
}

template <class StructInterface, class ... Interfaces>
ErrCode GenericStructImpl<StructInterface, Interfaces...>::getSerializeId(ConstCharPtr* id) const
{
    *id = SerializeId();

    return OPENDAQ_SUCCESS;
}

template <class StructInterface, class ... Interfaces>
ErrCode GenericStructImpl<StructInterface, Interfaces...>::toString(CharPtr* str)
{
    if (str == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    std::ostringstream strStream;
    bool first = true;
    for (const auto& field: fields)
    {
        if (!first)
            strStream << "; ";

        first = false;
        strStream << field.first.toStdString() << "=" << (field.second.assigned() ? static_cast<std::string>(field.second) : "null");
    }

    const auto len = strStream.str().size() + 1;
    *str = static_cast<CharPtr>(daqAllocateMemory(len));
    if (*str == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

#if defined(__STDC_SECURE_LIB__) || defined(__STDC_LIB_EXT1__)
    strcpy_s(*str, len, strStream.str().c_str());
#else
    strncpy(*str, strStream.str().c_str(), len);
#endif
    return OPENDAQ_SUCCESS;
}

template <class StructInterface, class ... Interfaces>
ConstCharPtr GenericStructImpl<StructInterface, Interfaces...>::SerializeId()
{
    return "Struct";
}

template <class StructInterface, class... Interfaces>
ErrCode GenericStructImpl<StructInterface, Interfaces...>::Deserialize(ISerializedObject* ser, IBaseObject* context, IBaseObject** obj)
{
    TypeManagerPtr typeManager;
    if (context == nullptr || OPENDAQ_FAILED(context->queryInterface(ITypeManager::Id, reinterpret_cast<void**>(&typeManager))))
        return OPENDAQ_ERR_NO_TYPE_MANAGER;

    StringPtr typeName;
    ErrCode errCode = ser->readString("typeName"_daq, &typeName);
    if (OPENDAQ_FAILED(errCode))
        return errCode;

    BaseObjectPtr fields;
    errCode = ser->readObject("fields"_daq, context, &fields);
    if (OPENDAQ_FAILED(errCode))
        return errCode;

    try
    {
        StructPtr structPtr;
        createStruct(&structPtr, typeName, fields.asPtr<IDict>(), typeManager);
        *obj = structPtr.detach();
    }
    catch(const DaqException& e)
    {
        return e.getErrCode();
    }
    catch(...)
    {
        return OPENDAQ_ERR_GENERALERROR;
    }

    return OPENDAQ_SUCCESS;
}

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(StructImpl)

END_NAMESPACE_OPENDAQ
