#include <copendaq.h>

#include <gtest/gtest.h>

using COpendaqSignalTest = testing::Test;

daqContext* createContext()
{
    daqContext* ctx = nullptr;
    daqList* sinks = nullptr;
    daqList_createList(&sinks);

    daqLoggerSink* sink = nullptr;
    daqLoggerSink_createStdErrLoggerSink(&sink);
    daqList_pushBack(sinks, sink);
    daqBaseObject_releaseRef(sink);

    daqLogger* logger = nullptr;
    daqLogger_createLogger(&logger, sinks, daqLogLevel::daqLogLevelDebug);
    daqBaseObject_releaseRef(sinks);

    daqTypeManager* typeManager = nullptr;
    daqTypeManager_createTypeManager(&typeManager);

    daqDict *options = nullptr, *discoveryServers = nullptr;
    daqDict_createDict(&options);
    daqDict_createDict(&discoveryServers);

    daqContext_createContext(&ctx, nullptr, logger, typeManager, nullptr, nullptr, options, discoveryServers);

    daqBaseObject_releaseRef(discoveryServers);
    daqBaseObject_releaseRef(options);
    daqBaseObject_releaseRef(typeManager);
    daqBaseObject_releaseRef(logger);

    return ctx;
}

daqDataDescriptor* createValueDescriptor()
{
    daqDataDescriptor* descriptor = nullptr;
    daqDataDescriptorBuilder* builder = nullptr;
    daqDataDescriptorBuilder_createDataDescriptorBuilder(&builder);
    daqDataDescriptorBuilder_setSampleType(builder, daqSampleType::daqSampleTypeInt64);

    daqUnitBuilder* unitBuilder = nullptr;
    daqUnitBuilder_createUnitBuilder(&unitBuilder);

    daqString* unitName = nullptr;
    daqString_createString(&unitName, "volts");
    daqUnitBuilder_setName(unitBuilder, unitName);
    daqBaseObject_releaseRef(unitName);

    daqString* unitSymbol = nullptr;
    daqString_createString(&unitSymbol, "V");
    daqUnitBuilder_setSymbol(unitBuilder, unitSymbol);
    daqBaseObject_releaseRef(unitSymbol);

    daqString* unitQuantity = nullptr;
    daqString_createString(&unitQuantity, "voltage");
    daqUnitBuilder_setQuantity(unitBuilder, unitQuantity);
    daqBaseObject_releaseRef(unitQuantity);

    daqUnitBuilder_setId(unitBuilder, -1);

    daqUnit* unit = nullptr;
    daqUnitBuilder_build(unitBuilder, &unit);
    daqDataDescriptorBuilder_setUnit(builder, unit);
    daqBaseObject_releaseRef(unitBuilder);
    daqBaseObject_releaseRef(unit);

    daqString* name = nullptr;
    daqString_createString(&name, "vals");
    daqDataDescriptorBuilder_setName(builder, name);
    daqBaseObject_releaseRef(name);

    daqDataDescriptorBuilder_build(builder, &descriptor);
    daqBaseObject_releaseRef(builder);

    return descriptor;
}

