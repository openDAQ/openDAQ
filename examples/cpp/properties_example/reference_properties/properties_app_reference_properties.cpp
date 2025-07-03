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
    auto fb = instance.addFunctionBlock("ExampleFBPropertyReferenceProperties");

    // Apply changes in one sweep later
    fb.beginUpdate();

    // Print before modifications
    std::cout << "\nBefore modifications/update:\n";
    print(fb);

    // Property visibility depending on another Property
    fb.setPropertyValue("SometimesVisible", 2);

    // Referenced and reference Bool
    fb.setPropertyValue("Reference", True);

    // Check if Properties are referenced
    std::cout << "Referenced is referenced: " << Boolean(fb.getProperty("Referenced").getIsReferenced()) << "\n";
    std::cout << "Reference is referenced: " << Boolean(fb.getProperty("Reference").getIsReferenced()) << "\n";

    // Coerced Int
    fb.setPropertyValue("CoercedProp", 4);    // No coercion
    fb.setPropertyValue("CoercedProp", 142);  // Coerced to 10

    // Print after calling set but before modifications are applied via endUpdate
    std::cout << "\nFB4 after calling set but before modifications are applied (should be the same as before):\n";
    print(fb);

    // Apply changes in one sweep
    fb.endUpdate();

    // Read-only Int
    try
    {
        fb.setPropertyValue("ReadOnlyInt", 42);
    }
    catch (const std::exception& e)
    {
        std::cout << "Exception: " << e.what() << "\n";
    }

    // Validated Int
    fb.setPropertyValue("ValidatedProp", 43);  // Valid
    try
    {
        fb.setPropertyValue("ValidatedProp", 1000);  // Fails validation
    }
    catch (const std::exception& e)
    {
        std::cout << "Exception: " << e.what() << "\n";
    }

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
