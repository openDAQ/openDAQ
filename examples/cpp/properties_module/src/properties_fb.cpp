#include <coreobjects/argument_info_factory.h>
#include <coreobjects/callable_info_factory.h>
#include <properties_module/properties_fb.h>
#include <iostream>

BEGIN_NAMESPACE_PROPERTIES_MODULE
PropertiesFb::PropertiesFb(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId)
    : FunctionBlock(CreateType(), ctx, parent, localId)
{
    initProperties();
}

void PropertiesFb::initProperties()
{
    // Bool
    auto boolProp = BoolProperty("myPropBool", False);
    objPtr.addProperty(boolProp);
    objPtr.getOnPropertyValueWrite("myPropBool") += [this](PropertyObjectPtr& /*obj*/, const PropertyValueEventArgsPtr& args)
    { std::cout << "myPropBool changed to: " << args.getValue() << "\n"; };

    // Int
    auto intProp = IntProperty("myPropInt", 42);
    objPtr.addProperty(intProp);
    objPtr.getOnPropertyValueWrite("myPropInt") += [this](PropertyObjectPtr& /*obj*/, const PropertyValueEventArgsPtr& args)
    { std::cout << "myPropInt changed to: " << args.getValue() << "\n"; };

    // Float
    auto floatProp = FloatProperty("myPropFloat", 7.2);
    objPtr.addProperty(floatProp);
    objPtr.getOnPropertyValueWrite("myPropFloat") += [this](PropertyObjectPtr& /*obj*/, const PropertyValueEventArgsPtr& args)
    { std::cout << "myPropFloat changed to: " << args.getValue() << "\n"; };

    // String
    auto stringProp = StringProperty("myPropString", "Hello World");
    objPtr.addProperty(stringProp);
    objPtr.getOnPropertyValueWrite("myPropString") += [this](PropertyObjectPtr& /*obj*/, const PropertyValueEventArgsPtr& args)
    { std::cout << "myPropString changed to: " << args.getValue() << "\n"; };

    // Ratio
    auto ratioProp = RatioProperty("myPropRatio", Ratio(1, 12));
    objPtr.addProperty(ratioProp);
    objPtr.getOnPropertyValueWrite("myPropRatio") += [this](PropertyObjectPtr& /*obj*/, const PropertyValueEventArgsPtr& args)
    { std::cout << "myPropRatio changed to: " << args.getValue() << "\n"; };

    // List
    auto list = List<IInteger>();
    auto listProp = ListProperty("myPropList", list);
    objPtr.addProperty(listProp);
    objPtr.getOnPropertyValueWrite("myPropList") += [this](PropertyObjectPtr& /*obj*/, const PropertyValueEventArgsPtr& args)
    { std::cout << "myPropList changed to: " << args.getValue() << "\n"; };

    // Dictionary
    auto dict = Dict<IString, IString>();
    dict["key1"] = "cheese";
    dict["key2"] = "cake";
    dict["key3"] = "lady";
    auto dictProp = DictProperty("myPropDict", dict);
    objPtr.addProperty(dictProp);
    objPtr.getOnPropertyValueWrite("myPropDict") += [this](PropertyObjectPtr& /*obj*/, const PropertyValueEventArgsPtr& args)
    {
        DictPtr<IString, IString> dict = args.getValue();
        std::cout << "myPropDict changed to: " << "\n";
        for (const auto& item : dict)
        {
            std::cout << "  " << item.first << ": " << item.second << "\n";
        }
    };

    // Struct
    auto manager = context.getTypeManager();
    manager.addType(StructType("myStruct", List<IString>("myInt", "myString"), List<IType>(SimpleType(ctInt), SimpleType(ctString))));
    auto stru = StructBuilder("myStruct", manager).set("myInt", 42).set("myString", "flowers").build();
    auto structProp = StructProperty("myPropStruct", stru);
    objPtr.addProperty(structProp);
    objPtr.getOnPropertyValueWrite("myPropStruct") += [this](PropertyObjectPtr& /*obj*/, const PropertyValueEventArgsPtr& args)
    { std::cout << "myPropStruct changed to: " << args.getValue() << "\n"; };

    // Enumeration
    auto enumNames = List<IString>();
    enumNames.pushBack("first");
    enumNames.pushBack("second");
    enumNames.pushBack("third");
    manager.addType(EnumerationType("myEnum", enumNames));
    auto enu = Enumeration("myEnum", "second", manager);
    auto enumProp = EnumerationProperty("myPropEnum", enu);
    objPtr.addProperty(enumProp);
    objPtr.getOnPropertyValueWrite("myPropEnum") += [this](PropertyObjectPtr& /*obj*/, const PropertyValueEventArgsPtr& args)
    { std::cout << "myPropEnum changed to: " << args.getValue() << "\n"; };

    // Procedure
    auto procProp = FunctionProperty("myPropProcedure", ProcedureInfo(List<IArgumentInfo>(ArgumentInfo("a", ctInt))));
    objPtr.addProperty(procProp);
    auto proc = Procedure([](IntegerPtr a) { std::cout << "Procedure called with: " << a << "\n"; });
    objPtr.setPropertyValue("myPropProcedure", proc);
    objPtr.getOnPropertyValueWrite("myPropProcedure") += [this](PropertyObjectPtr& /*obj*/, const PropertyValueEventArgsPtr& args)
    { std::cout << "myPropProcedure changed to: " << args.getValue() << "\n"; };

    // Function
    auto funProp =
        FunctionProperty("myPropFunction", FunctionInfo(ctInt, List<IArgumentInfo>(ArgumentInfo("a", ctInt), ArgumentInfo("b", ctInt))));
    objPtr.addProperty(funProp);
    auto fun = Function(
        [](IntegerPtr a, IntegerPtr b)
        {
            std::cout << "Function called\n";
            return a + b;
        });
    objPtr.setPropertyValue("myPropFunction", fun);
    objPtr.getOnPropertyValueWrite("myPropFunction") += [this](PropertyObjectPtr& /*obj*/, const PropertyValueEventArgsPtr& args)
    { std::cout << "myPropFunction changed to: " << args.getValue() << "\n"; };

    // Selection
    auto selectionProp = SelectionProperty("myPropSelection", List<IString>("first", "second", "third"), 1);
    objPtr.addProperty(selectionProp);
    objPtr.getOnPropertyValueWrite("myPropSelection") += [this](PropertyObjectPtr& /*obj*/, const PropertyValueEventArgsPtr& args)
    { std::cout << "myPropSelection changed to: " << args.getValue() << "\n"; };

    // Sparse selection
    auto selection = Dict<Int, IString>();
    selection.set(4, "first");
    selection.set(5, "second");
    selection.set(6, "third");
    auto sparseProp = SparseSelectionProperty("myPropSparse", selection, 4);
    objPtr.addProperty(sparseProp);
    objPtr.getOnPropertyValueWrite("myPropSparse") += [this](PropertyObjectPtr& /*obj*/, const PropertyValueEventArgsPtr& args)
    { std::cout << "myPropSparse changed to: " << args.getValue() << "\n"; };

    // Object
    auto innerObj = PropertyObject();
    innerObj.addProperty(BoolProperty("myBool", False));
    auto innerProp = ObjectProperty("myPropInnerObject", innerObj);
    auto propObj = PropertyObject();
    propObj.addProperty(innerProp);
    propObj.addProperty(IntProperty("myInt", 42));
    propObj.addProperty(FloatProperty("myFloat", 7.2));
    auto objProp = ObjectProperty("myPropObject", propObj);
    objPtr.addProperty(objProp);
    propObj.getOnPropertyValueWrite("myInt") += [this](PropertyObjectPtr& /*obj*/, const PropertyValueEventArgsPtr& args)
    { std::cout << "myPropObject.myPropInnerObject.myInt changed to: " << args.getValue() << "\n"; };
}

FunctionBlockTypePtr PropertiesFb::CreateType()
{
    return FunctionBlockType("PropertiesFb", "Properties", "Function Block focused on Properties");
}

END_NAMESPACE_PROPERTIES_MODULE