daqDataDescriptor* createDomainDescriptor()
{
    daqDataDescriptor* descriptor = nullptr;

    daqDataDescriptorBuilder* builder = nullptr;
    daqDataDescriptorBuilder_createDataDescriptorBuilder(&builder);

    daqDataDescriptorBuilder_setSampleType(builder, daqSampleType::daqSampleTypeInt64);

    daqUnitBuilder* unitBuilder = nullptr;
    daqUnitBuilder_createUnitBuilder(&unitBuilder);

    daqString* unitName = nullptr;
    daqString_createString(&unitName, "seconds");
    daqUnitBuilder_setName(unitBuilder, unitName);
    daqBaseObject_releaseRef(unitName);

    daqString* unitSymbol = nullptr;
    daqString_createString(&unitSymbol, "s");
    daqUnitBuilder_setSymbol(unitBuilder, unitSymbol);
    daqBaseObject_releaseRef(unitSymbol);

    daqString* unitQuantity = nullptr;
    daqString_createString(&unitQuantity, "time");
    daqUnitBuilder_setQuantity(unitBuilder, unitQuantity);
    daqBaseObject_releaseRef(unitQuantity);

    daqUnitBuilder_setId(unitBuilder, -1);

    daqUnit* unit = nullptr;
    daqUnitBuilder_build(unitBuilder, &unit);
    daqBaseObject_releaseRef(unitBuilder);

    daqDataDescriptorBuilder_setUnit(builder, unit);
    daqBaseObject_releaseRef(unit);

    daqString* name = nullptr;
    daqString_createString(&name, "time");
    daqDataDescriptorBuilder_setName(builder, name);
    daqBaseObject_releaseRef(name);

    daqRatio* ratio = nullptr;
    daqRatio_createRatio(&ratio, 1, 1000);
    daqDataDescriptorBuilder_setTickResolution(builder, ratio);
    daqBaseObject_releaseRef(ratio);

    daqInteger* delta = nullptr;
    daqInteger_createInteger(&delta, 1);
    daqInteger* start = nullptr;
    daqInteger_createInteger(&start, 0);

    daqNumber* deltaNum = nullptr;
    daqBaseObject_queryInterface(delta, DAQ_NUMBER_INTF_ID, (void**) &deltaNum);
    daqNumber* startNum = nullptr;
    daqBaseObject_queryInterface(start, DAQ_NUMBER_INTF_ID, (void**) &startNum);

    daqBaseObject_releaseRef(delta);
    daqBaseObject_releaseRef(start);

    daqDataRule* rule = nullptr;
    daqDataRule_createLinearDataRule(&rule, deltaNum, startNum);
    daqBaseObject_releaseRef(deltaNum);
    daqBaseObject_releaseRef(startNum);

    daqDataDescriptorBuilder_setRule(builder, rule);
    daqBaseObject_releaseRef(rule);

    daqString* origin = nullptr;
    daqString_createString(&origin, "2025-01-01T00:00:00Z");
    daqDataDescriptorBuilder_setOrigin(builder, origin);
    daqBaseObject_releaseRef(origin);

    daqDataDescriptorBuilder_build(builder, &descriptor);
    daqBaseObject_releaseRef(builder);

    return descriptor;
}

TEST_F(COpendaqSignalTest, Allocator)
{
    daqAllocator* allocator = nullptr;
    daqAllocator_createMallocAllocator(&allocator);
    ASSERT_NE(allocator, nullptr);

    daqDataDescriptor* valueDescriptor = createValueDescriptor();

    void* address = nullptr;
    daqErrCode err = daqAllocator_allocate(allocator, valueDescriptor, 32, 4, &address);

    ASSERT_EQ(err, 0);
    ASSERT_NE(address, nullptr);

    err = daqAllocator_free(allocator, address);
    ASSERT_EQ(err, 0);

    daqBaseObject_releaseRef(allocator);
    daqBaseObject_releaseRef(valueDescriptor);
}

TEST_F(COpendaqSignalTest, DataDescriptor)
{
    daqDataDescriptor* valueDescriptor = createValueDescriptor();

    daqString* name = nullptr;
    daqDataDescriptor_getName(valueDescriptor, &name);
    daqConstCharPtr nameStr = nullptr;
    daqString_getCharPtr(name, &nameStr);
    ASSERT_STREQ(nameStr, "vals");
    daqBaseObject_releaseRef(name);

    daqUnit* unit = nullptr;
    daqDataDescriptor_getUnit(valueDescriptor, &unit);
    daqString* symbol = nullptr;
    daqUnit_getSymbol(unit, &symbol);
    daqConstCharPtr symbolStr = nullptr;
    daqString_getCharPtr(symbol, &symbolStr);
    ASSERT_STREQ(symbolStr, "V");
    daqBaseObject_releaseRef(symbol);
    daqBaseObject_releaseRef(unit);

    daqSampleType sampleType = daqSampleType::daqSampleTypeNull;
    daqDataDescriptor_getSampleType(valueDescriptor, &sampleType);
    ASSERT_EQ(sampleType, daqSampleType::daqSampleTypeInt64);

    daqBaseObject_releaseRef(valueDescriptor);
}

