#include <opendaq/synchronization_impl.h>

BEGIN_NAMESPACE_OPENDAQ

template class SynchronizationImpl<>;

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC_OBJ(
    LIBRARY_FACTORY, SynchronizationBase, 
    ISynchronization, createSynchronization
)

END_NAMESPACE_OPENDAQ
