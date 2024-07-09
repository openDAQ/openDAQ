#include <opendaq/sync_component_impl.h>

BEGIN_NAMESPACE_OPENDAQ


OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, SyncComponent, ISyncComponent,
    ITypeManager*, manager,
    IString*, className
)

END_NAMESPACE_OPENDAQ