#include <copendaq.h>

#include <gtest/gtest.h>

using COpendaqSignalTest = testing::Test;

Context* createContext()
{
    Context* ctx = nullptr;
    List* sinks = nullptr;
    List_createList(&sinks);

    LoggerSink* sink = nullptr;
    LoggerSink_createStdErrLoggerSink(&sink);
    List_pushBack(sinks, sink);
    BaseObject_releaseRef(sink);

    Logger* logger = nullptr;
    Logger_createLogger(&logger, sinks, LogLevel::LogLevelDebug);
    BaseObject_releaseRef(sinks);

    TypeManager* typeManager = nullptr;
    TypeManager_createTypeManager(&typeManager);

    Dict *options = nullptr, *discoveryServers = nullptr;
    Dict_createDict(&options);
    Dict_createDict(&discoveryServers);

    Context_createContext(&ctx, nullptr, logger, typeManager, nullptr, nullptr, options, discoveryServers);

    BaseObject_releaseRef(discoveryServers);
    BaseObject_releaseRef(options);
    BaseObject_releaseRef(typeManager);
    BaseObject_releaseRef(logger);

    return ctx;
}

DataDescriptor* createValueDescriptor()
{
    DataDescriptor* descriptor = nullptr;
    DataDescriptorBuilder* builder = nullptr;
    DataDescriptorBuilder_createDataDescriptorBuilder(&builder);
    DataDescriptorBuilder_setSampleType(builder, SampleType::SampleTypeInt64);

    UnitBuilder* unitBuilder = nullptr;
    UnitBuilder_createUnitBuilder(&unitBuilder);

    String* unitName = nullptr;
    String_createString(&unitName, "volts");
    UnitBuilder_setName(unitBuilder, unitName);
    BaseObject_releaseRef(unitName);

    String* unitSymbol = nullptr;
    String_createString(&unitSymbol, "V");
    UnitBuilder_setSymbol(unitBuilder, unitSymbol);
    BaseObject_releaseRef(unitSymbol);

    String* unitQuantity = nullptr;
    String_createString(&unitQuantity, "voltage");
    UnitBuilder_setQuantity(unitBuilder, unitQuantity);
    BaseObject_releaseRef(unitQuantity);

    UnitBuilder_setId(unitBuilder, -1);

    Unit* unit = nullptr;
    UnitBuilder_build(unitBuilder, &unit);
    DataDescriptorBuilder_setUnit(builder, unit);
    BaseObject_releaseRef(unitBuilder);
    BaseObject_releaseRef(unit);

    String* name = nullptr;
    String_createString(&name, "vals");
    DataDescriptorBuilder_setName(builder, name);
    BaseObject_releaseRef(name);

    DataDescriptorBuilder_build(builder, &descriptor);
    BaseObject_releaseRef(builder);

    return descriptor;
}

DataDescriptor* createDomainDescriptor()
{
    DataDescriptor* descriptor = nullptr;

    DataDescriptorBuilder* builder = nullptr;
    DataDescriptorBuilder_createDataDescriptorBuilder(&builder);

    DataDescriptorBuilder_setSampleType(builder, SampleType::SampleTypeInt64);

    UnitBuilder* unitBuilder = nullptr;
    UnitBuilder_createUnitBuilder(&unitBuilder);

    String* unitName = nullptr;
    String_createString(&unitName, "seconds");
    UnitBuilder_setName(unitBuilder, unitName);
    BaseObject_releaseRef(unitName);

    String* unitSymbol = nullptr;
    String_createString(&unitSymbol, "s");
    UnitBuilder_setSymbol(unitBuilder, unitSymbol);
    BaseObject_releaseRef(unitSymbol);

    String* unitQuantity = nullptr;
    String_createString(&unitQuantity, "time");
    UnitBuilder_setQuantity(unitBuilder, unitQuantity);
    BaseObject_releaseRef(unitQuantity);

    UnitBuilder_setId(unitBuilder, -1);

    Unit* unit = nullptr;
    UnitBuilder_build(unitBuilder, &unit);
    BaseObject_releaseRef(unitBuilder);

    DataDescriptorBuilder_setUnit(builder, unit);
    BaseObject_releaseRef(unit);

    String* name = nullptr;
    String_createString(&name, "time");
    DataDescriptorBuilder_setName(builder, name);
    BaseObject_releaseRef(name);

    Ratio* ratio = nullptr;
    Ratio_createRatio(&ratio, 1, 1000);
    DataDescriptorBuilder_setTickResolution(builder, ratio);
    BaseObject_releaseRef(ratio);

    Integer* delta = nullptr;
    Integer_createInteger(&delta, 1);
    Integer* start = nullptr;
    Integer_createInteger(&start, 0);

    Number* deltaNum = nullptr;
    BaseObject_queryInterface(delta, NUMBER_INTF_ID, reinterpret_cast<void**>(&deltaNum));
    Number* startNum = nullptr;
    BaseObject_queryInterface(start, NUMBER_INTF_ID, reinterpret_cast<void**>(&startNum));

    BaseObject_releaseRef(delta);
    BaseObject_releaseRef(start);

    DataRule* rule = nullptr;
    DataRule_createLinearDataRule(&rule, deltaNum, startNum);
    BaseObject_releaseRef(deltaNum);
    BaseObject_releaseRef(startNum);

    DataDescriptorBuilder_setRule(builder, rule);
    BaseObject_releaseRef(rule);

    String* origin = nullptr;
    String_createString(&origin, "2025-01-01T00:00:00Z");
    DataDescriptorBuilder_setOrigin(builder, origin);
    BaseObject_releaseRef(origin);

    DataDescriptorBuilder_build(builder, &descriptor);
    BaseObject_releaseRef(builder);

    return descriptor;
}

