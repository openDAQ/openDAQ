#include <ref_device_module/ref_channel_impl.h>
#include <coreobjects/eval_value_factory.h>
#include <coretypes/procedure_factory.h>
#include <opendaq/signal_factory.h>
#include <coreobjects/coercer_factory.h>
#include <opendaq/range_factory.h>
#include <opendaq/packet_factory.h>
#include <fmt/format.h>
#include <coreobjects/callable_info_factory.h>
#include <opendaq/data_rule_factory.h>
#include <coreobjects/unit_factory.h>
#include <opendaq/scaling_factory.h>
#include <opendaq/custom_log.h>
#include <coreobjects/property_object_protected_ptr.h>
#include <coreobjects/argument_info_factory.h>


#define PI 3.141592653589793

BEGIN_NAMESPACE_REF_DEVICE_MODULE

RefChannelImpl::RefChannelImpl(const ContextPtr& context,
                               const ComponentPtr& parent,
                               const StringPtr& localId,
                               const RefChannelInit& init)
    : ChannelImpl(FunctionBlockType("RefChannel",  fmt::format("AI{}", init.index + 1), ""), context, parent, localId)
    , waveformType(WaveformType::Sine)
    , freq(0)
    , ampl(0)
    , dc(0)
    , noiseAmpl(0)
    , constantValue(0)
    , sampleRate(0)
    , index(init.index)
    , globalSampleRate(init.globalSampleRate)
    , counter(0)
    , startTime(init.startTime)
    , microSecondsFromEpochToStartTime(init.microSecondsFromEpochToStartTime)
    , lastCollectTime(0)
    , samplesGenerated(0)
    , re(std::random_device()())
    , needsSignalTypeChanged(false)
    , referenceDomainId(init.referenceDomainId)
{
    initProperties();
    waveformChangedInternal();
    signalTypeChangedInternal();
    packetSizeChangedInternal();
    resetCounter();
    createSignals();
    buildSignalDescriptors();
}

void RefChannelImpl::signalTypeChangedIfNotUpdating(const PropertyValueEventArgsPtr& args)
{
    if (!args.getIsUpdating())
        signalTypeChanged();
    else
        needsSignalTypeChanged = true;
}

