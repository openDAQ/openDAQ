/*
 * Copyright 2022-2025 openDAQ d.o.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once
#include <opendaq/streaming_impl.h>
#include <opendaq/streaming_to_device_impl.h>
#include <native_streaming_client_module/common.h>

#include <native_streaming_protocol/native_streaming_client_handler.h>
#include <boost/asio/dispatch.hpp>
#include <opendaq/reader_factory.h>

BEGIN_NAMESPACE_OPENDAQ_NATIVE_STREAMING_CLIENT_MODULE

static const char* NativeStreamingPrefix = "daq.ns";
static const char* NativeStreamingID = "OpenDAQNativeStreaming";

DECLARE_OPENDAQ_INTERFACE(INativeStreamingPrivate, IBaseObject)
{
    virtual void INTERFACE_FUNC upgradeToSafeProcessingCallbacks() = 0;
};

template <typename Impl>
class NativeStreamingBaseImpl : public Impl
{
public:
    template <class ... Args>
    explicit NativeStreamingBaseImpl(const StringPtr& connectionString,
                                     const ContextPtr& context,
                                     opendaq_native_streaming_protocol::NativeStreamingClientHandlerPtr transportClientHandler,
                                     std::shared_ptr<boost::asio::io_context> processingIOContextPtr,
                                     Int streamingInitTimeout,
                                     const ProcedurePtr& onDeviceSignalAvailableCallback,
                                     const ProcedurePtr& onDeviceSignalUnavailableCallback,
                                     opendaq_native_streaming_protocol::OnConnectionStatusChangedCallback onDeviceConnectionStatusChangedCb,
                                     const Args& ... args)
        : Impl(connectionString, context, false, args ...)
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

    ~NativeStreamingBaseImpl()
    {
        protocolInitTimer->cancel();

        transportClientHandler->resetStreamingHandlers();
        stopProcessingOperations();
    }

    // INativeStreamingPrivate
    void INTERFACE_FUNC upgradeToSafeProcessingCallbacks() override
    {
        upgradeClientHandlerCallbacks();
    }

protected:
    void onSetActive(bool active) override
    {
    }
    void onAddSignal(const MirroredSignalConfigPtr& signal) override
    {
    }
    void onRemoveSignal(const MirroredSignalConfigPtr& signal) override
    {
    }
    void onSubscribeSignal(const StringPtr& signalStreamingId) override
    {
        transportClientHandler->subscribeSignal(signalStreamingId);
    }
    void onUnsubscribeSignal(const StringPtr& signalStreamingId) override
    {
        transportClientHandler->unsubscribeSignal(signalStreamingId);
    }

    void signalAvailableHandler(const StringPtr& signalStringId, const StringPtr& serializedSignal)
    {
        Impl::addToAvailableSignals(signalStringId);
        if (onDeviceSignalAvailableCallback.assigned())
        {
            ErrCode errCode = wrapHandler(onDeviceSignalAvailableCallback, signalStringId, serializedSignal);
            checkErrorInfo(errCode);
        }
    }
    void signalUnavailableHandler(const StringPtr& signalStringId)
    {
        Impl::removeFromAvailableSignals(signalStringId);
        if (onDeviceSignalUnavailableCallback.assigned())
        {
            ErrCode errCode = wrapHandler(onDeviceSignalUnavailableCallback, signalStringId);
            checkErrorInfo(errCode);
        }
    }

    void updateConnectionStatus(const EnumerationPtr& status, const StringPtr& statusMessage) override
    {
        if (onDeviceConnectionStatusChangedCb)
        {
            onDeviceConnectionStatusChangedCb(status, statusMessage);
        }

        Impl::updateConnectionStatus(status, statusMessage);
    }
    void processTransportConnectionStatus(const EnumerationPtr& status, const StringPtr& statusMessage)
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

    void initClientHandlerCallbacks()
    {
        using namespace boost::asio;

        opendaq_native_streaming_protocol::OnSignalAvailableCallback signalAvailableCb =
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
        opendaq_native_streaming_protocol::OnSignalUnavailableCallback signalUnavailableCb =
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
        opendaq_native_streaming_protocol::OnPacketCallback onPacketCallback =
            [this](const StringPtr& signalStringId, const PacketPtr& packet)
        {
            dispatch(
                *processingIOContextPtr,
                [this, signalStringId, packet]()
                {
                    Impl::onPacket(signalStringId, packet);
                }
                );
        };
        opendaq_native_streaming_protocol::OnSignalSubscriptionAckCallback onSignalSubscriptionAckCallback =
            [this](const StringPtr& signalStringId, bool subscribed)
        {
            dispatch(
                *processingIOContextPtr,
                [this, signalStringId, subscribed]()
                {
                    Impl::triggerSubscribeAck(signalStringId, subscribed);
                }
                );
        };
        opendaq_native_streaming_protocol::OnConnectionStatusChangedCallback onConnectionStatusChangedCb =
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
        opendaq_native_streaming_protocol::OnStreamingInitDoneCallback onStreamingInitDoneCb =
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
    void upgradeClientHandlerCallbacks()
    {
        using namespace boost::asio;
        WeakRefPtr<IStreaming> thisRef = this->template borrowPtr<StreamingPtr>();

        opendaq_native_streaming_protocol::OnSignalAvailableCallback signalAvailableCb =
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
        opendaq_native_streaming_protocol::OnSignalUnavailableCallback signalUnavailableCb =
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
        opendaq_native_streaming_protocol::OnPacketCallback onPacketCallback =
            [this, thisRef](const StringPtr& signalStringId, const PacketPtr& packet)
        {
            dispatch(
                *processingIOContextPtr,
                [this, thisRef, signalStringId, packet]()
                {
                    if (auto thisPtr = thisRef.getRef(); thisPtr.assigned())
                        Impl::onPacket(signalStringId, packet);
                }
                );
        };
        opendaq_native_streaming_protocol::OnSignalSubscriptionAckCallback onSignalSubscriptionAckCallback =
            [this, thisRef](const StringPtr& signalStringId, bool subscribed)
        {
            dispatch(
                *processingIOContextPtr,
                [this, thisRef, signalStringId, subscribed]()
                {
                    if (auto thisPtr = thisRef.getRef(); thisPtr.assigned())
                        Impl::triggerSubscribeAck(signalStringId, subscribed);
                }
                );
        };
        opendaq_native_streaming_protocol::OnConnectionStatusChangedCallback onTransportConnectionStatusChangedCb =
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
        opendaq_native_streaming_protocol::OnStreamingInitDoneCallback onStreamingInitDoneCb =
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

    void stopProcessingOperations()
    {
        if (!processingIOContextPtr->stopped())
        {
            processingIOContextPtr->stop();
        }
    }
    void requestStreamingOnReconnection()
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

    opendaq_native_streaming_protocol::NativeStreamingClientHandlerPtr transportClientHandler;

    // pseudo device callbacks
    ProcedurePtr onDeviceSignalAvailableCallback;
    ProcedurePtr onDeviceSignalUnavailableCallback;
    opendaq_native_streaming_protocol::OnConnectionStatusChangedCallback onDeviceConnectionStatusChangedCb;

    std::shared_ptr<boost::asio::io_context> processingIOContextPtr;

    std::promise<void> protocolInitPromise;
    std::future<void> protocolInitFuture;

    std::chrono::milliseconds streamingInitTimeout;
    std::shared_ptr<boost::asio::io_context> timerContextPtr;
    std::shared_ptr<boost::asio::steady_timer> protocolInitTimer;
};

template <typename Impl>
class NativeStreamingImpl : public NativeStreamingBaseImpl<Impl>
{
public:
    using Super = NativeStreamingBaseImpl<Impl>;

    NativeStreamingImpl(const StringPtr& connectionString,
                        const ContextPtr& context,
                        opendaq_native_streaming_protocol::NativeStreamingClientHandlerPtr transportClientHandler,
                        std::shared_ptr<boost::asio::io_context> processingIOContextPtr,
                        Int streamingInitTimeout,
                        const ProcedurePtr& onDeviceSignalAvailableCallback,
                        const ProcedurePtr& onDeviceSignalUnavailableCallback,
                        opendaq_native_streaming_protocol::OnConnectionStatusChangedCallback onDeviceConnectionStatusChangedCb)
        : Super(connectionString,
                context,
                transportClientHandler,
                processingIOContextPtr,
                streamingInitTimeout,
                onDeviceSignalAvailableCallback,
                onDeviceSignalUnavailableCallback,
                onDeviceConnectionStatusChangedCb)
    {}
};

using NativeStreamingBasicImpl = NativeStreamingImpl<StreamingImpl<IStreaming, INativeStreamingPrivate>>;

template <>
class NativeStreamingImpl<StreamingToDeviceImpl<INativeStreamingPrivate>>
    : public NativeStreamingBaseImpl<StreamingToDeviceImpl<INativeStreamingPrivate>>
{
public:
    using Super = NativeStreamingBaseImpl<StreamingToDeviceImpl<INativeStreamingPrivate>>;

    NativeStreamingImpl(const StringPtr& connectionString,
                        const ContextPtr& context,
                        opendaq_native_streaming_protocol::NativeStreamingClientHandlerPtr transportClientHandler,
                        std::shared_ptr<boost::asio::io_context> processingIOContextPtr,
                        Int streamingInitTimeout,
                        const ProcedurePtr& onDeviceSignalAvailableCallback,
                        const ProcedurePtr& onDeviceSignalUnavailableCallback,
                        opendaq_native_streaming_protocol::OnConnectionStatusChangedCallback onDeviceConnectionStatusChangedCb,
                        const StringPtr& protocolId)
        : Super(connectionString,
                context,
                transportClientHandler,
                processingIOContextPtr,
                streamingInitTimeout,
                nullptr,
                nullptr,
                nullptr,
                protocolId)
    {
        initStreamingToDeviceCallbacks();
        startReadThread();
    }

    // INativeStreamingPrivate
    void INTERFACE_FUNC upgradeToSafeProcessingCallbacks() override
    {
        upgradeStreamingToDeviceCallbacks();
        Super::upgradeToSafeProcessingCallbacks();
    }

    ~NativeStreamingImpl() override
    {
        this->transportClientHandler->resetStreamingToDeviceHandlers();
        for (const auto& [_, signalRef] : this->streamedSignals)
        {
            if (auto signal = signalRef.getRef(); signal.assigned())
                this->transportClientHandler->removeClientSignal(signal);
        }
    }

protected:
    void onRegisterStreamedSignal(const SignalPtr& signal) override
    {
        this->transportClientHandler->addClientSignal(signal);
    }

    void onUnregisterStreamedSignal(const SignalPtr& signal) override
    {
        this->transportClientHandler->removeClientSignal(signal);
    }

    void signalReadingFunc() override
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

private:
    void initStreamingToDeviceCallbacks()
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
    void upgradeStreamingToDeviceCallbacks()
    {
        using namespace boost::asio;
        WeakRefPtr<IStreamingToDevice> thisRef = this->template borrowPtr<StreamingToDevicePtr>();

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
    }

    void signalSubscribeHandler(const SignalPtr& signal)
    {
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

    void signalUnsubscribeHandler(const SignalPtr& signal)
    {
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

    std::vector<std::pair<SignalPtr, PacketReaderPtr>> signalReaders;
    std::mutex readersSync;
};

using NativeStreamingToDeviceImpl = NativeStreamingImpl<StreamingToDeviceImpl<INativeStreamingPrivate>>;

END_NAMESPACE_OPENDAQ_NATIVE_STREAMING_CLIENT_MODULE
