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

static uint64_t calculateNearestDivider(uint64_t deviceSampleRate, uint64_t previousDivider)
{
    if (deviceSampleRate % previousDivider == 0)
        return previousDivider;

    for (uint64_t delta = 0;; ++delta)
    {
        if (deviceSampleRate % (previousDivider + delta) == 0)
            return previousDivider + delta;

        if (deviceSampleRate % (previousDivider - delta) == 0)
            return previousDivider - delta;
    }
}

SimulatorDeviceImpl::SimulatorDeviceImpl(const PropertyObjectPtr& config, const ContextPtr& ctx, const ComponentPtr& parent, const DeviceInfoPtr& info)
    : Device(ctx, parent, fmt::format("{}_{}", info.getManufacturer(), info.getSerialNumber()), nullptr, info.getName())
    , acqLoopTime(20)
    , stopAcq(false)
    , ticksSinceEpochToDeviceStart(0)
    , samplesGenerated(0)
    , sampleRate(10000)
    , deltaTicks(100) // resolution.denominator / sampleRate; Must be integer-divisible
    , offset(0) // resolution.denominator / sampleRate; Must be integer-divisible
    , dividerLcm(1)
    , resolution(getResolution())
    , domainUnit(getDomainUnit())
    , epoch(getEpoch())
{
    this->loggerComponent = this->context.getLogger().getOrAddComponent(SIMULATOR_MODULE_NAME);
    this->deviceInfo = info;
    
    initProperties();
    initClock();
    createTimeSignal();
    createChannels(config);
    updateDividers();
    updateAcqLoopTime();

    acqThread = std::thread{ &SimulatorDeviceImpl::acqLoop, this };
}