TEST_F(COpendaqSignalTest, DataPacket)
{
    daqDataDescriptor* valueDescriptor = createValueDescriptor();
    daqDataPacket* packet = nullptr;

    daqInteger* offset = nullptr;
    daqInteger_createInteger(&offset, 0);
    daqNumber* offsetNum = nullptr;
    daqBaseObject_queryInterface(offset, DAQ_NUMBER_INTF_ID, (void**) &offsetNum);
    daqBaseObject_releaseRef(offset);

    daqSizeT sampleCount = 10;
    daqDataPacket_createDataPacket(&packet, valueDescriptor, sampleCount, offsetNum);
    daqBaseObject_releaseRef(offsetNum);

    void* data = nullptr;
    daqDataPacket_getRawData(packet, &data);
    ASSERT_NE(data, nullptr);

    daqBaseObject_releaseRef(packet);
    daqBaseObject_releaseRef(valueDescriptor);
}

TEST_F(COpendaqSignalTest, DimensionRule)
{
    daqDimensionRuleBuilder* builder = nullptr;
    daqDimensionRuleBuilder_createDimensionRuleBuilder(&builder);
    daqDimensionRuleBuilder_setType(builder, daqDimensionRuleType::daqDimensionRuleTypeLinear);
    daqInteger* delta = nullptr;
    daqInteger_createInteger(&delta, 1);
    daqInteger* start = nullptr;
    daqInteger_createInteger(&start, 0);
    daqInteger* size = nullptr;
    daqInteger_createInteger(&size, 10);
    daqNumber* deltaNum = nullptr;
    daqBaseObject_queryInterface(delta, DAQ_NUMBER_INTF_ID, (void**) &deltaNum);
    daqNumber* startNum = nullptr;
    daqBaseObject_queryInterface(start, DAQ_NUMBER_INTF_ID, (void**) &startNum);
    daqNumber* sizeNum = nullptr;
    daqBaseObject_queryInterface(size, DAQ_NUMBER_INTF_ID, (void**) &sizeNum);
    daqBaseObject_releaseRef(delta);
    daqBaseObject_releaseRef(start);
    daqBaseObject_releaseRef(size);

    daqString* deltaStr = nullptr;
    daqString_createString(&deltaStr, "delta");
    daqString* startStr = nullptr;
    daqString_createString(&startStr, "start");
    daqString* sizeStr = nullptr;
    daqString_createString(&sizeStr, "size");
    daqDimensionRuleBuilder_addParameter(builder, sizeStr, sizeNum);
    daqDimensionRuleBuilder_addParameter(builder, deltaStr, deltaNum);
    daqDimensionRuleBuilder_addParameter(builder, startStr, startNum);
    daqBaseObject_releaseRef(deltaNum);
    daqBaseObject_releaseRef(startNum);
    daqBaseObject_releaseRef(sizeNum);

    daqDimensionRule* rule = nullptr;
    daqDimensionRuleBuilder_build(builder, &rule);
    daqBaseObject_releaseRef(builder);

    ASSERT_NE(rule, nullptr);

    daqDimensionRuleType ruleType = daqDimensionRuleType::daqDimensionRuleTypeOther;
    daqDimensionRule_getType(rule, &ruleType);

    ASSERT_EQ(ruleType, daqDimensionRuleType::daqDimensionRuleTypeLinear);

    daqDict* params = nullptr;
    daqDimensionRule_getParameters(rule, &params);
    ASSERT_NE(params, nullptr);

    daqNumber* outDelta = nullptr;
    daqDict_get(params, deltaStr, (daqBaseObject**) &outDelta);
    daqNumber* outStart = nullptr;
    daqDict_get(params, startStr, (daqBaseObject**) &outStart);
    daqNumber* outSize = nullptr;
    daqDict_get(params, sizeStr, (daqBaseObject**) &outSize);

    daqInt deltaInt = -1;
    daqInt startInt = -1;
    daqInt sizeInt = -1;
    daqNumber_getIntValue(outDelta, &deltaInt);
    daqNumber_getIntValue(outStart, &startInt);
    daqNumber_getIntValue(outSize, &sizeInt);
    ASSERT_EQ(deltaInt, 1);
    ASSERT_EQ(startInt, 0);
    ASSERT_EQ(sizeInt, 10);

    daqBaseObject_releaseRef(outSize);
    daqBaseObject_releaseRef(outDelta);
    daqBaseObject_releaseRef(outStart);
    daqBaseObject_releaseRef(params);
    daqBaseObject_releaseRef(rule);
    daqBaseObject_releaseRef(deltaStr);
    daqBaseObject_releaseRef(startStr);
    daqBaseObject_releaseRef(sizeStr);
}

