#include <ref_device_module/ref_device_impl.h>
#include <ref_device_module/ref_channel_impl.h>
#include <ref_device_module/ref_can_channel_impl.h>
#include <fmt/format.h>
#include <opendaq/device_domain_factory.h>

BEGIN_NAMESPACE_REF_DEVICE_MODULE

static constexpr size_t DEFAULT_NUMBER_OF_CHANNELS = 2;
static constexpr bool DEFAULT_ENABLE_CAN_CHANNEL = false;
static constexpr double DEFAULT_DEVICE_SAMPLE_RATE = 1000;
static constexpr int DEFAULT_ACQ_LOOP_TIME = 20;

RefDeviceBase::RefDeviceBase(const templates::DeviceParams& params)
    : DeviceTemplateHooks(std::make_shared<RefDeviceImpl>(), params)
{
}

RefDeviceImpl::RefDeviceImpl()
    : acqLoopTime(0)
    , stopAcq(false)
    , microSecondsFromEpochToDeviceStart(0)
    , domainUnit(UnitBuilder().setName("microsecond").setSymbol("us").setQuantity("time").build())
{
}

RefDeviceImpl::~RefDeviceImpl()
{
    {
        std::scoped_lock lock(sync);
        stopAcq = true;
    }

    cv.notify_one();
    acqThread.join();
}

void RefDeviceImpl::handleConfig(const PropertyObjectPtr& config)
{
    const auto obj = getDevice();

    if (config.hasProperty("NumberOfChannels"))
        obj.setPropertyValue("NumberOfChannels", config.getPropertyValue("NumberOfChannels"));

    if (config.hasProperty("EnableCANChannel"))
        obj.setPropertyValue("EnableCANChannel", config.getPropertyValue("EnableCANChannel"));
}

void RefDeviceImpl::handleOptions(const DictPtr<IString, IBaseObject>& options)
{
    const auto obj = getDevice();

    if (options.hasKey("NumberOfChannels"))
        obj.setPropertyValue("NumberOfChannels", options.get("NumberOfChannels"));

    if (options.hasKey("EnableCANChannel"))
        obj.setPropertyValue("EnableCANChannel", options.get("EnableCANChannel"));
}

void RefDeviceImpl::initProperties()
{
    const auto obj = getDevice();

    const auto globalSampleRateProp =
        FloatPropertyBuilder("SampleRate", DEFAULT_DEVICE_SAMPLE_RATE).setUnit(Unit("Hz")).setMinValue(1.0).setMaxValue(1000000.0).build();

    const auto acqLoopTimeProp =
        IntPropertyBuilder("AcquisitionLoopTime", DEFAULT_ACQ_LOOP_TIME).setUnit(Unit("ms")).setMinValue(10).setMaxValue(1000).build();
    
    obj.addProperty(globalSampleRateProp);
    obj.addProperty(acqLoopTimeProp);
    obj.addProperty(IntProperty("NumberOfChannels", DEFAULT_NUMBER_OF_CHANNELS));
    obj.addProperty(BoolProperty("EnableCANChannel", DEFAULT_ENABLE_CAN_CHANNEL));
}

BaseObjectPtr RefDeviceImpl::onPropertyWrite(const templates::PropertyEventArgs& args)
{
    if (args.propertyName == "NumberOfChannels")
        updateNumberOfChannels(args.value);
    else if (args.propertyName == "SampleRate")
        updateDeviceSampleRate(args.value);
    else if (args.propertyName == "AcquisitionLoopTime")
        updateAcqLoopTime(args.value);
    else if (args.propertyName == "EnableCANChannel")
        enableCANChannel(args.value);

    return nullptr;
}

uint64_t RefDeviceImpl::getTicksSinceOrigin()
{
    return static_cast<uint64_t>((microSecondsFromEpochToDeviceStart + getMicroSecondsSinceDeviceStart()).count());
}

std::chrono::microseconds RefDeviceImpl::getMicroSecondsSinceDeviceStart() const
{
    const auto currentTime = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(currentTime - startTime);
}

void RefDeviceImpl::initIOFolder(const IoFolderConfigPtr& ioFolder)
{
    aiFolder = createAndAddIOFolder("AI", ioFolder);
    canFolder = createAndAddIOFolder("CAN", ioFolder);

    const auto obj = getDevice();
    updateNumberOfChannels(obj.getPropertyValue("NumberOfChannels"));
    enableCANChannel(obj.getPropertyValue("EnableCANChannel"));
}

DeviceDomainPtr RefDeviceImpl::initDeviceDomain()
{
    startTime = std::chrono::steady_clock::now();
    microSecondsFromEpochToDeviceStart = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch());
    return DeviceDomain(RefChannelImpl::getResolution(), RefChannelImpl::getEpoch(), domainUnit);
}

