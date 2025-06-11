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

    // TODO: kind of hacky
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
    std::cout << "\nFunction Block: " << fb.getName() << "\n";
    for (const auto& prop : fb.getAllProperties())
    {
        printProperty(prop);
    }
    std::cout << "\n";
}

int main(int /*argc*/, const char* /*argv*/[])
{
    // Create an Instance, loading modules in the default module path
    const InstancePtr instance = Instance();

    // PROPERTIES FUNCTION BLOCK 1 DEMO
    {
        // Add Function Block by type ID
        auto fb1 = instance.addFunctionBlock("PropertiesFb1");

        // Print before modifications
        std::cout << "\nFB1 before modifications:\n";
        print(fb1);

        // Bool
        fb1.setPropertyValue("Bool", true);

        // Int
        fb1.setPropertyValue("Int", 100);

        // Float
        fb1.setPropertyValue("Float", 3.14);

        // String
        fb1.setPropertyValue("String", "Hello openDAQ");

        // Ratio
        fb1.setPropertyValue("Ratio", Ratio(1, 2));

        // Min and max Float
        fb1.setPropertyValue("MinMaxProp", 101.1);  // Clamped to 100.0
        fb1.setPropertyValue("MinMaxProp", -1.1);   // Clamped to 0.0
        fb1.setPropertyValue("MinMaxProp", 50.1);   // Within range

        // Suggested values Float
        auto suggested = fb1.getProperty("SuggestedProp").getSuggestedValues();
        auto selected = suggested[2].asPtrOrNull<IFloat>();
        fb1.setPropertyValue("SuggestedProp", selected);  // Will set to 3.3, because that's the third suggested value in the module

        // Stubborn Int
        fb1.setPropertyValue("StubbornInt", 41);  // Will be forced to 43

        // Print after modifications
        std::cout << "\nFB1 after modifications:\n";
        print(fb1);

        // Register callback for single property read
        fb1.getOnPropertyValueRead("Bool") += [](PropertyObjectPtr&, const PropertyValueEventArgsPtr&) { std::cout << "Bool read\n"; };
        fb1.getOnPropertyValueWrite("Int") += [](PropertyObjectPtr&, const PropertyValueEventArgsPtr& args)
        { std::cout << "Int written with value: " << args.getValue() << "\n"; };

        // Register callback for any property read/writes
        fb1.getOnAnyPropertyValueRead() += [](PropertyObjectPtr&, const PropertyValueEventArgsPtr&) { std::cout << "Something read\n"; };
        fb1.getOnAnyPropertyValueWrite() +=
            [](PropertyObjectPtr&, const PropertyValueEventArgsPtr&) { std::cout << "Something written\n"; };

        // Test callbacks
        auto dummyBool = fb1.getPropertyValue("Bool");
        fb1.setPropertyValue("Int", 3);
    }

    // PROPERTIES FUNCTION BLOCK 2 DEMO
    {
        // Add Function Block by type ID
        auto fb2 = instance.addFunctionBlock("PropertiesFb2");

        // Print before modifications
        std::cout << "\nFB2 before modifications:\n";
        print(fb2);

        // List
        auto list = List<IInteger>();
        list.pushBack(32);
        list.pushBack(64);
        fb2.setPropertyValue("List", list);
        std::cout << "Second element in list: " << fb2.getPropertyValue("List[1]") << "\n";

        // Dictionary
        auto dict = Dict<IString, IString>();
        dict["key1"] = "Horse";
        dict["key2"] = "Tired";
        fb2.setPropertyValue("Dict", dict);

        // Struct
        auto manager = instance.getContext().getTypeManager();
        auto stru = StructBuilder("Struct", manager).set("Int", 100).set("String", "openDAQ").build();
        fb2.setPropertyValue("Struct", stru);

        // Struct modification via builder
        StructPtr struMod = fb2.getPropertyValue("Struct");
        auto structBuild = StructBuilder(struMod).set("Int", 200).set("String", "openDAQ modified").build();
        fb2.setPropertyValue("Struct", structBuild);

        // Enumeration
        auto enumVal = Enumeration("Enum", "Third", manager);
        fb2.setPropertyValue("Enum", enumVal);

        // Procedure
        ProcedurePtr oldProc = fb2.getPropertyValue("Procedure");
        oldProc(42);
        auto proc = Procedure([](IntegerPtr a) { std::cout << "New procedure called with: " << a << "\n"; });
        fb2.setPropertyValue("Procedure", proc);
        ProcedurePtr newProc = fb2.getPropertyValue("Procedure");
        newProc(42);

        // Function
        FunctionPtr oldFun = fb2.getPropertyValue("FunctionObject.Function");
        auto res = oldFun(2, 3);
        std::cout << "Old function result (2 + 3): " << res << "\n";
        auto fun = Function(
            [](IntegerPtr a, IntegerPtr b)
            {
                std::cout << "New function called\n";
                return a * b;
            });
        fb2.setPropertyValue("FunctionObject.Function", fun);
        FunctionPtr newFun = fb2.getPropertyValue("FunctionObject.Function");
        auto newRes = newFun(2, 3);
        std::cout << "New function result (2 * 3): " << newRes << "\n";

        // Selection
        fb2.setPropertyValue("Selection", 2);

        // Sparse selection
        fb2.setPropertyValue("Sparse", 6);

        // Object
        fb2.setPropertyValue("Object.InnerObject.Bool", True);
        fb2.setPropertyValue("Object.Int", 987);
        fb2.setPropertyValue("Object.Float", 4.44);

        // Print after modifications
        std::cout << "\nFB2 after modifications:\n";
        print(fb2);
    }
    // PROPERTIES FUNCTION BLOCK 4 DEMO
    {
        // Add Function Block by type ID
        auto fb4 = instance.addFunctionBlock("PropertiesFb4");

        // Print before modifications
        std::cout << "\nFB4 before modifications:\n";
        print(fb4);

        // Property visibility depending on another Property
        fb4.setPropertyValue("SometimesVisible", 2);

        // Referenced and reference Bool
        fb4.setPropertyValue("Reference", True);

        // Check if Properties are referenced
        std::cout << "Referenced is referenced: " << Boolean(fb4.getProperty("Referenced").getIsReferenced()) << "\n";
        std::cout << "Reference is referenced: " << Boolean(fb4.getProperty("Reference").getIsReferenced()) << "\n";

        // Read-only Int
        try
        {
            fb4.setPropertyValue("ReadOnlyInt", 42);
        }
        catch (const std::exception& e)
        {
            std::cout << "Exception: " << e.what() << "\n";
        }

        // Coerced Int
        fb4.setPropertyValue("CoercedProp", 4);    // No coercion
        fb4.setPropertyValue("CoercedProp", 142);  // Coerced to 10

        // Validated Int
        fb4.setPropertyValue("ValidatedProp", 43);  // Valid
        try
        {
            fb4.setPropertyValue("ValidatedProp", 1000);  // Fails validation
        }
        catch (const std::exception& e)
        {
            std::cout << "Exception: " << e.what() << "\n";
        }

        // Print after modifications
        std::cout << "\nFB4 after modifications:\n";
        print(fb4);
    }

    // Gracefully exit
    std::cout << "Press \"enter\" to exit the application...\n";
    std::cin.get();
    return 0;
}
