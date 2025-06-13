#include <coreobjects/argument_info_factory.h>
#include <properties_module/properties_fb_2.h>

BEGIN_NAMESPACE_PROPERTIES_MODULE

PropertiesFb2::PropertiesFb2(const ContextPtr& ctx, const ComponentPtr& par, const StringPtr& locId)
    : FunctionBlock(CreateType(), ctx, par, locId)
{
    initProperties();
}

void PropertiesFb2::initProperties()
{
    // List (may contain other types) - used for storing multiple values of the same type
    auto list = List<IInteger>();
    auto listProp = ListProperty("List", list);
    objPtr.addProperty(listProp);

    // Dictionary - used for storing key-value pairs
    auto dict = Dict<IString, IString>();
    dict["Key1"] = "Cheese";
    dict["Key2"] = "Cake";
    dict["Key3"] = "Lady";
    auto dictProp = DictProperty("Dict", dict);
    objPtr.addProperty(dictProp);

    // Struct - used for grouping multiple properties of different types
    auto manager = context.getTypeManager();  // Struct type must be registered before creating the Struct
    manager.addType(StructType("Struct", List<IString>("Int", "String"), List<IType>(SimpleType(ctInt), SimpleType(ctString))));
    auto stru = StructBuilder("Struct", manager).set("Int", 42).set("String", "Flowers").build();
    auto structProp = StructProperty("Struct", stru);
    objPtr.addProperty(structProp);

    // Enumeration - used for defining a set of named values (such as values for a drop-down menu)
    auto enumNames = List<IString>();
    enumNames.pushBack("First");
    enumNames.pushBack("Second");
    enumNames.pushBack("Third");
    manager.addType(EnumerationType("Enum", enumNames));  // Must be registered before creating the Enumeration
    auto enu = Enumeration("Enum", "Second", manager);
    auto enumProp = EnumerationProperty("Enum", enu);
    objPtr.addProperty(enumProp);

    // Selection - used for selecting one value from a list of options
    auto selectionProp = SelectionProperty("Selection", List<IFloat>(41.1, 42.2, 43.3), 1);
    objPtr.addProperty(selectionProp);

    // Sparse selection - used for selecting one value from a sparse set of options
    auto selection = Dict<Int, IString>();
    selection.set(0, "First");
    selection.set(2, "Second");
    selection.set(4, "Third");
    auto sparseProp = SparseSelectionProperty("Sparse", selection, 4);
    objPtr.addProperty(sparseProp);
}

FunctionBlockTypePtr PropertiesFb2::CreateType()
{
    return FunctionBlockType("PropertiesFb2", "Properties2", "Function Block focused on Properties 2");
}

END_NAMESPACE_PROPERTIES_MODULE
