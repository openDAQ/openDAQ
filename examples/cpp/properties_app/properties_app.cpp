/**
 * Properties application
 */

#include <opendaq/opendaq.h>
#include <iostream>

using namespace daq;

void printProperty(PropertyPtr property, size_t indent = 0)
{
    std::cout << std::string(indent * 2, ' ')
    {
        std::cout << "  ";
    }
    std::cout << "Property: " << property.getName() << " ";
    auto content = property.getValue().asPtrOrNull<IPropertyObject>();
    if (content.assigned())
    {
        std::cout << "\n";

        for (const auto& prop : content.getAllProperties())
        {
            printProperty(prop, indent + 1);
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
    fb.setPropertyValue("myPropBool", true);

    // Int
    fb.setPropertyValue("myPropInt", 100);

    // Float
    fb.setPropertyValue("myPropFloat", 3.14);

    // String
    fb.setPropertyValue("myPropString", "Hello openDAQ");

    // Ratio
    fb.setPropertyValue("myPropRatio", Ratio(1, 2));

    // List
    auto list = List<IInteger>();
    list.pushBack(32);
    list.pushBack(64);
    fb.setPropertyValue("myPropList", list);

    // Dictionary
    auto dict = Dict<IString, IString>();
    dict["key1"] = "horse";
    dict["key2"] = "tired";
    fb.setPropertyValue("myPropDict", dict);

    // Struct
    auto manager = instance.getContext().getTypeManager();
    auto stru = StructBuilder("myStruct", manager).set("myInt", 100).set("myString", "openDAQ").build();
    fb.setPropertyValue("myPropStruct", stru);

    // Enumeration
    auto enumVal = Enumeration("myEnum", "third", manager);
    fb.setPropertyValue("myPropEnum", enumVal);

    // Procedure
    ProcedurePtr oldProc = fb.getPropertyValue("myPropProcedure");
    oldProc(42);
    auto proc = Procedure([](IntegerPtr a) { std::cout << "New procedure called with: " << a << "\n"; });
    fb.setPropertyValue("myPropProcedure", proc);
    ProcedurePtr newProc = fb.getPropertyValue("myPropProcedure");
    newProc(42);

    // Function
    FunctionPtr oldFun = fb.getPropertyValue("myPropFunction");
    auto res = oldFun(2, 3);
    std::cout << "Old function result (2 + 3): " << res << "\n";
    auto fun = Function(
        [](IntegerPtr a, IntegerPtr b)
        {
            std::cout << "New function called\n";
            return a * b;
        });
    fb.setPropertyValue("myPropFunction", fun);
    FunctionPtr newFun = fb.getPropertyValue("myPropFunction");
    auto newRes = newFun(2, 3);
    std::cout << "New function result (2 * 3): " << newRes << "\n";

    // Selection
    fb.setPropertyValue("myPropSelection", 2);

    // Sparse selection
    fb.setPropertyValue("myPropSparse", 6);

    // Object
    PropertyObjectPtr propObj = fb.getPropertyValue("myPropObject");
    fb.setPropertyValue("myPropObject.myPropInnerObject.myBool", True);
    fb.setPropertyValue("myPropObject.myInt", 987);
    fb.setPropertyValue("myPropObject.myFloat", 4.44);

    // Print after modifications
    std::cout << "\nAfter modifications:\n";
    print(fb);

    // Gracefully exit
    std::cout << "Press \"enter\" to exit the application...\n";
    std::cin.get();
    return 0;
}
