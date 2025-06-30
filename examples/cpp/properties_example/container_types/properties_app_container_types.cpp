/*
 * Container types and selection properties
 */

#include <utils.h>

using namespace daq;

int main(int /*argc*/, const char* /*argv*/[])
{
    // Create an Instance, loading modules in the default module path
    const InstancePtr instance = Instance();

    // Add Function Block by type ID
    auto fb = instance.addFunctionBlock("ExampleFBPropertyContainerTypes");

    // Print before modifications
    std::cout << "\nFB2 before modifications:\n";
    print(fb);

    // List
    auto list = List<IInteger>();
    list.pushBack(32);
    list.pushBack(64);
    fb.setPropertyValue("List", list);
    std::cout << "Second element in list: " << fb.getPropertyValue("List[1]") << "\n";

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
    auto struMod = fb.getPropertyValue("Struct");
    auto structBuild = StructBuilder(struMod).set("Int", 200).set("String", "openDAQ modified").build();
    fb.setPropertyValue("Struct", structBuild);

    // Enumeration
    auto enumVal = Enumeration("Enum", "Third", manager);
    fb.setPropertyValue("Enum", enumVal);

    // Selection
    fb.setPropertyValue("Selection", 2);

    // Sparse selection
    fb.setPropertyValue("Sparse", 2);

    // Print after modifications
    std::cout << "\nFB2 after modifications:\n";
    print(fb);

    // Modify
    modify(fb, instance.getContext().getTypeManager());

    // Print after modifications
    std::cout << "\nFB2 after second round of modifications:\n";
    print(fb);

    // Gracefully exit
    std::cout << "Press \"enter\" to exit the application...\n";
    std::cin.get();
    return 0;
}
