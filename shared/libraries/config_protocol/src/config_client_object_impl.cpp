#include <config_protocol/config_client_object_impl.h>

namespace daq::config_protocol
{

ConfigClientObjectImpl::ConfigClientObjectImpl(ConfigProtocolClientCommPtr configProtocolClientComm)
    : configProtocolClientComm(std::move(configProtocolClientComm))
{
}

}
