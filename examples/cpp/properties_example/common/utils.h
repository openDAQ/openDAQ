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
#include <coretypes/coretype_utils.h>
#include <opendaq/function_block_impl.h>
#include <opendaq/opendaq.h>
#include <iostream>

enum PropertyType
{
    ptObject,
    ptDict,
    ptBinaryData,
    ptBool,
    ptComplexNumber,
    ptEnumeration,
    ptFloat,
    ptInt,
    ptSelection,
    ptSparseSelection,
    ptList,
    ptProcedure,
    ptFunc,
    ptRatio,
    ptString,
    ptStruct,
    ptUnknown
};

inline void printMetadata(const daq::BaseObjectPtr& obj, const daq::StringPtr& name, size_t indent)
{
    std::cout << std::string(indent * 2, ' ') << name << ": " << obj << "\n";
}

inline void printProperty(const daq::PropertyPtr& property, size_t indent = 0)
{
    std::cout << "\n"
              << std::string(indent * 2, ' ') << "******************\n"
              << std::string(indent * 2, ' ') << "* START PROPERTY *\n"
              << std::string(indent * 2, ' ') << "******************\n\n";

    printMetadata(property.getName(), "Name", indent);
    printMetadata(daq::coretype_utils::coreTypeToString(property.getValueType()), "Value Type", indent + 1);
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

    std::cout << "\n"
              << std::string(indent * 2, ' ') << "****************\n"
              << std::string(indent * 2, ' ') << "* END PROPERTY *\n"
              << std::string(indent * 2, ' ') << "****************\n\n";
}

inline void printFBProperties(const daq::FunctionBlockPtr& fb)
{
    std::cout << "\nFunction Block: " << fb.getName() << "\n";
    for (const auto& prop : fb.getAllProperties())
    {
        printProperty(prop);
    }
    std::cout << "\n";
}

inline PropertyType getPropertyType(const daq::PropertyPtr& property)
{
    // Determine the type of the property based on its core type
    switch (property.getValue().getCoreType())
    {
        case daq::CoreType::ctObject:
            return ptObject;
        case daq::CoreType::ctDict:
            return ptDict;
        case daq::CoreType::ctBinaryData:
            return ptBinaryData;
        case daq::CoreType::ctBool:
            return ptBool;
        case daq::CoreType::ctComplexNumber:
            return ptComplexNumber;
        case daq::CoreType::ctEnumeration:
            return ptEnumeration;
        case daq::CoreType::ctFloat:
            return ptFloat;
        case daq::CoreType::ctInt:
        {
            // Check if the property has selection values
            auto vals = property.getSelectionValues();
            if (vals.assigned())
            {
                // If it does have selection values, determine the type based on the core type of those values
                if (vals.getCoreType() == daq::CoreType::ctList)
                {
                    // If the selection values are a list, return Selection
                    return ptSelection;
                }
                if (vals.getCoreType() == daq::CoreType::ctDict)
                {
                    // If the selection values are a dictionary, return SparseSelection
                    return ptSparseSelection;
                }
            }
            // If no selection values, return Int
            return ptInt;
        }
        case daq::CoreType::ctList:
            return ptList;
        case daq::CoreType::ctProc:
            return ptProcedure;
        case daq::CoreType::ctFunc:
            return ptFunc;
        case daq::CoreType::ctRatio:
            return ptRatio;
        case daq::CoreType::ctString:
            return ptString;
        case daq::CoreType::ctStruct:
            return ptStruct;
        default:
            return ptUnknown;
    }
}

// Demonstrates how to configure a List property
inline void configureListProperty(const daq::PropertyObjectPtr& propObject)
{
    std::cout << "Configuring List property...\n";
    auto prop = propObject.getProperty("List");

    std::cout << "Property value type: " << daq::coretype_utils::coreTypeToString(prop.getValueType()) << "\n";
    std::cout << "List item type: " << daq::coretype_utils::coreTypeToString(prop.getItemType()) << "\n";

    daq::ListPtr<daq::IInteger> currentValues = propObject.getPropertyValue("List");
    std::cout << "Current List values: " << currentValues << "\n";

    // New value
    auto list = daq::List<daq::IInteger>();
    list.pushBack(32);
    list.pushBack(64);
    propObject.setPropertyValue("List", list);

    std::cout << "Updated List values: " << propObject.getPropertyValue("List") << "\n";

    // List properties can also be accessed by index using the syntax "List[index]" in the property name argument
    std::cout << "Second element in updated list: " << propObject.getPropertyValue("List[1]") << "\n\n";
}

