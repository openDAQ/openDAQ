#include <coreobjects/unit_factory.h>
#include <coretypes/filesystem.h>
#include <fmt/format.h>
#include <opendaq/custom_log.h>
#include <opendaq/device_domain_factory.h>
#include <opendaq/device_info_factory.h>
#include <opendaq/device_type_factory.h>
#include <opendaq/packet_factory.h>
#include <opendaq/sync_component_private_ptr.h>
#include <simulator_device_module/simulator_channel_impl.h>
#include <simulator_device_module/simulator_device_impl.h>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <utility>
#include <opendaq/thread_name.h>

BEGIN_NAMESPACE_SIMULATOR_DEVICE_MODULE

static StringPtr getEpoch()
{
    const std::time_t epochTime = std::chrono::system_clock::to_time_t(std::chrono::time_point<std::chrono::system_clock>{});

    char buf[48];
    strftime(buf, sizeof buf, "%Y-%m-%dT%H:%M:%SZ", gmtime(&epochTime));

    return { buf };
}

static RatioPtr getResolution()
{
    return Ratio(1, 1'000'000);
}

static UnitPtr getDomainUnit()
{
    return UnitBuilder().setName("second").setSymbol("s").setQuantity("time").build();
}

SimulatorDeviceImpl::SimulatorDeviceImpl(const PropertyObjectPtr& config, const ContextPtr& ctx, const ComponentPtr& parent, const DeviceInfoPtr& info)
    : Device(ctx, parent, fmt::format("{}_{}", info.getManufacturer(), info.getSerialNumber()), nullptr, info.getName())
    , acqLoopTime(20)
    , stopAcq(false)
    , ticksSinceEpochToDeviceStart(0)
    , samplesGenerated(0)
    , sampleRate(1000)
    , deltaTicks(1000)
    , resolution(getResolution())
    , domainUnit(getDomainUnit())
    , epoch(getEpoch()) // resolution.denominator / sampleRate; Must be integer-divisible
{
    this->loggerComponent = this->context.getLogger().getOrAddComponent(SIMULATOR_MODULE_NAME);
    this->deviceInfo = info;
    
    initProperties();
    initClock();
    createTimeSignal();
    createChannels(config);
    updateAcqLoopTime();

    acqThread = std::thread{ &SimulatorDeviceImpl::acqLoop, this };
}

SimulatorDeviceImpl::~SimulatorDeviceImpl()
{
    {
        auto lock = this->getAcquisitionLock();
        stopAcq = true;
    }
    cv.notify_one();

    acqThread.join();
}

DeviceInfoPtr SimulatorDeviceImpl::CreateDeviceInfo(const DictPtr<IString, IBaseObject>& moduleOptions)
{
    const StringPtr name = moduleOptions.get("Name");
    const StringPtr manufacturer = moduleOptions.get("Manufacturer");
    const StringPtr serialNumber = moduleOptions.get("SerialNumber");

    auto devInfo = DeviceInfoWithChanegableFields({"userName", "location"});
    devInfo.setName(name);
    devInfo.setConnectionString(fmt::format("daq.simulator://{}_{}", manufacturer, serialNumber));
    devInfo.setManufacturer(manufacturer);
    devInfo.setModel("Simulator device");
    devInfo.setSerialNumber(serialNumber);
    devInfo.setDeviceType(CreateType());

    return devInfo;
}

DeviceTypePtr SimulatorDeviceImpl::CreateType()
{
    const auto defaultConfig = PropertyObject();
    defaultConfig.addProperty(IntProperty("NumberOfChannels", 8));
    defaultConfig.addProperty(BoolProperty("EnableLogging", False));
    defaultConfig.addProperty(StringProperty("LoggingPath", "simulator_device_simulator.log"));

    return DeviceType("SimulatorDevice",
                      "Simulator device",
                      "openDAQ signal generator simulator",
                      "daq.simulator",
                      defaultConfig);
}

uint64_t SimulatorDeviceImpl::onGetTicksSinceOrigin()
{
    return samplesGenerated * deltaTicks + ticksSinceEpochToDeviceStart;
}

bool SimulatorDeviceImpl::allowAddDevicesFromModules()
{
    return true;
}

bool SimulatorDeviceImpl::allowAddFunctionBlocksFromModules()
{
    return true;
}

std::set<OperationModeType> SimulatorDeviceImpl::onGetAvailableOperationModes()
{
    return {OperationModeType::Idle, OperationModeType::Operation};
}

void SimulatorDeviceImpl::onOperationModeChanged(OperationModeType modeType)
{
    timeSignal.setActive(modeType != OperationModeType::Idle);
}

void SimulatorDeviceImpl::initProperties()
{
    // Coerce for SR and Resolution.den to be integer divisible.
    const auto sampleRateProp =
        IntPropertyBuilder("SampleRate", 1000).setUnit(Unit("Hz")).setMinValue(1).setMaxValue(1'000'000).build();

    objPtr.addProperty(sampleRateProp);
    objPtr.getOnPropertyValueWrite("SampleRate") +=
        [this](PropertyObjectPtr&, const PropertyValueEventArgsPtr& args) { updateSampleRate(args.getValue()); };

    const auto acqLoopTimeProp =
        IntPropertyBuilder("AcquisitionLoopTime", 20).setUnit(Unit("ms")).setMinValue(10).setMaxValue(1000).build();

    objPtr.addProperty(acqLoopTimeProp);
    objPtr.getOnPropertyValueWrite("AcquisitionLoopTime") +=
        [this](PropertyObjectPtr&, PropertyValueEventArgsPtr&) { updateAcqLoopTime(); };
}

