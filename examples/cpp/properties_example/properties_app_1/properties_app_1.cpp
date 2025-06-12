/**
 * properties_app_1
 * (demos using properties_fb_1)
 */

#include <utils.h>

int main(int /*argc*/, const char* /*argv*/[])
{
    // Create an Instance, loading modules in the default module path
    const InstancePtr instance = Instance();

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

    // Gracefully exit
    std::cout << "Press \"enter\" to exit the application...\n";
    std::cin.get();
    return 0;
}
