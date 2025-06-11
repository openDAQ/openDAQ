#include <coreobjects/argument_info_factory.h>
#include <coreobjects/callable_info_factory.h>
#include <properties_module/properties_fb_3.h>
#include <iostream>

BEGIN_NAMESPACE_PROPERTIES_MODULE
PropertiesFb3::PropertiesFb3(const ContextPtr& ctx, const ComponentPtr& par, const StringPtr& locId)
    : FunctionBlock(CreateType(), ctx, par, locId)
{
    initProperties();
}

void PropertiesFb3::initProperties()
{
    // Object - used for grouping multiple properties of different types, including nested objects
    auto innerObj = PropertyObject();
    innerObj.addProperty(BoolProperty("Bool", False));
    auto innerProp = ObjectProperty("InnerObject", innerObj);
    auto propObj = PropertyObject();
    propObj.addProperty(innerProp);
    propObj.addProperty(IntProperty("Int", 42));
    propObj.addProperty(FloatProperty("Float", 7.2));
    auto objProp = ObjectProperty("Object", propObj);
    objPtr.addProperty(objProp);

    // Procedure - used for defining a callable procedure with arguments (doesn't return anything)
    auto procProp = FunctionProperty("Procedure", ProcedureInfo(List<IArgumentInfo>(ArgumentInfo("a", ctInt))));
    objPtr.addProperty(procProp);
    auto proc = Procedure([](IntegerPtr a) { std::cout << "Procedure called with: " << a << "\n"; });
    objPtr.setPropertyValue("Procedure", proc);

    // Protected nested Function - used for defining a callable function with arguments and a return value
    auto funObj = PropertyObject();
    auto funProp =
        FunctionProperty("Function", FunctionInfo(ctInt, List<IArgumentInfo>(ArgumentInfo("a", ctInt), ArgumentInfo("b", ctInt))));
    funObj.addProperty(funProp);
    // Explicit permissions for function execution - used for controlling access to the function
    // TODO: this is not used in the example (we don't add any servers, etc.), but it can be useful for more complex scenarios
    auto permissions = PermissionsBuilder()
                           .inherit(false)
                           .assign("everyone", PermissionMaskBuilder().read())
                           .assign("admin", PermissionMaskBuilder().read().write().execute())
                           .build();
    funObj.getPermissionManager().setPermissions(permissions);
    auto funObjProp = ObjectProperty("FunctionObject", funObj);
    auto fun = Function(
        [](IntegerPtr a, IntegerPtr b)
        {
            std::cout << "Function called\n";
            return a + b;
        });
    funObj.setPropertyValue("Function", fun);

    objPtr.addProperty(funObjProp);

    // Object class - used for defining a class-like structure with properties and methods
    auto objectClassProp = PropertyObjectClassBuilder("Class")
                               .addProperty(IntProperty("Integer", 10))
                               .addProperty(SelectionProperty("Selection", List<IString>("Banana", "Apple", "Kiwi"), 1))
                               .build();
    auto typeManager = context.getTypeManager();
    typeManager.addType(objectClassProp);
    auto obj = PropertyObject(typeManager, "Class");
    auto classProp = ObjectProperty("ClassObject", obj);
    objPtr.addProperty(classProp);
}

FunctionBlockTypePtr PropertiesFb3::CreateType()
{
    return FunctionBlockType("PropertiesFb3", "Properties3", "Function Block focused on Properties 3");
}

END_NAMESPACE_PROPERTIES_MODULE
