#include <coretypes/event_args_impl.h>

BEGIN_NAMESPACE_OPENDAQ

// OPENDAQ_DEFINE_CLASS_FACTORY(LIBRARY_FACTORY, EventArgs, Int, id, IString*, eventName)

extern "C"
ErrCode PUBLIC_EXPORT createEventArgs(IEventArgs** objTmp,
                                      Int id,
                                      IString* eventName
)
{
    return createObject<IEventArgs, EventArgsImpl, Int, IString*>(objTmp, id, eventName);
}

END_NAMESPACE_OPENDAQ
