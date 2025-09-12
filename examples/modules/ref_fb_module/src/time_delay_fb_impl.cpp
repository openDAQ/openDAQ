#include <ref_fb_module/time_delay_fb_impl.h>
#include <opendaq/event_packet_params.h>
#include <opendaq/packet_factory.h>
#include <opendaq/reusable_data_packet_ptr.h>

BEGIN_NAMESPACE_REF_FB_MODULE

namespace TimeScaler
{

TimeDelayFbImpl::TimeDelayFbImpl(const ContextPtr& ctx, 
                                   const ComponentPtr& parent, 
                                   const StringPtr& localId,
                                   const PropertyObjectPtr& config)
    : FunctionBlock(CreateType(), ctx, parent, localId)
{
    initComponentStatus();
    initProperties();
    applyConfig(config);
    initInputPorts();
    initSignals();
}

FunctionBlockTypePtr TimeDelayFbImpl::CreateType()
{
    auto config = PropertyObject();
    const auto timeDelayProperty = IntPropertyBuilder("TimeDelay", 0).setUnit(Unit("s", -1, "seconds", "time"))
                                                                                          .build();
    config.addProperty(timeDelayProperty);

    return FunctionBlockType("RefFBModuleTimeDelay",
                             "Time Delay",
                             "Adds delay to the time signal both in positive and negative way",
                             config
    );
}

void TimeDelayFbImpl::initProperties()
{
    const auto timeDelayProperty = IntPropertyBuilder("TimeDelay", 0).setUnit(Unit("s", -1, "seconds", "time"))
                                                                                          .build();

    objPtr.addProperty(timeDelayProperty);
    objPtr.getOnPropertyValueWrite("TimeDelay") += [this](PropertyObjectPtr& /*obj*/, PropertyValueEventArgsPtr& args) 
    { 
        updateTimeDelay(args.getValue());
    };
}

void TimeDelayFbImpl::applyConfig(const PropertyObjectPtr& config)
{
    objPtr.setPropertyValue("TimeDelay", config.getPropertyValue("TimeDelay"));
}

void TimeDelayFbImpl::updateTimeDelay(Int timeDelay)
{
    this->timeDelay = timeDelay;
}

void TimeDelayFbImpl::initInputPorts()
{
    inputPort = createAndAddInputPort("Input", PacketReadyNotification::SameThread);
}

void TimeDelayFbImpl::initSignals()
{
    dataSignal = createAndAddSignal("Data");
    domainSignal = createAndAddSignal("Time", nullptr, false);
    dataSignal.setDomainSignal(domainSignal);
}

void TimeDelayFbImpl::onConnected(const InputPortPtr& port)
{
    auto lock = this->getRecursiveConfigLock();

    const auto dataSignal = port.getSignal();
    if (!dataSignal.assigned())
        return;

    const auto domainSignal = dataSignal.getDomainSignal();
    if (domainSignal.assigned())
    {
        setComponentStatus(ComponentStatus::Ok);
    }
    else
    {
        inputPort.setActive(false);
        setComponentStatusWithMessage(ComponentStatus::Error, "Expecting the signal with domain signal");
    }
}

void TimeDelayFbImpl::onPacketReceived(const InputPortPtr& port)
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

void TimeDelayFbImpl::handleDataPacket(const DataPacketPtr& inputPacket)
{
    const auto inputDomainPacket = inputPacket.getDomainPacket();

    if (timeDelay == 0)
    {
        domainSignal.sendPacket(inputDomainPacket);
        dataSignal.sendPacket(inputPacket);
        return;
    }

    const auto offsetToAdd = timeDelay * domainTickResolution.getDenominator() / domainTickResolution.getNumerator();

    DataPacketPtr outputDomainPacket;
    if (domainRuleType == DataRuleType::Linear)
    {
        const auto packetOffset = inputDomainPacket.getOffset() + offsetToAdd;
        outputDomainPacket = DataPacket(inputDomainPacket.getDataDescriptor(), inputDomainPacket.getSampleCount(), packetOffset);
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
        outputDataPacket = DataPacketWithDomain(outputDomainPacket, dataDescriptor, inputPacket.getSampleCount(), inputPacket.getOffset());
        void* src = inputPacket.getRawData();
        void* dst = outputDataPacket.getRawData();
        memcpy(dst, src, inputPacket.getRawDataSize());
    } 

    domainSignal.sendPacket(outputDomainPacket);
    dataSignal.sendPacket(outputDataPacket);
}

void TimeDelayFbImpl::handleEventPacket(const EventPacketPtr& packet)
{
    if (packet.getEventId() == event_packet_id::DATA_DESCRIPTOR_CHANGED)
    {
        try
        {
            DataDescriptorPtr inputDataDescriptor = packet.getParameters().get(event_packet_param::DATA_DESCRIPTOR);
            if (inputDataDescriptor.assigned())
            {
                dataDescriptor = inputDataDescriptor;
                dataSignal.setDescriptor(inputDataDescriptor);

                const auto dataRule = inputDataDescriptor.getRule();
                if (!dataRule.assigned())
                    DAQ_THROW_EXCEPTION(ArgumentNullException, "Data rule is not assigned");

                dataRuleType = dataRule.getType();
                if (dataRuleType != DataRuleType::Linear && dataRuleType != DataRuleType::Explicit)
                    DAQ_THROW_EXCEPTION(InvalidParametersException, "Data rule must be Linear or Explicit");
            }
            DataDescriptorPtr inputDomainDataDescriptor = packet.getParameters().get(event_packet_param::DOMAIN_DATA_DESCRIPTOR);
            if (inputDomainDataDescriptor.assigned())
            {
                domainDescriptor = inputDomainDataDescriptor;
                domainSignal.setDescriptor(inputDomainDataDescriptor);

                const auto domainUnit = inputDomainDataDescriptor.getUnit();
                if (!domainUnit.assigned())
                    DAQ_THROW_EXCEPTION(ArgumentNullException, "Domain unit is not assigned");

                if (domainUnit.getSymbol() != "s" && domainUnit.getSymbol() != "seconds")
                    DAQ_THROW_EXCEPTION(InvalidParametersException, "Domain unit expected in seconds");

                if (inputDomainDataDescriptor.getSampleType() != SampleType::Int64 && inputDomainDataDescriptor.getSampleType() != SampleType::UInt64)
                    DAQ_THROW_EXCEPTION(InvalidParametersException, "Incompatible domain data sample type");

                const auto domainRule = inputDomainDataDescriptor.getRule();
                if (!domainRule.assigned())
                    DAQ_THROW_EXCEPTION(ArgumentNullException, "Domain rule is not assigned");

                domainRuleType = domainRule.getType();
                if (domainRuleType != DataRuleType::Linear)
                    DAQ_THROW_EXCEPTION(InvalidParametersException, "Domain rule must be Linear");

                domainTickResolution = inputDomainDataDescriptor.getTickResolution();
                if (!domainTickResolution.assigned())
                    DAQ_THROW_EXCEPTION(ArgumentNullException, "Domain tick resolition is not assigned");
            }

            inputPort.setActive(true);
            setComponentStatus(ComponentStatus::Ok);
        }
        catch (const std::exception& e)
        {
            inputPort.setActive(false);
            setComponentStatusWithMessage(ComponentStatus::Error, e.what());
        }
    } 
}

} // namespace TimeScaler;
END_NAMESPACE_REF_FB_MODULE
