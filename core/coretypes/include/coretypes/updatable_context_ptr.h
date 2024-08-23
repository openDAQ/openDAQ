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
#include <coretypes/updatable_context.h>
#include <coretypes/objectptr.h>
#include <coretypes/dict_ptr.h>
#include <coretypes/string_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

class UpdatableContextPtr;

template <>
struct InterfaceToSmartPtr<IUpdatableContext>
{
    typedef UpdatableContextPtr SmartPtr;
};

/*!
 * @addtogroup types_updatable
 * @{
 */

class UpdatableContextPtr : public ObjectPtr<IUpdatableContext>
{
public:
    using daq::ObjectPtr<IUpdatableContext>::ObjectPtr;

    UpdatableContextPtr()
    {
    }

    UpdatableContextPtr(daq::ObjectPtr<IUpdatableContext>&& ptr)
        : daq::ObjectPtr<IUpdatableContext>(std::move(ptr))
    {
    }

    UpdatableContextPtr(const daq::ObjectPtr<IUpdatableContext>& ptr)
        : daq::ObjectPtr<IUpdatableContext>(ptr)
    {
    }

    UpdatableContextPtr(const UpdatableContextPtr& other)
        : daq::ObjectPtr<IUpdatableContext>(other)
    {
    }

    UpdatableContextPtr(UpdatableContextPtr&& other) noexcept
        : daq::ObjectPtr<IUpdatableContext>(std::move(other))
    {
    }

    UpdatableContextPtr& operator=(const UpdatableContextPtr& other)
    {
        if (this == &other)
            return *this;

        daq::ObjectPtr<IUpdatableContext>::operator=(other);
        return *this;
    }

    UpdatableContextPtr& operator=(UpdatableContextPtr&& other) noexcept
    {
        if (this == std::addressof(other))
        {
            return *this;
        }

        daq::ObjectPtr<IUpdatableContext>::operator=(std::move(other));
        return *this;
    }

    void setInputPortConnection(const StringPtr& parentId, const StringPtr& portId, const StringPtr& signalId) const
    {
        if (this->object == nullptr)
            throw daq::InvalidParameterException();

        auto errCode = this->object->setInputPortConnection(parentId, portId, signalId);
        daq::checkErrorInfo(errCode);
    }

    DictPtr<IString,IString> getInputPortConnection(const StringPtr& parentId) const
    {
        if (this->object == nullptr)
            throw daq::InvalidParameterException();

        DictPtr<IString,IString> connections = nullptr;
        auto errCode = this->object->getInputPortConnection(parentId, &connections);
        daq::checkErrorInfo(errCode);

       return connections;
    }

    void setNotMutedComponent(const StringPtr& componentId) const
    {
        if (this->object == nullptr)
            throw daq::InvalidParameterException();

        auto errCode = this->object->setNotMutedComponent(componentId);
        daq::checkErrorInfo(errCode);
    }

    Bool getComponentIsMuted(const StringPtr& componentId) const
    {
        if (this->object == nullptr)
            throw daq::InvalidParameterException();

        Bool muted = false;
        auto errCode = this->object->getComponentIsMuted(componentId, &muted);
        daq::checkErrorInfo(errCode);

        return muted;
    }
};

/*!
 * @}
 */

END_NAMESPACE_OPENDAQ
