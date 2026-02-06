#include <audio_device_module/audio_device_module_impl.h>
#include <audio_device_module/audio_device_impl.h>
#include <audio_device_module/wav_writer_fb_impl.h>
#include <audio_device_module/wav_reader_fb_impl.h>
#include <audio_device_module/version.h>
#include <coretypes/version_info_factory.h>
#include <opendaq/custom_log.h>

#ifdef _WIN32
#include <combaseapi.h>
#endif

BEGIN_NAMESPACE_AUDIO_DEVICE_MODULE

AudioDeviceModule::AudioDeviceModule(const ContextPtr& context)
    : Module("AudioDeviceModule",
             VersionInfo(AUDIO_DEVICE_MODULE_MAJOR_VERSION, AUDIO_DEVICE_MODULE_MINOR_VERSION, AUDIO_DEVICE_MODULE_PATCH_VERSION),
             context,
             "AudioDeviceModule")
      , maContext(std::make_shared<ma_utils::MiniaudioContext>())
      , deviceIndex(0)
{
}

ListPtr<IDeviceInfo> AudioDeviceModule::onGetAvailableDevices()
{
    ma_device_info* pCaptureDeviceInfos;
    ma_uint32 captureDeviceCount;
    
    std::scoped_lock lock(sync);
    ma_utils::getMiniAudioDevices(&pCaptureDeviceInfos, &captureDeviceCount, maContext->getPtr());

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
    auto id = ma_utils::getIdFromConnectionString(connectionString);

    std::scoped_lock lock(sync);
    std::string localId = fmt::format("MiniAudioDev{}", deviceIndex++);

    auto devicePtr = createWithImplementation<IDevice, AudioDeviceImpl>(maContext, id, context, parent, StringPtr(localId));
    return devicePtr;
}

DictPtr<IString, IFunctionBlockType> AudioDeviceModule::onGetAvailableFunctionBlockTypes()
{
    auto types = Dict<IString, IFunctionBlockType>();

    auto typeWriter = WAVWriterFbImpl::CreateType();
    auto typeReader = WAVReaderFbImpl::CreateType();

    types.set(typeWriter.getId(), typeWriter);
    types.set(typeReader.getId(), typeReader);

    return types;
}

FunctionBlockPtr AudioDeviceModule::onCreateFunctionBlock(const StringPtr& id, const ComponentPtr& parent, const StringPtr& localId, const PropertyObjectPtr& config)
{
    if (id == WAVWriterFbImpl::CreateType().getId())
    {
        FunctionBlockPtr fb = createWithImplementation<IFunctionBlock, WAVWriterFbImpl>(context, parent, localId);
        return fb;
    }
    if (id == WAVReaderFbImpl::CreateType().getId())
    {
        FunctionBlockPtr fb = createWithImplementation<IFunctionBlock, WAVReaderFbImpl>(context, parent, localId);
        return fb;
    }

    LOG_W("Function block \"{}\" not found", id);
    DAQ_THROW_EXCEPTION(NotFoundException, "Function block not found");
}

END_NAMESPACE_AUDIO_DEVICE_MODULE
