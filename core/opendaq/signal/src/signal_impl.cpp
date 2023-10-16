#include <opendaq/signal_impl.h>

BEGIN_NAMESPACE_OPENDAQ

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, Signal, ISignalConfig,
    IContext*, context,
    IComponent*, parent,
    IString*, localId,
    IString*, className
)

using SignalWithDescriptorImpl = SignalImpl;

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, SignalWithDescriptor, ISignalConfig,
    IContext*, context,
    IDataDescriptor*, descriptor,
    IComponent*, parent,
    IString*, localId,
    IString*, className
)

END_NAMESPACE_OPENDAQ
