/*
 * Advanced features like references, validation, coercion, and conditional visibility
 */

#include <utils.h>

using namespace daq;

int main(int /*argc*/, const char* /*argv*/[])
{
    // Create an Instance, loading modules in the default module path
    const InstancePtr instance = Instance();

    // Add Function Block by type ID
    auto fb4 = instance.addFunctionBlock("ExampleFBPropertyReferenceProperties");

    // Apply changes in one sweep later
    fb4.beginUpdate();

    // Print before modifications
    std::cout << "\nBefore modifications/update:\n";
    print(fb4);

    // Property visibility depending on another Property
    fb4.setPropertyValue("SometimesVisible", 2);

    // Referenced and reference Bool
    fb4.setPropertyValue("Reference", True);

    // Check if Properties are referenced
    std::cout << "Referenced is referenced: " << Boolean(fb4.getProperty("Referenced").getIsReferenced()) << "\n";
    std::cout << "Reference is referenced: " << Boolean(fb4.getProperty("Reference").getIsReferenced()) << "\n";

    // Coerced Int
    fb4.setPropertyValue("CoercedProp", 4);    // No coercion
    fb4.setPropertyValue("CoercedProp", 142);  // Coerced to 10

    // Print after calling set but before modifications are applied via endUpdate
    std::cout << "\nFB4 after calling set but before modifications are applied (should be the same as before):\n";
    print(fb4);

    // Apply changes in one sweep
    fb4.endUpdate();

    // Read-only Int
    try
    {
        fb4.setPropertyValue("ReadOnlyInt", 42);
    }
    catch (const std::exception& e)
    {
        std::cout << "Exception: " << e.what() << "\n";
    }

    // Validated Int
    fb4.setPropertyValue("ValidatedProp", 43);  // Valid
    try
    {
        fb4.setPropertyValue("ValidatedProp", 1000);  // Fails validation
    }
    catch (const std::exception& e)
    {
        std::cout << "Exception: " << e.what() << "\n";
    }

    // Print after modifications
    std::cout << "\nAfter modifications:\n";
    print(fb4);

    // Modify
    modify(fb4, instance.getContext().getTypeManager());

    // Print after modifications
    std::cout << "\nAfter second round of modifications:\n";
    print(fb4);

    // Gracefully exit
    std::cout << "Press \"enter\" to exit the application...\n";
    std::cin.get();
    return 0;
}
