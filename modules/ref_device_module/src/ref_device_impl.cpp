#include <ref_device_module/ref_device_impl.h>
#include <opendaq/device_info_factory.h>
#include <coreobjects/unit_factory.h>
#include <ref_device_module/ref_channel_impl.h>
#include <ref_device_module/ref_can_channel_impl.h>
#include <fmt/format.h>
#include <opendaq/custom_log.h>
#include <opendaq/device_type_factory.h>
#include <opendaq/device_domain_factory.h>
#include <utility>
#include <opendaq/sync_component_private_ptr.h>
#include <coreobjects/argument_info_factory.h>
#include <coreobjects/callable_info_factory.h>
#include <opendaq/log_file_info_factory.h>
#include <coretypes/filesystem.h>
#include <fstream>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <opendaq/packet_factory.h>

BEGIN_NAMESPACE_REF_DEVICE_MODULE

RefDeviceImpl::RefDeviceImpl(size_t id, const PropertyObjectPtr& config, const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId, const StringPtr& name)
    : GenericDevice<>(ctx, parent, localId, nullptr, name)
    , id(id)
    , serialNumber(fmt::format("DevSer{}", id))
    , microSecondsFromEpochToDeviceStart(0)
    , acqLoopTime(0)
    , stopAcq(false)
    , logger(ctx.getLogger())
    , loggerComponent( this->logger.assigned()
                          ? this->logger.getOrAddComponent(REF_MODULE_NAME)
                          : throw ArgumentNullException("Logger must not be null"))
    , samplesGenerated(0)
{
    if (config.assigned() && config.hasProperty("SerialNumber"))
    {
        const StringPtr serialTemp = config.getPropertyValue("SerialNumber");
        serialNumber = serialTemp.getLength() ? serialTemp : serialNumber;
    }

    const auto options = this->context.getModuleOptions(REF_MODULE_NAME);
    if (options.assigned() && options.hasKey("SerialNumber"))
    {
        const StringPtr serialTemp = config.getPropertyValue("SerialNumber");
        serialNumber = serialTemp.getLength() ? serialTemp : serialNumber;
    }

    initIoFolder();
    initSyncComponent();
    initClock();
    initProperties(config);
    createSignals();
    configureTimeSignal();
    updateNumberOfChannels();
    enableCANChannel();
    enableProtectedChannel();
    updateAcqLoopTime();

    acqThread = std::thread{ &RefDeviceImpl::acqLoop, this };
}

