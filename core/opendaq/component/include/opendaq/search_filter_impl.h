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
#include <opendaq/search_filter.h>
#include <coretypes/list_factory.h>
#include <opendaq/component_ptr.h>
#include <opendaq/search_filter_ptr.h>
#include <opendaq/recursive_search_ptr.h>


BEGIN_NAMESPACE_OPENDAQ

// Filter by Visible

class VisibleSearchFilterImpl final : public ImplementationOf<ISearchFilter>
{
public:
    explicit VisibleSearchFilterImpl();

    ErrCode INTERFACE_FUNC acceptsComponent(IComponent* component, Bool* accepts) override;
    ErrCode INTERFACE_FUNC visitChildren(IComponent* component, Bool* visit) override;
};

// Filter by Required tags

class RequiredTagsSearchFilterImpl final : public ImplementationOf<ISearchFilter>
{
public:
    explicit RequiredTagsSearchFilterImpl(const ListPtr<IString>& requiredTags);

    ErrCode INTERFACE_FUNC acceptsComponent(IComponent* component, Bool* accepts) override;
    ErrCode INTERFACE_FUNC visitChildren(IComponent* component, Bool* visit) override;

private:
    std::unordered_set<std::string> requiredTags;
};

// Filter by Excluded tags

class ExcludedTagsSearchFilterImpl final : public ImplementationOf<ISearchFilter>
{
public:
    explicit ExcludedTagsSearchFilterImpl(const ListPtr<IString>& excludedTags);

    ErrCode INTERFACE_FUNC acceptsComponent(IComponent* component, Bool* accepts) override;
    ErrCode INTERFACE_FUNC visitChildren(IComponent* component, Bool* visit) override;

private:
    std::unordered_set<std::string> excludedTags;
};

// Filter by Search ID

class SearchIdSearchFilterImpl final : public ImplementationOf<ISearchFilter>
{
public:
    explicit SearchIdSearchFilterImpl(IntfID id);

    ErrCode INTERFACE_FUNC acceptsComponent(IComponent* component, Bool* accepts) override;
    ErrCode INTERFACE_FUNC visitChildren(IComponent* component, Bool* visit) override;

private:
    IntfID searchId;
};

// No filter

class AnySearchFilterImpl final : public ImplementationOf<ISearchFilter>
{
public:
    explicit AnySearchFilterImpl();

    ErrCode INTERFACE_FUNC acceptsComponent(IComponent* component, Bool* accepts) override;
    ErrCode INTERFACE_FUNC visitChildren(IComponent* component, Bool* visit) override;
};

// Disjunction

class OrSearchFilterImpl final : public ImplementationOf<ISearchFilter>
{
public:
    explicit OrSearchFilterImpl(const SearchFilterPtr& left, const SearchFilterPtr& right);

    ErrCode INTERFACE_FUNC acceptsComponent(IComponent* component, Bool* accepts) override;
    ErrCode INTERFACE_FUNC visitChildren(IComponent* component, Bool* visit) override;

private:
    SearchFilterPtr left;
    SearchFilterPtr right;
};

// Conjunction

class AndSearchFilterImpl final : public ImplementationOf<ISearchFilter>
{
public:
    explicit AndSearchFilterImpl(const SearchFilterPtr& left, const SearchFilterPtr& right);

    ErrCode INTERFACE_FUNC acceptsComponent(IComponent* component, Bool* accepts) override;
    ErrCode INTERFACE_FUNC visitChildren(IComponent* component, Bool* visit) override;

private:
    SearchFilterPtr left;
    SearchFilterPtr right;
};

// Negation

class NotSearchFilterImpl final : public ImplementationOf<ISearchFilter>
{
public:
    explicit NotSearchFilterImpl(const SearchFilterPtr& filter);

    ErrCode INTERFACE_FUNC acceptsComponent(IComponent* component, Bool* accepts) override;
    ErrCode INTERFACE_FUNC visitChildren(IComponent* component, Bool* visit) override;

private:
    SearchFilterPtr filter;
};

// Custom filter

class CustomSearchFilterImpl final : public ImplementationOf<ISearchFilter>
{
public:
    explicit CustomSearchFilterImpl(const FunctionPtr& acceptsFunction, const FunctionPtr& visitFunction);

    ErrCode INTERFACE_FUNC acceptsComponent(IComponent* component, Bool* accepts) override;
    ErrCode INTERFACE_FUNC visitChildren(IComponent* component, Bool* visit) override;
private:
    FunctionPtr acceptsFunc;
    FunctionPtr visitFunc;
};

// Recursive filter

class RecursiveSearchFilterImpl final : public ImplementationOf<ISearchFilter, IRecursiveSearch>
{
public:
    explicit RecursiveSearchFilterImpl(const SearchFilterPtr& filter);

    ErrCode INTERFACE_FUNC acceptsComponent(IComponent* component, Bool* accepts) override;
    ErrCode INTERFACE_FUNC visitChildren(IComponent* component, Bool* visit) override;
private:
    SearchFilterPtr filter;
};

END_NAMESPACE_OPENDAQ
