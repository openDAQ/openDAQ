#include <opendaq/context_impl.h>


BEGIN_NAMESPACE_OPENDAQ


OPENDAQ_DEFINE_CLASS_FACTORY(
    LIBRARY_FACTORY,
    Context,
    IScheduler*, Scheduler,
    ILogger*, Logger,
    ITypeManager*, typeManager,
    IModuleManager*, moduleManager,
    IAuthenticationProvider*, authenticationProvider,
    IDict*, options,
    IDict*, discoveryServices
)

END_NAMESPACE_OPENDAQ
