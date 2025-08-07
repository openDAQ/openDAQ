#include <native_streaming_client_module/native_streaming_impl.h>

#include <opendaq/signal_config_ptr.h>
#include <opendaq/custom_log.h>

#include <opendaq/mirrored_signal_private_ptr.h>
#include <opendaq/subscription_event_args_factory.h>

#include <boost/asio/dispatch.hpp>
#include <opendaq/reader_factory.h>

BEGIN_NAMESPACE_OPENDAQ_NATIVE_STREAMING_CLIENT_MODULE

using namespace opendaq_native_streaming_protocol;

NativeStreamingImpl::NativeStreamingImpl(
    const StringPtr& connectionString,
    const ContextPtr& context,
    NativeStreamingClientHandlerPtr transportClientHandler,
    std::shared_ptr<boost::asio::io_context> processingIOContextPtr,
    Int streamingInitTimeout,
    const ProcedurePtr& onDeviceSignalAvailableCallback,
    const ProcedurePtr& onDeviceSignalUnavailableCallback,
    OnConnectionStatusChangedCallback onDeviceConnectionStatusChangedCb,
    bool isClientToDeviceStreamingSupported)
    : Super(connectionString, context, false, NativeStreamingID, isClientToDeviceStreamingSupported)
    , transportClientHandler(transportClientHandler)
    , onDeviceSignalAvailableCallback(onDeviceSignalAvailableCallback)
    , onDeviceSignalUnavailableCallback(onDeviceSignalUnavailableCallback)
    , onDeviceConnectionStatusChangedCb(onDeviceConnectionStatusChangedCb)
    , processingIOContextPtr(processingIOContextPtr)
    , protocolInitFuture(protocolInitPromise.get_future())
    , streamingInitTimeout(std::chrono::milliseconds(streamingInitTimeout))
    , timerContextPtr(transportClientHandler->getIoContext())
    , protocolInitTimer(
          std::make_shared<boost::asio::steady_timer>(*timerContextPtr)
    )
{
    initClientHandlerCallbacks();
    this->transportClientHandler->sendStreamingRequest();

    if (protocolInitFuture.wait_for(this->streamingInitTimeout) != std::future_status::ready)
    {
        stopProcessingOperations();
        DAQ_THROW_EXCEPTION(GeneralErrorException,
                            "Streaming protocol intialization timed out; connection string: {}",
                            connectionString);
    }
}

NativeStreamingImpl::~NativeStreamingImpl()
{
    protocolInitTimer->cancel();

    transportClientHandler->resetStreamingHandlers();
    stopProcessingOperations();
}

void NativeStreamingImpl::upgradeToSafeProcessingCallbacks()
{
    upgradeClientHandlerCallbacks();
}

void NativeStreamingImpl::stopProcessingOperations()
{
    if (!processingIOContextPtr->stopped())
    {
        processingIOContextPtr->stop();
    }
}

void NativeStreamingImpl::signalAvailableHandler(const StringPtr& signalStringId, const StringPtr& serializedSignal)
{
    addToAvailableSignals(signalStringId);
    if (onDeviceSignalAvailableCallback.assigned())
    {
        ErrCode errCode = wrapHandler(onDeviceSignalAvailableCallback, signalStringId, serializedSignal);
        checkErrorInfo(errCode);
    }
}

void NativeStreamingImpl::signalUnavailableHandler(const StringPtr& signalStringId)
{
    removeFromAvailableSignals(signalStringId);
    if (onDeviceSignalUnavailableCallback.assigned())
    {
        ErrCode errCode = wrapHandler(onDeviceSignalUnavailableCallback, signalStringId);
        checkErrorInfo(errCode);
    }
}

void NativeStreamingImpl::updateConnectionStatus(const EnumerationPtr& status, const StringPtr& statusMessage)
{
    if (onDeviceConnectionStatusChangedCb)
    {
        onDeviceConnectionStatusChangedCb(status, statusMessage);
    }

    Super::updateConnectionStatus(status, statusMessage);
}

void NativeStreamingImpl::processTransportConnectionStatus(const EnumerationPtr& status, const StringPtr& statusMessage)
{
    if (status == "Connected")
    {
        requestStreamingOnReconnection();
    }
    else
    {
        updateConnectionStatus(status, statusMessage);
    }
}

