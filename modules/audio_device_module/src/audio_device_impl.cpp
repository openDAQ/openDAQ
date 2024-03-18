#include <audio_device_module/audio_device_impl.h>
#include <opendaq/device_info_factory.h>
#include <coreobjects/unit_factory.h>
#include <audio_device_module/audio_channel_impl.h>
#include <boost/locale.hpp>
#include <opendaq/signal_factory.h>
#include <opendaq/packet_factory.h>
#include <opendaq/data_rule_factory.h>
#include <opendaq/custom_log.h>
#include <opendaq/device_type_factory.h>
#include <opendaq/device_domain_factory.h>

BEGIN_NAMESPACE_AUDIO_DEVICE_MODULE

AudioDeviceImpl::AudioDeviceImpl(const std::shared_ptr<MiniaudioContext>& maContext, const ma_device_id& id, const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId)
    : GenericDevice<>(ctx, parent, localId)
    , maId(id)
    , maContext(maContext)
    , started(false)
    , logger(ctx.getLogger())
    , loggerComponent( this->logger.assigned()
                          ? this->logger.getOrAddComponent("AudioDevice")
                          : throw ArgumentNullException("Logger must not be null"))
{
    // time signal is owned by device, because in case of multiple channels they should share the same time signal
    timeSignal = createAndAddSignal("time");

    initProperties();
    createAudioChannel();

    start();
    
    this->setDeviceDomain(DeviceDomain(Ratio(1, maDevice.sampleRate),
                                       "",
                                       UnitBuilder().setName("second").setSymbol("s").setQuantity("time").build()));
}

AudioDeviceImpl::~AudioDeviceImpl()
{
    stop();
}

DeviceInfoPtr AudioDeviceImpl::CreateDeviceInfo(const std::shared_ptr<MiniaudioContext>& maContext, const ma_device_info& deviceInfo)
{
    auto devInfo = DeviceInfo(getConnectionStringFromId(maContext->getPtr()->backend, deviceInfo.id));
    devInfo.setName(deviceInfo.name);
    devInfo.setDeviceType(createType());

    return devInfo;
}

DeviceInfoPtr AudioDeviceImpl::onGetInfo()
{
    ma_result result;
    ma_device_info info;
    result = ma_device_get_info(&maDevice, ma_device_type_capture, &info);
    if (result != MA_SUCCESS)
    {
        LOG_W("Miniaudio get device information failed: {}", ma_result_description(result));
    }
    return CreateDeviceInfo(maContext, info);
}

uint64_t AudioDeviceImpl::onGetTicksSinceOrigin()
{
    return 0;
}

void AudioDeviceImpl::initProperties()
{
    auto sampleRatePropInfo = IntPropertyBuilder("SampleRate", 44100).setSuggestedValues(List<Int>(11025, 22050, 44100)).build();
    objPtr.addProperty(sampleRatePropInfo);
    objPtr.getOnPropertyValueWrite("SampleRate") +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChanged(); };

    readProperties();
}

static void miniaudioDataCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
    auto this_ = static_cast<AudioDeviceImpl*>(pDevice->pUserData);
    this_->addData(pInput, frameCount);
}

void AudioDeviceImpl::addData(const void* data, size_t sampleCount)
{
    try
    {
        auto domainPacket = DataPacket(timeSignal.getDescriptor(), sampleCount, samplesCaptured);
        channel.asPtr<IAudioChannel>()->addData(domainPacket, data, sampleCount);
        samplesCaptured += sampleCount;
    }
    catch (const std::exception& e)
    {
        LOG_W("addData failed: {}", e.what());
        throw;
    }
}

void AudioDeviceImpl::start()
{
    if (started || disposeCalled)
        return;

    ma_result result;
    ma_device_config devConfig = ma_device_config_init(ma_device_type_capture);
    devConfig.capture.pDeviceID = &maId;
    devConfig.capture.channels = 1;
    devConfig.capture.format = ma_format_f32;
    devConfig.sampleRate = sampleRate;
    devConfig.dataCallback = miniaudioDataCallback;
    devConfig.pUserData = reinterpret_cast<void*>(this);

    if ((result = ma_device_init(maContext->getPtr(), &devConfig, &maDevice)) != MA_SUCCESS)
    {
        LOG_W("Miniaudio device init failed: {}", ma_result_description(result));
        return;
    }

    configure();

    samplesCaptured = 0;

    if ((result = ma_device_start(&maDevice)) != MA_SUCCESS)
    {
        LOG_W("Miniaudio device start failed: {}", ma_result_description(result));
        ma_device_uninit(&maDevice);
        return;
    }

    started = true;
}

void AudioDeviceImpl::stop()
{
    if (!started)
        return;

    ma_device_uninit(&maDevice);

    started = false;
}

