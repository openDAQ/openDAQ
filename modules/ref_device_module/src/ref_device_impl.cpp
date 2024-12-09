#include <ref_device_module/ref_device_impl.h>
#include <ref_device_module/ref_channel_impl.h>
#include <ref_device_module/ref_can_channel_impl.h>
#include <fmt/format.h>
#include <opendaq/device_domain_factory.h>
#include <coreobjects/argument_info_factory.h>
#include <coreobjects/callable_info_factory.h>
#include <opendaq/log_file_info_factory.h>
#include <coretypes/filesystem.h>
#include <fstream>
#include <sstream>
#include <chrono>
#include <iomanip>

BEGIN_NAMESPACE_REF_DEVICE_MODULE

static constexpr size_t DEFAULT_NUMBER_OF_CHANNELS = 2;
static constexpr bool DEFAULT_ENABLE_CAN_CHANNEL = false;
static constexpr bool DEFAULT_ENABLE_PROTECTED_CHANNEL = false;
static constexpr bool DEFAULT_ENABLE_LOGGING = false;
static constexpr char DEFAULT_LOGGING_PATH[] = "ref_device_simulator.log";
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
        auto lock = this->getAcquisitionLock();
        stopAcq = true;
    }

    cv.notify_one();
    acqThread.join();
}

void RefDeviceImpl::initProperties()
{
    const auto globalSampleRateProp =
        FloatPropertyBuilder("SampleRate", DEFAULT_DEVICE_SAMPLE_RATE).setUnit(Unit("Hz")).setMinValue(1.0).setMaxValue(1000000.0).build();

    const auto acqLoopTimeProp =
        IntPropertyBuilder("AcquisitionLoopTime", DEFAULT_ACQ_LOOP_TIME).setUnit(Unit("ms")).setMinValue(10).setMaxValue(1000).build();

    const auto loggingPathProp = StringPropertyBuilder("LoggingPath", DEFAULT_LOGGING_PATH).setReadOnly(true).build();

    objPtr.addProperty(globalSampleRateProp);
    objPtr.addProperty(acqLoopTimeProp);
    objPtr.addProperty(IntProperty("NumberOfChannels", DEFAULT_NUMBER_OF_CHANNELS));
    objPtr.addProperty(BoolProperty("EnableCANChannel", DEFAULT_ENABLE_CAN_CHANNEL));
    objPtr.addProperty(BoolProperty("EnableProtectedChannel", DEFAULT_ENABLE_PROTECTED_CHANNEL));
    objPtr.addProperty(BoolProperty("LoggingEnabled", DEFAULT_ENABLE_LOGGING));
    objPtr.addProperty(loggingPathProp);
    objPtr.addProperty(ObjectProperty("ProtectedObject", createProtectedObject()));
}

void RefDeviceImpl::applyConfig(const PropertyObjectPtr& config)
{
    objPtr.setPropertyValue("NumberOfChannels", config.getPropertyValue("NumberOfChannels"));
    objPtr.setPropertyValue("EnableCANChannel", config.getPropertyValue("EnableCANChannel"));
    objPtr.setPropertyValue("EnableProtectedChannel", config.getPropertyValue("EnableProtectedChannel"));
    objPtr.setPropertyValue("LoggingEnabled", config.getPropertyValue("EnableLogging"));
    loggingPath = config.getPropertyValue("LoggingPath");
    objPtr.asPtr<IPropertyObjectProtected>().setProtectedPropertyValue("LoggingPath", loggingPath);
}

// TODO: Add reference domain implementation
DeviceDomainPtr RefDeviceImpl::initDeviceDomain()
{
    startTime = std::chrono::steady_clock::now();
    microSecondsFromEpochToDeviceStart = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch());
    return DeviceDomain(RefChannelImpl::getResolution(), RefChannelImpl::getEpoch(), domainUnit);
}

void RefDeviceImpl::initIOFolder(const IoFolderConfigPtr& ioFolder)
{
    aiFolder = createAndAddIOFolder("AI", ioFolder);
    canFolder = createAndAddIOFolder("CAN", ioFolder);

    updateNumberOfChannels(objPtr.getPropertyValue("NumberOfChannels"));
    enableCANChannel(objPtr.getPropertyValue("EnableCANChannel"));
}

// TODO: Add actual synchronization implementation
void RefDeviceImpl::initSyncComponent(const SyncComponentPrivatePtr& syncComponent)
{
    syncComponent.addInterface(PropertyObject(this->context.getTypeManager(), "PtpSyncInterface"));
    syncComponent.addInterface(PropertyObject(this->context.getTypeManager(), "InterfaceClockSync"));
    syncComponent.setSyncLocked(true);
}

void RefDeviceImpl::start()
{
    updateAcqLoopTime(objPtr.getPropertyValue("AcquisitionLoopTime"));
    acqThread = std::thread{ &RefDeviceImpl::acqLoop, this };
}

// TODO: Change to TAI
std::chrono::microseconds RefDeviceImpl::getMicroSecondsSinceDeviceStart() const
{
    const auto currentTime = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(currentTime - startTime);
}

uint64_t RefDeviceImpl::getTicksSinceOrigin()
{
    return static_cast<uint64_t>((microSecondsFromEpochToDeviceStart + getMicroSecondsSinceDeviceStart()).count());
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
    else if (args.propertyName == "EnableProtectedChannel")
        enableProtectedChannel();

    return nullptr;
}

