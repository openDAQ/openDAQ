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
#include <coretypes/common.h>
#include <coretypes/ctutils.h>
#include <coretypes/objectptr.h>

BEGIN_NAMESPACE_OPENDAQ

template <typename U>
class NativeIterator
{
public:
    using iterator_category = std::input_iterator_tag;
    using value_type = U;
    using difference_type = std::ptrdiff_t;
    using pointer = U*;
    using reference = U&;
    using const_reference = const U&;

    NativeIterator(IIterator*&& iterator);
    NativeIterator(NativeIterator&& it) noexcept;
    NativeIterator(const NativeIterator& it);

    NativeIterator<U>& operator=(const NativeIterator& it);
    NativeIterator<U>& operator++();
    bool operator!=(const NativeIterator<U>& other) const;
    bool operator==(const NativeIterator<U>& other) const;

    U operator*() const;

protected:
    ObjectPtr<IIterator> iterator;
};

template <class U>
NativeIterator<U>::NativeIterator(IIterator*&& iterator)
    : iterator(std::move(iterator))
{
}

template <class U>
NativeIterator<U>::NativeIterator(NativeIterator&& it) noexcept
    : iterator(it.iterator)
{
    it.iterator = nullptr;
}

template <class U>
NativeIterator<U>::NativeIterator(const NativeIterator& it)
    : iterator(it.iterator)
{
}

template <class U>
NativeIterator<U>& NativeIterator<U>::operator=(const NativeIterator<U>& other)
{
    if (this != &other)  // not a self-assignment
    {
        iterator = other.iterator;
    }
    return *this;
}

template <class U>
NativeIterator<U>& NativeIterator<U>::operator++()
{
    const auto res = iterator->moveNext();
    checkErrorInfo(res);

    return *this;
}

template <class U>
bool NativeIterator<U>::operator!=(const NativeIterator<U>& other) const
{
    Bool eq{false};
    const ErrCode errCode = iterator->equals(other.iterator, &eq);
    checkErrorInfo(errCode);

    return !eq;
}

template <class U>
bool NativeIterator<U>::operator==(const NativeIterator<U>& other) const
{
    return !operator!=(other);
}

template <class U>
U NativeIterator<U>::operator*() const
{
    ObjectPtr<IBaseObject> obj;
    ErrCode errCode = iterator->getCurrent(&obj);
    checkErrorInfo(errCode);

    if (obj == nullptr)
        return U();

    if constexpr (IsTemplateOf<U, std::pair>::value)
    {
        ObjectPtr<IList> list(std::move(obj));

        ObjectPtr<IBaseObject> key;
        errCode = list->getItemAt(0, &key);
        checkErrorInfo(errCode);

        ObjectPtr<IBaseObject> value;
        errCode = list->getItemAt(1, &value);
        checkErrorInfo(errCode);

        return std::make_pair(std::move(key), std::move(value));
    }
    else
    {
        return U(std::move(obj));
    }
}

END_NAMESPACE_OPENDAQ