TEST_F(COpendaqSignalTest, EventPacket)
{
    daqEventPacket* packet = nullptr;
    daqDataDescriptor* valueDescriptor = createValueDescriptor();
    daqDataDescriptor* domainDescriptor = createDomainDescriptor();
    daqEventPacket_createDataDescriptorChangedEventPacket(&packet, valueDescriptor, domainDescriptor);

    ASSERT_NE(packet, nullptr);
    daqString* id = nullptr;
    daqEventPacket_getEventId(packet, &id);
    daqConstCharPtr idStr = nullptr;
    daqString_getCharPtr(id, &idStr);
    ASSERT_STREQ(idStr, "DATA_DESCRIPTOR_CHANGED");

    daqBaseObject_releaseRef(id);
    daqBaseObject_releaseRef(packet);
    daqBaseObject_releaseRef(valueDescriptor);
    daqBaseObject_releaseRef(domainDescriptor);
}

TEST_F(COpendaqSignalTest, InputPort)
{
    daqInputPortConfig* inputPortConfig = nullptr;
    daqContext* ctx = createContext();
    daqString* id = nullptr;
    daqString_createString(&id, "daqInputPort");
    daqInputPortConfig_createInputPort(&inputPortConfig, ctx, nullptr, id, False);
    ASSERT_NE(inputPortConfig, nullptr);

    daqBaseObject_releaseRef(id);
    daqBaseObject_releaseRef(ctx);
    daqBaseObject_releaseRef(inputPortConfig);
}

TEST_F(COpendaqSignalTest, Range)
{
    daqRange* range = nullptr;
    daqInteger* lowValue = nullptr;
    daqInteger_createInteger(&lowValue, 0);
    daqInteger* highValue = nullptr;
    daqInteger_createInteger(&highValue, 10);
    daqNumber* lowValueNum = nullptr;
    daqBaseObject_queryInterface(lowValue, DAQ_NUMBER_INTF_ID, (void**) &lowValueNum);
    daqBaseObject_releaseRef(lowValue);
    daqNumber* highValueNum = nullptr;
    daqBaseObject_queryInterface(highValue, DAQ_NUMBER_INTF_ID, (void**) &highValueNum);
    daqBaseObject_releaseRef(highValue);
    daqRange_createRange(&range, lowValueNum, highValueNum);
    daqBaseObject_releaseRef(lowValueNum);
    daqBaseObject_releaseRef(highValueNum);

    ASSERT_NE(range, nullptr);

    daqNumber* outLowValue = nullptr;
    daqNumber* outHighValue = nullptr;
    daqRange_getLowValue(range, &outLowValue);
    daqRange_getHighValue(range, &outHighValue);
    daqInt lowValueInt = -1;
    daqInt highValueInt = -1;
    daqNumber_getIntValue(outLowValue, &lowValueInt);
    daqNumber_getIntValue(outHighValue, &highValueInt);
    ASSERT_EQ(lowValueInt, 0);
    ASSERT_EQ(highValueInt, 10);

    daqBaseObject_releaseRef(outHighValue);
    daqBaseObject_releaseRef(outLowValue);
    daqBaseObject_releaseRef(range);
}