void NativeStreamingImpl::requestStreamingOnReconnection()
{
    transportClientHandler->sendStreamingRequest();

    protocolInitPromise = std::promise<void>();
    protocolInitFuture = protocolInitPromise.get_future();
    WeakRefPtr<IStreaming> thisRef = this->template borrowPtr<StreamingPtr>();

    protocolInitTimer->expires_from_now(streamingInitTimeout);
    protocolInitTimer->async_wait(
        [this, thisRef](const boost::system::error_code& ec)
        {
            if (ec)
                return;

            if (auto thisPtr = thisRef.getRef(); thisPtr.assigned())
            {
                dispatch(
                    *processingIOContextPtr,
                    [this, thisRef]()
                    {
                        if (auto thisPtr = thisRef.getRef(); thisPtr.assigned())
                        {
                            if (protocolInitFuture.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)
                                updateConnectionStatus(Enumeration("ConnectionStatusType", "Connected", this->context.getTypeManager()), "");
                            else
                                updateConnectionStatus(Enumeration("ConnectionStatusType", "Unrecoverable", this->context.getTypeManager()),
                                                       "Reconnection failed – streaming request timed out");
                        }
                    }
                );
            };
        }
    );
}

void NativeStreamingImpl::upgradeClientHandlerCallbacks()
{
    using namespace boost::asio;
    WeakRefPtr<IStreaming> thisRef = this->template borrowPtr<StreamingPtr>();

    OnSignalAvailableCallback signalAvailableCb =
        [this, thisRef](const StringPtr& signalStringId,
               const StringPtr& serializedSignal)
    {
        dispatch(
            *processingIOContextPtr,
            [this, thisRef, signalStringId, serializedSignal]()
            {
                if (auto thisPtr = thisRef.getRef(); thisPtr.assigned())
                    signalAvailableHandler(signalStringId, serializedSignal);
            }
        );
    };
    OnSignalUnavailableCallback signalUnavailableCb =
        [this, thisRef](const StringPtr& signalStringId)
    {
        dispatch(
            *processingIOContextPtr,
            [this, thisRef, signalStringId]()
            {
                if (auto thisPtr = thisRef.getRef(); thisPtr.assigned())
                    signalUnavailableHandler(signalStringId);
            }
        );
    };
    OnPacketCallback onPacketCallback =
        [this, thisRef](const StringPtr& signalStringId, const PacketPtr& packet)
    {
        dispatch(
            *processingIOContextPtr,
            [this, thisRef, signalStringId, packet]()
            {
                if (auto thisPtr = thisRef.getRef(); thisPtr.assigned())
                    this->onPacket(signalStringId, packet);
            }
        );
    };
    OnSignalSubscriptionAckCallback onSignalSubscriptionAckCallback =
        [this, thisRef](const StringPtr& signalStringId, bool subscribed)
    {
        dispatch(
            *processingIOContextPtr,
            [this, thisRef, signalStringId, subscribed]()
            {
                if (auto thisPtr = thisRef.getRef(); thisPtr.assigned())
                    this->triggerSubscribeAck(signalStringId, subscribed);
            }
        );
    };
    OnConnectionStatusChangedCallback onTransportConnectionStatusChangedCb =
        [this, thisRef](const EnumerationPtr& status, const StringPtr& statusMessage)
    {
        dispatch(
            *processingIOContextPtr,
            [this, thisRef, status, statusMessage]()
            {
                if (auto thisPtr = thisRef.getRef(); thisPtr.assigned())
                    processTransportConnectionStatus(status, statusMessage);
            }
        );
    };
    OnStreamingInitDoneCallback onStreamingInitDoneCb =
        [this, thisRef]()
    {
        dispatch(
            *processingIOContextPtr,
            [this, thisRef]()
            {
                if (auto thisPtr = thisRef.getRef(); thisPtr.assigned())
                    protocolInitPromise.set_value();
            }
        );
    };

    transportClientHandler->setStreamingHandlers(signalAvailableCb,
                                                 signalUnavailableCb,
                                                 onPacketCallback,
                                                 onSignalSubscriptionAckCallback,
                                                 onTransportConnectionStatusChangedCb,
                                                 onStreamingInitDoneCb);
}

