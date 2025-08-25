#include <coreobjects/argument_info_factory.h>
#include <coreobjects/callable_info_factory.h>
#include <coreobjects/eval_value_factory.h>
#include <coreobjects/property_object_protected_ptr.h>
#include <coreobjects/unit_factory.h>
#include <fmt/format.h>
#include <opendaq/custom_log.h>
#include <opendaq/data_rule_factory.h>
#include <opendaq/packet_factory.h>
#include <opendaq/range_factory.h>
#include <opendaq/scaling_factory.h>
#include <opendaq/signal_factory.h>
#include <simulator_device_module/simulator_channel_impl.h>
#include <date/date.h>
#include <opendaq/event_packet_params.h>

#define PI 3.141592653589793

BEGIN_NAMESPACE_SIMULATOR_DEVICE_MODULE

SimulatorChannelImpl::SimulatorChannelImpl(const ContextPtr& context, const ComponentPtr& parent, const StringPtr& localId)
    : ChannelImpl(FunctionBlockType("SimulatorAI", "Simulated analog input channel", ""), context, parent, localId)
    , waveformType(WaveformType::Sine)
    , samplesGenerated(0)
    , freq(0)
    , ampl(0)
    , dc(0)
    , noiseAmpl(0)
    , constantValue(0)
    , clientSideScaling(false)
    , counter(0)
    , re(std::random_device()())
    , needsSignalTypeChanged(false)
{
    initProperties();
    createValueSignal();
    waveformChanged();
    signalTypeChanged();
    createDomainSignalInputPort();

    resetCounter();
    initComponentStatus();
}

