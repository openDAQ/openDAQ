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
#include <coreobjects/property_filter.h>
#include <coretypes/list_factory.h>
#include <coreobjects/property_ptr.h>
#include <coreobjects/property_filter_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

// Filter by Visible

class VisiblePropertyFilterImpl final : public ImplementationOf<IPropertyFilter>
{
public:
    explicit VisiblePropertyFilterImpl();

    ErrCode INTERFACE_FUNC acceptsProperty(IProperty* property, Bool* accepts) override;
};

// Filter by Read-only flag

class ReadOnlyPropertyFilterImpl final : public ImplementationOf<IPropertyFilter>
{
public:
    explicit ReadOnlyPropertyFilterImpl();

    ErrCode INTERFACE_FUNC acceptsProperty(IProperty* property, Bool* accepts) override;
};

// Filter by type

class TypePropertyFilterImpl final : public ImplementationOf<IPropertyFilter>
{
public:
    explicit TypePropertyFilterImpl(const CoreType& type);

    ErrCode INTERFACE_FUNC acceptsProperty(IProperty* property, Bool* accepts) override;

private:
    CoreType type;
};

// Filter by name

class NamePropertyFilterImpl final : public ImplementationOf<IPropertyFilter>
{
public:
    explicit NamePropertyFilterImpl(const StringPtr& name);

    ErrCode INTERFACE_FUNC acceptsProperty(IProperty* property, Bool* accepts) override;

private:
    StringPtr name;
};

// No filter

class AnyPropertyFilterImpl final : public ImplementationOf<IPropertyFilter>
{
public:
    explicit AnyPropertyFilterImpl();

    ErrCode INTERFACE_FUNC acceptsProperty(IProperty* property, Bool* accepts) override;
};

// Disjunction

class OrPropertyFilterImpl final : public ImplementationOf<IPropertyFilter>
{
public:
    explicit OrPropertyFilterImpl(const PropertyFilterPtr& left, const PropertyFilterPtr& right);

    ErrCode INTERFACE_FUNC acceptsProperty(IProperty* property, Bool* accepts) override;

private:
    PropertyFilterPtr left;
    PropertyFilterPtr right;
};

// Conjunction

class AndPropertyFilterImpl final : public ImplementationOf<IPropertyFilter>
{
public:
    explicit AndPropertyFilterImpl(const PropertyFilterPtr& left, const PropertyFilterPtr& right);

    ErrCode INTERFACE_FUNC acceptsProperty(IProperty* property, Bool* accepts) override;

private:
    PropertyFilterPtr left;
    PropertyFilterPtr right;
};

// Negation

class NotPropertyFilterImpl final : public ImplementationOf<IPropertyFilter>
{
public:
    explicit NotPropertyFilterImpl(const PropertyFilterPtr& filter);

    ErrCode INTERFACE_FUNC acceptsProperty(IProperty* property, Bool* accepts) override;

private:
    PropertyFilterPtr filter;
};

// Custom filter

class CustomPropertyFilterImpl final : public ImplementationOf<IPropertyFilter>
{
public:
    explicit CustomPropertyFilterImpl(const FunctionPtr& acceptsFunction);

    ErrCode INTERFACE_FUNC acceptsProperty(IProperty* property, Bool* accepts) override;
private:
    FunctionPtr acceptsFunc;
};

// Recursive filter

//class RecursivePropertyFilterImpl final : public ImplementationOf<IPropertyFilter, IRecursiveSearch>
//{
//public:
//    explicit RecursivePropertyFilterImpl(const PropertyFilterPtr& filter);

//    ErrCode INTERFACE_FUNC acceptsProperty(IProperty* property, Bool* accepts) override;

//private:
//    PropertyFilterPtr filter;
//};

END_NAMESPACE_OPENDAQ
