/**
 * properties_app_3
 * (demos using properties_fb_3)
 */

#include <utils.h>

using namespace daq;

int main(int /*argc*/, const char* /*argv*/[])
{
    // Create an Instance, loading modules in the default module path
    const InstancePtr instance = Instance();

    // Add Function Block by type ID
    auto fb3 = instance.addFunctionBlock("PropertiesFb3");

    // Print before modifications
    std::cout << "\nFB3 before modifications:\n";
    print(fb3);

    // Object
    fb3.setPropertyValue("Object.InnerObject.Bool", True);
    fb3.setPropertyValue("Object.Int", 987);
    fb3.setPropertyValue("Object.Float", 4.44);
    fb3.clearPropertyValue("Object");  // Resets the Object to its default state, so the above lines will be invalidated

    // Procedure
    ProcedurePtr oldProc = fb3.getPropertyValue("Procedure");
    oldProc(42);
    auto proc = Procedure([](IntegerPtr a) { std::cout << "New procedure called with: " << a << "\n"; });
    fb3.setPropertyValue("Procedure", proc);
    ProcedurePtr newProc = fb3.getPropertyValue("Procedure");
    newProc(42);

    // Function
    FunctionPtr oldFun = fb3.getPropertyValue("FunctionObject.Function");
    auto res = oldFun(2, 3);
    std::cout << "Old function result (2 + 3): " << res << "\n";
    auto fun = Function(
        [](IntegerPtr a, IntegerPtr b)
        {
            std::cout << "New function called\n";
            return a * b;
        });
    fb3.setPropertyValue("FunctionObject.Function", fun);
    FunctionPtr newFun = fb3.getPropertyValue("FunctionObject.Function");
    auto newRes = newFun(2, 3);
    std::cout << "New function result (2 * 3): " << newRes << "\n";

    // Object class
    fb3.setPropertyValue("ClassObject.Foo", "BarBar");
    fb3.setPropertyValue("ClassObject.Integer", 5);

    // Print after modifications
    std::cout << "\nFB3 after modifications:\n";
    print(fb3);

    // Modify
    modify(fb3, instance.getContext().getTypeManager());

    // Print after modifications
    std::cout << "\nFB3 after second round of modifications:\n";
    print(fb3);

    // Gracefully exit
    std::cout << "Press \"enter\" to exit the application...\n";
    std::cin.get();
    return 0;
}
