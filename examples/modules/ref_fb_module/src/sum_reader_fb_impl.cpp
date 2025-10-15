#include <ref_fb_module/sum_reader_fb_impl.h>
#include <opendaq/function_block_ptr.h>
#include <opendaq/data_descriptor_ptr.h>
#include <opendaq/event_packet_ptr.h>
#include <opendaq/signal_factory.h>
#include <opendaq/event_packet_params.h>
#include <opendaq/data_packet_ptr.h>
#include <opendaq/packet_factory.h>
#include <opendaq/sample_type_traits.h>
#include <opendaq/reader_factory.h>
#include <opendaq/reader_config_ptr.h>

BEGIN_NAMESPACE_REF_FB_MODULE

namespace SumReader
{

static bool descriptorNotNull(const DataDescriptorPtr& descriptor)
{
    return descriptor.assigned() && descriptor != NullDataDescriptor();
}

static void getDataDescriptors(const EventPacketPtr& eventPacket, DataDescriptorPtr& valueDesc, DataDescriptorPtr& domainDesc)
{
    if (eventPacket.getEventId() == event_packet_id::DATA_DESCRIPTOR_CHANGED)
    {
        valueDesc = eventPacket.getParameters().get(event_packet_param::DATA_DESCRIPTOR);
        domainDesc = eventPacket.getParameters().get(event_packet_param::DOMAIN_DATA_DESCRIPTOR);
    }
}

static bool getDomainDescriptor(const EventPacketPtr& eventPacket, DataDescriptorPtr& domainDesc)
{
    if (eventPacket.getEventId() == event_packet_id::DATA_DESCRIPTOR_CHANGED)
    {
        domainDesc = eventPacket.getParameters().get(event_packet_param::DOMAIN_DATA_DESCRIPTOR);
        return true;
    }
    return false;
}

SumReaderFbImpl::SumReaderFbImpl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId)
    : FunctionBlock(CreateType(), ctx, parent, localId)
{
    initComponentStatus();
    updateInputPorts();
    createSignals();
}

FunctionBlockTypePtr SumReaderFbImpl::CreateType()
{
    return FunctionBlockType("RefFBModuleSumReader", "Sum with reader", "Calculates equal-rate signal sum using multi reader");
}

std::string SumReaderFbImpl::getNextPortID() const
{
    int maxId = 0;
    for (const auto& port : connectedPorts)
    {
        std::string portId = port.getLocalId();
        auto pos = portId.find_last_of('_');
        int curId = std::stoi(portId.substr(pos + 1));
        maxId = curId > maxId ? curId : maxId;
    }

    return fmt::format("SumPort_{}", maxId + 1);
}

void SumReaderFbImpl::createSignals()
{
    sumSignal = createAndAddSignal("Sum");
    sumSignal.setName("Sum");
    sumDomainSignal = createAndAddSignal("SumDomain", nullptr, false);
    sumDomainSignal.setName("SumDomain");
    sumSignal.setDomainSignal(sumDomainSignal);
}

void SumReaderFbImpl::updateInputPorts()
{
    if (disconnectedPort.assigned() && disconnectedPort.getConnection().assigned())
    {
        connectedPorts.emplace_back(disconnectedPort);
        cachedDescriptors.insert(std::make_pair(disconnectedPort.getGlobalId(), NullDataDescriptor()));
        disconnectedPort.release();
    }

    for (auto it = connectedPorts.begin(); it != connectedPorts.end(); )
    {
        if (!it->getConnection().assigned())
        {
            cachedDescriptors.erase(it->getGlobalId().toStdString());
            this->inputPorts.removeItem(*it);
            it = connectedPorts.erase(it);
        }
        else
        {
            ++it;
        }
    }

    if (!disconnectedPort.assigned())
    {
        std::string id = getNextPortID();
        auto inputPort = createAndAddInputPort(id, PacketReadyNotification::Scheduler);
        inputPort.setListener(this->thisPtr<InputPortNotificationsPtr>());
        disconnectedPort = inputPort;
    }
}

// Reader must currently be rebuilt to add/remove input ports
void SumReaderFbImpl::updateReader()
{
    // Disposing the reader is necessary to release port ownership
    reader.dispose();
    auto builder = MultiReaderBuilder()
                 .setDomainReadType(SampleType::Int64)
                 .setValueReadType(SampleType::Float64)
                 .setAllowDifferentSamplingRates(false)
                 .setInputPortNotificationMethod(PacketReadyNotification::Scheduler);

    for (const auto& port : connectedPorts)
        builder.addInputPort(port);

    reader = builder.build();

    reader.setExternalListener(this->thisPtr<InputPortNotificationsPtr>());
    auto thisWeakRef = this->template getWeakRefInternal<IFunctionBlock>();
    reader.setOnDataAvailable([this, thisWeakRef = std::move(thisWeakRef)]
    {
        const auto thisFb = thisWeakRef.getRef();
        if (thisFb.assigned())
            this->onDataReceived();
    });
}

