/*
 * Properties Application: Container types and selection properties
 * Demonstrates features like container types and selection properties usage
 */

#include <utils.h>

using namespace daq;

// Demonstrates how to configure a List property
void configureListProperty(const PropertyObjectPtr& propObject)
{
    std::cout << "Configuring List property...\n";
    auto prop = propObject.getProperty("List");

    std::cout << "Property value type: " << coreTypeToString(prop.getValueType()) << "\n";
    std::cout << "List item type: " << coreTypeToString(prop.getItemType()) << "\n";

    ListPtr<IInteger> currentValues = propObject.getPropertyValue("List");
    std::cout << "Current List values: " << currentValues << "\n";

    // New value
    auto list = List<IInteger>();
    list.pushBack(32);
    list.pushBack(64);
    propObject.setPropertyValue("List", list);

    std::cout << "Updated List values: " << propObject.getPropertyValue("List") << "\n";

    // List properties can also be accessed by index using the syntax "List[index]" in the property name argument
    std::cout << "Second element in updated list: " << propObject.getPropertyValue("List[1]") << "\n";
}

// Demonstrates how to configure a Dict property
void configureDictProperty(const PropertyObjectPtr& propObject)
{
    std::cout << "Configuring Dict property...\n";
    auto prop = propObject.getProperty("Dict");

    auto valueType = prop.getValueType();
    std::cout << "Property value type: " << coreTypeToString(valueType) << "\n";

    auto keyType = prop.getKeyType();
    std::cout << "Dict key type: " << coreTypeToString(keyType) << "\n";

    auto itemType = prop.getItemType();
    std::cout << "Dict item type: " << coreTypeToString(itemType) << "\n";

    auto currentDict = propObject.getProperty("Dict");
    std::cout << "Current Dict values:\n";
    printProperty(currentDict);

    // New value
    auto dict = Dict<IString, IString>();
    dict["key1"] = "Horse";
    dict["key2"] = "Excited";
    propObject.setPropertyValue("Dict", dict);

    auto newDict = propObject.getProperty("Dict");
    std::cout << "New Dict values:\n";
    printProperty(newDict);
}

// Demonstrates how to configure a Struct property
void configureStructProperty(const PropertyObjectPtr& propObject, const TypeManagerPtr& manager)
{
    std::cout << "Configuring Struct property...\n";
    auto prop = propObject.getProperty("Struct");

    printProperty(prop);

    // New value (requires the Type Manager which stores the possible types)
    auto stru = StructBuilder("Struct", manager).set("Int", 100).set("String", "openDAQ").build();
    propObject.setPropertyValue("Struct", stru);

    std::cout << "Updated Struct values: " << "\n";
    auto newStruct = propObject.getProperty("Struct");
    printProperty(newStruct);
}

// Demos how to modify a Struct property using an alternative overload on the builder
void reconfigureStructProperty(const PropertyObjectPtr& propObject)
{
    std::cout << "Reconfiguring Struct property...\n";

    // This overload is used to modify an existing Struct property (the Type Manager is not required this time)
    auto struMod = propObject.getPropertyValue("Struct");
    auto structBuild = StructBuilder(struMod).set("Int", 200).set("String", "openDAQ modified").build();
    propObject.setPropertyValue("Struct", structBuild);

    auto newProp = propObject.getPropertyValue("Struct");

    std::cout << "Updated Struct values: " << "\n";
    printProperty(propObject.getProperty("Struct"));
}

void configureEnum(const PropertyObjectPtr& propObject, const TypeManagerPtr& manager)
{
    std::cout << "Current Enum value: " << "\n";
    auto value = propObject.getPropertyValue("Enum");
    printProperty(value);

    std::cout << "Configuring Enum property...\n";
    auto enumVal = Enumeration("Enum", "Third", manager);
    propObject.setPropertyValue("Enum", enumVal);

    std::cout << "New Enum value: " << "\n";
    auto newValue = propObject.getPropertyValue("Enum").asPtr<IEnumeration>();
    printProperty(newValue);
}

int main(int /*argc*/, const char* /*argv*/[])
{
    // Create an Instance, loading modules in the default module path
    const InstancePtr instance = Instance();

    // Pretty print
    std::cout << "\nopenDAQ(TM) Properties Application: Container Types and Selection Properties\n\n";

    // Add Function Block by type ID
    auto fb = instance.addFunctionBlock("ExampleFBPropertyContainerTypes");

    // List
    configureListProperty(fb);

    // Dictionary
    configureDictProperty(fb);

    // For Struct and Enum Properties, the types must be first be registered in the Type Manager
    auto manager = instance.getContext().getTypeManager();

    // Struct
    configureStructProperty(fb, manager);

    // Struct reconfiguration via builder
    reconfigureStructProperty(fb);

    // Enumeration
    configureEnum(fb, manager);

    // Selection
    fb.setPropertyValue("Selection", 2);

    // Sparse selection
    fb.setPropertyValue("Sparse", 2);

    // Gracefully exit
    std::cout << "Press \"enter\" to exit the application...\n";
    std::cin.get();
    return 0;
}
