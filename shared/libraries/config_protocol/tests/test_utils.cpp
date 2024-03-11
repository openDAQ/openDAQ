#include "test_utils.h"
#include "coreobjects/argument_info_factory.h"
#include "coreobjects/callable_info_factory.h"
#include "coreobjects/coercer_factory.h"
#include "coreobjects/property_object_class_factory.h"
#include "coreobjects/validator_factory.h"
#include "opendaq/context_factory.h"
#include "opendaq/component_status_container_private_ptr.h"

namespace daq::config_protocol::test_utils
{

DevicePtr createServerDevice()
{
    const auto context = NullContext();
    const auto typeManager = context.getTypeManager();

    const auto obj = PropertyObject();
    obj.addProperty(StringProperty("NestedStringProperty", "string"));
    const auto mockClass = PropertyObjectClassBuilder("MockClass")
                               .addProperty(StringProperty("MockString", "string"))
                               .addProperty(ObjectProperty("MockChild", obj))
                               .build();

    typeManager.addType(mockClass);

    const auto serverDevice = createWithImplementation<IDevice, MockDevice2Impl>(context, nullptr, "root_dev");
    serverDevice.asPtr<IPropertyObjectInternal>().enableCoreEventTrigger();
    return serverDevice;
}

ComponentPtr createAdvancedPropertyComponent(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId)
{
    // TODO: Test struct and enum types once type manager is transferred.

    auto functionProp = FunctionProperty(
        "function", FunctionInfo(ctString, List<IArgumentInfo>(ArgumentInfo("int", ctInt), ArgumentInfo("float", ctFloat))));
    FunctionPtr funcCallback = Function(
        [](ListPtr<IBaseObject> args)
        {
            int intVal = args[0];
            double floatVal = args[1];

            if (floatVal > intVal)
                return String("Float is greater.");

            return String("Int is greater or equal.");
        });

    auto procProp =
        FunctionProperty("procedure",
                         ProcedureInfo(List<IArgumentInfo>(
                             ArgumentInfo("ratio", ctRatio),
                             ArgumentInfo("string", ctString),
                             ArgumentInfo("bool", ctBool))));
    ProcedurePtr procCallback = Procedure(
        [&](ListPtr<IBaseObject> /*args*/)
        {
        });

    const auto obj = PropertyObject();
    obj.addProperty(IntProperty("ObjNumber", 0));
    obj.addProperty(functionProp);
    obj.setPropertyValue("function", funcCallback);
    obj.addProperty(procProp);
    obj.setPropertyValue("procedure", procCallback);

    auto obj2 = PropertyObject();
    obj2.addProperty(StringProperty("String", "test"));
    auto obj1 = PropertyObject();
    obj1.addProperty(StringProperty("String", "test"));
    obj1.addProperty(ObjectProperty("Child", obj2));

    ComponentPtr component = Component(ctx, parent, localId);
    component.addProperty(IntPropertyBuilder("Integer", 1)
                                                   .setDescription("MyInteger")
                                                   .setMaxValue(10)
                                                   .setMinValue(0)
                                                   .setSuggestedValues(List<IInteger>(1, 3, 5, 7, 10))
                                                   .setUnit(EvalValue("Unit(%IntegerUnit:SelectedValue)"))
                                                   .build());
    component.addProperty(FloatPropertyBuilder("Float", EvalValue("$Integer - 0.123"))
                              .setDescription("MyFloat")
                              .setMaxValue(EvalValue("$Integer + 1"))
                              .setMinValue(0)
                              .setSuggestedValues(EvalValue("[1.0, 3.0, 5.0, 7.0, 10.0] * if($Integer < 5, 1.0, 0.5)"))
                              .build());
    component.addProperty(ListProperty("IntList", List<Int>(1, 2, 3, 4, 5)));
    component.addProperty(ListProperty("StringList", List<IString>("foo", "bar")));
    component.addProperty(BoolPropertyBuilder("BoolReadOnly", False).setReadOnly(True).build());
    component.addProperty(DictProperty("IntFloatDict", Dict<IInteger, IFloat>({{0, 1.123}, {2, 2.345}, {4, 3.456}})));
    component.addProperty(SelectionProperty("IntegerUnit", List<IString>("s", "ms"), 0, false));
    component.addProperty(SelectionProperty("Range", EvalValue("[50.0, 10.0, 1.0, 0.1] * if($Integer < 5, 1.0, 1.123)"), 0));
    component.addProperty(SparseSelectionProperty(
                 "StringSparseSelection", Dict<IInteger, IString>({{0, "foo"}, {10, "bar"}}), 10, EvalValue("$Integer < 5")));
    component.addProperty(ReferenceProperty("IntOrFloat", EvalValue("if($Float < 1, %Integer, %Float)")));
    component.addProperty(ObjectProperty("Object", obj));
    component.addProperty(StringPropertyBuilder("String", "foo").setReadOnly(EvalValue("$Float < 1.0")).build());
    component.addProperty(ReferenceProperty("ObjectOrString", EvalValue("if($Integer < 5, %Object, %String)")));
    component.addProperty(IntPropertyBuilder("ValidatedInt", 5).setValidator(Validator("Value < 10")).build());
    component.addProperty(IntPropertyBuilder("CoercedInt", 10).setCoercer(Coercer("if(Value > 10, 10, Value)")).build());
    component.addProperty(RatioProperty("Ratio", Ratio(1, 1000)));
    component.addProperty(ObjectPropertyBuilder("ObjectWithMetadata", obj1).setVisible(false).build());

    return component;
}

static PropertyObjectPtr createMockNestedPropertyObject()
{
    PropertyObjectPtr parent = PropertyObject();
    PropertyObjectPtr child1 = PropertyObject();
    PropertyObjectPtr child2 = PropertyObject();
    PropertyObjectPtr child1_1 = PropertyObject();
    PropertyObjectPtr child1_2 = PropertyObject();
    PropertyObjectPtr child1_2_1 = PropertyObject();
    PropertyObjectPtr child2_1 = PropertyObject();
    
    auto functionProp = FunctionProperty(
        "Function", FunctionInfo(ctInt, List<IArgumentInfo>(ArgumentInfo("int", ctInt))));
    auto procedureProp = FunctionProperty(
        "Procedure", ProcedureInfo(List<IArgumentInfo>(ArgumentInfo("int", ctInt))));

    FunctionPtr funcCallback = Function(
        [](const IntegerPtr& intVal)
        {
            return intVal;
        });

    ProcedurePtr procCallback = Procedure(
        [&](const IntegerPtr& intVal) {
            if (intVal < Integer(1))
                throw InvalidParameterException{};
        });

    child1_2_1.addProperty(StringProperty("String", "string"));
    child1_2_1.addProperty(StringPropertyBuilder("ReadOnlyString", "string").setReadOnly(true).build());
    child1_2_1.addProperty(functionProp);
    child1_2_1.addProperty(procedureProp);
    child1_2_1.setPropertyValue("Function", funcCallback);
    child1_2_1.setPropertyValue("Procedure", procCallback);

    child1_2.addProperty(ObjectProperty("child1_2_1", child1_2_1));
    child1_2.addProperty(IntProperty("Int", 1));

    child1_1.addProperty(FloatProperty("Float", 1.1));

    child1.addProperty(ObjectProperty("child1_1", child1_1));
    child1.addProperty(ObjectProperty("child1_2", child1_2));

    child2_1.addProperty(RatioProperty("Ratio", Ratio(1, 2)));

    child2.addProperty(ObjectProperty("child2_1", child2_1));

    parent.addProperty(ObjectProperty("child1", child1));
    parent.addProperty(ObjectProperty("child2", child2));

    return parent;
}

MockFb1Impl::MockFb1Impl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId)
    : FunctionBlock(FunctionBlockType("test_uid", "test_name", "test_description"), ctx, parent, localId, "MockClass")
{
    const auto sig1 = createAndAddSignal("sig1");
    const auto sig2 = createAndAddSignal("sig2");
    const auto sigDomain = createAndAddSignal("sig_domain");
    sig1.setDomainSignal(sigDomain);
    sig2.setDomainSignal(sigDomain);

    createAndAddInputPort("ip", PacketReadyNotification::None);
}

