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
#include <coreobjects/search_filter.h>
#include <coretypes/intfs.h>
#include <coretypes/string_ptr.h>
#include <regex>

BEGIN_NAMESPACE_OPENDAQ

// Filter by Visible

class VisiblePropertyFilterImpl final : public ImplementationOf<ISearchFilter>
{
public:
    explicit VisiblePropertyFilterImpl();

    ErrCode INTERFACE_FUNC acceptsObject(IBaseObject* obj, Bool* accepts) override;
    ErrCode INTERFACE_FUNC visitChildren(IBaseObject* obj, Bool* visit) override;
};

// Filter by Read-only flag

class ReadOnlyPropertyFilterImpl final : public ImplementationOf<ISearchFilter>
{
public:
    explicit ReadOnlyPropertyFilterImpl();

    ErrCode INTERFACE_FUNC acceptsObject(IBaseObject* obj, Bool* accepts) override;
    ErrCode INTERFACE_FUNC visitChildren(IBaseObject* obj, Bool* visit) override;
};

// Filter by name

class NamePropertyFilterImpl final : public ImplementationOf<ISearchFilter>
{
public:
    explicit NamePropertyFilterImpl(const StringPtr& regex);

    ErrCode INTERFACE_FUNC acceptsObject(IBaseObject* obj, Bool* accepts) override;
    ErrCode INTERFACE_FUNC visitChildren(IBaseObject* obj, Bool* visit) override;

private:
    const std::regex regex;
};

END_NAMESPACE_OPENDAQ