TEST_F(COpendaqSignalTest, Scaling)
{
    daqScalingBuilder* builder = nullptr;
    daqScalingBuilder_createScalingBuilder(&builder);
    daqScalingBuilder_setInputDataType(builder, daqSampleType::daqSampleTypeInt16);
    daqScalingBuilder_setOutputDataType(builder, daqScaledSampleType::daqScaledSampleTypeFloat32);
    daqScalingBuilder_setScalingType(builder, daqScalingType::daqScalingTypeLinear);

    daqDict* params = nullptr;
    daqDict_createDict(&params);

    daqString* scaleStr = nullptr;
    daqString_createString(&scaleStr, "scale");
    daqString* offsetStr = nullptr;
    daqString_createString(&offsetStr, "offset");

    daqInteger* scale = nullptr;
    daqInteger_createInteger(&scale, 10);
    daqInteger* offset = nullptr;
    daqInteger_createInteger(&offset, 10);

    daqDict_set(params, scaleStr, scale);
    daqDict_set(params, offsetStr, offset);
    daqScalingBuilder_setParameters(builder, params);
    daqBaseObject_releaseRef(scale);
    daqBaseObject_releaseRef(offset);
    daqBaseObject_releaseRef(scaleStr);
    daqBaseObject_releaseRef(offsetStr);

    daqScaling* scaling = nullptr;
    daqScalingBuilder_build(builder, &scaling);
    daqBaseObject_releaseRef(builder);

    ASSERT_NE(scaling, nullptr);

    daqScalingType scalingType = daqScalingType::daqScalingTypeOther;
    daqScaling_getType(scaling, &scalingType);
    ASSERT_EQ(scalingType, daqScalingType::daqScalingTypeLinear);
    daqSampleType inputSampleType = daqSampleType::daqSampleTypeNull;
    daqScaling_getInputSampleType(scaling, &inputSampleType);
    ASSERT_EQ(inputSampleType, daqSampleType::daqSampleTypeInt16);
    daqScaledSampleType outputSampleType = daqScaledSampleType::daqScaledSampleTypeInvalid;
    daqScaling_getOutputSampleType(scaling, &outputSampleType);
    ASSERT_EQ(outputSampleType, daqScaledSampleType::daqScaledSampleTypeFloat32);
    daqDict* scalingParams = nullptr;
    daqScaling_getParameters(scaling, &scalingParams);
    ASSERT_NE(scalingParams, nullptr);

    daqBool equal = False;
    daqBaseObject_equals(scalingParams, params, &equal);
    ASSERT_EQ(equal, True);

    daqBaseObject_releaseRef(scalingParams);
    daqBaseObject_releaseRef(scaling);
    daqBaseObject_releaseRef(params);
}

TEST_F(COpendaqSignalTest, Signal)
{
    daqSignalConfig* signalConfig = nullptr;
    daqString* id = nullptr;
    daqString_createString(&id, "sig");
    daqContext* ctx = createContext();
    daqSignalConfig_createSignal(&signalConfig, ctx, nullptr, id, nullptr);
    ASSERT_NE(signalConfig, nullptr);
    daqBaseObject_releaseRef(id);
    daqBaseObject_releaseRef(ctx);
    daqBaseObject_releaseRef(signalConfig);
}
