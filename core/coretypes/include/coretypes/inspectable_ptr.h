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
#include <coretypes/objectptr.h>
#include <coretypes/inspectable.h>
#include <coretypes/string_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

class InspectablePtr;

template <>
struct InterfaceToSmartPtr<IInspectable>
{
    typedef InspectablePtr SmartPtr;
};

/*!
 * @ingroup coretypes
 * @addtogroup types_inspectable
 * @{
 */

class InspectablePtr : public ObjectPtr<IInspectable>
{
public:
    using ObjectPtr<IInspectable>::ObjectPtr;

    /**
     * @brief Retrieves the Ids of interfaces this object implements.
     * @return The interface ids implemented without @c IBaseObject and @c IUnknown.
     */
    [[nodiscard]]
    std::vector<IntfID> getInterfaceIds() const
    {
        if (this->object == nullptr)
        {
            throw InvalidParameterException();
        }

        SizeT count{};
        ErrCode errCode = object->getInterfaceIds(&count, nullptr);
        checkErrorInfo(errCode);

        std::vector<IntfID> ids(count);

        IntfID* begin = ids.data();
        errCode = object->getInterfaceIds(&count, &begin);
        checkErrorInfo(errCode);

        return ids;
    }

    /**
    * @brief Gets the fully qualified name of the openDAQ object providing the implementation.
    * @return The actual implementation class name.
    */
    [[nodiscard]]
    StringPtr getRuntimeClassName() const
    {
        if (this->object == nullptr)
            throw daq::InvalidParameterException();

        daq::StringPtr name;
        auto errCode = this->object->getRuntimeClassName(&name);
        daq::checkErrorInfo(errCode);

        return name;
    }
};

/*!@}*/

END_NAMESPACE_OPENDAQ
