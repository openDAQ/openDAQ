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
    const auto boolProp = BoolProperty("myPropBool", False);
    objPtr.addProperty(boolProp);
    objPtr.getOnPropertyValueWrite("myPropBool") += [this](PropertyObjectPtr& obj, const PropertyValueEventArgsPtr& args)
    { std::cout << "myPropBool changed to: " << args.getValue() << "\n"; };

    const auto intProp = IntProperty("myPropInt", 42);
    objPtr.addProperty(intProp);
    objPtr.getOnPropertyValueWrite("myPropInt") += [this](PropertyObjectPtr& obj, const PropertyValueEventArgsPtr& args)
    { std::cout << "myPropInt changed to: " << args.getValue() << "\n"; };

    const auto floatProp = FloatProperty("myPropFloat", 7.2);
    objPtr.addProperty(floatProp);
    objPtr.getOnPropertyValueWrite("myPropFloat") += [this](PropertyObjectPtr& obj, const PropertyValueEventArgsPtr& args)
    { std::cout << "myPropFloat changed to: " << args.getValue() << "\n"; };

    const auto stringProp = StringProperty("myPropString", "Hello World");
    objPtr.addProperty(stringProp);
    objPtr.getOnPropertyValueWrite("myPropString") += [this](PropertyObjectPtr& obj, const PropertyValueEventArgsPtr& args)
    { std::cout << "myPropString changed to: " << args.getValue() << "\n"; };

    readProperties();
}

void PropertiesFb::propertyChanged()
{
    readProperties();
}

void PropertiesFb::readProperties()
{
    myBool = objPtr.getPropertyValue("myPropBool");
    myInt = objPtr.getPropertyValue("myPropInt");
    myFloat = objPtr.getPropertyValue("myPropFloat");
    myString = objPtr.getPropertyValue("myPropString");
}

FunctionBlockTypePtr PropertiesFb::CreateType()
{
    return FunctionBlockType("PropertiesFb", "Properties", "Function Block focused on Properties");
}

END_NAMESPACE_PROPERTIES_MODULE
