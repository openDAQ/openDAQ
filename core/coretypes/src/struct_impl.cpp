#include <coretypes/struct_impl.h>

BEGIN_NAMESPACE_OPENDAQ

#if defined(coretypes_EXPORTS)

OPENDAQ_DEFINE_CLASS_FACTORY(LIBRARY_FACTORY, Struct, IString*, name, IDict*, fields, ITypeManager*, typeManager)

#endif

END_NAMESPACE_OPENDAQ
