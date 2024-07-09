/*
 * Copyright 2022-2024 openDAQ d. o. o.
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
#include "test_value_object.h"
#include <coretypes/common.h>
#include <coretypes/intfs.h>
#include <coretypes/impl.h>

BEGIN_NAMESPACE_OPENDAQ

class TestValueObjectImpl : public ImplementationOf<ITestValueObject, ICoreType>
{
public:
    TestValueObjectImpl(Int value)
        : value(value)
    {
    }

    ErrCode INTERFACE_FUNC getValue(Int* value) override
    {
        *value = this->value;
        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC getCoreType(CoreType* coreType) override
    {
        *coreType = ctObject;
        return OPENDAQ_SUCCESS;
    }

private:
    Int value;
};

OPENDAQ_DEFINE_CLASS_FACTORY(INLINE_FACTORY, TestValueObject, Int, value)

END_NAMESPACE_OPENDAQ