void AudioDeviceImpl::readProperties()
{
    sampleRate = objPtr.getPropertyValue("SampleRate");
    LOG_I("Properties: SampleRate {}", sampleRate);
}

void AudioDeviceImpl::createAudioChannel()
{
    channel = createAndAddChannel<AudioChannelImpl>(ioFolder, "audio");
}

void AudioDeviceImpl::propertyChanged()
{
    std::scoped_lock lock(sync);

    stop();

    readProperties();

    start();
}

void AudioDeviceImpl::configureTimeSignal()
{
    auto dataDescriptor = DataDescriptorBuilder()
                              .setSampleType(SampleType::Int64)
                              .setTickResolution(Ratio(1, maDevice.sampleRate))
                              .setRule(LinearDataRule(1, 0))
                              .setUnit(Unit("s", -1, "second", "time"))
                              .setName("Time")
                              .build();

    timeSignal.setDescriptor(dataDescriptor);
}

void AudioDeviceImpl::configure()
{
    channel.asPtr<IAudioChannel>()->configure(maDevice, timeSignal);
    configureTimeSignal();
}

std::string AudioDeviceImpl::getConnectionStringFromId(ma_backend backend, ma_device_id id)
{
    std::string connStr = "miniaudio://";
    switch (backend)
    {
    case ma_backend_wasapi:
    {
        connStr += "wasapi/";
        std::string wasapiId = boost::locale::conv::utf_to_utf<char>(id.wasapi);
        connStr += wasapiId;
    }
    break;
    case ma_backend_dsound:
    {
        connStr += "dsound/";
        for (size_t i = 0; i < 16; i++)
            connStr += fmt::format("{:02x}", id.dsound[i]);
    }
    break;
    case ma_backend_winmm:
    {
        connStr += "winmm/";
        connStr += fmt::format("{:x}", id.winmm);
    }
    break;
    case ma_backend_coreaudio:
    {
        connStr += "coreaudio/";
        connStr += id.coreaudio;
    }
    break;
    case ma_backend_sndio:
        connStr += "sndio/";
        break;
    case ma_backend_audio4:
        connStr += "audio4/";
        break;
    case ma_backend_oss:
        connStr += "oss/";
        break;
    case ma_backend_pulseaudio:
    {
        connStr += "pulseaudio/";
        connStr += id.pulse;
    }
    break;
    case ma_backend_alsa:
    {
        connStr += "alsa/";
        connStr += id.alsa;
        break;
    }
    case ma_backend_jack:
    {
        connStr += "jack/";
        connStr += fmt::format("{:x}", id.jack);
    }
    break;
    case ma_backend_aaudio:
        connStr += "aaudio/";
        break;
    case ma_backend_opensl:
        connStr += "opensl/";
        break;
    default:
        connStr += "unknown/";
    }

    return connStr;
}

DeviceTypePtr AudioDeviceImpl::createType()
{
    return DeviceType("miniaudio", "Audio device", "");
}

ma_device_id AudioDeviceImpl::getIdFromConnectionString(std::string connectionString)
{
    std::string prefix = "miniaudio://";
    auto found = connectionString.find(prefix);
    if (found != 0)
        throw InvalidParameterException();

    auto backendWithIdStr = connectionString.substr(prefix.size(), std::string::npos);
    std::string slash = "/";
    found = backendWithIdStr.find(slash);
    if (found == std::string::npos)
        throw InvalidParameterException();

    auto backendStr = backendWithIdStr.substr(0, found);
    std::string idStr = backendWithIdStr.substr(found + 1, std::string::npos);

    ma_device_id devId;
    std::memset(&devId, 0, sizeof(ma_device_id));

    if (backendStr == "wasapi")
    {
        std::wstring wasapiId = boost::locale::conv::utf_to_utf<wchar_t>(idStr);
        std::memcpy(&devId.wasapi[0], wasapiId.c_str(), wasapiId.size() * sizeof(wchar_t));
        devId.wasapi[wasapiId.size()] = '\0';
    }
    else if (backendStr == "dsound")
    {
        for (size_t i = 0; i < 16; ++i)
        {
            auto subStr = idStr.substr(i * 2, 2);
            devId.dsound[i] = std::stoul(subStr, nullptr, 16);
        }
    }
    else if (backendStr == "winmm")
    {
        devId.winmm = std::stoul(idStr, nullptr, 16);
    }
    else if (backendStr == "coreaudio")
    {
        strcpy(devId.coreaudio, idStr.c_str());
    }
    else if (backendStr == "alsa")
    {
        strcpy(devId.alsa, idStr.c_str());
    }
    else if (backendStr == "pulseaudio")
    {
        strcpy(devId.pulse, idStr.c_str());
    }
    else if (backendStr == "jack")
    {
        devId.jack = std::stoul(idStr, nullptr, 16);
    }

    return devId;
}

END_NAMESPACE_AUDIO_DEVICE_MODULE
