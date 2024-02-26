#include <opendaq/packet_reader_impl.h>
#include <coretypes/list_factory.h>
#include <coreobjects/property_object_factory.h>
#include <coreobjects/ownable_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

PacketReaderImpl::PacketReaderImpl(const SignalPtr& signal)
{
    if (!signal.assigned())
        throw ArgumentNullException("Signal must not be null.");

    port = InputPort(signal.getContext(), nullptr, "readsignal");
    this->internalAddRef();

    port.setListener(this->thisPtr<InputPortNotificationsPtr>());
    port.setNotificationMethod(PacketReadyNotification::SameThread);
    port.connect(signal);
    
    connection = port.getConnection();
}

PacketReaderImpl::PacketReaderImpl(IInputPortConfig* port)
{
    if (!port)
        throw ArgumentNullException("Input port must not be null.");

    portBinder = PropertyObject();
    this->port = port;
    this->port.asPtr<IOwnable>().setOwner(portBinder);

    this->internalAddRef();

    this->port.setListener(this->thisPtr<InputPortNotificationsPtr>());
    this->port.setNotificationMethod(PacketReadyNotification::Scheduler);

    if (this->port.getConnection().assigned())
        connection = this->port.getConnection();
}

PacketReaderImpl::~PacketReaderImpl()
{
    if (port.assigned() && !portBinder.assigned())
        port.remove();
}

ErrCode PacketReaderImpl::getAvailableCount(SizeT* count)
{
    std::scoped_lock lock(mutex);
    return connection->getPacketCount(count);
}

ErrCode PacketReaderImpl::setOnDataAvailable(IProcedure* callback)
{
    std::scoped_lock lock(mutex);

    readCallback = callback;
    return OPENDAQ_SUCCESS;
}

ErrCode PacketReaderImpl::read(IPacket** packet)
{
    std::scoped_lock lock(mutex);
    return connection->dequeue(packet);
}

ErrCode PacketReaderImpl::readAll(IList** allPackets)
{
    OPENDAQ_PARAM_NOT_NULL(allPackets);

    ErrCode errCode = createListWithElementType(allPackets, IPacket::Id);
    if (OPENDAQ_FAILED(errCode))
        return errCode;

    std::scoped_lock lock(mutex);

    SizeT size{};
    errCode = connection->getPacketCount(&size);
    if (OPENDAQ_FAILED(errCode))
        return errCode;

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

    *accept = true;
    return OPENDAQ_SUCCESS;
}

ErrCode PacketReaderImpl::connected(IInputPort* port)
{
    OPENDAQ_PARAM_NOT_NULL(port);
    
    std::scoped_lock lock(mutex);
    connection = InputPortConfigPtr::Borrow(port).getConnection();
    return OPENDAQ_SUCCESS;
}

ErrCode PacketReaderImpl::disconnected(IInputPort* port)
{
    OPENDAQ_PARAM_NOT_NULL(port);

    std::scoped_lock lock(mutex);
    connection = nullptr;
    return OPENDAQ_SUCCESS;
}

ErrCode PacketReaderImpl::packetReceived(IInputPort* port)
{
    OPENDAQ_PARAM_NOT_NULL(port);
    ProcedurePtr callback;
    SizeT count{0};

    {
        std::scoped_lock lock(mutex);
        callback = readCallback;

        if (!callback.assigned())
            return OPENDAQ_SUCCESS;

        connection->getPacketCount(&count);
        if (!count)
            return OPENDAQ_SUCCESS;
    }
    return wrapHandler(callback);
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
