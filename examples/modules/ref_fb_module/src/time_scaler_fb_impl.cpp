#include <ref_fb_module/time_scaler_fb_impl.h>
#include <opendaq/event_packet_params.h>
#include <opendaq/packet_factory.h>
#include <opendaq/reusable_data_packet_ptr.h>

BEGIN_NAMESPACE_REF_FB_MODULE

namespace TimeScaler
{

TimeScalerFbImpl::TimeScalerFbImpl(const ContextPtr& ctx, 
                                   const ComponentPtr& parent, 
                                   const StringPtr& localId,
                                   const PropertyObjectPtr& config)
    : FunctionBlock(CreateType(), ctx, parent, localId)
{
    initProperties();
    applyConfig(config);
    initInputPorts();
    initSignals();
}

FunctionBlockTypePtr TimeScalerFbImpl::CreateType()
{
    auto config = PropertyObject();
    const auto timeOffsetProperty = IntPropertyBuilder("TimeOffset", 0).setUnit(Unit("s", -1, "seconds", "time"))
                                                                                          .build();
    config.addProperty(timeOffsetProperty);

    return FunctionBlockType("RefFBTimeScaler",
                             "Time Scaler",
                             "Adds offset to time",
                             config
    );
}

void TimeScalerFbImpl::initProperties()
{
    const auto timeOffsetProperty = IntPropertyBuilder("TimeOffset", 0).setUnit(Unit("s", -1, "seconds", "time"))
                                                                                          .build();

    objPtr.addProperty(timeOffsetProperty);
    objPtr.getOnPropertyValueWrite("TimeOffset") += [this](PropertyObjectPtr& /*obj*/, PropertyValueEventArgsPtr& args) 
    { 
        updateTimeOffset(args.getValue());
    };
}

void TimeScalerFbImpl::applyConfig(const PropertyObjectPtr& config)
{
    objPtr.setPropertyValue("TimeOffset", config.getPropertyValue("TimeOffset"));
}

void TimeScalerFbImpl::updateTimeOffset(Int timeOffset)
{
    this->timeOffset = timeOffset;
}

void TimeScalerFbImpl::initInputPorts()
{
    inputPort = createAndAddInputPort("Input", PacketReadyNotification::SameThread);
}

void TimeScalerFbImpl::initSignals()
{
    dataSignal = createAndAddSignal("Data");
    domainSignal = createAndAddSignal("Time", nullptr, false);
    dataSignal.setDomainSignal(domainSignal);
}

void TimeScalerFbImpl::onPacketReceived(const InputPortPtr& port)
{
    auto lock = this->getAcquisitionLock();
    const auto packet = port.getConnection().dequeue();
    if (packet.assigned())
    {
        switch (packet.getType())
        {
            case PacketType::None:
                LOG_W("Packet type None");
                break;
            case PacketType::Data:
                handleDataPacket(packet.asPtr<IDataPacket>(true));
                break;
            case PacketType::Event:
                handleEventPacket(packet.asPtr<IEventPacket>(true));
                break;
        }
    }
}

void TimeScalerFbImpl::handleDataPacket(const DataPacketPtr& inputPacket)
{
    const auto inputDomainPacket = inputPacket.getDomainPacket();
    const auto offsetToAdd = timeOffset * domainTickResolution.getDenominator() / domainTickResolution.getNumerator();

    DataPacketPtr outputDomainPacket;
    if (domainRuleType == DataRuleType::Linear)
    {
        auto packetOffset = inputDomainPacket.getOffset();
        outputDomainPacket = DataPacket(inputDomainPacket.getDataDescriptor(), inputDomainPacket.getSampleCount(), packetOffset + offsetToAdd);
    }
    else if (domainRuleType == DataRuleType::Explicit)
    {
        // For now we should not enter this scope. But in future we can support Explicit time rule
    }

    DataPacketPtr outputDataPacket;
    if (dataRuleType == DataRuleType::Linear)
    {
        outputDataPacket = DataPacketWithDomain(outputDomainPacket, dataDescriptor, inputPacket.getSampleCount(), inputPacket.getOffset());
    }
    else if (dataRuleType == DataRuleType::Explicit)
    {
        outputDataPacket = DataPacketWithDomain(outputDomainPacket, dataDescriptor, inputPacket.getSampleCount());
        void* src = inputPacket.getRawData();
        void* dst = outputDataPacket.getRawData();
        memcpy(dst, src, inputPacket.getRawDataSize());
    } 

    dataSignal.sendPacket(outputDataPacket);
    domainSignal.sendPacket(outputDomainPacket);
}

void TimeScalerFbImpl::handleEventPacket(const EventPacketPtr& packet)
{
    if (packet.getEventId() == event_packet_id::DATA_DESCRIPTOR_CHANGED)
    {
        DataDescriptorPtr inputDataDescriptor = packet.getParameters().get(event_packet_param::DATA_DESCRIPTOR);
        if (inputDataDescriptor.assigned())
        {
            dataDescriptor = inputDataDescriptor;
            dataSignal.setDescriptor(inputDataDescriptor);

            const auto dataRule = inputDataDescriptor.getRule();
            if (!dataRule.assigned())
            {
                inputPort.setActive(false);
                DAQ_THROW_EXCEPTION(ArgumentNullException, "Data rule is not assigned");
            }

            dataRuleType = dataRule.getType();
            if (dataRuleType != DataRuleType::Linear && dataRuleType != DataRuleType::Explicit)
            {
                inputPort.setActive(false);
                DAQ_THROW_EXCEPTION(InvalidParametersException, "Data rule must be Linear or Explicit");
            }
        }
        DataDescriptorPtr inputDomainDataDescriptor = packet.getParameters().get(event_packet_param::DOMAIN_DATA_DESCRIPTOR);
        if (inputDomainDataDescriptor.assigned())
        {
            domainDescriptor = inputDomainDataDescriptor;
            domainSignal.setDescriptor(inputDomainDataDescriptor);

            const auto domainUnit = inputDomainDataDescriptor.getUnit();
            if (!domainUnit.assigned())
            {
                inputPort.setActive(false);
                DAQ_THROW_EXCEPTION(ArgumentNullException, "Domain unit is not assigned");
            }

            if (domainUnit.getSymbol() != "s" && domainUnit.getSymbol() != "seconds")
            {
                inputPort.setActive(false);
                DAQ_THROW_EXCEPTION(InvalidParametersException, "Domain unit expected in seconds");
            }

            if (inputDomainDataDescriptor.getSampleType() != SampleType::Int64 && inputDomainDataDescriptor.getSampleType() != SampleType::UInt64)
            {
                inputPort.setActive(false);
                DAQ_THROW_EXCEPTION(InvalidParametersException, "Incompatible domain data sample type");
            }

            const auto domainRule = inputDomainDataDescriptor.getRule();
            if (!domainRule.assigned())
            {
                inputPort.setActive(false);
                DAQ_THROW_EXCEPTION(ArgumentNullException, "Domain rule is not assigned");
            }

            domainRuleType = domainRule.getType();
            if (domainRuleType != DataRuleType::Linear)
            {
                inputPort.setActive(false);
                DAQ_THROW_EXCEPTION(InvalidParametersException, "Domain rule must be Linear");
            }

            domainTickResolution = inputDomainDataDescriptor.getTickResolution();
            if (!domainTickResolution.assigned())
            {
                inputPort.setActive(false);
                DAQ_THROW_EXCEPTION(ArgumentNullException, "Domain tick resolition is not assigned");
            }
        }

        inputPort.setActive(true);
    } 
}

} // namespace TimeScaler;
END_NAMESPACE_REF_FB_MODULE