void NativeStreamingImpl::initClientHandlerCallbacks()
{
    using namespace boost::asio;

    OnSignalAvailableCallback signalAvailableCb =
        [this](const StringPtr& signalStringId,
               const StringPtr& serializedSignal)
    {
        dispatch(
            *processingIOContextPtr,
            [this, signalStringId, serializedSignal]()
            {
                signalAvailableHandler(signalStringId, serializedSignal);
            }
        );
    };
    OnSignalUnavailableCallback signalUnavailableCb =
        [this](const StringPtr& signalStringId)
    {
        dispatch(
            *processingIOContextPtr,
            [this, signalStringId]()
            {
                signalUnavailableHandler(signalStringId);
            }
        );
    };
    OnPacketCallback onPacketCallback =
        [this](const StringPtr& signalStringId, const PacketPtr& packet)
    {
        dispatch(
            *processingIOContextPtr,
            [this, signalStringId, packet]()
            {
                this->onPacket(signalStringId, packet);
            }
        );
    };
    OnSignalSubscriptionAckCallback onSignalSubscriptionAckCallback =
        [this](const StringPtr& signalStringId, bool subscribed)
    {
        dispatch(
            *processingIOContextPtr,
            [this, signalStringId, subscribed]()
            {
                this->triggerSubscribeAck(signalStringId, subscribed);
            }
        );
    };
    OnConnectionStatusChangedCallback onConnectionStatusChangedCb =
        [this](const EnumerationPtr& status, const StringPtr& statusMessage)
    {
        dispatch(
            *processingIOContextPtr,
            [this, status, statusMessage]()
            {
                processTransportConnectionStatus(status, statusMessage);
            }
        );
    };
    OnStreamingInitDoneCallback onStreamingInitDoneCb =
        [this]()
    {
        dispatch(
            *processingIOContextPtr,
            [this]()
            {
                protocolInitPromise.set_value();
            }
        );
    };

    transportClientHandler->setStreamingHandlers(signalAvailableCb,
                                                 signalUnavailableCb,
                                                 onPacketCallback,
                                                 onSignalSubscriptionAckCallback,
                                                 onConnectionStatusChangedCb,
                                                 onStreamingInitDoneCb);
}

void NativeStreamingImpl::onSetActive(bool active)
{
}

void NativeStreamingImpl::onAddSignal(const MirroredSignalConfigPtr& signal)
{
}

void NativeStreamingImpl::onRemoveSignal(const MirroredSignalConfigPtr& signal)
{
}

void NativeStreamingImpl::onSubscribeSignal(const StringPtr& signalStreamingId)
{
    transportClientHandler->subscribeSignal(signalStreamingId);
}

void NativeStreamingImpl::onUnsubscribeSignal(const StringPtr& signalStreamingId)
{
    transportClientHandler->unsubscribeSignal(signalStreamingId);
}

NativeStreamingToDeviceImpl::NativeStreamingToDeviceImpl(const StringPtr& connectionString,
                                                         const ContextPtr& context,
                                                         opendaq_native_streaming_protocol::NativeStreamingClientHandlerPtr transportClientHandler,
                                                         std::shared_ptr<boost::asio::io_context> processingIOContextPtr,
                                                         Int streamingInitTimeout)
    : Super(connectionString,
            context,
            transportClientHandler,
            processingIOContextPtr,
            streamingInitTimeout,
            nullptr,
            nullptr,
            nullptr,
            true)
    , readThreadRunning(false)
    , readThreadSleepTime(std::chrono::milliseconds(20))
{
    initStreamingToDeviceCallbacks();
    startReadThread();
}

NativeStreamingToDeviceImpl::~NativeStreamingToDeviceImpl()
{
    if (readThreadRunning)
        stopReadThread();

    this->transportClientHandler->resetStreamingToDeviceHandlers();
    for (const auto& [_, signalRef] : this->streamedSignals)
    {
        if (auto signal = signalRef.getRef(); signal.assigned())
            this->transportClientHandler->removeClientSignal(signal);
    }
}

void NativeStreamingToDeviceImpl::upgradeToSafeProcessingCallbacks()
{
    upgradeStreamingToDeviceCallbacks();
    Super::upgradeToSafeProcessingCallbacks();
}

void NativeStreamingToDeviceImpl::onRegisterStreamedClientSignal(const SignalPtr& signal)
{
    this->transportClientHandler->addClientSignal(signal);
}

void NativeStreamingToDeviceImpl::onUnregisterStreamedClientSignal(const SignalPtr& signal)
{
    this->transportClientHandler->removeClientSignal(signal);
}

void NativeStreamingToDeviceImpl::initStreamingToDeviceCallbacks()
{
    using namespace boost::asio;
    opendaq_native_streaming_protocol::OnSignalSubscribedCallback signaSubscribeCb =
        [this](const SignalPtr& signal)
    {
        dispatch(
            *processingIOContextPtr,
            [this, signal]()
            {
                this->signalSubscribeHandler(signal);
            }
        );
    };
    opendaq_native_streaming_protocol::OnSignalUnsubscribedCallback signaUnsubscribeCb =
        [this](const SignalPtr& signal)
    {
        dispatch(
            *processingIOContextPtr,
            [this, signal]()
            {
                this->signalUnsubscribeHandler(signal);
            }
        );
    };
    this->transportClientHandler->setStreamingToDeviceHandlers(signaSubscribeCb, signaUnsubscribeCb);
}