TEST_F(COpendaqSignalTest, Allocator)
{
    Allocator* allocator = nullptr;
    Allocator_createMallocAllocator(&allocator);
    ASSERT_NE(allocator, nullptr);

    DataDescriptor* valueDescriptor = createValueDescriptor();

    void* address = nullptr;
    ErrCode err = Allocator_allocate(allocator, valueDescriptor, 32, 4, &address);

    ASSERT_EQ(err, 0);
    ASSERT_NE(address, nullptr);

    err = Allocator_free(allocator, address);
    ASSERT_EQ(err, 0);

    BaseObject_releaseRef(allocator);
    BaseObject_releaseRef(valueDescriptor);
}

TEST_F(COpendaqSignalTest, DataDescriptor)
{
    DataDescriptor* valueDescriptor = createValueDescriptor();

    String* name = nullptr;
    DataDescriptor_getName(valueDescriptor, &name);
    ConstCharPtr nameStr = nullptr;
    String_getCharPtr(name, &nameStr);
    ASSERT_STREQ(nameStr, "vals");
    BaseObject_releaseRef(name);

    Unit* unit = nullptr;
    DataDescriptor_getUnit(valueDescriptor, &unit);
    String* symbol = nullptr;
    Unit_getSymbol(unit, &symbol);
    ConstCharPtr symbolStr = nullptr;
    String_getCharPtr(symbol, &symbolStr);
    ASSERT_STREQ(symbolStr, "V");
    BaseObject_releaseRef(symbol);
    BaseObject_releaseRef(unit);

    SampleType sampleType = SampleType::SampleTypeNull;
    DataDescriptor_getSampleType(valueDescriptor, &sampleType);
    ASSERT_EQ(sampleType, SampleType::SampleTypeInt64);

    BaseObject_releaseRef(valueDescriptor);
}

TEST_F(COpendaqSignalTest, DataPacket)
{
    DataDescriptor* valueDescriptor = createValueDescriptor();
    DataPacket* packet = nullptr;

    Integer* offset = nullptr;
    Integer_createInteger(&offset, 0);
    Number* offsetNum = nullptr;
    BaseObject_queryInterface(offset, NUMBER_INTF_ID, reinterpret_cast<void**>(&offsetNum));
    BaseObject_releaseRef(offset);

    SizeT sampleCount = 10;
    DataPacket_createDataPacket(&packet, valueDescriptor, sampleCount, offsetNum);
    BaseObject_releaseRef(offsetNum);

    void* data = nullptr;
    DataPacket_getRawData(packet, &data);
    ASSERT_NE(data, nullptr);

    BaseObject_releaseRef(packet);
    BaseObject_releaseRef(valueDescriptor);
}

