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
#include <coretypes/struct_builder.h>
#include <coretypes/objectptr.h>
#include <coretypes/dictobject_factory.h>
#include <coretypes/baseobject_factory.h>
#include <coretypes/struct_type_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

class StructBuilderPtr;

template <>
struct InterfaceToSmartPtr<IStructBuilder>
{
    using SmartPtr = StructBuilderPtr;
};


/*!
 * @ingroup types_structs
 * @defgroup types_structs_struct Struct Builder
 * @{
 */

/*!
 * @brief Builder component of Struct objects. Contains setter methods to configure the Struct parameters, and a
 * `build` method that builds the Struct object.
 */
class StructBuilderPtr : public ObjectPtr<IStructBuilder>
{
public:
    using ObjectPtr<IStructBuilder>::ObjectPtr;
    

    StructBuilderPtr()
        : daq::ObjectPtr<IStructBuilder>()

    {
    }

    StructBuilderPtr(daq::ObjectPtr<IStructBuilder>&& ptr)
        : daq::ObjectPtr<IStructBuilder>(std::move(ptr))

    {
    }

    StructBuilderPtr(const daq::ObjectPtr<IStructBuilder>& ptr)
        : daq::ObjectPtr<IStructBuilder>(ptr)

    {
    }

    StructBuilderPtr(const StructBuilderPtr& other)
        : daq::ObjectPtr<IStructBuilder>(other)

    {
    }

    StructBuilderPtr(StructBuilderPtr&& other) noexcept
        : daq::ObjectPtr<IStructBuilder>(std::move(other))

    {
    }
    
    StructBuilderPtr& operator=(const StructBuilderPtr& other)
    {
        if (this == &other)
            return *this;

        daq::ObjectPtr<IStructBuilder>::operator =(other);


        return *this;
    }

    StructBuilderPtr& operator=(StructBuilderPtr&& other) noexcept
    {
        if (this == std::addressof(other))
        {
            return *this;
        }


        daq::ObjectPtr<IStructBuilder>::operator =(std::move(other));

        return *this;
    }
    /*!
     * @brief Builds and returns a Struct object using the currently set values of the Builder.
     * @returns The built Struct.
     */
    StructPtr build() const
    {
        if (this->object == nullptr)
            throw InvalidParameterException();

        StructPtr struct_;
        const auto errCode = this->object->build(&struct_);
        checkErrorInfo(errCode);

        return struct_;
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
     * @brief Sets a list of all Struct field values.
     * @param values The list of field values.
     *
     * The list of names will be of equal length to the list of values. Additionally, the name of a field at any given
     * index corresponds to the value stored in the list of values.
     */
    StructBuilderPtr setFieldValues(const ListPtr<IBaseObject> values) const
    {
        if (this->object == nullptr)
            throw InvalidParameterException();
        
        const auto errCode = this->object->setFieldValues(values);
        checkErrorInfo(errCode);
        return this->addRefAndReturn();
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
     * @param field The value of the field.
     */
    StructBuilderPtr set(const StringPtr& name, const BaseObjectPtr& field) const
    {
        if (this->object == nullptr)
            throw InvalidParameterException();
        
        const auto errCode = this->object->set(name, field);
        checkErrorInfo(errCode);
        return this->addRefAndReturn();
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
