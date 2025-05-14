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

    const auto ratioProp = RatioProperty("myPropRatio", Ratio(1, 12));
    objPtr.addProperty(ratioProp);
    objPtr.getOnPropertyValueWrite("myPropRatio") += [this](PropertyObjectPtr& obj, const PropertyValueEventArgsPtr& args)
    { std::cout << "myPropRatio changed to: " << args.getValue() << "\n"; };

    auto list = List<IInteger>();
    list.pushBack(1);
    list.pushBack(2);
    list.pushBack(3);
    const auto listProp = ListProperty("myPropList", list);
    objPtr.addProperty(listProp);
    objPtr.getOnPropertyValueWrite("myPropList") += [this](PropertyObjectPtr& obj, const PropertyValueEventArgsPtr& args)
    { std::cout << "myPropList changed to: " << args.getValue() << "\n"; };

    auto dict = Dict<IString, IString>();
    dict["key1"] = "cheese";
    dict["key2"] = "cake";
    dict["key3"] = "lady";
    const auto dictProp = DictProperty("myPropDict", dict);
    objPtr.addProperty(dictProp);
    objPtr.getOnPropertyValueWrite("myPropDict") += [this](PropertyObjectPtr& obj, const PropertyValueEventArgsPtr& args)
    {
        DictPtr<IString, IString> dict = args.getValue();
        std::cout << "myPropDict changed to: " << "\n";
        for (const auto& item : dict)
        {
            std::cout << "  " << item.first << ": " << item.second << "\n";
        }
    };

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
    myRatio = objPtr.getPropertyValue("myPropRatio");
    myList = objPtr.getPropertyValue("myPropList");
    myDict = objPtr.getPropertyValue("myPropDict");
}

FunctionBlockTypePtr PropertiesFb::CreateType()
{
    return FunctionBlockType("PropertiesFb", "Properties", "Function Block focused on Properties");
}

END_NAMESPACE_PROPERTIES_MODULE
