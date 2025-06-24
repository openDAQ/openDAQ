#include <properties_module/properties_fb_1.h>
#include <iostream>

BEGIN_NAMESPACE_PROPERTIES_MODULE
PropertiesFb1::PropertiesFb1(const ContextPtr& ctx, const ComponentPtr& par, const StringPtr& locId)
    : FunctionBlock(CreateType(), ctx, par, locId)
{
    initProperties();
}

void PropertiesFb1::initProperties()
{
    // Bool - used for properties with two states, like on/off, true/false, etc.
    auto boolProp = BoolPropertyBuilder("Bool", False)
                        .setDescription("A very nice boolean")  //  Description is optional, without it, we could forego using the builder
                        .build();
    objPtr.addProperty(boolProp);

    // Int - used for properties that hold integer values, like counts, indices, etc.
    auto intProp = IntPropertyBuilder("Int", 42).setUnit(Unit("Unit")).build();  // Unit is optional
    objPtr.addProperty(intProp);

    // Float - used for properties that hold floating-point values, like measurements, etc.
    auto floatProp = FloatProperty("Float", 7.2);
    objPtr.addProperty(floatProp);

    // String - used for properties that hold text values, like names, descriptions, etc.
    auto stringProp = StringProperty("String", "Hello World");
    objPtr.addProperty(stringProp);

    // Ratio - used for properties that hold ratios, like fractions, proportions, etc.
    auto ratioProp = RatioProperty("Ratio", Ratio(1, 12));
    objPtr.addProperty(ratioProp);

    // Min and max Float
    auto minMaxProp = FloatPropertyBuilder("MinMaxProp", 50.0).setMinValue(0.0).setMaxValue(100.0).build();
    objPtr.addProperty(minMaxProp);

    // Suggested values Float
    auto suggestedProp = FloatPropertyBuilder("SuggestedProp", 2.2).setSuggestedValues(List<IFloat>(1.1, 2.2, 3.3)).build();
    objPtr.addProperty(suggestedProp);

    // Stubborn Int (always sets to 43 - we set this up below)
    auto stubbornProp = IntProperty("StubbornInt", 43);
    objPtr.addProperty(stubbornProp);

    // We can add callbacks

    // Read callbacks will mess with our prints in application if enabled, so we comment them out
    // objPtr.getOnAnyPropertyValueRead() += [](PropertyObjectPtr&, const PropertyValueEventArgsPtr&) { std::cout << "Something read\n"; };
    // objPtr.getOnPropertyValueRead("Bool") += [](PropertyObjectPtr&, const PropertyValueEventArgsPtr&) { std::cout << "Bool read\n"; };
    objPtr.getOnAnyPropertyValueWrite() += [](PropertyObjectPtr&, const PropertyValueEventArgsPtr&) { std::cout << "Something written\n"; };
    objPtr.getOnPropertyValueWrite("StubbornInt") += [](PropertyObjectPtr&, const PropertyValueEventArgsPtr& args)
    {
        args.setValue(43);  // Force value to 43
        std::cout << "StubbornInt changed to: " << args.getValue() << "\n";
    };
}

FunctionBlockTypePtr PropertiesFb1::CreateType()
{
    return FunctionBlockType("PropertiesFb1", "Properties1", "Function Block focused on Properties 1");
}

END_NAMESPACE_PROPERTIES_MODULE
