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

#include <coreobjects/callable_info_factory.h>
#include <coretypes/coretype.h>
#include <opendaq/function_block_impl.h>
#include <opendaq/opendaq.h>
#include <iostream>

inline daq::StringPtr coreTypeToString(const daq::CoreType& coreType)
{
    switch (coreType)
    {
        case daq::CoreType::ctBool:
            return "Bool";
        case daq::CoreType::ctInt:
            return "Int";
        case daq::CoreType::ctFloat:
            return "Float";
        case daq::CoreType::ctString:
            return "String";
        case daq::CoreType::ctList:
            return "List";
        case daq::CoreType::ctDict:
            return "Dict";
        case daq::CoreType::ctRatio:
            return "Ratio";
        case daq::CoreType::ctProc:
            return "Proc";
        case daq::CoreType::ctObject:
            return "Object";
        case daq::CoreType::ctBinaryData:
            return "BinaryData";
        case daq::CoreType::ctFunc:
            return "Func";
        case daq::CoreType::ctComplexNumber:
            return "ComplexNumber";
        case daq::CoreType::ctStruct:
            return "Struct";
        case daq::CoreType::ctEnumeration:
            return "Enumeration";
        case daq::CoreType::ctUndefined:
            return "Undefined";
    }
    return "Unknown";
}

inline void printMetadata(const daq::BaseObjectPtr& obj, const daq::StringPtr& name, size_t indent)
{
    std::cout << std::string(indent * 2, ' ') << name << ": " << obj << "\n";
}

inline void printProperty(const daq::PropertyPtr& property, size_t indent = 0)
{
    printMetadata(property.getName(), "Name", indent);
    printMetadata(coreTypeToString(property.getValueType()), "Value Type", indent + 1);
    printMetadata(property.getDescription(), "Description", indent + 1);
    printMetadata(property.getDefaultValue(), "Default Value", indent + 1);
    printMetadata(daq::Boolean(property.getReadOnly()), "Read Only", indent + 1);
    printMetadata(daq::Boolean(property.getVisible()), "Visible", indent + 1);
    printMetadata(property.getUnit(), "Unit", indent + 1);

    auto value = property.getValue();
    if (value.getCoreType() == daq::CoreType::ctObject)
    {
        auto objPtr = value.asPtrOrNull<daq::IPropertyObject>();
        if (objPtr.assigned())
        {
            for (const auto& prop : objPtr.getAllProperties())
            {
                printProperty(prop, indent + 1);
            }
        }
    }
    else if (value.getCoreType() == daq::CoreType::ctDict)
    {
        auto dictPtr = value.asPtrOrNull<daq::IDict>();
        if (dictPtr.assigned())
        {
            for (const auto& [key, val] : dictPtr)
            {
                std::cout << std::string(indent * 2, ' ') << "  Key: " << key << " Value: " << val << "\n";
            }
        }
    }
    else
    {
        std::cout << std::string(indent * 2, ' ') << "  Value: " << value << "\n";
    }
}

inline void print(const daq::FunctionBlockPtr& fb)
{
    std::cout << "\nFunction Block: " << fb.getName() << "\n";
    for (const auto& prop : fb.getAllProperties())
    {
        printProperty(prop);
    }
    std::cout << "\n";
}

