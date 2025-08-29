#include <audio_device_module/wav_writer_fb_impl.h>
#include <opendaq/event_packet_utils.h>
#include <opendaq/reader_factory.h>
#include <cstdint>

BEGIN_NAMESPACE_AUDIO_DEVICE_MODULE

WAVWriterFbImpl::WAVWriterFbImpl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId)
    : FunctionBlockImpl<IFunctionBlock, IRecorder>(CreateType(), ctx, parent, localId, nullptr)
    , recording(false)   
{
    createInputPort();
    initProperties();
}

FunctionBlockTypePtr WAVWriterFbImpl::CreateType()
{
    return FunctionBlockType(
        "AudioDeviceModuleWavWriter",
        "WAVWriter",
        "Writes input signals to WAV files"
    );
}

ErrCode WAVWriterFbImpl::startRecording()
{
    auto lock = this->getRecursiveConfigLock2();

    assert(!recording);

    if (!inputValueDataDescriptor.assigned() || !inputTimeDataDescriptor.assigned())
    {
        LOG_W("Incomplete input signal descriptors")
        return OPENDAQ_FAILED(OPENDAQ_ERR_VALIDATE_FAILED);
    }

    if (!validateDataDescriptor() || !validateDomainDescriptor())
        return OPENDAQ_FAILED(OPENDAQ_ERR_VALIDATE_FAILED);

    recording = initializeEncoder();
    if (recording)
    {
        LOG_I("Recording started")
    }

    return OPENDAQ_SUCCESS;
}

ErrCode WAVWriterFbImpl::stopRecording()
{
    auto lock = this->getRecursiveConfigLock2();

    if (!stopRecordingInternal())
        return OPENDAQ_FAILED(OPENDAQ_ERR_INVALIDSTATE);

    return OPENDAQ_SUCCESS;
}

ErrCode WAVWriterFbImpl::getIsRecording(Bool* isRecording)
{
    auto lock = this->getRecursiveConfigLock2();

    if (!isRecording)
    {
        return OPENDAQ_FAILED(OPENDAQ_ERR_INVALIDPARAMETER);
    }

    *isRecording = recording;

    return OPENDAQ_SUCCESS;
}

bool WAVWriterFbImpl::stopRecordingInternal()
{
    if(!recording)
        return false;

    ma_encoder_uninit(&encoder);
    recording = false;
    LOG_I("Recording stopped")

    return true;
}

WAVWriterFbImpl::~WAVWriterFbImpl()
{
    stopRecording();
}

bool WAVWriterFbImpl::validateDataDescriptor() const
{
    const auto inputSampleType = inputValueDataDescriptor.getSampleType();
    if (inputSampleType == SampleType::Int16)
    {
        return true;
    }

    if (inputSampleType != SampleType::Float64 && inputSampleType != SampleType::Float32)
    {
        LOG_W("Value data descriptor sample type must be Float64 or Float32, but is {}", convertSampleTypeToString(inputSampleType))
        return false;
    }

    return true;
}

bool WAVWriterFbImpl::validateDomainDescriptor() const
{
    const auto inputSampleType = inputTimeDataDescriptor.getSampleType();
    if (inputSampleType != SampleType::Int64 && inputSampleType != SampleType::UInt64)
    {
        LOG_W("Time data descriptor sample type must be Int64 or UInt64, but is {}", convertSampleTypeToString(inputSampleType))
        return false;
    }

    if (inputTimeDataDescriptor.getUnit().getSymbol() != "s")
    {
        LOG_W("Time data descriptor unit symbol must be \"s\", but is {}", inputTimeDataDescriptor.getUnit().getSymbol())
        return false;
    }

    if (inputTimeDataDescriptor.getRule().getType() != DataRuleType::Linear)
    {
        LOG_W("Time data rule type is not Linear")
        return false;
    }

    return true;
}

