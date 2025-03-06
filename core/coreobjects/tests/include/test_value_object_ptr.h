/*
 * Copyright 2022-2025 openDAQ d.o.o.
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
#include "test_value_object.h"

BEGIN_NAMESPACE_OPENDAQ

class TestValueObjectPtr;

template <>
struct InterfaceToSmartPtr<ITestValueObject>
{
    typedef TestValueObjectPtr SmartPtr;
};

class TestValueObjectPtr : public ObjectPtr<ITestValueObject>
{
public:
    using ObjectPtr<ITestValueObject>::ObjectPtr;

    Int getValue() const
    {
        if (!object)
            THROW_OPENDAQ_EXCEPTION(InvalidParameterException());

        Int value;
        const auto errCode = object->getValue(&value);
        checkErrorInfo(errCode);
        return value;
    }
};

END_NAMESPACE_OPENDAQ
