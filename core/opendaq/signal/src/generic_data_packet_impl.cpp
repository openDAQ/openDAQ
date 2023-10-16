#include <opendaq/generic_data_packet_impl.h>

BEGIN_NAMESPACE_OPENDAQ

std::atomic<Int> globalPacketId {0};

Int generatePacketId()
{
    return std::atomic_fetch_add_explicit(&globalPacketId, Int(1), std::memory_order_relaxed);
}

END_NAMESPACE_OPENDAQ
