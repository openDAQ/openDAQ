#include <audio_device_module/audio_channel_impl.h>
#include <opendaq/signal_factory.h>
#include <opendaq/packet_factory.h>
#include <opendaq/range_factory.h>

BEGIN_NAMESPACE_AUDIO_DEVICE_MODULE

AudioChannelImpl::AudioChannelImpl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId)
    : ChannelImpl(FunctionBlockType("AudioChannel", "Audio", ""), ctx, parent, localId)
{
    outputSignal = createAndAddSignal("Audio");
}

AudioChannelImpl::~AudioChannelImpl() = default;

void AudioChannelImpl::configure(const ma_device& device, const SignalPtr& timeSignal)
{
    std::string channelName = device.capture.name;
    auto dataDescriptor =
        DataDescriptorBuilder().setSampleType(SampleType::Float32).setValueRange(Range(-1.0, 1.0)).setName(channelName).build();

    std::scoped_lock lock(sync);

    outputSignal.setDomainSignal(timeSignal);
    outputSignal.setDescriptor(dataDescriptor);
}

void AudioChannelImpl::addData(const DataPacketPtr& domainPacket, const void* data, size_t sampleCount)
{
    auto dataPacket = DataPacketWithDomain(domainPacket, outputSignal.getDescriptor(), sampleCount);

    auto packetData = dataPacket.getRawData();
    std::memcpy(packetData, data, sampleCount * sizeof(float));

    outputSignal.sendPacket(dataPacket);
}

END_NAMESPACE_AUDIO_DEVICE_MODULE
