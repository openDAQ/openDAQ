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

    this->port = InputPortConfigPtr(port);
    this->port.asPtr<IOwnable>().setOwner(portBinder);
    if (!this->port.getConnection().assigned())
        throw ArgumentNullException("Input port not connected to signal");

    this->internalAddRef();

    this->port.setListener(this->thisPtr<InputPortNotificationsPtr>());
    this->port.setNotificationMethod(PacketReadyNotification::SameThread);
    
    connection = this->port.getConnection();
}

ErrCode PacketReaderImpl::getAvailableCount(SizeT* count)
{
    std::scoped_lock lock(mutex);
    return connection->getPacketCount(count);
}

ErrCode PacketReaderImpl::setOnDescriptorChanged(IFunction* callback)
{
    OPENDAQ_PARAM_NOT_NULL(callback);

    return  OPENDAQ_IGNORED;
}

ErrCode PacketReaderImpl::setOnAvailablePackets(IFunction* callback)
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
    OPENDAQ_PARAM_NOT_NULL(port);
    OPENDAQ_PARAM_NOT_NULL(signal);
    OPENDAQ_PARAM_NOT_NULL(accept);

    *accept = true;
    return OPENDAQ_SUCCESS;
}

ErrCode PacketReaderImpl::connected(IInputPort* port)
{
    OPENDAQ_PARAM_NOT_NULL(port);
    return OPENDAQ_SUCCESS;
}

ErrCode PacketReaderImpl::disconnected(IInputPort* port)
{
    OPENDAQ_PARAM_NOT_NULL(port);
    return OPENDAQ_SUCCESS;
}

ErrCode PacketReaderImpl::packetReceived(IInputPort* port)
{
    OPENDAQ_PARAM_NOT_NULL(port);
    if (!readCallback.assigned())
        return OPENDAQ_SUCCESS;

    SizeT count{0};
    auto callback = readCallback;
    connection->getPacketCount(&count);
    while (callback.assigned() && count)
    {
        callback();
        connection->getPacketCount(&count);
        callback = readCallback;
    }

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
