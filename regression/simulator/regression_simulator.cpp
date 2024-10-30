#include <opendaq/opendaq.h>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <thread>
#include <iostream>
#include <coreobjects/argument_info_factory.h>
#include "../../build/__external/src/streaming_protocol/include/streaming_protocol/Types.h"
#include "opendaq/device_domain_factory.h"

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
    instance.addProperty(StructProperty("TestStruct", Struct("TestName", dict, daq::TypeManager())));
    // Add custom Argument Info Property
    auto argInfo = (nullptr, daq::CoreType::ctInt);
    auto argInfoProp = daq::PropertyBuilder("TestArgInfoProp").setDefaultValue(argInfo).setValueType(daq::CoreType::ctInt).build();
    instance.addProperty(argInfoProp);
    // Add custom Callable Info Property
    auto callInfo = CallableInfo_Create(list, daq::CoreType::ctInt, false);
    auto callInfoProp = daq::PropertyBuilder("TestCallInfoProp").setDefaultValue(callInfo).setValueType(daq::CoreType::ctString).build();
    instance.addProperty(callInfoProp);
    // Add custom Unit Property
    auto unit = daq::Unit("TestSymbol", -1, "TestName", "TestQuantity");
    auto unitProp = daq::PropertyBuilder("TestUnitProp").setDefaultValue(unit).setValueType(daq::ctStruct).build();
    instance.addProperty(unitProp);
    // Add custom Complex Number Property
    auto complex = daq::ComplexFloat32(31, 32);
    auto complexProp = daq::PropertyBuilder("TestComplexProp").setDefaultValue(complex).setValueType(daq::CoreType::ctComplexNumber).build();
    instance.addProperty(complexProp);
    // Add custom Ratio Property
    auto ratioProp = daq::RatioProperty("TestRatioProp", daq::Ratio(3, 4));
    instance.addProperty(ratioProp);
    // Add custom Function Block Type Property
    auto fbType = daq::FunctionBlockType("TestFunctionBlockId", "TestFunctionBlockName", "TestFunctionBlockDesc");
    auto fbTypeProp = daq::PropertyBuilder("TestFunctionBlockProp").setDefaultValue(fbType).setValueType(daq::ctStruct).build();
    instance.addProperty(fbTypeProp);
    // Add custom Data Descriptor Property
    auto dims = daq::List<daq::IDimension>();
    dims.pushBack(Dimension(daq::LinearDimensionRule(1, 2, 3), unit, daq::String("TestDimensionName")));
    auto dataDesc = daq::DataDescriptorBuilder()
                    .setDimensions(dims)
                    //.setMetadata()
                    .setName("TestDataDescriptor")
                    .setOrigin("1970")
                    //.setPostScaling(LinearScaling(2, 1))
                    .setRule(daq::LinearDataRule(2, 3))
                    .setSampleType(daq::SampleType::Float64)
                    //.setStructFields()
                    .setTickResolution(daq::Ratio(1, 666))
                    .setUnit(unit)
                        .setValueRange(daq::streaming_protocol::Range(20))
                    .build();
    auto dataDescProp = daq::PropertyBuilder("TestDataDescProp").setDefaultValue(dataDesc).setValueType(daq::CoreType::ctStruct).build();
    instance.addProperty(dataDescProp);
    // Add additional Data Descriptor Property (Post Scaling)
    auto altDataDesc =
        daq::DataDescriptorBuilder().setPostScaling(daq::LinearScaling(2, 1)).setSampleType(daq::SampleType::Float64).build();
    auto altDataDescProp =
        daq::PropertyBuilder("TestAltDataDescProp").setDefaultValue(altDataDesc).setValueType(daq::CoreType::ctStruct).build();
    instance.addProperty(altDataDescProp);
    // Add Device Domain Property
    auto deviceDomain = daq::DeviceDomain(daq::Ratio(3, 4), daq::String("1997"), unit);
    auto deviceDomainProp =
        daq::PropertyBuilder("TestDeviceDomainProp").setDefaultValue(deviceDomain).setValueType(daq::CoreType::ctStruct).build();
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
        std::this_thread::sleep_for(100);
    }

    return 0;
}
