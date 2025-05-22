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
#include <coretypes/baseobject.h>
#include <coretypes/function.h>

BEGIN_NAMESPACE_OPENDAQ

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
DECLARE_OPENDAQ_INTERFACE(ISearchFilter, IBaseObject)
{
    /*!
     * @brief Defines whether or not the object should be included in the search results
     * @param obj The object being evaluated.
     * @param[out] accepts True of the object is to be included in the results; false otherwise.
     */
    virtual ErrCode INTERFACE_FUNC acceptsObject(IBaseObject* obj, Bool* accepts) = 0;

    /*!
     * @brief Defines whether or not the children of said object should be traversed during a recursive search.
     * @param obj The object being evaluated.
     * @param[out] visit True of the object's children should be traversed; false otherwise.
     */
    virtual ErrCode INTERFACE_FUNC visitChildren(IBaseObject* obj, Bool* visit) = 0;
};

/*!@}*/

/*!
 * @ingroup types_search_filter
 * @addtogroup types_search_filter_factories Factories
 * @{
 */

/*!
 * @brief Creates a search filter that accepts all objects. "Visit children" always returns `true`.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, AnySearchFilter, ISearchFilter)

/*!
 * @brief Creates a "conjunction" search filter that combines 2 filters, accepting an object only if both filters accept it.
 * "Visit children" returns `true` only if both filters do so.
 * @param left The first argument of the conjunction operation.
 * @param right The second argument of the conjunction operation.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, AndSearchFilter, ISearchFilter, ISearchFilter*, left, ISearchFilter*, right)

/*!
 * @brief Creates a "disjunction" search filter that combines 2 filters, accepting an object if any of the two filters accepts it.
 * "Visit children" returns `true` if any of the two filters accepts does so.
 * @param left The first argument of the disjunction operation.
 * @param right The second argument of the disjunction operation.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, OrSearchFilter, ISearchFilter, ISearchFilter*, left, ISearchFilter*, right)

/*!
 * @brief Creates a search filter that negates the "accepts object" result of the filter provided as construction argument.
 * Does not negate the "visit children" result.
 * @param filter The filter of which results should be negated.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, NotSearchFilter, ISearchFilter, ISearchFilter*, filter)

/*!
 * @brief Creates a custom search filter with a user-defined "accepts object" and "visit children" function.
 * @param acceptsFunction The function to be called when "accepts object" is called. Should return `true` or `false`.
 * @param visitFunction The function to be called when "visit children" is called. Should return `true` or `false`.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, CustomSearchFilter, ISearchFilter, IFunction*, acceptsFunction, IFunction*, visitFunction)

/*!
 * @brief Creates a search filter that indicates that the search method should recursively search through the object's child elements.
 * This filter constructor should always be the final filter wrapper, and should not be used as a constructor argument
 * for another filter.
 * @param filter The filter to be wrapped with a "recursive" flag.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, RecursiveSearchFilter, ISearchFilter, ISearchFilter*, filter)

/*!@}*/

END_NAMESPACE_OPENDAQ
