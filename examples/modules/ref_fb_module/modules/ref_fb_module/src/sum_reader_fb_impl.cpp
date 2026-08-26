#include <ref_fb_module/sum_reader_fb_impl.h>
#include <coreobjects/unit_ptr.h>
#include <coretypes/event_wrapper.h>
#include <opendaq/data_descriptor_ptr.h>
#include <opendaq/data_packet_ptr.h>
#include <opendaq/multi_reader2_factory.h>
#include <opendaq/packet_factory.h>
#include <opendaq/signal_factory.h>

BEGIN_NAMESPACE_REF_FB_MODULE

namespace SumReader
{

SumReaderFbImpl::SumReaderFbImpl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId, const PropertyObjectPtr& /*config*/)
    : FunctionBlock(CreateType(), ctx, parent, localId)
{
    initComponentStatus();
    setComponentStatusWithMessage(ComponentStatus::Warning, "No signals connected!");

    initProperties();
    createDisconnectedPort();
    createSignals();
    createReader();
}

FunctionBlockTypePtr SumReaderFbImpl::CreateType()
{
    return FunctionBlockType("RefFBModuleSumReader", "Sum with reader", "Calculates equal-rate signal sum using multi reader 2");
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

void SumReaderFbImpl::initProperties()
{
    objPtr.addProperty(SelectionProperty("BadInputHandling", List<IString>("Exclude", "Deactivate"), 0));
    objPtr.getOnPropertyValueWrite("BadInputHandling") +=
        [this](PropertyObjectPtr&, PropertyValueEventArgsPtr&) { badInputHandlingChanged(); };
}

void SumReaderFbImpl::createSignals()
{
    sumSignal = createAndAddSignal("Sum");
    sumSignal.setName("Sum");
    sumDomainSignal = createAndAddSignal("SumDomain", nullptr, false);
    sumDomainSignal.setName("SumDomain");
    sumSignal.setDomainSignal(sumDomainSignal);
}

void SumReaderFbImpl::createDisconnectedPort()
{
    // The reader forces SameThread on its inputs, so the port's own mode does not matter
    disconnectedPort = createAndAddInputPort(getNextPortID(), PacketReadyNotification::SameThread);
}

// The reader owns the port notifications, so connectivity arrives through its events, not
// through the function block's own onConnected/onDisconnected overrides
void SumReaderFbImpl::createReader()
{
    reconfigureReaderLocked();

    const auto subscribe = [this](ErrCode (INTERFACE_FUNC IMultiReader2::*getter)(IEvent**), auto handler)
    {
        IEvent* eventIntf;
        checkErrorInfo((reader.getObject()->*getter)(&eventIntf));
        Event<InputPortPtr, EventArgsPtr<>> event{ObjectPtr<IEvent>::Adopt(eventIntf)};
        event += handler;
    };

    // Raw this + weak ref: a strong capture would cycle fb -> reader -> handler -> fb
    auto portHandler = [this, weakRef = this->getWeakRefInternal<IFunctionBlock>()](InputPortPtr&, EventArgsPtr<>&)
    {
        if (const auto fb = weakRef.getRef(); fb.assigned())
        {
            auto lock = this->getAcquisitionLock2();
            onPortEventLocked();
        }
    };
    subscribe(&IMultiReader2::getOnConnected, portHandler);
    subscribe(&IMultiReader2::getOnDisconnected, portHandler);

    subscribe(&IMultiReader2::getOnDataAvailable,
              [this, weakRef = this->getWeakRefInternal<IFunctionBlock>()](InputPortPtr&, EventArgsPtr<>&)
              {
                  if (const auto fb = weakRef.getRef(); fb.assigned())
                      onDataReceived();
              });
}

void SumReaderFbImpl::updateInputPortsLocked()
{
    if (disconnectedPort.getConnection().assigned())
    {
        // The spare got a signal: promote it and grow a fresh spare
        connectedPorts.emplace_back(disconnectedPort);
        createDisconnectedPort();
    }

    for (auto it = connectedPorts.begin(); it != connectedPorts.end();)
    {
        if (!it->getConnection().assigned())
        {
            this->inputPorts.removeItem(*it);
            it = connectedPorts.erase(it);
        }
        else
        {
            ++it;
        }
    }

    if (connectedPorts.empty())
        setComponentStatusWithMessage(ComponentStatus::Warning, "No signals connected!");
}

void SumReaderFbImpl::reconfigureReaderLocked()
{
    auto params = daq::MultiReader2Params();

    auto inputs = List<IComponent>();
    for (const auto& port : connectedPorts)
        inputs.pushBack(port);
    inputs.pushBack(disconnectedPort);
    checkErrorInfo(params->setInputs(inputs));
    checkErrorInfo(params->setValueReadType(SampleType::Float64));

    // With nothing connected all inputs stay used and the reader just waits for a connection
    if (!connectedPorts.empty())
    {
        checkErrorInfo(params->setMainInput(connectedPorts.front()));
        checkErrorInfo(params->setUnusedInputs(List<IComponent>(disconnectedPort)));
    }

    if (!reader.assigned())
    {
        reader = daq::MultiReader2(params);
    }
    else if (OPENDAQ_FAILED(reader->configure(params)))
    {
        daqClearErrorInfo();
        setComponentStatusWithMessage(ComponentStatus::Error, "Reader reconfiguration failed");
    }
}

void SumReaderFbImpl::badInputHandlingChanged()
{
    // Property write callbacks already run under the object lock
    excludeBadInputs = static_cast<Int>(objPtr.getPropertyValue("BadInputHandling")) == 0;
    reconfigureReaderLocked();
}

void SumReaderFbImpl::onPortEventLocked()
{
    updateInputPortsLocked();
    reconfigureReaderLocked();
}

void SumReaderFbImpl::onDataReceived()
{
    auto lock = this->getAcquisitionLock2();

    constexpr SizeT chunk = 1024;
    for (int pass = 0; pass < 64; pass++)
    {
        // One zeroed buffer per reader input, spare included: silent inputs contribute nothing
        const SizeT total = connectedPorts.size() + 1;
        std::vector<std::vector<double>> buffers(total, std::vector<double>(chunk, 0.0));
        std::vector<void*> raw(total);
        for (SizeT i = 0; i < total; i++)
            raw[i] = buffers[i].data();

        SizeT count = chunk;
        SizeT offset = 0;
        IMultiReader2Status* statusRaw;
        if (OPENDAQ_FAILED(reader->read(&statusRaw, raw.data(), &count, &offset)))
        {
            daqClearErrorInfo();
            return;
        }
        const auto status = ObjectPtr<IMultiReader2Status>::Adopt(statusRaw);

        MultiReader2StatusType type;
        checkErrorInfo(status->getStatus(&type));
        if (type == MultiReader2StatusType::Event)
        {
            handleEventLocked(status);
            continue;
        }
        if (count == 0)
            return;

        emitSumLocked(buffers, count, offset);
    }
}

void SumReaderFbImpl::handleEventLocked(const ObjectPtr<IMultiReader2Status>& status)
{
    IDict* errorsRaw;
    checkErrorInfo(status->getErrors(&errorsRaw));
    const auto errors = DictPtr<IString, IInteger>(ObjectPtr<IDict>::Adopt(errorsRaw));

    if (errors.getCount() > 0)
    {
        if (excludeBadInputs)
        {
            for (const auto& [inputId, error] : errors)
            {
                if (OPENDAQ_FAILED(reader->setUsed(inputId, False)))
                    daqClearErrorInfo();
            }
        }
        else if (OPENDAQ_FAILED(reader->setActive(False)))
        {
            daqClearErrorInfo();
        }

        std::string failing;
        for (const auto& [inputId, error] : errors)
            failing += (failing.empty() ? "" : ", ") + inputId.toStdString();
        setComponentStatusWithMessage(ComponentStatus::Warning, fmt::format("Inputs failing: {}", failing));
    }
    else
    {
        // A clean descriptor handshake is the recovery evidence: everything rejoins
        if (excludeBadInputs)
        {
            for (const auto& port : connectedPorts)
            {
                if (OPENDAQ_FAILED(reader->setUsed(port.getGlobalId(), True)))
                    daqClearErrorInfo();
            }
        }
        else if (OPENDAQ_FAILED(reader->setActive(True)))
        {
            daqClearErrorInfo();
        }

        configureOutputLocked(status);
    }

    if (OPENDAQ_FAILED(reader->commitEvent()))
        daqClearErrorInfo();
}

void SumReaderFbImpl::configureOutputLocked(const ObjectPtr<IMultiReader2Status>& status)
{
    IDataDescriptor* domainRaw;
    checkErrorInfo(status->getDomainDescriptor(&domainRaw));
    const auto domainDescriptor = DataDescriptorPtr::Adopt(domainRaw);
    if (!domainDescriptor.assigned())
    {
        setComponentStatusWithMessage(ComponentStatus::Warning, "The main input has no domain descriptor");
        return;
    }

    IDict* descriptorsRaw;
    checkErrorInfo(status->getDescriptors(&descriptorsRaw));
    const auto descriptors = DictPtr<IString, IDataDescriptor>(ObjectPtr<IDict>::Adopt(descriptorsRaw));

    // The sum range spans the summed input ranges; the unit follows the first input carrying one
    UnitPtr unit;
    double lowValue = 0;
    double highValue = 0;
    for (const auto& [inputId, descriptor] : descriptors)
    {
        if (!unit.assigned())
            unit = descriptor.getUnit();
        const auto range = descriptor.getValueRange();
        if (range.assigned())
        {
            lowValue += static_cast<double>(range.getLowValue().getFloatValue());
            highValue += static_cast<double>(range.getHighValue().getFloatValue());
        }
    }

    auto builder = DataDescriptorBuilder().setSampleType(SampleType::Float64);
    if (unit.assigned())
        builder.setUnit(unit);
    builder.setValueRange(std::fabs(highValue - lowValue) > 1e-9 ? Range(lowValue, highValue) : Range(-10, 10));
    sumDataDescriptor = builder.build();
    sumDomainDataDescriptor = domainDescriptor;

    sumSignal.setDescriptor(sumDataDescriptor);
    sumDomainSignal.setDescriptor(sumDomainDataDescriptor);

    setComponentStatus(ComponentStatus::Ok);
}

void SumReaderFbImpl::emitSumLocked(const std::vector<std::vector<double>>& buffers, SizeT count, SizeT offset)
{
    if (!sumDomainDataDescriptor.assigned())
        return;

    const auto sumDomainPacket = DataPacket(sumDomainDataDescriptor, count, offset);
    const auto sumValuePacket = DataPacketWithDomain(sumDomainPacket, sumDataDescriptor, count);
    const auto sumValueData = static_cast<double*>(sumValuePacket.getRawData());
    std::fill_n(sumValueData, count, 0.0);

    for (const auto& buffer : buffers)
    {
        for (SizeT i = 0; i < count; i++)
            sumValueData[i] += buffer[i];
    }

    sumDomainSignal.sendPacket(sumDomainPacket);
    sumSignal.sendPacket(sumValuePacket);
}
}

END_NAMESPACE_REF_FB_MODULE