RefDeviceImpl::~RefDeviceImpl()
{
    {
        auto lock = this->getAcquisitionLock();
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
    devInfo.setSerialNumber(serialNumber.assigned() && serialNumber.getLength() != 0 ? serialNumber : String(fmt::format("DevSer{}", id)));
    devInfo.setDeviceType(CreateType());

    return devInfo;
}

DeviceTypePtr RefDeviceImpl::CreateType()
{
    const auto defaultConfig = PropertyObject();
    defaultConfig.addProperty(IntProperty("NumberOfChannels", 2));
    defaultConfig.addProperty(BoolProperty("EnableCANChannel", False));
    defaultConfig.addProperty(BoolProperty("EnableProtectedChannel", False));
    defaultConfig.addProperty(StringProperty("SerialNumber", ""));
    defaultConfig.addProperty(BoolProperty("EnableLogging", False));
    defaultConfig.addProperty(StringProperty("LoggingPath", "ref_device_simulator.log"));
    defaultConfig.addProperty(StringProperty("Name", ""));

    return DeviceType("daqref",
                      "Reference device",
                      "Reference device",
                      "daqref",
                      defaultConfig);
}

DeviceInfoPtr RefDeviceImpl::onGetInfo()
{
    return RefDeviceImpl::CreateDeviceInfo(id, serialNumber);
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

PropertyObjectPtr RefDeviceImpl::createProtectedObject() const
{
    const auto func = Function([](Int a, Int b) { return a + b; });

    const auto funcProp =
        FunctionPropertyBuilder("Sum", FunctionInfo(ctInt, List<IArgumentInfo>(ArgumentInfo("A", ctInt), ArgumentInfo("B", ctInt))))
            .setReadOnly(false)
            .build();

    auto protectedObject = PropertyObject();
    protectedObject.addProperty(StringProperty("Owner", "openDAQ TM"));
    protectedObject.addProperty(funcProp);
    protectedObject.setPropertyValue("Sum", func);

    // group "everyone" has a read-only access to the protected object
    // group "admin" can change the protected object and call methods on it

    auto permissions = PermissionsBuilder()
                           .inherit(false)
                           .assign("everyone", PermissionMaskBuilder().read())
                           .assign("admin", PermissionMaskBuilder().read().write().execute())
                           .build();

    protectedObject.getPermissionManager().setPermissions(permissions);

    return protectedObject;
}

void RefDeviceImpl::initClock()
{
    startTime = std::chrono::steady_clock::now();
    startTimeInMs = std::chrono::duration_cast<std::chrono::microseconds>(startTime.time_since_epoch());
    auto startAbsTime = std::chrono::system_clock::now();
    refDomainId = "openDAQ_" + serialNumber;

    microSecondsFromEpochToDeviceStart = std::chrono::duration_cast<std::chrono::microseconds>(startAbsTime.time_since_epoch());

    this->setDeviceDomain(
        DeviceDomain(RefChannelImpl::getResolution(),
                     RefChannelImpl::getEpoch(),
                     UnitBuilder().setName("second").setSymbol("s").setQuantity("time").build(),
                     ReferenceDomainInfoBuilder().setReferenceDomainId(refDomainId).setReferenceDomainOffset(0).build()));
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
    SyncComponentPrivatePtr syncComponentPrivate = syncComponent.asPtr<ISyncComponentPrivate>(true);

    syncComponentPrivate.addInterface(PropertyObject(this->context.getTypeManager(), "PtpSyncInterface"));
    syncComponentPrivate.addInterface(PropertyObject(this->context.getTypeManager(), "InterfaceClockSync"));
    syncComponent.setSelectedSource(1);
    syncComponentPrivate.setSyncLocked(true);
}

void RefDeviceImpl::acqLoop()
{
    using namespace std::chrono_literals;
    using  milli = std::chrono::milliseconds;

    auto startLoopTime = std::chrono::high_resolution_clock::now();
    const auto loopTime = milli(acqLoopTime);

    auto lock = getUniqueLock();

    while (!stopAcq)
    {
        const auto time = std::chrono::high_resolution_clock::now();
        const auto loopDuration = std::chrono::duration_cast<milli>(time - startLoopTime);
        const auto waitTime = loopDuration.count() >= loopTime.count() ? milli(0) : milli(loopTime.count() - loopDuration.count());
        startLoopTime = time;

        cv.wait_for(lock, waitTime);
        if (!stopAcq)
        if (!stopAcq)
        {
            const auto curTime = getMicroSecondsSinceDeviceStart();

            collectTimeSignalSamples(curTime);

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

            if (protectedChannel.assigned())
            {
                auto chPrivate = protectedChannel.asPtr<IRefChannel>();
                chPrivate->collectSamples(curTime);
            }

            lastCollectTime = curTime;
        }
    }
}

void RefDeviceImpl::initProperties(const PropertyObjectPtr& config)
{
    size_t numberOfChannels = 2;
    bool enableCANChannel = false;
    bool enableProtectedChannel = false;

    if (config.assigned())
    {
        if (config.hasProperty("NumberOfChannels"))
            numberOfChannels = config.getPropertyValue("NumberOfChannels");

        if (config.hasProperty("EnableCANChannel"))
            enableCANChannel = config.getPropertyValue("EnableCANChannel");

        if (config.hasProperty("EnableProtectedChannel"))
            enableProtectedChannel = config.getPropertyValue("EnableProtectedChannel");
        
        if (config.hasProperty("EnableLogging"))
            loggingEnabled = config.getPropertyValue("EnableLogging");

        if (config.hasProperty("LoggingPath"))
            loggingPath = config.getPropertyValue("LoggingPath");
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

    objPtr.addProperty(BoolProperty("EnableProtectedChannel", enableProtectedChannel));
    objPtr.getOnPropertyValueWrite("EnableProtectedChannel") +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { this->enableProtectedChannel(); };

    auto protectedObject = createProtectedObject();
    objPtr.addProperty(ObjectProperty("Protected", protectedObject));

    objPtr.addProperty(BoolProperty("EnableLogging", loggingEnabled));
    objPtr.getOnPropertyValueWrite("EnableLogging") +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { this->enableLogging(); };
    enableLogging();
}

void RefDeviceImpl::collectTimeSignalSamples(std::chrono::microseconds curTime)
{
    const uint64_t samplesSinceStart = getSamplesSinceStart(curTime);
    auto newSamples = samplesSinceStart - samplesGenerated;

    const auto packetTime = samplesGenerated * deltaT + static_cast<uint64_t>(microSecondsFromEpochToDeviceStart.count());

    auto domainPacket = DataPacket(timeSignal.getDescriptor(), newSamples, packetTime);
    timeSignal.sendPacket(std::move(domainPacket));

    samplesGenerated += newSamples;
}

uint64_t RefDeviceImpl::getSamplesSinceStart(std::chrono::microseconds time) const
{
    const uint64_t samplesSinceStart =
        static_cast<uint64_t>(std::trunc(static_cast<double>(time.count()) / 1000000.0 * globalSampleRate));
    return samplesSinceStart;
}

void RefDeviceImpl::updateSamplesGenerated()
{
    if (lastCollectTime.count() > 0)
        samplesGenerated = getSamplesSinceStart(lastCollectTime);
}

void RefDeviceImpl::updateNumberOfChannels()
{
    std::size_t num = objPtr.getPropertyValue("NumberOfChannels");
    LOG_I("Properties: NumberOfChannels {}", num);

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
        RefChannelInit init{i, globalSampleRate, microSecondsSinceDeviceStart, microSecondsFromEpochToDeviceStart, localId};
        auto chLocalId = fmt::format("RefCh{}", i);
        auto ch = createAndAddChannel<RefChannelImpl>(aiFolder, chLocalId, init);
        channels.push_back(std::move(ch));
    }
}

void RefDeviceImpl::enableCANChannel()
{
    bool enableCANChannel = objPtr.getPropertyValue("EnableCANChannel");

    if (!enableCANChannel)
    {
        if (canChannel.assigned() && hasChannel(canFolder, canChannel))
            removeChannel(canFolder, canChannel);

        canChannel.release();
    }
    else
    {
        auto microSecondsSinceDeviceStart = getMicroSecondsSinceDeviceStart();
        RefCANChannelInit init{microSecondsSinceDeviceStart, microSecondsFromEpochToDeviceStart};
        canChannel = createAndAddChannel<RefCANChannelImpl>(canFolder, "refcanch", init);
    }
}

void RefDeviceImpl::enableProtectedChannel()
{
    bool enabled = objPtr.getPropertyValue("EnableProtectedChannel");

    if (!enabled)
    {
        if (protectedChannel.assigned() && hasChannel(aiFolder, protectedChannel))
            removeChannel(aiFolder, protectedChannel);

        protectedChannel.release();
    }
    else
    {
        auto globalSampleRate = objPtr.getPropertyValue("GlobalSampleRate");
        auto microSecondsSinceDeviceStart = getMicroSecondsSinceDeviceStart();
        size_t index = channels.size();

        RefChannelInit init{index, globalSampleRate, microSecondsSinceDeviceStart, microSecondsFromEpochToDeviceStart, localId};
        const auto channelLocalId = "ProtectedChannel";

        auto permissions = PermissionsBuilder()
                               .inherit(false)
                               .assign("admin", PermissionMaskBuilder().read().write().execute())
                               .build();

        protectedChannel = createAndAddChannelWithPermissions<RefChannelImpl>(aiFolder, channelLocalId, permissions, init);
    }
}

void RefDeviceImpl::updateGlobalSampleRate()
{
    auto lock = getRecursiveConfigLock();

    configureTimeSignal();
    updateSamplesGenerated();
    LOG_I("Properties: GlobalSampleRate {}", globalSampleRate)

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

    this->acqLoopTime = static_cast<size_t>(loopTime);
}

void RefDeviceImpl::configureTimeSignal()
{
    globalSampleRate = objPtr.getPropertyValue("GlobalSampleRate");
    deltaT = RefChannelImpl::getDeltaT(globalSampleRate);

    const auto timeDescriptor =
        DataDescriptorBuilder()
            .setSampleType(SampleType::Int64)
            .setUnit(Unit("s", -1, "seconds", "time"))
            .setTickResolution(RefChannelImpl::getResolution())
            .setRule(LinearDataRule(deltaT, 0))
            .setOrigin(RefChannelImpl::getEpoch())
            .setName("Time")
            .setReferenceDomainInfo(ReferenceDomainInfoBuilder().setReferenceDomainId(refDomainId).setReferenceDomainOffset(0).build())
            .build();

    timeSignal.setDescriptor(timeDescriptor);
}

void RefDeviceImpl::enableLogging()
{
    loggingEnabled = objPtr.getPropertyValue("EnableLogging");
}

StringPtr toIso8601(const std::chrono::system_clock::time_point& timePoint) 
{
    std::time_t time = std::chrono::system_clock::to_time_t(timePoint);
    std::tm tm = *std::gmtime(&time);  // Use gmtime for UTC

    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ"); // ISO 8601 format
    return oss.str();
}

ListPtr<ILogFileInfo> RefDeviceImpl::onGetLogFileInfos()
{    
    {
        auto lock = getAcquisitionLock();
        if (!loggingEnabled)
        {
            return List<ILogFileInfo>();
        }
    }

    fs::path path(loggingPath);
    if (!fs::exists(path))
    {
        return List<ILogFileInfo>();
    }

    SizeT size = fs::file_size(path);

    auto ftime = fs::last_write_time(path);

    // Convert to time_point
    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now()
    );

    auto lastModified = toIso8601(sctp);

    auto logFileInfo = LogFileInfoBuilder().setName(path.filename().string())
                                           .setId(path.string())
                                           .setDescription("Log file for the reference device")
                                           .setSize(size)
                                           .setEncoding("utf-8")
                                           .setLastModified(lastModified)
                                           .build();
    
    return List<ILogFileInfo>(logFileInfo);
}

StringPtr RefDeviceImpl::onGetLog(const StringPtr& id, Int size, Int offset)
{
    {
        auto lock = getAcquisitionLock();
        if (!loggingEnabled)
            return "";

        if (id != loggingPath)
            return "";
    }
    
    std::ifstream file(loggingPath.toStdString(), std::ios::binary);
    if (!file.is_open())
        return "";

    file.seekg(0, std::ios::end);
    std::streampos fileSize = file.tellg();
    if (offset >= fileSize)
        return "";

    file.seekg(offset, std::ios::beg);

    if (size == -1)
        size = static_cast<Int>(static_cast<std::streamoff>(fileSize) - offset);
    else
        size = std::min(size, static_cast<Int>(static_cast<std::streamoff>(fileSize) - offset));

    std::vector<char> buffer(size);
    file.read(buffer.data(), size);
    file.close();

    return String(buffer.data(), size);
}

void RefDeviceImpl::createSignals()
{
    timeSignal = createAndAddSignal("Time", nullptr, true);
    timeSignal.getTags().asPtr<ITagsPrivate>(true).add("DeviceDomain");
}

END_NAMESPACE_REF_DEVICE_MODULE
