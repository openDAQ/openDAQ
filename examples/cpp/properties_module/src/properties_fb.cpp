#include <coreobjects/argument_info_factory.h>
#include <coreobjects/callable_info_factory.h>
#include <properties_module/properties_fb.h>
#include <iostream>

BEGIN_NAMESPACE_PROPERTIES_MODULE
PropertiesFb::PropertiesFb(const ContextPtr& ctx, const ComponentPtr& par, const StringPtr& locId)
    : FunctionBlock(CreateType(), ctx, par, locId)
{
    initProperties();
}

void PropertiesFb::initProperties()
{
    // Bool
    auto boolProp = BoolPropertyBuilder("Bool", False).setDescription("A very nice boolean").build();  //  description is optional
    objPtr.addProperty(boolProp);
    objPtr.getOnPropertyValueWrite("Bool") += [this](PropertyObjectPtr& /*obj*/, const PropertyValueEventArgsPtr& args)
    { std::cout << "Bool changed to: " << args.getValue() << "\n"; };

    // Int
    auto intProp = IntPropertyBuilder("Int", 42).setUnit(Unit("Unit")).build();  // unit is optional
    objPtr.addProperty(intProp);
    objPtr.getOnPropertyValueWrite("Int") += [this](PropertyObjectPtr& /*obj*/, const PropertyValueEventArgsPtr& args)
    { std::cout << "Int changed to: " << args.getValue() << "\n"; };

    // Float
    auto floatProp = FloatProperty("Float", 7.2);
    objPtr.addProperty(floatProp);
    objPtr.getOnPropertyValueWrite("Float") += [this](PropertyObjectPtr& /*obj*/, const PropertyValueEventArgsPtr& args)
    { std::cout << "Float changed to: " << args.getValue() << "\n"; };

    // String
    auto stringProp = StringProperty("String", "Hello World");
    objPtr.addProperty(stringProp);
    objPtr.getOnPropertyValueWrite("String") += [this](PropertyObjectPtr& /*obj*/, const PropertyValueEventArgsPtr& args)
    { std::cout << "String changed to: " << args.getValue() << "\n"; };

    // Ratio
    auto ratioProp = RatioProperty("Ratio", Ratio(1, 12));
    objPtr.addProperty(ratioProp);
    objPtr.getOnPropertyValueWrite("Ratio") += [this](PropertyObjectPtr& /*obj*/, const PropertyValueEventArgsPtr& args)
    { std::cout << "Ratio changed to: " << args.getValue() << "\n"; };

    // List
    auto list = List<IInteger>();
    auto listProp = ListProperty("List", list);
    objPtr.addProperty(listProp);
    objPtr.getOnPropertyValueWrite("List") += [this](PropertyObjectPtr& /*obj*/, const PropertyValueEventArgsPtr& args)
    { std::cout << "List changed to: " << args.getValue() << "\n"; };

    // Dictionary
    auto dict = Dict<IString, IString>();
    dict["key1"] = "Cheese";
    dict["key2"] = "Cake";
    dict["key3"] = "Lady";
    auto dictProp = DictProperty("Dict", dict);
    objPtr.addProperty(dictProp);
    objPtr.getOnPropertyValueWrite("Dict") += [this](PropertyObjectPtr& /*obj*/, const PropertyValueEventArgsPtr& args)
    {
        DictPtr<IString, IString> dict = args.getValue();
        std::cout << "Dict changed to: " << "\n";
        for (const auto& item : dict)
        {
            std::cout << "  " << item.first << ": " << item.second << "\n";
        }
    };

    // Struct
    auto manager = context.getTypeManager();
    manager.addType(StructType("Struct", List<IString>("Int", "String"), List<IType>(SimpleType(ctInt), SimpleType(ctString))));
    auto stru = StructBuilder("Struct", manager).set("Int", 42).set("String", "Flowers").build();
    auto structProp = StructProperty("Struct", stru);
    objPtr.addProperty(structProp);
    objPtr.getOnPropertyValueWrite("Struct") += [this](PropertyObjectPtr& /*obj*/, const PropertyValueEventArgsPtr& args)
    { std::cout << "Struct changed to: " << args.getValue() << "\n"; };

    // Enumeration
    auto enumNames = List<IString>();
    enumNames.pushBack("First");
    enumNames.pushBack("Second");
    enumNames.pushBack("Third");
    manager.addType(EnumerationType("Enum", enumNames));
    auto enu = Enumeration("Enum", "Second", manager);
    auto enumProp = EnumerationProperty("Enum", enu);
    objPtr.addProperty(enumProp);
    objPtr.getOnPropertyValueWrite("Enum") += [this](PropertyObjectPtr& /*obj*/, const PropertyValueEventArgsPtr& args)
    { std::cout << "Enum changed to: " << args.getValue() << "\n"; };

    // Procedure
    auto procProp = FunctionProperty("Procedure", ProcedureInfo(List<IArgumentInfo>(ArgumentInfo("a", ctInt))));
    objPtr.addProperty(procProp);
    auto proc = Procedure([](IntegerPtr a) { std::cout << "Procedure called with: " << a << "\n"; });
    objPtr.setPropertyValue("Procedure", proc);
    objPtr.getOnPropertyValueWrite("Procedure") += [this](PropertyObjectPtr& /*obj*/, const PropertyValueEventArgsPtr& args)
    { std::cout << "Procedure changed to: " << args.getValue() << "\n"; };

    // Function
    auto funProp =
        FunctionProperty("Function", FunctionInfo(ctInt, List<IArgumentInfo>(ArgumentInfo("a", ctInt), ArgumentInfo("b", ctInt))));
    objPtr.addProperty(funProp);
    auto fun = Function(
        [](IntegerPtr a, IntegerPtr b)
        {
            std::cout << "Function called\n";
            return a + b;
        });
    objPtr.setPropertyValue("Function", fun);
    objPtr.getOnPropertyValueWrite("Function") += [this](PropertyObjectPtr& /*obj*/, const PropertyValueEventArgsPtr& args)
    { std::cout << "Function changed to: " << args.getValue() << "\n"; };

    // Selection
    auto selectionProp = SelectionProperty("Selection", List<IString>("first", "second", "third"), 1);
    objPtr.addProperty(selectionProp);
    objPtr.getOnPropertyValueWrite("Selection") += [this](PropertyObjectPtr& /*obj*/, const PropertyValueEventArgsPtr& args)
    { std::cout << "Selection changed to: " << args.getValue() << "\n"; };

    // Sparse selection
    auto selection = Dict<Int, IString>();
    selection.set(4, "First");
    selection.set(5, "Second");
    selection.set(6, "Third");
    auto sparseProp = SparseSelectionProperty("Sparse", selection, 4);
    objPtr.addProperty(sparseProp);
    objPtr.getOnPropertyValueWrite("Sparse") += [this](PropertyObjectPtr& /*obj*/, const PropertyValueEventArgsPtr& args)
    { std::cout << "Sparse changed to: " << args.getValue() << "\n"; };

    // Object
    auto innerObj = PropertyObject();
    innerObj.addProperty(BoolProperty("Bool", False));
    auto innerProp = ObjectProperty("InnerObject", innerObj);
    auto propObj = PropertyObject();
    propObj.addProperty(innerProp);
    propObj.addProperty(IntProperty("Int", 42));
    propObj.addProperty(FloatProperty("Float", 7.2));
    auto objProp = ObjectProperty("Object", propObj);
    objPtr.addProperty(objProp);
    propObj.getOnPropertyValueWrite("Int") += [this](PropertyObjectPtr& /*obj*/, const PropertyValueEventArgsPtr& args)
    { std::cout << "Object.InnerObject.Int changed to: " << args.getValue() << "\n"; };

    // Referenced Bool
    auto otherVisible = BoolProperty("OtherVisible", False);
    objPtr.addProperty(otherVisible);
    objPtr.getOnPropertyValueWrite("OtherVisible") += [this](PropertyObjectPtr& /*obj*/, const PropertyValueEventArgsPtr& args)
    { std::cout << "OtherVisible changed to: " << args.getValue() << "\n"; };

    // Property visibility depending on referenced Property
    auto sometimeVisibleProperty = BoolPropertyBuilder("SometimeVisible", False)
                                       .setVisible(EvalValue("$OtherVisible"))  // This will evaluate OtherVisible Property
                                       .build();
    objPtr.addProperty(sometimeVisibleProperty);
    objPtr.getOnPropertyValueWrite("SometimeVisible") += [this](PropertyObjectPtr& /*obj*/, const PropertyValueEventArgsPtr& args)
    { std::cout << "SometimeVisible changed to: " << args.getValue() << "\n"; };
}

FunctionBlockTypePtr PropertiesFb::CreateType()
{
    return FunctionBlockType("PropertiesFb", "Properties", "Function Block focused on Properties");
}

END_NAMESPACE_PROPERTIES_MODULE
