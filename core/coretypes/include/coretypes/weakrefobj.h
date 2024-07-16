/*
 * Copyright 2022-2024 openDAQ d.o.o.
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
#include <coretypes/weakref.h>
#include <coretypes/weakref_impl.h>
#include <coretypes/intfs.h>
#include <coretypes/weakrefptr.h>

BEGIN_NAMESPACE_OPENDAQ

template <typename MainInterface, typename ... Intfs>
class IntfObjectWithWeakRefImpl : public IntfObjectSupportsWeakRefImpl<MainInterface, ISupportsWeakRef, Intfs...>
{
public:
    ErrCode INTERFACE_FUNC getWeakRef(IWeakRef** weakRef) override;
protected:
    template <class TInterface>
    WeakRefPtr<TInterface> getWeakRefInternal();
};

template <typename MainInterface, typename... Intfs>
ErrCode IntfObjectWithWeakRefImpl<MainInterface, Intfs...>::getWeakRef(IWeakRef** weakRef)
{
    std::atomic_fetch_add_explicit(&this->refCount->weak, 1, std::memory_order_relaxed);

    IBaseObject* thisBaseObject;
    this->borrowInterface(IBaseObject::Id, reinterpret_cast<void**>(&thisBaseObject));
    *weakRef = new WeakRefImpl(thisBaseObject, this->refCount.get());
    (*weakRef)->addRef();

    return OPENDAQ_SUCCESS;
}

template <typename MainInterface, typename ... Intfs>
template <class TInterface>
WeakRefPtr<TInterface> IntfObjectWithWeakRefImpl<MainInterface, Intfs...>::getWeakRefInternal()
{
    std::atomic_fetch_add_explicit(&this->refCount->weak, 1, std::memory_order_relaxed);

    IWeakRef* weakRef = new WeakRefImpl(this->template borrowInterface<MainInterface>(), this->refCount.get());
    return WeakRefPtr<TInterface>(weakRef);
}

template <typename... Interfaces>
using ImplementationOfWeak = IntfObjectWithWeakRefImpl<Interfaces..., IInspectable>;

//template <typename... Intfs>
//using ImplementationOfWeak = typename Meta::FoldType<typename ActualInterfaces<Intfs...>::Interfaces, IntfObjectWithWeakRefImpl>::Folded;

END_NAMESPACE_OPENDAQ
