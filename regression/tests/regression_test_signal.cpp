#include <chrono>
#include <thread>
#include "setup_regression.h"

class RegressionTestSignal : public testing::Test
{
private:
    ModuleManagerPtr moduleManager;
    ContextPtr context;
    InstancePtr instance;
    DevicePtr device;

protected:
    SignalPtr signal;

    void SetUp() override
    {
        moduleManager = ModuleManager("");
        context = Context(nullptr, Logger(), TypeManager(), moduleManager);

        instance = InstanceCustom(context, "mock_instance");

        device = instance.addDevice(connectionString);

        signal = instance.getSignalsRecursive()[0];
    }
};

TEST_F(RegressionTestSignal, getPublic)
{
    ASSERT_NO_THROW(signal.getPublic());
}

TEST_F(RegressionTestSignal, setPublic)
{
    ASSERT_NO_THROW(signal.setPublic(True));
}

TEST_F(RegressionTestSignal, getDescriptor)
{
    DataDescriptorPtr descriptor;
    ASSERT_NO_THROW(descriptor = signal.getDescriptor());
    ASSERT_EQ(descriptor.getName(), "AI 1");
    ASSERT_EQ(descriptor.getDimensions().getCount(), 0);
    ASSERT_EQ(descriptor.getSampleType(), SampleType::Float64);
    ASSERT_EQ(descriptor.getUnit().getId(), -1);
    ASSERT_EQ(descriptor.getUnit().getSymbol(), "V");
    ASSERT_EQ(descriptor.getUnit().getName(), "volts");
    ASSERT_EQ(descriptor.getUnit().getQuantity(), "voltage");
    ASSERT_FLOAT_EQ(descriptor.getValueRange().getLowValue(), -10.0);
    ASSERT_FLOAT_EQ(descriptor.getValueRange().getHighValue(), 10.0);
    ASSERT_EQ(descriptor.getRule(), DataRule(DataRuleType::Explicit, Dict<IString, IBaseObject>()));
    ASSERT_EQ(descriptor.getPostScaling(), nullptr);
    ASSERT_EQ(descriptor.getOrigin(), "");
    ASSERT_EQ(descriptor.getTickResolution(), nullptr);
    ASSERT_EQ(descriptor.getStructFields().getCount(), 0);
    ASSERT_EQ(descriptor.getMetadata().getCount(), 0);
}

TEST_F(RegressionTestSignal, getDomainSignal)
{
    StringPtr name;
    StringPtr localID;
    SampleType sampleType;  // TODO: why different in lt?

    if (protocol == "opcua" || protocol == "nd")
    {
        name = localID = "ai0_time";
        sampleType = SampleType::Int64;
    }
    else if (protocol == "ns")
    {
        name = localID = "*ref_dev1*IO*ai*refch0*Sig*ai0_time";
        sampleType = SampleType::Int64;
    }
    else if (protocol == "lt")
    {
        name = "ai0_time";
        localID = "#ref_dev1#IO#ai#refch0#Sig#ai0_time";
        sampleType = SampleType::UInt64;
    }

    SignalPtr domain;
    ASSERT_NO_THROW(domain = signal.getDomainSignal());
    ASSERT_EQ(domain.getLocalId(), localID);
    ASSERT_EQ(domain.getName(), name);
    ASSERT_EQ(domain.getActive(), true);
    ASSERT_EQ(domain.getPublic(), true);

    ASSERT_EQ(domain.getDescriptor().getName(), "Time AI 1");
    ASSERT_EQ(domain.getDescriptor().getDimensions().getCount(), 0);
    ASSERT_EQ(domain.getDescriptor().getSampleType(), sampleType);
    ASSERT_EQ(domain.getDescriptor().getUnit().getId(), -1);
    ASSERT_EQ(domain.getDescriptor().getUnit().getSymbol(), "s");
    ASSERT_EQ(domain.getDescriptor().getUnit().getName(), "seconds");
    ASSERT_EQ(domain.getDescriptor().getUnit().getQuantity(), "time");
    ASSERT_EQ(domain.getDescriptor().getValueRange(), nullptr);
    auto dict = Dict<IString, IBaseObject>();
    dict.set("delta", 1000);
    dict.set("start", 0);
    ASSERT_EQ(domain.getDescriptor().getRule(), DataRule(DataRuleType::Linear, dict));
    ASSERT_EQ(domain.getDescriptor().getPostScaling(), nullptr);
    ASSERT_EQ(domain.getDescriptor().getOrigin(), "1970-01-01T00:00:00Z");
    ASSERT_EQ(domain.getDescriptor().getTickResolution(), Ratio(1, 1000000));
    ASSERT_EQ(domain.getDescriptor().getStructFields().getCount(), 0);
    ASSERT_EQ(domain.getDescriptor().getMetadata().getCount(), 0);

    ASSERT_EQ(domain.getDomainSignal(), nullptr);
    ASSERT_EQ(domain.getConnections().getCount(), 0);
    ASSERT_EQ(domain.getRelatedSignals().getCount(), 0);
}

TEST_F(RegressionTestSignal, getRelatedSignals)
{
    ListPtr<ISignal> signals;
    ASSERT_NO_THROW(signals = signal.getRelatedSignals());
    ASSERT_EQ(signals.getCount(), 0);
}

TEST_F(RegressionTestSignal, getConnections)
{
    ListPtr<IConnection> connections;
    ASSERT_NO_THROW(connections = signal.getConnections());
    ASSERT_EQ(connections.getCount(), 0);
}

TEST_F(RegressionTestSignal, getStreamed)
{
    Bool streamed;
    ASSERT_NO_THROW(streamed = signal.getStreamed());
    ASSERT_EQ(streamed, true);
}

TEST_F(RegressionTestSignal, setStreamed)
{
    ASSERT_NO_THROW(signal.setStreamed(True));
}

TEST_F(RegressionTestSignal, getLastValue)
{
    BaseObjectPtr lastValue;
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    ASSERT_NO_THROW(lastValue = signal.getLastValue());
    if (protocol == "opcua" || protocol == "nd")
        ASSERT_TRUE(lastValue.assigned());
}

TEST_F(RegressionTestSignal, reader)
{
    StreamReaderPtr reader = StreamReader<double, int64_t>(signal);
    double samples[100];
    SizeT count = 100;
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    ASSERT_NO_THROW(reader.read(samples, &count));
    if (protocol == "opcua")
    {
        ASSERT_EQ(count, 0);
    }
    else if (protocol == "nd" || protocol == "ns" || protocol == "lt")
    {
        ASSERT_EQ(count, 100);
    }
}

TEST_F(RegressionTestSignal, readerWithDomain)
{
    StreamReaderPtr reader = StreamReader<double, int64_t>(signal);
    double samples[100];
    int64_t domain[100];
    SizeT count = 100;
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    ASSERT_NO_THROW(reader.readWithDomain(samples, domain, &count));
    if (protocol == "opcua")
    {
        ASSERT_EQ(count, 0);
    }
    else if (protocol == "nd" || protocol == "ns" || protocol == "lt")
    {
        ASSERT_EQ(count, 100);
    }
}
