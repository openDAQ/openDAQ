#include <audio_device_module/audio_device_impl.h>
#include <audio_device_module/audio_channel_impl.h>
#include <opendaq/device_info_factory.h>
#include <opendaq/device_type_factory.h>
#include <opendaq/device_domain_factory.h>
#include <miniaudio/miniaudio.h>

BEGIN_NAMESPACE_AUDIO_DEVICE_MODULE

AudioDeviceImpl::AudioDeviceImpl(const std::shared_ptr<ma_utils::MiniaudioContext>& maContext,
                                 const ma_device_id& id,
                                 const ContextPtr& ctx,
                                 const ComponentPtr& parent,
                                 const StringPtr& localId)
    : GenericDevice(ctx, parent, localId)
    , maId(id)
    , maContext(maContext)
    , started(false)
{
    loggerComponent = context.getLogger().getOrAddComponent("AudioDeviceModule");

    initProperties();
    createAudioChannel();
    start();
    
    setDeviceInfo();
    setDeviceDomain(DeviceDomain(Ratio(1, maDevice.sampleRate), "", Unit("s", -1, "seconds", "time")));
}

AudioDeviceImpl::~AudioDeviceImpl()
{
    auto lock = getAcquisitionLock2();
    stop();
}

DeviceTypePtr AudioDeviceImpl::createType()
{
    return DeviceType("MiniAudio", "Audio device", "", "miniaudio");
}

DeviceInfoPtr AudioDeviceImpl::CreateDeviceInfo(const std::shared_ptr<ma_utils::MiniaudioContext>& maContext, const ma_device_info& deviceInfo)
{
    auto connectionString = ma_utils::getConnectionStringFromId(maContext->getPtr()->backend, deviceInfo.id);
    auto devInfo = DeviceInfo(connectionString);
    devInfo.setName(deviceInfo.name);
    devInfo.setDeviceType(createType());

    return devInfo;
}

uint64_t AudioDeviceImpl::onGetTicksSinceOrigin()
{
    return channel.asPtr<IAudioChannel>()->getSamplesGenerated();
}

void AudioDeviceImpl::initProperties()
{
    auto availableSRSelectionDict = Dict<IInteger, IInteger>({{3, 11025}, {6, 22050}, {9, 44100}});
    auto sampleRateProperty = SparseSelectionPropertyBuilder("SampleRate", availableSRSelectionDict, 3).setUnit(Unit("Hz")).build();

    objPtr.addProperty(sampleRateProperty);
    objPtr.getOnPropertyValueWrite("SampleRate") +=
        [this](PropertyObjectPtr&, PropertyValueEventArgsPtr& args)
        {
            DictPtr<Int, Int> selectionValues = args.getProperty().getSelectionValues();
            sampleRateChanged(selectionValues.get(args.getValue()));
        };

    sampleRate = objPtr.getPropertySelectionValue("SampleRate");
}

void AudioDeviceImpl::createAudioChannel()
{
    channel = createAndAddChannel<AudioChannelImpl>(ioFolder, "audio");
}

void AudioDeviceImpl::setDeviceInfo()
{
    ma_device_info info;
    ma_utils::getMiniAudioDeviceInfo(&maDevice, &info);
    this->deviceInfo = CreateDeviceInfo(maContext, info);
}

void AudioDeviceImpl::sampleRateChanged(uint32_t sampleRate)
{
    stop();
    this->sampleRate = sampleRate;
    start();
}

static void MiniaudioDataCallback(ma_device* pDevice, void*, const void* pInput, ma_uint32 frameCount)
{
    auto audioChannel = static_cast<AudioChannelImpl*>(pDevice->pUserData);
    audioChannel->generatePackets(pInput, frameCount);
}

void AudioDeviceImpl::start()
{
    if (started || disposeCalled)
        return;
    
    ma_device_config devConfig = ma_device_config_init(ma_device_type_capture);
    devConfig.capture.pDeviceID = &maId;
    devConfig.capture.channels = 1;
    devConfig.capture.format = ma_format_f32;
    devConfig.sampleRate = sampleRate;
    devConfig.dataCallback = MiniaudioDataCallback;
    devConfig.pUserData = reinterpret_cast<void*>(channel.getObject());

    if (!ma_utils::initAudioDevice(maContext->getPtr(), &devConfig, &maDevice, loggerComponent))
        return;

    channel.asPtr<IAudioChannel>()->configure(maDevice);
    channel.asPtr<IAudioChannel>()->reset();

    if (!ma_utils::startAudioDevice(&maDevice, loggerComponent))
        return;

    started = true;
}

void AudioDeviceImpl::stop()
{
    if (!started)
        return;

    ma_utils::uninitializeDevice(&maDevice);
    started = false;
}

END_NAMESPACE_AUDIO_DEVICE_MODULE
