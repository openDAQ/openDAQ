#include <config_protocol/config_client_object_impl.h>

namespace daq::config_protocol
{

ConfigClientObjectImpl::ConfigClientObjectImpl(ConfigProtocolClientCommPtr clientComm, std::string remoteGlobalId)
    : clientComm(std::move(clientComm))
    , remoteGlobalId(std::move(remoteGlobalId))
{
}

}
