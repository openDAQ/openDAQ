#include <config_protocol/config_client_component_impl.h>
#include <opendaq/component_impl.h>
#include <config_protocol/config_client_object.h>

namespace daq::config_protocol
{

// Explicit template instantiation
template class ConfigClientComponentBaseImpl<ComponentImpl<IComponent, IConfigClientObject>>;

}