void RefDeviceImpl::removeRedundantChannels(size_t numberOfChannels)
{
    if (numberOfChannels > channels.size())
        return;

    const auto removeFunc = [this](const std::shared_ptr<RefChannelImpl>& ch)
        {
            LOG_T("Removed AI Channel: {}", ch.getLocalId())
            removeComponent(aiFolder, ch->getChannel());
        };

    std::for_each(std::next(channels.begin(), numberOfChannels), channels.end(), removeFunc);
    channels.erase(std::next(channels.begin(), numberOfChannels), channels.end());
}

void RefDeviceImpl::addMissingChannels(size_t numberOfChannels)
{
    const auto microSecondsSinceDeviceStart = getMicroSecondsSinceDeviceStart();
    const auto deviceSampleRate = objPtr.getPropertyValue("SampleRate");

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
        LOG_T("Added AI Channel: {}", localId)
    }
}

void RefDeviceImpl::updateNumberOfChannels(size_t numberOfChannels)
{
    LOG_I("Properties: NumberOfChannels {}", numberOfChannels)
    removeRedundantChannels(numberOfChannels);
    addMissingChannels(numberOfChannels);
}

void RefDeviceImpl::enableCANChannel(bool /*enableCANChannel*/)
{
    //bool enableCANChannel = objPtr.getPropertyValue("EnableCANChannel");

    //if (!enableCANChannel)
    //{
    //    if (canChannel.assigned() && hasChannel(canFolder, canChannel))
    //        removeChannel(canFolder, canChannel);
    //
    //    canChannel.release();
    //}
    // else
    //{
    //    auto microSecondsSinceDeviceStart = getMicroSecondsSinceDeviceStart();
    //    RefCANChannelInit init{microSecondsSinceDeviceStart, microSecondsFromEpochToDeviceStart};
    //    canChannel = createAndAddChannel<RefCANChannelImpl>(canFolder, "refcanch", init);
    //}
}

void RefDeviceImpl::enableProtectedChannel()
{
    //bool enabled = objPtr.getPropertyValue("EnableProtectedChannel");

    //if (!enabled)
    //{
    //    if (protectedChannel.assigned() && hasChannel(aiFolder, protectedChannel))
    //        removeChannel(aiFolder, protectedChannel);

    //    protectedChannel.release();
    //}
    //else
    //{
    //    auto globalSampleRate = objPtr.getPropertyValue("GlobalSampleRate");
    //    auto microSecondsSinceDeviceStart = getMicroSecondsSinceDeviceStart();
    //    size_t index = channels.size();

    //    RefChannelInit init{index, globalSampleRate, microSecondsSinceDeviceStart, microSecondsFromEpochToDeviceStart, localId};
    //    const auto channelLocalId = "ProtectedChannel";

    //    auto permissions = PermissionsBuilder()
    //                           .inherit(false)
    //                           .assign("admin", PermissionMaskBuilder().read().write().execute())
    //                           .build();

    //    protectedChannel = createAndAddChannelWithPermissions<RefChannelImpl>(aiFolder, channelLocalId, permissions, init);
    //}
}

void RefDeviceImpl::updateAcqLoopTime(size_t loopTime)
{
    LOG_I("Properties: AcquisitionLoopTime {}", loopTime)
    this->acqLoopTime = loopTime;
}

void RefDeviceImpl::updateDeviceSampleRate(double sampleRate)
{
    LOG_I("Properties: GlobalSampleRate {}", sampleRate)
    for (auto& ch : channels)
        ch->globalSampleRateChanged(sampleRate);
}

StringPtr toIso8601(const std::chrono::system_clock::time_point& timePoint) 
{
    std::time_t time = std::chrono::system_clock::to_time_t(timePoint);
    std::tm tm = *std::gmtime(&time);  // Use gmtime for UTC

    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ"); // ISO 8601 format
    return oss.str();
}

ListPtr<ILogFileInfo> RefDeviceImpl::getLogFileInfos()
{    
    if (!objPtr.getPropertyValue("LoggingEnabled"))
        return List<ILogFileInfo>();

    fs::path path(loggingPath);
    if (!fs::exists(path))
        return List<ILogFileInfo>();

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

StringPtr RefDeviceImpl::getLog(const StringPtr& id, Int size, Int offset)
{;
    if(!objPtr.getPropertyValue("LoggingEnabled"))
        return "";

    if (id != loggingPath)
        return "";

    std::ifstream file(loggingPath.toStdString(), std::ios::binary);
    if (!file.is_open())
        return "";

    file.seekg(0, std::ios::end);
    std::streampos fileSize = file.tellg();
    if (offset >= fileSize)
        return "";

    file.seekg(offset, std::ios::beg);

    if (size == -1)
        size = fileSize - offset;
    else
        size = std::min(size, static_cast<Int>(fileSize - offset));

    std::vector<char> buffer(size);
    file.read(buffer.data(), size);
    file.close();

    return String(buffer.data(), size);
}

// TODO: Change to representative example
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

void RefDeviceImpl::acqLoop()
{
    using namespace std::chrono_literals;
    using milli = std::chrono::milliseconds;

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
        {
            const auto curTime = getMicroSecondsSinceDeviceStart();

            for (const auto& ch : channels)
                ch->collectSamples(curTime);

            //if (canChannel.assigned())
            //{
            //    auto chPrivate = canChannel.asPtr<IRefChannel>();
            //    chPrivate->collectSamples(curTime);
            //}

            //if (protectedChannel.assigned())
            //{
            //    auto chPrivate = protectedChannel.asPtr<IRefChannel>();
            //    chPrivate->collectSamples(curTime);
            //}
        }
    }
}

END_NAMESPACE_REF_DEVICE_MODULE
