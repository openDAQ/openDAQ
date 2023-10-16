#include <coretypes/float_impl.h>
#include <coretypes/impl.h>

BEGIN_NAMESPACE_OPENDAQ

OPENDAQ_DEFINE_CLASS_FACTORY(LIBRARY_FACTORY, Float, const Float, value)
OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY,
    Float, IFloat,
    createFloatObject,
    const Float,
    value
)

END_NAMESPACE_OPENDAQ
