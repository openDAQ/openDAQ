#include <coreobjects/argument_info_factory.h>
#include <coreobjects/callable_info_factory.h>
#include <properties_module/properties_fb_1.h>
#include <iostream>

BEGIN_NAMESPACE_PROPERTIES_MODULE

PropertiesFb1::PropertiesFb1(const ContextPtr& ctx, const ComponentPtr& par, const StringPtr& locId)
    : FunctionBlock(CreateType(), ctx, par, locId)
{
    initProperties();
}

void PropertiesFb1::addPropertyAndCallback(const PropertyPtr& prop)
{
    objPtr.addProperty(prop);
    auto name = prop.getName();
    objPtr.getOnPropertyValueWrite(name) += [name](PropertyObjectPtr&, const PropertyValueEventArgsPtr& args)
    { std::cout << name << " changed to: " << args.getValue() << "\n"; };
}

void PropertiesFb1::initProperties()
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
}

FunctionBlockTypePtr PropertiesFb1::CreateType()
{
    return FunctionBlockType("PropertiesFb1", "Properties1", "Function Block focused on Properties 1");
}

END_NAMESPACE_PROPERTIES_MODULE
