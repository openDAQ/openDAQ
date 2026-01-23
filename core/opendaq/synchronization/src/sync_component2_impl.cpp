#include <opendaq/sync_component2_impl.h>

BEGIN_NAMESPACE_OPENDAQ

template class SyncComponent2Impl<>;

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC_OBJ(
    LIBRARY_FACTORY, SyncComponent2Base, 
    ISyncComponent2, createSyncComponent2,
    IContext*, context,
    IComponent*, parent,
    IString*, localId,
    IString*, className,
    IString*, name,
    bool, registerEvents
)

END_NAMESPACE_OPENDAQ
