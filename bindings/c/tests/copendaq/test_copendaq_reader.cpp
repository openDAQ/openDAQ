#include <copendaq.h>

#include <gtest/gtest.h>

class COpendaqReaderTest : public testing::Test
{
protected:
    daqDataDescriptor* SetUpDataDescriptor()
    {
        daqDataDescriptor* descriptor = nullptr;
        daqDataDescriptorBuilder* builder = nullptr;
        daqDataDescriptorBuilder_createDataDescriptorBuilder(&builder);
        daqDataDescriptorBuilder_setSampleType(builder, daqSampleType::daqSampleTypeFloat64);

        daqUnit* unit = nullptr;
        daqString *symbol = nullptr, *unitName = nullptr, *quantity = nullptr;
        daqString_createString(&symbol, "V");
        daqString_createString(&unitName, "volts");
        daqString_createString(&quantity, "voltage");
        daqUnit_createUnit(&unit, -1, symbol, unitName, quantity);
        daqBaseObject_releaseRef(symbol);
        daqBaseObject_releaseRef(unitName);
        daqBaseObject_releaseRef(quantity);

        daqDataDescriptorBuilder_setUnit(builder, unit);
        daqBaseObject_releaseRef(unit);

        daqString* signalName = nullptr;
        daqString_createString(&signalName, "signal_values");
        daqDataDescriptorBuilder_setName(builder, signalName);
        daqBaseObject_releaseRef(signalName);

        daqDataDescriptorBuilder_build(builder, &descriptor);
        daqBaseObject_releaseRef(builder);
        return descriptor;
    }

    daqDataDescriptor* SetUpDomainDescriptor()
    {
        daqDataDescriptor* descriptor = nullptr;
        daqDataDescriptorBuilder* builder = nullptr;
        daqDataDescriptorBuilder_createDataDescriptorBuilder(&builder);
        daqDataDescriptorBuilder_setSampleType(builder, daqSampleType::daqSampleTypeInt64);
        daqUnit* unit = nullptr;
        daqString *symbol = nullptr, *unitName = nullptr, *quantity = nullptr;

        daqString_createString(&symbol, "s");
        daqString_createString(&unitName, "seconds");
        daqString_createString(&quantity, "time");
        daqUnit_createUnit(&unit, -1, symbol, unitName, quantity);
        daqBaseObject_releaseRef(symbol);
        daqBaseObject_releaseRef(unitName);
        daqBaseObject_releaseRef(quantity);

        daqDataDescriptorBuilder_setUnit(builder, unit);
        daqBaseObject_releaseRef(unit);

        daqString* signalName = nullptr;
        daqString_createString(&signalName, "signal_time");
        daqDataDescriptorBuilder_setName(builder, signalName);
        daqBaseObject_releaseRef(signalName);

        daqRatio* tickResolution = nullptr;
        daqRatio_createRatio(&tickResolution, 1, 1000);
        daqDataDescriptorBuilder_setTickResolution(builder, tickResolution);
        daqBaseObject_releaseRef(tickResolution);
        daqDataRule* rule = nullptr;

        daqInteger *start = nullptr, *delta = nullptr;
        daqInteger_createInteger(&start, 0);
        daqInteger_createInteger(&delta, 1);
        daqNumber *deltaNum = nullptr, *startNum = nullptr;
        daqBaseObject_queryInterface(delta, DAQ_NUMBER_INTF_ID, (daqBaseObject**) &deltaNum);
        daqBaseObject_queryInterface(start, DAQ_NUMBER_INTF_ID, (daqBaseObject**) &startNum);
        daqBaseObject_releaseRef(delta);
        daqBaseObject_releaseRef(start);
        daqDataRule_createLinearDataRule(&rule, startNum, deltaNum);
        daqBaseObject_releaseRef(deltaNum);
        daqBaseObject_releaseRef(startNum);
        daqDataDescriptorBuilder_setRule(builder, rule);
        daqBaseObject_releaseRef(rule);

        daqString* epoch = nullptr;
        daqString_createString(&epoch, "2025-01-01T00:00:00+0000");
        daqDataDescriptorBuilder_setOrigin(builder, epoch);
        daqBaseObject_releaseRef(epoch);

        daqDataDescriptorBuilder_build(builder, &descriptor);
        daqBaseObject_releaseRef(builder);
        return descriptor;
    }

