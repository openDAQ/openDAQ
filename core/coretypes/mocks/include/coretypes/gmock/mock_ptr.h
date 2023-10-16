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
#include <memory>
#include <coretypes/objectptr.h>

// This wrapper type somewhat simplifies dealing with mocked openDAQ interface implementations.
// Specifically, it addresses some difficulties that arise when using an ObjectPtr<> directly:
//
// 1. EXPECT_CALL() expects a reference to the mocked object, but getting this from an ObjectPtr<>
//    requires dynamic_cast'ing ptr.getObject(), which is ugly.
// 2. It's often useful to access member variables of a mocked object, such as a mocked
//    IContext which contains a mocked IScheduler. Accessing these members via an
//    ObjectPtr again requires a dynamic_cast on ptr.getObject().
//
// To solve these problems, this type wraps a ObjectPtr<>, provides a dereference (->) operator to
// access the ObjectPtr's members, provides an explicit accessor for the underlying mocked object,
// and provides an implicit conversion operator for conversion to a ObjectPtr or an IInterface *
// so that it can be passed directly to native and auto-generated openDAQ wrapper functions.
// This type should of course only be used in unit tests and never in "production" SDK code.

#if defined(_WIN32)
    #define MOCK_CALL , Calltype(INTERFACE_FUNC)
#else
    #define MOCK_CALL
#endif

template <typename Intf, typename Ptr, typename Mock, typename ... CtorArgs>
struct MockPtr
{
    Ptr ptr;

    MockPtr(CtorArgs&&... args)
    {
        ptr = daq::createWithImplementation<Intf, Mock>(std::forward<CtorArgs>(args)...);
    }

    Ptr& operator*()
    {
        return ptr;
    }

    Ptr *operator->()
    {
        // must use std::addressof() because ObjectPtr<> overloads operator&
        return std::addressof(ptr);
    }

    Mock& mock()
    {
        return *dynamic_cast<Mock*>(ptr.getObject());
    }

    operator Ptr& ()
    {
        return ptr;
    }
};