void NativeStreamingToDeviceImpl::upgradeStreamingToDeviceCallbacks()
{
    using namespace boost::asio;
    WeakRefPtr<IStreaming> thisRef = this->template borrowPtr<StreamingPtr>();

    opendaq_native_streaming_protocol::OnSignalSubscribedCallback signaSubscribeCb =
        [this, thisRef](const SignalPtr& signal)
    {
        dispatch(
            *processingIOContextPtr,
            [this, thisRef, signal]()
            {
                if (auto thisPtr = thisRef.getRef(); thisPtr.assigned())
                    this->signalSubscribeHandler(signal);
            }
        );
    };
    opendaq_native_streaming_protocol::OnSignalUnsubscribedCallback signaUnsubscribeCb =
        [this, thisRef](const SignalPtr& signal)
    {
        dispatch(
            *processingIOContextPtr,
            [this, thisRef, signal]()
            {
                if (auto thisPtr = thisRef.getRef(); thisPtr.assigned())
                    this->signalUnsubscribeHandler(signal);
            }
        );
    };
    this->transportClientHandler->setStreamingToDeviceHandlers(signaSubscribeCb, signaUnsubscribeCb);
    // TODO clear read signals on disconnection
}

void NativeStreamingToDeviceImpl::signalSubscribeHandler(const SignalPtr &signal)
{
    std::scoped_lock lock(readersSync);
    auto it = std::find_if(signalReaders.begin(),
                           signalReaders.end(),
                           [&signal](const std::pair<SignalPtr, PacketReaderPtr>& element)
                           {
                               return element.first == signal;
                           });
    if (it != signalReaders.end())
        return;

    LOG_I("Add reader for client signal {}", signal.getGlobalId());
    auto reader = PacketReader(signal);
    signalReaders.push_back(std::pair<SignalPtr, PacketReaderPtr>({signal, reader}));
}

void NativeStreamingToDeviceImpl::signalUnsubscribeHandler(const SignalPtr &signal)
{
    std::scoped_lock lock(readersSync);
    auto it = std::find_if(signalReaders.begin(),
                           signalReaders.end(),
                           [&signal](const std::pair<SignalPtr, PacketReaderPtr>& element)
                           {
                               return element.first == signal;
                           });
    if (it == signalReaders.end())
        return;

    LOG_I("Remove reader for client signal {}", signal.getGlobalId());
    signalReaders.erase(it);
}

void NativeStreamingToDeviceImpl::startReadThread()
{
    assert(!readThreadRunning);
    readThreadRunning = true;
    readerThread = std::thread(&Self::readingThreadFunc, this);
}

void NativeStreamingToDeviceImpl::readingThreadFunc()
{
    daqNameThread("NativeC2DSread");
    DAQLOGF_D(this->loggerComponent, "Streaming-to-device read thread started")
    while (readThreadRunning)
    {
        {
            std::scoped_lock lock(readersSync);
            bool hasPacketsToRead;
            do
            {
                hasPacketsToRead = false;
                for (const auto& [signal, reader] : signalReaders)
                {
                    if (reader.getAvailableCount() == 0)
                        continue;

                    auto packet = reader.read();
                    this->transportClientHandler->sendPacket(signal.getGlobalId().toStdString(), std::move(packet));

                    if (reader.getAvailableCount() > 0)
                        hasPacketsToRead = true;
                }
            }
            while(hasPacketsToRead);
        }

        std::this_thread::sleep_for(readThreadSleepTime);
    }
    DAQLOGF_D(this->loggerComponent, "Streaming-to-device read thread stopped");
}

void NativeStreamingToDeviceImpl::stopReadThread()
{
    assert(readThreadRunning);
    readThreadRunning = false;
    if (readerThread.get_id() != std::this_thread::get_id())
    {
        if (readerThread.joinable())
        {
            readerThread.join();
            DAQLOGF_D(this->loggerComponent, "Streaming-to-device read thread joined");
        }
        else
        {
            DAQLOGF_W(this->loggerComponent, "Streaming-to-device read thread is not joinable");
        }
    }
    else
    {
        DAQLOGF_C(this->loggerComponent, "Streaming-to-device read thread cannot join itself");
    }
}

END_NAMESPACE_OPENDAQ_NATIVE_STREAMING_CLIENT_MODULE
