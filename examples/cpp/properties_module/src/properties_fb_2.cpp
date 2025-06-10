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

    // Dictionary with custom callback
    objPtr.addProperty(dictProp);

    // Struct - used for grouping multiple properties of different types
    auto manager = context.getTypeManager();
    manager.addType(StructType("Struct", List<IString>("Int", "String"), List<IType>(SimpleType(ctInt), SimpleType(ctString))));
    auto stru = StructBuilder("Struct", manager).set("Int", 42).set("String", "Flowers").build();
    auto structProp = StructProperty("Struct", stru);
    objPtr.addProperty(structProp);

    // Enumeration - used for defining a set of named values (such as values for a drop-down menu)
    auto enumNames = List<IString>();
    enumNames.pushBack("First");
    enumNames.pushBack("Second");
    enumNames.pushBack("Third");
    manager.addType(EnumerationType("Enum", enumNames));
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
    auto selectionProp = SelectionProperty("Selection", List<IUnit>(Unit("FirstUnit"), Unit("SecondUnit"), Unit("ThirdUnit")), 1);
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

    // Referenced Bool - used for demo purposes fo referencing another Property
    auto referencedProp = BoolProperty("Referenced", False);
    objPtr.addProperty(referencedProp);

    // Reference Bool, and using EvalValue syntax
    auto referenceProp = ReferenceProperty("Reference", EvalValue("%Referenced"));
    objPtr.addProperty(referenceProp);

    // Property visibility depending on another Property, and using EvalValue syntax
    auto sometimesVisibleProperty = IntPropertyBuilder("SometimesVisible", 3)
                                        .setVisible(EvalValue("$Referenced"))  // This will evaluate referenced Property
                                        .setUnit(EvalValue("%Selection:SelectedValue"))
                                        .build();
    objPtr.addProperty(sometimesVisibleProperty);

    // Stubborn Int (always sets to 43)
    auto stubbornProp = IntProperty("StubbornInt", 42);

    // This one has a special callback
    objPtr.addProperty(stubbornProp);

    // Read-only Int
    auto readOnlyProp = IntPropertyBuilder("ReadOnlyInt", 42).setReadOnly(true).build();
    objPtr.addProperty(readOnlyProp);
    // No point in adding an on-write callback for a read-only property, as it cannot be changed by the user

    // Coerced Int
    auto coercedProp = IntPropertyBuilder("CoercedProp", 5).setCoercer(Coercer("if(Value > 10, 10, Value)")).build();
    objPtr.addProperty(coercedProp);

    // Validated Int
    auto validatedProp = IntPropertyBuilder("ValidatedProp", 42).setValidator(Validator("Value < 100")).build();
    objPtr.addProperty(validatedProp);

    // Min and max Float
    auto minMaxProp = FloatPropertyBuilder("MinMaxProp", 0.0).setMinValue(0.0).setMaxValue(100.0).build();
    objPtr.addProperty(minMaxProp);

    // Suggested values Float
    auto suggestedProp = FloatPropertyBuilder("SuggestedProp", 2.2).setSuggestedValues(List<IFloat>(1.1, 2.2, 3.3)).build();
    objPtr.addProperty(suggestedProp);
}

FunctionBlockTypePtr PropertiesFb2::CreateType()
{
    return FunctionBlockType("PropertiesFb2", "Properties2", "Function Block focused on Properties 2");
}

END_NAMESPACE_PROPERTIES_MODULE