void RefChannelImpl::initProperties()
{
    const auto waveformProp = SelectionProperty("Waveform", List<IString>("Sine", "Rect", "None", "Counter", "Constant"), 0);
    
    objPtr.addProperty(waveformProp);
    objPtr.getOnPropertyValueWrite("Waveform") +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { signalTypeChangedIfNotUpdating(args); };

    const auto freqProp = FloatPropertyBuilder("Frequency", 10.0)
                              .setVisible(EvalValue("$Waveform < 2"))
                              .setUnit(Unit("Hz"))
                              .setMinValue(0.1)
                              .setMaxValue(10000.0)
                              .build();
    
    objPtr.addProperty(freqProp);
    objPtr.getOnPropertyValueWrite("Frequency") +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { waveformChanged(); };

    const auto dcProp =
        FloatPropertyBuilder("DC", 0.0).setVisible(EvalValue("$Waveform < 3")).setUnit(Unit("V")).setMaxValue(10.0).setMinValue(-10.0).build();
    

    objPtr.addProperty(dcProp);
    objPtr.getOnPropertyValueWrite("DC") += [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { waveformChanged(); };

    const auto amplProp =
        FloatPropertyBuilder("Amplitude", 5.0).setVisible(EvalValue("$Waveform < 2")).setUnit(Unit("V")).setMaxValue(10.0).setMinValue(0.0).build();

    objPtr.addProperty(amplProp);
    objPtr.getOnPropertyValueWrite("Amplitude") +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { waveformChanged(); };

    const auto noiseAmplProp = FloatPropertyBuilder("NoiseAmplitude", 0.0)
                                   .setUnit(Unit("V"))
                                   .setVisible(EvalValue("$Waveform < 3"))
                                   .setMinValue(0.0)
                                   .setMaxValue(10.0)
                                   .build();
    
    objPtr.addProperty(noiseAmplProp);
    objPtr.getOnPropertyValueWrite("NoiseAmplitude") +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { waveformChanged(); };

    const auto useGlobalSampleRateProp = BoolProperty("UseGlobalSampleRate", True);

    objPtr.addProperty(useGlobalSampleRateProp);
    objPtr.getOnPropertyValueWrite("UseGlobalSampleRate") +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { signalTypeChangedIfNotUpdating(args); };

    const auto sampleRateProp = FloatPropertyBuilder("SampleRate", 100.0)
                                    .setVisible(EvalValue("!$UseGlobalSampleRate"))
                                    .setUnit(Unit("Hz"))
                                    .setMinValue(1.0)
                                    .setMaxValue(1000000.0)
                                    .setSuggestedValues(List<Float>(10.0, 100.0, 1000.0, 10000.0, 100000.0, 1000000.0))
                                    .build();

    objPtr.addProperty(sampleRateProp);
    objPtr.getOnPropertyValueWrite("SampleRate") += [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args)
    {
        if (args.getPropertyEventType() == PropertyEventType::Update)
        {
            double sr = args.getValue();
            auto coercedSr = this->coerceSampleRate(sr);
            if (coercedSr != sr)
                args.setValue(coercedSr);
        }

        signalTypeChangedIfNotUpdating(args);
    };

    objPtr.getOnPropertyValueRead("SampleRate") += [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args)
    {
        if (objPtr.getPropertyValue("UseGlobalSampleRate"))
        {
            args.setValue(globalSampleRate);
        }
    };

    const auto resetCounterProp =
        FunctionProperty("ResetCounter", ProcedureInfo(), EvalValue("$Waveform == 3"));
    objPtr.addProperty(resetCounterProp);
    objPtr.setPropertyValue("ResetCounter", Procedure([this] { this->resetCounter(); }));

    const auto getCurrentAndSetCounterProp =
        FunctionProperty("GetAndSetCounter", FunctionInfo(ctInt, List<IArgumentInfo>(ArgumentInfo("Val", ctInt))), EvalValue("$Waveform == 3"));
    objPtr.addProperty(getCurrentAndSetCounterProp);
    objPtr.setPropertyValue("GetAndSetCounter", Function([this](Int val)
    {
        auto lock = this->getRecursiveConfigLock();
        const auto cnt = counter;
        this->setCounter(val, false);
        return cnt;
    }));

    const auto clientSideScalingProp = BoolProperty("ClientSideScaling", False);

    objPtr.addProperty(clientSideScalingProp);
    objPtr.getOnPropertyValueWrite("ClientSideScaling") += [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { signalTypeChangedIfNotUpdating(args); };

    const auto testReadOnlyProp = BoolPropertyBuilder("TestReadOnly", False).setReadOnly(True).build();
    objPtr.addProperty(testReadOnlyProp);
    objPtr.asPtr<IPropertyObjectProtected>().setProtectedPropertyValue("TestReadOnly", True);

    const auto defaultCustomRange = Range(-10.0, 10.0);
    objPtr.addProperty(StructPropertyBuilder("CustomRange", defaultCustomRange).build());
    objPtr.getOnPropertyValueWrite("CustomRange") +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { signalTypeChangedIfNotUpdating(args); };

    objPtr.addProperty(BoolPropertyBuilder("FixedPacketSize", False).build());
    objPtr.getOnPropertyValueWrite("FixedPacketSize") +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { packetSizeChanged(); };

    objPtr.addProperty(IntPropertyBuilder("PacketSize", 1000).setVisible(EvalValue("$FixedPacketSize")).build());
    objPtr.getOnPropertyValueWrite("PacketSize") +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { packetSizeChanged(); };

    const auto constantValueProp =
        FloatPropertyBuilder("ConstantValue", 2.0)
            .setVisible(EvalValue("$Waveform == 4"))
            .setMinValue(-10.0)
            .setMaxValue(10.0)
            .build();

    objPtr.addProperty(constantValueProp);
    objPtr.getOnPropertyValueWrite("ConstantValue") +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { waveformChanged(); };
}

void RefChannelImpl::packetSizeChangedInternal()
{
    fixedPacketSize = objPtr.getPropertyValue("FixedPacketSize");
    packetSize = objPtr.getPropertyValue("PacketSize");
}

void RefChannelImpl::packetSizeChanged()
{
    packetSizeChangedInternal();
}

void RefChannelImpl::waveformChangedInternal()
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

void RefChannelImpl::updateSamplesGenerated()
{
    if (lastCollectTime.count() > 0)
        samplesGenerated = getSamplesSinceStart(lastCollectTime);
}

void RefChannelImpl::waveformChanged()
{
    waveformChangedInternal();
}

void RefChannelImpl::signalTypeChanged()
{
    signalTypeChangedInternal();
    buildSignalDescriptors();
    updateSamplesGenerated();
}

void RefChannelImpl::signalTypeChangedInternal()
{
    // TODO: Should global sample rate be coerced? We only coerce it on read now.
    if (objPtr.getPropertyValue("UseGlobalSampleRate"))
        sampleRate = globalSampleRate;
    else
        sampleRate = objPtr.getPropertyValue("SampleRate");
    clientSideScaling = objPtr.getPropertyValue("ClientSideScaling");

    customRange = objPtr.getPropertyValue("CustomRange");

    waveformType = objPtr.getPropertyValue("Waveform");

    LOG_I("Properties: SampleRate {}, ClientSideScaling {}", sampleRate, clientSideScaling);
}

void RefChannelImpl::resetCounter()
{
    auto lock = this->getRecursiveConfigLock();
    counter = 0;
}

void RefChannelImpl::setCounter(uint64_t cnt, bool shouldLock)
{
    if (shouldLock)
    {
        auto lock = this->getRecursiveConfigLock();
	    counter = cnt;
    }
    else
        counter = cnt;
}

uint64_t RefChannelImpl::getSamplesSinceStart(std::chrono::microseconds time) const
{
    const uint64_t samplesSinceStart = static_cast<uint64_t>(std::trunc(static_cast<double>((time - startTime).count()) / 1000000.0 * sampleRate));
    return samplesSinceStart;
}

void RefChannelImpl::collectSamples(std::chrono::microseconds curTime)
{
    auto lock = this->getAcquisitionLock();
    const uint64_t samplesSinceStart = getSamplesSinceStart(curTime);
    auto newSamples = samplesSinceStart - samplesGenerated;

    if (newSamples > 0)
    {
        if (!fixedPacketSize)
        {
            if (valueSignal.getActive())
            {
                const auto packetTime = samplesGenerated * deltaT + static_cast<uint64_t>(microSecondsFromEpochToStartTime.count());
                auto [dataPacket, domainPacket] = generateSamples(static_cast<int64_t>(packetTime), samplesGenerated, newSamples);

                valueSignal.sendPacket(std::move(dataPacket));
                timeSignal.sendPacket(std::move(domainPacket));
            }

            samplesGenerated = samplesSinceStart;
        }
        else
        {
            auto packets = List<IPacket>();
            auto domainPackets = List<IPacket>();
            while (newSamples >= packetSize)
            {
                if (valueSignal.getActive())
                {
                    const auto packetTime = samplesGenerated * deltaT + static_cast<uint64_t>(microSecondsFromEpochToStartTime.count());
                    auto [dataPacket, domainPacket] = generateSamples(static_cast<int64_t>(packetTime), samplesGenerated, packetSize);
                    packets.pushBack(std::move(dataPacket));
                    domainPackets.pushBack(std::move(domainPacket));
                }

                samplesGenerated += packetSize;
                newSamples -= packetSize;
            }

			if (!packets.empty())
            {           
                valueSignal.sendPackets(std::move(packets));
                timeSignal.sendPackets(std::move(domainPackets));
            }
        }
    }

    lastCollectTime = curTime;
}

std::tuple<PacketPtr, PacketPtr> RefChannelImpl::generateSamples(int64_t curTime, uint64_t samplesGenerated, uint64_t newSamples)
{
    auto domainPacket = DataPacket(timeSignal.getDescriptor(), newSamples, curTime);
    DataPacketPtr dataPacket;
    if (waveformType == WaveformType::ConstantValue)
    {
        dataPacket = ConstantDataPacketWithDomain(domainPacket, valueSignal.getDescriptor(), newSamples, constantValue);
    }
    else
    {
        dataPacket = DataPacketWithDomain(domainPacket, valueSignal.getDescriptor(), newSamples);

        double* buffer;

        if (clientSideScaling)
        {
            buffer = static_cast<double*>(std::malloc(newSamples * sizeof(double)));
        }
        else
            buffer = static_cast<double*>(dataPacket.getRawData());

        switch(waveformType)
        {
            case WaveformType::Counter:
            {
                for (uint64_t i = 0; i < newSamples; i++)
                    buffer[i] = static_cast<double>(counter++) / sampleRate;
                break;
            }
            case WaveformType::Sine:
            {
                for (uint64_t i = 0; i < newSamples; i++)
                    buffer[i] = std::sin(2.0 * PI * freq / sampleRate * static_cast<double>((samplesGenerated + i))) * ampl + dc + noiseAmpl * dist(re);
                break;
            }
            case WaveformType::Rect:
            {
                for (uint64_t i = 0; i < newSamples; i++)
                {
                    double val = std::sin(2.0 * PI * freq / sampleRate * static_cast<double>((samplesGenerated + i)));
                    val = val > 0 ? 1.0 : -1.0;
                    buffer[i] = val * ampl + dc + noiseAmpl * dist(re);
                }
                break;
            }
            case WaveformType::None:
            {
                for (uint64_t i = 0; i < newSamples; i++)
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
            for (size_t i = 0; i < newSamples; i++)
                *packetBuffer++ = static_cast<uint32_t>((buffer[i] + 10.0) / 20.0 * f);

            std::free(static_cast<void*>(buffer));
        }

    }

    return {dataPacket, domainPacket};
}

Int RefChannelImpl::getDeltaT(double sr)
{
    const double tickPeriod = getResolution();
    const double samplePeriod = 1.0 / sr;
    const Int deltaT = static_cast<Int>(std::round(samplePeriod / tickPeriod));
    return deltaT;
}

void RefChannelImpl::buildSignalDescriptors()
{
    const auto valueDescriptor = DataDescriptorBuilder()
                                 .setSampleType(SampleType::Float64)
                                 .setUnit(Unit("V", -1, "volts", "voltage"))
                                 .setValueRange(customRange)
                                 .setName("AI " + std::to_string(index + 1));

    if (clientSideScaling)
    {
        const double scale = 20.0 / std::pow(2, 24);
        constexpr double offset = -10.0;
        valueDescriptor.setPostScaling(LinearScaling(scale, offset, SampleType::Int32, ScaledSampleType::Float64));
    }

    if (waveformType == WaveformType::ConstantValue)
    {
        valueDescriptor.setRule(ConstantDataRule());
    }


    valueSignal.setDescriptor(valueDescriptor.build());

    deltaT = getDeltaT(sampleRate);

    const auto timeDescriptor =
        DataDescriptorBuilder()
            .setSampleType(SampleType::Int64)
            .setUnit(Unit("s", -1, "seconds", "time"))
            .setTickResolution(getResolution())
            .setRule(LinearDataRule(deltaT, 0))
            .setOrigin(getEpoch())
            .setName("Time AI " + std::to_string(index + 1))
            .setReferenceDomainInfo(
                ReferenceDomainInfoBuilder().setReferenceDomainId(referenceDomainId).setReferenceDomainOffset(0).build());

    timeSignal.setDescriptor(timeDescriptor.build());
}

double RefChannelImpl::coerceSampleRate(const double wantedSampleRate) const
{
    const double tickPeriod = getResolution();
    const double samplePeriod = 1.0 / wantedSampleRate;

    const double multiplier = samplePeriod / tickPeriod;

    double roundedMultiplier = std::round(multiplier);

    if (roundedMultiplier < 1.0)
        roundedMultiplier = 1.0;

    const double roundedSamplePeriod = roundedMultiplier * tickPeriod;

    double roundedSampleRate = 1.0 / roundedSamplePeriod;

    if (roundedSampleRate > 1000000)
        roundedSampleRate = 1000000;

    return roundedSampleRate;
}

void RefChannelImpl::createSignals()
{
    valueSignal = createAndAddSignal(fmt::format("AI{}", index));
    timeSignal = createAndAddSignal(fmt::format("AI{}Time", index), nullptr, false);
    valueSignal.setDomainSignal(timeSignal);
}

void RefChannelImpl::globalSampleRateChanged(double newGlobalSampleRate)
{
    const auto lock = getRecursiveConfigLock();
    globalSampleRate = coerceSampleRate(newGlobalSampleRate);
    signalTypeChanged();
}

std::string RefChannelImpl::getEpoch()
{
    const std::time_t epochTime = std::chrono::system_clock::to_time_t(std::chrono::time_point<std::chrono::system_clock>{});

    char buf[48];
    strftime(buf, sizeof buf, "%Y-%m-%dT%H:%M:%SZ", gmtime(&epochTime));

    return { buf };
}

RatioPtr RefChannelImpl::getResolution()
{
    return Ratio(1, 1000000);
}

void RefChannelImpl::endApplyProperties(const UpdatingActions& propsAndValues, bool parentUpdating)
{
    ChannelImpl<IRefChannel>::endApplyProperties(propsAndValues, parentUpdating);

    if (needsSignalTypeChanged)
    {
        signalTypeChanged();
        needsSignalTypeChanged = false;
    }
}

END_NAMESPACE_REF_DEVICE_MODULE
