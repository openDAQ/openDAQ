#include <audio_device_module/wav_writer_fb_impl.h>
#include <opendaq/event_packet_ids.h>
#include <opendaq/event_packet_ptr.h>
#include <opendaq/data_descriptor_ptr.h>
#include <opendaq/event_packet_params.h>
#include <opendaq/sample_type_traits.h>
#include <opendaq/custom_log.h>

BEGIN_NAMESPACE_AUDIO_DEVICE_MODULE

WAVWriterFbImpl::WAVWriterFbImpl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId)
    : FunctionBlock(CreateType(), ctx, parent, localId)
    , storing(false)
{
    initProperties();
    createInputPort();
}

FunctionBlockTypePtr WAVWriterFbImpl::CreateType()
{
    return FunctionBlockType(
        "audio_device_module_wav_writer",
        "WAVWriter",
        "Writes WAV files"
    );
}

WAVWriterFbImpl::~WAVWriterFbImpl()
{
    stopStore();
}

void WAVWriterFbImpl::initProperties()
{
    const auto fileNamePropInfo = StringProperty("FileName", "test.wav");
    
    objPtr.addProperty(fileNamePropInfo);
    objPtr.getOnPropertyValueWrite("FileName") += [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChanged(); }; 

    readProperties();
}

void WAVWriterFbImpl::propertyChanged()
{
    std::scoped_lock lock(sync);

    stopStore();
    readProperties();
    startStore();
}


void WAVWriterFbImpl::readProperties()
{
    fileName = static_cast<std::string>(objPtr.getPropertyValue("FileName"));
    LOG_I("Properties: FileName {}", fileName);
}

void WAVWriterFbImpl::createInputPort()
{
    inputPort = createAndAddInputPort("Input", PacketReadyNotification::Scheduler);
}

void WAVWriterFbImpl::startStore()
{
    assert(!storing);

    if (!inputValueDataDescriptor.assigned() && !inputTimeDataDescriptor.assigned())
    {
        LOG_W("Incomplete input signal descriptors")
        return;
    }

    if (inputValueDataDescriptor.isStructDescriptor())
    {
        LOG_W("Incompatible input value data descriptor")
        return;
    }

    if (inputValueDataDescriptor.getSampleType() != SampleType::Float32)
    {
        LOG_W("Incompatible value sample type {}", convertSampleTypeToString(inputValueDataDescriptor.getSampleType()));
        return;
    }

    if (inputTimeDataDescriptor.isStructDescriptor())
    {
        LOG_W("Incompatible input domain data descriptor")
        return;
    }

    if (inputTimeDataDescriptor.getSampleType() != SampleType::Int64)
    {
        LOG_W("Incompatible domain data sample type {}", convertSampleTypeToString(inputTimeDataDescriptor.getSampleType()));
        return;
    }
    if (inputTimeDataDescriptor.getUnit().getSymbol() != "s")
    {
        LOG_W("Incompatible domain data unit {}", inputTimeDataDescriptor.getUnit().getSymbol());
        return;
    }

    const auto domainRule = inputTimeDataDescriptor.getRule();
    if (domainRule.getType() != DataRuleType::Linear)
    {
        LOG_W("Domain data rule type is not Linear");
        return;
    }
    const auto domainRuleParams = domainRule.getParameters();
    const auto inputDeltaTicks = domainRuleParams.get("delta");

    auto domainRes = inputTimeDataDescriptor.getTickResolution();

    uint32_t sampleRate = static_cast<uint32_t>(static_cast<double>(inputDeltaTicks) / static_cast<double>(domainRes));

    ma_encoder_config config = ma_encoder_config_init(ma_encoding_format_wav, ma_format_f32, 1, sampleRate);
    ma_result result = ma_encoder_init_file(fileName.c_str(), &config, &encoder);
    if (result != MA_SUCCESS)
    {
        LOG_W("Miniaudio encoder init file {} failed: {}", fileName, ma_result_description(result));
        return;
    }

    storing = true;
}

void WAVWriterFbImpl::stopStore()
{
    if (!storing)
        return;

    ma_encoder_uninit(&encoder);

    storing = false;
}

void WAVWriterFbImpl::processSignalDescriptorChanged(const DataDescriptorPtr& valueDataDescriptor, const DataDescriptorPtr& timeDataDescriptor)
{
    stopStore();

    if (valueDataDescriptor.assigned())
        inputValueDataDescriptor = valueDataDescriptor;
    if (timeDataDescriptor.assigned())
        inputTimeDataDescriptor = timeDataDescriptor;

    startStore();
}

void WAVWriterFbImpl::processDataPacket(const DataPacketPtr& packet)
{
    if (!storing)
        return;

    const auto data = packet.getData();
    const auto sampleCount = packet.getSampleCount();
    ma_uint64 samplesWritten;
    ma_result result;
    if ((result = ma_encoder_write_pcm_frames(&encoder, data, sampleCount, &samplesWritten)) != MA_SUCCESS)
    {
        LOG_W("Miniaudio failure: {}", ma_result_description(result));
    }
}

void WAVWriterFbImpl::onPacketReceived(const InputPortPtr& port)
{
    processPackets();
}

void WAVWriterFbImpl::processPackets()
{
    std::scoped_lock lock(sync);

    const auto conn = inputPort.getConnection();
    if (!conn.assigned())
        return;

    auto packet = conn.dequeue();
    while (packet.assigned())
    {
        const auto packetType = packet.getType();
        if (packetType == PacketType::Event)
        {
            auto eventPacket = packet.asPtr<IEventPacket>(true);
            LOG_T("Processing {} event", eventPacket.getEventId())
            if (eventPacket.getEventId() == event_packet_id::DATA_DESCRIPTOR_CHANGED)
            {
                DataDescriptorPtr valueSignalDescriptor = eventPacket.getParameters().get(event_packet_param::DATA_DESCRIPTOR);
                DataDescriptorPtr domainSignalDescriptor = eventPacket.getParameters().get(event_packet_param::DOMAIN_DATA_DESCRIPTOR);
                processSignalDescriptorChanged(valueSignalDescriptor, domainSignalDescriptor);
            }
        }
        else if (packetType == PacketType::Data)
        {
            auto dataPacket = packet.asPtr<IDataPacket>();
            processDataPacket(dataPacket);
        }

        packet = conn.dequeue();
    }
}

END_NAMESPACE_AUDIO_DEVICE_MODULE
