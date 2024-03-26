/*
 * Copyright 2022-2024 Blueberry d.o.o.
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
#include <coretypes/type_manager_factory.h>
#include <coreobjects/property_object_class_factory.h>
#include <test_value_object_factory.h>
#include <coreobjects/property_factory.h>

BEGIN_NAMESPACE_OPENDAQ

struct TestObjectPropertyClass
{
    static constexpr char ClassName[] = "TestObject";
    static constexpr char ClassParent[] = "\0";

    static constexpr char ValueObjectProp[] = "ValueObject";
};

class TestObjectPropertyClassRegistrator : public TestObjectPropertyClass
{
public:
    TestObjectPropertyClassRegistrator()
    {
        manager = TypeManager();

        auto prop = PropertyBuilder(ValueObjectProp)
                    .setValueType(ctObject)
                    .setDefaultValue(TestValueObject(0))
                    .build();

        auto propClassBuilder = PropertyObjectClassBuilder(ClassName).addProperty(prop);
        if (strlen(ClassParent) > 0)
        {
            propClassBuilder.setParentName(ClassParent);
        }

        manager.addType(propClassBuilder.build());

    }

    ~TestObjectPropertyClassRegistrator()
    {
        try
        {
            manager.removeType(ClassName);
        }
        catch (...)
        {
        }
    }

    TypeManagerPtr manager;
};

END_NAMESPACE_OPENDAQ
