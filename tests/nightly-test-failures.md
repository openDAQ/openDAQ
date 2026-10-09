# Nightly test failures, 2026-08-18 to 2026-10-08

## Overview

An actual issue is a defect in openDAQ, LT or ws-streaming.

| # | Issue | Kind | CI failures | Verified here |
| --- | --- | --- | --- | --- |
| 1 | LT client loses signals published right after it connects | Actual issue | 3 | New test fails 10/10 before the fix and passes after. Two CI failures reproduced with their exact symptoms. |
| 2 | LT server calls ws-streaming off its I/O thread | Actual issue | 1 | New test fails 20/20 before and passes after. The CI test crashed here 1 time in 2400 before the fix, 0 after. |
| 3 | LT client teardown waits for the metadata-fetch timer | Actual issue | 0 | New test fails 10/10 before (4.7 s) and passes after. |
| 4 | ws-streaming client ignores a reset right after the upgrade | Actual issue | 0 | New test fails 30/30 before and passes after. |
| 5 | ws-streaming sends JSON-RPC requests to port 1 | Actual issue | 0 | New test fails 10/10 before and passes after. |

## Issues

### 1. LT client loses signals published right after it connects

#### Reason

The `WsStreaming` constructor connects and starts its I/O thread, and that thread publishes each signal through `onSignalAvailable` as soon as the signal's metadata arrives. `WsStreamingDevice` connects its slots to `onSignalAvailable` only after the constructor returns. A signal published in between is marked published with nobody listening and is never offered again. A value signal whose domain signal was lost this way then fails to register too, because the device cannot find its domain.

#### Failed in CI

| Date | CI job | Test | Symptom |
| --- | --- | --- | --- |
| 08-24 | `macos-26-armv8-ninja-appleclang-release` | `test_device_modules`: `StreamingTestForModernLt.DataPackets/1` | The process aborted: `NotFoundException: ... refers to unregistered domain signal`. |
| 09-22 | `windows-2022-x86_64-ninja-clang-release` | `test_ws_stream_cl_module`: `DeviceCompatibilityTest.ReadvertisedHiddenDomainSignalStartsNoNewFetch` | 0 signals instead of 2. |
| 09-29 | `macos-26-x86_64-ninja-appleclang-17-debug` | `test_device_modules`: `WebsocketModulesChannelTest.GetRemoteDeviceObjects/Tls` | 4 signals instead of 5. |

The 09-22 and 09-29 failures reproduce exactly when the pre-fix device connects its slots only after the first signal has published. On 09-22 the time signal is lost and the value signal then fails its domain lookup, which leaves 0 signals. On 09-29 the device `Time` signal is lost, which leaves 4 of 5. The 08-24 run used an older LT build (`08326f0`), which did not catch the error and so aborted. With a longer gap the same `refers to unregistered domain signal` error appears, so issue 1 is the likely cause there too.

#### Fix

The connection starts in a new `WsStreaming::connect()` instead of the constructor. The device connects its slots first and then calls `connect()`, and the module's streaming path calls `connect()` right after construction. Signals can arrive before the device constructor returns, so `onSignalAvailable` finds the domain signal in the device's own map instead of through `thisPtr()`, which would delete the half-built device. The new test passes 10/10, and the LT client (65 tests), LT server (22) and TLS integration (27) suites pass.

#### Commits

