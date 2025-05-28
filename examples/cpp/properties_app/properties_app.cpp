/**
 * Properties application
 * (Uses properties_module)
 */

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

    // TODO: get CoreType instead of casting?
    auto propObj = property.getValue().asPtrOrNull<IPropertyObject>();
    auto dictObj = property.getValue().asPtrOrNull<IDict>();
    if (propObj.assigned())
    {
        for (const auto& prop : propObj.getAllProperties())
        {
            printProperty(prop, indent + 1);
        }
    }
    else if (dictObj.assigned())
    {
        for (const auto& [key, value] : dictObj)
        {
            std::cout << std::string(indent * 2, ' ') << "  Key:" << key << " Value: " << value << "\n";
        }
    }
    else
    {
        std::cout << std::string(indent * 2, ' ') << "  Value: " << property.getValue() << "\n";
    }
}

void print(const FunctionBlockPtr& fb)
{
    // Get all Properties
    auto properties = fb.getAllProperties();

    // Print all Function Block Properties
    std::cout << "\nFunction Block: " << fb.getName() << "\n";

    for (const auto& prop : properties)
    {
        printProperty(prop);
    }

    std::cout << "\n";
}

int main(int /*argc*/, const char* /*argv*/[])
{
    // Create an Instance, loading modules at MODULE_PATH
    const InstancePtr instance = Instance(MODULE_PATH);

    auto fbTypes = instance.getAvailableFunctionBlockTypes();

    // Add Function Block by type ID
    auto fb = instance.addFunctionBlock("PropertiesFb");

    // Print before modifications
    std::cout << "\nBefore modifications:\n";
    print(fb);

    // Make modifications
    std::cout << "\nDuring setting property values:\n\n";

    // Bool
    fb.setPropertyValue("Bool", true);

    // Int
    fb.setPropertyValue("Int", 100);

    // Float
    fb.setPropertyValue("Float", 3.14);

    // String
    fb.setPropertyValue("String", "Hello openDAQ");

    // Ratio
    fb.setPropertyValue("Ratio", Ratio(1, 2));

    // List
    auto list = List<IInteger>();
    list.pushBack(32);
    list.pushBack(64);
    fb.setPropertyValue("List", list);

    // Dictionary
    auto dict = Dict<IString, IString>();
    dict["key1"] = "Horse";
    dict["key2"] = "Tired";
    fb.setPropertyValue("Dict", dict);

    // Struct
    auto manager = instance.getContext().getTypeManager();
    auto stru = StructBuilder("Struct", manager).set("Int", 100).set("String", "openDAQ").build();
    fb.setPropertyValue("Struct", stru);

    // Struct modification via builder
    StructPtr struMod = fb.getPropertyValue("Struct");
    auto structBuild = StructBuilder(struMod).set("Int", 200).set("String", "openDAQ modified").build();
    fb.setPropertyValue("Struct", structBuild);

    // Enumeration
    auto enumVal = Enumeration("Enum", "Third", manager);
    fb.setPropertyValue("Enum", enumVal);

    // Procedure
    ProcedurePtr oldProc = fb.getPropertyValue("Procedure");
    oldProc(42);
    auto proc = Procedure([](IntegerPtr a) { std::cout << "New procedure called with: " << a << "\n"; });
    fb.setPropertyValue("Procedure", proc);
    ProcedurePtr newProc = fb.getPropertyValue("Procedure");
    newProc(42);

    // Function
    FunctionPtr oldFun = fb.getPropertyValue("Function");
    auto res = oldFun(2, 3);
    std::cout << "Old function result (2 + 3): " << res << "\n";
    auto fun = Function(
        [](IntegerPtr a, IntegerPtr b)
        {
            std::cout << "New function called\n";
            return a * b;
        });
    fb.setPropertyValue("Function", fun);
    FunctionPtr newFun = fb.getPropertyValue("Function");
    auto newRes = newFun(2, 3);
    std::cout << "New function result (2 * 3): " << newRes << "\n";

    // Selection
    fb.setPropertyValue("Selection", 2);

    // Sparse selection
    fb.setPropertyValue("Sparse", 6);

    // Object
    PropertyObjectPtr propObj = fb.getPropertyValue("Object");
    fb.setPropertyValue("Object.InnerObject.Bool", True);
    fb.setPropertyValue("Object.Int", 987);
    fb.setPropertyValue("Object.Float", 4.44);

    // Property visibility depending on another Property
    fb.setPropertyValue("SometimesVisible", 2);

    // Referenced and reference Bool
    fb.setPropertyValue("Reference", True);

    // Check if Properties are referenced
    std::cout << "Referenced is referenced: " << Boolean(fb.getProperty("Referenced").getIsReferenced()) << "\n";
    std::cout << "Reference is referenced: " << Boolean(fb.getProperty("Reference").getIsReferenced()) << "\n";

    // Stubborn Int
    fb.setPropertyValue("StubbornInt", 41);  // Will actually set the value to 43, due to getOnPropertyValueWrite callback in module

    // Read-only Int
    try
    {
        fb.setPropertyValue("ReadOnlyInt", 42);
    }
    catch (const std::exception& e)
    {
        std::cout << "Exception: " << e.what() << "\n";
    }

    // Coerced Int
    fb.setPropertyValue("CoercedProp", 4);    // Will set to 4, no coercion
    fb.setPropertyValue("CoercedProp", 142);  // Will set to 10, due to coercion

    // Validated Int
    fb.setPropertyValue("ValidatedProp", 43);  // Will set to 43
    try
    {
        fb.setPropertyValue("ValidatedProp", 1000);  // Will fail, due to validation
    }
    catch (const std::exception& e)
    {
        std::cout << "Exception: " << e.what() << "\n";
    }

    // Min and max Float
    fb.setPropertyValue("MinMaxProp", 101.1);  // Will set to 100.0
    fb.setPropertyValue("MinMaxProp", -1.1);   // Will set to 0.0
    fb.setPropertyValue("MinMaxProp", 50.1);   // Will set to 50.1

    // Suggested values Float
    auto suggested = fb.getProperty("SuggestedProp").getSuggestedValues();
    auto selected = suggested[2].asPtrOrNull<IFloat>();
    fb.setPropertyValue("SuggestedProp", selected);  // Will set to 3.3, because that's the third suggested value in the module

    // Print after modifications
    std::cout << "\nAfter modifications:\n";
    print(fb);

    // Register callback for single property read
    auto boolProp = fb.getProperty("Bool");
    fb.getOnPropertyValueRead("Bool") +=
        [](PropertyObjectPtr& /*obj*/, const PropertyValueEventArgsPtr& /*args*/) { std::cout << "Bool read\n"; };

    // Register callback for any property read/writes
    fb.getOnAnyPropertyValueRead() +=
        [](PropertyObjectPtr& /*obj*/, const PropertyValueEventArgsPtr& /*args*/) { std::cout << "Something read\n"; };
    fb.getOnAnyPropertyValueWrite() +=
        [](PropertyObjectPtr& /*obj*/, const PropertyValueEventArgsPtr& /*args*/) { std::cout << "Something written\n"; };

    // Test the previously registered callbacks
    auto dummyBool = fb.getPropertyValue("Bool");
    fb.setPropertyValue("Int", 3);

    // Gracefully exit
    std::cout << "Press \"enter\" to exit the application...\n";
    std::cin.get();
    return 0;
}
