#include <opendaq/streaming_type_impl.h>

BEGIN_NAMESPACE_OPENDAQ

OPENDAQ_DEFINE_CLASS_FACTORY(
    LIBRARY_FACTORY,
    StreamingType,
    IString*,
    id,
    IString*,
    name,
    IString*,
    description,
    IPropertyObject*,
    defaultConfig
)

END_NAMESPACE_OPENDAQ