void SimulatorDeviceImpl::initClock()
{
    resetClock();
    auto referenceDomainInfo = ReferenceDomainInfoBuilder().setReferenceDomainId(this->localId).setReferenceDomainOffset(0).build();
    this->setDeviceDomain(DeviceDomain(resolution, epoch, domainUnit, referenceDomainInfo));
}

void SimulatorDeviceImpl::resetClock()
{
    auto startAbsTime = std::chrono::system_clock::now();
    ticksSinceEpochToDeviceStart = std::chrono::duration_cast<std::chrono::microseconds>(startAbsTime.time_since_epoch()).count();
    samplesGenerated = 0;
}

void SimulatorDeviceImpl::createTimeSignal()
{
    timeSignal = Signal(context, signals, "DeviceTime");
    timeSignal.getTags().asPtr<ITagsPrivate>(true).add("DeviceDomain");
    signals.addItem(timeSignal);

    updateTimeSignal();
}

void SimulatorDeviceImpl::updateTimeSignal() const
{
    auto domainInfo = objPtr.asPtr<IDevice>().getDomain();
    auto referenceDomainInfo = domainInfo.getReferenceDomainInfo();

    const auto timeDescriptor = DataDescriptorBuilder()
                                .setSampleType(SampleType::Int64)
                                .setUnit(domainInfo.getUnit())
                                .setTickResolution(resolution)
                                .setRule(LinearDataRule(deltaTicks, 0))
                                .setOrigin(epoch)
                                .setReferenceDomainInfo(domainInfo.getReferenceDomainInfo())
                                .build();

    timeSignal.setDescriptor(timeDescriptor);
}

void SimulatorDeviceImpl::createChannels(const PropertyObjectPtr& config)
{
    // Analog input folder
    aiFolder = IoFolder(this->context, ioFolder, "AI");
    ioFolder.addItem(aiFolder);

    int numberOfChannels = config.getPropertyValue("NumberOfChannels");
    for (auto i = 1; i <= numberOfChannels; i++)
    {
        auto chLocalId = fmt::format("AI_{}", i);
        auto ch = createWithImplementation<IChannel, SimulatorChannelImpl>(context, aiFolder, chLocalId);
        ch.getInputPorts(search::Any())[0].connect(timeSignal);
        aiFolder.addItem(std::move(ch));
    }
}

void SimulatorDeviceImpl::updateAcqLoopTime()
{
    Int loopTime = objPtr.getPropertyValue("AcquisitionLoopTime");
    LOG_I("Properties: AcquisitionLoopTime {}ms", loopTime)

    this->acqLoopTime = static_cast<size_t>(loopTime);
}

void SimulatorDeviceImpl::updateSampleRate(uint64_t newSampleRate)
{
    this->sampleRate = newSampleRate;
    this->deltaTicks = resolution.getDenominator() / sampleRate;
    resetClock();
    updateTimeSignal();
    LOG_I("Properties: SampleRate {}", sampleRate)
}

void SimulatorDeviceImpl::acqLoop()
{
    daqNameThread("SimulatorDevice");

    using namespace std::chrono_literals;
    using milli = std::chrono::milliseconds;
    const auto loopTime = milli(acqLoopTime);
    auto prevLoopTime = std::chrono::steady_clock::now();

    auto lock = getUniqueLock();

    while (!stopAcq)
    {
        const auto time = std::chrono::high_resolution_clock::now();
        const auto loopDuration = std::chrono::duration_cast<milli>(time - prevLoopTime);
        const auto waitTime = loopDuration.count() >= loopTime.count() ? milli(0) : milli(loopTime.count() - loopDuration.count());

        cv.wait_for(lock, waitTime);
        if (!stopAcq)
        {
            auto curLoopTime = std::chrono::steady_clock::now();
            auto newSampleCount = getNewSampleCount(curLoopTime, prevLoopTime);
            sendTimePacket(newSampleCount);
            prevLoopTime = curLoopTime;
        }
    }
}

void SimulatorDeviceImpl::sendTimePacket(uint64_t newSampleCount)
{
    // TODO: Fixed packet size
    const uint64_t packetTime = samplesGenerated * deltaTicks + ticksSinceEpochToDeviceStart;
    timeSignal.sendPacket(std::move(DataPacket(timeSignal.getDescriptor(), newSampleCount, packetTime)));
    samplesGenerated += newSampleCount;
}

uint64_t SimulatorDeviceImpl::getNewSampleCount(const std::chrono::steady_clock::time_point& curLoopTime,
                                                const std::chrono::steady_clock::time_point& prevLoopTime) const
{
    auto microsecondsSinceLastLoop = std::chrono::duration_cast<std::chrono::microseconds>(curLoopTime - prevLoopTime);
    double resolutionDenominator = static_cast<double>(resolution.getDenominator());
    double newSamplesDouble = static_cast<double>(microsecondsSinceLastLoop.count()) / resolutionDenominator * static_cast<double>(sampleRate);
    return static_cast<uint64_t>(std::trunc(newSamplesDouble));
}

END_NAMESPACE_SIMULATOR_DEVICE_MODULE
