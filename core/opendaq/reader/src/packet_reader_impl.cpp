#include <opendaq/packet_reader_impl.h>
#include <coretypes/list_factory.h>

BEGIN_NAMESPACE_OPENDAQ

PacketReaderImpl::PacketReaderImpl(const SignalPtr& signal)
{
    if (!signal.assigned())
        throw ArgumentNullException("Signal must not be null.");

    port = InputPort(signal.getContext(), nullptr, "readsignal");

    port.connect(signal);
    connection = port.getConnection();
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

END_NAMESPACE_OPENDAQ