MockFb2Impl::MockFb2Impl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId)
    : FunctionBlock(FunctionBlockType("test_uid", "test_name", "test_description"), ctx, parent, localId, "MockClass")
{
    createAndAddSignal("sig");
    createAndAddInputPort("ip", PacketReadyNotification::None);
    const auto childFb = createWithImplementation<IFunctionBlock, MockFb1Impl>(ctx, this->functionBlocks, "childFb");
    addNestedFunctionBlock(childFb);
}

MockChannel1Impl::MockChannel1Impl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId)
    : Channel(FunctionBlockType("ch", "", ""), ctx, parent, localId, "MockClass")
{
    objPtr.addProperty(StringProperty("TestStringProp", "test"));
    objPtr.addProperty(BoolPropertyBuilder("TestStringPropWritten", false).setReadOnly(true).build());
    objPtr.getOnPropertyValueWrite("TestStringProp") +=
        [&](PropertyObjectPtr&, PropertyValueEventArgsPtr&) { objPtr.asPtr<IPropertyObjectProtected>().setProtectedPropertyValue("TestStringPropWritten", true); };
    const auto valueSig = createAndAddSignal("sig_ch");
    const auto domainSig = createAndAddSignal("sig_ch_time");
    valueSig.setDomainSignal(domainSig);
    const auto childFb = createWithImplementation<IFunctionBlock, MockFb1Impl>(ctx, this->functionBlocks, "childFb");
    addNestedFunctionBlock(childFb);
}

