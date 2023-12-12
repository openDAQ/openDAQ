#include <coretypes/struct_impl.h>

BEGIN_NAMESPACE_OPENDAQ

#if defined(coretypes_EXPORTS)

OPENDAQ_DEFINE_CLASS_FACTORY(LIBRARY_FACTORY, Struct, IString*, name, IDict*, fields, ITypeManager*, typeManager)
OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(LIBRARY_FACTORY, Struct, IStruct, createStructFromBuilder, IStructBuilder*, builder)

#endif

END_NAMESPACE_OPENDAQ