TEST_F(COpendaqSignalTest, DimensionRule)
{
    DimensionRuleBuilder* builder = nullptr;
    DimensionRuleBuilder_createDimensionRuleBuilder(&builder);
    DimensionRuleBuilder_setType(builder, DimensionRuleType::DimensionRuleTypeLinear);
    Integer* delta = nullptr;
    Integer_createInteger(&delta, 1);
    Integer* start = nullptr;
    Integer_createInteger(&start, 0);
    Integer* size = nullptr;
    Integer_createInteger(&size, 10);
    Number* deltaNum = nullptr;
    BaseObject_queryInterface(delta, NUMBER_INTF_ID, reinterpret_cast<void**>(&deltaNum));
    Number* startNum = nullptr;
    BaseObject_queryInterface(start, NUMBER_INTF_ID, reinterpret_cast<void**>(&startNum));
    Number* sizeNum = nullptr;
    BaseObject_queryInterface(size, NUMBER_INTF_ID, reinterpret_cast<void**>(&sizeNum));
    BaseObject_releaseRef(delta);
    BaseObject_releaseRef(start);
    BaseObject_releaseRef(size);

    String* deltaStr = nullptr;
    String_createString(&deltaStr, "delta");
    String* startStr = nullptr;
    String_createString(&startStr, "start");
    String* sizeStr = nullptr;
    String_createString(&sizeStr, "size");
    DimensionRuleBuilder_addParameter(builder, sizeStr, sizeNum);
    DimensionRuleBuilder_addParameter(builder, deltaStr, deltaNum);
    DimensionRuleBuilder_addParameter(builder, startStr, startNum);
    BaseObject_releaseRef(deltaNum);
    BaseObject_releaseRef(startNum);
    BaseObject_releaseRef(sizeNum);

    DimensionRule* rule = nullptr;
    DimensionRuleBuilder_build(builder, &rule);
    BaseObject_releaseRef(builder);

    ASSERT_NE(rule, nullptr);

    DimensionRuleType ruleType = DimensionRuleType::DimensionRuleTypeOther;
    DimensionRule_getType(rule, &ruleType);

    ASSERT_EQ(ruleType, DimensionRuleType::DimensionRuleTypeLinear);

    Dict* params = nullptr;
    DimensionRule_getParameters(rule, &params);
    ASSERT_NE(params, nullptr);

    Number* outDelta = nullptr;
    Dict_get(params, deltaStr, reinterpret_cast<BaseObject**>(&outDelta));
    Number* outStart = nullptr;
    Dict_get(params, startStr, reinterpret_cast<BaseObject**>(&outStart));
    Number* outSize = nullptr;
    Dict_get(params, sizeStr, reinterpret_cast<BaseObject**>(&outSize));

    Int deltaInt = -1;
    Int startInt = -1;
    Int sizeInt = -1;
    Number_getIntValue(outDelta, &deltaInt);
    Number_getIntValue(outStart, &startInt);
    Number_getIntValue(outSize, &sizeInt);
    ASSERT_EQ(deltaInt, 1);
    ASSERT_EQ(startInt, 0);
    ASSERT_EQ(sizeInt, 10);

    BaseObject_releaseRef(outSize);
    BaseObject_releaseRef(outDelta);
    BaseObject_releaseRef(outStart);
    BaseObject_releaseRef(params);
    BaseObject_releaseRef(rule);
    BaseObject_releaseRef(deltaStr);
    BaseObject_releaseRef(startStr);
    BaseObject_releaseRef(sizeStr);
}

TEST_F(COpendaqSignalTest, EventPacket)
{
    EventPacket* packet = nullptr;
    DataDescriptor* valueDescriptor = createValueDescriptor();
    DataDescriptor* domainDescriptor = createDomainDescriptor();
    EventPacket_createDataDescriptorChangedEventPacket(&packet, valueDescriptor, domainDescriptor);

    ASSERT_NE(packet, nullptr);
    String* id = nullptr;
    EventPacket_getEventId(packet, &id);
    ConstCharPtr idStr = nullptr;
    String_getCharPtr(id, &idStr);
    ASSERT_STREQ(idStr, "DATA_DESCRIPTOR_CHANGED");

    BaseObject_releaseRef(id);
    BaseObject_releaseRef(packet);
    BaseObject_releaseRef(valueDescriptor);
    BaseObject_releaseRef(domainDescriptor);
}

TEST_F(COpendaqSignalTest, InputPort)
{
    InputPortConfig* inputPortConfig = nullptr;
    Context* ctx = createContext();
    String* id = nullptr;
    String_createString(&id, "InputPort");
    InputPortConfig_createInputPort(&inputPortConfig, ctx, nullptr, id, False);
    ASSERT_NE(inputPortConfig, nullptr);

    BaseObject_releaseRef(id);
    BaseObject_releaseRef(ctx);
    BaseObject_releaseRef(inputPortConfig);
}

