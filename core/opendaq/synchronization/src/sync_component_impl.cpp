#include <opendaq/sync_component_impl.h>

BEGIN_NAMESPACE_OPENDAQ


OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, SyncComponent, ISyncComponent,
    IContext*, context,
    IComponent*, ParseFailedException,
    IString*, localId
)

END_NAMESPACE_OPENDAQ