#include <audio_device_module/audio_channel_impl.h>
#include <opendaq/signal_factory.h>
#include <opendaq/packet_factory.h>
#include <opendaq/range_factory.h>

BEGIN_NAMESPACE_AUDIO_DEVICE_MODULE

AudioChannelImpl::AudioChannelImpl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId)
    : ChannelImpl(FunctionBlockType("AudioChannel", "Audio", ""), ctx, parent, localId)
    , samplesGenerated(0)
{
    loggerComponent = context.getLogger().getOrAddComponent("AudioDeviceModule");
    objPtr.asPtr<IPropertyObjectInternal>().setLockingStrategy(LockingStrategy::InheritLock);
    initSignals();
}

AudioChannelImpl::~AudioChannelImpl() = default;

// Always called under lock
void AudioChannelImpl::configure(const ma_device& device)
{
    this->name = device.capture.name;
    outputSignal.setName(device.capture.name);

    auto dataDescriptor = DataDescriptorBuilder()
                              .setSampleType(SampleType::Int64)
                              .setTickResolution(Ratio(1, device.sampleRate))
                              .setRule(LinearDataRule(1, 0))
                              .setUnit(Unit("s", -1, "seconds", "time"))
                              .build();

    timeSignal.setDescriptor(dataDescriptor);
}

// Always called under lock
void AudioChannelImpl::reset()
{
    samplesGenerated = 0;
}

void AudioChannelImpl::generatePackets(const void* data, size_t sampleCount)
{
    try
    {
        auto lock = getAcquisitionLock2();
        auto domainPacket = DataPacket(timeSignal.getDescriptor(), sampleCount, samplesGenerated);
        auto dataPacket = DataPacketWithDomain(domainPacket, outputSignal.getDescriptor(), sampleCount);

        auto packetData = dataPacket.getRawData();
        std::memcpy(packetData, data, sampleCount * sizeof(float));

        timeSignal.sendPacket(domainPacket);
        outputSignal.sendPacket(dataPacket);

        samplesGenerated += sampleCount;
    }
    catch (const std::exception& e)
    {
        LOG_W("Miniaudio device failed to generate packets: {}", e.what());
        throw;
    }
}

uint64_t AudioChannelImpl::getSamplesGenerated()
{
    auto lock = getAcquisitionLock2();
    return samplesGenerated;
}

void AudioChannelImpl::initSignals()
{
    auto dataDescriptor =
        DataDescriptorBuilder().setSampleType(SampleType::Float32).setValueRange(Range(-1.0, 1.0)).build();

    outputSignal = createAndAddSignal("AudioSignal", dataDescriptor);
    timeSignal = createAndAddSignal("AudioTimeSignal", nullptr, false);
    outputSignal.setDomainSignal(timeSignal);
}

END_NAMESPACE_AUDIO_DEVICE_MODULE
