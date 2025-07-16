#include <opendaq/event_packet_ids.h>
#include <opendaq/event_packet_params.h>
#include <opendaq/packet_factory.h>
#include <opendaq/reusable_data_packet_ptr.h>
#include <licensing_module/passthrough_fb_impl.h>

BEGIN_NAMESPACE_LICENSING_MODULE


const daq::StringPtr strRequiredLicense("passthrough");

PassthroughFbImpl::PassthroughFbImpl(const ContextPtr& ctx,
                                     const ComponentPtr& parent,
                                     const StringPtr& localId,
                                     LicenseCheckerPtr licenseComponent)
    : FunctionBlock(CreateType(), ctx, parent, localId)
    , mLicenseComponent(licenseComponent)
    , mIsLicenseCheckedOut(false)
{
    initComponentStatus();
    createInputPorts();
    createSignals();
}

PassthroughFbImpl::~PassthroughFbImpl()
{
    if (mIsLicenseCheckedOut)
    {
        if (OPENDAQ_SUCCEEDED(mLicenseComponent->checkIn(strRequiredLicense, 1)))
        {
            LOG_I("~PassthroughFbImpl({}): The previously checked out \"{}\" license was checked in again.", localId, strRequiredLicense);
        }
        else
        {
            LOG_C("~PassthroughFbImpl({}): The previously checked out \"{}\" license could not be checked in again!", localId, strRequiredLicense);
        }
    }
}

FunctionBlockTypePtr PassthroughFbImpl::CreateType()
{
    return FunctionBlockType(TypeID, "Passthrough", "Passes the input signal data through if the 'passthrough' license is available.");
}
void PassthroughFbImpl::createInputPorts()
{
    _inputPort = createAndAddInputPort("Input", PacketReadyNotification::SchedulerQueueWasEmpty);
}
void PassthroughFbImpl::createSignals()
{
    _outputSignal = createAndAddSignal("output");
    _outputDomainSignal = createAndAddSignal("output_domain", nullptr, false);

    _outputSignal.setDomainSignal(_outputDomainSignal);
}

void PassthroughFbImpl::onConnected(const InputPortPtr& port)
{
    if (!mIsLicenseCheckedOut)
    {
        if (OPENDAQ_SUCCEEDED(mLicenseComponent->checkOut(strRequiredLicense, 1)))
        {
            LOG_I("onConnected({}): Successfully checked out \"{}\" license.", localId, strRequiredLicense);
            mIsLicenseCheckedOut = true;
            setComponentStatus(ComponentStatus::Ok);
        }
        else
        {
            LOG_W("onConnected({}): Failed to check out required \"{}\" license!", localId, strRequiredLicense);
            setComponentStatusWithMessage(ComponentStatus::Warning, "Required license is missing!");
        }
    }
}

void PassthroughFbImpl::onDisconnected(const InputPortPtr& port)
{
    if (mIsLicenseCheckedOut)
    {
        if (OPENDAQ_SUCCEEDED(mLicenseComponent->checkIn(strRequiredLicense, 1)))
        {
            mIsLicenseCheckedOut = false;
            LOG_I("onDisconnected({}): Successfully checked in \"{}\" license.", localId, strRequiredLicense);
        }
        else
        {
            LOG_I("onDisconnected({}): Failed to check in \"{}\" license.", localId, strRequiredLicense);
        }
    }
}