void RefDeviceImpl::start()
{
    updateAcqLoopTime(getDevice().getPropertyValue("AcquisitionLoopTime"));
    acqThread = std::thread{ &RefDeviceImpl::acqLoop, this };
}

bool RefDeviceImpl::allowAddDevicesFromModules()
{
    return false;
}

bool RefDeviceImpl::allowAddFunctionBlocksFromModules()
{
    return true;
}

void RefDeviceImpl::initSyncComponent(const SyncComponentPrivatePtr& syncComponent)
{
    syncComponent.addInterface(PropertyObject(this->context.getTypeManager(), "PtpSyncInterface"));
    syncComponent.addInterface(PropertyObject(this->context.getTypeManager(), "InterfaceClockSync"));
    syncComponent.setSyncLocked(true);
}

void RefDeviceImpl::acqLoop()
{
    using namespace std::chrono_literals;
    using milli = std::chrono::milliseconds;

    auto startLoopTime = std::chrono::high_resolution_clock::now();
    const auto loopTime = milli(acqLoopTime);

    std::unique_lock lock(sync);

    while (!stopAcq)
    {
        const auto time = std::chrono::high_resolution_clock::now();
        const auto loopDuration = std::chrono::duration_cast<milli>(time - startLoopTime);
        const auto waitTime = loopDuration.count() >= loopTime.count() ? milli(0) : milli(loopTime.count() - loopDuration.count());
        startLoopTime = time;

        cv.wait_for(lock, waitTime);
        if (!stopAcq)
        {
            const auto curTime = getMicroSecondsSinceDeviceStart();

            for (const auto& ch : channels)
                ch->collectSamples(curTime);

            //if (canChannel != nullptr)
            //    canChannel->collectSamples(curTime);
        }
    }
}

void RefDeviceImpl::updateNumberOfChannels(size_t numberOfChannels)
{
    LOG_I("Properties: NumberOfChannels {}", numberOfChannels)
        
    std::scoped_lock lock(sync);
    const auto deviceSampleRate = getDevice().getPropertyValue("SampleRate");

    if (numberOfChannels < channels.size())
    {
        std::for_each(std::next(channels.begin(), numberOfChannels), channels.end(), [this](const std::shared_ptr<RefChannelImpl>& ch)
            {
                LOG_T("Removed AI Channel: {}", ch.getLocalId())
                removeComponent(aiFolder, ch->getChannel());
            });
        channels.erase(std::next(channels.begin(), numberOfChannels), channels.end());
    }

    const auto microSecondsSinceDeviceStart = getMicroSecondsSinceDeviceStart();
    for (auto i = channels.size() + 1; i < numberOfChannels + 1; i++)
    {
        RefChannelInit init{ i, deviceSampleRate, microSecondsSinceDeviceStart, microSecondsFromEpochToDeviceStart };

        templates::ChannelParams params;
        params.context = this->context;
        params.localId = fmt::format("AI_{}", i);
        params.parent = aiFolder;
        params.type = FunctionBlockType("RefChannel", "Reference Channel", "Simulates waveform data");
        params.logName = "RefChannel";

        channels.push_back(createAndAddChannel<RefChannelBase, RefChannelImpl, const RefChannelInit&>(params, init));
    }
}

void RefDeviceImpl::enableCANChannel(bool /*enableCANChannel*/)
{
    //LOG_I("Properties: EnableCANChannel {}", enableCANChannel)
    //    
    //std::scoped_lock lock(sync);
    //if (!enableCANChannel)
    //{
    //    if (canChannel.assigned())
    //        removeComponent(canFolder, canChannel);
    //    canChannel.release();
    //}
    //else
    //{
    //    RefCANChannelInit init{getMicroSecondsSinceDeviceStart(), microSecondsFromEpochToDeviceStart};
    //    canChannel = createAndAddChannel<RefCANChannelImpl>(canFolder, "CAN", init);
    //}
}

void RefDeviceImpl::updateAcqLoopTime(size_t loopTime)
{
    LOG_I("Properties: AcquisitionLoopTime {}", loopTime)
        
    std::scoped_lock lock(sync);
    this->acqLoopTime = loopTime;
}

void RefDeviceImpl::updateDeviceSampleRate(double sampleRate)
{
    LOG_I("Properties: GlobalSampleRate {}", sampleRate)
        
    std::scoped_lock lock(sync);
    for (auto& ch : channels)
        ch->globalSampleRateChanged(sampleRate);
}

END_NAMESPACE_REF_DEVICE_MODULE
