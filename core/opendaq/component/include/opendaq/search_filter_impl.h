/*
 * Copyright 2022-2025 openDAQ d.o.o.
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
#include <opendaq/search_filter.h>
#include <coretypes/list_factory.h>

BEGIN_NAMESPACE_OPENDAQ

// Filter by Visible

class VisibleSearchFilterImpl final : public ImplementationOf<ISearchFilter>
{
public:
    explicit VisibleSearchFilterImpl();

    ErrCode INTERFACE_FUNC acceptsObject(IBaseObject* obj, Bool* accepts) override;
    ErrCode INTERFACE_FUNC visitChildren(IBaseObject* obj, Bool* visit) override;
};

// Filter by Required tags

class RequiredTagsSearchFilterImpl final : public ImplementationOf<ISearchFilter>
{
public:
    explicit RequiredTagsSearchFilterImpl(const ListPtr<IString>& requiredTags);

    ErrCode INTERFACE_FUNC acceptsObject(IBaseObject* obj, Bool* accepts) override;
    ErrCode INTERFACE_FUNC visitChildren(IBaseObject* obj, Bool* visit) override;

private:
    std::unordered_set<std::string> requiredTags;
};

// Filter by Excluded tags

class ExcludedTagsSearchFilterImpl final : public ImplementationOf<ISearchFilter>
{
public:
    explicit ExcludedTagsSearchFilterImpl(const ListPtr<IString>& excludedTags);

    ErrCode INTERFACE_FUNC acceptsObject(IBaseObject* obj, Bool* accepts) override;
    ErrCode INTERFACE_FUNC visitChildren(IBaseObject* obj, Bool* visit) override;

private:
    std::unordered_set<std::string> excludedTags;
};

// Filter by interface ID

class InterfaceIdSearchFilterImpl final : public ImplementationOf<ISearchFilter>
{
public:
    explicit InterfaceIdSearchFilterImpl(const IntfID& id);

    ErrCode INTERFACE_FUNC acceptsObject(IBaseObject* obj, Bool* accepts) override;
    ErrCode INTERFACE_FUNC visitChildren(IBaseObject* obj, Bool* visit) override;

private:
    IntfID intfId;
};

// Filter by local ID

class LocalIdSearchFilterImpl final : public ImplementationOf<ISearchFilter>
{
public:
    explicit LocalIdSearchFilterImpl(const StringPtr& localId);

    ErrCode INTERFACE_FUNC acceptsObject(IBaseObject* obj, Bool* accepts) override;
    ErrCode INTERFACE_FUNC visitChildren(IBaseObject* obj, Bool* visit) override;

private:
    StringPtr localId;
};

END_NAMESPACE_OPENDAQ
