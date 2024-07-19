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
#include <coretypes/coretypes.h>

BEGIN_NAMESPACE_OPENDAQ

template <class Intf>
struct ObjectHash
{
    size_t operator()(const ObjectPtr<Intf>& obj) const
    {
        SizeT hashCode;
        obj->getHashCode(&hashCode);
        return hashCode;
    }

    size_t operator()(const typename InterfaceToSmartPtr<Intf>::SmartPtr& obj) const
    {
        SizeT hashCode;
        obj->getHashCode(&hashCode);
        return hashCode;
    }
};

template <class Intf>
struct ObjectEqualTo
{
    bool operator()(const ObjectPtr<Intf>& a, const ObjectPtr<Intf>& b) const
    {
        assert(a != nullptr && b != nullptr);
        return a.equals(b);
    }

    bool operator()(const typename InterfaceToSmartPtr<Intf>::SmartPtr& a, const typename InterfaceToSmartPtr<Intf>::SmartPtr& b) const
    {
        assert(a != nullptr && b != nullptr);
        return a.equals(b);
    }
};

struct StringHash
{
    size_t operator()(const StringPtr& str) const
    {
        SizeT hashCode;
        str->getHashCode(&hashCode);
        return hashCode;
    }
};

struct StringEqualTo
{
    bool operator()(const StringPtr& a, const StringPtr& b) const
    {
        assert(a != nullptr && b != nullptr);

        ConstCharPtr aChPtr;
        a->getCharPtr(&aChPtr);
        ConstCharPtr bChPtr;
        b->getCharPtr(&bChPtr);
        return strcmp(aChPtr, bChPtr) == 0;
    };
};

END_NAMESPACE_OPENDAQ

namespace std
{
    template <>
    struct hash<daq::StringPtr> : daq::StringHash
    {
    };

    template <>
    struct equal_to<daq::StringPtr> : daq::StringEqualTo
    {
    };
}
