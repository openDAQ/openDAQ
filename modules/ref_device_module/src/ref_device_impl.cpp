#include <ref_device_module/ref_device_impl.h>
#include <opendaq/device_info_factory.h>
#include <coreobjects/unit_factory.h>
#include <ref_device_module/ref_channel_impl.h>
#include <ref_device_module/ref_can_channel_impl.h>
#include <opendaq/module_manager_ptr.h>
#include <fmt/format.h>
#include <opendaq/custom_log.h>
#include <opendaq/device_type_factory.h>
#include <opendaq/device_domain_factory.h>
#include <utility>
#include <opendaq/sync_component_internal_ptr.h>

BEGIN_NAMESPACE_REF_DEVICE_MODULE

RefDeviceImpl::RefDeviceImpl(size_t id, const PropertyObjectPtr& config, const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId, const StringPtr& name)
    : GenericDevice<>(ctx, parent, localId, nullptr, name)
    , id(id)
    , microSecondsFromEpochToDeviceStart(0)
    , acqLoopTime(0)
    , stopAcq(false)
    , logger(ctx.getLogger())
    , loggerComponent( this->logger.assigned()
                          ? this->logger.getOrAddComponent(REF_MODULE_NAME)
                          : throw ArgumentNullException("Logger must not be null"))
{
    initIoFolder();
    initSyncComponent();
    initClock();
    initProperties(config);
    updateNumberOfChannels();
    enableCANChannel();
    updateAcqLoopTime();

    if (config.assigned())
    {
        if (config.hasProperty("LocalId"))
            serialNumber = config.getPropertyValue("SerialNumber");
    }
    
    const auto options = this->context.getModuleOptions(REF_MODULE_NAME);
    if (options.assigned())
    {
        if (options.hasKey("SerialNumber"))
            serialNumber = options.get("SerialNumber");
    }

    acqThread = std::thread{ &RefDeviceImpl::acqLoop, this };
}

RefDeviceImpl::~RefDeviceImpl()
{
    {
        std::scoped_lock<std::mutex> lock(sync);
        stopAcq = true;
    }
    cv.notify_one();

    acqThread.join();
}

DeviceInfoPtr RefDeviceImpl::CreateDeviceInfo(size_t id, const StringPtr& serialNumber)
{
    auto devInfo = DeviceInfo(fmt::format("daqref://device{}", id));
    devInfo.setName(fmt::format("Device {}", id));
    devInfo.setManufacturer("openDAQ");
    devInfo.setModel("Reference device");
    devInfo.setSerialNumber(serialNumber.assigned() ? serialNumber : String(fmt::format("dev_ser_{}", id)));
    devInfo.setDeviceType(CreateType());

    return devInfo;
}

DeviceTypePtr RefDeviceImpl::CreateType()
{
    return DeviceType("daqref",
                      "Reference device",
                      "Reference device",
                      "daqref");
}

DeviceInfoPtr RefDeviceImpl::onGetInfo()
{
    auto deviceInfo = RefDeviceImpl::CreateDeviceInfo(id, serialNumber);
    deviceInfo.freeze();
    return deviceInfo;
}

uint64_t RefDeviceImpl::onGetTicksSinceOrigin()
{
    auto microSecondsSinceDeviceStart = getMicroSecondsSinceDeviceStart();
    auto ticksSinceEpoch = microSecondsFromEpochToDeviceStart + microSecondsSinceDeviceStart;
    return static_cast<SizeT>(ticksSinceEpoch.count());
}

bool RefDeviceImpl::allowAddDevicesFromModules()
{
    return true;
}

bool RefDeviceImpl::allowAddFunctionBlocksFromModules()
{
    return true;
}

std::chrono::microseconds RefDeviceImpl::getMicroSecondsSinceDeviceStart() const
{
    auto currentTime = std::chrono::steady_clock::now();
    auto microSecondsSinceDeviceStart = std::chrono::duration_cast<std::chrono::microseconds>(currentTime - startTime);
    return microSecondsSinceDeviceStart;
}

void RefDeviceImpl::initClock()
{
    startTime = std::chrono::steady_clock::now();
    auto startAbsTime = std::chrono::system_clock::now();

    microSecondsFromEpochToDeviceStart = std::chrono::duration_cast<std::chrono::microseconds>(startAbsTime.time_since_epoch());

    this->setDeviceDomain(DeviceDomain(RefChannelImpl::getResolution(), RefChannelImpl::getEpoch(), UnitBuilder().setName("second").setSymbol("s").setQuantity("time").build()));
}

void RefDeviceImpl::initIoFolder()
{
    aiFolder = this->addIoFolder("AI", ioFolder);
    canFolder = this->addIoFolder("CAN", ioFolder);
}

void RefDeviceImpl::initSyncComponent()
{
    SyncComponentPtr syncComponent;
    this->getSyncComponent(&syncComponent);
    SyncComponentInternalPtr syncComponentInternal = syncComponent.asPtr<ISyncComponentInternal>(true);

    syncComponentInternal.addInterface(PropertyObject(this->context.getTypeManager(), "PtpSyncInterface"));
    syncComponentInternal.addInterface(PropertyObject(this->context.getTypeManager(), "InterfaceClockSync"));
    syncComponent.setSelectedSource(1);
    syncComponentInternal.setSyncLocked(true);
}

