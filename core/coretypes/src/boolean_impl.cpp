#include <coretypes/boolean_impl.h>
#include <coretypes/impl.h>

BEGIN_NAMESPACE_OPENDAQ

OPENDAQ_DEFINE_CLASS_FACTORY(LIBRARY_FACTORY, Boolean, const Bool, value)
OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY,
    Boolean,
    IBoolean,
    createBoolObject,
    const Bool,
    value
)

END_NAMESPACE_OPENDAQ
