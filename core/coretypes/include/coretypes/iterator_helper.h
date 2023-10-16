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
#include <coretypes/iterator.h>

BEGIN_NAMESPACE_OPENDAQ

inline ErrCode compareIterators(const IIterator* it1, const IIterator* it2, Bool* equal)
{
    if (it1 == nullptr || it2 == nullptr || equal == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *equal = false;

    IBaseObject* obj1;
    auto err = it1->getCurrent(&obj1);
    if (err == OPENDAQ_ERR_NOTASSIGNED)
    {
        obj1 = nullptr;
    }
    else if (!OPENDAQ_SUCCEEDED(err))
    {
        return err;
    }

    Finally final2([&obj1]()
    {
        if (obj1)
            obj1->releaseRef();
    });

    IBaseObject* obj2;
    err = it2->getCurrent(&obj2);
    if (err == OPENDAQ_ERR_NOTASSIGNED)
    {
        obj2 = nullptr;
    }
    else if (!OPENDAQ_SUCCEEDED(err))
    {
        return err;
    }

    Finally final3([&obj2]()
    {
        if (obj2)
            obj2->releaseRef();
    });

    if (obj2 == nullptr)
    {
        *equal = obj1 == nullptr;
        return OPENDAQ_SUCCESS;
    }

    if (obj1 == nullptr)
        return OPENDAQ_SUCCESS;

    return obj2->equals(obj1, equal);
}

template <class T>
ErrCode iteratorMoveNext(bool& started, typename T::const_iterator& it, const typename T::const_iterator& end)
{
    if (!started)
    {
        started = true;
        if (it == end)
            return OPENDAQ_NO_MORE_ITEMS;

        return OPENDAQ_SUCCESS;
    }

    if (it == end || ++it == end)
        return OPENDAQ_NO_MORE_ITEMS;

    return OPENDAQ_SUCCESS;
}

END_NAMESPACE_OPENDAQ