MockChannel2Impl::MockChannel2Impl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId)
    : Channel(FunctionBlockType("ch", "", ""), ctx, parent, localId, "MockClass")
{
    createAndAddSignal("sig_ch");
    createAndAddInputPort("ip", PacketReadyNotification::None);

    objPtr.addProperty(StringPropertyBuilder("StrProp", "-").build());
    objPtr.addProperty(StringPropertyBuilder("StrPropProtected", "").setReadOnly(True).build());
}

MockDevice1Impl::MockDevice1Impl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId)
    : Device(ctx, parent, localId, "MockClass")
    , ticksSinceOrigin(0)
{
    const auto sig = createAndAddSignal("sig_device");

    auto aiIoFolder = this->addIoFolder("ai", ioFolder);
    createAndAddChannel<MockChannel1Impl>(aiIoFolder, "ch");

    const auto fb = createWithImplementation<IFunctionBlock, MockFb1Impl>(ctx, this->functionBlocks, "fb");
    addNestedFunctionBlock(fb);

    fb.getInputPorts()[0].connect(sig);
}

DictPtr<IString, IFunctionBlockType> MockDevice1Impl::onGetAvailableFunctionBlockTypes()
{
    auto fbTypes = Dict<IString, IFunctionBlockType>({{"mockfb1", FunctionBlockType("mockfb1", "MockFB1", "Mock FB1", nullptr)}});
    return fbTypes;
}

FunctionBlockPtr MockDevice1Impl::onAddFunctionBlock(const StringPtr& typeId, const PropertyObjectPtr& config)
{
    if (typeId == "mockfb1")
    {
        if (!config.assigned())
            throw InvalidParameterException();

        const StringPtr param = config.getPropertyValue("Param");
        if (param != "Value")
            throw InvalidParameterException();

        const auto fb = createWithImplementation<IFunctionBlock, MockFb1Impl>(context, this->functionBlocks, "newFb");
        addNestedFunctionBlock(fb);
        return fb;
    }
    throw NotFoundException();
}

void MockDevice1Impl::onRemoveFunctionBlock(const FunctionBlockPtr& functionBlock)
{
    removeNestedFunctionBlock(functionBlock);
}

DeviceInfoPtr MockDevice1Impl::onGetInfo()
{
    const auto info = DeviceInfo("mock://dev1", "MockDevice1");
    info.setManufacturer("Testing");
    info.freeze();
    return info;
}

RatioPtr MockDevice1Impl::onGetResolution()
{
    return Ratio(1, 100);
}

uint64_t MockDevice1Impl::onGetTicksSinceOrigin()
{
    return ticksSinceOrigin++;
}

std::string MockDevice1Impl::onGetOrigin()
{
    return "N/A";
}

UnitPtr MockDevice1Impl::onGetDomainUnit()
{
    return Unit("s", -1, "second", "time");
}

MockDevice2Impl::MockDevice2Impl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId)
    : Device(ctx, parent, localId, "MockClass")
{
    createAndAddSignal("sig_device");

    auto aiIoFolder = this->addIoFolder("ai", ioFolder);
    createAndAddChannel<MockChannel2Impl>(aiIoFolder, "ch");
    addExistingComponent(createAdvancedPropertyComponent(ctx, thisPtr<ComponentPtr>(), "AdvancedPropertiesComponent"));
    const auto dev = createWithImplementation<IDevice, MockDevice1Impl>(ctx, this->devices, "dev");
    devices.addItem(dev);

	const auto structMembers = Dict<IString, IBaseObject>({{"string", "bar"}, {"integer", 10}, {"float", 5.123}});
	const auto defStructValue = Struct("FooStruct", structMembers, manager.getRef());

	objPtr.addProperty(StructPropertyBuilder("StructProp", defStructValue).build());
    
    objPtr.addProperty(StringPropertyBuilder("StrProp", "-").build());

    const auto statusType = EnumerationType("StatusType", List<IString>("Status0", "Status1"));
    if (!ctx.getTypeManager().hasType(statusType.getName()))
        ctx.getTypeManager().addType(statusType);

    const auto statusInitValue = Enumeration("StatusType", "Status0", ctx.getTypeManager());
    statusContainer.asPtr<IComponentStatusContainerPrivate>().addStatus("TestStatus", statusInitValue);

    this->objPtr.addProperty(ObjectProperty("ObjectProperty", createMockNestedPropertyObject()));
}

}
