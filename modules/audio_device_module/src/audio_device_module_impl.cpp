#include <audio_device_module/audio_device_module_impl.h>
#include <audio_device_module/audio_device_impl.h>
#include <audio_device_module/wav_writer_fb_impl.h>
#include <audio_device_module/version.h>
#include <coretypes/version_info_factory.h>
#include <miniaudio/miniaudio.h>
#include <opendaq/custom_log.h>

#ifdef _WIN32
#include <combaseapi.h>
#endif


BEGIN_NAMESPACE_AUDIO_DEVICE_MODULE

AudioDeviceModule::AudioDeviceModule(const ContextPtr& context)
    : Module("AudioDeviceModule",
            daq::VersionInfo(AUDIO_DEVICE_MODULE_MAJOR_VERSION, AUDIO_DEVICE_MODULE_MINOR_VERSION, AUDIO_DEVICE_MODULE_PATCH_VERSION),
            context,
            "AudioDeviceModule")
    , maContext(std::make_shared<MiniaudioContext>())
    , deviceIndex(0)
{
}

ListPtr<IDeviceInfo> AudioDeviceModule::onGetAvailableDevices()
{
    ma_result result;
    ma_device_info* pCaptureDeviceInfos;
    ma_uint32 captureDeviceCount;

#ifdef MA_WIN32
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);
#endif

    std::scoped_lock lock(sync);

    result = ma_context_get_devices(maContext->getPtr(), nullptr, nullptr, &pCaptureDeviceInfos, &captureDeviceCount);
    if (result != MA_SUCCESS)
    {
        LOG_W("Miniaudio get devices failed: {}", ma_result_description(result));
        throw GeneralErrorException("Failed to retrieve device information");
    }

    auto availableDevices = List<IDeviceInfo>();
    for (size_t i = 0; i < captureDeviceCount; i++)
    {
        auto info = AudioDeviceImpl::CreateDeviceInfo(maContext, pCaptureDeviceInfos[i]);
        availableDevices.pushBack(info);
    }

#ifdef MA_WIN32
    CoUninitialize();
#endif

    return availableDevices;
}

DictPtr<IString, IDeviceType> AudioDeviceModule::onGetAvailableDeviceTypes()
{
    auto result = Dict<IString, IDeviceType>();

    auto deviceType = AudioDeviceImpl::createType();
    result.set(deviceType.getId(), deviceType);

    return result;
}

DevicePtr AudioDeviceModule::onCreateDevice(const StringPtr& connectionString,
                                            const ComponentPtr& parent,
                                            const PropertyObjectPtr& /*config*/)
{
    auto id = AudioDeviceImpl::getIdFromConnectionString(connectionString);

    std::scoped_lock lock(sync);

    std::string localId = fmt::format("MiniAudioDev{}", deviceIndex++);

    auto devicePtr = createWithImplementation<IDevice, AudioDeviceImpl>(maContext, id, context, parent, StringPtr(localId));
    return devicePtr;
}

DictPtr<IString, IFunctionBlockType> AudioDeviceModule::onGetAvailableFunctionBlockTypes()
{
    auto types = Dict<IString, IFunctionBlockType>();

    auto type = WAVWriterFbImpl::CreateType();
    types.set(type.getId(), type);

    return types;
}

FunctionBlockPtr AudioDeviceModule::onCreateFunctionBlock(const StringPtr& id, const ComponentPtr& parent, const StringPtr& localId, const PropertyObjectPtr& config)
{
    if (id == WAVWriterFbImpl::CreateType().getId())
    {
        FunctionBlockPtr fb = createWithImplementation<IFunctionBlock, WAVWriterFbImpl>(context, parent, localId);
        return fb;
    }

    LOG_W("Function block \"{}\" not found", id);
    throw NotFoundException("Function block not found");
}

END_NAMESPACE_AUDIO_DEVICE_MODULE