    daqContext* daqSetUpContext()
    {
        daqList* sinks = nullptr;
        daqList_createList(&sinks);

        daqLoggerSink* sink = nullptr;
        daqLoggerSink_createStdErrLoggerSink(&sink);
        daqList_pushBack(sinks, sink);

        daqLogger* logger = nullptr;
        daqLogger_createLogger(&logger, sinks, daqLogLevel::daqLogLevelDebug);

        daqTypeManager* typeManager = nullptr;
        daqTypeManager_createTypeManager(&typeManager);

        daqScheduler* scheduler = nullptr;
        daqScheduler_createScheduler(&scheduler, logger, 1);

        daqDict *options = nullptr, *discoveryServers = nullptr;
        daqDict_createDict(&options);
        daqDict_createDict(&discoveryServers);

        daqContext_createContext(&ctx, scheduler, logger, typeManager, nullptr, nullptr, options, discoveryServers);
        daqBaseObject_releaseRef(discoveryServers);
        daqBaseObject_releaseRef(options);
        daqBaseObject_releaseRef(scheduler);
        daqBaseObject_releaseRef(typeManager);
        daqBaseObject_releaseRef(logger);
        daqBaseObject_releaseRef(sink);
        daqBaseObject_releaseRef(sinks);

        return ctx;
    }

    daqPacket* daqPrepareDataPacket()
    {
        daqDataPacket* domainPacket = nullptr;

        daqInteger* offset = nullptr;
        daqInteger_createInteger(&offset, 0);
        daqNumber* offsetNum = nullptr;
        daqBaseObject_queryInterface(offset, DAQ_NUMBER_INTF_ID, (daqBaseObject**) &offsetNum);
        daqBaseObject_releaseRef(offset);

        daqDataPacket_createDataPacket(&domainPacket, domainDescriptor, 10, offsetNum);

        daqDataPacket* packet = nullptr;
        daqDataPacket_createDataPacketWithDomain(&packet, domainPacket, valueDescriptor, 10, offsetNum);
        daqBaseObject_releaseRef(domainPacket);
        daqBaseObject_releaseRef(offsetNum);

        daqFloat values[] = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0};

        daqFloat* data = nullptr;
        daqDataPacket_getRawData(packet, (void**) &data);
        memcpy(data, values, sizeof(values));

        daqPacket* outputPacket = nullptr;
        daqBaseObject_queryInterface(packet, DAQ_PACKET_INTF_ID, (daqBaseObject**) &outputPacket);
        daqBaseObject_releaseRef(packet);

        return outputPacket;
    }

    void SetUp() override
    {
        daqString* id = nullptr;
        daqString_createString(&id, "sig_values");
        daqString* domainId = nullptr;
        daqString_createString(&domainId, "sig_time");

        ctx = daqSetUpContext();
        valueDescriptor = SetUpDataDescriptor();
        domainDescriptor = SetUpDomainDescriptor();

        signalConfig = nullptr;
        daqSignalConfig_createSignal(&signalConfig, ctx, nullptr, id, nullptr);

        domainSignalConfig = nullptr;
        daqSignalConfig_createSignal(&domainSignalConfig, ctx, nullptr, domainId, nullptr);

        daqBaseObject_queryInterface(signalConfig, DAQ_SIGNAL_INTF_ID, (daqBaseObject**) &signal);
        daqBaseObject_queryInterface(domainSignalConfig, DAQ_SIGNAL_INTF_ID, (daqBaseObject**) &domainSignal);

        daqSignalConfig_setDescriptor(signalConfig, valueDescriptor);
        daqSignalConfig_setDescriptor(domainSignalConfig, domainDescriptor);
        daqSignalConfig_setDomainSignal(signalConfig, domainSignal);

        daqBaseObject_releaseRef(id);
        daqBaseObject_releaseRef(domainId);
    }

    void TearDown() override
    {
        daqBaseObject_releaseRef(ctx);
        daqBaseObject_releaseRef(signal);
        daqBaseObject_releaseRef(domainSignal);
        daqBaseObject_releaseRef(signalConfig);
        daqBaseObject_releaseRef(domainSignalConfig);
        daqBaseObject_releaseRef(valueDescriptor);
        daqBaseObject_releaseRef(domainDescriptor);
    }

    daqContext* ctx = nullptr;
    daqSignal* signal = nullptr;
    daqSignal* domainSignal = nullptr;
    daqSignalConfig* signalConfig = nullptr;
    daqSignalConfig* domainSignalConfig = nullptr;
    daqDataDescriptor* valueDescriptor = nullptr;
    daqDataDescriptor* domainDescriptor = nullptr;
};

