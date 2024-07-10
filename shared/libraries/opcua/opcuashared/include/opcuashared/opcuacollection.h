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

/*
Common template for collection of endpoints, clients etc.
*/
#pragma once
#include <opcuashared/opcua.h>
#include <vector>
#include <algorithm>

BEGIN_NAMESPACE_OPENDAQ_OPCUA

template <class T, class OpcUaCollectionT = std::vector<T>>
class OpcUaCollection : public OpcUaCollectionT
{
    using OpcUaCollectionT::OpcUaCollectionT;

public:
    using BaseType = T;

    void remove(size_t index);
    int indexOf(const T item);
};

template <class T, class OpcUaCollectionT>
void OpcUaCollection<T, OpcUaCollectionT>::remove(size_t index)
{
    OpcUaCollection::erase(OpcUaCollection::begin() + index);
}

template <class T, class OpcUaCollectionT>
int OpcUaCollection<T, OpcUaCollectionT>::indexOf(const T item)
{
    size_t pos = std::find(OpcUaCollection::begin(), OpcUaCollection::end(), item) - OpcUaCollection::begin();
    if (pos < OpcUaCollection<T, OpcUaCollectionT>::size())
        return int(pos);
    return -1;
}

END_NAMESPACE_OPENDAQ_OPCUA
