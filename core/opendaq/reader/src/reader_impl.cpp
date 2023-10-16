#include <opendaq/reader_impl.h>
#include <opendaq/block_reader.h>
#include <opendaq/packet_reader.h>
#include <opendaq/stream_reader.h>
#include <opendaq/tail_reader.h>

BEGIN_NAMESPACE_OPENDAQ

template class ReaderImpl<IBlockReader>;
template class ReaderImpl<ITailReader>;

END_NAMESPACE_OPENDAQ
