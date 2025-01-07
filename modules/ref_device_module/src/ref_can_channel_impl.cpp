#include <ref_device_module/ref_can_channel_impl.h>
#include <opendaq/packet_factory.h>
#include <coreobjects/unit_factory.h>
#include <opendaq/dimension_factory.h>



BEGIN_NAMESPACE_REF_DEVICE_MODULE

#define PI 3.141592653589793

RefCANChannelBase::RefCANChannelBase(const templates::ChannelParams& params, const RefCANChannelInit& init)
    : ChannelTemplateHooks(std::make_shared<RefCANChannelImpl>(init), params)
{
}

RefCANChannelImpl::RefCANChannelImpl(const RefCANChannelInit& init)
    : lowerLimit(-1000)
    , upperLimit(1000)
    , counter1(0)
    , counter2(0)
    , microSecondsFromEpochToStartTime(init.microSecondsFromEpochToStartTime)
    , lastCollectTime(0)
{
}

void RefCANChannelImpl::initProperties()
{
    objPtr.addProperty(IntPropertyBuilder("UpperLimit", 1000).setMaxValue(10000000).setMinValue(1).build());
    objPtr.addProperty(IntPropertyBuilder("LowerLimit", -1000).setMaxValue(1).setMinValue(-10000000).build());
}

BaseObjectPtr RefCANChannelImpl::onPropertyWrite(const templates::PropertyEventArgs& args)
{
    if (args.isUpdating)
        return ChannelTemplate::onPropertyWrite(args);

    propertyChanged();
    return ChannelTemplate::onPropertyWrite(args);
}

void RefCANChannelImpl::propertyChanged()
{
    lowerLimit = objPtr.getPropertyValue("LowerLimit");
    upperLimit = objPtr.getPropertyValue("UpperLimit");
    counter1 = 0;
    counter2 = 0;
}

void RefCANChannelImpl::onEndUpdate(const templates::UpdateEndArgs& args)
{
    if (!args.changedProperties.empty())
        propertyChanged();
}

void RefCANChannelImpl::initSignals(const FolderConfigPtr& signalsFolder)
{
    const auto arbIdDescriptor = DataDescriptorBuilder().setName("ArbId").setSampleType(SampleType::Int32).build();
    const auto lengthDescriptor = DataDescriptorBuilder().setName("Length").setSampleType(SampleType::Int8).build();

    const auto dimensions = List<IDimension>(DimensionBuilder().setRule(LinearDimensionRule(0, 1, 64)).setName("Dimension").build());
    const auto dataDescriptor = DataDescriptorBuilder()
                                .setName("Data")
                                .setSampleType(SampleType::UInt8)
                                .setDimensions(dimensions)
                                .build();

    const auto canMsgDescriptor = DataDescriptorBuilder()
                                  .setSampleType(SampleType::Struct)
                                  .setStructFields(List<IDataDescriptor>(arbIdDescriptor, lengthDescriptor, dataDescriptor))
                                  .setName("CAN")
                                  .build();

    const auto timeDescriptor = DataDescriptorBuilder()
                                .setSampleType(SampleType::Int64)
                                .setUnit(Unit("s", -1, "seconds", "time"))
                                .setTickResolution(getResolution())
                                .setOrigin(getEpoch())
                                .setName("Time CAN")
                                .build();

    templates::SignalParams timeSigParams;
    timeSigParams.localId = "CANTime";
    timeSigParams.attributes.visible.value = false;
    timeSigParams.descriptor = timeDescriptor;
    timeSignal = createAndAddSignal(timeSigParams);

    templates::SignalParams valueSigParams;
    valueSigParams.localId = "CAN";
    valueSigParams.descriptor = canMsgDescriptor;
    valueSigParams.attributes.domainSignal.value = timeSignal;
    valueSignal = createAndAddSignal(valueSigParams);
}

void RefCANChannelImpl::collectSamples(std::chrono::microseconds curTime)
{
    auto lock = this->getAcquisitionLock();
    const auto duration = static_cast<int64_t>(curTime.count() - lastCollectTime.count());

    if (duration > 0 && valueSignal.getActive())
    {
        const auto time = static_cast<int64_t>(lastCollectTime.count()) + static_cast<int64_t>(microSecondsFromEpochToStartTime.count());
        generateSamples(time, duration, 2);
    }

    lastCollectTime = curTime;
}

void RefCANChannelImpl::generateSamples(int64_t curTime, uint64_t duration, size_t newSamples)
{
    const auto domainPacket = DataPacket(timeSignal.getDescriptor(), newSamples, curTime);
    const auto dataPacket = DataPacketWithDomain(domainPacket, valueSignal.getDescriptor(), newSamples);

    CANData* dataBuffer = static_cast<CANData*>(dataPacket.getRawData());
    int64_t* timeBuffer = static_cast<int64_t*>(domainPacket.getRawData());

    for (size_t i = 0; i < newSamples; i++)
    {
        dataBuffer->arbId = 12;
        dataBuffer->length = 8;

        int32_t* dataPtr = reinterpret_cast<int32_t*>(&(dataBuffer->data[0]));
        *dataPtr++ = counter1++;
        *dataPtr++ = counter2--;

        if (counter1 == upperLimit)
            counter1 = 0;
        if (counter2 == lowerLimit)
            counter2 = 0;
        dataBuffer++;

        *timeBuffer++ = curTime;
        curTime += duration / static_cast<uint64_t>(newSamples);
    }


    valueSignal.sendPacket(dataPacket);
    timeSignal.sendPacket(domainPacket);
}

std::string RefCANChannelImpl::getEpoch()
{
    const std::time_t epochTime = std::chrono::system_clock::to_time_t(std::chrono::time_point<std::chrono::system_clock>{});

    char buf[48];
    strftime(buf, sizeof buf, "%Y-%m-%dT%H:%M:%SZ", gmtime(&epochTime));

    return { buf };
}

RatioPtr RefCANChannelImpl::getResolution()
{
    return Ratio(1, 1000000);
}

END_NAMESPACE_REF_DEVICE_MODULE
