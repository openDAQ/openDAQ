#include <opendaq/opendaq.h>

namespace
{
// TODO! opcua nd ns lt
daq::StringPtr protocol = "opcua";  // getenv("protocol"); 
daq::StringPtr connectionString = "daq." + protocol + "://127.0.0.1";
}
