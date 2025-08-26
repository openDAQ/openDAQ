/*
 * Properties Application: Container Types and Selection Properties
 * Demonstrates features like container types and selection properties usage
 */

#include <utils.h>

using namespace daq;

// Demonstrates how to configure a List property
inline void configureListProperty(const daq::PropertyObjectPtr& propObject)
{
    // Get Property by name
    auto property = propObject.getProperty("List");

    // Print some metadata
    std::cout << "Property value type: " << daq::coretype_utils::coreTypeToString(property.getValueType()) << "\n";
    std::cout << "List item type: " << daq::coretype_utils::coreTypeToString(property.getItemType()) << "\n";

    // Print old Property value
    std::cout << "Current List values: " << property.getValue() << "\n";

    // New value
    std::cout << "Configuring List property...\n";
    auto list = daq::List<daq::IInteger>();
    list.pushBack(32);
    list.pushBack(64);
    property.setValue(list);

    // Print updated Property value
    std::cout << "Updated List values: " << property.getValue() << "\n";

    // List properties can also be accessed by index using the syntax "List[index]" in the property name argument
    std::cout << "Second element in updated list: " << propObject.getPropertyValue("List[1]") << "\n\n";
}

// Demonstrates how to configure a Dict property
inline void configureDictProperty(const daq::PropertyObjectPtr& propObject)
{
    // Get Property by name
    auto property = propObject.getProperty("Dict");

    // Print some metadata
    auto valueType = property.getValueType();
    std::cout << "Property value type: " << daq::coretype_utils::coreTypeToString(valueType) << "\n";

    auto keyType = property.getKeyType();
    std::cout << "Dict key type: " << daq::coretype_utils::coreTypeToString(keyType) << "\n";

    auto itemType = property.getItemType();
    std::cout << "Dict item type: " << daq::coretype_utils::coreTypeToString(itemType) << "\n";

    // Print old Property
    std::cout << "Current Dict values:\n";
    printProperty(property, true);

    // New value
    std::cout << "Configuring Dict property...\n";
    auto dict = daq::Dict<daq::IString, daq::IString>();
    dict["key1"] = "Horse";
    dict["key2"] = "Excited";
    property.setValue(dict);

    // Print updated Property
    std::cout << "New Dict values:\n";
    printProperty(property);
}

// Demonstrates how to configure a Struct property
inline void configureStructProperty(const daq::PropertyObjectPtr& propObject, const daq::TypeManagerPtr& manager)
{
    // Get Property by name
    auto property = propObject.getProperty("Struct");

    // Print old Property
    std::cout << "Current Struct: " << "\n";
    printProperty(property, true);

    // New value (requires the Type Manager which stores the possible types)
    std::cout << "Configuring Struct property...\n";
    auto stru = StructBuilder("Struct", manager).set("Int", 100).set("String", "openDAQ").build();
    property.setValue(stru);

    // Print updated Property
    std::cout << "Updated Struct: " << "\n";
    printProperty(property);
}

// Demos how to modify a Struct property using an alternative overload on the builder
inline void reconfigureStructProperty(const daq::PropertyObjectPtr& propObject)
{
    // This overload is used to modify an existing Struct property (the Type Manager is not required this time)
    std::cout << "Reconfiguring Struct property...\n";
    auto struMod = propObject.getPropertyValue("Struct");
    auto stru = StructBuilder(struMod).set("Int", 200).set("String", "openDAQ modified").build();
    propObject.setPropertyValue("Struct", stru);

    // Print updated Property
    std::cout << "Updated Struct: " << "\n";
    printProperty(propObject.getProperty("Struct"));
}

// Demos how to modify an Enum property
inline void configureEnumProperty(const daq::PropertyObjectPtr& propObject, const daq::TypeManagerPtr& manager)
{
    // Get Property by name
    auto property = propObject.getProperty("Enum");

    // Print old Property
    std::cout << "Current Enum value: " << "\n";
    printProperty(property, true);

    // Configure an Enum Property with a new value
    std::cout << "Setting Enum property...\n";
    auto enumVal = Enumeration("Enum", "Third", manager);
    property.setValue(enumVal);

    // Print updated Property
    std::cout << "Updated Enum: " << "\n";
    printProperty(property);
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
    configureEnumProperty(fb, manager);

    // Selection
    configureBasicProperty(fb, "Selection", 2);

    // Sparse selection
    configureBasicProperty(fb, "Sparse", 2);

    // Gracefully exit
    std::cout << "Press \"enter\" to exit the application...\n";
    std::cin.get();
    return 0;
}
