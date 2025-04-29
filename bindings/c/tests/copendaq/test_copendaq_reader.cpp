#include <copendaq.h>

#include <gtest/gtest.h>

class COpendaqReaderTest : public testing::Test
{
protected:
    DataDescriptor* SetUpDataDescriptor()
    {
        DataDescriptor* descriptor = nullptr;
        DataDescriptorBuilder* builder = nullptr;
        DataDescriptorBuilder_createDataDescriptorBuilder(&builder);
        DataDescriptorBuilder_setSampleType(builder, SampleType::SampleTypeFloat64);

        Unit* unit = nullptr;
        String *symbol = nullptr, *unitName = nullptr, *quantity = nullptr;
        String_createString(&symbol, "V");
        String_createString(&unitName, "volts");
        String_createString(&quantity, "voltage");
        Unit_createUnit(&unit, -1, symbol, unitName, quantity);
        BaseObject_releaseRef(symbol);
        BaseObject_releaseRef(unitName);
        BaseObject_releaseRef(quantity);

        DataDescriptorBuilder_setUnit(builder, unit);
        BaseObject_releaseRef(unit);

        String* signalName = nullptr;
        String_createString(&signalName, "signal_values");
        DataDescriptorBuilder_setName(builder, signalName);
        BaseObject_releaseRef(signalName);

        DataDescriptorBuilder_build(builder, &descriptor);
        BaseObject_releaseRef(builder);
        return descriptor;
    }

    DataDescriptor* SetUpDomainDescriptor()
    {
        DataDescriptor* descriptor = nullptr;
        DataDescriptorBuilder* builder = nullptr;
        DataDescriptorBuilder_createDataDescriptorBuilder(&builder);
        DataDescriptorBuilder_setSampleType(builder, SampleType::SampleTypeInt64);
        Unit* unit = nullptr;
        String *symbol = nullptr, *unitName = nullptr, *quantity = nullptr;

        String_createString(&symbol, "s");
        String_createString(&unitName, "seconds");
        String_createString(&quantity, "time");
        Unit_createUnit(&unit, -1, symbol, unitName, quantity);
        BaseObject_releaseRef(symbol);
        BaseObject_releaseRef(unitName);
        BaseObject_releaseRef(quantity);

        DataDescriptorBuilder_setUnit(builder, unit);
        BaseObject_releaseRef(unit);

        String* signalName = nullptr;
        String_createString(&signalName, "signal_time");
        DataDescriptorBuilder_setName(builder, signalName);
        BaseObject_releaseRef(signalName);

        Ratio* tickResolution = nullptr;
        Ratio_createRatio(&tickResolution, 1, 1000);
        DataDescriptorBuilder_setTickResolution(builder, tickResolution);
        BaseObject_releaseRef(tickResolution);
        DataRule* rule = nullptr;

        Integer *start = nullptr, *delta = nullptr;
        Integer_createInteger(&start, 0);
        Integer_createInteger(&delta, 1);
        Number *deltaNum = nullptr, *startNum = nullptr;
        BaseObject_queryInterface(delta, NUMBER_INTF_ID, reinterpret_cast<BaseObject**>(&deltaNum));
        BaseObject_queryInterface(start, NUMBER_INTF_ID, reinterpret_cast<BaseObject**>(&startNum));
        BaseObject_releaseRef(delta);
        BaseObject_releaseRef(start);
        DataRule_createLinearDataRule(&rule, startNum, deltaNum);
        BaseObject_releaseRef(deltaNum);
        BaseObject_releaseRef(startNum);
        DataDescriptorBuilder_setRule(builder, rule);
        BaseObject_releaseRef(rule);

        String* epoch = nullptr;
        String_createString(&epoch, "2025-01-01T00:00:00+0000");
        DataDescriptorBuilder_setOrigin(builder, epoch);
        BaseObject_releaseRef(epoch);

        DataDescriptorBuilder_build(builder, &descriptor);
        BaseObject_releaseRef(builder);
        return descriptor;
    }

    Context* SetUpContext()
    {
        List* sinks = nullptr;
        List_createList(&sinks);

        LoggerSink* sink = nullptr;
        LoggerSink_createStdErrLoggerSink(&sink);
        List_pushBack(sinks, sink);

        Logger* logger = nullptr;
        Logger_createLogger(&logger, sinks, LogLevel::LogLevelDebug);

        TypeManager* typeManager = nullptr;
        TypeManager_createTypeManager(&typeManager);

        Scheduler* scheduler = nullptr;
        Scheduler_createScheduler(&scheduler, logger, 1);

        Dict *options = nullptr, *discoveryServers = nullptr;
        Dict_createDict(&options);
        Dict_createDict(&discoveryServers);

        Context_createContext(&ctx, scheduler, logger, typeManager, nullptr, nullptr, options, discoveryServers);
        BaseObject_releaseRef(discoveryServers);
        BaseObject_releaseRef(options);
        BaseObject_releaseRef(scheduler);
        BaseObject_releaseRef(typeManager);
        BaseObject_releaseRef(logger);
        BaseObject_releaseRef(sink);
        BaseObject_releaseRef(sinks);

        return ctx;
    }

