#include "setup_regression.h"

class RegressionTestSerialization : public testing::Test
{
private:
    ModuleManagerPtr moduleManager;
    ContextPtr context;
    InstancePtr instance;

protected:
    DevicePtr device;

    void SetUp() override
    {
        PROTOCOLS("nd")

        moduleManager = ModuleManager("");
        context = Context(nullptr, Logger(), TypeManager(), moduleManager, nullptr, nullptr, nullptr);

        instance = InstanceCustom(context, "mock_instance");

        device = instance.addDevice(connectionString);
    }
};

TEST_F(RegressionTestSerialization, deserializeArgInfo)
{
    PROTOCOLS("nd")

    // Get Property
    auto prop = device.getProperty("TestArgInfoProp");

    // Check expected default
    ArgumentInfoPtr expectedDefault = ArgumentInfo_Create(String("TestArgInfo"), CoreType::ctInt);
    ArgumentInfoPtr defaultVal = prop.getDefaultValue();
    ASSERT_EQ(expectedDefault, defaultVal);

    // Check Property value
    ASSERT_EQ(defaultVal, device.getPropertyValue("TestArgInfoProp"));

    // Set new value and check
    ArgumentInfoPtr newVal = ArgumentInfo_Create(String("NewTestArgInfo"), CoreType::ctFloat);
    device.setPropertyValue("TestArgInfoProp", newVal);
    ASSERT_EQ(newVal, device.getPropertyValue("TestArgInfoProp"));
}

TEST_F(RegressionTestSerialization, deserializeCallInfo)
{
    PROTOCOLS("nd")

    // Get Property
    auto prop = device.getProperty("TestCallInfoProp");

    // Check expected default
    auto list = List<IInteger>();
    list.pushBack(1);
    list.pushBack(2);
    list.pushBack(3);
    CallableInfoPtr expectedDefault = CallableInfo_Create(list, CoreType::ctInt);
    CallableInfoPtr defaultVal = prop.getDefaultValue();
    ASSERT_EQ(expectedDefault, defaultVal);

    // Check Property value
    ASSERT_EQ(defaultVal, device.getPropertyValue("TestCallInfoProp"));

    // Set new value and check
    auto newList = List<IInteger>();
    newList.pushBack(4);
    newList.pushBack(2);
    newList.pushBack(6);
    CallableInfoPtr newVal = CallableInfo_Create(newList, CoreType::ctInt);
    device.setPropertyValue("TestCallInfoProp", newVal);
    ASSERT_EQ(newVal, device.getPropertyValue("TestCallInfoProp"));
}

TEST_F(RegressionTestSerialization, deserializeUnit)
{
    PROTOCOLS("nd")

    // Get Property
    auto prop = device.getProperty("TestUnitProp");

    // Check expected default
    UnitPtr expectedDefault = Unit("TestSymbol", -1, "TestName", "TestQuantity");
    UnitPtr defaultVal = prop.getDefaultValue();
    ASSERT_EQ(expectedDefault, defaultVal);

    // Check Property value
    ASSERT_EQ(defaultVal, device.getPropertyValue("TestUnitProp"));

    // Set new value and check
    UnitPtr newVal = Unit("NewTestSymbol", -1, "NewTestName", "NewTestQuantity");
    device.setPropertyValue("TestUnitProp", newVal);
    ASSERT_EQ(newVal, device.getPropertyValue("TestUnitProp"));
}

TEST_F(RegressionTestSerialization, deserializeComplexNumber)
{
    PROTOCOLS("nd")

    // Get Property
    auto prop = device.getProperty("TestComplexProp");

    // Check expected default
    ComplexNumberPtr expectedDefault = ComplexFloat32(31, 32);
    ComplexNumberPtr defaultVal = prop.getDefaultValue();
    ASSERT_EQ(expectedDefault, defaultVal);

    // Check Property value
    ASSERT_EQ(defaultVal, device.getPropertyValue("TestComplexProp"));

    // Set new value and check
    ComplexNumberPtr newVal = ComplexFloat32(33, 34);
    device.setPropertyValue("TestComplexProp", newVal);
    ASSERT_EQ(newVal, device.getPropertyValue("TestComplexProp"));
}

TEST_F(RegressionTestSerialization, deserializeRatio)
{
    PROTOCOLS("nd")

    // Get Property
    auto prop = device.getProperty("TestRatioProp");

    // Check expected default
    RatioPtr expectedDefault = Ratio(3, 4);
    RatioPtr defaultVal = prop.getDefaultValue();
    ASSERT_EQ(expectedDefault, defaultVal);

    // Check Property value
    ASSERT_EQ(defaultVal, device.getPropertyValue("TestRatioProp"));

    // Set new value and check
    RatioPtr newVal = Ratio(8, 6);
    device.setPropertyValue("TestRatioProp", newVal);
    ASSERT_EQ(newVal, device.getPropertyValue("TestRatioProp"));
}