void RefDeviceImpl::acqLoop()
{
    using namespace std::chrono_literals;
    using  milli = std::chrono::milliseconds;

    auto startLoopTime = std::chrono::high_resolution_clock::now();
    const auto loopTime = milli(acqLoopTime);

    std::unique_lock<std::mutex> lock(sync);

    while (!stopAcq)
    {
        const auto time = std::chrono::high_resolution_clock::now();
        const auto loopDuration = std::chrono::duration_cast<milli>(time - startLoopTime);
        const auto waitTime = loopDuration.count() >= loopTime.count() ? milli(0) : milli(loopTime.count() - loopDuration.count());
        startLoopTime = time;

        cv.wait_for(lock, waitTime);
        if (!stopAcq)
        {
            auto curTime = getMicroSecondsSinceDeviceStart();

            for (auto& ch : channels)
            {
                auto chPrivate = ch.asPtr<IRefChannel>();
                chPrivate->collectSamples(curTime);
            }

            if (canChannel.assigned())
            {
                auto chPrivate = canChannel.asPtr<IRefChannel>();
                chPrivate->collectSamples(curTime);
            }
        }
    }
}

void RefDeviceImpl::initProperties(const PropertyObjectPtr& config)
{
    size_t numberOfChannels = 2;
    bool enableCANChannel = false;

    if (config.assigned())
    {
        if (config.hasProperty("NumberOfChannels"))
            numberOfChannels = config.getPropertyValue("NumberOfChannels");

        if (config.hasProperty("EnableCANChannel"))
            enableCANChannel = config.getPropertyValue("EnableCANChannel");
    } 
    
    const auto options = this->context.getModuleOptions(REF_MODULE_NAME);
    if (options.assigned())
    {
        if (options.hasKey("NumberOfChannels"))
            numberOfChannels = options.get("NumberOfChannels");

        if (options.hasKey("EnableCANChannel"))
            enableCANChannel = options.get("EnableCANChannel");
    }

    if (numberOfChannels < 1 || numberOfChannels > 4096)
        throw InvalidParameterException("Invalid number of channels");

    objPtr.addProperty(IntProperty("NumberOfChannels", numberOfChannels));
    objPtr.getOnPropertyValueWrite("NumberOfChannels") +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { updateNumberOfChannels(); };
		
    const auto globalSampleRatePropInfo =
        FloatPropertyBuilder("GlobalSampleRate", 1000.0).setUnit(Unit("Hz")).setMinValue(1.0).setMaxValue(1000000.0).build();

    objPtr.addProperty(globalSampleRatePropInfo);
    objPtr.getOnPropertyValueWrite("GlobalSampleRate") +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { updateGlobalSampleRate(); };

    const auto acqLoopTimePropInfo =
        IntPropertyBuilder("AcquisitionLoopTime", 20).setUnit(Unit("ms")).setMinValue(10).setMaxValue(1000).build();

    objPtr.addProperty(acqLoopTimePropInfo);
    objPtr.getOnPropertyValueWrite("AcquisitionLoopTime") += [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) {
        updateAcqLoopTime();
    };

    objPtr.addProperty(BoolProperty("EnableCANChannel", enableCANChannel));
    objPtr.getOnPropertyValueWrite("EnableCANChannel") +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { this->enableCANChannel(); };
}

void RefDeviceImpl::updateNumberOfChannels()
{
    std::size_t num = objPtr.getPropertyValue("NumberOfChannels");
    LOG_I("Properties: NumberOfChannels {}", num);
    auto globalSampleRate = objPtr.getPropertyValue("GlobalSampleRate");

    std::scoped_lock lock(sync);

    if (num < channels.size())
    {
        std::for_each(std::next(channels.begin(), num), channels.end(), [this](const ChannelPtr& ch)
            {
                removeChannel(nullptr, ch);
            });
        channels.erase(std::next(channels.begin(), num), channels.end());
    }

    auto microSecondsSinceDeviceStart = getMicroSecondsSinceDeviceStart();
    for (auto i = channels.size(); i < num; i++)
    {
        RefChannelInit init{ i, globalSampleRate, microSecondsSinceDeviceStart, microSecondsFromEpochToDeviceStart };
        auto localId = fmt::format("RefCh{}", i);
        auto ch = createAndAddChannel<RefChannelImpl>(aiFolder, localId, init);
        channels.push_back(std::move(ch));
    }
}

void RefDeviceImpl::enableCANChannel()
{
    bool enableCANChannel = objPtr.getPropertyValue("EnableCANChannel");

    std::scoped_lock lock(sync);

    if (!enableCANChannel)
    {
        if (canChannel.assigned() && hasChannel(nullptr, canChannel))
            removeChannel(nullptr, canChannel);
        canChannel.release();
    }
    else
    {
        auto microSecondsSinceDeviceStart = getMicroSecondsSinceDeviceStart();
        RefCANChannelInit init{microSecondsSinceDeviceStart, microSecondsFromEpochToDeviceStart};
        canChannel = createAndAddChannel<RefCANChannelImpl>(canFolder, "refcanch", init);
    }
}

void RefDeviceImpl::updateGlobalSampleRate()
{
    auto globalSampleRate = objPtr.getPropertyValue("GlobalSampleRate");
    LOG_I("Properties: GlobalSampleRate {}", globalSampleRate);

    std::scoped_lock lock(sync);

    for (auto& ch : channels)
    {
        auto chPriv = ch.asPtr<IRefChannel>();
        chPriv->globalSampleRateChanged(globalSampleRate);
    }
}

void RefDeviceImpl::updateAcqLoopTime()
{
    Int loopTime = objPtr.getPropertyValue("AcquisitionLoopTime");
    LOG_I("Properties: AcquisitionLoopTime {}", loopTime);

    std::scoped_lock lock(sync);
    this->acqLoopTime = static_cast<size_t>(loopTime);
}

END_NAMESPACE_REF_DEVICE_MODULE