inline void modifyProperty(const daq::PropertyPtr& property, const daq::TypeManagerPtr typeManager)
{
    auto value = property.getValue();
    switch (value.getCoreType())
    {
        case daq::CoreType::ctObject:
        {
            auto objPtr = value.asPtrOrNull<daq::IPropertyObject>();
            if (objPtr.assigned())
            {
                for (const auto& prop : objPtr.getAllProperties())
                {
                    modifyProperty(prop, typeManager);
                }
            }
            break;
        }
        case daq::CoreType::ctDict:
        {
            auto dictPtr = value.asPtrOrNull<daq::IDict>();
            if (dictPtr.assigned())
            {
                auto newDict = daq::Dict<daq::IString, daq::IString>();
                newDict.set("Key1", "Milk");
                newDict.set("Key2", "Cow");
                property.setValue(newDict);
            }
            break;
        }
        case daq::CoreType::ctBinaryData:
        {
            // Example: Create a new binary data with a simple byte array
            auto newBinaryData = daq::BinaryData(42);
            property.setValue(newBinaryData);
            break;
        }
        case daq::CoreType::ctBool:
        {
            property.setValue(!value);
            break;
        }
        case daq::CoreType::ctComplexNumber:
        {
            auto complex = value.asPtrOrNull<daq::IComplexNumber>();
            if (complex.assigned())
            {
                property.setValue(daq::ComplexNumber(complex.getImaginary(), complex.getReal()));
            }
            break;
        }
        case daq::CoreType::ctEnumeration:
        {
            auto enu = Enumeration("Enum", "Second", typeManager);
            property.setValue(enu);
            break;
        }
        case daq::CoreType::ctFloat:
        {
            property.setValue(value + 1.5);
            break;
        }
        case daq::CoreType::ctInt:
        {
            // Handle selection and sparse selection properties
            auto vals = property.getSelectionValues();
            if (vals.assigned() && (vals.getCoreType() == daq::CoreType::ctList || vals.getCoreType() == daq::CoreType::ctDict))
            {
                property.setValue(0);  // Select first item as an example
            }
            else
            {
                if (!property.getReadOnly())  // Only modify if not read-only
                    property.setValue(42);    // Set to a specific integer value
            }
            break;
        }
        case daq::CoreType::ctList:
        {
            auto list = daq::List<daq::IntegerPtr>();
            list.pushBack(33);
            list.pushBack(44);
            list.pushBack(55);
            property.setValue(list);
            break;
        }
        case daq::CoreType::ctProc:
        {
            auto newProc = daq::Procedure([](daq::IntegerPtr a) { std::cout << "Newest procedure called with: " << a << "\n"; });
            property.setValue(newProc);
            break;
        }
        case daq::CoreType::ctFunc:
        {
            auto newFunc = daq::Function(
                [](const daq::IntegerPtr& a, const daq::IntegerPtr& b) -> daq::IntegerPtr
                {
                    std::cout << "Newest function called with: " << a << " and " << b << "\n";
                    return daq::Integer(10) * a + b;
                });
            property.setValue(newFunc);
            break;
        }
        case daq::CoreType::ctRatio:
        {
            auto ratio = value.asPtrOrNull<daq::IRatio>();
            if (ratio.assigned())
            {
                property.setValue(daq::Ratio(ratio.getDenominator(), ratio.getNumerator()));
            }
            break;
        }
        case daq::CoreType::ctString:
        {
            property.setValue(daq::String("Modified String"));
            break;
        }
        case daq::CoreType::ctStruct:
        {
            auto structType = property.getStructType();
            if (structType.assigned())
            {
                // Create a new Struct instance using the struct type
                auto newStruct = StructBuilder(property.getValue());
                auto fieldNames = structType.getFieldNames();
                auto fieldTypes = structType.getFieldTypes();
                auto nameIt = fieldNames.begin();
                auto typeIt = fieldTypes.begin();
                for (; nameIt != fieldNames.end() && typeIt != fieldTypes.end(); ++nameIt, ++typeIt)
                {
                    if ((*typeIt).getCoreType() == daq::CoreType::ctString)
                    {
                        newStruct.set(*nameIt, daq::String("Twice modified"));
                    };
                    // TODO: Handle other types of fields if necessary, using *typeIt
                }
                property.setValue(newStruct.build());
            }
            break;
        }
        default:
            std::cout << "Unhandled property type: " << coreTypeToString(value.getCoreType()) << "\n";
            break;
    }
}

inline void modify(const daq::PropertyObjectPtr& propertyObject, daq::TypeManagerPtr typeManager)
{
    for (const auto& property : propertyObject.getAllProperties())
    {
        auto value = property.getValue();
        if (value.getCoreType() == daq::CoreType::ctObject)
        {
            auto objPtr = value.asPtrOrNull<daq::IPropertyObject>();
            if (objPtr.assigned())
            {
                for (const auto& innerProperty : objPtr.getAllProperties())
                {
                    modifyProperty(innerProperty, typeManager);
                }
            }
        }
        else
        {
            modifyProperty(property, typeManager);
        }
    }
}
