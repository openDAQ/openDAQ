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
#include <iomanip>
#include <utility>
#include <opendaq/thread_name.h>
#include <coreobjects/callable_info_factory.h>

BEGIN_NAMESPACE_SIMULATOR_DEVICE_MODULE

SimulatorDeviceImpl::SimulatorDeviceImpl(const PropertyObjectPtr& config, const ContextPtr& ctx, const ComponentPtr& parent, const DeviceInfoPtr& info)
    : Device(ctx, parent, fmt::format("{}_{}", info.getManufacturer(), info.getSerialNumber()), nullptr, info.getName())
    , acqLoopTime(20)
    , stopAcq(false)
    , samplesGenerated(0)
    , packetOffset(0)
    , sampleRate(10000)
    , deltaTicks(100)
    , offset(0)
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
    createChannels(config.getPropertyValue("NumberOfChannels"));

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

    auto connectionString = fmt::format("daq.simulator://{}_{}", manufacturer, serialNumber);
    auto devInfo = DeviceInfo(connectionString);
    devInfo.setName(name);
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

    return DeviceType("SimulatorDevice",
                      "Simulator device",
                      "openDAQ signal generator simulator",
                      "daq.simulator",
                      defaultConfig);
}

uint64_t SimulatorDeviceImpl::onGetTicksSinceOrigin()
{
    auto lock = getAcquisitionLock2();
    return packetOffset - deltaTicks;
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
    const auto sampleRateProp =
        IntPropertyBuilder("SampleRate", sampleRate).setUnit(Unit("Hz")).setMinValue(100).setMaxValue(1'000'000).build();

    const auto acqLoopTimeProp =
        IntPropertyBuilder("AcquisitionLoopTime", 20).setUnit(Unit("ms")).setMinValue(10).setMaxValue(1000).build();

    const auto offsetProp = IntPropertyBuilder("Offset", 0).setVisible(true).setUnit(Unit("s")).build();

    objPtr.addProperty(sampleRateProp);
    objPtr.addProperty(acqLoopTimeProp);
    objPtr.addProperty(offsetProp);
    objPtr.addProperty(FunctionProperty("RecalculateDividerLCM", ProcedureInfo(), false));
    
    auto dividerChangedProc = Procedure([this] { this->calculateDividerLcm(); });
    objPtr.setPropertyValue("RecalculateDividerLCM", dividerChangedProc);

    objPtr.getOnAnyPropertyValueWrite() += event(&SimulatorDeviceImpl::propertyWriteCallback);
}

void SimulatorDeviceImpl::createChannels(uint64_t numberOfChannels)
{
    aiFolder = IoFolder(this->context, ioFolder, "AI");
    ioFolder.addItem(aiFolder);

    for (uint64_t i = 1; i <= numberOfChannels; i++)
    {
        auto chLocalId = fmt::format("AI_{}", i);
        auto ch = createWithImplementation<IChannel, SimulatorChannelImpl>(context, aiFolder, chLocalId, objPtr);
        auto inputPort = ch.asPtr<IChannel>().getInputPorts(search::Any())[0];
        inputPort.connect(timeSignal);
        aiFolder.addItem(std::move(ch));
    }
}

void SimulatorDeviceImpl::createTimeSignal()
{
    timeSignal = Signal(context, signals, "DeviceTime");
    timeSignal.getTags().asPtr<ITagsPrivate>(true).add("DeviceTime");
    signals.addItem(timeSignal);

    updateTimeSignal();
}

void SimulatorDeviceImpl::initClock()
{
    auto startAbsTime = std::chrono::system_clock::now();
    packetOffset = std::chrono::duration_cast<std::chrono::microseconds>(startAbsTime.time_since_epoch()).count();

    auto referenceDomainInfo = ReferenceDomainInfoBuilder().setReferenceDomainId(this->localId).setReferenceDomainOffset(0).build();
    this->setDeviceDomain(DeviceDomain(resolution, epoch, domainUnit, referenceDomainInfo));
}

