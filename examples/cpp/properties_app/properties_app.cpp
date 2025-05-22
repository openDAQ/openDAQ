/**
 * Properties application
 * (uses properties_module)
 */

#include <opendaq/opendaq.h>
#include <iostream>

using namespace daq;

void printProperty(PropertyPtr property, size_t indent = 0)
{
    std::cout << std::string(indent * 2, ' ');
    std::cout << "Property: " << property.getName() << " ";
    auto propObj = property.getValue().asPtrOrNull<IPropertyObject>();
    auto dictObj = property.getValue().asPtrOrNull<IDict>();
    if (propObj.assigned())
    {
        std::cout << "\n";

        for (const auto& prop : propObj.getAllProperties())
        {
            printProperty(prop, indent + 1);
        }
    }
    else if (dictObj.assigned())
    {
        std::cout << "\n";

        for (const auto& [key, value] : dictObj)
        {
            std::cout << "  " << key << ": " << value << "\n";
        }
    }
    else
    {
        std::cout << property.getValue() << "\n";
    }
}

void print(FunctionBlockPtr fb)
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
    std::cout << "\nDuring setting property values:\n";

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

    // Print after modifications
    std::cout << "\nAfter modifications:\n";
    print(fb);

    // Gracefully exit
    std::cout << "Press \"enter\" to exit the application...\n";
    std::cin.get();
    return 0;
}
