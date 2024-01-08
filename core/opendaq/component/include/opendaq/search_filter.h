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
#include <opendaq/component.h>

BEGIN_NAMESPACE_OPENDAQ

DECLARE_OPENDAQ_INTERFACE(ISearchFilter, IBaseObject)
{
    virtual ErrCode INTERFACE_FUNC acceptsComponent(IComponent* component, Bool* accepts) = 0;
    virtual ErrCode INTERFACE_FUNC visitChildren(IComponent* component, Bool* visit) = 0;
};

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, VisibleSearchFilter, ISearchFilter)
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, RequiredTagsSearchFilter, ISearchFilter, IList*, requiredTags)
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, ExcludedTagsSearchFilter, ISearchFilter, IList*, excludedTags)
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, SearchIdSearchFilter, ISearchFilter, IntfID, searchId)
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, AnySearchFilter, ISearchFilter)
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, AndSearchFilter, ISearchFilter, ISearchFilter*, left, ISearchFilter*, right)
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, OrSearchFilter, ISearchFilter, ISearchFilter*, left, ISearchFilter*, right)
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, NotSearchFilter, ISearchFilter, ISearchFilter*, filter)
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, CustomSearchFilter, ISearchFilter, IFunction*, acceptsFunction, IFunction*, visitFunction)
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, RecursiveSearchFilter, ISearchFilter, ISearchFilter*, filter)

END_NAMESPACE_OPENDAQ