SimulatorDeviceImpl::~SimulatorDeviceImpl()
{
    {
        auto lock = this->getAcquisitionLock2();
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

void SimulatorDeviceImpl::onSRDivChanged(const ChannelPtr& channel, Int newDivider)
{
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

// TODO: Add internal flag that stops sending data on TimeSignal
void SimulatorDeviceImpl::onOperationModeChanged(OperationModeType modeType)
{
    timeSignal.setActive(modeType != OperationModeType::Idle);
}

// TODO: Migrate to onAny value write
// TODO: Add property descriptions
void SimulatorDeviceImpl::initProperties()
{
    // Device setup props
    // Coerce for SR and Resolution.den to be integer divisible.
    const auto sampleRateProp =
        IntPropertyBuilder("SampleRate", sampleRate).setUnit(Unit("Hz")).setMinValue(1).setMaxValue(1'000'000).build();

    const auto acqLoopTimeProp =
        IntPropertyBuilder("AcquisitionLoopTime", 20).setUnit(Unit("ms")).setMinValue(10).setMaxValue(1000).build();

    const auto useFixedPacketSizeProp = BoolProperty("UseFixedPacketSize", false);
    const auto fixedPacketSizeProp = IntProperty("FixedPacketSize", 500, EvalValue("$UseFixedPacketSize"));

    // Global value signal props
    const auto clientSideScalingProp = BoolProperty("ClientSideScaling", False);

    // Global time signal props
    const auto offsetProp = IntPropertyBuilder("Offset", 0)
                            .setVisible(true)
                            .setMinValue(0)
                            .build();

    objPtr.addProperty(sampleRateProp);
    objPtr.getOnPropertyValueWrite("SampleRate") +=
        [this](PropertyObjectPtr&, const PropertyValueEventArgsPtr& args)
        {
            updateSampleRate(args.getValue());
            args.setValue(this->sampleRate);
        };


    objPtr.addProperty(acqLoopTimeProp);
    objPtr.getOnPropertyValueWrite("AcquisitionLoopTime") +=
        [this](PropertyObjectPtr&, PropertyValueEventArgsPtr&) { updateAcqLoopTime(); };

    // TODO: Implement and onChange events
    objPtr.addProperty(useFixedPacketSizeProp);
    objPtr.addProperty(fixedPacketSizeProp);

    objPtr.addProperty(clientSideScalingProp);
    objPtr.getOnPropertyValueWrite("ClientSideScaling") += [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { signalTypeChanged(); };

    objPtr.addProperty(offsetProp);
    objPtr.getOnPropertyValueWrite("Offset") +=
        [this](PropertyObjectPtr&, PropertyValueEventArgsPtr& args) { signalTypeChanged(); };
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
    timeSignal.getTags().asPtr<ITagsPrivate>(true).add("DeviceTime");
    signals.addItem(timeSignal);

    updateTimeSignal();
}

void SimulatorDeviceImpl::updateTimeSignal() const
{
    auto domainInfo = objPtr.asPtr<IDevice>().getDomain();
    auto referenceDomainInfo = domainInfo.getReferenceDomainInfo();

    const auto timeDescriptorBuilder = DataDescriptorBuilder()
                                       .setSampleType(SampleType::Int64)
                                       .setUnit(domainInfo.getUnit())
                                       .setTickResolution(resolution)
                                       .setOrigin(epoch)
                                       .setReferenceDomainInfo(domainInfo.getReferenceDomainInfo())
                                       .setRule(LinearDataRule(deltaTicks, offset));

    timeSignal.setDescriptor(timeDescriptorBuilder.build());
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
        auto inputPort = ch.asPtr<IChannel>().getInputPorts(search::Any())[0];
        inputPort.connect(timeSignal);
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
    // Round to nearest acceptable sampling rate. Resolution.den must be divisible by the sampling rate.
    // TODO: Round down instead of up
    this->deltaTicks = resolution.getDenominator() / newSampleRate;
    this->sampleRate = resolution.getDenominator() / deltaTicks;

    resetClock();
    updateDividers();
    updateTimeSignal();
}

void SimulatorDeviceImpl::updateDividers()
{
    for (const auto& channel : aiFolder.getItems())
    {
        auto availableDividers = calculateAvailableSampleRateDividers(sampleRate);
        channel.asPtr<IPropertyObjectInternal>().setProtectedPropertyValueNoLock("AvailableSRDividers", availableDividers);
    }

    calculateDividerLcm();
}

void SimulatorDeviceImpl::calculateDividerLcm()
{
    dividerLcm = 1;
    for (const auto& channel : aiFolder.getItems())
    {
        uint64_t divider = channel.getPropertySelectionValue("SampleRateDivider");
        dividerLcm = std::lcm(dividerLcm, divider);
    }
}

void SimulatorDeviceImpl::acqLoop()
{
    daqNameThread("SimulatorDevice");

    using namespace std::chrono_literals;
    using milli = std::chrono::milliseconds;
    const auto loopTime = milli(acqLoopTime);
    auto prevLoopTime = std::chrono::steady_clock::now();
    uint64_t remainingSamples = 0;

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
            auto newSampleCount = getNewSampleCount(curLoopTime, prevLoopTime, remainingSamples);
            remainingSamples = sendTimePackets(newSampleCount);
            prevLoopTime = curLoopTime;
        }
    }
}

void SimulatorDeviceImpl::signalTypeChanged()
{
    //clientSideScaling = objPtr.getPropertyValue("ClientSideScaling");
    offset = objPtr.getPropertyValue("Offset");
    updateTimeSignal();
}

uint64_t SimulatorDeviceImpl::sendTimePackets(uint64_t newSampleCount)
{
    // TODO: Fixed packet size
    const uint64_t packetTime = samplesGenerated * deltaTicks + ticksSinceEpochToDeviceStart;
    uint64_t sampleRemainder = newSampleCount % dividerLcm;
    uint64_t actualPacketSize = newSampleCount - sampleRemainder;

    if (actualPacketSize > 0)
    {
        timeSignal.sendPacket(std::move(DataPacket(timeSignal.getDescriptor(), actualPacketSize, packetTime)));
    }

    samplesGenerated += actualPacketSize;
    return sampleRemainder;
}

uint64_t SimulatorDeviceImpl::getNewSampleCount(const std::chrono::steady_clock::time_point& curLoopTime,
                                                const std::chrono::steady_clock::time_point& prevLoopTime,
                                                uint64_t remainingSamples) const
{
    auto microsecondsSinceLastLoop = std::chrono::duration_cast<std::chrono::microseconds>(curLoopTime - prevLoopTime);
    double resolutionDenominator = static_cast<double>(resolution.getDenominator());
    double newSamplesDouble = static_cast<double>(microsecondsSinceLastLoop.count()) / resolutionDenominator * static_cast<double>(sampleRate);
    return static_cast<uint64_t>(std::trunc(newSamplesDouble)) + remainingSamples;
}

END_NAMESPACE_SIMULATOR_DEVICE_MODULE
