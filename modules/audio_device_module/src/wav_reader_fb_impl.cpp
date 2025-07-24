#include <audio_device_module/wav_reader_fb_impl.h>
#include <opendaq/packet_factory.h>
#include <filesystem>

BEGIN_NAMESPACE_AUDIO_DEVICE_MODULE

WAVReaderFbImpl::WAVReaderFbImpl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId)
    : FunctionBlock(CreateType(), ctx, parent, localId)
    , filePath("")
    , decoderInitialized(false)
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
    setComponentStatusWithMessage(ComponentStatus::Warning, "No file path!");
}

WAVReaderFbImpl::~WAVReaderFbImpl()
{
    uninitializeDecoder();
}

void WAVReaderFbImpl::miniaudioDataCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
    auto this_ = static_cast<WAVReaderFbImpl*>(pDevice->pUserData);
    ma_decoder_read_pcm_frames(&(this_->decoder), pOutput, frameCount, NULL);

    this_->sendPacket(this_->buildPacket(pOutput, frameCount));
    int nVoids = frameCount * this_->dataDescriptor.getSampleSize();
    unsigned char* outputBytes = static_cast<unsigned char*>(pOutput);
    for (int i = 0; i < nVoids; i++)
    {
        outputBytes[i] = 0;
    }
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
            objPtr.asPtr<IPropertyObjectProtected>().setProtectedPropertyValue("EOF", true);
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
        setComponentStatusWithMessage(ComponentStatus::Warning, "Could not load file!");
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
        setComponentStatusWithMessage(ComponentStatus::Warning, "Failed to initialize miniaudio playback device.");
        ma_decoder_uninit(&decoder);
        return false;
    }

    samplesCaptured = 0;
    framesAvailable = true;
    objPtr.asPtr<IPropertyObjectProtected>().setProtectedPropertyValue("EOF", false);

    decoderInitialized = true;

    return true;
}

bool WAVReaderFbImpl::reinitializeDecoder()
{
    uninitializeDecoder();
    return initializeDecoder();
}

void WAVReaderFbImpl::uninitializeDecoder()
{
    if (decoderInitialized)
    {
        ma_device_uninit(&device);
        ma_decoder_uninit(&decoder);
        decoderInitialized = false;
    }
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
        setComponentStatusWithMessage(ComponentStatus::Warning, "Failed to start playback device.\n");
        ma_device_uninit(&device);
        ma_decoder_uninit(&decoder);
    }
}

void WAVReaderFbImpl::stopRead()
{
    if (ma_device_stop(&device) != MA_SUCCESS)
    {
        setComponentStatusWithMessage(ComponentStatus::Warning, "Failed to stop playback device.\n");
        ma_device_uninit(&device);
        ma_decoder_uninit(&decoder);
    }
}

bool WAVReaderFbImpl::updateFilePath(const std::string& newPath)
{
    if (!std::filesystem::exists(newPath))
    {
        return false;
    }

    filePath = newPath;

    return true;
}

void WAVReaderFbImpl::setRead(bool read)
{
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
            setComponentStatusWithMessage(ComponentStatus::Warning, "Unsupported sample type Int24");
            return false;
        case ma_format::ma_format_s32:
            sampleFormat = SampleType::Int32;
            break;
        case ma_format::ma_format_f32:
            sampleFormat = SampleType::Float32;
            break;
        default:
            setComponentStatusWithMessage(ComponentStatus::Warning, "Unknown audio sample type");
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
            objPtr.setPropertyValue("Reading", false);
            std::string path = static_cast<std::string>(args.getValue());

            if (!updateFilePath(path))
            {
                setComponentStatusWithMessage(ComponentStatus::Warning, "Invalid file path!");
                return;
            }
            if (!reinitializeDecoder())
            {
                return;
            }
            if (!initializeSignal())
            {
                uninitializeDecoder();
                return;
            }
            setComponentStatusWithMessage(ComponentStatus::Ok, "File ready.");
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
                    return;
                }
            }
            setRead(read);
        };

    objPtr.addProperty(BoolPropertyBuilder("EOF", false).setReadOnly(true).build());
}


END_NAMESPACE_AUDIO_DEVICE_MODULE

