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
#include <opendaq/context_ptr.h>
#include <opendaq/context_internal_ptr.h>
#include <coretypes/intfs.h>
#include <gmock/gmock.h>
#include <coretypes/gmock/mock_ptr.h>
#include <opendaq/gmock/scheduler.h>

struct MockContext : daq::ImplementationOf<daq::IContext, daq::IContextInternal>
{
    typedef MockPtr<
        daq::IContext,
        daq::ContextPtr,
        MockContext
    > Strict;

    MOCK_METHOD(daq::ErrCode, getScheduler, (daq::IScheduler** scheduler), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, getLogger, (daq::ILogger** logger), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, getModuleManager, (daq::IBaseObject** manager), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, getTypeManager, (daq::ITypeManager** manager), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, getAuthenticationProvider, (daq::IAuthenticationProvider** authenticationProvider), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, getOnCoreEvent, (daq::IEvent** event), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, moveModuleManager, (daq::IModuleManager** manager), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, getOptions, (daq::IDict** options), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, getModuleOptions, (daq::IString* moduleId, daq::IDict** options), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, getAvailableDiscoveryServices, (daq::IDict** services), (override MOCK_CALL));

    daq::SchedulerPtr scheduler;
    daq::LoggerPtr logger;
    daq::TypeManagerPtr typeManager;
    daq::AuthenticationProviderPtr authenticationProvider;
    daq::BaseObjectPtr moduleManager;
    daq::EventEmitter<daq::ComponentPtr, daq::CoreEventArgsPtr> coreEvent;

    MockContext()
    {
        using namespace testing;

        EXPECT_CALL(*this, getScheduler)
            .Times(AnyNumber())
            .WillRepeatedly(DoAll(
                Invoke([&](daq::IScheduler** schedulerOut) { *schedulerOut = scheduler.addRefAndReturn(); }),
                Return(OPENDAQ_SUCCESS)));

        EXPECT_CALL(*this, getLogger)
            .Times(AnyNumber())
            .WillRepeatedly(DoAll(Invoke([&](daq::ILogger** loggerOut) { *loggerOut = logger.addRefAndReturn(); }),
                                  Return(OPENDAQ_SUCCESS)));

        EXPECT_CALL(*this, getTypeManager)
            .Times(AnyNumber())
            .WillRepeatedly(DoAll(Invoke([&](daq::ITypeManager** typeManagerOut) { *typeManagerOut = typeManager.addRefAndReturn(); }),
                                  Return(OPENDAQ_SUCCESS)));

        EXPECT_CALL(*this, getAuthenticationProvider)
            .Times(AnyNumber())
            .WillRepeatedly(DoAll(Invoke([&](daq::IAuthenticationProvider** authenticationProviderOut)
                                         { *authenticationProviderOut = authenticationProvider.addRefAndReturn(); }),
                                  Return(OPENDAQ_SUCCESS)));

        EXPECT_CALL(*this, getModuleManager)
            .Times(AnyNumber())
            .WillRepeatedly(DoAll(Invoke([&](daq::IBaseObject** moduleManagerOut) { *moduleManagerOut = moduleManager.addRefAndReturn(); }),
                                  Return(OPENDAQ_SUCCESS)));

        EXPECT_CALL(*this, getOnCoreEvent)
            .Times(AnyNumber())
            .WillRepeatedly(DoAll(Invoke([&](daq::IEvent** event) { *event = coreEvent.addRefAndReturn(); }),
                                  Return(OPENDAQ_SUCCESS)));
    }
};
