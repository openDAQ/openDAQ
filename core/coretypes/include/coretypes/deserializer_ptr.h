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
#include <coretypes/string_ptr.h>
#include <coretypes/baseobject_factory.h>
#include <coretypes/deserializer.h>
#include <coretypes/updatable_ptr.h>
#include <coretypes/function_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @addtogroup types_deserializer
 * @{
 */

class DeserializerPtr : public ObjectPtr<IDeserializer>
{
public:
    using ObjectPtr<IDeserializer>::ObjectPtr;

    BaseObjectPtr deserialize(const StringPtr& serialized, const BaseObjectPtr& context = nullptr, const FunctionPtr& factoryCallback = nullptr) const
    {
        if (!object)
        {
            throw InvalidParameterException();
        }

        BaseObjectPtr baseObj;
        checkErrorInfo(object->deserialize(serialized, context, factoryCallback, &baseObj));

        return baseObj;
    }

    void update(const UpdatablePtr& updatable, const StringPtr& serialized) const
    {
        if (!object)
        {
            throw InvalidParameterException();
        }

        checkErrorInfo(object->update(updatable, serialized));
    }
};

/*!
 * @}
 */

END_NAMESPACE_OPENDAQ
