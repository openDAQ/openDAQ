/*
 * Copyright 2022-2024 openDAQ d. o. o.
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
#include <coretypes/type_ptr.h>
#include <coretypes/struct_type.h>
#include <coretypes/listobject_factory.h>

BEGIN_NAMESPACE_OPENDAQ

class StructTypePtr;

template <>
struct InterfaceToSmartPtr<IStructType>
{
    using SmartPtr = StructTypePtr;
};

/*!
 * @ingroup types_structs
 * @defgroup types_structs_struct StructType
 * @{
 */

/*!
 * @brief Struct types define the fields (names and value types, as well as optional default values) of Structs with a name
 * matching that of the Struct type.
 *
 * A Struct type contains a String-type name, a list of field names (list of Strings), a list of field types (list of Type objects),
 * and an optional list of Default values (list of Base objects). The Struct types should be added to the Type manager to be used
 * for Struct validation on creation. Alternatively, if a Struct is created with no matching Struct type in the manager, a default
 * Struct type is created based on the field names and types of the Created struct. Said Struct type is then added to the manager.
 *
 * The field types are a list of Type objects. These determine the types of values that should be used to fill in the corresponding
 * field value. The Type objects at the moment available in openDAQ are Simple types and Struct types. When adding any field other
 * than a Struct type, the Simple type corresponding to the Core type of the value should be created. When adding Struct fields, a
 * Struct type should be added to the field types.
 *
 * A Struct can only have fields of Core type: `ctBool`, `ctInt`, `ctFloat`, `ctString`, `ctList`, `ctDict`, `ctRatio`, `ctComplexNumber`,
 * `ctStruct`, `ctEnumeration`, or `ctUndefined`. Additionally, all Container types (`ctList`, `ctDict`) should only have values of the aforementioned
 * types.
 */
class StructTypePtr : public GenericTypePtr<IStructType>
{
public:
    using GenericTypePtr<IStructType>::GenericTypePtr;


    StructTypePtr()
        : GenericTypePtr<IStructType>()
    {
    }

    StructTypePtr(daq::ObjectPtr<IStructType>&& ptr)
        : GenericTypePtr<IStructType>(std::move(ptr))
    {
    }

    StructTypePtr(const daq::ObjectPtr<IStructType>& ptr)
        : GenericTypePtr<IStructType>(ptr)
    {
    }

    StructTypePtr(const StructTypePtr& other)
        : GenericTypePtr<IStructType>(other)
    {
    }

    StructTypePtr(StructTypePtr&& other) noexcept
        : GenericTypePtr<IStructType>(std::move(other))
    {
    }
    
    StructTypePtr& operator=(const StructTypePtr& other)
    {
        if (this == &other)
            return *this;

        GenericTypePtr::operator =(other);
        return *this;
    }

    StructTypePtr& operator=(StructTypePtr&& other) noexcept
    {
        if (this == std::addressof(other))
            return *this;

        GenericTypePtr::operator =(std::move(other));
        return *this;
    }

    /*!
     * @brief Gets the list of field names.
     * @returns The list of field names (String objects)
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
     * @brief Gets the list of field default values.
     * @returns The list of field default values (Base objects)
     */
    ListPtr<IBaseObject> getFieldDefaultValues() const
    {
        if (this->object == nullptr)
            throw InvalidParameterException();

        ListPtr<IBaseObject> defaultValues;
        const auto errCode = this->object->getFieldDefaultValues(&defaultValues);
        checkErrorInfo(errCode);

        return defaultValues;
    }

    /*!
     * @brief Gets the list of field types.
     * @returns The list of field types (Type objects)
     */
    ListPtr<IType> getFieldTypes() const
    {
        if (this->object == nullptr)
            throw InvalidParameterException();

        ListPtr<CoreType> types;
        const auto errCode = this->object->getFieldTypes(&types);
        checkErrorInfo(errCode);

        return types;
    }
};

/*!@}*/

END_NAMESPACE_OPENDAQ
