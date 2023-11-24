#include <native_streaming_server_module/native_streaming_server_impl.h>
#include <coretypes/impl.h>
#include <coreobjects/property_object_factory.h>
#include <coreobjects/property_factory.h>
#include <opendaq/server_type_factory.h>
#include <opendaq/device_private.h>
#include <opendaq/streaming_info_factory.h>
#include <opendaq/reader_factory.h>
#include <opendaq/custom_log.h>
#include <opendaq/event_packet_ids.h>

#include <native_streaming_protocol/native_streaming_server_handler.h>

BEGIN_NAMESPACE_OPENDAQ_NATIVE_STREAMING_SERVER_MODULE

using namespace daq;
using namespace opendaq_native_streaming_protocol;

NativeStreamingServerImpl::NativeStreamingServerImpl(DevicePtr rootDevice, PropertyObjectPtr config, const ContextPtr& context)
    : Server(config, rootDevice, context, nullptr)
    , readThreadActive(false)
    , readThreadSleepTime(std::chrono::milliseconds(20))
    , ioContextPtr(std::make_shared<boost::asio::io_context>())
    , workGuard(ioContextPtr->get_executor())
    , logger(context.getLogger())
    , loggerComponent(logger.getOrAddComponent("NativeStreamingServerImpl"))
{
    startAsyncOperations();

    prepareServerHandler();
    const uint16_t port = config.getPropertyValue("NativeStreamingPort");
    serverHandler->startServer(port);

    StreamingInfoConfigPtr streamingInfo = StreamingInfo("daq.ns");
    streamingInfo.addProperty(IntProperty("Port", port));
    ErrCode errCode = this->rootDevice.asPtr<IDevicePrivate>()->addStreamingOption(streamingInfo);
    checkErrorInfo(errCode);

    startReading();
}

NativeStreamingServerImpl::~NativeStreamingServerImpl()
{
    stopReading();
    stopAsyncOperations();
}

void NativeStreamingServerImpl::startAsyncOperations()
{
    ioThread = std::thread([this]()
                           {
                               ioContextPtr->run();
                               LOG_I("IO thread finished");
                           });
}

void NativeStreamingServerImpl::stopAsyncOperations()
{
    ioContextPtr->stop();
    if (ioThread.joinable())
    {
        ioThread.join();
        LOG_I("IO thread joined");
    }
}

void NativeStreamingServerImpl::prepareServerHandler()
{
    auto signalSubscribedHandler = [this](const SignalPtr& signal)
    {
        std::scoped_lock lock(readersSync);
        signalsToStartRead.push(signal);
    };
    auto signalUnsubscribedHandler = [this](const SignalPtr& signal)
    {
        std::scoped_lock lock(readersSync);
        signalsToStopRead.push(signal);
    };
    serverHandler = std::make_shared<NativeStreamingServerHandler>(context,
                                                                   ioContextPtr,
                                                                   rootDevice.getSignalsRecursive(),
                                                                   signalSubscribedHandler,
                                                                   signalUnsubscribedHandler);
}

PropertyObjectPtr NativeStreamingServerImpl::createDefaultConfig()
{
    constexpr Int minPortValue = 0;
    constexpr Int maxPortValue = 65535;

    auto defaultConfig = PropertyObject();

    const auto websocketPortProp = IntPropertyBuilder("NativeStreamingPort", 7420)
        .setMinValue(minPortValue)
        .setMaxValue(maxPortValue)
        .build();
    defaultConfig.addProperty(websocketPortProp);

    return defaultConfig;
}

ServerTypePtr NativeStreamingServerImpl::createType()
{
    auto configurationCallback = [](IBaseObject* input, IBaseObject** output) -> ErrCode
    {
        PropertyObjectPtr propObjPtr;
        ErrCode errCode = wrapHandlerReturn(&NativeStreamingServerImpl::createDefaultConfig, propObjPtr);
        *output = propObjPtr.detach();
        return errCode;
    };

    return ServerType(
        "openDAQ Native Streaming",
        "openDAQ Native Streaming server",
        "Publishes device signals as a flat list and streams data over openDAQ native streaming protocol",
        configurationCallback);
}

void NativeStreamingServerImpl::onStopServer()
{
    stopReading();
    ErrCode errCode = rootDevice.asPtr<IDevicePrivate>()->removeStreamingOption(String("daq.ns"));
    checkErrorInfo(errCode);
    serverHandler->stopServer();
}

void NativeStreamingServerImpl::startReading()
{
    readThreadActive = true;
    this->readThread = std::thread([this]()
    {
        this->startReadThread();
        LOG_I("Reading thread finished");
    });
}

void NativeStreamingServerImpl::stopReading()
{
    readThreadActive = false;
    if (readThread.joinable())
    {
        readThread.join();
        LOG_I("Reading thread joined");
    }

    signalReaders.clear();
}

void NativeStreamingServerImpl::startReadThread()
{
    while (readThreadActive)
    {
        updateReaders();
        for (const auto& [signal, reader] : signalReaders)
        {
            PacketPtr packet = reader.read();
            while (packet.assigned())
            {
                serverHandler->sendPacket(signal, packet);
                packet = reader.read();
            }
        }

        std::this_thread::sleep_for(readThreadSleepTime);
    }
}

void NativeStreamingServerImpl::createReaders()
{
    signalReaders.clear();
    auto signals = rootDevice.getSignalsRecursive();

    for (const auto& signal : signals)
    {
        addReader(signal);
    }
}

void NativeStreamingServerImpl::updateReaders()
{
    std::scoped_lock lock(readersSync);
    while (!signalsToStartRead.empty())
    {
        addReader(signalsToStartRead.front());
        signalsToStartRead.pop();
    }
    while (!signalsToStopRead.empty())
    {
        removeReader(signalsToStopRead.front());
        signalsToStopRead.pop();
    }
}

void NativeStreamingServerImpl::addReader(SignalPtr signalToRead)
{
    auto it = std::find_if(signalReaders.begin(),
                           signalReaders.end(),
                           [&signalToRead](const std::pair<SignalPtr, PacketReaderPtr>& element)
                           {
                               return element.first == signalToRead;
                           });
    if (it != signalReaders.end())
        return;

    auto reader = PacketReader(signalToRead);
    signalReaders.push_back(std::pair<SignalPtr, PacketReaderPtr>({signalToRead, reader}));
}

void NativeStreamingServerImpl::removeReader(SignalPtr signalToRead)
{
    signalReaders.erase(std::remove_if(signalReaders.begin(),
                                       signalReaders.end(),
                                       [&signalToRead](std::pair<SignalPtr, PacketReaderPtr>& item)
                                       {
                                           return item.first == signalToRead;
                                       }));
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(
    INTERNAL_FACTORY, NativeStreamingServer, daq::IServer,
    daq::DevicePtr, rootDevice,
    PropertyObjectPtr, config,
    const ContextPtr&, context
)

END_NAMESPACE_OPENDAQ_NATIVE_STREAMING_SERVER_MODULE
