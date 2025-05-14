/**
 * Properties application
 */

#include <opendaq/opendaq.h>
#include <iostream>

using namespace daq;

void print(FunctionBlockPtr fb)
{
    std::cout << "Bool: " << fb.getPropertyValue("myPropBool") << "\n";
    std::cout << "Int: " << fb.getPropertyValue("myPropInt") << "\n";
    std::cout << "Float: " << fb.getPropertyValue("myPropFloat") << "\n";
    std::cout << "String: " << fb.getPropertyValue("myPropString") << "\n";
    std::cout << "Ratio: " << fb.getPropertyValue("myPropRatio") << "\n";
    std::cout << "List: " << fb.getPropertyValue("myPropList") << "\n";
    std::cout << "Dict: " << "\n";
    DictPtr<IString, IString> dict = fb.getPropertyValue("myPropDict");
    for (const auto& item : dict)
    {
        std::cout << "  " << item.first << ": " << item.second << "\n";
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

    // Print Function Block name
    std::cout << "\nFunction Block: " << fb.getName() << "\n";

    // Get all properties
    auto properties = fb.getAllProperties();

    // Print all properties
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

    // Print before modifications
    std::cout << "Before modifications:\n";
    print(fb);

    // Make modifications
    fb.setPropertyValue("myPropBool", true);
    fb.setPropertyValue("myPropInt", 100);
    fb.setPropertyValue("myPropFloat", 3.14);
    fb.setPropertyValue("myPropString", "Hello openDAQ");
    fb.setPropertyValue("myPropRatio", Ratio(1, 2));
    auto list = List<IInteger>();
    list.pushBack(32);
    list.pushBack(64);
    fb.setPropertyValue("myPropList", list);
    auto dict = Dict<IString, IString>();
    dict["key1"] = "horse";
    dict["key2"] = "tired";
    fb.setPropertyValue("myPropDict", dict);

    // Print after modifications
    std::cout << "\nAfter modifications:\n";
    print(fb);

    // Gracefully exit
    std::cout << "Press \"enter\" to exit the application...\n";
    std::cin.get();
    return 0;
}