TEST_F(COpendaqReaderTest, daqBlockReader)
{
    daqBlockReader* blockReader = nullptr;
    daqBlockReader_createBlockReader(&blockReader, signal, 2, daqSampleTypeFloat64, daqSampleTypeInt64, daqReadModeScaled);

    daqPacket* packet = daqPrepareDataPacket();

    daqSignalConfig_sendPacket(signalConfig, packet);
    daqBaseObject_releaseRef(packet);

    daqFloat data[10] = {0};
    daqBlockReaderStatus* status = nullptr;
    daqSizeT count = 5;
    daqSizeT timeoutMs = 1000;

    daqBlockReader_read(blockReader, data, &count, timeoutMs, &status);
    ASSERT_EQ(count, 0u);
    daqReadStatus statusValue = daqReadStatus::daqReadStatusUnknown;
    daqReaderStatus_getReadStatus((daqReaderStatus*) status, &statusValue);
    ASSERT_EQ(statusValue, daqReadStatus::daqReadStatusEvent);
    daqBaseObject_releaseRef(status);
    count = 5u;
    daqBlockReader_read(blockReader, data, &count, timeoutMs, &status);
    daqReadStatus statusValue2 = daqReadStatus::daqReadStatusUnknown;
    daqReaderStatus_getReadStatus((daqReaderStatus*) status, &statusValue2);
    ASSERT_EQ(statusValue2, daqReadStatus::daqReadStatusOk);
    ASSERT_EQ(count, 5u);
    for (daqSizeT i = 0; i < count; ++i)
    {
        ASSERT_EQ(data[i], (daqFloat) i + 1);
    }
    daqBaseObject_releaseRef(status);
    daqBaseObject_releaseRef(blockReader);
}

TEST_F(COpendaqReaderTest, daqPacketReader)
{
    daqPacketReader* packetReader = nullptr;
    daqPacketReader_createPacketReader(&packetReader, signal);
    daqPacket* packet = daqPrepareDataPacket();
    daqSignalConfig_sendPacket(signalConfig, packet);
    daqBaseObject_releaseRef(packet);
    packet = nullptr;

    daqPacketReader_read(packetReader, &packet);
    ASSERT_NE(packet, nullptr);

    daqPacketType type = daqPacketType::daqPacketTypeNone;
    daqPacket_getType(packet, &type);
    ASSERT_EQ(type, daqPacketType::daqPacketTypeEvent);
    daqBaseObject_releaseRef(packet);
    packet = nullptr;

    daqPacketReader_read(packetReader, &packet);
    ASSERT_NE(packet, nullptr);
    daqPacket_getType(packet, &type);
    ASSERT_EQ(type, daqPacketType::daqPacketTypeData);

    daqBaseObject_releaseRef(packet);
    daqBaseObject_releaseRef(packetReader);
}

