#include <testutils/testutils.h>
#include <licensing_module/module_dll.h>
#include <licensing_module/version.h>
#include <gmock/gmock.h>
#include <opendaq/module_ptr.h>
#include <opendaq/device_ptr.h>
#include <coretypes/common.h>
#include <opendaq/context_factory.h>
#include <opendaq/scheduler_factory.h>
#include <opendaq/module_manager_factory.h>
#include <opendaq/context_internal_ptr.h>
#include <opendaq/signal_config_ptr.h>
#include <opendaq/data_descriptor_factory.h>
#include <opendaq/signal_factory.h>
#include <opendaq/data_packet_ptr.h>
#include <opendaq/event_packet_ptr.h>
#include <opendaq/packet_factory.h>
#include <opendaq/reader_factory.h>

#include <iostream>

const std::string resourcesPath = TEST_RESOURCE_PATH;

using ModuleTest = testing::Test;
using namespace daq;

static ModulePtr CreateModule()
{
    ModulePtr module;
    createModule(&module, NullContext());
    return module;
}

class LicensingModuleTest : public testing::Test
{
public:
    ContextPtr context;
    ModulePtr module;

protected:
    void SetUp() override
    {
        const auto logger = Logger();
        auto moduleManager = ModuleManager("[[none]]");
        context = Context(Scheduler(logger), logger, TypeManager(), moduleManager, nullptr);
        createModule(&module, context);
        moduleManager.addModule(module);
        moduleManager = context.asPtr<IContextInternal>().moveModuleManager();
    }
};

class LicensingModuleTestWithSignal : public LicensingModuleTest
{
public:
    SignalConfigPtr signal;
    SignalConfigPtr signalDomain;
    DataPacketPtr domainPacket;
    DataPacketPtr valuePacket;
    FunctionBlockPtr fb;
    StreamReaderPtr reader;
    SizeT noOfSamples = 100;

protected:
    void SetUp() override
    {
        LicensingModuleTest::SetUp();
        setupSignalPackets();
    }

    void setupSignalPackets()
    {
        signal = Signal(context, nullptr, "ramp");
        signalDomain = Signal(context, nullptr, "domain");

        signal.setDomainSignal(signalDomain);

        //next step: figure out if this entire description is necessary (you can play around with it after you setup the pipeline too)
        const auto domainDescriptor = DataDescriptorBuilder()
                                        .setSampleType(SampleType::Int64)
                                        .setTickResolution(Ratio(1, 1000))
                                        .setOrigin("1970-01-01T00:00:00")
                                        .setRule(LinearDataRule(1, 0))
                                        .setUnit(Unit("s", -1, "second", "time"))
                                        .build();

        signalDomain.setDescriptor(domainDescriptor);

        const auto valueDescriptor = DataDescriptorBuilder().setSampleType(SampleType::Float32).build();
        signal.setDescriptor(valueDescriptor);
        domainPacket = DataPacket(domainDescriptor, noOfSamples, 0);

        valuePacket = DataPacketWithDomain(domainPacket, valueDescriptor, noOfSamples);
        auto valueDataRaw = static_cast<float*>(valuePacket.getRawData());
        for (size_t i = 0; i < noOfSamples; i++)
            *valueDataRaw++ = static_cast<float>(i);
    }

    void createPassthroughFb()
    {
        auto tmp = module.createFunctionBlock("LicensingModulePassthrough", nullptr, "id");
        fb = tmp;
    }

    void authenticateValid()
    {
        auto config = module.getAuthenticationConfig();
        config.setPropertyValue("AuthenticationKeyPath", resourcesPath + "/authentication_key.txt");
        module.authenticate(config);
    }

    void authenticateInvalid()
    {
        auto config = module.getAuthenticationConfig();
        config.setPropertyValue("AuthenticationKeyPath", "");
        module.authenticate(config);
    }

    void loadValidLicense()
    {
        auto config = module.getLicenseConfig();
        config.setPropertyValue("LicensePath", resourcesPath + "/license.lic");
        module.loadLicense(config);
    }

