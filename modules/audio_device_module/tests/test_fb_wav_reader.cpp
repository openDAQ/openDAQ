#include <testutils/testutils.h>
#include <audio_device_module/module_dll.h>
#include <opendaq/context_factory.h>
#include <opendaq/scheduler_factory.h>
#include <opendaq/module_manager_factory.h>
#include <opendaq/context_internal_ptr.h>
#include <opendaq/reader_factory.h>
#include <filesystem>
#include <thread>
#include <string>


using AudioDeviceModuleTest = testing::Test;
namespace fs = std::filesystem;
using namespace daq;
std::string wavReaderResourcePath = TEST_RESOURCE_PATH;

inline std::string resolvePath(const std::string& relPath)
{
    auto baseDir = fs::current_path();
    while (baseDir.has_parent_path())
    {
        auto combinePath = baseDir / relPath;
        if (fs::exists(combinePath))
        {
            return combinePath.string();
        }
        if (baseDir.parent_path() == baseDir)
        {
            break;
        }
        baseDir = baseDir.parent_path();
    }
    throw std::runtime_error("File not found!");
}

class WavReaderTest : public testing::Test
{
public:
    FunctionBlockPtr fb;
    ContextPtr context;

protected:
    void SetUp() override
    {
        // Create module
        ModulePtr module;
        const auto logger = Logger();
        auto moduleManager = ModuleManager("[[none]]");
        context = Context(Scheduler(logger), logger, TypeManager(), moduleManager, nullptr);
        createModule(&module, context);
        moduleManager.addModule(module);
        moduleManager = context.asPtr<IContextInternal>().moveModuleManager();

        // Create function block
        fb = module.createFunctionBlock("AudioDeviceModuleWavReader", nullptr, "fb");
    }
};

TEST_F(AudioDeviceModuleTest, TestFilesExist)
{
    ASSERT_NO_THROW(resolvePath(wavReaderResourcePath + "\\quack_mono_16bit_44kHz.wav"));
    ASSERT_NO_THROW(resolvePath(wavReaderResourcePath + "\\quack_mono_16bit_44kHz_cut.wav"));
    ASSERT_NO_THROW(resolvePath(wavReaderResourcePath + "\\quack_mono_32bit_48kHz.wav"));
    ASSERT_NO_THROW(resolvePath(wavReaderResourcePath + "\\quack_stereo_16bit_44kHz.wav"));
}

TEST_F(WavReaderTest, StatusOk)
{
    ASSERT_EQ(fb.getStatusContainer().getStatus("ComponentStatus"), Enumeration("ComponentStatusType", "Warning", context.getTypeManager()));
}

TEST_F(WavReaderTest, OpenValidFile)
{
    ASSERT_EQ(fb.getStatusContainer().getStatus("ComponentStatus"), Enumeration("ComponentStatusType", "Warning", context.getTypeManager()));

    fb.setPropertyValue("FilePath", wavReaderResourcePath + "/quack_mono_16bit_44kHz.wav");

    ASSERT_EQ(fb.getStatusContainer().getStatus("ComponentStatus"), Enumeration("ComponentStatusType", "Ok", context.getTypeManager()));
}

TEST_F(WavReaderTest, OpenInvalidFile)
{
    ASSERT_EQ(fb.getStatusContainer().getStatus("ComponentStatus"), Enumeration("ComponentStatusType", "Warning", context.getTypeManager()));

    fb.setPropertyValue("FilePath", wavReaderResourcePath + "/fake_quack.wav");

    ASSERT_EQ(fb.getStatusContainer().getStatus("ComponentStatus"), Enumeration("ComponentStatusType", "Warning", context.getTypeManager()));
}

TEST_F(WavReaderTest, CheckSignalSampleType)
{
    fb.setPropertyValue("FilePath", wavReaderResourcePath + "/quack_mono_16bit_44kHz.wav");

    auto descriptor = fb.getSignals()[0].getDescriptor();
    daq::SampleType sampleType;
    descriptor->getSampleType(&sampleType);

    ASSERT_EQ(sampleType, SampleType::Int16);

    fb.setPropertyValue("FilePath", wavReaderResourcePath + "/quack_mono_32bit_48kHz.wav");

    descriptor = fb.getSignals()[0].getDescriptor();
    descriptor->getSampleType(&sampleType);

    ASSERT_EQ(sampleType, SampleType::Int32);
}

TEST_F(WavReaderTest, DISABLED_OpenStereo)
{
    fb.setPropertyValue("FilePath", wavReaderResourcePath + "/quack_stereo_16bit_44kHz.wav");

    //TODO: when stereo is supported check if both signals are correct
}

TEST_F(WavReaderTest, ReadFile)
{
    fb.setPropertyValue("FilePath", wavReaderResourcePath + "/quack_mono_16bit_44kHz_cut.wav");

    ASSERT_EQ(fb.getStatusContainer().getStatus("ComponentStatus"), Enumeration("ComponentStatusType", "Ok", context.getTypeManager()));
    bool reading = fb.getPropertyValue("Reading");
    ASSERT_FALSE(reading);

    auto signal = fb.getSignals()[0];

    const auto reader = StreamReaderBuilder()
                            .setSkipEvents(True)
                            .setSignal(signal)
                            .setValueReadType(SampleType::Int16)
                            .setDomainReadType(SampleType::Int64)
                            .setReadMode(ReadMode::Scaled)
                            .setReadTimeoutType(ReadTimeoutType::All)
                            .build();

    std::vector<short> data(100);
    std::vector<int64_t> time(100);
    SizeT count = 100;

    fb.setPropertyValue("Reading", true);
    bool eof = fb.getPropertyValue("EOF");

    ASSERT_FALSE(eof);
    reader.readWithDomain(data.data(), time.data(), &count, 10000);
    ASSERT_EQ(count, 100);

    while (bool framesAvailable = !fb.getPropertyValue("EOF"))
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    eof = fb.getPropertyValue("EOF");
    reading = fb.getPropertyValue("Reading");
    ASSERT_TRUE(eof);
    ASSERT_TRUE(reading);

    ASSERT_EQ(fb.getStatusContainer().getStatus("ComponentStatus"), Enumeration("ComponentStatusType", "Warning", context.getTypeManager()));
    ASSERT_EQ(fb.getStatusContainer().getStatusMessage("ComponentStatus"), "End of file reached.");

    // Open "new" file.
    fb.setPropertyValue("FilePath", wavReaderResourcePath + "/quack_mono_16bit_44kHz.wav");

    ASSERT_EQ(fb.getStatusContainer().getStatus("ComponentStatus"), Enumeration("ComponentStatusType", "Ok", context.getTypeManager()));
    eof = fb.getPropertyValue("EOF");
    reading = fb.getPropertyValue("Reading");
    ASSERT_FALSE(eof);
    ASSERT_FALSE(reading);
}
