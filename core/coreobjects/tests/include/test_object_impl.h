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
#include <coretypes/common.h>
#include <coreobjects/property_object_impl.h>
#include <coreobjects/property_value.h>
#include "test_object.h"
#include "test_object_property_class.h"

BEGIN_NAMESPACE_OPENDAQ

class TestObjectImpl : public GenericPropertyObjectImpl<ITestObject>
                     , private TestObjectPropertyClass
{
public:
    TestObjectImpl(TypeManagerPtr manager)
        : GenericPropertyObjectImpl<ITestObject>(manager, StringPtr(ClassName))
        , valueObject(this, ValueObjectProp)
    {
    }

    ErrCode INTERFACE_FUNC getValueObject(ITestValueObject** value) override
    {
        *value = valueObject.addRefAndReturn();
        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC setValueObject(ITestValueObject* value) override
    {
        valueObject = value;
        return OPENDAQ_SUCCESS;
    }
    ErrCode INTERFACE_FUNC setValueObjectProtected(ITestValueObject* value) override
    {
        valueObject.setProtected(value);
        return OPENDAQ_SUCCESS;
    }

protected:
    PropertyValue<ITestValueObject> valueObject;
};

extern "C" inline ErrCode createTestObject(ITestObject** objTmp, ITypeManager* manager)
{
    return daq::createObject<ITestObject, TestObjectImpl>(objTmp, manager);
}

END_NAMESPACE_OPENDAQ
