#include <audio_device_module/wav_writer_fb_impl.h>
#include <opendaq/event_packet_ids.h>
#include <opendaq/event_packet_ptr.h>
#include <opendaq/data_descriptor_ptr.h>
#include <opendaq/event_packet_params.h>
#include <opendaq/sample_type_traits.h>
#include <opendaq/custom_log.h>
#include <opendaq/reader_factory.h>
#include <opendaq/function_block_type_factory.h>

BEGIN_NAMESPACE_AUDIO_DEVICE_MODULE

WAVWriterFbImpl::WAVWriterFbImpl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId)
    : FunctionBlock(CreateType(), ctx, parent, localId)
    , storing(false)   
    , selfChange(false)   
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

WAVWriterFbImpl::~WAVWriterFbImpl()
{
    stopStore();
}

bool WAVWriterFbImpl::validateDataDescriptor() const
{
    const auto inputSampleType = inputValueDataDescriptor.getSampleType();
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
    const auto inputDeltaTicks = domainRuleParams.get("Delta");
    const auto tickResolution = inputTimeDataDescriptor.getTickResolution();

    const uint32_t sampleRate = static_cast<uint32_t>(static_cast<double>(inputDeltaTicks) / static_cast<double>(tickResolution));

    const ma_encoder_config config = ma_encoder_config_init(ma_encoding_format_wav, ma_format_f32, 1, sampleRate);
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

    objPtr.addProperty(BoolProperty("Storing", false));
    objPtr.getOnPropertyValueWrite("Storing") += [this](PropertyObjectPtr& /*obj*/, PropertyValueEventArgsPtr& args)
    {
        if (selfChange)
            storingChangedNoLock(args.getValue());
        else
            storingChanged(args.getValue());
    };

    fileNameChanged();
}

void WAVWriterFbImpl::fileNameChanged()
{
    std::scoped_lock lock(sync);
    fileName = static_cast<std::string>(objPtr.getPropertyValue("FileName"));
    LOG_I("File name: {}", fileName)
    stopStoreInternal();
}

void WAVWriterFbImpl::storingChanged(bool store)
{
    std::scoped_lock lock(sync);
    storingChangedNoLock(store);
}

void WAVWriterFbImpl::storingChangedNoLock(bool store)
{
    if (store)
        startStore();
    else
        stopStore();
}

void WAVWriterFbImpl::createInputPort()
{
    inputPort = createAndAddInputPort("Input", PacketReadyNotification::Scheduler);
    reader = StreamReaderFromPort(inputPort, SampleType::Float32, SampleType::UInt64);
    reader.setOnDataAvailable([this] { calculate();});
}

void WAVWriterFbImpl::startStore()
{
    assert(!storing);

    if (!inputValueDataDescriptor.assigned() && !inputTimeDataDescriptor.assigned())
    {
        LOG_W("Incomplete input signal descriptors")
        return;
    }

    if (!validateDataDescriptor() || !validateDomainDescriptor())
        return;

    storing = initializeEncoder();
    if (storing)
    {
        LOG_I("Stroring started")
    }
}

void WAVWriterFbImpl::stopStore()
{
    if (!storing)
        return;

    ma_encoder_uninit(&encoder);
    storing = false;
    LOG_I("Storing stopped")
}

void WAVWriterFbImpl::stopStoreInternal()
{
    selfChange = true;
    objPtr.setPropertyValue("Storing", false);
    selfChange = false;
}

void WAVWriterFbImpl::processEventPacket(const EventPacketPtr& packet)
{
    if (packet.getEventId() == event_packet_id::DATA_DESCRIPTOR_CHANGED)
    {
        stopStoreInternal();
        const auto params = packet.getParameters();

        const DataDescriptorPtr valueSignalDescriptor = packet.getParameters().get(event_packet_param::DATA_DESCRIPTOR);
        const DataDescriptorPtr domainSignalDescriptor = packet.getParameters().get(event_packet_param::DOMAIN_DATA_DESCRIPTOR);

        if (valueSignalDescriptor.assigned())
            inputValueDataDescriptor = valueSignalDescriptor;
        if (domainSignalDescriptor.assigned())
            inputTimeDataDescriptor = domainSignalDescriptor;
    }
}

void WAVWriterFbImpl::calculate()
{
    std::scoped_lock lock(sync);
    SizeT availableData = reader.getAvailableCount();

    std::vector<float> inputData;
    inputData.reserve(std::max(availableData, static_cast<SizeT>(1)));

    const auto status = reader.read(inputData.data(), &availableData);

    if (storing)
    {
        ma_uint64 samplesWritten;
        ma_result result;
        if ((result = ma_encoder_write_pcm_frames(&encoder, inputData.data(), availableData, &samplesWritten)) != MA_SUCCESS)
        {
            LOG_W("Miniaudio failure: {}", ma_result_description(result));
        }
    }

    if (status.getReadStatus() == ReadStatus::Event)
    {
        processEventPacket(status.getEventPacket());
    }
}

END_NAMESPACE_AUDIO_DEVICE_MODULE
