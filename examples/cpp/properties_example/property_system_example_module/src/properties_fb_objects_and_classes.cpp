#include <coreobjects/argument_info_factory.h>
#include <coreobjects/callable_info_factory.h>
#include <property_system_example_module/properties_fb_objects_and_classes.h>
#include <iostream>

BEGIN_NAMESPACE_PROPERTIES_MODULE
ExampleFBPropertyObjectsAndClasses::ExampleFBPropertyObjectsAndClasses(const ContextPtr& ctx, const ComponentPtr& par, const StringPtr& locId)
    : FunctionBlock(CreateType(), ctx, par, locId)
{
    initProperties();
}

void ExampleFBPropertyObjectsAndClasses::initProperties()
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
    auto funObjProp = ObjectProperty("FunctionObject", funObj);
    auto fun = Function(
        [](IntegerPtr a, IntegerPtr b)
        {
            std::cout << "Function called\n";
            return a + b;
        });
    funObj.setPropertyValue("Function", fun);
    objPtr.addProperty(funObjProp);

    // Explicit permissions for function execution - used for controlling access to the function
    // TODO: this is not used in the example (we don't add any servers, etc.), but it can be useful for more complex scenarios
    auto permissions = PermissionsBuilder()
                           .inherit(false)
                           .assign("everyone", PermissionMaskBuilder().read())
                           .assign("admin", PermissionMaskBuilder().read().write().execute())
                           .build();
    funObj.getPermissionManager().setPermissions(permissions);

    // Object class - used for defining a class-like structure with properties
    auto typeManager = context.getTypeManager();

    auto inheritedObjClassProp = PropertyObjectClassBuilder("InheritedClass")
                                     .addProperty(IntProperty("Integer", 10))
                                     .addProperty(SelectionProperty("Selection", List<IString>("Banana", "Apple", "Kiwi"), 1))
                                     .build();
    typeManager.addType(inheritedObjClassProp);  // TODO: this should be done in the module initialization, not here?

    // Object class - can also use inheritance
    auto objClassProp =
        PropertyObjectClassBuilder(typeManager, "Class").addProperty(StringProperty("Foo", "Bar")).setParentName("InheritedClass").build();
    typeManager.addType(objClassProp);  // TODO: this should be done in the module initialization, not here?

    auto obj = PropertyObject(typeManager, "Class");
    auto classProp = ObjectProperty("ClassObject", obj);
    objPtr.addProperty(classProp);
}

FunctionBlockTypePtr ExampleFBPropertyObjectsAndClasses::CreateType()
{
    return FunctionBlockType("ExampleFBPropertyObjectsAndClasses", "ExampleFBPropertyObjectsAndClasses", "Function Block focused on objects and classes");
}

END_NAMESPACE_PROPERTIES_MODULE
