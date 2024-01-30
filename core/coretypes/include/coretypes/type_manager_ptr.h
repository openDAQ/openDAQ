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
#include "coretypes/type_manager.h"
#include <coretypes/type_ptr.h>
#include <coretypes/string_ptr.h>
#include <coretypes/listobject_factory.h>


BEGIN_NAMESPACE_OPENDAQ


class TypeManagerPtr;

template <>
struct InterfaceToSmartPtr<ITypeManager>
{
    using SmartPtr = TypeManagerPtr;
};

/*!
 * @ingroup types_types
 * @defgroup types_types_type_manager Type manager
 * @{
 */

/*!
 * @brief Container for Type objects. The Type manager is used when creating certain types of objects
 * (eg. Structs and Property object classes). The Types stored within the manager contain pre-defined fields,
 * as well as constraints specifying how objects should look.
 *
 * The currently available types in openDAQ that should be added to the Type manager are the Struct type
 * and the Property object class. The former is used to validate Structs when they are created, while the latter
 * contains pre-defined properties that are added to Property objects on construction.
 *
 * When adding a Property object class to the manager, they can form inheritance chains with one-another.
 * Thus, a parent of a given class must be added to the manager before any of its children. Likewise, a class
 * cannot be removed before its children are removed.
 */
class TypeManagerPtr : public ObjectPtr<ITypeManager>
{
public:
    using ObjectPtr::ObjectPtr;



    TypeManagerPtr()
    {
    }

    TypeManagerPtr(ObjectPtr&& ptr)
        : ObjectPtr(std::move(ptr))
    {
    }

    TypeManagerPtr(const ObjectPtr& ptr)
        : ObjectPtr(ptr)
    {
    }

    TypeManagerPtr(const TypeManagerPtr& other)
        : ObjectPtr(other)
    {
    }

    TypeManagerPtr(TypeManagerPtr&& other) noexcept
        : ObjectPtr(std::move(other))
    {
    }
    
    TypeManagerPtr& operator=(const TypeManagerPtr& other)
    {
        if (this == &other)
            return *this;

        ObjectPtr::operator =(other);
        return *this;
    }

    TypeManagerPtr& operator=(TypeManagerPtr&& other) noexcept
    {
        if (this == std::addressof(other))
            return *this;

        ObjectPtr::operator =(std::move(other));
        return *this;
    }

    /*!
     * @brief Adds a type to the manager.
     * @param type The Type to be added.
     * @retval OPENDAQ_ERR_ALREADYEXISTS if a type with the same name is already added.
     * @retval OPENDAQ_ERR_INVALIDPARAMETER if either the type name is an empty string.
     *
     * The type name must be unique and. If a Property object class specifies a parent class,
     * then the parent class must be added before it.
     */
    void addType(const TypePtr& type) const
    {
        if (this->object == nullptr)
            throw InvalidParameterException();

        const auto errCode = this->object->addType(type);
        checkErrorInfo(errCode);
    }

    /*!
     * @brief Removes the type from the manager.
     * @param typeName The type's name.
     * @retval OPENDAQ_ERR_NOTFOUND if the class is not registered.
     *
     * The removed class must not be a parent of another added class. If it is, those classes must be removed
     * before it.
     */
    void removeType(const StringPtr& name) const
    {
        if (this->object == nullptr)
            throw InvalidParameterException();

        const auto errCode = this->object->removeType(name);
        checkErrorInfo(errCode);
    }

    /*!
     * @brief Gets an added Type by name.
     * @param typeName The Type's name.
     * @returns The Type with name equal to `name`.
     * @retval OPENDAQ_ERR_NOTFOUND if a Type with the specified name is not added.
     */
    TypePtr getType(const StringPtr& name) const
    {
        if (this->object == nullptr)
            throw InvalidParameterException();

        TypePtr type;
        const auto errCode = this->object->getType(name, &type);
        checkErrorInfo(errCode);

        return type;
    }

    /*!
     * @brief Gets a list of all added Types.
     * @returns The list of all added Types.
     */
    ListPtr<IString> getTypes() const
    {
        if (this->object == nullptr)
            throw InvalidParameterException();

        ListPtr<IString> types;
        const auto errCode = this->object->getTypes(&types);
        checkErrorInfo(errCode);

        return types;
    }

    /*!
     * @brief Checks if a type with the specified name is already added.
     * @param typeName The name of the checked type.
     * @returns True if the type is aready added to the manager; False otherwise.
     */
    Bool hasType(const StringPtr& typeName) const
    {
        if (this->object == nullptr)
            throw InvalidParameterException();

        Bool hasType;
        const auto errCode = this->object->hasType(typeName, &hasType);
        checkErrorInfo(errCode);

        return hasType;
    }
};

END_NAMESPACE_OPENDAQ
