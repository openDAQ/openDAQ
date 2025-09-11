#include <simulator_device_module/signal_generator.h>
#include <coreobjects/property_factory.h>
#include <coreobjects/unit_factory.h>
#include <coreobjects/eval_value_factory.h>
#include <coreobjects/callable_info_factory.h>
#include <opendaq/custom_log.h>
#include <coreobjects/property_object_factory.h>
#include <opendaq/data_descriptor_factory.h>
#include <coreobjects/property_object_internal_ptr.h>
#include <coretypes/intfs.h>

BEGIN_NAMESPACE_OPENDAQ

#define PI 3.141592653589793

SignalGenerator::SignalGenerator()
    : sampleRate(0)
    , samplesGenerated(0)
    , waveformType(WaveformType::Sine)
    , freq(10.0)
    , ampl(5.0)
    , dc(0.0)
    , noiseAmpl(0.0)
    , constantValue(2.0)
    , counter(0)
    , re(std::random_device()())
{
}

PropertyObjectPtr SignalGenerator::initProperties()
{
    const auto waveformProp =
        SelectionProperty("Waveform", List<IString>("Sine", "Rect", "None", "Counter", "Constant"), 0);

    const auto frequencyProp = FloatPropertyBuilder("Frequency", 10.0)
                               .setVisible(EvalValue("$Waveform < 2"))
                               .setUnit(Unit("Hz"))
                               .setMinValue(0.1)
                               .setMaxValue(10000.0)
                               .build();

    const auto dcProp = FloatPropertyBuilder("DC", 0.0)
                        .setVisible(EvalValue("$Waveform < 2"))
                        .setUnit(Unit("V"))
                        .setMinValue(-10.0)
                        .setMaxValue(10.0)
                        .build();

    const auto amplitudeProp = FloatPropertyBuilder("Amplitude", 5.0)
                               .setVisible(EvalValue("$Waveform < 2"))
                               .setUnit(Unit("V"))
                               .setMinValue(0.0)
                               .setMaxValue(10.0)
                               .build();

    const auto noiseAmplitudeProp = FloatPropertyBuilder("NoiseAmplitude", 0.0)
                                    .setVisible(EvalValue("$Waveform < 3"))
                                    .setUnit(Unit("V"))
                                    .setMinValue(0.0)
                                    .setMaxValue(10.0)
                                    .build();

    const auto constantValueProp = FloatPropertyBuilder("ConstantValue", 2.0)
                                   .setVisible(EvalValue("$Waveform == 4"))
                                   .setMinValue(-10.0)
                                   .setMaxValue(10.0)
                                   .build();


    const auto resetCounterProp = FunctionProperty("ResetCounter", ProcedureInfo(), EvalValue("$Waveform == 2"));
    
    generatorSettings = PropertyObject();
    generatorSettings.addProperty(waveformProp);
    generatorSettings.addProperty(frequencyProp);
    generatorSettings.addProperty(dcProp);
    generatorSettings.addProperty(amplitudeProp);
    generatorSettings.addProperty(noiseAmplitudeProp);
    generatorSettings.addProperty(resetCounterProp);
    generatorSettings.addProperty(constantValueProp);

    generatorSettings.setPropertyValue("ResetCounter", Procedure([this] { this->resetCounter(); }));

    generatorSettings.getOnAnyPropertyValueWrite() += event(this, &SignalGenerator::waveformChanged);

    generatorSettings.asPtr<IPropertyObjectInternal>().setLockingStrategy(LockingStrategy::InheritLock);
    return generatorSettings;
}

PacketPtr SignalGenerator::generateData(const DataPacketPtr& domainPacket)
{
    DataPacketPtr dataPacket;
    size_t newSampleCount = domainPacket.getSampleCount();
    auto descriptor = valueSignal.getDescriptor();

    if (waveformType == WaveformType::ConstantValue)
    {
        dataPacket = ConstantDataPacketWithDomain(domainPacket, descriptor, newSampleCount, constantValue);
    }
    else
    {
        dataPacket = DataPacketWithDomain(domainPacket, descriptor, newSampleCount);
        double* buffer = static_cast<double*>(dataPacket.getRawData());

        switch(waveformType)
        {
            case WaveformType::Counter:
            {
                for (uint64_t i = 0; i < newSampleCount; i++)
                    buffer[i] = static_cast<double>(counter++) / static_cast<double>(sampleRate);
                break;
            }
            case WaveformType::Sine:
            {
                for (uint64_t i = 0; i < newSampleCount; i++)
                    buffer[i] = std::sin(2.0 * PI * freq / static_cast<double>(sampleRate) * static_cast<double>(samplesGenerated + i)) * ampl + dc + noiseAmpl * dist(re);
                break;
            }
            case WaveformType::Rect:
            {
                for (uint64_t i = 0; i < newSampleCount; i++)
                {
                    double val = std::sin(2.0 * PI * freq / static_cast<double>(sampleRate) * static_cast<double>(samplesGenerated + i));
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
    }
    
    samplesGenerated += newSampleCount;
    return dataPacket;
}

DataDescriptorPtr SignalGenerator::buildDescriptor() const
{
    const auto valueDescriptorBuilder =
        DataDescriptorBuilder().setSampleType(SampleType::Float64).setUnit(Unit("V", -1, "volts", "voltage"));

    if (waveformType < WaveformType::Counter)
    {
        valueDescriptorBuilder.setValueRange(Range(-10, 10));
    }

    if (waveformType == WaveformType::ConstantValue)
    {
        valueDescriptorBuilder.setRule(ConstantDataRule());
    }

    return valueDescriptorBuilder.build();
}

void SignalGenerator::waveformChanged(PropertyObjectPtr&, PropertyValueEventArgsPtr& args)
{
    enum class WaveformProperty { Waveform, Frequency, DC, Amplitude, NoiseAmplitude, ConstantValue, Unknown };
    static const std::unordered_map<std::string, WaveformProperty> waveformPropLookup
    {
        {"Waveform", WaveformProperty::Waveform},
        {"Frequency", WaveformProperty::Frequency},
        {"DC", WaveformProperty::DC},
        {"Amplitude", WaveformProperty::Amplitude},
        {"NoiseAmplitude", WaveformProperty::NoiseAmplitude},
        {"ConstantValue", WaveformProperty::ConstantValue},
        {"Unknown", WaveformProperty::Unknown},
    };
    
    auto propName = args.getProperty().getName();
    auto value = args.getValue();

    switch (waveformPropLookup.count(propName) ? waveformPropLookup.at(propName) : WaveformProperty::Unknown)
    {
        case WaveformProperty::Waveform:
            waveformType = value;
            valueSignal.setDescriptor(buildDescriptor());
            break;

        case WaveformProperty::Frequency:
            freq = value;
            break;

        case WaveformProperty::DC:
            dc = value;
            break;

        case WaveformProperty::Amplitude:
            ampl = value;
            break;

        case WaveformProperty::NoiseAmplitude:
            noiseAmpl = value;
            break;

        case WaveformProperty::ConstantValue:
            constantValue = value;
            break;

        case WaveformProperty::Unknown:
            break;
    }
}

void SignalGenerator::resetCounter()
{
    counter = 0;
}

END_NAMESPACE_OPENDAQ