    Packet* PrepareDataPacket()
    {
        DataPacket* domainPacket = nullptr;

        Integer* offset = nullptr;
        Integer_createInteger(&offset, 0);
        Number* offsetNum = nullptr;
        BaseObject_queryInterface(offset, NUMBER_INTF_ID, reinterpret_cast<BaseObject**>(&offsetNum));
        BaseObject_releaseRef(offset);

        DataPacket_createDataPacket(&domainPacket, domainDescriptor, 10, offsetNum);

        DataPacket* packet = nullptr;
        DataPacket_createDataPacketWithDomain(&packet, domainPacket, valueDescriptor, 10, offsetNum);
        BaseObject_releaseRef(domainPacket);
        BaseObject_releaseRef(offsetNum);

        Float values[] = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0};

        Float* data = nullptr;
        DataPacket_getRawData(packet, reinterpret_cast<void**>(&data));
        memcpy(data, values, sizeof(values));

        Packet* outputPacket = nullptr;
        BaseObject_queryInterface(packet, PACKET_INTF_ID, reinterpret_cast<BaseObject**>(&outputPacket));
        BaseObject_releaseRef(packet);

        return outputPacket;
    }

    void SetUp() override
    {
        String* id = nullptr;
        String_createString(&id, "sig_values");
        String* domainId = nullptr;
        String_createString(&domainId, "sig_time");

        ctx = SetUpContext();
        valueDescriptor = SetUpDataDescriptor();
        domainDescriptor = SetUpDomainDescriptor();

        signalConfig = nullptr;
        SignalConfig_createSignal(&signalConfig, ctx, nullptr, id, nullptr);

        domainSignalConfig = nullptr;
        SignalConfig_createSignal(&domainSignalConfig, ctx, nullptr, domainId, nullptr);

        BaseObject_queryInterface(signalConfig, SIGNAL_INTF_ID, reinterpret_cast<BaseObject**>(&signal));
        BaseObject_queryInterface(domainSignalConfig, SIGNAL_INTF_ID, reinterpret_cast<BaseObject**>(&domainSignal));

        SignalConfig_setDescriptor(signalConfig, valueDescriptor);
        SignalConfig_setDescriptor(domainSignalConfig, domainDescriptor);
        SignalConfig_setDomainSignal(signalConfig, domainSignal);

        BaseObject_releaseRef(id);
        BaseObject_releaseRef(domainId);
    }

    void TearDown() override
    {
        BaseObject_releaseRef(ctx);
        BaseObject_releaseRef(signal);
        BaseObject_releaseRef(domainSignal);
        BaseObject_releaseRef(signalConfig);
        BaseObject_releaseRef(domainSignalConfig);
        BaseObject_releaseRef(valueDescriptor);
        BaseObject_releaseRef(domainDescriptor);
    }

    Context* ctx = nullptr;
    Signal* signal = nullptr;
    Signal* domainSignal = nullptr;
    SignalConfig* signalConfig = nullptr;
    SignalConfig* domainSignalConfig = nullptr;
    DataDescriptor* valueDescriptor = nullptr;
    DataDescriptor* domainDescriptor = nullptr;
};

TEST_F(COpendaqReaderTest, BlockReader)
{
    BlockReader* blockReader = nullptr;
    BlockReader_createBlockReader(&blockReader, signal, 2, SampleTypeFloat64, SampleTypeInt64, ReadModeScaled);

    Packet* packet = PrepareDataPacket();

    SignalConfig_sendPacket(signalConfig, packet);
    BaseObject_releaseRef(packet);

    Float data[10] = {0};
    BlockReaderStatus* status = nullptr;
    SizeT count = 5;
    SizeT timeoutMs = 1000;

    BlockReader_read(blockReader, data, &count, timeoutMs, &status);
    ASSERT_EQ(count, 0);
    ReadStatus statusValue = ReadStatus::ReadStatusUnknown;
    ReaderStatus_getReadStatus(reinterpret_cast<ReaderStatus*>(status), &statusValue);
    ASSERT_EQ(statusValue, ReadStatus::ReadStatusEvent);
    BaseObject_releaseRef(status);
    count = 5;
    BlockReader_read(blockReader, data, &count, timeoutMs, &status);
    ReadStatus statusValue2 = ReadStatus::ReadStatusUnknown;
    ReaderStatus_getReadStatus(reinterpret_cast<ReaderStatus*>(status), &statusValue2);
    ASSERT_EQ(statusValue2, ReadStatus::ReadStatusOk);
    ASSERT_EQ(count, 5);
    for (SizeT i = 0; i < count; ++i)
    {
        ASSERT_EQ(data[i], static_cast<Float>(i + 1));
    }
    BaseObject_releaseRef(status);
    BaseObject_releaseRef(blockReader);
}