- LTStreamingModulesModern [`c8760e3`](https://github.com/openDAQ/LTStreamingModulesModern/commit/c8760e34bdf7f28491b184c244303cc273081258): Start the LT streaming connection after the device connects its slots
- openDAQ [`6da0824`](https://github.com/openDAQ/openDAQ/commit/6da082465f9777cd50ed88e724d623234d5c9324): Start the LT streaming connection after the device connects its slots

#### Reproducing test

`NoSignalIsPublishedBeforeTheOwnerListens` creates the streaming object against the fake LT peer, waits 500 ms as a slow owner would, and only then connects a slot and calls `connect()`. Both signals must reach the slot. Before the fix it fails 10/10 with 0 signals; that run leaves out the `connect()` call, which does not exist yet. The diff also holds the issue 3 test.

```diff
diff --git a/modules/websocket_streaming_client_module/tests/test_device_compatibility.cpp b/modules/websocket_streaming_client_module/tests/test_device_compatibility.cpp
--- a/modules/websocket_streaming_client_module/tests/test_device_compatibility.cpp
+++ b/modules/websocket_streaming_client_module/tests/test_device_compatibility.cpp
@@ -9,6 +9,7 @@
  * request handling.
  */
 
+#include <atomic>
 #include <chrono>
 #include <cstdint>
 #include <functional>
@@ -35,6 +36,7 @@
 #include <ws-streaming/detail/streaming_protocol.hpp>
 
 #include <testutils/testutils.h>
+#include <websocket_streaming/ws_streaming.h>
 #include <websocket_streaming_client_module/module_dll.h>
 
 #include <opendaq/context_factory.h>
@@ -474,6 +476,13 @@ auto buildStreamReader(const SignalPtr& signal)
         .build();
 }
 
+// Creates the streaming object on its own, without the device that normally owns it
+StreamingPtr createStreaming(const FakeLtPeer& peer)
+{
+    return createWithImplementation<IStreaming, websocket_streaming::WsStreaming>(
+        String("daq.lt://127.0.0.1:" + std::to_string(peer.port()) + "/"), NullContext(), nullptr);
+}
+
 }  // namespace
 
 using DeviceCompatibilityTest = testing::Test;
@@ -660,3 +669,40 @@ TEST_F(DeviceCompatibilityTest, SignalPublishesWithoutDomainWhenMetadataNeverArr
     ASSERT_TRUE(valueSignal.assigned());
     ASSERT_FALSE(valueSignal.getDomainSignal().assigned());
 }
+
+// The device connects its slots after creating the streaming object; a signal published before is lost
+TEST_F(DeviceCompatibilityTest, NoSignalIsPublishedBeforeTheOwnerListens)
+{
+    FakeLtPeer peer({});
+    auto streaming = createStreaming(peer);
+    auto& wsStreaming = static_cast<websocket_streaming::WsStreaming&>(*streaming.getObject());
+
+    // an owner slower than the peer, which has answered the metadata fetch by now
+    std::this_thread::sleep_for(500ms);
+
+    std::atomic<unsigned> published{0};
+    boost::signals2::scoped_connection slot = wsStreaming.onSignalAvailable.connect(
+        [&published](wss::remote_signal_ptr, wss::remote_signal_ptr, const DataDescriptorPtr&) { ++published; });
+    wsStreaming.connect();
+
+    for (int i = 0; i < 100 && published < 2; ++i)
+        std::this_thread::sleep_for(50ms);
+    EXPECT_EQ(published, 2u);
+}
+
+// The metadata fetch arms a 1.5 s sweep timer; closing the connection must cancel it, not wait for it
+TEST_F(DeviceCompatibilityTest, ClosingDoesNotWaitForTheFetchSweep)
+{
+    FakeLtPeer peer({});
+    auto streaming = createStreaming(peer);
+    static_cast<websocket_streaming::WsStreaming&>(*streaming.getObject()).connect();
+
+    for (int i = 0; i < 100 && peer.subscribeRequestCount() == 0; ++i)
+        std::this_thread::sleep_for(10ms);
+    ASSERT_EQ(peer.subscribeRequestCount(), 1u);
+
+    const auto start = std::chrono::steady_clock::now();
+    streaming.release();
+    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start);
+    EXPECT_LT(elapsed.count(), 2000);
+}
```

#### Fix diff

```diff
diff --git a/modules/websocket_streaming_client_module/src/websocket_streaming_client_module_impl.cpp b/modules/websocket_streaming_client_module/src/websocket_streaming_client_module_impl.cpp
--- a/modules/websocket_streaming_client_module/src/websocket_streaming_client_module_impl.cpp
+++ b/modules/websocket_streaming_client_module/src/websocket_streaming_client_module_impl.cpp
@@ -191,7 +191,9 @@ StreamingPtr WebsocketStreamingClientModule::onCreateStreaming(const StringPtr&
         streamingConfig = createDefaultStreamingConfig(formNewStyleConnectionString(connectionString));
 
     const StringPtr str = formConnectionString(connectionString, streamingConfig);
-    return createWithImplementation<IStreaming, WsStreaming>(str, context, streamingConfig);
+    auto streaming = createWithImplementation<IStreaming, WsStreaming>(str, context, streamingConfig);
+    reinterpret_cast<WsStreaming*>(streaming.getObject())->connect();
+    return streaming;
 }
 
 Bool WebsocketStreamingClientModule::onCompleteServerCapability(const ServerCapabilityPtr& source, const ServerCapabilityConfigPtr& target)
diff --git a/modules/websocket_streaming_client_module/tests/test_websocket_streaming_client_module.cpp b/modules/websocket_streaming_client_module/tests/test_websocket_streaming_client_module.cpp
--- a/modules/websocket_streaming_client_module/tests/test_websocket_streaming_client_module.cpp
+++ b/modules/websocket_streaming_client_module/tests/test_websocket_streaming_client_module.cpp
@@ -311,9 +311,8 @@ TEST_F(WebsocketStreamingClientModuleTest, InsecureStreamingCompletesNullConfig)
 {
     // The plain channel completes a missing configuration too, and so fails on the unreachable
     // peer rather than on the configuration.
-    ASSERT_THROW((createWithImplementation<IStreaming, WsStreaming>(
-                      String("daq.lt://127.0.0.1:1/"), NullContext(), nullptr)),
-                 NotFoundException);
+    auto streaming = createWithImplementation<IStreaming, WsStreaming>(String("daq.lt://127.0.0.1:1/"), NullContext(), nullptr);
+    ASSERT_THROW(reinterpret_cast<WsStreaming*>(streaming.getObject())->connect(), NotFoundException);
 }
 
 TEST_F(WebsocketStreamingClientModuleTest, DefaultInsecureStreamingConfig)
diff --git a/shared/libraries/websocket_streaming/include/websocket_streaming/ws_streaming.h b/shared/libraries/websocket_streaming/include/websocket_streaming/ws_streaming.h
--- a/shared/libraries/websocket_streaming/include/websocket_streaming/ws_streaming.h
+++ b/shared/libraries/websocket_streaming/include/websocket_streaming/ws_streaming.h
@@ -106,7 +106,7 @@ class WsStreaming : public Streaming
     public:
 
         /*!
-         * @brief Constructs a streaming object and initiates a connection to the remote peer.
+         * @brief Constructs a streaming object. The connection starts with connect().
          *
          * @param connectionString The openDAQ connection string, which must use the `daq.lt://`
          *     prefix. The remote peer address and TCP port number are parsed from the connection
@@ -123,6 +123,17 @@ class WsStreaming : public Streaming
          */
         ~WsStreaming();
 
+        /*!
+         * @brief Connects to the remote peer and waits until the connection is established.
+         *
+         * Connect the slots of onSignalAvailable and onSignalUnavailable first: signals are
+         * published as soon as the connection is up, and each one is published only once.
+         *
+         * @throws NotFoundException The peer cannot be reached.
+         * @throws AuthenticationFailedException The TLS handshake failed.
+         */
+        void connect();
+
         /*!
          * @brief An event raised when a signal becomes available.
          *
@@ -208,6 +219,9 @@ class WsStreaming : public Streaming
         void onInitialFetchSweep(const boost::system::error_code& ec);
         void onInitialFetchResubscribe(const boost::system::error_code& ec);
 
+        std::string wsConnectionString;
+        bool isSecureChannel = false;
+
         boost::asio::io_context ioContext;
         std::thread thread;
 
diff --git a/shared/libraries/websocket_streaming/src/ws_streaming.cpp b/shared/libraries/websocket_streaming/src/ws_streaming.cpp
--- a/shared/libraries/websocket_streaming/src/ws_streaming.cpp
+++ b/shared/libraries/websocket_streaming/src/ws_streaming.cpp
@@ -114,12 +114,12 @@ WsStreaming::WsStreaming(
     // The ws-streaming library wants a URL like ws://1.2.3.4:7418/foo.
     // So we simply need to replace the daq.lt:// prefix with ws://
     // and daq.lts:// with wss:// for secure channel
-    auto wsConnectionString = connectionString.toStdString();
+    wsConnectionString = connectionString.toStdString();
     boost::replace_all(wsConnectionString, "daq.lt://", "ws://");
     boost::replace_all(wsConnectionString, "daq.ws://", "ws://");
     boost::replace_all(wsConnectionString, "daq.lts://", "wss://");
     boost::replace_all(wsConnectionString, "daq.wss://", "wss://");
-    bool isSecureChannel = wsConnectionString.find("wss://") != std::string::npos;
+    isSecureChannel = wsConnectionString.find("wss://") != std::string::npos;
 
 #if !DAQMODULES_LT_STREAMING_ENABLE_TLS
     if (isSecureChannel)
@@ -192,7 +192,10 @@ WsStreaming::WsStreaming(
     }
 
 #endif
+}
 
+void WsStreaming::connect()
+{
     // Start the ws-streaming connection attempt.
     LOG_I("Connecting to {}", wsConnectionString);
     wsClient.async_connect(wsConnectionString,
@@ -240,6 +243,10 @@ WsStreaming::WsStreaming(
 
 WsStreaming::~WsStreaming()
 {
+    // connect() was not called, or it failed and has already stopped the thread
+    if (!thread.joinable())
+        return;
+
     LOG_I("Closing streaming connection and stopping Boost.Asio I/O context thread");
 
     // Tear the connection down on the I/O context's thread. The ws-streaming peer is not thread-safe,
diff --git a/shared/libraries/websocket_streaming/src/ws_streaming_device.cpp b/shared/libraries/websocket_streaming/src/ws_streaming_device.cpp
--- a/shared/libraries/websocket_streaming/src/ws_streaming_device.cpp
+++ b/shared/libraries/websocket_streaming/src/ws_streaming_device.cpp
@@ -88,6 +88,7 @@ WsStreamingDevice::WsStreamingDevice(
 
     streamingEvents.emplace_back(wsStreaming.onSignalAvailable.connect(std::bind(&WsStreamingDevice::onSignalAvailable, this, _1, _2, _3)));
     streamingEvents.emplace_back(wsStreaming.onSignalUnavailable.connect(std::bind(&WsStreamingDevice::onSignalUnavailable, this, _1)));
+    wsStreaming.connect();
 }
 
 PropertyObjectPtr WsStreamingDevice::createDefaultConfig()
@@ -135,13 +136,12 @@ void WsStreamingDevice::onSignalAvailable(
 
     if (domainSignal)
     {
-        auto localId = WsStreamingSignal::createLocalId(domainSignal->id());
-        for (const auto& s : thisPtr<daq::DevicePtr>().getSignals())
-            if (s.getLocalId() == localId)
-                openDaqDomainSignal = s;
-        if (!openDaqDomainSignal.assigned())
+        // no thisPtr() here: this runs while the constructor is still connecting
+        auto it = streamingSignals.find(domainSignal->id());
+        if (it == streamingSignals.end())
             DAQ_THROW_EXCEPTION(NotFoundException,
                 "Streaming signal '{}' refers to unregistered domain signal '{}'", signal->id(), domainSignal->id());
+        openDaqDomainSignal = it->second;
     }
 
     auto openDaqSignal = createWithImplementation<IMirroredSignalPrivate, WsStreamingSignal>(
```

### 2. LT server calls ws-streaming off its I/O thread

#### Reason

A core event runs the LT server's `rescan()` on the thread that raised the event. `rescan()` adds and removes signals on the ws-streaming server, which sends the change to every client from that thread while the server's I/O thread writes to the same connections. ws-streaming is not thread-safe, so the two threads corrupt the TLS write queue.

#### Failed in CI

| Date | CI job | Test | Symptom |
| --- | --- | --- | --- |
| 08-28 | `ubuntu-24.04-x86_64-ninja-gcc-14-debug` | `test_device_modules`: `WebsocketModulesChannelTest.UpdateRemoveSignals/Tls` | Segfault right after the test removed a channel. |

The CI log has no stack. The same test on the same code, run 2400 times here before the fix, crashed once on the server's I/O thread in `wss::detail::peer_tls::finish_write()`, right after the test thread pushed the channel removal into that connection. It also failed 12 times in a second way: the client lost all its signals, which looks like the same race breaking the connection. With the fix, 2400 runs had no crash and no failure.

#### Fix

`rescan()` still reads the device tree on the calling thread, but only into a list of snapshots, and posts registering them to the I/O thread (`applySignals()`). A snapshot can arrive after a newer one, so registering skips signals that are already removed. The I/O thread must not take the tree's locks: removing a server joins that thread while the tree is locked. A first version that posted the whole `rescan()` deadlocked the TLS integration suite that way. The new test passes 20/20, and the LT server suite and the TLS integration suite (27 tests) pass.

[NEEDS-DECISION] D4: the subscribe handler still creates its listener on the I/O thread, and connecting the listener locks the signal. A client that subscribes while the server is being removed can deadlock the same way.

#### Commits

- LTStreamingModulesModern [`3c39b21`](https://github.com/openDAQ/LTStreamingModulesModern/commit/3c39b21c5af15c460b125521493769f0e341ac18): Register LT server signals with ws-streaming on its I/O thread
- openDAQ [`8c8d175`](https://github.com/openDAQ/openDAQ/commit/8c8d175be6ef4b0fc4a61a5c1e6069c6bf8303a4): Register LT server signals with ws-streaming on its I/O thread

#### Reproducing test

`SignalAddedOnAnotherThreadIsAnnouncedFromTheServerThread` connects a ws-streaming client, blocks the server's I/O thread, adds a signal from the test thread, and checks that the client hears nothing while that thread is blocked. Before the fix the test thread sends the announcement straight to the socket, and the test fails 20/20.

```diff
diff --git a/modules/websocket_streaming_server_module/tests/test_websocket_streaming_server_module.cpp b/modules/websocket_streaming_server_module/tests/test_websocket_streaming_server_module.cpp
--- a/modules/websocket_streaming_server_module/tests/test_websocket_streaming_server_module.cpp
+++ b/modules/websocket_streaming_server_module/tests/test_websocket_streaming_server_module.cpp
@@ -1,5 +1,22 @@
 #include "test_websocket_streaming_server_module.h"
 
+#include <future>
+#include <mutex>
+#include <set>
+#include <thread>
+
+#include <boost/asio/io_context.hpp>
+#include <boost/asio/post.hpp>
+
+#include <opendaq/data_descriptor_factory.h>
+#include <opendaq/device_impl.h>
+#include <opendaq/device_info_factory.h>
+
+#include <ws-streaming/client.hpp>
+#include <ws-streaming/connection.hpp>
+
+using namespace std::chrono_literals;
+
 TEST_F(WsStreamingServerModuleTest, CreateModule)
 {
     IModule* module = nullptr;
@@ -356,3 +373,110 @@ TEST_F(WsStreamingServerModuleTest, ProviderOptionsIgnoreUnknownKeys)
     ASSERT_FALSE(config.hasProperty("NoSuchProperty"));
     ASSERT_EQ(config.getPropertyValue(PROPERTY_WS_STREAMING_PORT_SERVER), 7661);
 }
+
+namespace
+{
+
+// A root device whose signals the test adds from its own thread.
+class SignalAddingDevice : public Device
+{
+public:
+    explicit SignalAddingDevice(const ContextPtr& context)
+        : Device(context, nullptr, "dev")
+    {
+    }
+
+    std::string addTestSignal(const std::string& localId)
+    {
+        return createAndAddSignal(localId, DataDescriptorBuilder().setSampleType(SampleType::Float64).build())
+            .getGlobalId()
+            .toStdString();
+    }
+
+protected:
+    DeviceInfoPtr onGetInfo() override
+    {
+        return DeviceInfo("daqtest://dev");
+    }
+};
+
+}
+
+// The ws-streaming server is not thread-safe: a signal added on another thread is registered on its I/O thread
+TEST_F(WsStreamingServerModuleTest, SignalAddedOnAnotherThreadIsAnnouncedFromTheServerThread)
+{
+    const auto context = NullContext();
+    const auto module = CreateModule(context);
+    auto device = createWithImplementation<IDevice, SignalAddingDevice>(context);
+    auto& deviceImpl = static_cast<SignalAddingDevice&>(*device.getObject());
+    device.asPtr<IPropertyObjectInternal>().enableCoreEventTrigger();
+    const auto initialId = deviceImpl.addTestSignal("initial");
+
+    auto server = createWithImplementation<IServer, WsStreamingServer>(device, CreateWsOnlyConfig(module, 7662), context);
+    auto& wsServer = static_cast<WsStreamingServer&>(*server.getObject()).getWsServer();
+
+    boost::asio::io_context clientIoc;
+    wss::client client{clientIoc.get_executor()};
+    wss::connection_ptr connection;
+    boost::signals2::scoped_connection onAvailable;
+    std::mutex mutex;
+    std::set<std::string> available;
+
+    client.async_connect("ws://127.0.0.1:7662/",
+        [&](const boost::system::error_code& ec, wss::connection_ptr c)
+        {
+            if (ec)
+                return;
+            connection = c;
+            onAvailable = c->on_available.connect(
+                [&](wss::remote_signal_ptr signal)
+                {
+                    std::scoped_lock lock(mutex);
+                    available.insert(signal->id());
+                });
+        });
+    std::thread clientThread{[&] { clientIoc.run(); }};
+
+    const auto isAvailable = [&](const std::string& id)
+    {
+        std::scoped_lock lock(mutex);
+        return available.count(id) > 0;
+    };
+    const auto waitAvailable = [&](const std::string& id)
+    {
+        for (int i = 0; i < 100 && !isAvailable(id); ++i)
+            std::this_thread::sleep_for(50ms);
+        return isAvailable(id);
+    };
+
+    EXPECT_TRUE(waitAvailable(initialId));
+
+    std::promise<void> entered;
+    std::promise<void> release;
+    boost::asio::post(wsServer.executor(),
+        [&entered, released = release.get_future()]
+        {
+            entered.set_value();
+            released.wait();
+        });
+    entered.get_future().wait();
+
+    const auto addedId = deviceImpl.addTestSignal("added");
+    std::this_thread::sleep_for(300ms);
+    const bool announcedWhileServerThreadBlocked = isAvailable(addedId);
+    release.set_value();
+
+    EXPECT_FALSE(announcedWhileServerThreadBlocked) << "the signal was announced from the thread that added it";
+    EXPECT_TRUE(waitAvailable(addedId));
+
+    boost::asio::post(clientIoc,
+        [&]
+        {
+            onAvailable.disconnect();
+            if (connection)
+                connection->close();
+            clientIoc.stop();
+        });
+    clientThread.join();
+    server.stop();
+}
```

#### Fix diff

```diff
diff --git a/shared/libraries/websocket_streaming/include/websocket_streaming/ws_streaming_server.h b/shared/libraries/websocket_streaming/include/websocket_streaming/ws_streaming_server.h
--- a/shared/libraries/websocket_streaming/include/websocket_streaming/ws_streaming_server.h
+++ b/shared/libraries/websocket_streaming/include/websocket_streaming/ws_streaming_server.h
@@ -20,6 +20,7 @@
 #include <memory>
 #include <string>
 #include <thread>
+#include <vector>
 
 #include <boost/asio/io_context.hpp>
 #include <boost/signals2/connection.hpp>
@@ -81,7 +82,17 @@ class WsStreamingServer : public Server
         void addCapability();
         void removeCapability();
 
-        void createListener(const SignalPtr& signal);
+        // A signal as read from the device tree, so the I/O thread can register it without the tree's locks
+        struct SignalSnapshot
+        {
+            SignalPtr signal;
+            SignalPtr domainSignal;
+            std::string id;
+            wss::metadata metadata;
+        };
+
+        std::vector<SignalSnapshot> scanSignals();
+        void createListener(const SignalSnapshot& snapshot);
 
         void onClientConnected(
             const wss::connection_ptr& connection);
@@ -111,6 +122,7 @@ class WsStreamingServer : public Server
             CoreEventArgsPtr& args);
 
         void rescan();
+        void applySignals(const std::vector<SignalSnapshot>& snapshots);
 
     private:
 
diff --git a/shared/libraries/websocket_streaming/src/ws_streaming_server.cpp b/shared/libraries/websocket_streaming/src/ws_streaming_server.cpp
--- a/shared/libraries/websocket_streaming/src/ws_streaming_server.cpp
+++ b/shared/libraries/websocket_streaming/src/ws_streaming_server.cpp
@@ -438,21 +438,47 @@ void WsStreamingServer::removeCapability()
         info.asPtr<IDeviceInfoInternal>(true).removeServerCapability(CONST_LTS_STREAMING_ID);
 }
 
-void WsStreamingServer::createListener(const SignalPtr& signal)
+std::vector<WsStreamingServer::SignalSnapshot> WsStreamingServer::scanSignals()
 {
-    SignalPtr domainSignal = signal.getDomainSignal();
+    std::vector<SignalSnapshot> snapshots;
 
-    if (domainSignal.assigned())
-        createListener(domainSignal);
+    // domain signals are listed before the signals that use them
+    const std::function<void(const SignalPtr&)> add = [&](const SignalPtr& signal)
+    {
+        SignalPtr domainSignal = signal.getDomainSignal();
+        if (domainSignal.assigned())
+            add(domainSignal);
+
+        snapshots.push_back({signal,
+                             domainSignal,
+                             signal.getGlobalId(),
+                             daq::websocket_streaming::descriptorToMetadata(signal, signal.getDescriptor())});
+    };
+
+    auto items = _rootDevice.getItems(search::Recursive(search::Any()));
+    for (const auto& item : items)
+        if (auto signal = item.asPtrOrNull<daq::ISignal>(); signal.assigned() && signal.getDescriptor().assigned())
+            add(signal);
+
+    return snapshots;
+}
+
+void WsStreamingServer::createListener(const SignalSnapshot& snapshot)
+{
+    const SignalPtr& signal = snapshot.signal;
+
+    // the snapshot may predate a removal on another thread
+    if (signal.isRemoved())
+        return;
 
-    auto it = _localSignals.find(signal.getGlobalId());
+    auto it = _localSignals.find(snapshot.id);
     if (it != _localSignals.end())
     {
         // Check if the signal has acquired a new/different domain signal since it was added. If
         // so, we need to unregister the local_signal from ws-streaming and re-register it so that
         // the domain tab table is linked correctly.
 
-        if (domainSignal != it->second.domainSignal)
+        if (snapshot.domainSignal.getObject() != it->second.domainSignal.getObject())
         {
             _server.remove_local_signal(*it->second.localSignal);
             _localSignals.erase(it);
@@ -467,19 +493,19 @@ void WsStreamingServer::createListener(const SignalPtr& signal)
     auto& streamableSignal = _localSignals.emplace(
             std::piecewise_construct,
             std::forward_as_tuple(
-                signal.getGlobalId()),
+                snapshot.id),
             std::forward_as_tuple(
-                signal.getGlobalId(),
-                daq::websocket_streaming::descriptorToMetadata(signal, signal.getDescriptor()),
+                snapshot.id,
+                snapshot.metadata,
                 signal))
         .first->second;
 
-    streamableSignal.domainSignal = domainSignal;
+    streamableSignal.domainSignal = snapshot.domainSignal;
 
     streamableSignal.localSignal->on_subscribed.connect([
         =,
         &streamableSignal,
-        signal_id = signal.getGlobalId().toStdString()
+        signal_id = snapshot.id
     ]()
     {
         streamableSignal.listener = createWithImplementation<IInputPortNotifications, WsStreamingListener>(
@@ -493,7 +519,7 @@ void WsStreamingServer::createListener(const SignalPtr& signal)
     streamableSignal.localSignal->on_unsubscribed.connect([
         =,
         &streamableSignal,
-        signal_id = signal.getGlobalId().toStdString()
+        signal_id = snapshot.id
     ]()
     {
         streamableSignal.listener.release();
@@ -585,7 +611,13 @@ void WsStreamingServer::onAttributeChanged(
     rescan();
 }
 
+// The I/O thread owns the ws-streaming server but must not lock the tree: removing the server joins it under that lock
 void WsStreamingServer::rescan()
+{
+    boost::asio::post(_ioc, [this, snapshots = scanSignals()] { applySignals(snapshots); });
+}
+
+void WsStreamingServer::applySignals(const std::vector<SignalSnapshot>& snapshots)
 {
     auto it = _localSignals.begin();
     while (it != _localSignals.end())
@@ -601,10 +633,8 @@ void WsStreamingServer::rescan()
             ++it;
     }
 
-    auto items = _rootDevice.getItems(search::Recursive(search::Any()));
-    for (const auto& item : items)
-        if (auto signal = item.asPtrOrNull<daq::ISignal>(); signal.assigned() && signal.getDescriptor().assigned())
-            createListener(signal);
+    for (const auto& snapshot : snapshots)
+        createListener(snapshot);
 }
 
 END_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING
```

### 3. LT client teardown waits for the metadata-fetch timer

#### Reason

The `WsStreaming` destructor closes the connection and joins the I/O thread, but never cancels `initialFetchTimer`. The join therefore waits for the timer's sweeps, 1.5 s each: about 3 s after a normal connection, up to 4.7 s when metadata is still pending.

#### Failed in CI

No CI failure. On 09-29 (`macos-26-x86_64-ninja-appleclang-17-debug`) the client in `LtStreamingTlsTest.BothChannelsSecureClient` closed at 24.619 and the next test started 3.0 s later, which is two sweeps of a held subscription.

#### Fix

Cancel the timer when the connection closes. The new test passes 10/10.

#### Commits

- LTStreamingModulesModern [`7df33d0`](https://github.com/openDAQ/LTStreamingModulesModern/commit/7df33d0f92d1bbc968c64ee5881b2c477753e3e0): Cancel the LT metadata-fetch timer when the connection closes
- openDAQ [`8f12b0f`](https://github.com/openDAQ/openDAQ/commit/8f12b0f9908256e78f2773ea2958e28da67d818a): Cancel the LT metadata-fetch timer when the connection closes

#### Reproducing test

`ClosingDoesNotWaitForTheFetchSweep` (in the issue 1 test diff) releases the streaming object right after the metadata fetch started and requires it to finish within 2000 ms. Before the fix it takes 4692 ms and fails 10/10; after it, 2 ms. It closes while the metadata is still pending, so it exercises the retry path (4.7 s), while the CI case was the hold path (3.0 s); both come from the same uncancelled timer.

#### Fix diff

```diff
diff --git a/shared/libraries/websocket_streaming/src/ws_streaming.cpp b/shared/libraries/websocket_streaming/src/ws_streaming.cpp
--- a/shared/libraries/websocket_streaming/src/ws_streaming.cpp
+++ b/shared/libraries/websocket_streaming/src/ws_streaming.cpp
@@ -252,6 +252,7 @@ WsStreaming::~WsStreaming()
     {
         onAvailableConnection.disconnect();
         onUnavailableConnection.disconnect();
+        initialFetchTimer.cancel();
 
         for (auto& [id, entry] : signals)
         {
```

### 4. ws-streaming client ignores a reset right after the upgrade

#### Reason

After the HTTP upgrade succeeds, the client reads the peer's address to name the connection. If the peer has already reset the connection, `remote_endpoint()` throws, and the handler returns without calling the connect handler. `WsStreaming::connect()` then waits forever. The same code exists for `ws://`, `wss://` and plain TCP.

#### Failed in CI

No CI failure. Found while reading the code for issue 1.

#### Fix

Use the non-throwing `remote_endpoint(ec)` and pass the error to the connect handler, in all three places. The new test passes 30/30, and the whole ws-streaming suite passes. This goes upstream in ws-streaming, with a release and a pin bump in LT.

#### Commits

- ws-streaming [`999e276`](https://github.com/openDAQ/ws-streaming/commit/999e27684beb15f7e900571d9b2f72fa76680b15): Report a connection reset right after the WebSocket upgrade
- LTStreamingModulesModern [`83d92a0`](https://github.com/openDAQ/LTStreamingModulesModern/commit/83d92a0224db8ce595b2b994a696af90672d9b59): Report a connection reset right after the WebSocket upgrade
- openDAQ [`b437fb3`](https://github.com/openDAQ/openDAQ/commit/b437fb316160a83f8664de017a442f0679da9ffd): Report a connection reset right after the WebSocket upgrade

#### Reproducing test

A raw TCP server answers the upgrade with `101` and resets the connection before the client reads the response. The connect handler must be called. Before the fix it never is, and the test fails 30/30. The two new ws-streaming test files are registered in `tests/CMakeLists.txt`.

```diff
diff --git a/tests/test_client.cpp b/tests/test_client.cpp
new file mode 100644
--- /dev/null
+++ b/tests/test_client.cpp
@@ -0,0 +1,66 @@
+// Tests for establishing a connection with wss::client.
+
+#include <chrono>
+#include <future>
+#include <optional>
+#include <string>
+#include <thread>
+
+#include <boost/asio/buffer.hpp>
+#include <boost/asio/io_context.hpp>
+#include <boost/asio/ip/tcp.hpp>
+#include <boost/asio/read_until.hpp>
+#include <boost/asio/streambuf.hpp>
+#include <boost/asio/write.hpp>
+
+#include <gtest/gtest.h>
+
+#include <ws-streaming/client.hpp>
+#include <ws-streaming/connection.hpp>
+
+using boost::asio::ip::tcp;
+
+// The server resets the connection right after the upgrade, so the socket has no peer address any more
+TEST(Client, ReportsAConnectionResetRightAfterTheUpgrade)
+{
+    boost::asio::io_context server_ioc;
+    tcp::acceptor acceptor{server_ioc, {boost::asio::ip::make_address("127.0.0.1"), 0}};
+    std::promise<void> request_received;
+    std::promise<void> client_paused;
+
+    std::thread server{[&]
+    {
+        tcp::socket socket{server_ioc};
+        acceptor.accept(socket);
+        boost::asio::streambuf request;
+        boost::asio::read_until(socket, request, "\r\n\r\n");
+        request_received.set_value();
+        client_paused.get_future().wait();
+
+        boost::asio::write(socket, boost::asio::buffer(std::string{
+            "HTTP/1.1 101 Switching Protocols\r\n"
+            "Upgrade: websocket\r\n"
+            "Connection: Upgrade\r\n\r\n"}));
+        socket.set_option(tcp::socket::linger{true, 0});
+        socket.close();
+    }};
+
+    boost::asio::io_context ioc;
+    wss::client client{ioc.get_executor()};
+    std::optional<boost::system::error_code> result;
+    client.async_connect(
+        "ws://127.0.0.1:" + std::to_string(acceptor.local_endpoint().port()) + "/",
+        [&](const boost::system::error_code& ec, wss::connection_ptr) { result = ec; });
+
+    // the client sends the request, then reads only once the response and the reset have both arrived
+    auto received = request_received.get_future();
+    while (received.wait_for(std::chrono::milliseconds(1)) != std::future_status::ready)
+        ioc.poll();
+    client_paused.set_value();
+    server.join();
+
+    ioc.run_for(std::chrono::seconds(2));
+
+    ASSERT_TRUE(result.has_value()) << "the connect handler was never called";
+    EXPECT_TRUE(*result);
+}
```

```diff
diff --git a/tests/CMakeLists.txt b/tests/CMakeLists.txt
--- a/tests/CMakeLists.txt
+++ b/tests/CMakeLists.txt
@@ -1,5 +1,7 @@
 set(sources
     ./test_base64.cpp
+    ./test_client.cpp
+    ./test_command_interface_client_factory.cpp
     ./test_dynamic_buffer.cpp
     ./test_metadata.cpp
     ./test_peer_limits.cpp
```

#### Fix diff

```diff
diff --git a/src/client.cpp b/src/client.cpp
--- a/src/client.cpp
+++ b/src/client.cpp
@@ -109,17 +109,13 @@ void wss::client::async_connect(
                 if (response.result() != boost::beast::http::status::switching_protocols)
                     return handler(boost::beast::http::error::bad_status, {});
 
-                std::string connection_local_stream_id;
-                try
-                {
-                    auto remote_endpoint = stream.socket().remote_endpoint();
-                    connection_local_stream_id = remote_endpoint.address().to_string()
-                                                 + ":" + std::to_string(remote_endpoint.port());
-                }
-                catch (const std::exception& /*e*/)
-                {
-                    return;
-                }
+                boost::system::error_code endpoint_ec;
+                auto remote_endpoint = stream.socket().remote_endpoint(endpoint_ec);
+                if (endpoint_ec)
+                    return handler(endpoint_ec, {});
+
+                std::string connection_local_stream_id = remote_endpoint.address().to_string()
+                    + ":" + std::to_string(remote_endpoint.port());
 
                 auto connection = std::make_shared<wss::connection>(
                     stream.release_socket(),
@@ -156,17 +152,13 @@ void wss::client::async_connect(
                 if (response.result() != boost::beast::http::status::switching_protocols)
                     return handler(boost::beast::http::error::bad_status, {});
 
-                std::string connection_local_stream_id;
-                try
-                {
-                    auto remote_endpoint = stream.next_layer().socket().remote_endpoint();
-                    connection_local_stream_id = remote_endpoint.address().to_string()
-                                                 + ":" + std::to_string(remote_endpoint.port());
-                }
-                catch (const std::exception& /*e*/)
-                {
-                    return;
-                }
+                boost::system::error_code endpoint_ec;
+                auto remote_endpoint = stream.next_layer().socket().remote_endpoint(endpoint_ec);
+                if (endpoint_ec)
+                    return handler(endpoint_ec, {});
+
+                std::string connection_local_stream_id = remote_endpoint.address().to_string()
+                    + ":" + std::to_string(remote_endpoint.port());
 
                 stream.next_layer().expires_never();
 
@@ -219,17 +211,13 @@ void wss::client::async_connect(
                         if (ec)
                             return handler(ec, {});
 
-                        std::string connection_local_stream_id;
-                        try
-                        {
-                            auto remote_endpoint = socket->remote_endpoint();
-                            connection_local_stream_id = remote_endpoint.address().to_string()
-                                                         + ":" + std::to_string(remote_endpoint.port());
-                        }
-                        catch (const std::exception& /*e*/)
-                        {
-                            return;
-                        }
+                        boost::system::error_code endpoint_ec;
+                        auto remote_endpoint = socket->remote_endpoint(endpoint_ec);
+                        if (endpoint_ec)
+                            return handler(endpoint_ec, {});
+
+                        std::string connection_local_stream_id = remote_endpoint.address().to_string()
+                            + ":" + std::to_string(remote_endpoint.port());
 
                         auto connection = std::make_shared<wss::connection>(
                             std::move(*socket),
```

### 5. ws-streaming sends JSON-RPC requests to port 1

#### Reason

When a server advertises its `jsonrpc-http` port as a number, the client converts `port.is_number_integer()`, which is `true`, so every request goes to port 1. The openDAQ server answers commands in-band, so openDAQ does not use this path today.

#### Failed in CI

No CI failure. Found while reading the code for issue 1.

#### Fix

Read the integer. The new test passes 10/10. This goes upstream with issue 4.

#### Commits

- ws-streaming [`4abcc1a`](https://github.com/openDAQ/ws-streaming/commit/4abcc1a539f5bbbbd85d1261c503e16bacc11779): Send JSON-RPC requests to a numeric HTTP port the server advertises
- LTStreamingModulesModern [`2cd1ab9`](https://github.com/openDAQ/LTStreamingModulesModern/commit/2cd1ab90bd20202f2de88d4d8a84da63fb429386): Send JSON-RPC requests to a numeric HTTP port the server advertises
- openDAQ [`0484db4`](https://github.com/openDAQ/openDAQ/commit/0484db498c907285f92acd58ca3f9d3559a5c3a2): Send JSON-RPC requests to a numeric HTTP port the server advertises

#### Reproducing test

The factory is given a numeric port, and a request must arrive on it. Before the fix the request goes to port 1, and the test fails 10/10.

```diff
diff --git a/tests/test_command_interface_client_factory.cpp b/tests/test_command_interface_client_factory.cpp
new file mode 100644
--- /dev/null
+++ b/tests/test_command_interface_client_factory.cpp
@@ -0,0 +1,49 @@
+// Tests for choosing the command interface a server advertises.
+
+#include <chrono>
+#include <memory>
+
+#include <boost/asio/io_context.hpp>
+#include <boost/asio/ip/tcp.hpp>
+
+#include <gtest/gtest.h>
+
+#include <nlohmann/json.hpp>
+
+#include <ws-streaming/detail/command_interface_client_factory.hpp>
+#include <ws-streaming/detail/peer.hpp>
+
+using boost::asio::ip::tcp;
+
+// The server may advertise the port of its HTTP JSON-RPC interface as a number.
+TEST(CommandInterfaceClientFactory, SendsRequestsToANumericJsonRpcHttpPort)
+{
+    boost::asio::io_context ioc;
+    auto loopback = boost::asio::ip::make_address("127.0.0.1");
+
+    tcp::acceptor streaming_acceptor{ioc, {loopback, 0}};
+    tcp::socket streaming_socket{ioc};
+    streaming_socket.connect(streaming_acceptor.local_endpoint());
+    tcp::socket streaming_server_side = streaming_acceptor.accept();
+    auto peer = std::make_shared<wss::detail::peer>(std::move(streaming_socket), true);
+
+    tcp::acceptor http_acceptor{ioc, {loopback, 0}};
+    bool accepted = false;
+    http_acceptor.async_accept([&](const boost::system::error_code& ec, tcp::socket) { accepted = !ec; });
+
+    nlohmann::json interfaces = {{"jsonrpc-http", {
+        {"httpMethod", "POST"},
+        {"httpPath", "/"},
+        {"httpVersion", "1.1"},
+        {"port", http_acceptor.local_endpoint().port()}}}};
+
+    auto client = wss::detail::command_interface_client_factory::create_client(interfaces, peer);
+    ASSERT_NE(client, nullptr);
+    client->async_request("subscribe", nlohmann::json::array({"signal"}), [](auto&&, auto&&) { });
+
+    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
+    while (!accepted && ioc.run_one_until(deadline)) { }
+
+    EXPECT_TRUE(accepted) << "no request reached the advertised port";
+    client->cancel();
+}
```

#### Fix diff

```diff
diff --git a/src/detail/command_interface_client_factory.cpp b/src/detail/command_interface_client_factory.cpp
--- a/src/detail/command_interface_client_factory.cpp
+++ b/src/detail/command_interface_client_factory.cpp
@@ -34,7 +34,7 @@ wss::detail::command_interface_client_factory::create_client(
     {
         std::string port;
         if (interfaces["jsonrpc-http"]["port"].is_number_integer())
-            port = std::to_string(interfaces["jsonrpc-http"]["port"].is_number_integer());
+            port = std::to_string(interfaces["jsonrpc-http"]["port"].get<std::int64_t>());
         else
             port = interfaces["jsonrpc-http"]["port"];
 
```
