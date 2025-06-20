#include <audio_device_module/wav_reader_fb_impl.h>
#include <opendaq/packet_factory.h>

BEGIN_NAMESPACE_AUDIO_DEVICE_MODULE

static void miniaudioDataCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
    auto this_ = static_cast<WAVReaderFbImpl*>(pDevice->pUserData);
    ma_decoder_read_pcm_frames(&(this_->decoder), pOutput, frameCount, NULL);

    this_->sendPacket(this_->buildPacket(pOutput, frameCount));
    
    (void) pInput;
}

DataPacketPtr WAVReaderFbImpl::buildPacket(const void* data, size_t sampleCount)
{
    try
    {
        auto domainPacket = DataPacket(domainDescriptor, sampleCount, samplesCaptured);
        samplesCaptured += sampleCount;
        auto dataPacket = DataPacketWithDomain(domainPacket, dataDescriptor, sampleCount);

        void* packetData = dataPacket.getRawData();
        std::memcpy(packetData, data, sampleCount * dataDescriptor.getSampleSize());

        ma_uint64 frames;
        ma_decoder_get_available_frames(&decoder, &frames);

        if (frames == 0 && framesAvailable == true)
        {
            setComponentStatusWithMessage(ComponentStatus::Warning, "End of file reached.");
            objPtr.setPropertyValue("EOF", true);
            framesAvailable = false;
        }

        return dataPacket;
    }
    catch (const std::exception& e)
    {
        LOG_E("Building packet failed: {}", e.what());
        throw;
    }
}

void WAVReaderFbImpl::sendPacket(DataPacketPtr packet)
{
    outputSignal.sendPacket(packet);
}

WAVReaderFbImpl::WAVReaderFbImpl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId)
    : FunctionBlock(CreateType(), ctx, parent, localId)
    , filePath("")
    , reading(false)
    , framesAvailable(false)
    , logger(ctx.getLogger())
    , loggerComponent(this->logger.assigned() ? this->logger.getOrAddComponent("AudioDeviceModule")
                                              : throw ArgumentNullException("Logger must not be null"))
{
    timeSignal = createAndAddSignal("Time", nullptr, false);
    outputSignal = createAndAddSignal("Audio");
    outputSignal.setDomainSignal(timeSignal);

    initComponentStatus();
    initProperties();
}

WAVReaderFbImpl::~WAVReaderFbImpl()
{
    uninitializeDecoder();
}

FunctionBlockTypePtr WAVReaderFbImpl::CreateType()
{
    return FunctionBlockType(
        "AudioDeviceModuleWavReader",
        "WAVReader",
        "Readers and creates input signals from WAV files");
}

bool WAVReaderFbImpl::initializeDecoder()
{
    ma_result result;
    ma_device_config deviceConfig;

    result = ma_decoder_init_file(filePath.c_str(), NULL, &decoder);
    if (result != MA_SUCCESS)
    {
        setComponentStatusWithMessage(ComponentStatus::Error, "Could not load file!");
        return false;
    }

    deviceConfig = ma_device_config_init(ma_device_type_playback);
    deviceConfig.playback.format = decoder.outputFormat;
    deviceConfig.playback.channels = decoder.outputChannels;
    deviceConfig.sampleRate = decoder.outputSampleRate;
    deviceConfig.dataCallback = miniaudioDataCallback;
    deviceConfig.pUserData = this;
    
    if (ma_device_init(NULL, &deviceConfig, &device) != MA_SUCCESS)
    {
        setComponentStatusWithMessage(ComponentStatus::Error, "Failed to initialize miniaudio playback device.");
        ma_decoder_uninit(&decoder);
        return false;
    }

    if (!initializeSignal())
    {
        return false;
    }

    samplesCaptured = 0;
    framesAvailable = true;
    objPtr.setPropertyValue("EOF", false);
    setComponentStatusWithMessage(ComponentStatus::Ok, "File ready.");

    return true;
}

bool WAVReaderFbImpl::uninitializeDecoder()
{
    ma_device_uninit(&device);
    ma_decoder_uninit(&decoder);

    return true;
}

