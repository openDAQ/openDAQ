#include <properties_module/properties_fb_1.h>

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
}

FunctionBlockTypePtr PropertiesFb1::CreateType()
{
    return FunctionBlockType("PropertiesFb1", "Properties1", "Function Block focused on Properties 1");
}

END_NAMESPACE_PROPERTIES_MODULE