void PassthroughFbImpl::onPacketReceived(const InputPortPtr& port)
{
    const auto connection = port.getConnection();
    if (!connection.assigned())
        return;

    auto outQueue = List<IPacket>();
    auto outDomainQueue = List<IPacket>();

    auto lock = this->getAcquisitionLock();

    auto packet = connection.dequeue();
    while (packet.assigned())
    {
        switch (packet.getType())
        {
            case PacketType::Event:
                processEventPacket(packet);
                break;
            case PacketType::Data:
                processDataPacket(std::move(packet), outQueue, outDomainQueue);
                break;
        }

        packet = connection.dequeue();
    }

    //You can safely ignore the incorrect C26110 warning in Visual Studio ("Caller failing to hold lock 'lock' before calling function '...'").
    //Please also note that a "#pragma warning ignore" does not help to remove the squiggles in Visual Studio.
    if (outQueue.getCount() > 0)
    {
        _outputSignal.sendPackets(std::move(outQueue));
        _outputDomainSignal.sendPackets(std::move(outDomainQueue));
    }
}
void PassthroughFbImpl::processEventPacket(const EventPacketPtr& packet)
{
    if (packet.getEventId() == event_packet_id::DATA_DESCRIPTOR_CHANGED)
    {
        if (!mIsLicenseCheckedOut)
        {
            // Must NOT log any messages here because this function is called for every packet !!!
            //LOG_W("processEventPacket({}): Required \"{}\" license could not be checked out!", localId, strRequiredLicense);
            return;
        }

        const DataDescriptorPtr inputDataDescriptor = packet.getParameters().get(event_packet_param::DATA_DESCRIPTOR);
        const DataDescriptorPtr inputDomainDataDescriptor = packet.getParameters().get(event_packet_param::DOMAIN_DATA_DESCRIPTOR);

        if (!inputDataDescriptor.assigned())
        {
            //Setting the component status to Error (but only once otherwise we will be flooded by error log messages)
            const auto oldStatus = statusContainer.getStatus("ComponentStatus");
            const auto oldMessage = statusContainer.getStatusMessage("ComponentStatus");
            const auto newMessage = "Failed to set descriptor for output signal: Input data descriptor has not been provided!";
            if (!(oldStatus == ComponentStatus::Error) || oldMessage != newMessage)  //!= operator is currently not available for ComponentStatus (see: https://blueberrydaq.atlassian.net/servicedesk/customer/portal/34/SHS-175)
                setComponentStatusWithMessage(ComponentStatus::Error, newMessage);

            _outputSignal.setDescriptor(nullptr);
            return;
        }

        if (!inputDomainDataDescriptor.assigned())
        {
            const auto oldStatus = statusContainer.getStatus("ComponentStatus");
            const auto oldMessage = statusContainer.getStatusMessage("ComponentStatus");
            const auto newMessage = "Failed to set descriptor for output signal: Input domain data descriptor has not been provided!";
            if (!(oldStatus == ComponentStatus::Error) || oldMessage != newMessage)
                setComponentStatusWithMessage(ComponentStatus::Error, newMessage);

            _outputSignal.setDescriptor(nullptr);
            return;
        }

        const auto inputName = inputDataDescriptor.getName();
        auto outputDataDescriptorBuilder = DataDescriptorBuilderCopy(inputDataDescriptor)
            .setName(inputName.toStdString() + "/passthrough");

        const auto outputDataDescriptor = outputDataDescriptorBuilder.build();

        _outputSignal.setDescriptor(outputDataDescriptor);
        _outputDomainSignal.setDescriptor(inputDomainDataDescriptor);

        setComponentStatus(ComponentStatus::Ok);
    }
}
void PassthroughFbImpl::processDataPacket(DataPacketPtr&& packet, ListPtr<IPacket>& outQueue, ListPtr<IPacket>& outDomainQueue)
{
    if (!mIsLicenseCheckedOut)
        return;

    DataPacketPtr outputPacket;
    DataPacketPtr outputDomainPacket = packet.getDomainPacket();

    const auto outputDataDescriptor = _outputSignal.getDescriptor();
    const auto reusablePacket = packet.asPtrOrNull<IReusableDataPacket>(true);
    if (reusablePacket.assigned() && packet.getRefCount() == 1 &&
        reusablePacket.reuse(outputDataDescriptor, std::numeric_limits<SizeT>::max(), nullptr, nullptr, false))
    {
        outputPacket = std::move(packet);
    }
    else
    {
        const auto sampleCount = packet.getSampleCount();

        outputPacket = DataPacketWithDomain(outputDomainPacket, outputDataDescriptor, sampleCount);

        auto ptrDestData = outputPacket.getData();
        const auto ptrSourceData = packet.getData();
        const auto byteSize = packet.getDataSize();

        std::memcpy(ptrDestData, ptrSourceData, byteSize);
    }

    outQueue.pushBack(std::move(outputPacket));
    outDomainQueue.pushBack(std::move(outputDomainPacket));
}

END_NAMESPACE_LICENSING_MODULE
