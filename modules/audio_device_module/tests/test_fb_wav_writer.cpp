#include <testutils/testutils.h>
#include <audio_device_module/module_dll.h>
#include <audio_device_module/version.h>
#include <gmock/gmock.h>
#include <opendaq/module_ptr.h>
#include <opendaq/device_ptr.h>
#include <coretypes/common.h>
#include <opendaq/context_factory.h>
#include <opendaq/scheduler_factory.h>
#include <opendaq/module_manager_factory.h>
#include <opendaq/context_internal_ptr.h>
#include <opendaq/reader_factory.h>
#include <opendaq/recorder_ptr.h>
#include <filesystem>
#include <iostream>


using AudioDeviceModuleTest = testing::Test;

namespace fs = std::filesystem;
using namespace daq;

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

class WavWriterTest : public testing::Test
{
public:
    FunctionBlockPtr reader;
    FunctionBlockPtr writerFb;
    RecorderPtr writerRe;
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
        reader = module.createFunctionBlock("AudioDeviceModuleWavReader", nullptr, "reader");
        writerFb = module.createFunctionBlock("AudioDeviceModuleWavWriter", nullptr, "writer");
        writerRe = writerFb;
    }
};

TEST_F(WavWriterTest, ValidWriteToFile)
{
    reader.setPropertyValue("FilePath", "resources\\quack_mono_16bit_44kHz_cut.wav");

    ASSERT_EQ(reader.getStatusContainer().getStatus("ComponentStatus"), Enumeration("ComponentStatusType", "Ok", context.getTypeManager()));

    writerFb.getInputPorts()[0].connect(reader.getSignals()[0]);
    writerFb.setPropertyValue("FileName", "recorded_quack.wav");

    reader.setPropertyValue("Reading", true);

    ASSERT_NO_THROW(writerRe.startRecording());
    ASSERT_TRUE(writerRe.getIsRecording());

    std::this_thread::sleep_for(std::chrono::seconds(1));

    ASSERT_NO_THROW(writerRe.stopRecording());
    ASSERT_FALSE(writerRe.getIsRecording());

    reader.setPropertyValue("Reading", false);

    std::string path;
    ASSERT_NO_THROW(path = resolvePath("recorded_quack.wav"));

    ASSERT_TRUE(fs::remove(path));
}

TEST_F(WavWriterTest, InvalidWriteToFile)
{
    writerFb.setPropertyValue("FileName", "recorded_quack.wav");

    ASSERT_ANY_THROW(writerRe.startRecording());
    ASSERT_FALSE(writerRe.getIsRecording());

    ASSERT_ANY_THROW(writerRe.stopRecording());
    ASSERT_FALSE(writerRe.getIsRecording());

    ASSERT_ANY_THROW(resolvePath("recorded_quack.wav"));
}
