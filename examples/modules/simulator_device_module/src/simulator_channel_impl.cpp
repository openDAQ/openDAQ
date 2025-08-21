#include <coreobjects/argument_info_factory.h>
#include <coreobjects/callable_info_factory.h>
#include <coreobjects/eval_value_factory.h>
#include <coreobjects/property_object_protected_ptr.h>
#include <coreobjects/unit_factory.h>
#include <fmt/format.h>
#include <opendaq/custom_log.h>
#include <opendaq/data_rule_factory.h>
#include <opendaq/packet_factory.h>
#include <opendaq/signal_factory.h>
#include <simulator_device_module/simulator_channel_impl.h>
#include <date/date.h>
#include <opendaq/event_packet_params.h>


BEGIN_NAMESPACE_SIMULATOR_DEVICE_MODULE

static ListPtr<IInteger> calculateAvailableSampleRateDividers(uint64_t deviceSampleRate)
{
    auto availableDividers = List<IInteger>();
    constexpr uint16_t maxDividerCount = 10;
    for (uint64_t i = 1; i < deviceSampleRate / 2 && availableDividers.getCount() < maxDividerCount; ++i)
    {
        if (deviceSampleRate % i == 0)
            availableDividers.pushBack( i);
    }

    return availableDividers;
}

static uint16_t calculateNearestDivider(uint64_t deviceSampleRate, uint16_t previousDivider)
{
    if (deviceSampleRate % previousDivider == 0)
        return previousDivider;

    for (uint16_t delta = 0;; ++delta)
    {
        if (deviceSampleRate % (previousDivider + delta) == 0)
            return previousDivider + delta;

        if (deviceSampleRate % (previousDivider - delta) == 0)
            return previousDivider - delta;
    }
}

SimulatorChannelImpl::SimulatorChannelImpl(const ContextPtr& context, const ComponentPtr& parent, const StringPtr& localId)
    : ChannelImpl(FunctionBlockType("SimulatorAI", "Simulated analog input channel", ""), context, parent, localId)
    , selfChange(false)
    , sampleRateDivider(1)
    , generator(std::make_unique<SignalGenerator>())
{
    objPtr.asPtr<IPropertyObjectInternal>().setLockingStrategy(LockingStrategy::InheritLock);

    initProperties();
    createSignals();
    createDomainSignalInputPort();

    initComponentStatus();
}

void SimulatorChannelImpl::initProperties()
{
    objPtr.addProperty(ObjectProperty("SignalGenerator", generator->initProperties()));
    auto availableRatesProp = ListPropertyBuilder("AvailableSRDividers", List<IInteger>(1))
                                  .setVisible(false)
                                  .setReadOnly(true)
                                  .build();

    objPtr.addProperty(availableRatesProp);
    objPtr.getOnPropertyValueWrite("AvailableSRDividers") +=
        [this](PropertyObjectPtr&, PropertyValueEventArgsPtr& args)
        {
            ListPtr<IInteger> availableDividers = args.getValue();
            auto newDivider = calculateNearestDivider(sampleRate, sampleRateDivider);
            auto it = std::find(availableDividers.begin(), availableDividers.end(), newDivider);
            size_t index = it != availableDividers.end() ? std::distance(availableDividers.begin(), it) : 0;

            objPtr.asPtr<IPropertyObjectInternal>().setPropertyValueNoLock("SampleRateDivider", index);
        };


    objPtr.addProperty(SelectionProperty("SampleRateDivider", EvalValue("$AvailableSRDividers"), 0));
    objPtr.getOnPropertyValueWrite("SampleRateDivider") +=
        [this](PropertyObjectPtr&, PropertyValueEventArgsPtr& args) { this->dividerChanged(args); };
}

void SimulatorChannelImpl::sendData(const DataPacketPtr& domainPacket) const
{
    auto sampleCount = domainPacket.getSampleCount();
    if (sampleCount && valueSignal.getActive())
    {
        auto channelDomainPacket = DataPacket(timeSignal.getDescriptor(), sampleCount / sampleRateDivider, domainPacket.getOffset());
        timeSignal.sendPacket(channelDomainPacket);
        valueSignal.sendPacket(generator->generateData(channelDomainPacket));
    }
}

