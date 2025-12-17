#include <opendaq/packet_reader_impl.h>
#include <coretypes/list_factory.h>
#include <coreobjects/property_object_factory.h>
#include <coreobjects/ownable_ptr.h>
#include <opendaq/connection_internal.h>

BEGIN_NAMESPACE_OPENDAQ

PacketReaderImpl::PacketReaderImpl(const SignalPtr& signal)
{
    if (!signal.assigned())
        DAQ_THROW_EXCEPTION(ArgumentNullException, "Signal must not be null.");

    port = InputPort(signal.getContext(), nullptr, "readsignal");
    this->internalAddRef();
    try
    {
        port.setListener(this->thisPtr<InputPortNotificationsPtr>());
        port.setNotificationMethod(PacketReadyNotification::SameThread);
        port.connect(signal);
        
        connection = port.getConnection();
    }
    catch (...)
    {
        this->releaseWeakRefOnException();
        throw;
    }
}

PacketReaderImpl::PacketReaderImpl(IInputPortConfig* port)
{
    if (!port)
        DAQ_THROW_EXCEPTION(ArgumentNullException, "Input port must not be null.");

    this->port = port;

    portBinder = PropertyObject();
    this->port.asPtr<IOwnable>().setOwner(portBinder);
    connection = this->port.getConnection();

    this->internalAddRef();
    try
    {
        this->port.setListener(this->thisPtr<InputPortNotificationsPtr>());
        this->port.setNotificationMethod(PacketReadyNotification::Scheduler);
    }
    catch (...)
    {
        this->releaseWeakRefOnException();
        throw;
    }
}

PacketReaderImpl::~PacketReaderImpl()
{
    if (port.assigned() && !portBinder.assigned())
        port.remove();
}

ErrCode PacketReaderImpl::getAvailableCount(SizeT* count)
{
    std::scoped_lock lock(mutex);
    *count = 0;
    if (connection.assigned())
        return connection->getPacketCount(count);
    return OPENDAQ_SUCCESS;
}

ErrCode PacketReaderImpl::getEmpty(Bool* empty)
{
    OPENDAQ_PARAM_NOT_NULL(empty);
    SizeT count;
    getAvailableCount(&count);
    *empty = count == 0;
    return OPENDAQ_SUCCESS;
}

ErrCode PacketReaderImpl::setOnDataAvailable(IProcedure* callback)
{
    std::scoped_lock lock(mutex);

    readCallback = callback;
    return OPENDAQ_SUCCESS;
}

ErrCode PacketReaderImpl::setExternalListener(IInputPortNotifications* listener)
{
    this->externalListener = listener;
    return OPENDAQ_SUCCESS;
}

ErrCode PacketReaderImpl::read(IPacket** packet)
{
    OPENDAQ_PARAM_NOT_NULL(packet);
    *packet = nullptr;

    std::scoped_lock lock(mutex);
    if (connection.assigned())
    {
        return connection->dequeue(packet);
    }
    return OPENDAQ_SUCCESS;
}

ErrCode PacketReaderImpl::readAll(IList** allPackets)
{
    OPENDAQ_PARAM_NOT_NULL(allPackets);

    ErrCode errCode = createListWithElementType(allPackets, IPacket::Id);
    OPENDAQ_RETURN_IF_FAILED(errCode);

    std::scoped_lock lock(mutex);

    if (!connection.assigned())
        return OPENDAQ_SUCCESS;

    SizeT size{};
    errCode = connection->getPacketCount(&size);
    OPENDAQ_RETURN_IF_FAILED(errCode);

    auto readPackets = ListPtr<IPacket>::Borrow(*allPackets);
    for (std::size_t i = 0u; i < size; ++i)
    {
        readPackets.pushBack(connection.dequeue());
    }

    return OPENDAQ_SUCCESS;
}

ErrCode PacketReaderImpl::acceptsSignal(IInputPort* port, ISignal* signal, Bool* accept)
{
    OPENDAQ_PARAM_NOT_NULL(accept);
    
    if (externalListener.assigned() && externalListener.getRef().assigned())
        return externalListener.getRef()->acceptsSignal(port, signal, accept);

    *accept = true;
    return OPENDAQ_SUCCESS;
}

ErrCode PacketReaderImpl::connected(IInputPort* port)
{
    OPENDAQ_PARAM_NOT_NULL(port);

    // TODO: Thread safety
    port->getConnection(&connection);
    
    if (externalListener.assigned() && externalListener.getRef().assigned())
        return externalListener.getRef()->connected(port);

    return OPENDAQ_SUCCESS;
}

ErrCode PacketReaderImpl::disconnected(IInputPort* port)
{
    OPENDAQ_PARAM_NOT_NULL(port);
    ProcedurePtr callback;

    {
        std::scoped_lock lock(mutex);
        connection = nullptr;
    }

    if (externalListener.assigned() && externalListener.getRef().assigned())
        return externalListener.getRef()->disconnected(port);

    return OPENDAQ_SUCCESS;
}

ErrCode PacketReaderImpl::packetReceived(IInputPort* port)
{
    OPENDAQ_PARAM_NOT_NULL(port);
    ProcedurePtr callback;

    {
        std::scoped_lock lock(mutex);
        callback = readCallback;
    }
    
    if (callback.assigned())
        OPENDAQ_RETURN_IF_FAILED(wrapHandler(callback));

    if (externalListener.assigned() && externalListener.getRef().assigned())
        return externalListener.getRef()->packetReceived(port);

    return OPENDAQ_SUCCESS;
}

OPENDAQ_DEFINE_CLASS_FACTORY(
    LIBRARY_FACTORY, PacketReader,
    ISignal*, signal
)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, PacketReader,
    IPacketReader, createPacketReaderFromPort,
    IInputPortConfig*, port
)


END_NAMESPACE_OPENDAQ
