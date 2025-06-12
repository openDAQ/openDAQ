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

#include <opendaq/function_block_impl.h>
#include <opendaq/opendaq.h>
#include <iostream>

using namespace daq;

StringPtr coreTypeToString(const CoreType& coreType)
{
    switch (coreType)
    {
        case ctBool:
            return "Bool";
        case ctInt:
            return "Int";
        case ctFloat:
            return "Float";
        case ctString:
            return "String";
        case ctList:
            return "List";
        case ctDict:
            return "Dict";
        case ctRatio:
            return "Ratio";
        case ctProc:
            return "Proc";
        case ctObject:
            return "Object";
        case ctBinaryData:
            return "BinaryData";
        case ctFunc:
            return "Func";
        case ctComplexNumber:
            return "ComplexNumber";
        case ctStruct:
            return "Struct";
        case ctEnumeration:
            return "Enumeration";
        case ctUndefined:
            return "Undefined";
    }
    return "";
}

void printMetadata(const BaseObjectPtr& obj, const StringPtr& name, const size_t& indent)
{
    std::cout << std::string(indent * 2, ' ') << name << ": " << obj << "\n";
}

void printProperty(const PropertyPtr& property, const size_t& indent = 0)
{
    printMetadata(property.getName(), "Name", indent);
    printMetadata(coreTypeToString(property.getValueType()), "Value Type", indent + 1);
    printMetadata(property.getDescription(), "Description", indent + 1);
    printMetadata(property.getDefaultValue(), "Default Value", indent + 1);
    printMetadata(Boolean(property.getReadOnly()), "Read Only", indent + 1);
    printMetadata(Boolean(property.getVisible()), "Visible", indent + 1);
    printMetadata(property.getUnit(), "Unit", indent + 1);

    if (property.getValue().getCoreType() == CoreType::ctObject)
    {
        for (const auto& prop : property.getValue().asPtrOrNull<IPropertyObject>().getAllProperties())
        {
            printProperty(prop, indent + 1);
        }
    }
    else if (property.getValue().getCoreType() == CoreType::ctDict)
    {
        for (const auto& [key, value] : property.getValue().asPtrOrNull<IDict>())
        {
            std::cout << std::string(indent * 2, ' ') << "  Key: " << key << " Value: " << value << "\n";
        }
    }
    else
    {
        std::cout << std::string(indent * 2, ' ') << "  Value: " << property.getValue() << "\n";
    }
}

void print(const FunctionBlockPtr& fb)
{
    std::cout << "\nFunction Block: " << fb.getName() << "\n";
    for (const auto& prop : fb.getAllProperties())
    {
        printProperty(prop);
    }
    std::cout << "\n";
}
