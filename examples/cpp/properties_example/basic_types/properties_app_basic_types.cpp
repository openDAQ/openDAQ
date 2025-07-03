/*
 * Basic property types and callbacks
 */

#include <utils.h>

using namespace daq;

int main(int /*argc*/, const char* /*argv*/[])
{
    // Create an Instance, loading modules in the default module path
    const InstancePtr instance = Instance();

    // Add Function Block by type ID
    auto fb = instance.addFunctionBlock("ExampleFBPropertyBasicTypes");

    // Print before modifications
    std::cout << "\nBefore modifications:\n";
    print(fb);

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

    // Min and max Float
    fb.setPropertyValue("MinMaxProp", 101.1);  // Clamped to 100.0
    fb.setPropertyValue("MinMaxProp", -1.1);   // Clamped to 0.0
    fb.setPropertyValue("MinMaxProp", 50.1);   // Within range

    // Suggested values Float
    auto suggested = fb.getProperty("SuggestedProp").getSuggestedValues();
    auto selected = suggested[2].asPtrOrNull<IFloat>();
    fb.setPropertyValue("SuggestedProp", selected);  // Will set to 3.3, because that's the third suggested value in the module

    // Stubborn Int
    fb.setPropertyValue("StubbornInt", 41);  // Will be forced to 43

    // Print after modifications
    std::cout << "\nAfter modifications:\n";
    print(fb);

    // Modify
    modify(fb, instance.getContext().getTypeManager());

    // Print after modifications
    std::cout << "\nAfter second round of modifications:\n";
    print(fb);

    // Gracefully exit
    std::cout << "Press \"enter\" to exit the application...\n";
    std::cin.get();
    return 0;
}
