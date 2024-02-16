#include <opendaq/function_block_type_impl.h>

BEGIN_NAMESPACE_OPENDAQ

OPENDAQ_DEFINE_CLASS_FACTORY(
    LIBRARY_FACTORY,
    FunctionBlockType,
    IString*,
    id,
    IString*,
    name,
    IString*,
    description,
    IFunction*,
    createDefaultConfigCallback
    )

END_NAMESPACE_OPENDAQ
