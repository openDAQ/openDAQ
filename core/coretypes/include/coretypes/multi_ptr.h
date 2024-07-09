/*
 * Copyright 2022-2024 openDAQ d. o. o.
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
#include <coretypes/common.h>
#include <coretypes/arguments.h>

BEGIN_NAMESPACE_OPENDAQ

//template <typename... TPtrs>
//class MultiPtr : public TPtrs...
//{
//private:
//    // Workaround for a Visual Studio bug
//    template<typename T>
//    struct ExpandArgs
//    {
//        using DeclaredInterface = typename T::DeclaredInterface;
//    };
//
//public:
//    using Ptrs = Args<TPtrs...>;
//    using Interfaces = Args<typename ExpandArgs<TPtrs>::DeclaredInterface...>;
//
//    using MainInterface = typename Interfaces::Head;
//    using MainPtr = typename Ptrs::Head;
//
//    template <typename T>
//    using InterfacePtr = typename Meta::TypeAt<Ptrs, Meta::IndexOf<Interfaces, T>::Index>::Result;
//
//    using MainPtr::as;
//    using MainPtr::asOrNull;
//    using MainPtr::asPtr;
//    using MainPtr::asPtrOrNull;
//    using MainPtr::operator->;
//
//    MultiPtr() = default;
//
//    template <typename T>
//    MultiPtr(T*& ptr)
//        : TPtrs(std::move(ptr))...
//    {
//        if (ptr)
//            ptr->addRef();
//    }
//
//    template <typename T>
//    MultiPtr(T*&& ptr)
//        : TPtrs(std::move(ptr))...
//    {
//    }
//
//    template <typename T>
//    MultiPtr(ObjectPtr<T>& ptr)
//        : MultiPtr(ptr.addRefAndReturn())
//    {
//    }
//
//    ~MultiPtr() override
//    {
//        release();
//    }
//
//    // Avoid ambiguity issues
//
//    template <typename T = MainInterface>
//    T* detach()
//    {
//        static_assert(Meta::HasArgumentWithType<Interfaces, T>::Value, "The requested interface is not declared in any of the SmartPtrs.");
//
//        T* raw = InterfacePtr<T>::detach();
//        detachInternal<typename Meta::RemoveOneOf<InterfacePtr<T>, Ptrs>::Folded>();
//        return raw;
//    }
//
//    void release()
//    {
//        Ptrs::Head::release();
//        releaseInternal<typename Ptrs::Tail>();
//    }
//
//    void dispose()
//    {
//        Ptrs::Head::dispose();
//    }
//
//    template <typename T = MainInterface>
//    T* addRefAndReturn()
//    {
//        static_assert(Meta::HasArgumentWithType<Interfaces, T>::Value, "The requested interface is not declared in any of the SmartPtrs.");
//
//        return InterfacePtr<T>::addRefAndReturn();
//    }
//
//    template <typename T = MainInterface>
//    T** addressOf()
//    {
//        static_assert(Meta::HasArgumentWithType<Interfaces, T>::Value, "The requested interface is not declared in any of the SmartPtrs.");
//
//        return InterfacePtr<T>::addressOf();
//    }
//
//    template <typename T = MainInterface>
//    T* getObject() const
//    {
//        static_assert(Meta::HasArgumentWithType<Interfaces, T>::Value, "The requested interface is not declared in any of the SmartPtrs.");
//
//        return InterfacePtr<T>::getObject();
//    }
//
//    bool assigned()
//    {
//        return (TPtrs::assigned() && ...);
//    }
//
//private:
//    template <typename TArgs>
//    void releaseInternal()
//    {
//        using Ptr = typename TArgs::Head;
//        using Rest = typename TArgs::Tail;
//
//        Ptr::object = nullptr;
//        Ptr::borrowed = false;
//
//        Ptr::release();
//
//        if constexpr (!std::is_same<Rest, Details::EndTag>::value)
//        {
//            releaseInternal<Rest>();
//        }
//    }
//
//    template <typename TArgs>
//    void detachInternal()
//    {
//        using Ptr = typename TArgs::Head;
//        using Rest = typename TArgs::Tail;
//
//        Ptr::detach();
//
//        if constexpr (!std::is_same<Rest, Details::EndTag>::value)
//        {
//            detachInternal<Rest>();
//        }
//    }
//};

END_NAMESPACE_OPENDAQ
