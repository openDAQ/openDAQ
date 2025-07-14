/*
 * Properties Application: Objects and Classes
 * Demonstrates features like Object properties, procedures, functions, and inheritance
 */

#include <utils.h>

using namespace daq;

int main(int /*argc*/, const char* /*argv*/[])
{
    // Create an Instance, loading modules in the default module path
    const InstancePtr instance = Instance();

    // Add Function Block by type ID
    auto fb = instance.addFunctionBlock("ExampleFBPropertyObjectsAndClasses");

    // Print before modifications
    std::cout << "\nBefore modifications:\n";
    printFBProperties(fb);

    // Object
    fb.setPropertyValue("Object.InnerObject.Bool", True);
    fb.setPropertyValue("Object.Int", 987);
    fb.setPropertyValue("Object.Float", 4.44);
    fb.clearPropertyValue("Object");  // Resets the Object to its default state, so the above lines will be invalidated

    // Procedure
    ProcedurePtr oldProc = fb.getPropertyValue("Procedure");
    oldProc(42);
    auto proc = Procedure([](IntegerPtr a) { std::cout << "New procedure called with: " << a << "\n"; });
    fb.setPropertyValue("Procedure", proc);
    ProcedurePtr newProc = fb.getPropertyValue("Procedure");
    newProc(42);

    // Function
    FunctionPtr oldFun = fb.getPropertyValue("FunctionObject.Function");
    auto res = oldFun(2, 3);
    std::cout << "Old function result (2 + 3): " << res << "\n";
    auto fun = Function(
        [](IntegerPtr a, IntegerPtr b)
        {
            std::cout << "New function called\n";
            return a * b;
        });
    fb.setPropertyValue("FunctionObject.Function", fun);
    FunctionPtr newFun = fb.getPropertyValue("FunctionObject.Function");
    auto newRes = newFun(2, 3);
    std::cout << "New function result (2 * 3): " << newRes << "\n";

    // Object class
    fb.setPropertyValue("ClassObject.Foo", "BarBar");
    fb.setPropertyValue("ClassObject.Integer", 5);

    // Gracefully exit
    std::cout << "Press \"enter\" to exit the application...\n";
    std::cin.get();
    return 0;
}
