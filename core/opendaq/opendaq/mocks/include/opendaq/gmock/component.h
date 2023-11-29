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
#include <opendaq/component_ptr.h>
#include <opendaq/search_filter_ptr.h>
#include <coretypes/intfs.h>
#include <gmock/gmock.h>
#include <coretypes/gmock/mock_ptr.h>
#include <coreobjects/property_object_impl.h>

BEGIN_NAMESPACE_OPENDAQ

template <class Class, class TInterface>
struct MockGenericComponent : GenericPropertyObjectImpl<TInterface>
{
    typedef MockPtr<
        TInterface,
        typename daq::InterfaceToSmartPtr<TInterface>::SmartPtr,
        Class
    > Strict;

    MOCK_METHOD(daq::ErrCode, getLocalId, (daq::IString** localId), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, getGlobalId, (daq::IString** globalId), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, getActive, (daq::Bool* active), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, setActive, (daq::Bool active), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, getContext, (daq::IContext** root), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, getParent, (daq::IComponent** parent), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, getName, (daq::IString** name), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, setName, (daq::IString* name), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, getDescription, (daq::IString** description), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, setDescription, (daq::IString* description), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, getTags, (daq::ITagsConfig** tags), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, getVisible, (Bool* visible), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, setVisible, (Bool visible), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, getLockedAttributes, (IList** attributes), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, getOnComponentCoreEvent, (IEvent** event), (override MOCK_CALL));

    MockGenericComponent()
        : GenericPropertyObjectImpl<TInterface>()
    {
    }
};

struct MockSearchFilter : daq::ImplementationOf<daq::ISearchFilter>
{
    typedef MockPtr<
        daq::ISearchFilter,
        daq::SearchFilterPtr,
        testing::StrictMock<MockSearchFilter>
    > Strict;

    MOCK_METHOD(daq::ErrCode, acceptsComponent, (daq::IComponent* component, daq::Bool* accepts), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, visitChildren, (daq::IComponent* component, daq::Bool* visit), (override MOCK_CALL));
};

struct MockComponent: MockGenericComponent<MockComponent, IComponent>
{
};

template <class Class, class TInterface>
struct MockGenericSignalContainer : MockGenericComponent<Class, TInterface>
{
    typedef MockPtr<TInterface, typename daq::InterfaceToSmartPtr<TInterface>::SmartPtr, Class> Strict;

    MOCK_METHOD(daq::ErrCode, getItem, (daq::IString* localId, daq::IComponent** component), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, getItems, (daq::IList** components, daq::ISearchFilter* searchFilter), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, isEmpty, (daq::Bool* empty), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, hasItem, (daq::IString* localId, daq::Bool* value), (override MOCK_CALL));

    MockGenericSignalContainer()
        : MockGenericComponent<Class, TInterface>()
    {
    }
};

struct GetBool
{
    GetBool(Bool val)
        : val(val)
    {
    }

    ErrCode operator()(Bool* value) const
    {
        *value = val;
        return OPENDAQ_SUCCESS;
    }

    Bool val;
};

template <typename TSmartPtr>
struct Get
{
    Get(const TSmartPtr& ptr)
        : ptr(ptr)
    {
    }
    Get(TSmartPtr&& ptr)
        : ptr(std::move(ptr))
    {
    }

    ErrCode operator()(typename TSmartPtr::DeclaredInterface** value)
    {
        *value = ptr.addRefAndReturn();
        return OPENDAQ_SUCCESS;
    }

    TSmartPtr ptr;
};

template <typename TSmartPtr, typename TOptionalArgPtr>
struct GetOptionalExtraArg
{
    GetOptionalExtraArg(const TSmartPtr& ptr)
        : ptr(ptr)
    {
    }
    GetOptionalExtraArg(TSmartPtr&& ptr)
        : ptr(std::move(ptr))
    {
    }

    ErrCode operator()(typename TSmartPtr::DeclaredInterface** value, typename TOptionalArgPtr::DeclaredInterface*)
    {
        *value = ptr.addRefAndReturn();
        return OPENDAQ_SUCCESS;
    }

    TSmartPtr ptr;
};

END_NAMESPACE_OPENDAQ
