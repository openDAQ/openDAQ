#include <websocket_streaming/websocket_streaming_init.h>
#include <coretypes/errors.h>
#include <streaming_protocol/SynchronousSignal.hpp>
#include <streaming_protocol/StreamWriter.h>
#include <streaming_protocol/Logging.hpp>

using namespace daq::streaming_protocol;

void daqInitStreamingLibrary()
{
    auto writer = StreamWriter(nullptr);
    auto dummySignal = SynchronousSignal<int32_t>("id", 100, 1000, writer, Logging::logCallback());
}