TEST_F(COpendaqReaderTest, PacketReader)
{
    PacketReader* packetReader = nullptr;
    PacketReader_createPacketReader(&packetReader, signal);
    Packet* packet = PrepareDataPacket();
    SignalConfig_sendPacket(signalConfig, packet);
    BaseObject_releaseRef(packet);
    packet = nullptr;

    PacketReader_read(packetReader, &packet);
    ASSERT_NE(packet, nullptr);

    PacketType type = PacketType::PacketTypeNone;
    Packet_getType(packet, &type);
    ASSERT_EQ(type, PacketType::PacketTypeEvent);
    BaseObject_releaseRef(packet);
    packet = nullptr;

    PacketReader_read(packetReader, &packet);
    ASSERT_NE(packet, nullptr);
    Packet_getType(packet, &type);
    ASSERT_EQ(type, PacketType::PacketTypeData);

    BaseObject_releaseRef(packet);
    BaseObject_releaseRef(packetReader);
}

TEST_F(COpendaqReaderTest, Reader)
{
    StreamReader* streamReader = nullptr;
    StreamReader_createStreamReader(&streamReader, signal, SampleTypeFloat64, SampleTypeInt64, ReadModeScaled, ReadTimeoutTypeAll);
    Reader* reader = nullptr;
    BaseObject_queryInterface(streamReader, READER_INTF_ID, reinterpret_cast<BaseObject**>(&reader));

    Packet* packet = PrepareDataPacket();
    SignalConfig_sendPacket(signalConfig, packet);
    BaseObject_releaseRef(packet);

    Float data[10] = {0};
    SizeT count = 10;
    SizeT timeoutMs = 1000;
    ReaderStatus* status = nullptr;

    StreamReader_read(streamReader, data, &count, timeoutMs, &status);
    ASSERT_EQ(count, 0);
    ReadStatus statusValue = ReadStatus::ReadStatusUnknown;
    ReaderStatus_getReadStatus(status, &statusValue);
    ASSERT_EQ(statusValue, ReadStatus::ReadStatusEvent);
    BaseObject_releaseRef(status);

    Reader_getAvailableCount(reader, &count);
    ASSERT_EQ(count, 10);

    BaseObject_releaseRef(reader);
    BaseObject_releaseRef(streamReader);
}

TEST_F(COpendaqReaderTest, StreamReader)
{
    StreamReader* streamReader = nullptr;
    StreamReader_createStreamReader(&streamReader, signal, SampleTypeFloat64, SampleTypeInt64, ReadModeScaled, ReadTimeoutTypeAll);

    Packet* packet = PrepareDataPacket();

    SignalConfig_sendPacket(signalConfig, packet);
    BaseObject_releaseRef(packet);

    Float data[10] = {0};
    ReaderStatus* status = nullptr;
    SizeT count = 10;
    SizeT timeoutMs = 1000;

    StreamReader_read(streamReader, data, &count, timeoutMs, &status);
    ASSERT_EQ(count, 0);
    ReadStatus statusValue = ReadStatus::ReadStatusUnknown;
    ReaderStatus_getReadStatus(status, &statusValue);
    ASSERT_EQ(statusValue, ReadStatus::ReadStatusEvent);
    BaseObject_releaseRef(status);
    count = 10;
    StreamReader_read(streamReader, data, &count, timeoutMs, &status);
    ReadStatus statusValue2 = ReadStatus::ReadStatusUnknown;
    ReaderStatus_getReadStatus(status, &statusValue2);
    ASSERT_EQ(statusValue2, ReadStatus::ReadStatusOk);
    ASSERT_EQ(count, 10);
    for (SizeT i = 0; i < count; ++i)
    {
        ASSERT_EQ(data[i], static_cast<Float>(i + 1));
    }
    BaseObject_releaseRef(status);
    BaseObject_releaseRef(streamReader);
}

TEST_F(COpendaqReaderTest, TailReader)
{
    TailReader* tailReader = nullptr;
    TailReader_createTailReader(&tailReader, signal, 10, SampleTypeFloat64, SampleTypeInt64, ReadModeScaled);

    Packet* packet = PrepareDataPacket();

    SignalConfig_sendPacket(signalConfig, packet);
    BaseObject_releaseRef(packet);

    Float data[10] = {0};
    TailReaderStatus* status = nullptr;
    SizeT count = 10;

    TailReader_read(tailReader, data, &count, &status);
    ASSERT_EQ(count, 0);
    ReadStatus statusValue = ReadStatus::ReadStatusUnknown;
    ReaderStatus_getReadStatus(reinterpret_cast<ReaderStatus*>(status), &statusValue);
    ASSERT_EQ(statusValue, ReadStatus::ReadStatusEvent);
    BaseObject_releaseRef(status);
    count = 10;
    TailReader_read(tailReader, data, &count, &status);
    ReadStatus statusValue2 = ReadStatus::ReadStatusUnknown;
    ReaderStatus_getReadStatus(reinterpret_cast<ReaderStatus*>(status), &statusValue2);
    ASSERT_EQ(statusValue2, ReadStatus::ReadStatusOk);
    ASSERT_EQ(count, 10);
    for (SizeT i = 0; i < count; ++i)
    {
        ASSERT_EQ(data[i], static_cast<Float>(i + 1));
    }
    BaseObject_releaseRef(status);
    BaseObject_releaseRef(tailReader);
}
