#include <audio_device_module/audio_device_module_impl.h>
#include <audio_device_module/audio_device_impl.h>
#include <audio_device_module/wav_writer_fb_impl.h>
#include <audio_device_module/wav_reader_fb_impl.h>
#include <audio_device_module/version.h>
#include <coretypes/version_info_factory.h>
#include <miniaudio/miniaudio.h>
#include <opendaq/custom_log.h>
#include <audio_device_module/audio_device_utils.h>

#ifdef _WIN32
#include <combaseapi.h>
#endif


BEGIN_NAMESPACE_AUDIO_DEVICE_MODULE

AudioDeviceModule::AudioDeviceModule(const ContextPtr& context)
    : Module("AudioDeviceModule",
             VersionInfo(AUDIO_DEVICE_MODULE_MAJOR_VERSION, AUDIO_DEVICE_MODULE_MINOR_VERSION, AUDIO_DEVICE_MODULE_PATCH_VERSION),
             context,
             "AudioDeviceModule")
      , maContext(std::make_shared<MiniaudioContext>())
      , deviceIndex(0)
{
}

ListPtr<IDeviceInfo> AudioDeviceModule::onGetAvailableDevices()
{
    ma_device_info* pCaptureDeviceInfos;
    ma_uint32 captureDeviceCount;
    
    std::scoped_lock lock(sync);

    getMiniAudioDeviceInfo(&pCaptureDeviceInfos, &captureDeviceCount);
    auto availableDevices = List<IDeviceInfo>();
    for (size_t i = 0; i < captureDeviceCount; i++)
        availableDevices.pushBack(AudioDeviceImpl::CreateDeviceInfo(maContext, pCaptureDeviceInfos[i]));

    return availableDevices;
}

DictPtr<IString, IDeviceType> AudioDeviceModule::onGetAvailableDeviceTypes()
{
    auto deviceType = AudioDeviceImpl::createType();
    return Dict<IString, IDeviceType>({{deviceType.getId(), deviceType}});
}

DevicePtr AudioDeviceModule::onCreateDevice(const StringPtr& connectionString,
                                            const ComponentPtr& parent,
                                            const PropertyObjectPtr& /*config*/)
{
    auto id = utils::getIdFromConnectionString(connectionString);

    std::scoped_lock lock(sync);

    // TODO: Get better local ID
    std::string localId = fmt::format("MiniAudioDev{}", deviceIndex++);

    auto devicePtr = createWithImplementation<IDevice, AudioDeviceImpl>(maContext, id, context, parent, StringPtr(localId));
    return devicePtr;
}

DictPtr<IString, IFunctionBlockType> AudioDeviceModule::onGetAvailableFunctionBlockTypes()
{
    auto types = Dict<IString, IFunctionBlockType>();

    auto typeWriter = WAVWriterFbImpl::CreateType(moduleInfo);
    auto typeReader = WAVReaderFbImpl::CreateType(moduleInfo);

    types.set(typeWriter.getId(), typeWriter);
    types.set(typeReader.getId(), typeReader);

    return types;
}

FunctionBlockPtr AudioDeviceModule::onCreateFunctionBlock(const StringPtr& id, const ComponentPtr& parent, const StringPtr& localId, const PropertyObjectPtr& config)
{
    if (id == WAVWriterFbImpl::CreateType(moduleInfo).getId())
    {
        FunctionBlockPtr fb = createWithImplementation<IFunctionBlock, WAVWriterFbImpl>(moduleInfo, context, parent, localId);
        return fb;
    }
    if (id == WAVReaderFbImpl::CreateType(moduleInfo).getId())
    {
        FunctionBlockPtr fb = createWithImplementation<IFunctionBlock, WAVReaderFbImpl>(moduleInfo, context, parent, localId);
        return fb;
    }

    LOG_W("Function block \"{}\" not found", id);
    DAQ_THROW_EXCEPTION(NotFoundException, "Function block not found");
}

void AudioDeviceModule::getMiniAudioDeviceInfo(ma_device_info** ppCaptureDeviceInfos, ma_uint32* pCaptureDeviceCount) const
{
    ma_result result;
#ifdef MA_WIN32
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);
#endif

    result = ma_context_get_devices(maContext->getPtr(), nullptr, nullptr, ppCaptureDeviceInfos, pCaptureDeviceCount);
    if (result != MA_SUCCESS)
    {
        LOG_W("Miniaudio get devices failed: {}", ma_result_description(result));
        DAQ_THROW_EXCEPTION(GeneralErrorException, "Failed to retrieve device information");
    }

#ifdef MA_WIN32
    CoUninitialize();
#endif
}

END_NAMESPACE_AUDIO_DEVICE_MODULE
