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
#include <coretypes/struct.h>
#include <coretypes/objectptr.h>
#include <coretypes/dictobject_factory.h>
#include <coretypes/baseobject_factory.h>
#include <coretypes/struct_type_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

template <typename InterfaceType = daq::IStruct>
class GenericStructPtr;


using StructPtr = GenericStructPtr<>;

template <>
struct InterfaceToSmartPtr<IStruct>
{
    using SmartPtr = daq::GenericStructPtr<daq::IStruct>;
};

/*!
 * @ingroup types_structs
 * @defgroup types_structs_struct Struct
 * @{
 */

/*!
 * @brief Structs are immutable objects that contain a set of key-value pairs. The key, as well as the types of each
 * associated value for each struct are defined in advance within a Struct type that has the same name as the Struct.
 *
 * The Struct types are stored within a Type manager. In any given instance of openDAQ, a single Type manager should
 * exist that is part of its Context.
 *
 * When creating a Struct, the Type manager is used to validate the given dictionary of keys and values against the
 * Struct type stored within the manager. If no type with the given Struct name is currently stored, a default type
 * is created using the Struct field names and values as its parameters. When creating a Struct, fields that are part
 * of the Struct type can be omitted. If so, they will be replaced by either `null` or, if provided by the Struct type,
 * the default value of the field.
 *
 * In the case that a field name is present that is not part of the struct type, or if the value type of the field does
 * not match, construction of the Struct will fail.
 *
 * NOTE: Field values of fields with the Core type `ctUndefined` can hold any value, regardless of its type.
 *
 * Structs are an openDAQ core type (ctStruct). Several objects in openDAQ such as an Unit, or DataDescriptor are Structs,
 * allowing for access to their fields through Struct methods. Such objects are, by definition, immutable - their fields
 * cannot be modified. In order to change the value of a Struct-type object, a new Struct must be created.
 *
 * A Struct can only have fields of Core type: `ctBool`, `ctInt`, `ctFloat`, `ctString`, `ctList`, `ctDict`, `ctRatio`, `ctComplexNumber`,
 * `ctStruct`, or `ctUndefined`. Additionally, all Container types (`ctList`, `ctDict`) should only have values of the aforementioned
 * types.
 */
template <typename InterfaceType>
class GenericStructPtr : public ObjectPtr<InterfaceType>
{
public:
    using ObjectPtr<InterfaceType>::ObjectPtr;
    
    GenericStructPtr()
        : daq::ObjectPtr<InterfaceType>()
    {
    }

    GenericStructPtr(ObjectPtr<InterfaceType>&& ptr)
        : ObjectPtr<InterfaceType>(std::move(ptr))
    {
    }

    GenericStructPtr(const ObjectPtr<InterfaceType>& ptr)
        : ObjectPtr<InterfaceType>(ptr)
    {
    }


    GenericStructPtr(const StructPtr& ptr)
        : daq::ObjectPtr<InterfaceType>(ptr)

    {
    }

    GenericStructPtr(StructPtr&& ptr) noexcept
        : daq::ObjectPtr<InterfaceType>(std::move(ptr))

    {
    }
        
    GenericStructPtr& operator=(const StructPtr& other)
    {
        if (this == &other)
            return *this;

        daq::ObjectPtr<InterfaceType>::operator =(other);
        return *this;
    }

    GenericStructPtr& operator=(StructPtr&& other) noexcept
    {
        if (this == std::addressof(other))
        {
            return *this;
        }

        daq::ObjectPtr<InterfaceType>::operator =(std::move(other));
        return *this;
    }

    /*!
     * @brief Gets the Struct's type.
     * @returns The Struct type
     */
    StructTypePtr getStructType() const
    {
        if (this->object == nullptr)
            throw InvalidParameterException();

        StructTypePtr type;
        const auto errCode = this->object->getStructType(&type);
        checkErrorInfo(errCode);

        return type;
    }

    /*!
     * @brief Gets a list of all Struct field names.
     * @returns The list of field names.
     *
     * The list of names will be of equal length to the list of values. Additionally, the name of a field at any given
     * index corresponds to the value stored in the list of values.
     */
    ListPtr<IString> getFieldNames() const
    {
        if (this->object == nullptr)
            throw InvalidParameterException();

        ListPtr<IString> names;
        const auto errCode = this->object->getFieldNames(&names);
        checkErrorInfo(errCode);

        return names;
    }

    /*!
     * @brief Gets a list of all Struct field values.
     * @returns The list of field values.
     *
     * The list of names will be of equal length to the list of values. Additionally, the name of a field at any given
     * index corresponds to the value stored in the list of values.
     */
    ListPtr<IBaseObject> getFieldValues() const
    {
        if (this->object == nullptr)
            throw InvalidParameterException();

        ListPtr<IBaseObject> values;
        const auto errCode = this->object->getFieldValues(&values);
        checkErrorInfo(errCode);

        return values;
    }

    /*!
     * @brief Gets the value of a field with the given name.
     * @param name The name of the queried field.
     * @returns The value of the field.
     */
    BaseObjectPtr get(const StringPtr& name) const
    {
        if (this->object == nullptr)
            throw InvalidParameterException();

        BaseObjectPtr field;
        const auto errCode = this->object->get(name, &field);
        checkErrorInfo(errCode);

        return field;
    }

    /*!
     * @brief Gets the field names and values of the Struct as a Dictionary.
     * @returns The Dictionary object with field names as keys, and field values as its values.
     */   
    DictPtr<IString, IBaseObject> getAsDictionary() const
    {
        if (this->object == nullptr)
            throw InvalidParameterException();

        DictPtr<IString, IBaseObject> dict;
        const auto errCode = this->object->getAsDictionary(&dict);
        checkErrorInfo(errCode);

        return dict;
    }

    /*!
     * @brief Checks whether a field with the given name exists in the Struct
     * @param name The name of the checked field.
     * @returns True if the a field with `name` exists in the Struct; false otherwise.
     */
    Bool hasField(const StringPtr& name) const
    {
        if (this->object == nullptr)
            throw InvalidParameterException();

        Bool hasField;
        const auto errCode = this->object->hasField(name, &hasField);
        checkErrorInfo(errCode);

        return hasField;
    }

    BaseObjectPtr operator[](const StringPtr& name) const
    {
        return get(name);
    }
};

/*!@}*/

END_NAMESPACE_OPENDAQ
