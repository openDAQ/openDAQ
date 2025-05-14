/**
 * Properties application
 */

#include <opendaq/opendaq.h>
#include <iostream>

using namespace daq;

void print(FunctionBlockPtr fb)
{
    // Get all Properties
    auto properties = fb.getAllProperties();

    // Print all Function Block Properties
    std::cout << "\nFunction Block: " << fb.getName() << "\n";

    for (const auto& prop : properties)
    {
        auto dict = prop.getValue().asPtrOrNull<IDict>();
        if (dict.assigned())
        {
            std::cout << "  Property: " << prop.getName() << "\n";
            for (const auto& item : dict)
            {
                std::cout << "    Key: " << item.first << " Value: " << item.second << "\n";
            }
        }
        else
        {
            std::cout << "  Property: " << prop.getName() << " Value: " << prop.getValue() << "\n";
        }
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
    std::cout << "\nDuring modifications:\n";

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

    // Function
    FunctionPtr oldFun = fb.getPropertyValue("myPropFunction");
    auto res = oldFun(2, 3);
    std::cout << "Old function result (2 + 3): " << res << "\n";
    auto fun = Function(
        [](Int a, Int b)
        {
            std::cout << "New function called\n";
            return a * b;
        });
    fb.setPropertyValue("myPropFunction", fun);
    FunctionPtr newFun = fb.getPropertyValue("myPropFunction");
    auto newRes = newFun(2, 3);
    std::cout << "New function result (2 * 3): " << newRes << "\n";

    // Print after modifications
    std::cout << "\nAfter modifications:\n";
    print(fb);

    // Gracefully exit
    std::cout << "Press \"enter\" to exit the application...\n";
    std::cin.get();
    return 0;
}
