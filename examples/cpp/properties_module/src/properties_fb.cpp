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
    const auto boolProp = BoolProperty("myPropBool", False);
    objPtr.addProperty(boolProp);
    objPtr.getOnPropertyValueWrite("myPropBool") += [this](PropertyObjectPtr& /*obj*/, const PropertyValueEventArgsPtr& args)
    { std::cout << "myPropBool changed to: " << args.getValue() << "\n"; };

    // Int
    const auto intProp = IntProperty("myPropInt", 42);
    objPtr.addProperty(intProp);
    objPtr.getOnPropertyValueWrite("myPropInt") += [this](PropertyObjectPtr& /*obj*/, const PropertyValueEventArgsPtr& args)
    { std::cout << "myPropInt changed to: " << args.getValue() << "\n"; };

    // Float
    const auto floatProp = FloatProperty("myPropFloat", 7.2);
    objPtr.addProperty(floatProp);
    objPtr.getOnPropertyValueWrite("myPropFloat") += [this](PropertyObjectPtr& /*obj*/, const PropertyValueEventArgsPtr& args)
    { std::cout << "myPropFloat changed to: " << args.getValue() << "\n"; };

    // String
    const auto stringProp = StringProperty("myPropString", "Hello World");
    objPtr.addProperty(stringProp);
    objPtr.getOnPropertyValueWrite("myPropString") += [this](PropertyObjectPtr& /*obj*/, const PropertyValueEventArgsPtr& args)
    { std::cout << "myPropString changed to: " << args.getValue() << "\n"; };

    // Ratio
    const auto ratioProp = RatioProperty("myPropRatio", Ratio(1, 12));
    objPtr.addProperty(ratioProp);
    objPtr.getOnPropertyValueWrite("myPropRatio") += [this](PropertyObjectPtr& /*obj*/, const PropertyValueEventArgsPtr& args)
    { std::cout << "myPropRatio changed to: " << args.getValue() << "\n"; };

    // List
    auto list = List<IInteger>();
    const auto listProp = ListProperty("myPropList", list);
    objPtr.addProperty(listProp);
    objPtr.getOnPropertyValueWrite("myPropList") += [this](PropertyObjectPtr& /*obj*/, const PropertyValueEventArgsPtr& args)
    { std::cout << "myPropList changed to: " << args.getValue() << "\n"; };

    // Dictionary
    auto dict = Dict<IString, IString>();
    dict["key1"] = "cheese";
    dict["key2"] = "cake";
    dict["key3"] = "lady";
    const auto dictProp = DictProperty("myPropDict", dict);
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
    const auto structProp = StructProperty("myPropStruct", stru);
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
    const auto enumProp = EnumerationProperty("myPropEnum", enu);
    objPtr.addProperty(enumProp);
    objPtr.getOnPropertyValueWrite("myPropEnum") += [this](PropertyObjectPtr& /*obj*/, const PropertyValueEventArgsPtr& args)
    { std::cout << "myPropEnum changed to: " << args.getValue() << "\n"; };

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
    myStruct = objPtr.getPropertyValue("myPropStruct");
    myEnum = objPtr.getPropertyValue("myPropEnum");
}

FunctionBlockTypePtr PropertiesFb::CreateType()
{
    return FunctionBlockType("PropertiesFb", "Properties", "Function Block focused on Properties");
}

END_NAMESPACE_PROPERTIES_MODULE