TEST_F(RegressionTestSerialization, deserializeFunctionBlockType)
{
    PROTOCOLS("nd")

    // Get Property
    auto prop = device.getProperty("TestFunctionBlockProp");

    // Check expected default
    FunctionBlockTypePtr expectedDefault = FunctionBlockType("TestFunctionBlockId", "TestFunctionBlockName", "TestFunctionBlockDesc");
    FunctionBlockTypePtr defaultVal = prop.getDefaultValue();
    ASSERT_EQ(expectedDefault, defaultVal);

    // Check Property value
    ASSERT_EQ(defaultVal, device.getPropertyValue("TestFunctionBlockProp"));

    // Set new value and check
    FunctionBlockTypePtr newVal = FunctionBlockType("NewTestFunctionBlockId", "NewTestFunctionBlockName", "NewTestFunctionBlockDesc");
    device.setPropertyValue("TestFunctionBlockProp", newVal);
    ASSERT_EQ(newVal, device.getPropertyValue("TestFunctionBlockProp"));
}

TEST_F(RegressionTestSerialization, deserializeDataDescriptor)
{
    PROTOCOLS("nd")

    // Get Property
    auto prop = device.getProperty("TestDataDescProp");

    // Check expected default
    auto unit = Unit("TestSymbol", -1, "TestName", "TestQuantity");
    auto dims = List<IDimension>();
    dims.pushBack(Dimension_Create(LinearDimensionRule(1, 2, 3), unit, String("TestDimensionName")));
    DataDescriptorPtr expectedDefault = DataDescriptorBuilder()
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
    DataDescriptorPtr defaultVal = prop.getDefaultValue();
    ASSERT_EQ(expectedDefault, defaultVal);

    // Check Property value
    ASSERT_EQ(defaultVal, device.getPropertyValue("TestDataDescProp"));

    // Set new value and check
    auto newUnit = Unit("NewTestSymbol", -1, "NewTestName", "NewTestQuantity");
    auto newDims = List<IDimension>();
    newDims.pushBack(Dimension_Create(LinearDimensionRule(4, 5, 6), newUnit, String("NewTestDimensionName")));
    DataDescriptorPtr newVal = DataDescriptorBuilder()
                                   .setDimensions(newDims)
                                   //.setMetadata()
                                   .setName("NewTestDataDescriptor")
                                   .setOrigin("1971")
                                   //.setPostScaling(LinearScaling(2, 1))
                                   .setRule(LinearDataRule(3, 4))
                                   .setSampleType(SampleType::Float32)
                                   //.setStructFields()
                                   .setTickResolution(Ratio(3, 500))
                                   .setUnit(newUnit)
                                   .setValueRange(Range(-43, 43))
                                   .build();
    device.setPropertyValue("TestDataDescProp", newVal);
    ASSERT_EQ(newVal, device.getPropertyValue("TestDataDescProp"));
}

TEST_F(RegressionTestSerialization, deserializeAltDataDescriptor)
{
    PROTOCOLS("nd")

    // Get Property
    auto prop = device.getProperty("TestAltDataDescProp");

    // Check expected default
    DataDescriptorPtr expectedDefault =
        DataDescriptorBuilder().setPostScaling(LinearScaling(2, 1)).setSampleType(SampleType::Float64).build();
    DataDescriptorPtr defaultVal = prop.getDefaultValue();
    ASSERT_EQ(expectedDefault, defaultVal);

    // Check Property value
    ASSERT_EQ(defaultVal, device.getPropertyValue("TestAltDataDescProp"));

    // Set new value and check
    DataDescriptorPtr newVal = DataDescriptorBuilder().setPostScaling(LinearScaling(4, 5)).setSampleType(SampleType::Float64).build();
    device.setPropertyValue("TestAltDataDescProp", newVal);
    ASSERT_EQ(newVal, device.getPropertyValue("TestAltDataDescProp"));
}

TEST_F(RegressionTestSerialization, deserializeDeviceDomain)
{
    PROTOCOLS("nd")

    // Get Property
    auto prop = device.getProperty("TestDeviceDomainProp");

    // Check expected default
    auto unit = Unit("TestSymbol", -1, "TestName", "TestQuantity");
    DeviceDomainPtr expectedDefault = DeviceDomain_Create(Ratio(3, 4), String("1997"), unit);
    DeviceDomainPtr defaultVal = prop.getDefaultValue();
    ASSERT_EQ(expectedDefault, defaultVal);

    // Check Property value
    ASSERT_EQ(defaultVal, device.getPropertyValue("TestDeviceDomainProp"));

    // Set new value and check
    auto newUnit = Unit("NewTestSymbol", -1, "NewTestName", "NewTestQuantity");
    DeviceDomainPtr newVal = DeviceDomain_Create(Ratio(4, 5), String("1998"), newUnit);
    device.setPropertyValue("TestDeviceDomainProp", newVal);
    ASSERT_EQ(newVal, device.getPropertyValue("TestDeviceDomainProp"));
}
