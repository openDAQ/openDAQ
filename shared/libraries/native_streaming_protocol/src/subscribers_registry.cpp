#include <native_streaming_protocol/subscribers_registry.h>

#include <opendaq/custom_log.h>

BEGIN_NAMESPACE_OPENDAQ_NATIVE_STREAMING_PROTOCOL

using namespace daq::native_streaming;

SubscribersRegistry::SubscribersRegistry(const ContextPtr& context)
    : context(context)
    , logger(context.getLogger())
{
    if (!this->logger.assigned())
        throw ArgumentNullException("Logger must not be null");
    loggerComponent = this->logger.getOrAddComponent("NativeStreamingSubscribers");
}

void SubscribersRegistry::sendToClients(SendToClientCallback sendCallback)
{
    std::scoped_lock lock(sync);
    for (auto& sessionHandler : sessionHandlers)
    {
        sendCallback(sessionHandler);
    }
}

void SubscribersRegistry::sendToSubscribers(const SignalPtr& signal, SendToClientCallback sendCallback)
{
    auto signalKey = signal.getGlobalId().toStdString();
    auto iter = signalsSubscribers.find(signalKey);
    if (iter != signalsSubscribers.end())
    {
        auto& subscribers = iter->second;
        std::scoped_lock lock(sync);
        for (auto& sessionHandler : subscribers)
        {
            sendCallback(sessionHandler);
        }
    }
    else
    {
        throw NativeStreamingProtocolException("Signal is not registered");
    }
}

void SubscribersRegistry::sendToClient(SessionPtr session, SendToClientCallback sendCallback)
{
    std::scoped_lock lock(sync);
    auto iter = std::find_if(sessionHandlers.begin(),
                             sessionHandlers.end(),
                             [&session](std::shared_ptr<ServerSessionHandler>& handler)
                             {
                                 return handler->getSession() == session;
                             });
    if (iter != sessionHandlers.end())
    {
        sendCallback(*iter);
    }
    else
    {
        throw NativeStreamingProtocolException("Client is not registered");
    }
}

void SubscribersRegistry::registerSignal(const SignalPtr& signal)
{
    auto signalKey = signal.getGlobalId().toStdString();
    auto iter = signalsSubscribers.find(signalKey);
    if (iter == signalsSubscribers.end())
    {
        signalsSubscribers.insert({signalKey, std::vector<std::shared_ptr<ServerSessionHandler>>()});
    }
    else
    {
        throw NativeStreamingProtocolException("Signal is already registered");
    }
}

bool SubscribersRegistry::removeSignal(const SignalPtr& signal)
{
    bool doSignalUnsubscribe = false;
    auto signalKey = signal.getGlobalId().toStdString();
    auto signalIter = signalsSubscribers.find(signalKey);
    if (signalIter != signalsSubscribers.end())
    {
        std::scoped_lock lock(sync);
        auto& subscribers = signalIter->second;
        if (!subscribers.empty())
            doSignalUnsubscribe = true;
        signalsSubscribers.erase(signalIter);
    }
    else
    {
        throw NativeStreamingProtocolException("Signal is not registered");
    }
    return doSignalUnsubscribe;
}

void SubscribersRegistry::registerClient(std::shared_ptr<ServerSessionHandler> sessionHandler)
{
    std::scoped_lock lock(sync);
    sessionHandlers.push_back(sessionHandler);
}

std::vector<std::string> SubscribersRegistry::unregisterClient(SessionPtr session)
{
    std::vector<std::string> toUnsubscribe;

    // find and remove session handler from subscribers
    for (auto& signalIter : signalsSubscribers)
    {
        std::scoped_lock lock(sync);
        auto& subscribers = signalIter.second;
        auto subscriberIter = std::find_if(subscribers.begin(),
                                           subscribers.end(),
                                           [&session](std::shared_ptr<ServerSessionHandler>& handler)
                                           {
                                               return handler->getSession() == session;
                                           });
        if (subscriberIter != subscribers.end())
        {
            subscribers.erase(subscriberIter);
            if (subscribers.empty())
            {
                LOG_D("Signal: {} - is unsubscribed", signalIter.first);
                toUnsubscribe.push_back(signalIter.first);
            }
        }
    }

    // remove session handler
    std::scoped_lock lock(sync);
    sessionHandlers.erase(std::remove_if(sessionHandlers.begin(),
                                         sessionHandlers.end(),
                                         [&session](std::shared_ptr<ServerSessionHandler>& handler)
                                         {
                                             return handler->getSession() == session;
                                         }));

    return toUnsubscribe;
}

bool SubscribersRegistry::registerSignalSubscriber(const std::string& signalStringId, SessionPtr session)
{
    bool doSignalSubscribe = false;
    auto signalKey = signalStringId;
    auto iter = signalsSubscribers.find(signalKey);
    if (iter != signalsSubscribers.end())
    {
        std::scoped_lock lock(sync);
        auto& subscribers = iter->second;
        auto subscribersIter = std::find_if(subscribers.begin(),
                                            subscribers.end(),
                                            [&session](std::shared_ptr<ServerSessionHandler>& handler)
                                            {
                                                return handler->getSession() == session;
                                            });
        if (subscribersIter == subscribers.end())
        {
            if (subscribers.empty())
            {
                LOG_D("Signal: {} has first subscriber", signalStringId);
                doSignalSubscribe = true;
            }
            auto sessionHandlersIter = std::find_if(sessionHandlers.begin(),
                                                    sessionHandlers.end(),
                                                    [&session](std::shared_ptr<ServerSessionHandler>& handler)
                                                    {
                                                        return handler->getSession() == session;
                                                    });
            if (sessionHandlersIter != sessionHandlers.end())
            {
                subscribers.push_back(*sessionHandlersIter);
            }
        }
    }
    else
    {
        throw NativeStreamingProtocolException("Signal is not registered");
    }

    return doSignalSubscribe;
}

bool SubscribersRegistry::removeSignalSubscriber(const std::string& signalStringId, SessionPtr session)
{
    bool doSignalUnsubscribe = false;
    auto signalKey = signalStringId;
    auto iter = signalsSubscribers.find(signalKey);
    if (iter != signalsSubscribers.end())
    {
        std::scoped_lock lock(sync);
        auto& subscribers = iter->second;
        auto subscribersIter = std::find_if(subscribers.begin(),
                                            subscribers.end(),
                                            [&session](std::shared_ptr<ServerSessionHandler>& handler)
                                            {
                                                return handler->getSession() == session;
                                            });
        if (subscribersIter != subscribers.end())
        {
            subscribers.erase(subscribersIter);
            if (subscribers.empty())
            {
                LOG_D("Signal: {} has not subscribers", signalStringId);
                doSignalUnsubscribe = true;
            }
        }
    }
    else
    {
        throw NativeStreamingProtocolException("Signal is not registered");
    }

    return doSignalUnsubscribe;
}

END_NAMESPACE_OPENDAQ_NATIVE_STREAMING_PROTOCOL