// Demonstrates how to configure a Dict property
inline void configureDictProperty(const daq::PropertyObjectPtr& propObject)
{
    std::cout << "Configuring Dict property...\n";
    auto prop = propObject.getProperty("Dict");

    auto valueType = prop.getValueType();
    std::cout << "Property value type: " << daq::coretype_utils::coreTypeToString(valueType) << "\n";

    auto keyType = prop.getKeyType();
    std::cout << "Dict key type: " << daq::coretype_utils::coreTypeToString(keyType) << "\n";

    auto itemType = prop.getItemType();
    std::cout << "Dict item type: " << daq::coretype_utils::coreTypeToString(itemType) << "\n";

    auto currentDict = propObject.getProperty("Dict");
    std::cout << "Current Dict values:\n";
    printProperty(currentDict);

    // New value
    auto dict = daq::Dict<daq::IString, daq::IString>();
    dict["key1"] = "Horse";
    dict["key2"] = "Excited";
    propObject.setPropertyValue("Dict", dict);

    auto newDict = propObject.getProperty("Dict");
    std::cout << "New Dict values:\n";
    printProperty(newDict);
}

// Demonstrates how to configure a Struct property
inline void configureStructProperty(const daq::PropertyObjectPtr& propObject, const daq::TypeManagerPtr& manager)
{
    std::cout << "Configuring Struct property...\n";
    auto prop = propObject.getProperty("Struct");

    printProperty(prop);

    // New value (requires the Type Manager which stores the possible types)
    auto stru = StructBuilder("Struct", manager).set("Int", 100).set("String", "openDAQ").build();
    propObject.setPropertyValue("Struct", stru);

    std::cout << "Updated Struct values: " << "\n";
    auto newStruct = propObject.getProperty("Struct");
    printProperty(newStruct);
}

// Demos how to modify a Struct property using an alternative overload on the builder
inline void reconfigureStructProperty(const daq::PropertyObjectPtr& propObject)
{
    std::cout << "Reconfiguring Struct property...\n";

    // This overload is used to modify an existing Struct property (the Type Manager is not required this time)
    auto struMod = propObject.getPropertyValue("Struct");
    auto stru = StructBuilder(struMod).set("Int", 200).set("String", "openDAQ modified").build();
    propObject.setPropertyValue("Struct", stru);

    auto newProp = propObject.getPropertyValue("Struct");

    std::cout << "Updated Struct values: " << "\n";
    printProperty(propObject.getProperty("Struct"));
}

inline void configureEnumProperty(const daq::PropertyObjectPtr& propObject, const daq::TypeManagerPtr& manager)
{
    std::cout << "Current Enum value: " << "\n";
    auto value = propObject.getProperty("Enum");
    printProperty(value);

    std::cout << "Configuring Enum property...\n";
    auto enumVal = Enumeration("Enum", "Third", manager);
    propObject.setPropertyValue("Enum", enumVal);

    std::cout << "New Enum value: " << "\n";
    auto newValue = propObject.getProperty("Enum");
    printProperty(newValue);
}

// Demos how to modify a basic property
inline void configureBasicProperty(const daq::PropertyObjectPtr& propObject,
                                   const daq::StringPtr& propName,
                                   const daq::BaseObjectPtr& newValue)
{
    // Get Property by name
    auto property = propObject.getProperty(propName);

    // Print old Property value
    std::cout << "Before setting value of '" << propName << "':\n";
    printProperty(property);

    // Configure a basic Property with a new value
    std::cout << "Configuring a basic Property...\n\n";
    property.setValue(newValue);

    // Print new Property value
    std::cout << "After setting value of '" << propName << "':\n";
    printProperty(property);
}
