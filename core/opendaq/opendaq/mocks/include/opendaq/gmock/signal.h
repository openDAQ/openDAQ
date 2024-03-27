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
#include <vector>
#include <opendaq/signal.h>
#include <opendaq/signal_events.h>
#include <opendaq/signal_private.h>
#include <opendaq/removable.h>
#include <coreobjects/property_object_impl.h>
#include <gmock/gmock.h>
#include <coretypes/gmock/mock_ptr.h>

struct MockSignal : daq::GenericPropertyObjectImpl<daq::ISignal, daq::ISignalEvents, daq::ISignalPrivate, daq::IRemovable>
{
    typedef MockPtr<
        daq::ISignal,
        daq::SignalPtr,
        MockSignal
    > Strict;

    MOCK_METHOD(daq::ErrCode, getLocalId, (daq::IString** localId), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, getGlobalId, (daq::IString** globalId), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, getActive, (daq::Bool* active), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, setActive, (daq::Bool active), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, getContext, (daq::IContext** context), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, getParent, (daq::IComponent** parent), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, getName, (daq::IString * *name), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, getTags, (daq::ITags * *tags), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, getVisible, (daq::Bool* visible), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, setVisible, (daq::Bool visible), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, getLockedAttributes, (daq::IList** attributes), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, getOnComponentCoreEvent, (daq::IEvent** event), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, getStatusContainer, (daq::IComponentStatusContainer** statusContainer), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, findComponent, (daq::IString* id, daq::IComponent** outComponent), (override MOCK_CALL));

    MOCK_METHOD(daq::ErrCode, getPublic, (daq::Bool* public_), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, setPublic, (daq::Bool public_), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, getDescriptor, (daq::IDataDescriptor** descriptor), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, getDomainSignal, (daq::ISignal** signal), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, getRelatedSignals, (daq::IList** signals), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, getConnections, (daq::IList** connections), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, setName, (daq::IString* name), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, setDescription, (daq::IString* name), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, getDescription, (daq::IString** name), (override MOCK_CALL));

    MOCK_METHOD(daq::ErrCode, listenerConnected, (daq::IConnection* connection), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, listenerDisconnected, (daq::IConnection* connection), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, domainSignalReferenceSet, (daq::ISignal* signal), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, domainSignalReferenceRemoved, (daq::ISignal* signal), (override MOCK_CALL));

    MOCK_METHOD(daq::ErrCode, clearDomainSignalWithoutNotification, (), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, enableKeepLastValue, (daq::Bool enabled), (override MOCK_CALL));

    MOCK_METHOD(daq::ErrCode, remove, (), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, isRemoved, (daq::Bool* removed), (override MOCK_CALL));

    MOCK_METHOD(daq::ErrCode, setStreamed, (daq::Bool streamed), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, getStreamed, (daq::Bool* streamed), (override MOCK_CALL));

    MOCK_METHOD(daq::ErrCode, getLastValue, (IBaseObject** value), (override MOCK_CALL));

    std::vector<daq::ConnectionPtr> connections;

    MockSignal()
        : daq::GenericPropertyObjectImpl<daq::ISignal, daq::ISignalEvents, daq::ISignalPrivate, IRemovable>()
    {
        using namespace testing;

        EXPECT_CALL(*this, getConnections)
            .Times(AnyNumber())
            .WillRepeatedly(DoAll(
                Invoke([&](daq::IList** connections_out) {
                    daq::ListPtr<daq::IConnection> ptr { connections };
                    *connections_out = ptr.detach();
                }),
                Return(OPENDAQ_SUCCESS)
            ));

        ON_CALL(*this, listenerConnected)
            .WillByDefault(DoAll(
                Invoke([&](daq::IConnection* connection) {
                    connections.push_back(connection);
                }),
                Return(OPENDAQ_SUCCESS)
            ));

        ON_CALL(*this, listenerDisconnected)
            .WillByDefault(DoAll(
                Invoke([&](daq::IConnection* connection) {
                    const auto ptr = daq::ObjectPtr<daq::IConnection>::Borrow(connection);
                    auto it = std::find(connections.begin(), connections.end(), ptr);
                    if (it == connections.end())
                        return OPENDAQ_ERR_NOTFOUND;
                    connections.erase(it);
                    return OPENDAQ_SUCCESS;
                })
            ));
    }
};
