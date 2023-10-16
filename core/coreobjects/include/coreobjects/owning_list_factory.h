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
#include <coretypes/listptr.h>
#include <coretypes/listobject_factory.h>
#include <coreobjects/property_object_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, OwningList, IList, IPropertyObject*, owner, IString*, ref)

template <class TValueInterface, class TValuePtr = typename InterfaceToSmartPtr<TValueInterface>::SmartPtr>
ListObjectPtr<IList, TValueInterface, TValuePtr> OwningList(const PropertyObjectPtr& owner, StringPtr ref = "list")
{
    ListObjectPtr<IList, TValueInterface, TValuePtr> obj(OwningList_Create(owner, ref));
    return obj;
}

template <class T, class S = typename InterfaceToSmartPtr<T>::SmartPtr, typename... Elements>
ListPtr<T, S> OwningList(StringPtr ref, PropertyObjectPtr owner, Elements&&... elements)
{
    auto list = OwningList<T, S>(owner, std::move(ref));
    addToList(list, std::forward<Elements>(elements)...);
    return list;
}

template <typename... Elements>
auto OwningList(StringPtr ref, PropertyObjectPtr owner, Elements&&... elements)
{
    return OwningList<IBaseObject, ObjectPtr<IBaseObject>>(std::move(ref), owner, std::forward<Elements>(elements)...);
}

template <typename... Elements>
auto OwningList(PropertyObjectPtr owner, Elements&&... elements)
{
    return OwningList<IBaseObject, ObjectPtr<IBaseObject>>("list", owner, std::forward<Elements>(elements)...);
}

END_NAMESPACE_OPENDAQ
