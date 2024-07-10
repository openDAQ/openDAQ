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
#include <coretypes/intfs.h>
#include <coretypes/type_ptr.h>
#include <coretypes/baseobject_factory.h>
#include <utility>

BEGIN_NAMESPACE_OPENDAQ

template <class TypeInterface>
class GenericTypeImpl : public ImplementationOf<TypeInterface, ISerializable, ICoreType>
{
public:
    explicit GenericTypeImpl (StringPtr typeName, CoreType coreType);

    ErrCode INTERFACE_FUNC getName(IString** typeName) override;
    ErrCode INTERFACE_FUNC equals(IBaseObject* other, Bool* equal) const override;

    virtual ErrCode INTERFACE_FUNC getCoreType(CoreType* coreType) override;

protected:
    StringPtr typeName;
    CoreType coreType;
};

using TypeImpl = GenericTypeImpl<IType>;


template <class TypeInterface>
GenericTypeImpl<TypeInterface>::GenericTypeImpl (StringPtr typeName, CoreType coreType)
    : typeName(std::move(typeName))
    , coreType(coreType)
{
}

template <class TypeInterface>
ErrCode GenericTypeImpl<TypeInterface>::getName(IString** typeName)
{
    if (!typeName)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *typeName = this->typeName.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

template <class TypeInterface>
ErrCode GenericTypeImpl<TypeInterface>::equals(IBaseObject* other, Bool* equal) const
{
    if (equal == nullptr)
        return this->makeErrorInfo(OPENDAQ_ERR_ARGUMENT_NULL, "Equals out-parameter must not be null");

    *equal = false;
    if (other == nullptr)
        return OPENDAQ_SUCCESS;

    const TypePtr typeOther = BaseObjectPtr::Borrow(other).asPtrOrNull<IType>();
    if (typeOther == nullptr)
        return OPENDAQ_SUCCESS;

    *equal = typeOther.getName() == this->typeName && this->coreType == typeOther.getCoreType();
    return OPENDAQ_SUCCESS;
}

template <class TypeInterface>
ErrCode GenericTypeImpl<TypeInterface>::getCoreType(CoreType* coreType)
{
    if (!coreType)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *coreType = this->coreType;
    return OPENDAQ_SUCCESS;
}

END_NAMESPACE_OPENDAQ
