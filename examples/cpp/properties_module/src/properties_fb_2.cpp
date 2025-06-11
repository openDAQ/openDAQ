#include <coreobjects/argument_info_factory.h>
#include <coreobjects/callable_info_factory.h>
#include <properties_module/properties_fb_2.h>
#include <iostream>

BEGIN_NAMESPACE_PROPERTIES_MODULE

PropertiesFb2::PropertiesFb2(const ContextPtr& ctx, const ComponentPtr& par, const StringPtr& locId)
    : FunctionBlock(CreateType(), ctx, par, locId)
{
    initProperties();
}

void PropertiesFb2::initProperties()
{
    // List (may contain other types) - used for storing multiple values of the same type
    auto list = List<IInteger>();
    auto listProp = ListProperty("List", list);
    objPtr.addProperty(listProp);

    // Dictionary - used for storing key-value pairs
    auto dict = Dict<IString, IString>();
    dict["key1"] = "Cheese";
    dict["key2"] = "Cake";
    dict["key3"] = "Lady";
    auto dictProp = DictProperty("Dict", dict);
    objPtr.addProperty(dictProp);

    // Struct - used for grouping multiple properties of different types
    auto manager = context.getTypeManager();  // Struct type must be registered before creating the Struct
    manager.addType(StructType("Struct", List<IString>("Int", "String"), List<IType>(SimpleType(ctInt), SimpleType(ctString))));
    auto stru = StructBuilder("Struct", manager).set("Int", 42).set("String", "Flowers").build();
    auto structProp = StructProperty("Struct", stru);
    objPtr.addProperty(structProp);

    // Enumeration - used for defining a set of named values (such as values for a drop-down menu)
    auto enumNames = List<IString>();
    enumNames.pushBack("First");
    enumNames.pushBack("Second");
    enumNames.pushBack("Third");
    manager.addType(EnumerationType("Enum", enumNames));  // Must be registered before creating the Enumeration
    auto enu = Enumeration("Enum", "Second", manager);
    auto enumProp = EnumerationProperty("Enum", enu);
    objPtr.addProperty(enumProp);

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

    // Callback for nested function property - used for handling changes to the function property
    funObj.getOnPropertyValueWrite("Function") += [](PropertyObjectPtr&, const PropertyValueEventArgsPtr& args)
    { std::cout << "Nested Function changed to: " << args.getValue() << "\n"; };

    objPtr.addProperty(funObjProp);

    // Selection - used for selecting one value from a list of options
    auto selectionProp = SelectionProperty("Selection", List<IFloat>(41.1, 42.2, 43.3), 1);
    objPtr.addProperty(selectionProp);

    // Sparse selection - used for selecting one value from a sparse set of options
    auto selection = Dict<Int, IString>();
    selection.set(4, "First");
    selection.set(5, "Second");
    selection.set(6, "Third");
    auto sparseProp = SparseSelectionProperty("Sparse", selection, 4);
    objPtr.addProperty(sparseProp);

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
}

FunctionBlockTypePtr PropertiesFb2::CreateType()
{
    return FunctionBlockType("PropertiesFb2", "Properties2", "Function Block focused on Properties 2");
}

END_NAMESPACE_PROPERTIES_MODULE
