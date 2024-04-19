/*
 * Copyright 2022-2024 Blueberry d.o.o.
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
#include <opendaq/packet.h>
#include <coretypes/intfs.h>
#include <coretypes/validation.h>
#include <opendaq/packet_ptr.h>
#include <opendaq/packet_destruct_callback_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

template <typename TInterface = IPacket, typename ... TInterfaces>
class PacketImpl : public ImplementationOf<TInterface, TInterfaces ...>
{
public:
    PacketImpl();
    ~PacketImpl();

    ErrCode INTERFACE_FUNC getType(PacketType* type) override;
    ErrCode INTERFACE_FUNC subscribeForDestructNotification(IPacketDestructCallback* packetDestructCallback) override;
    ErrCode INTERFACE_FUNC getRefCount(SizeT* refCount) override;

    ErrCode INTERFACE_FUNC equals(IBaseObject* other, Bool* equals) const override;

protected:
    PacketType type;
    std::mutex sync;
    std::vector<PacketDestructCallbackPtr> packetDestructCallbackList;
};

template <typename TInterface, typename ... TInterfaces>
PacketImpl<TInterface, TInterfaces ...>::PacketImpl()
    : type(PacketType::None)
{
}

template <typename TInterface, typename ... TInterfaces>
PacketImpl<TInterface, TInterfaces...>::~PacketImpl()
{
    for (const auto& packetDestructCallback : packetDestructCallbackList)
        packetDestructCallback->onPacketDestroyed();
}

template <typename TInterface, typename... TInterfaces>
ErrCode PacketImpl<TInterface, TInterfaces...>::getType(PacketType* type)
{
    OPENDAQ_PARAM_NOT_NULL(type);

    *type = this->type;
    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename ... TInterfaces>
ErrCode PacketImpl<TInterface, TInterfaces...>::subscribeForDestructNotification(IPacketDestructCallback* packetDestructCallback)
{
    OPENDAQ_PARAM_NOT_NULL(packetDestructCallback);

    std::scoped_lock lock(sync);
    packetDestructCallbackList.push_back(packetDestructCallback);

    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename ... TInterfaces>
ErrCode PacketImpl<TInterface, TInterfaces...>::getRefCount(SizeT* refCount)
{
    OPENDAQ_PARAM_NOT_NULL(refCount);

    *refCount = this->getReferenceCount();
    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename... TInterfaces>
inline ErrCode INTERFACE_FUNC PacketImpl<TInterface, TInterfaces...>::equals(IBaseObject* other, Bool* equals) const
{
    if (equals == nullptr)
        return this->makeErrorInfo(OPENDAQ_ERR_ARGUMENT_NULL, "Equals out-parameter must not be null");

    *equals = false;
    if (other == nullptr)
        return OPENDAQ_SUCCESS;

    const PacketPtr packetOther = BaseObjectPtr::Borrow(other).asPtrOrNull<IPacket>();
    if (packetOther == nullptr)
        return OPENDAQ_SUCCESS;

    if (this->type != packetOther.getType())
        return OPENDAQ_SUCCESS;

    *equals = true;
    return OPENDAQ_SUCCESS;
}

END_NAMESPACE_OPENDAQ
