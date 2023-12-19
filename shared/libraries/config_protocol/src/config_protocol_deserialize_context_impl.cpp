#include <config_protocol/config_protocol_deserialize_context_impl.h>

namespace daq::config_protocol
{

ConfigProtocolClientCommPtr ConfigProtocolDeserializeContextImpl::getClientComm()
{
    return clientComm;
}

}