void SumReaderFbImpl::configure(const DataDescriptorPtr& domainDescriptor, const ListPtr<IDataDescriptor>& valueDescriptors)
{
    try
    {
        if (!domainDescriptor.assigned() || domainDescriptor == NullDataDescriptor())
        {
            throw std::runtime_error("Input domain descriptor is not set");
        }

        if (valueDescriptors.getCount() != connectedPorts.size())
        {
            throw std::runtime_error("Missing input value descriptors!");
        }
        
        UnitPtr unit = valueDescriptors[0].getUnit();

        double lowValue = 0;
        double highValue = 0;
        for (const auto& descriptor : valueDescriptors)
        {
            if (descriptor == NullDataDescriptor())
                throw std::runtime_error("An input value descriptor is not set!");

            if (descriptor.getUnit() != unit)
                throw std::runtime_error("Input value descriptor units must be equal!");

            auto range = descriptor.getValueRange();
            if (range.assigned())
            {
                lowValue += range.getLowValue().getFloatValue();
                highValue += range.getHighValue().getFloatValue();
            }
        }

        RangePtr range;
        if (std::fabs(lowValue - highValue) > 1e-9)
            range = Range(lowValue, highValue);
        else
            range = Range(-10, 10);

        sumDataDescriptor = DataDescriptorBuilder().setSampleType(SampleType::Float64).setUnit(unit).setValueRange(range).build();
        sumDomainDataDescriptor = domainDescriptor;

        sumSignal.setDescriptor(sumDataDescriptor);
        sumDomainSignal.setDescriptor(sumDomainDataDescriptor);

        setComponentStatus(ComponentStatus::Ok);
        reader.setActive(True);
    }
    catch (const std::exception& e)
    {
        setComponentStatusWithMessage(ComponentStatus::Warning, fmt::format("Failed to set descriptor for power signal: {}", e.what()));
        reader.setActive(False);
    }
}

void SumReaderFbImpl::onConnected(const InputPortPtr& inputPort)
{
    auto lock = this->getAcquisitionLock2();

    LOG_D("Sum Reader FB: Input port {} connected", inputPort.getLocalId())
    updateInputPorts();
    updateReader();
}

void SumReaderFbImpl::onDisconnected(const InputPortPtr& inputPort)
{
    auto lock = this->getAcquisitionLock2();

    LOG_D("Sum Reader FB: Input port {} disconnected", inputPort.getLocalId())
    updateInputPorts();
    updateReader();
}

void SumReaderFbImpl::onDataReceived()
{
    auto lock = this->getAcquisitionLock2();

    SizeT cnt = reader.getAvailableCount();

    auto numPorts = connectedPorts.size();
    std::vector<std::unique_ptr<double[]>> data;
    data.reserve(numPorts);

    for (size_t i = 0; i < numPorts; ++i)
        data.push_back(std::make_unique<double[]>(cnt));

    const MultiReaderStatusPtr status = reader.read(data.data(), &cnt);

    if (cnt > 0)
    {
        const auto sumDomainPacket = DataPacket(sumDomainSignal.getDescriptor(), cnt, status.getOffset());
        const auto sumValuePacket = DataPacketWithDomain(sumDomainPacket, sumSignal.getDescriptor(), cnt);
        double* sumValueData = static_cast<double*>(sumValuePacket.getRawData());
        std::fill_n(sumValueData, cnt, 0.0);

        for (const std::unique_ptr<double[]>& sigData : data)
        {
            const double* sigDataPtr = sigData.get();
            for (size_t i = 0; i < cnt; ++i)
                sumValueData[i] += sigDataPtr[i];
        }

        sumDomainSignal.sendPacket(sumDomainPacket);
        sumSignal.sendPacket(sumValuePacket);
    }

    if (status.getReadStatus() == ReadStatus::Event)
    {
        const auto eventPackets = status.getEventPackets();
        if (eventPackets.getCount() > 0)
        {
            DataDescriptorPtr domainDescriptor;
            ListPtr<IDataDescriptor> valueDescriptors = List<IDataDescriptor>();

            bool domainChanged = false;
            bool valueSigChanged = false;

            for (const auto& port : connectedPorts)
            {
                auto portGlobalId = port.getGlobalId();
                DataDescriptorPtr valueDescriptor;
                if (eventPackets.hasKey(portGlobalId))
                {
                    getDataDescriptors(eventPackets.get(portGlobalId), valueDescriptor, domainDescriptor);

                    if (descriptorNotNull(valueDescriptor))
                    {
                        valueSigChanged = true;
                        valueDescriptors.pushBack(valueDescriptor);
                    }

                    domainChanged |= descriptorNotNull(domainDescriptor);
                }

                if (!descriptorNotNull(valueDescriptor))
                    valueDescriptors.pushBack(cachedDescriptors[portGlobalId]);

            }
                
            getDomainDescriptor(status.getMainDescriptor(), domainDescriptor);

            if (valueSigChanged || domainChanged)
                configure(domainDescriptor, valueDescriptors);
        }

        if (!status.getValid())
        {
            LOG_D("Sum Reader FB: Attempting reader recovery")
            reader = MultiReaderFromExisting(reader, SampleType::Float64, SampleType::Int64);
        }
    }
}
}

END_NAMESPACE_REF_FB_MODULE