    void loadInvalidLicense()
    {
        auto config = module.getLicenseConfig();
        config.setPropertyValue("LicensePath", "");
        module.loadLicense(config);
    }

    void setupPipeline()
    {
        fb.getInputPorts()[0].connect(signal);

        reader = StreamReaderBuilder()
                     .setSignal(fb.getSignals()[0])
                     .setValueReadType(SampleType::Float32)
                     .setDomainReadType(SampleType::Int64)
                     .setReadMode(ReadMode::Scaled)
                     .setReadTimeoutType(ReadTimeoutType::All)
                     .setSkipEvents(true)
                     .build();
    }

    bool canReadSignal()
    {
        signalDomain.sendPacket(domainPacket);
        signal.sendPacket(valuePacket);

        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        std::vector<float> data(noOfSamples);
        std::vector<int64_t> time(noOfSamples);

        SizeT noOfSamplesRead = noOfSamples;
        const auto readerStatus = reader.readWithDomain(data.data(), time.data(), &noOfSamplesRead);

        return noOfSamplesRead == noOfSamples;
    }
};

TEST_F(ModuleTest, CreateModule)
{
    IModule* module = nullptr;
    ErrCode errCode = createModule(&module, NullContext());
    ASSERT_TRUE(OPENDAQ_SUCCEEDED(errCode));

    ASSERT_NE(module, nullptr);
    module->releaseRef();
}

TEST_F(ModuleTest, ModuleName)
{
    auto module = CreateModule();
    ASSERT_EQ(module.getModuleInfo().getName(), "LicensingModule");
}

bool validAuthenticationWorks = false;
TEST_F(LicensingModuleTestWithSignal, ValidAuthentication)
{
    authenticateValid();

    ASSERT_TRUE(module.isAuthenticated());

    createPassthroughFb();

    validAuthenticationWorks = !fb.isEmpty();
    ASSERT_TRUE(validAuthenticationWorks);
    
}

TEST_F(LicensingModuleTestWithSignal, InvalidAuthentication)
{
    authenticateInvalid();

    ASSERT_FALSE(module.isAuthenticated());

    createPassthroughFb();

    // Cannot get function block from an un-authenticated module
    ASSERT_TRUE(fb == nullptr);
}

TEST_F(LicensingModuleTestWithSignal, NoAuthentication)
{
    ASSERT_FALSE(module.isAuthenticated());

    loadValidLicense();
    createPassthroughFb();

    // Cannot get function block from an un-authenticated module
    ASSERT_TRUE(fb == nullptr);
}

TEST_F(LicensingModuleTestWithSignal, MultipleAuthentication)
{
    if (!validAuthenticationWorks)
        GTEST_SKIP();

    createPassthroughFb();
    ASSERT_TRUE(fb == nullptr);

    authenticateValid();

    createPassthroughFb();
    ASSERT_TRUE(!fb.isEmpty());

    authenticateInvalid();

    createPassthroughFb();
    ASSERT_TRUE(fb == nullptr);
    
    authenticateValid();

    createPassthroughFb();
    ASSERT_TRUE(!fb.isEmpty());
}

TEST_F(LicensingModuleTestWithSignal, ValidLicense)
{
    if (!validAuthenticationWorks)
        GTEST_SKIP();

    authenticateValid();

    loadValidLicense();

    createPassthroughFb();
    setupPipeline();

    ASSERT_TRUE(canReadSignal());
}

TEST_F(LicensingModuleTestWithSignal, InvalidLicense)
{
    if (!validAuthenticationWorks)
        GTEST_SKIP();

    authenticateValid();

    loadInvalidLicense();

    createPassthroughFb();
    setupPipeline();

    ASSERT_FALSE(canReadSignal());
}

TEST_F(LicensingModuleTestWithSignal, NoLicense)
{
    if (!validAuthenticationWorks)
        GTEST_SKIP();

    authenticateValid();

    createPassthroughFb();
    setupPipeline();

    ASSERT_FALSE(canReadSignal());
}

