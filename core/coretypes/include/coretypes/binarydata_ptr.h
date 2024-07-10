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
#include <coretypes/binarydata.h>
#include <coretypes/objectptr.h>

BEGIN_NAMESPACE_OPENDAQ

class BinaryDataPtr;

END_NAMESPACE_OPENDAQ

BEGIN_NAMESPACE_OPENDAQ

template <>
struct InterfaceToSmartPtr<IBinaryData>
{
    typedef BinaryDataPtr SmartPtr;
};

END_NAMESPACE_OPENDAQ

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @addtogroup types_binary_data
 * @{
 */

/*!
 * @brief Represents binary large object (BLOB).
 *
 * Binary data is just a continuously allocated memory of a specific size. A client can get a pointer to
 * internal buffer and size it.
 */
class BinaryDataPtr : public ObjectPtr<IBinaryData>
{
public:
    using ObjectPtr<IBinaryData>::ObjectPtr;

    BinaryDataPtr()
    {
    }

    BinaryDataPtr(ObjectPtr<IBinaryData>&& ptr)
        : ObjectPtr<IBinaryData>(std::move(ptr))
    {
    }

    BinaryDataPtr(const ObjectPtr<IBinaryData>& ptr)
        : ObjectPtr<IBinaryData>(ptr)
    {
    }

    /*!
     * @brief Gets the address of the buffer.
     * @return The buffer's starting address.
     */
    void* getAddress() const
    {
        if (this->object == nullptr)
            throw InvalidParameterException();

        void* data;
        auto errCode = this->object->getAddress(&data);
        checkErrorInfo(errCode);

        return data;
    }

    /*!
     * @brief Gets the size of the buffer.
     * @return The buffer's size.
     */
    SizeT getSize() const
    {
        if (this->object == nullptr)
            throw InvalidParameterException();

        SizeT size;
        auto errCode = this->object->getSize(&size);
        checkErrorInfo(errCode);

        return size;
    }
};

/*!@}*/

END_NAMESPACE_OPENDAQ
