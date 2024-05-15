#include <opendaq/opendaq.h>

namespace
{
daq::StringPtr protocol = "opcua";  // getenv("protocol"); //TODO! opcua nd ns lt
daq::StringPtr connectionString = "daq." + protocol + "://127.0.0.1";
}
