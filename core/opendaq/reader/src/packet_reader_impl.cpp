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

    port.connect(signal);
    connection = port.getConnection();
}

PacketReaderImpl::PacketReaderImpl(IInputPortConfig* port)
{
    if (!port)
        throw ArgumentNullException("Input port must not be null.");
    
    this->port = InputPortConfigPtr(port);
    this->port.asPtr<IOwnable>().setOwner(PropertyObject());

    if (!this->port.getConnection().assigned())
        throw ArgumentNullException("Input port not connected to signal");
    
    connection = this->port.getConnection();
}

PacketReaderImpl::~PacketReaderImpl()
{
    if (port.assigned())
        port.remove();
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
