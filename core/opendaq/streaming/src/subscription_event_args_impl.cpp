#include <opendaq/subscription_event_args_impl.h>
#include <coretypes/validation.h>

BEGIN_NAMESPACE_OPENDAQ

SubscriptionEventArgsImpl::SubscriptionEventArgsImpl(const StringPtr& streamingConnectionString,
                                                     SubscriptionEventType eventType)
    : EventArgsImplTemplate<ISubscriptionEventArgs>(0, "SubscriptionEvent")
    , streamingConnectionString(streamingConnectionString)
    , eventType(eventType)
{

}

ErrCode SubscriptionEventArgsImpl::getStreamingConnectionString(IString** streamingConnectionString)
{
    OPENDAQ_PARAM_NOT_NULL(streamingConnectionString);

    *streamingConnectionString = this->streamingConnectionString.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode SubscriptionEventArgsImpl::getSubscriptionEventType(SubscriptionEventType* eventType)
{
    OPENDAQ_PARAM_NOT_NULL(eventType);

    *eventType = this->eventType;
    return OPENDAQ_SUCCESS;
}

OPENDAQ_DEFINE_CLASS_FACTORY(
    LIBRARY_FACTORY, SubscriptionEventArgs,
    IString*, streamingConnectionString,
    SubscriptionEventType, eventType
)

END_NAMESPACE_OPENDAQ

