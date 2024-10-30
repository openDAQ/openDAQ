#include <opendaq/opendaq.h>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <thread>
#include <iostream>
#include <coreobjects/argument_info_factory.h>

int main (int /*argc*/, const char* /*argv*/[])
{
    // Set up the simulator

    // Create Instance
    auto instance = daq::Instance("");
    // Set Root Device
    instance.setRootDevice("daqref://device1");
    // Unlock all attributes (needed for OPC UA: RegressionTestComponent/setVisibleGetVisible)
    auto componentPrivate = instance.getRootDevice().asPtr<daq::IComponentPrivate>();
    componentPrivate.unlockAllAttributes();
    // Add Trigger Reference Function Block
    instance.addFunctionBlock("ref_fb_module_trigger");
    // Add all current servers
    instance.addServer("openDAQ OpcUa", nullptr);
    instance.addServer("openDAQ Native Streaming", nullptr);
    instance.addServer("openDAQ LT Streaming", nullptr);
    // Add custom String Property
    auto stringProperty = daq::StringPropertyBuilder("TestString", "TestDefaultString").setDescription("TestDescription").build();
    instance.addProperty(stringProperty);
    // Add custom Dictionary Property
    auto dict = daq::Dict<daq::IString, daq::IString>();
    dict.set("TestKey", "TestValue");
    instance.addProperty(DictProperty("TestDict", dict, true));
    // Add custom Int Property
    auto list = daq::List<daq::IInteger>();
    list.pushBack(1);
    list.pushBack(2);
    list.pushBack(3);
    instance.addProperty(daq::IntPropertyBuilder("TestInt", 42)
                         .setUnit(daq::Unit("TestUnit", -1, "TestName", "TestQunatity"))
                         .setMinValue(666)
                         .setMaxValue(777)
                         .setSuggestedValues(list)
                         .build());
    // Add custom Selection Property
    instance.addProperty(SelectionProperty("TestSelection", list, 2, true));
    // Add custom Reference Property
    instance.addProperty(ReferenceProperty("TestReference", daq::EvalValue("%TestString")));
    // Add custom Struct Property
    instance.addProperty(StructProperty("TestStruct", Struct("TestName", dict, TypeManager())));
    // Add custom Argument Info Property
    auto argInfo = ArgumentInfo(String("TestArgInfo"), CoreType::ctInt);
    auto argInfoProp = PropertyBuilder("TestArgInfoProp").setDefaultValue(argInfo).setValueType(CoreType::ctStruct).build();
    instance.addProperty(argInfoProp);
    // Add custom Callable Info Property
    auto callInfo = CallableInfo_Create(list, CoreType::ctInt, false);
    auto callInfoProp = PropertyBuilder("TestCallInfoProp").setDefaultValue(callInfo).setValueType(CoreType::ctStruct).build();
    instance.addProperty(callInfoProp);
    // Add custom Unit Property
    auto unit = Unit("TestSymbol", -1, "TestName", "TestQuantity");
    auto unitProp = PropertyBuilder("TestUnitProp").setDefaultValue(unit).setValueType(CoreType::ctStruct).build();
    instance.addProperty(unitProp);
    // Add custom Complex Number Property
    auto complex = ComplexFloat32(31, 32);
    auto complexProp = PropertyBuilder("TestComplexProp").setDefaultValue(complex).setValueType(CoreType::ctComplexNumber).build();
    instance.addProperty(complexProp);
    // Add custom Ratio Property
    auto ratioProp = RatioProperty("TestRatioProp", Ratio(3, 4));
    instance.addProperty(ratioProp);
    // Add custom Function Block Type Property
    auto fbType = FunctionBlockType("TestFunctionBlockId", "TestFunctionBlockName", "TestFunctionBlockDesc");
    auto fbTypeProp = PropertyBuilder("TestFunctionBlockProp").setDefaultValue(fbType).setValueType(CoreType::ctStruct).build();
    instance.addProperty(fbTypeProp);
    // Add custom Data Descriptor Property
    auto dims = List<IDimension>();
    dims.pushBack(Dimension(LinearDimensionRule(1, 2, 3), unit, String("TestDimensionName")));
    auto dataDesc = DataDescriptorBuilder()
                        .setDimensions(dims)
                        //.setMetadata()
                        .setName("TestDataDescriptor")
                        .setOrigin("1970")
                        //.setPostScaling(LinearScaling(2, 1))
                        .setRule(LinearDataRule(2, 3))
                        .setSampleType(SampleType::Float64)
                        //.setStructFields()
                        .setTickResolution(Ratio(1, 666))
                        .setUnit(unit)
                        .setValueRange(Range(-35, 35))
                        .build();
    auto dataDescProp = PropertyBuilder("TestDataDescProp").setDefaultValue(dataDesc).setValueType(CoreType::ctStruct).build();
    instance.addProperty(dataDescProp);
    // Add additional Data Descriptor Property (Post Scaling)
    auto altDataDesc = DataDescriptorBuilder().setPostScaling(LinearScaling(2, 1)).setSampleType(SampleType::Float64).build();
    auto altDataDescProp = PropertyBuilder("TestAltDataDescProp").setDefaultValue(altDataDesc).setValueType(CoreType::ctStruct).build();
    instance.addProperty(altDataDescProp);
    // Add Device Domain Property
    auto deviceDomain = DeviceDomain(Ratio(3, 4), String("1997"), unit);
    auto deviceDomainProp = PropertyBuilder("TestDeviceDomainProp").setDefaultValue(deviceDomain).setValueType(CoreType::ctStruct).build();
    instance.addProperty(deviceDomainProp);

    // Create an empty file named "ready" to let regression test suite know
    // the simulator is up and running and ready for tests
    std::ofstream ready;
    ready.open("ready", std::ios::out);
    ready.close();

    // GitHub Action will delete the "ready" file after
    // the tests for one protocol are done, which means
    // we can then gracefully shut down the simulator
    while (std::filesystem::exists("ready"))
    {
        std::this_thread::sleep_for(100ms);
    }

    return 0;
}