void SimulatorChannelImpl::processEventPacket(const EventPacketPtr& eventPacket)
{
    auto eventId = eventPacket.getEventId();
    if (eventId == event_packet_id::DATA_DESCRIPTOR_CHANGED)
    {
        inputDomainDescriptor = eventPacket.getParameters().get(event_packet_param::DATA_DESCRIPTOR);
        configureDomainSettings();
    }
    else if (eventId == event_packet_id::IMPLICIT_DOMAIN_GAP_DETECTED)
    {
        // TODO: Test this
        uint64_t gap = eventPacket.getParameters().get(event_packet_param::GAP_DIFF);
        uint64_t gapInTicks = gap / deltaT;
        generator->samplesGenerated += gapInTicks;
    }
}

void SimulatorChannelImpl::dividerChanged(const PropertyValueEventArgsPtr& args)
{
    size_t value = args.getValue();
    ListPtr<IInteger> availableDividers = args.getProperty().getSelectionValues();

    auto prevDivider = sampleRateDivider;
    sampleRateDivider = availableDividers[value];

    if (prevDivider != sampleRateDivider)
        configureDomainSettings();
}

void SimulatorChannelImpl::configureDomainSettings()
{
    const Int resolutionNum = inputDomainDescriptor.getTickResolution().getNumerator();
    const Int resolutionDen = inputDomainDescriptor.getTickResolution().getDenominator();
    
    const auto params = inputDomainDescriptor.getRule().getParameters();
    auto prevDelta = deltaT;
    deltaT = params.get("delta") * sampleRateDivider;

    if (prevDelta == deltaT)
        return;

    sampleRate = resolutionDen / resolutionNum / deltaT / sampleRateDivider;
    generator->sampleRate = sampleRate;
    generator->samplesGenerated = 0;

    configureDomainDescriptor();
}

void SimulatorChannelImpl::configureDomainDescriptor() const
{
    auto params = inputDomainDescriptor.getRule().getParameters();
    auto start = params.get("start");
    auto builder = DataDescriptorBuilderCopy(inputDomainDescriptor).setRule(LinearDataRule(deltaT, start));
    timeSignal.setDescriptor(builder.build());
}

void SimulatorChannelImpl::createSignals()
{    
    timeSignal = Signal(context, signals, "SignalTime");
    timeSignal.getTags().asPtr<ITagsPrivate>(true).add("SignalTime");

    timeSignal.asPtr<IComponentPrivate>().unlockAttributes(List<IString>("visible"));
    timeSignal.setVisible(false);
    timeSignal.asPtr<IComponentPrivate>().lockAttributes(List<IString>("visible"));

    valueSignal = Signal(context, signals, this->localId);
    valueSignal.getTags().asPtr<ITagsPrivate>(true).add("AnalogInput");
    valueSignal.setDescriptor(generator->buildDescriptor());
    generator->valueSignal = valueSignal;
    valueSignal.setDomainSignal(timeSignal);

    signals.addItem(timeSignal);
    signals.addItem(valueSignal);
}

void SimulatorChannelImpl::createDomainSignalInputPort() const
{
    auto inputPort = InputPort(context, inputPorts, "DomainIP", true);
    inputPort.setNotificationMethod(PacketReadyNotification::SameThread);
    inputPort.setListener(this->borrowPtr<InputPortNotificationsPtr>());

    inputPort.asPtr<IComponentPrivate>().unlockAllAttributes();
    inputPort.setVisible(false);
    inputPort.asPtr<IComponentPrivate>().lockAllAttributes();

    inputPorts.addItem(inputPort);
}

void SimulatorChannelImpl::onPacketReceived(const InputPortPtr& port)
{
    const auto connection = port.getConnection();
    if (!connection.assigned())
        return;

    PacketPtr packet = connection.dequeue();
    while (packet.assigned())
    {
        switch (packet.getType())
        {
            case PacketType::Event:
                processEventPacket(packet);
                break;

            case PacketType::Data:
                sendData(packet);
                break;

            default:
                break;
        }

        packet = connection.dequeue();
    }
}

END_NAMESPACE_SIMULATOR_DEVICE_MODULE