TEST_F(COpendaqSignalTest, Range)
{
    Range* range = nullptr;
    Integer* lowValue = nullptr;
    Integer_createInteger(&lowValue, 0);
    Integer* highValue = nullptr;
    Integer_createInteger(&highValue, 10);
    Number* lowValueNum = nullptr;
    BaseObject_queryInterface(lowValue, NUMBER_INTF_ID, reinterpret_cast<void**>(&lowValueNum));
    BaseObject_releaseRef(lowValue);
    Number* highValueNum = nullptr;
    BaseObject_queryInterface(highValue, NUMBER_INTF_ID, reinterpret_cast<void**>(&highValueNum));
    BaseObject_releaseRef(highValue);
    Range_createRange(&range, lowValueNum, highValueNum);
    BaseObject_releaseRef(lowValueNum);
    BaseObject_releaseRef(highValueNum);

    ASSERT_NE(range, nullptr);

    Number* outLowValue = nullptr;
    Number* outHighValue = nullptr;
    Range_getLowValue(range, &outLowValue);
    Range_getHighValue(range, &outHighValue);
    Int lowValueInt = -1;
    Int highValueInt = -1;
    Number_getIntValue(outLowValue, &lowValueInt);
    Number_getIntValue(outHighValue, &highValueInt);
    ASSERT_EQ(lowValueInt, 0);
    ASSERT_EQ(highValueInt, 10);

    BaseObject_releaseRef(outHighValue);
    BaseObject_releaseRef(outLowValue);
    BaseObject_releaseRef(range);
}

TEST_F(COpendaqSignalTest, Scaling)
{
    ScalingBuilder* builder = nullptr;
    ScalingBuilder_createScalingBuilder(&builder);
    ScalingBuilder_setInputDataType(builder, SampleType::SampleTypeInt16);
    ScalingBuilder_setOutputDataType(builder, ScaledSampleType::ScaledSampleTypeFloat32);
    ScalingBuilder_setScalingType(builder, ScalingType::ScalingTypeLinear);

    Dict* params = nullptr;
    Dict_createDict(&params);

    String* scaleStr = nullptr;
    String_createString(&scaleStr, "scale");
    String* offsetStr = nullptr;
    String_createString(&offsetStr, "offset");

    Integer* scale = nullptr;
    Integer_createInteger(&scale, 10);
    Integer* offset = nullptr;
    Integer_createInteger(&offset, 10);

    Dict_set(params, scaleStr, scale);
    Dict_set(params, offsetStr, offset);
    ScalingBuilder_setParameters(builder, params);
    BaseObject_releaseRef(scale);
    BaseObject_releaseRef(offset);
    BaseObject_releaseRef(scaleStr);
    BaseObject_releaseRef(offsetStr);

    Scaling* scaling = nullptr;
    ScalingBuilder_build(builder, &scaling);
    BaseObject_releaseRef(builder);

    ASSERT_NE(scaling, nullptr);

    ScalingType scalingType = ScalingType::ScalingTypeOther;
    Scaling_getType(scaling, &scalingType);
    ASSERT_EQ(scalingType, ScalingType::ScalingTypeLinear);
    SampleType inputSampleType = SampleType::SampleTypeNull;
    Scaling_getInputSampleType(scaling, &inputSampleType);
    ASSERT_EQ(inputSampleType, SampleType::SampleTypeInt16);
    ScaledSampleType outputSampleType = ScaledSampleType::ScaledSampleTypeInvalid;
    Scaling_getOutputSampleType(scaling, &outputSampleType);
    ASSERT_EQ(outputSampleType, ScaledSampleType::ScaledSampleTypeFloat32);
    Dict* scalingParams = nullptr;
    Scaling_getParameters(scaling, &scalingParams);
    ASSERT_NE(scalingParams, nullptr);

    Bool equal = False;
    BaseObject_equals(scalingParams, params, &equal);
    ASSERT_EQ(equal, True);

    BaseObject_releaseRef(scalingParams);
    BaseObject_releaseRef(scaling);
    BaseObject_releaseRef(params);
}

TEST_F(COpendaqSignalTest, Signal)
{
    SignalConfig* signalConfig = nullptr;
    String* id = nullptr;
    String_createString(&id, "sig");
    Context* ctx = createContext();
    SignalConfig_createSignal(&signalConfig, ctx, nullptr, id, nullptr);
    ASSERT_NE(signalConfig, nullptr);
    BaseObject_releaseRef(id);
    BaseObject_releaseRef(ctx);
    BaseObject_releaseRef(signalConfig);
}