void SimulatorDeviceImpl::propertyWriteCallback(PropertyObjectPtr&, PropertyValueEventArgsPtr& args)
{
    auto propName = args.getProperty().getName();
    auto value = args.getValue();
    if (propName == "SampleRate")
    {
        // Coerce Resolution.den to be integer divisible by SR and Rule.delta
        updateSampleRate(value);
        args.setValue(this->sampleRate);
    }
    else if (propName == "AcquisitionLoopTime")
    {
        updateAcqLoopTime(value);
    }
    else if (propName == "Offset")
    {
        offsetChanged(value);
    }
}

void SimulatorDeviceImpl::updateAcqLoopTime(uint64_t acqLoopTime)
{
    this->acqLoopTime = acqLoopTime;
}

void SimulatorDeviceImpl::offsetChanged(uint64_t offset)
{
    this->offset = offset * resolution.getDenominator();
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

void SimulatorDeviceImpl::calculateDividerLcm()
{
    dividerLcm = 1;
    for (const auto& channel : aiFolder.getItems())
    {
        uint64_t divider = channel.getPropertyValue("SampleRate");
        dividerLcm = std::lcm(dividerLcm, divider);
    }
}

bool SimulatorDeviceImpl::checkAndSetSR(uint64_t dt, uint64_t den)
{
    if (den % dt || den % (den / dt))
        return false;
    
    this->deltaTicks = dt;
    this->sampleRate = den / dt;
    return true;
}

void SimulatorDeviceImpl::updateSampleRate(uint64_t newSampleRate)
{
    // Round to nearest acceptable sampling rate.
    // Resolution.den must be divisible by the sampling rate and deltaTicks.

    const auto den = resolution.getDenominator();
    const auto dt = den / newSampleRate + (den % newSampleRate != 0); // Round up

    for (int i = 0;;++i)
    {
        if (dt - i > 0 && checkAndSetSR(dt - i, den))
            break;

        if (i != 0 && checkAndSetSR(dt + i, den))
            break;
    }

    updateTimeSignal();
    calculateDividerLcm();
}

uint64_t SimulatorDeviceImpl::sendTimePackets(uint64_t newSampleCount)
{
    uint64_t sampleRemainder = newSampleCount % dividerLcm;
    uint64_t actualPacketSize = newSampleCount - sampleRemainder;

    if (actualPacketSize > 0)
        timeSignal.sendPacket(std::move(DataPacket(timeSignal.getDescriptor(), actualPacketSize, packetOffset)));

    packetOffset += actualPacketSize * deltaTicks;
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

void SimulatorDeviceImpl::acqLoop()
{
    daqNameThread("SimulatorDevice");

    using namespace std::chrono_literals;
    using milli = std::chrono::milliseconds;

    auto lock = getUniqueLock();

    auto loopTime = milli(acqLoopTime);
    auto prevLoopTime = std::chrono::steady_clock::now();
    uint64_t remainingSamples = 0;

    while (!stopAcq)
    {
        cv.wait_until(lock, prevLoopTime + loopTime);
        if (!stopAcq)
        {
            const auto time = std::chrono::steady_clock::now();
            auto newSampleCount = getNewSampleCount(time, prevLoopTime, remainingSamples);
            remainingSamples = sendTimePackets(newSampleCount);

            prevLoopTime = time;
            loopTime = milli(acqLoopTime);
        }
    }
}

StringPtr SimulatorDeviceImpl::getEpoch()
{
    const std::time_t epochTime = std::chrono::system_clock::to_time_t(std::chrono::time_point<std::chrono::system_clock>{});

    char buf[48];
    strftime(buf, sizeof buf, "%Y-%m-%dT%H:%M:%SZ", gmtime(&epochTime));

    return { buf };
}

RatioPtr SimulatorDeviceImpl::getResolution()
{
    // Numerator is set to 1 and is optimized out of sample rate calculations.
    return Ratio(1, 1'000'000);
}

UnitPtr SimulatorDeviceImpl::getDomainUnit()
{
    return UnitBuilder().setName("seconds").setSymbol("s").setQuantity("time").build();
}

END_NAMESPACE_SIMULATOR_DEVICE_MODULE
