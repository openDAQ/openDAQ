/*
 * Properties Application: Basic Property types and callbacks
 * Demonstrates basic Property features
 */

#include <utils.h>

using namespace daq;

int main(int /*argc*/, const char* /*argv*/[])
{
    // Pretty print
    std::cout << "\nopenDAQ(TM) Properties Application: Basic Property types and callbacks\n\n";

    // Create an Instance, loading modules in the default module path
    const InstancePtr instance = Instance();

    // Add Function Block by type ID
    auto fb = instance.addFunctionBlock("ExampleFBPropertyBasicTypes");

    // Bool
    configureBasicProperty(fb, "Bool", True);

    // Int
    configureBasicProperty(fb, "Int", 100);

    // Float
    configureBasicProperty(fb, "Float", 3.14);

    // String
    configureBasicProperty(fb, "String", "Hello openDAQ");

    // Ratio
    configureBasicProperty(fb, "Ratio", Ratio(1, 2));

    // Min and max Float
    configureBasicProperty(fb, "MinMaxProp", 101.1);  // Clamped to 100.0
    configureBasicProperty(fb, "MinMaxProp", -1.1);   // Clamped to 0.0
    configureBasicProperty(fb, "MinMaxProp", 50.1);   // Within range

    // Suggested values Float
    auto suggested = fb.getProperty("SuggestedProp").getSuggestedValues();
    auto selected = suggested[2].asPtrOrNull<IFloat>();
    configureBasicProperty(fb, "SuggestedProp", selected);  // Will set to 3.3, because that's the third suggested value in the module

    // Stubborn Int
    configureBasicProperty(fb, "StubbornInt", 41);  // Will be forced to 43

    // Gracefully exit
    std::cout << "Press \"enter\" to exit the application...\n";
    std::cin.get();
    return 0;
}
