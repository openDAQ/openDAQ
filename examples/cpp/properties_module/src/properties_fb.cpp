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
    const auto boolProp = BoolProperty("myPropBool", False);
    objPtr.addProperty(boolProp);

    // Int
    const auto intProp = IntProperty("myPropInt", 42);
    objPtr.addProperty(intProp);

    // Float
    const auto floatProp = FloatProperty("myPropFloat", 7.2);
    objPtr.addProperty(floatProp);

    // String
    const auto stringProp = StringProperty("myPropString", "Hello World");
    objPtr.addProperty(stringProp);

    // Ratio
    const auto ratioProp = RatioProperty("myPropRatio", Ratio(1, 12));
    objPtr.addProperty(ratioProp);

    // List
    auto list = List<IInteger>();
    const auto listProp = ListProperty("myPropList", list);
    objPtr.addProperty(listProp);

    // Dictionary
    auto dict = Dict<IString, IString>();
    dict["key1"] = "cheese";
    dict["key2"] = "cake";
    dict["key3"] = "lady";
    const auto dictProp = DictProperty("myPropDict", dict);
    objPtr.addProperty(dictProp);

    // Struct
    auto manager = context.getTypeManager();
    manager.addType(StructType("myStruct", List<IString>("myInt", "myString"), List<IType>(SimpleType(ctInt), SimpleType(ctString))));
    auto stru = StructBuilder("myStruct", manager).set("myInt", 42).set("myString", "flowers").build();
    const auto structProp = StructProperty("myPropStruct", stru);
    objPtr.addProperty(structProp);

    // Enumeration
    auto enumNames = List<IString>();
    enumNames.pushBack("first");
    enumNames.pushBack("second");
    enumNames.pushBack("third");
    manager.addType(EnumerationType("myEnum", enumNames));
    auto enu = Enumeration("myEnum", "second", manager);
    const auto enumProp = EnumerationProperty("myPropEnum", enu);
    objPtr.addProperty(enumProp);

    // Procedure
    auto procProp = FunctionProperty("myPropProcedure", ProcedureInfo(List<IArgumentInfo>(ArgumentInfo("a", ctInt))));
    objPtr.addProperty(procProp);
    auto proc = Procedure([](IntegerPtr a) { std::cout << "Procedure called with: " << a << "\n"; });
    objPtr.setPropertyValue("myPropProcedure", proc);

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

    // Selection
    auto selectionProp = SelectionProperty("myPropSelection", List<IString>("first", "second", "third"), 1);
    objPtr.addProperty(selectionProp);

    // Sparse selection
    auto selection = Dict<Int, IString>();
    selection.set(4, "first");
    selection.set(5, "second");
    selection.set(6, "third");
    auto sparseProp = SparseSelectionProperty("myPropSparse", selection, 4);
    objPtr.addProperty(sparseProp);

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
    myProcedure = objPtr.getPropertyValue("myPropProcedure");
    myFunction = objPtr.getPropertyValue("myPropFunction");
    mySelection = objPtr.getPropertyValue("myPropSelection");
    mySparse = objPtr.getPropertyValue("myPropSparse");
    myObject = objPtr.getPropertyValue("myPropObject");
}

FunctionBlockTypePtr PropertiesFb::CreateType()
{
    return FunctionBlockType("PropertiesFb", "Properties", "Function Block focused on Properties");
}

END_NAMESPACE_PROPERTIES_MODULE
