/*
 * Properties Application: Objects and Classes
 * Demonstrates features like Object properties, procedures, functions, and inheritance
 */

#include <utils.h>

using namespace daq;

int main(int /*argc*/, const char* /*argv*/[])
{
    // Pretty print
    std::cout << "\nopenDAQ(TM) Properties Application: Objects and Classes\n\n";

    // Create an Instance, loading modules in the default module path
    const InstancePtr instance = Instance();

    // Add Function Block by type ID
    auto fb = instance.addFunctionBlock("ExampleFBPropertyObjectsAndClasses");

    // Object
    configureBasicProperty(fb, "Object.InnerObject.Bool", True);
    configureBasicProperty(fb, "Object.Int", 987);
    configureBasicProperty(fb, "Object.Float", 4.44);
    std::cout << "Clearing Property value of Object\n";
    fb.clearPropertyValue("Object");  // Resets the Object to its default state, so the above lines will be invalidated
    std::cout << "Object after clearing:\n";
    printProperty(fb.getProperty("Object"));

    // Procedure
    ProcedurePtr oldProc = fb.getPropertyValue("Procedure");
    oldProc(42);
    auto proc = Procedure([](IntegerPtr a) { std::cout << "New procedure called with: " << a << "\n"; });
    configureBasicProperty(fb, "Procedure", proc);
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
    configureBasicProperty(fb, "FunctionObject.Function", fun);
    FunctionPtr newFun = fb.getPropertyValue("FunctionObject.Function");
    auto newRes = newFun(2, 3);
    std::cout << "New function result (2 * 3): " << newRes << "\n";

    // Object class
    configureBasicProperty(fb, "ClassObject.Foo", "BarBar");
    configureBasicProperty(fb, "ClassObject.Integer", 5);

    // Gracefully exit
    std::cout << "Press \"enter\" to exit the application...\n";
    std::cin.get();
    return 0;
}
