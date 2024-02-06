#include <opendaq/component_deserialize_context_impl.h>

namespace daq
{

OPENDAQ_DEFINE_CLASS_FACTORY(
    LIBRARY_FACTORY, ComponentDeserializeContext, IContext*, context, IComponent*, root, IComponent*, parent, IString*, localId);

}
