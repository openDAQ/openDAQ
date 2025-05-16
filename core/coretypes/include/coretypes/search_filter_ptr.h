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
#include <coretypes/search_filter.h>
#include <coretypes/baseobject_factory.h>

BEGIN_NAMESPACE_OPENDAQ

class SearchFilterPtr;

template <>
struct InterfaceToSmartPtr<ISearchFilter>
{
    using SmartPtr = SearchFilterPtr;
};

/*!
 * @ingroup types_search
 * @addtogroup types_search_filter Search filter
 * @{
 */

/*!
 * @brief Search filter that can be passed as an optional parameter to search functions to filter
 * out unwanted results. Allows for recursive searches.
 *
 * Each filter defines an "accepts object" and "visit children" function.
 *
 * Accepts object defines whether or not the object being evaluated as part of a search method should be included
 * in the resulting output.
 *
 * Visit children defines whether or not the children of said object should be traversed during a recursive search.
 */

class SearchFilterPtr : public ObjectPtr<ISearchFilter>
{
public:
    using ObjectPtr<ISearchFilter>::ObjectPtr;

    SearchFilterPtr()
        : ObjectPtr<ISearchFilter>()
    {
    }

    SearchFilterPtr(ObjectPtr<ISearchFilter>&& ptr)
        : ObjectPtr<ISearchFilter>(std::move(ptr))
    {
    }

    SearchFilterPtr(const ObjectPtr<ISearchFilter>& ptr)
        : ObjectPtr<ISearchFilter>(ptr)
    {
    }

    SearchFilterPtr(const SearchFilterPtr& other)
        : ObjectPtr<ISearchFilter>(other)
    {
    }

    SearchFilterPtr(SearchFilterPtr&& other) noexcept
        : ObjectPtr<ISearchFilter>(std::move(other))
    {
    }
    
    SearchFilterPtr& operator=(const SearchFilterPtr& other)
    {
        if (this == &other)
            return *this;

        ObjectPtr<ISearchFilter>::operator =(other);
        return *this;
    }

    SearchFilterPtr& operator=(SearchFilterPtr&& other) noexcept
    {
        if (this == std::addressof(other))
            return *this;

        ObjectPtr<ISearchFilter>::operator =(std::move(other));
        return *this;
    }

    /*!
     * @brief Defines whether or not the object should be included in the search results
     * @param obj The object being evaluated.
     * @returns True of the object is to be included in the results; false otherwise.
     */
    Bool acceptsObject(const BaseObjectPtr& obj) const
    {
        if (this->object == nullptr)
            throw InvalidParameterException();

        Bool accepts;
        auto errCode = this->object->acceptsObject(obj, &accepts);
        checkErrorInfo(errCode);

        return accepts;
    }

    /*!
     * @brief Defines whether or not the children of said object should be traversed during a recursive search.
     * @param obj The object being evaluated.
     * @returns True of the object's children should be traversed; false otherwise.
     */
    Bool visitChildren(const BaseObjectPtr& obj) const
    {
        if (this->object == nullptr)
            throw InvalidParameterException();

        Bool visit;
        auto errCode = this->object->visitChildren(obj, &visit);
        checkErrorInfo(errCode);

        return visit;
    }
};

/*!
 * @}
 */

END_NAMESPACE_OPENDAQ
