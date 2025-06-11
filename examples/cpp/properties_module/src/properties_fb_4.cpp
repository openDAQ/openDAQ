#include <properties_module/properties_fb_4.h>

BEGIN_NAMESPACE_PROPERTIES_MODULE

PropertiesFb4::PropertiesFb4(const ContextPtr& ctx, const ComponentPtr& par, const StringPtr& locId)
    : FunctionBlock(CreateType(), ctx, par, locId)
{
    initProperties();
}

void PropertiesFb4::initProperties()
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
            .setVisible(EvalValue("$Referenced"))            // This will evaluate referenced Property
            .setUnit(EvalValue("%UnitSelection:SelectedValue"))  // This will set unit to the selected value of the Selection Property
            .build();
    objPtr.addProperty(sometimesVisibleProperty);

    // Read-only Int
    auto readOnlyProp = IntPropertyBuilder("ReadOnlyInt", 42).setReadOnly(true).build();
    objPtr.addProperty(readOnlyProp);

    // Coerced Int
    auto coercedProp = IntPropertyBuilder("CoercedProp", 5).setCoercer(Coercer("if(Value > 10, 10, Value)")).build();
    objPtr.addProperty(coercedProp);

    // Validated Int
    auto validatedProp = IntPropertyBuilder("ValidatedProp", 42).setValidator(Validator("Value < 100")).build();
    objPtr.addProperty(validatedProp);
}

FunctionBlockTypePtr PropertiesFb4::CreateType()
{
    return FunctionBlockType("PropertiesFb4", "Properties4", "Function Block focused on Properties 4");
}

END_NAMESPACE_PROPERTIES_MODULE
