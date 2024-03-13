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
#include "test_object.h"
#include "test_value_object_ptr.h"
#include <coreobjects/property_object_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

class TestObjectPtr;

template <>
struct InterfaceToSmartPtr<ITestObject>
{
    typedef TestObjectPtr SmartPtr;
};

class TestObjectPtr : public GenericPropertyObjectPtr<ITestObject>
{
public:
    using GenericPropertyObjectPtr<ITestObject>::GenericPropertyObjectPtr;

    TestObjectPtr()
        : GenericPropertyObjectPtr<ITestObject>()

    {
    }

    TestObjectPtr(ObjectPtr<ITestObject>&& ptr)
        : GenericPropertyObjectPtr<ITestObject>(ptr)

    {
    }

    TestObjectPtr(ObjectPtr<ITestObject>& ptr)
        : GenericPropertyObjectPtr<ITestObject>(ptr)

    {
    }

    TestValueObjectPtr getValueObject()
    {
        if (this->object == nullptr)
            throw InvalidParameterException();

        TestValueObjectPtr obj;
        const auto errCode = this->object->getValueObject(&obj);
        checkErrorInfo(errCode);

        return obj;
    }

    void setValueObject(const TestValueObjectPtr& valueObject)
    {
        if (this->object == nullptr)
            throw InvalidParameterException();

        const auto errCode = this->object->setValueObject(valueObject);
        checkErrorInfo(errCode);
    }

    void setValueObjectProtected(const TestValueObjectPtr& valueObject)
    {
        if (this->object == nullptr)
            throw InvalidParameterException();

        const auto errCode = this->object->setValueObjectProtected(valueObject);
        checkErrorInfo(errCode);
    }
};

END_NAMESPACE_OPENDAQ
