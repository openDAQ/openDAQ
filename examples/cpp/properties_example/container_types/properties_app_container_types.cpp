/*
 * Properties Application: Container types and selection properties
 * Demonstrates features like container types and selection properties usage
 */

#include <utils.h>

using namespace daq;

int main(int /*argc*/, const char* /*argv*/[])
{
    // Create an Instance, loading modules in the default module path
    const InstancePtr instance = Instance();

    // Pretty print
    std::cout << "\nopenDAQ(TM) Properties Application: Container Types and Selection Properties\n\n";

    // Add Function Block by type ID
    auto fb = instance.addFunctionBlock("ExampleFBPropertyContainerTypes");

    // List
    configureListProperty(fb);

    // Dictionary
    configureDictProperty(fb);

    // For Struct and Enum Properties, the types must be first be registered in the Type Manager
    auto manager = instance.getContext().getTypeManager();

    // Struct
    configureStructProperty(fb, manager);

    // Struct reconfiguration via builder
    reconfigureStructProperty(fb);

    // Enumeration
    configureEnumProperty(fb, manager);

    // Selection
    configureBasicProperty(fb, "Selection", 2);

    // Sparse selection
    configureBasicProperty(fb, "Sparse", 2);

    // Gracefully exit
    std::cout << "Press \"enter\" to exit the application...\n";
    std::cin.get();
    return 0;
}
