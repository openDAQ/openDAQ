/**
 * properties_app_2
 * (demos using properties_fb_2)
 * Container types and selection properties
 */

#include <utils.h>

using namespace daq;

int main(int /*argc*/, const char* /*argv*/[])
{
    // Create an Instance, loading modules in the default module path
    const InstancePtr instance = Instance();

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
    auto struMod = fb2.getPropertyValue("Struct");
    auto structBuild = StructBuilder(struMod).set("Int", 200).set("String", "openDAQ modified").build();
    fb2.setPropertyValue("Struct", structBuild);

    // Enumeration
    auto enumVal = Enumeration("Enum", "Third", manager);
    fb2.setPropertyValue("Enum", enumVal);

    // Selection
    fb2.setPropertyValue("Selection", 2);

    // Sparse selection
    fb2.setPropertyValue("Sparse", 2);

    // Print after modifications
    std::cout << "\nFB2 after modifications:\n";
    print(fb2);

    // Modify
    modify(fb2, instance.getContext().getTypeManager());

    // Print after modifications
    std::cout << "\nFB2 after second round of modifications:\n";
    print(fb2);

    // Gracefully exit
    std::cout << "Press \"enter\" to exit the application...\n";
    std::cin.get();
    return 0;
}
