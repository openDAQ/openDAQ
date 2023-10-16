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
#include <opcuashared/opcua.h>
#include <opcuashared/opcuaobject.h>
#include <initializer_list>
#include <vector>

BEGIN_NAMESPACE_OPENDAQ_OPCUA

template <typename UATYPE>
class OpcUaVectorProxy;
template <typename UATYPE>
class OpcUaVector;

template <typename UATYPE>
class OpcUaVectorProxy
{
public:
    OpcUaVector<UATYPE>& self;
    size_t key;

    OpcUaVectorProxy(OpcUaVector<UATYPE>& self, size_t key);

    OpcUaVectorProxy<UATYPE>& operator=(UATYPE& value);
    operator UATYPE() const;
    UATYPE* operator->() const;
};

template <typename UATYPE>
class OpcUaVector
{
public:
    OpcUaVector();
    explicit OpcUaVector(size_t size);
    OpcUaVector(std::initializer_list<UATYPE> items);

    OpcUaVector<UATYPE>& operator=(const OpcUaVector<UATYPE>& vector);
    OpcUaVectorProxy<UATYPE> const operator[](size_t i) const;
    OpcUaVectorProxy<UATYPE> operator[](size_t i);

    void push_back(const UATYPE& value);
    size_t size() const;
    void resize(size_t size);
    void clear();
    const UATYPE* data() const;
    void set(size_t i, UATYPE& value);
    const UATYPE& get(size_t i) const;

    ~OpcUaVector();

private:
    friend class OpcUaVectorProxy<UATYPE>;
    std::vector<UATYPE> items;

    void clearItemsRange(size_t from, size_t to);
    void clearItemsRange(size_t from);
    void copyItems(const std::vector<UATYPE>& src);
};

template <typename UATYPE>
inline OpcUaVectorProxy<UATYPE>::OpcUaVectorProxy(OpcUaVector<UATYPE>& self, size_t key)
    : self(self)
    , key(key)
{
}

template <typename UATYPE>
inline OpcUaVectorProxy<UATYPE>& OpcUaVectorProxy<UATYPE>::operator=(UATYPE& value)
{
    self.set(key, value);
    return *this;
}

template <typename UATYPE>
inline OpcUaVectorProxy<UATYPE>::operator UATYPE() const
{
    return self.get(key);
}

template <typename UATYPE>
inline UATYPE* OpcUaVectorProxy<UATYPE>::operator->() const
{
    return &self.get(key);
}

template <typename UATYPE>
inline OpcUaVector<UATYPE>::OpcUaVector()
{
}

template <typename UATYPE>
inline OpcUaVector<UATYPE>::OpcUaVector(size_t size)
    : items(size)
{
}

template <typename UATYPE>
inline OpcUaVector<UATYPE>::OpcUaVector(std::initializer_list<UATYPE> items)
    : items(items)
{
}

template <typename UATYPE>
inline OpcUaVector<UATYPE>& OpcUaVector<UATYPE>::operator=(const OpcUaVector<UATYPE>& vector)
{
    if (this == &vector)
        return *this;

    clear();
    resize(vector.size());
    copyItems(vector.items);

    return *this;
}

template <typename UATYPE>
inline OpcUaVectorProxy<UATYPE> const OpcUaVector<UATYPE>::operator[](size_t i) const
{
    return OpcUaVectorProxy<UATYPE>(*this, i);
}

template <typename UATYPE>
inline OpcUaVectorProxy<UATYPE> OpcUaVector<UATYPE>::operator[](size_t i)
{
    return OpcUaVectorProxy<UATYPE>(*this, i);
}

template <typename UATYPE>
inline OpcUaVector<UATYPE>::~OpcUaVector()
{
    clear();
}

template <typename UATYPE>
inline void OpcUaVector<UATYPE>::push_back(const UATYPE& value)
{
    UATYPE tmp;
    UA_copy(&value, &tmp, TypeToUaDataType<UATYPE>::DataType);
    items.push_back(tmp);
}

template <typename UATYPE>
inline size_t OpcUaVector<UATYPE>::size() const
{
    return items.size();
}

template <typename UATYPE>
inline void OpcUaVector<UATYPE>::resize(size_t size)
{
    if (size < items.size())
        clearItemsRange(size);
    items.resize(size);
}

template <typename UATYPE>
inline void OpcUaVector<UATYPE>::clear()
{
    for (size_t i = 0; i < items.size(); i++)
        clearItemsRange(0);
    items.clear();
}

template <typename UATYPE>
inline const UATYPE* OpcUaVector<UATYPE>::data() const
{
    return items.data();
}

template <typename UATYPE>
inline void OpcUaVector<UATYPE>::set(size_t i, UATYPE& value)
{
    UA_clear(&items[i], TypeToUaDataType<UATYPE>::DataType);
    UA_copy(&value, &items[i], TypeToUaDataType<UATYPE>::DataType);
}

template <typename UATYPE>
inline const UATYPE& OpcUaVector<UATYPE>::get(size_t i) const
{
    return items[i];
}

template <typename UATYPE>
inline void OpcUaVector<UATYPE>::clearItemsRange(size_t from, size_t to)
{
    for (size_t i = from; i < to; i++)
        UA_clear(&items[i], TypeToUaDataType<UATYPE>::DataType);
}

template <typename UATYPE>
inline void OpcUaVector<UATYPE>::clearItemsRange(size_t from)
{
    clearItemsRange(from, items.size());
}

template <typename UATYPE>
inline void OpcUaVector<UATYPE>::copyItems(const std::vector<UATYPE>& src)
{
    for (size_t i = 0; i < src.size(); i++)
        UA_copy(&src[i], &items[i], TypeToUaDataType<UATYPE>::DataType);
}

END_NAMESPACE_OPENDAQ_OPCUA
