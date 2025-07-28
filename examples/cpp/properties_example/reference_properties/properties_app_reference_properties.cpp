/*
 * Properties Application: Reference Properties
 * Demonstrates advanced features like references, validation, coercion, and conditional visibility
 */

#include <utils.h>

using namespace daq;

int main(int /*argc*/, const char* /*argv*/[])
{
    // Create an Instance, loading modules in the default module path
    const InstancePtr instance = Instance();

    // Add Function Block by type ID
    auto fb = instance.addFunctionBlock("ExampleFBPropertyReferenceProperties");

    // Apply changes in one go, later
    fb.beginUpdate();

    // Property visibility depending on another Property
    configureBasicProperty(fb, "SometimesVisible", 2);

    // Referenced and reference Bool
    configureBasicProperty(fb, "Referenced", True);

    // Apply changes in one go, no actual changes to Properties were made up to this point
    fb.endUpdate();

    // Print after calling endUpdate, changes are visible
    std::cout << "After calling endUpdate, changes are visible:" << std::endl;

    printProperty(fb.getProperty("SometimesVisible"));
    printProperty(fb.getProperty("Referenced"));

    // Check if Properties are referenced
    std::cout << "Referenced is referenced: " << Boolean(fb.getProperty("Referenced").getIsReferenced()) << "\n";
    std::cout << "Reference is referenced: " << Boolean(fb.getProperty("Reference").getIsReferenced()) << "\n";

    // Coerced Int
    configureBasicProperty(fb, "CoercedProp", 4);    // No coercion
    configureBasicProperty(fb, "CoercedProp", 142);  // Coerced to 10

    // Read-only Int
    try
    {
        configureBasicProperty(fb, "ReadOnlyInt", 42);
    }
    catch (const std::exception& e)
    {
        std::cout << "Exception: " << e.what() << "\n";
    }

    // Validated Int
    configureBasicProperty(fb, "ValidatedProp", 43);  // Valid
    try
    {
        configureBasicProperty(fb, "ValidatedProp", 1000);  // Fails validation
    }
    catch (const std::exception& e)
    {
        std::cout << "Exception: " << e.what() << "\n";
    }

    // Gracefully exit
    std::cout << "Press \"enter\" to exit the application...\n";
    std::cin.get();
    return 0;
}
