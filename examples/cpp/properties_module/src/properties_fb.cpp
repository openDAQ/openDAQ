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

void PropertiesFb::addPropertyAndCallback(const PropertyPtr& prop)
{
    objPtr.addProperty(prop);
    auto name = prop.getName();
    objPtr.getOnPropertyValueWrite(name) += [name](PropertyObjectPtr& /*obj*/, const PropertyValueEventArgsPtr& args)
    { std::cout << name << " changed to: " << args.getValue() << "\n"; };
}

void PropertiesFb::initProperties()
{
    // Bool
    auto boolProp = BoolPropertyBuilder("Bool", False)
                        .setDescription("A very nice boolean")  //  Description is optional, without it, we could forego using the builder
                        .build();
    addPropertyAndCallback(boolProp);

    // Int
    auto intProp = IntPropertyBuilder("Int", 42).setUnit(Unit("Unit")).build();  // Unit is optional
    addPropertyAndCallback(intProp);

    // Float
    auto floatProp = FloatProperty("Float", 7.2);
    addPropertyAndCallback(floatProp);

    // String
    auto stringProp = StringProperty("String", "Hello World");
    addPropertyAndCallback(stringProp);

    // Ratio
    auto ratioProp = RatioProperty("Ratio", Ratio(1, 12));
    addPropertyAndCallback(ratioProp);

    // List (may contain other types)
    auto list = List<IInteger>();
    auto listProp = ListProperty("List", list);
    addPropertyAndCallback(listProp);

    // Dictionary (associative array, key-value pairs, may contain other types)
    auto dict = Dict<IString, IString>();
    dict["key1"] = "Cheese";
    dict["key2"] = "Cake";
    dict["key3"] = "Lady";
    auto dictProp = DictProperty("Dict", dict);

    // This one has a special callback
    objPtr.addProperty(dictProp);
    objPtr.getOnPropertyValueWrite("Dict") += [](PropertyObjectPtr& /*obj*/, const PropertyValueEventArgsPtr& args)
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
    addPropertyAndCallback(structProp);

    // Enumeration
    auto enumNames = List<IString>();
    enumNames.pushBack("First");
    enumNames.pushBack("Second");
    enumNames.pushBack("Third");
    manager.addType(EnumerationType("Enum", enumNames));
    auto enu = Enumeration("Enum", "Second", manager);
    auto enumProp = EnumerationProperty("Enum", enu);
    addPropertyAndCallback(enumProp);

    // Procedure
    auto procProp = FunctionProperty("Procedure", ProcedureInfo(List<IArgumentInfo>(ArgumentInfo("a", ctInt))));
    addPropertyAndCallback(procProp);
    auto proc = Procedure([](IntegerPtr a) { std::cout << "Procedure called with: " << a << "\n"; });
    objPtr.setPropertyValue("Procedure", proc);

    // Function
    auto funProp =
        FunctionProperty("Function", FunctionInfo(ctInt, List<IArgumentInfo>(ArgumentInfo("a", ctInt), ArgumentInfo("b", ctInt))));
    addPropertyAndCallback(funProp);
    auto fun = Function(
        [](IntegerPtr a, IntegerPtr b)
        {
            std::cout << "Function called\n";
            return a + b;
        });
    objPtr.setPropertyValue("Function", fun);

    // Selection
    auto selectionProp = SelectionProperty("Selection", List<IUnit>(Unit("FirstUnit"), Unit("SecondUnit"), Unit("ThirdUnit")), 1);
    addPropertyAndCallback(selectionProp);

    // Sparse selection
    auto selection = Dict<Int, IString>();
    selection.set(4, "First");
    selection.set(5, "Second");
    selection.set(6, "Third");
    auto sparseProp = SparseSelectionProperty("Sparse", selection, 4);
    addPropertyAndCallback(sparseProp);

    // Object
    auto innerObj = PropertyObject();
    innerObj.addProperty(BoolProperty("Bool", False));
    auto innerProp = ObjectProperty("InnerObject", innerObj);
    auto propObj = PropertyObject();
    propObj.addProperty(innerProp);
    propObj.addProperty(IntProperty("Int", 42));
    propObj.addProperty(FloatProperty("Float", 7.2));
    auto objProp = ObjectProperty("Object", propObj);
    addPropertyAndCallback(objProp);

    // Referenced Bool
    auto referencedProp = BoolProperty("Referenced", False);
    addPropertyAndCallback(referencedProp);

    // Reference Bool, and using EvalValue syntax
    auto referenceProp = ReferenceProperty("Reference", EvalValue("%Referenced"));
    addPropertyAndCallback(referenceProp);

    // Property visibility depending on another Property, and using EvalValue syntax
    auto sometimesVisibleProperty = IntPropertyBuilder("SometimesVisible", 3)
                                        .setVisible(EvalValue("$Referenced"))  // This will evaluate referenced Property
                                        .setUnit(EvalValue("%Selection:SelectedValue"))
                                        .build();
    addPropertyAndCallback(sometimesVisibleProperty);

    // Stubborn Int
    auto stubbornProp = IntProperty("StubbornInt", 42);

    // This one has a special callback
    objPtr.addProperty(stubbornProp);
    objPtr.getOnPropertyValueWrite("StubbornInt") += [this](PropertyObjectPtr& /*obj*/, const PropertyValueEventArgsPtr& args)
    {
        args.setValue(43);  // This will set the value to 43, even if the user tried to set it to something else
        std::cout << "StubbornInt changed to: " << args.getValue() << "\n";
    };

    // Read-only Int
    auto readOnlyProp = IntPropertyBuilder("ReadOnlyInt", 42).setReadOnly(true).build();
    objPtr.addProperty(readOnlyProp);
    // No point in adding an on-write callback for a read-only property, as it cannot be changed by the user

    // Coerced Int
    auto coercedProp = IntPropertyBuilder("CoercedProp", 5).setCoercer(Coercer("if(Value > 10, 10, Value)")).build();
    addPropertyAndCallback(coercedProp);

    // Validated Int
    auto validatedProp = IntPropertyBuilder("ValidatedProp", 42).setValidator(Validator("Value < 100")).build();
    addPropertyAndCallback(validatedProp);

    // Min and max Float
    auto minMaxProp = FloatPropertyBuilder("MinMaxProp", 0.0).setMinValue(0.0).setMaxValue(100.0).build();
    addPropertyAndCallback(minMaxProp);

    // Suggested values Float
    auto suggestedProp = FloatPropertyBuilder("SuggestedProp", 2.2).setSuggestedValues(List<IFloat>(1.1, 2.2, 3.3)).build();
    addPropertyAndCallback(suggestedProp);
}

FunctionBlockTypePtr PropertiesFb::CreateType()
{
    return FunctionBlockType("PropertiesFb", "Properties", "Function Block focused on Properties");
}

END_NAMESPACE_PROPERTIES_MODULE
