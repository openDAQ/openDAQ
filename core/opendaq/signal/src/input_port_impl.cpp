#include <opendaq/input_port_impl.h>

BEGIN_NAMESPACE_OPENDAQ

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY,
    InputPort,
    IInputPortConfig,
    IContext*, context,
    IComponent*, parent,
    IString*, localId,
    Bool, gapChecking)

END_NAMESPACE_OPENDAQ