void SimulatorChannelImpl::initProperties()
{
    const auto waveformProp = SelectionProperty("Waveform", List<IString>("Sine", "Rect", "None", "Counter", "Constant"), 0);
    
    objPtr.addProperty(waveformProp);
    objPtr.getOnPropertyValueWrite("Waveform") +=
        [this](PropertyObjectPtr&, PropertyValueEventArgsPtr& args) { signalTypeChanged(); };

    const auto freqProp = FloatPropertyBuilder("Frequency", 10.0)
                              .setVisible(EvalValue("$Waveform < 2"))
                              .setUnit(Unit("Hz"))
                              .setMinValue(0.1)
                              .setMaxValue(10000.0)
                              .build();
    
    objPtr.addProperty(freqProp);
    objPtr.getOnPropertyValueWrite("Frequency") +=
        [this](PropertyObjectPtr&, PropertyValueEventArgsPtr&) { waveformChanged(); };

    const auto dcProp =
        FloatPropertyBuilder("DC", 0.0).setVisible(EvalValue("$Waveform < 3")).setUnit(Unit("V")).setMaxValue(10.0).setMinValue(-10.0).build();
    

    objPtr.addProperty(dcProp);
    objPtr.getOnPropertyValueWrite("DC") += [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { waveformChanged(); };

    const auto amplProp =
        FloatPropertyBuilder("Amplitude", 5.0).setVisible(EvalValue("$Waveform < 2")).setUnit(Unit("V")).setMaxValue(10.0).setMinValue(0.0).build();

    objPtr.addProperty(amplProp);
    objPtr.getOnPropertyValueWrite("Amplitude") +=
        [this](PropertyObjectPtr&, PropertyValueEventArgsPtr&) { waveformChanged(); };

    const auto noiseAmplProp = FloatPropertyBuilder("NoiseAmplitude", 0.0)
                                   .setUnit(Unit("V"))
                                   .setVisible(EvalValue("$Waveform < 3"))
                                   .setMinValue(0.0)
                                   .setMaxValue(10.0)
                                   .build();
    
    objPtr.addProperty(noiseAmplProp);
    objPtr.getOnPropertyValueWrite("NoiseAmplitude") +=
        [this](PropertyObjectPtr&, PropertyValueEventArgsPtr&) { waveformChanged(); };

    const auto resetCounterProp =
        FunctionProperty("ResetCounter", ProcedureInfo(), EvalValue("$Waveform == 3"));
    objPtr.addProperty(resetCounterProp);
    objPtr.setPropertyValue("ResetCounter", Procedure([this] { this->resetCounter(); }));

    const auto clientSideScalingProp = BoolProperty("ClientSideScaling", False);

    objPtr.addProperty(clientSideScalingProp);
    objPtr.getOnPropertyValueWrite("ClientSideScaling") += [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { signalTypeChanged(); };

    const auto testReadOnlyProp = BoolPropertyBuilder("TestReadOnly", False).setReadOnly(True).build();
    objPtr.addProperty(testReadOnlyProp);
    objPtr.asPtr<IPropertyObjectProtected>().setProtectedPropertyValue("TestReadOnly", True);

    const auto defaultCustomRange = Range(-10.0, 10.0);
    objPtr.addProperty(StructPropertyBuilder("CustomRange", defaultCustomRange).build());
    objPtr.getOnPropertyValueWrite("CustomRange") +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { signalTypeChanged(); };

    const auto constantValueProp =
        FloatPropertyBuilder("ConstantValue", 2.0)
            .setVisible(EvalValue("$Waveform == 4"))
            .setMinValue(-10.0)
            .setMaxValue(10.0)
            .build();

    objPtr.addProperty(constantValueProp);
    objPtr.getOnPropertyValueWrite("ConstantValue") +=
        [this](PropertyObjectPtr&, PropertyValueEventArgsPtr&) { waveformChanged(); };

    const auto offsetProp =
        IntPropertyBuilder("Offset", 0)
            .setVisible(true)
            .setMinValue(0)
            .build();

    objPtr.addProperty(offsetProp);
    objPtr.getOnPropertyValueWrite("Offset") +=
        [this](PropertyObjectPtr&, PropertyValueEventArgsPtr& args) { signalTypeChanged(); };
}

void SimulatorChannelImpl::waveformChanged()
{
    waveformType = objPtr.getPropertyValue("Waveform");
    freq = objPtr.getPropertyValue("Frequency");
    dc = objPtr.getPropertyValue("DC");
    ampl = objPtr.getPropertyValue("Amplitude");
    noiseAmpl = objPtr.getPropertyValue("NoiseAmplitude");
    constantValue = objPtr.getPropertyValue("ConstantValue");
    LOG_I("Properties: Waveform {}, Frequency {}, DC {}, Amplitude {}, NoiseAmplitude {}, ConstantValue {}",
          objPtr.getPropertySelectionValue("Waveform").toString(), freq, dc, ampl, noiseAmpl, constantValue);
}

void SimulatorChannelImpl::signalTypeChanged()
{
    clientSideScaling = objPtr.getPropertyValue("ClientSideScaling");

    customRange = objPtr.getPropertyValue("CustomRange");

    waveformType = objPtr.getPropertyValue("Waveform");

    offset = objPtr.getPropertyValue("Offset");

    buildSignalDescriptors();
}

void SimulatorChannelImpl::resetCounter()
{
    counter = 0;
}

void SimulatorChannelImpl::sendData(const DataPacketPtr& domainPacket)
{
    const uint64_t newSampleCount = domainPacket.getSampleCount();
    if (newSampleCount > 0)
    {
        if (valueSignal.getActive())
        {
            auto dataPacket = generateSamples(newSampleCount, domainPacket);
            valueSignal.sendPacket(std::move(dataPacket));
        }

        samplesGenerated += newSampleCount;
    }
}

void SimulatorChannelImpl::processEventPacket(const EventPacketPtr& eventPacket)
{
    auto eventId = eventPacket.getEventId();
    if (eventId == event_packet_id::DATA_DESCRIPTOR_CHANGED)
    {
        const DataDescriptorPtr descriptor = eventPacket.getParameters().get(event_packet_param::DATA_DESCRIPTOR);
        double resolutionNum = static_cast<double>(descriptor.getTickResolution().getNumerator());
        double resolutionDen = static_cast<double>(descriptor.getTickResolution().getDenominator());
        this->deltaT = descriptor.getRule().getParameters().get("delta");
        assert(descriptor.getUnit().getSymbol() == "s");

        this->sampleRate =  resolutionDen / resolutionNum / static_cast<double>(deltaT);
    }
    else if (eventId == event_packet_id::IMPLICIT_DOMAIN_GAP_DETECTED)
    {
        // TODO: Test this
        uint64_t gap = eventPacket.getParameters().get(event_packet_param::GAP_DIFF);
        uint64_t gapInTicks = gap / deltaT;
        samplesGenerated += gapInTicks;
    }
}

PacketPtr SimulatorChannelImpl::generateSamples(uint64_t newSampleCount, const PacketPtr& domainPacket)
{
    DataPacketPtr dataPacket;
    auto valueDescriptor = valueSignal.getDescriptor();
    if (waveformType == WaveformType::ConstantValue)
    {
        dataPacket = ConstantDataPacketWithDomain(domainPacket, valueDescriptor, newSampleCount, constantValue);
    }
    else
    {
        dataPacket = DataPacketWithDomain(domainPacket, valueDescriptor, newSampleCount);
        double* buffer;

        if (clientSideScaling)
            buffer = static_cast<double*>(std::malloc(newSampleCount * sizeof(double)));
        else
            buffer = static_cast<double*>(dataPacket.getRawData());

        switch(waveformType)
        {
            case WaveformType::Counter:
            {
                for (uint64_t i = 0; i < newSampleCount; i++)
                    buffer[i] = static_cast<double>(counter++) / sampleRate;
                break;
            }
            case WaveformType::Sine:
            {
                for (uint64_t i = 0; i < newSampleCount; i++)
                    buffer[i] = std::sin(2.0 * PI * freq / sampleRate * static_cast<double>(samplesGenerated + i)) * ampl + dc + noiseAmpl * dist(re);
                break;
            }
            case WaveformType::Rect:
            {
                for (uint64_t i = 0; i < newSampleCount; i++)
                {
                    double val = std::sin(2.0 * PI * freq / sampleRate * static_cast<double>(samplesGenerated + i));
                    val = val > 0 ? 1.0 : -1.0;
                    buffer[i] = val * ampl + dc + noiseAmpl * dist(re);
                }
                break;
            }
            case WaveformType::None:
            {
                for (uint64_t i = 0; i < newSampleCount; i++)
                    buffer[i] = dc + noiseAmpl * dist(re);
                break;
            }
            case WaveformType::ConstantValue:
                break;
        }

        if (clientSideScaling)
        {
            double f = std::pow(2, 24);
            auto packetBuffer = static_cast<uint32_t*>(dataPacket.getRawData());
            for (size_t i = 0; i < newSampleCount; i++)
                *packetBuffer++ = static_cast<uint32_t>((buffer[i] + 10.0) / 20.0 * f);

            std::free(buffer);
        }
    }

    return dataPacket;
}

void SimulatorChannelImpl::buildSignalDescriptors() const
{
    const auto valueDescriptorBuilder = DataDescriptorBuilder()
                                        .setSampleType(SampleType::Float64)
                                        .setUnit(Unit("V", -1, "volts", "voltage"))
                                        .setValueRange(customRange);

    if (clientSideScaling)
    {
        const double scale = 20.0 / std::pow(2, 24);
        constexpr double offset = -10.0;
        valueDescriptorBuilder.setPostScaling(LinearScaling(scale, offset, SampleType::Int32, ScaledSampleType::Float64));
    }

    if (waveformType == WaveformType::ConstantValue)
    {
        valueDescriptorBuilder.setRule(ConstantDataRule());
    }


    valueSignal.setDescriptor(valueDescriptorBuilder.build());
}

void SimulatorChannelImpl::createValueSignal()
{
    valueSignal = Signal(context, signals, this->localId);
    valueSignal.getTags().asPtr<ITagsPrivate>(true).add("AnalogInput");
    signals.addItem(valueSignal);
}

void SimulatorChannelImpl::onConnected(const InputPortPtr& port)
{
    valueSignal.setDomainSignal(port.getSignal());
}

void SimulatorChannelImpl::createDomainSignalInputPort() const
{
    auto inputPort = InputPort(context, inputPorts, "DomainIP", true);
    inputPort.setNotificationMethod(PacketReadyNotification::SameThread);
    inputPort.setListener(this->template borrowPtr<InputPortNotificationsPtr>());

    inputPort.asPtr<IComponentPrivate>().unlockAllAttributes();
    inputPort.setVisible(false);
    inputPort.asPtr<IComponentPrivate>().lockAllAttributes();

    inputPorts.addItem(inputPort);
}

void SimulatorChannelImpl::endApplyProperties(const UpdatingActions& propsAndValues, bool parentUpdating)
{
    Channel::endApplyProperties(propsAndValues, parentUpdating);

    if (needsSignalTypeChanged)
    {
        signalTypeChanged();
        needsSignalTypeChanged = false;
    }
}

void SimulatorChannelImpl::onPacketReceived(const InputPortPtr& port)
{
    auto lock = this->getAcquisitionLock();

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