TEST_F(COpendaqReaderTest, daqReader)
{
    daqStreamReader* streamReader = nullptr;
    daqStreamReader_createStreamReader(
        &streamReader, signal, daqSampleTypeFloat64, daqSampleTypeInt64, daqReadModeScaled, daqReadTimeoutTypeAll);
    daqReader* reader = nullptr;
    daqBaseObject_queryInterface(streamReader, DAQ_READER_INTF_ID, (daqBaseObject**) &reader);

    daqPacket* packet = daqPrepareDataPacket();
    daqSignalConfig_sendPacket(signalConfig, packet);
    daqBaseObject_releaseRef(packet);

    daqFloat data[10] = {0};
    daqSizeT count = 10;
    daqSizeT timeoutMs = 1000;
    daqReaderStatus* status = nullptr;

    daqStreamReader_read(streamReader, data, &count, timeoutMs, &status);
    ASSERT_EQ(count, 0u);
    daqReadStatus statusValue = daqReadStatus::daqReadStatusUnknown;
    daqReaderStatus_getReadStatus(status, &statusValue);
    ASSERT_EQ(statusValue, daqReadStatus::daqReadStatusEvent);
    daqBaseObject_releaseRef(status);

    daqReader_getAvailableCount(reader, &count);
    ASSERT_EQ(count, 10u);

    daqBaseObject_releaseRef(reader);
    daqBaseObject_releaseRef(streamReader);
}

TEST_F(COpendaqReaderTest, daqStreamReader)
{
    daqStreamReader* streamReader = nullptr;
    daqStreamReader_createStreamReader(
        &streamReader, signal, daqSampleTypeFloat64, daqSampleTypeInt64, daqReadModeScaled, daqReadTimeoutTypeAll);

    daqPacket* packet = daqPrepareDataPacket();

    daqSignalConfig_sendPacket(signalConfig, packet);
    daqBaseObject_releaseRef(packet);

    daqFloat data[10] = {0};
    daqReaderStatus* status = nullptr;
    daqSizeT count = 10;
    daqSizeT timeoutMs = 1000;

    daqStreamReader_read(streamReader, data, &count, timeoutMs, &status);
    ASSERT_EQ(count, 0u);
    daqReadStatus statusValue = daqReadStatus::daqReadStatusUnknown;
    daqReaderStatus_getReadStatus(status, &statusValue);
    ASSERT_EQ(statusValue, daqReadStatus::daqReadStatusEvent);
    daqBaseObject_releaseRef(status);
    count = 10;
    daqStreamReader_read(streamReader, data, &count, timeoutMs, &status);
    daqReadStatus statusValue2 = daqReadStatus::daqReadStatusUnknown;
    daqReaderStatus_getReadStatus(status, &statusValue2);
    ASSERT_EQ(statusValue2, daqReadStatus::daqReadStatusOk);
    ASSERT_EQ(count, 10u);
    for (daqSizeT i = 0; i < count; ++i)
    {
        ASSERT_EQ(data[i], (daqFloat) i + 1);
    }
    daqBaseObject_releaseRef(status);
    daqBaseObject_releaseRef(streamReader);
}

TEST_F(COpendaqReaderTest, daqTailReader)
{
    daqTailReader* tailReader = nullptr;
    daqTailReader_createTailReader(&tailReader, signal, 10, daqSampleTypeFloat64, daqSampleTypeInt64, daqReadModeScaled);

    daqPacket* packet = daqPrepareDataPacket();

    daqSignalConfig_sendPacket(signalConfig, packet);
    daqBaseObject_releaseRef(packet);

    daqFloat data[10] = {0};
    daqTailReaderStatus* status = nullptr;
    daqSizeT count = 10;

    daqTailReader_read(tailReader, data, &count, &status);
    ASSERT_EQ(count, 0u);
    daqReadStatus statusValue = daqReadStatus::daqReadStatusUnknown;
    daqReaderStatus_getReadStatus((daqReaderStatus*) status, &statusValue);
    ASSERT_EQ(statusValue, daqReadStatus::daqReadStatusEvent);
    daqBaseObject_releaseRef(status);
    count = 10u;
    daqTailReader_read(tailReader, data, &count, &status);
    daqReadStatus statusValue2 = daqReadStatus::daqReadStatusUnknown;
    daqReaderStatus_getReadStatus((daqReaderStatus*) status, &statusValue2);
    ASSERT_EQ(statusValue2, daqReadStatus::daqReadStatusOk);
    ASSERT_EQ(count, 10u);
    for (daqSizeT i = 0; i < count; ++i)
    {
        ASSERT_EQ(data[i], (daqFloat) i + 1);
    }
    daqBaseObject_releaseRef(status);
    daqBaseObject_releaseRef(tailReader);
}
