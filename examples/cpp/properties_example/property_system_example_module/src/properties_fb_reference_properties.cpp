#include <property_system_example_module/properties_fb_reference_properties.h>

BEGIN_NAMESPACE_PROPERTIES_MODULE

ExampleFBPropertyReferenceProperties::ExampleFBPropertyReferenceProperties(const ContextPtr& ctx,
                                                                           const ComponentPtr& par,
                                                                           const StringPtr& locId)
    : FunctionBlock(CreateType(), ctx, par, locId)
{
    initProperties();
}

void ExampleFBPropertyReferenceProperties::initProperties()
{
    // Referenced Bool - used for demo purposes fo referencing another Property
    auto referencedProp = BoolProperty("Referenced", False);
    objPtr.addProperty(referencedProp);

    // Reference Bool, and using EvalValue syntax
    auto referenceProp = ReferenceProperty("Reference", EvalValue("%Referenced"));
    objPtr.addProperty(referenceProp);

    // Selection - used for selecting one value from a list of options
    auto selectionProp = SelectionProperty("UnitSelection", List<IUnit>(Unit("FirstUnit"), Unit("SecondUnit"), Unit("ThirdUnit")), 1);
    objPtr.addProperty(selectionProp);

    // Property visibility depending on another Property, and using EvalValue syntax
    auto sometimesVisibleProperty =
        IntPropertyBuilder("SometimesVisible", 3)
            .setVisible(EvalValue("$Referenced"))                // This will evaluate referenced Property
            .setUnit(EvalValue("%UnitSelection:SelectedValue"))  // This will set unit to the selected value of the Selection Property
            .build();
    objPtr.addProperty(sometimesVisibleProperty);

    // Coerced Int - used to demonstrate coercion of Property values
    auto coercedProp = IntPropertyBuilder("CoercedProp", 5).setCoercer(Coercer("if(Value > 10, 10, Value)")).build();
    objPtr.addProperty(coercedProp);

    // Read-only Int - used to demonstrate a read-only Property
    auto readOnlyProp = IntPropertyBuilder("ReadOnlyInt", 42).setReadOnly(true).build();
    objPtr.addProperty(readOnlyProp);

    // Validated Int - used to demonstrate validation of Property values
    auto validatedProp = IntPropertyBuilder("ValidatedProp", 42).setValidator(Validator("Value < 100")).build();
    objPtr.addProperty(validatedProp);

    // Demonstrate property ordering
    // First, properties are in insertion order by default
    // Then set a custom order that groups related properties together
    auto customOrder = List<IString>("UnitSelection",     // Selection properties first
                                     "SometimesVisible",  // Then properties that depend on selection
                                     "Referenced",        // Then reference source
                                     "Reference",         // Then reference target
                                     "ReadOnlyInt",       // Then read-only properties
                                     "CoercedProp",       // Then properties with coercion
                                     "ValidatedProp"      // Finally properties with validation
    );
    objPtr.setPropertyOrder(customOrder);
}

FunctionBlockTypePtr ExampleFBPropertyReferenceProperties::CreateType()
{
    return FunctionBlockType(
        "ExampleFBPropertyReferenceProperties", "ExampleFBPropertyReferenceProperties", "Function Block focused on reference Properties");
}

END_NAMESPACE_PROPERTIES_MODULE