bool WAVWriterFbImpl::initializeEncoder()
{
    const auto domainRuleParams = inputTimeDataDescriptor.getRule().getParameters();
    const auto sampleType = inputValueDataDescriptor.getSampleType();
    const auto inputDeltaTicks = domainRuleParams.get("delta");
    const auto tickResolution = inputTimeDataDescriptor.getTickResolution();
    ma_format maSampleType = ma_format_f32;
    switch (sampleType)
    {
        case SampleType::Int16:
            maSampleType = ma_format_s16;
            break;
        case SampleType::Int32:
            maSampleType = ma_format_s32;
            break;
        case SampleType::Float32:
            maSampleType = ma_format_f32;
            break;
        case SampleType::Float64:
            maSampleType = ma_format_f32;
            LOG_W("Float64 not supported by miniaudio, truncating to Float32");
            break;
        default:
            maSampleType = ma_format_unknown;
            LOG_W("Unsupported miniaudio sample type");
            return false;
    }
    const uint32_t sampleRate = static_cast<uint32_t>(static_cast<double>(inputDeltaTicks) / static_cast<double>(tickResolution));

    const ma_encoder_config config = ma_encoder_config_init(ma_encoding_format_wav, maSampleType, 1, sampleRate);
    const ma_result result = ma_encoder_init_file(fileName.c_str(), &config, &encoder);

    if (result != MA_SUCCESS)
    {
        LOG_W("Miniaudio encoder init file {} failed: {}", fileName, ma_result_description(result))
        return false;
    }

    return true;
}

void WAVWriterFbImpl::initProperties()
{
    objPtr.addProperty(StringProperty("FileName", "test.wav"));
    objPtr.getOnPropertyValueWrite("FileName") += [this](PropertyObjectPtr& /*obj*/, PropertyValueEventArgsPtr& /*args*/) { fileNameChanged(); }; 

    fileNameChanged();
}

void WAVWriterFbImpl::fileNameChanged()
{
    stopRecording();
    fileName = static_cast<std::string>(objPtr.getPropertyValue("FileName"));
    LOG_I("File name: {}", fileName)
}

void WAVWriterFbImpl::createInputPort()
{
    inputPort = createAndAddInputPort("Input", PacketReadyNotification::Scheduler);
    reader = StreamReaderFromPort(inputPort, SampleType::Undefined, SampleType::Undefined);
    reader.setOnDataAvailable([this] { processInputData();});
}

void WAVWriterFbImpl::processEventPacket(const EventPacketPtr& packet)
{
    const auto [valueDescriptorChanged, domainDescriptorChanged, valueSignalDescriptor, domainSignalDescriptor] =
        parseDataDescriptorEventPacket(packet);

    if (valueDescriptorChanged)
        inputValueDataDescriptor = valueSignalDescriptor;
    if (domainDescriptorChanged)
        inputTimeDataDescriptor = domainSignalDescriptor;

    if (valueDescriptorChanged || domainDescriptorChanged)
    {
        if (inputValueDataDescriptor.getSampleType() == SampleType::Float64)
        {
            reader = StreamReaderFromExisting(reader, SampleType::Float32, inputTimeDataDescriptor.getSampleType());
        }
        else
        {
            reader = StreamReaderFromExisting(reader, inputValueDataDescriptor.getSampleType(), inputTimeDataDescriptor.getSampleType());
        }
    }
}

void WAVWriterFbImpl::processInputData()
{
    auto lock = this->getAcquisitionLock2();
    SizeT availableData = reader.getAvailableCount();
    size_t dataSize = 1;
    std::vector<unsigned char> inputData;

    switch (reader.getValueReadType())
    {
        case SampleType::Int16:
            dataSize = sizeof(short);
            break;
        case SampleType::Int32:
            dataSize = sizeof(int32_t);
            break;
        case SampleType::Float32:
            dataSize = sizeof(float_t);
            break;
        case SampleType::Float64:
            dataSize = sizeof(float_t);
            break;
        default:
            dataSize = 0;
            break;
    }

    inputData.reserve(std::max(availableData, static_cast<SizeT>(1)) * dataSize);
    const auto status = reader.read(inputData.data(), &availableData);

    if (status.getReadStatus() == ReadStatus::Event)
    {
        stopRecordingInternal();

        processEventPacket(status.getEventPacket());
    }

    if (recording)
    {
        ma_uint64 samplesWritten;
        ma_result result;
        if ((result = ma_encoder_write_pcm_frames(&encoder, inputData.data(), availableData, &samplesWritten)) != MA_SUCCESS)
        {
            LOG_W("Miniaudio failure: {}", ma_result_description(result));
        }
    }
}

END_NAMESPACE_AUDIO_DEVICE_MODULE