bool WAVReaderFbImpl::decoderReady()
{
    return ma_device_get_state(&device) == ma_device_state::ma_device_state_stopped;
}

bool WAVReaderFbImpl::decoderReading()
{
    return ma_device_get_state(&device) == ma_device_state::ma_device_state_started;
}

void WAVReaderFbImpl::startRead()
{
    if (ma_device_start(&device) != MA_SUCCESS)
    {
        setComponentStatusWithMessage(ComponentStatus::Error, "Failed to start playback device.\n");
        ma_device_uninit(&device);
        ma_decoder_uninit(&decoder);
    }
}

void WAVReaderFbImpl::stopRead()
{
    if (ma_device_stop(&device) != MA_SUCCESS)
    {
        setComponentStatusWithMessage(ComponentStatus::Error, "Failed to stop playback device.\n");
        ma_device_uninit(&device);
        ma_decoder_uninit(&decoder);
    }
}

bool WAVReaderFbImpl::updateFilePath(const std::string& newPath)
{
    if (newPath == filePath || newPath == "")
    {
        return true;
    }

    filePath = newPath;

    if (initializeDecoder())
    {
        return true;
    }
    else
    {
        filePath = "";
        return false;
    }
}

void WAVReaderFbImpl::setRead(bool read)
{
    if (reading == read)
    {
        return;
    }
    if (read)
    {
        startRead();
    }
    else
    {
        stopRead();
    }
    reading = read;
}

bool WAVReaderFbImpl::initializeSignal()
{
    SampleType sampleFormat;
    switch (decoder.outputFormat)
    {
        case ma_format::ma_format_u8:
            sampleFormat = SampleType::UInt8;
            break;
        case ma_format::ma_format_s16:
            sampleFormat = SampleType::Int16;
            break;
        case ma_format::ma_format_s24:
            setComponentStatusWithMessage(ComponentStatus::Error, "Unsupported sample type Int24");
            return false;
        case ma_format::ma_format_s32:
            sampleFormat = SampleType::Int32;
            break;
        case ma_format::ma_format_f32:
            sampleFormat = SampleType::Float32;
            break;
        default:
            setComponentStatusWithMessage(ComponentStatus::Error, "Unknown audio sample type");
            return false;
    }

    domainDescriptor = DataDescriptorBuilder()
                           .setSampleType(SampleType::Int64)
                           .setTickResolution(Ratio(1, decoder.outputSampleRate))
                           .setRule(LinearDataRule(1, 0))
                           .setUnit(Unit("s", -1, "second", "time"))
                           .setName("Time")
                           .build();

    dataDescriptor = DataDescriptorBuilder()
                        .setSampleType(sampleFormat)
                         .setValueRange(Range(std::numeric_limits<short>::min(), std::numeric_limits<short>::max()))
                        .setName("Audio")
                        .build();

    timeSignal.setDescriptor(domainDescriptor);
    outputSignal.setDescriptor(dataDescriptor);

    return true;
}

void WAVReaderFbImpl::initProperties()
{
    objPtr.addProperty(StringProperty("FilePath", ""));
    objPtr.getOnPropertyValueWrite("FilePath") += [this](PropertyObjectPtr& /*obj*/, PropertyValueEventArgsPtr& args)
        {
            setRead(false);
            if (!updateFilePath(static_cast<std::string>(args.getValue())))
            {
                args.setValue("");
                setComponentStatusWithMessage(ComponentStatus::Error, "Invalid file path!");
            }
        };

    objPtr.addProperty(BoolProperty("Reading", false));
    objPtr.getOnPropertyValueWrite("Reading") += [this](PropertyObjectPtr& /*obj*/, PropertyValueEventArgsPtr& args)
        {
            bool read = args.getValue();
            if (read)
            {
                if (decoderReading())
                {
                    return;
                }
                if (!decoderReady())
                {
                    args.setValue(false);
                    setComponentStatusWithMessage(ComponentStatus::Error, "Cannot set Reading to true, no valid file!");
                    return;
                }
            }
            setRead(read);
        };

    objPtr.addProperty(BoolProperty("EOF", false));
}


END_NAMESPACE_AUDIO_DEVICE_MODULE

