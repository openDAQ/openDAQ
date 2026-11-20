#include <config_protocol/config_protocol_server.h>
#include <coreobjects/user_factory.h>
#include <coretypes/ctutils.h>
#include <coretypes/enumeration_factory.h>
#include <coretypes/json_deserializer_factory.h>
#include <coretypes/json_serializer_factory.h>
#include <gtest/gtest.h>
#include <opendaq/component_status_container_private_ptr.h>
#include <opendaq/core_opendaq_event_args_factory.h>
#include <opendaq/folder_config_ptr.h>
#include <opendaq/function_block_impl.h>
#include <opendaq/function_block_type_factory.h>
#include <opendaq/mock/advanced_components_setup_utils.h>
#include <atomic>
#include <chrono>
#include <deque>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

using namespace daq;
using namespace daq::config_protocol;

// Removes itself from its folder while it is serialized, as another thread could at that moment
class SelfRemovingFunctionBlock : public FunctionBlock
{
public:
    using FunctionBlock::FunctionBlock;

protected:
    void serializeCustomObjectValues(const SerializerPtr& serializer, bool forUpdate) override
    {
        FunctionBlock::serializeCustomObjectValues(serializer, forUpdate);
        const FolderConfigPtr folder = this->parent.getRef();
        folder.removeItem(this->borrowPtr<ComponentPtr>());
    }
};

// Is serialized until the test lets it go on, as a large component takes its time
class SlowlySerializedFunctionBlock : public FunctionBlock
{
public:
    SlowlySerializedFunctionBlock(const FunctionBlockTypePtr& type,
                                  const ContextPtr& context,
                                  const ComponentPtr& parent,
                                  const StringPtr& localId,
                                  std::atomic<bool>& serializing,
                                  std::atomic<bool>& proceed)
        : FunctionBlock(type, context, parent, localId)
        , serializing(serializing)
        , proceed(proceed)
    {
    }

protected:
    void serializeCustomObjectValues(const SerializerPtr& serializer, bool forUpdate) override
    {
        FunctionBlock::serializeCustomObjectValues(serializer, forUpdate);
        serializing = true;
        while (!proceed)
            std::this_thread::yield();
    }

    std::atomic<bool>& serializing;
    std::atomic<bool>& proceed;
};

// Threads change one device locally and over RPCs, which they hand to a config protocol server as the native server's worker pool does
class ConfigCoreEventConcurrencyTest : public testing::Test
{
protected:
    // Returns whether it made its change; a write to a device that another worker has not added yet makes none
    using Worker = std::function<bool(uint64_t iteration)>;

    // One client connection: the server and the notifications it sent, of which the JSON is kept only when asked for
    struct Connection
    {
        std::atomic<uint64_t> notifications{0};
        // Appended by the server threads that send, read by the test thread
        mutable std::mutex sentSync;
        std::vector<std::string> sent;
        std::unique_ptr<ConfigProtocolServer> server;
    };

    struct WorkerProgress
    {
        std::atomic<uint64_t> changes{0};
        std::atomic<bool> done{false};
    };

    // Everything the worker threads use; leaked together with the threads when they deadlock
    struct State
    {
        DevicePtr device;
        std::deque<Connection> connections;
        std::deque<WorkerProgress> workers;
        std::atomic<bool> stop{false};
        std::atomic<uint64_t> requestId{0};
        std::mutex errorSync;
        std::string error;
    };

    static constexpr uint64_t ChangesPerWorker = 50;
    static constexpr const char* NewDevicePath = "Dev/newDevice";
    static constexpr const char* NewChannelPath = "Dev/newDevice/IO/AI/Ch";
    static constexpr const char* RootChannelPath = "IO/AI/Ch";

    static std::shared_ptr<State> createState()
    {
        auto state = std::make_shared<State>();
        state->device = test_utils::createTestDevice();
        return state;
    }

    static Connection& connect(State& state, uint16_t protocolVersion = GetLatestConfigProtocolVersion(), bool keepJson = false)
    {
        auto& connection = state.connections.emplace_back();
        connection.server = std::make_unique<ConfigProtocolServer>(
            state.device,
            [&connection, keepJson](const PacketBuffer& packet)
            {
                ++connection.notifications;
                if (!keepJson)
                    return;

                std::string json = packet.parseServerNotification().toStdString();
                std::scoped_lock lock(connection.sentSync);
                connection.sent.push_back(std::move(json));
            },
            User("", ""),
            ClientType::Control,
            test_utils::dummyExtSigFolder(state.device.getContext()));
        connection.server->setProtocolVersion(protocolVersion);
        return connection;
    }

    static std::string globalId(State& state, const std::string& path = "")
    {
        const std::string rootId = state.device.getGlobalId().toStdString();
        return path.empty() ? rootId : rootId + "/" + path;
    }

    static ListPtr<IBaseObject> deserialize(State& state, const std::string& json)
    {
        return JsonDeserializer().deserialize(json, state.device.getContext().getTypeManager());
    }

    // Returns whether the server replied with success
    static bool callRpc(State& state, Connection& connection, const std::string& name, const ParamsDictPtr& params)
    {
        auto request = Dict<IString, IBaseObject>();
        request.set("Name", String(name));
        request.set("Params", params);

        const auto serializer = JsonSerializer();
        request.serialize(serializer);
        const auto json = serializer.getOutput();

        const auto packet = PacketBuffer::createRpcRequestOrReply(++state.requestId, json.getCharPtr(), json.getLength());
        const std::string reply = connection.server->processRequestAndGetReply(packet).parseRpcRequestOrReply().toStdString();
        return reply.find(R"({"key":"ErrorCode","value":0})") != std::string::npos;
    }

    // Set by the function block while it is serialized, and by the test to let it go on
    struct SlowSerialization
    {
        std::atomic<bool> serializing{false};
        std::atomic<bool> proceed{false};
    };

    // Adds a function block on the returned thread, which reports its ComponentAdded event and serializes it until proceed is set
    static std::thread addSlowlySerializedFunctionBlock(State& state, SlowSerialization& slow)
    {
        const FolderConfigPtr folder = state.device.getItem("FB");
        std::thread thread(
            [folder, &slow]
            {
                const auto type = FunctionBlockType("test", "test", "");
                folder.addItem(createWithImplementation<IFunctionBlock, SlowlySerializedFunctionBlock>(
                    type, folder.getContext(), folder, "fb", slow.serializing, slow.proceed));
            });

        while (!slow.serializing)
            std::this_thread::yield();
        return thread;
    }

    // A copy of the notifications the connection sent so far, in the order they were sent
    static std::vector<std::string> sentJson(const Connection& connection)
    {
        std::scoped_lock lock(connection.sentSync);
        return connection.sent;
    }

    // The position of the first event of the name in what the connection sent, with the notifications in the order they were sent
    static size_t sentPosition(const Connection& connection, const std::string& eventName)
    {
        std::string sent;
        for (const auto& json : sentJson(connection))
            sent += json;
        return sent.find(R"("name":")" + eventName + '"');
    }

    static ParamsDictPtr componentParams(State& state, const std::string& path = "")
    {
        auto params = Dict<IString, IBaseObject>();
        params.set("ComponentGlobalId", String(globalId(state, path)));
        return params;
    }

    static bool addDeviceOverRpc(State& state, Connection& connection)
    {
        auto params = componentParams(state);
        params.set("ConnectionString", String("mock://test"));
        return callRpc(state, connection, "AddDevice", params);
    }

    static bool removeDeviceOverRpc(State& state, Connection& connection)
    {
        auto params = componentParams(state);
        params.set("LocalId", String("newDevice"));
        return callRpc(state, connection, "RemoveDevice", params);
    }

    static bool addAndRemoveDeviceOverRpc(State& state, Connection& connection)
    {
        const bool added = addDeviceOverRpc(state, connection);
        return removeDeviceOverRpc(state, connection) && added;
    }

    static bool writePropertyOverRpc(State& state, Connection& connection, const std::string& path, uint64_t value)
    {
        auto params = componentParams(state, path);
        params.set("PropertyName", String("StrProp"));
        params.set("PropertyValue", String(std::to_string(value)));
        return callRpc(state, connection, "SetPropertyValue", params);
    }

    static bool addAndRemoveDeviceLocally(State& state)
    {
        const auto device = state.device.addDevice("mock://test");
        state.device.removeDevice(device);
        return true;
    }

    static bool writePropertyLocally(State& state, uint64_t value)
    {
        const auto channel = state.device.findComponent(NewChannelPath);
        if (!channel.assigned())
            return false;

        channel.setPropertyValue("StrProp", String(std::to_string(value)));
        return true;
    }

    static bool setStatusLocally(State& state, uint64_t iteration)
    {
        const auto channel = state.device.findComponent(NewChannelPath);
        if (!channel.assigned())
            return false;

        const auto typeManager = state.device.getContext().getTypeManager();
        const auto statusContainer = channel.getStatusContainer();
        const auto statusContainerPrivate = statusContainer.asPtr<IComponentStatusContainerPrivate>();
        if (!statusContainer.getStatuses().hasKey("TestStatus"))
            statusContainerPrivate.addStatus("TestStatus", Enumeration("ComponentStatusType", "Ok", typeManager));

        const auto value = Enumeration("ComponentStatusType", iteration % 2 == 0 ? "Warning" : "Ok", typeManager);
        statusContainerPrivate.setStatus("TestStatus", value);
        return true;
    }

    // Runs the workers until each has made its change ChangesPerWorker times; fails when one makes no change for the stall timeout
    static void run(const std::shared_ptr<State>& state, const std::vector<Worker>& workers)
    {
        using namespace std::chrono_literals;
        using Clock = std::chrono::steady_clock;
        const auto stallTimeout = 10s;

        for (size_t i = 0; i < workers.size(); ++i)
            state->workers.emplace_back();

        std::vector<std::thread> threads;
        for (size_t i = 0; i < workers.size(); ++i)
        {
            threads.emplace_back(
                [state, i, worker = workers[i]]
                {
                    auto& progress = state->workers[i];
                    try
                    {
                        for (uint64_t iteration = 0; !state->stop; ++iteration)
                        {
                            if (worker(iteration))
                                ++progress.changes;
                        }
                    }
                    catch (const std::exception& e)
                    {
                        std::scoped_lock lock(state->errorSync);
                        state->error = e.what();
                        state->stop = true;
                    }

                    progress.done = true;
                });
        }

        std::vector<uint64_t> lastChanges(workers.size(), 0);
        std::vector<Clock::time_point> lastChangeTime(workers.size(), Clock::now());
        for (bool allDone = false; !allDone;)
        {
            std::this_thread::sleep_for(10ms);
            const auto now = Clock::now();

            allDone = true;
            bool allChanged = true;
            bool stalled = false;
            for (size_t i = 0; i < workers.size(); ++i)
            {
                const auto& progress = state->workers[i];
                const uint64_t changes = progress.changes;
                allChanged = allChanged && changes >= ChangesPerWorker;
                if (progress.done)
                    continue;

                allDone = false;
                if (changes != lastChanges[i])
                {
                    lastChanges[i] = changes;
                    lastChangeTime[i] = now;
                }
                stalled = stalled || now - lastChangeTime[i] > stallTimeout;
            }

            if (allChanged)
                state->stop = true;

            if (stalled)
            {
                std::ostringstream report;
                for (size_t i = 0; i < workers.size(); ++i)
                {
                    const auto stalledFor = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastChangeTime[i]);
                    report << "\n  worker " << i << ": " << lastChanges[i] << " changes, ";
                    if (state->workers[i].done)
                        report << "done";
                    else
                        report << "none for " << stalledFor.count() << " ms";
                }

                // The stuck threads never return, so they and everything they use are leaked instead of destroyed under them
                state->stop = true;
                for (auto& thread : threads)
                    thread.detach();
                static_cast<void>(new std::shared_ptr<State>(state));

                FAIL() << "No change by a worker for " << stallTimeout.count() << " s: deadlock, or the workers never met" << report.str();
            }
        }

        for (auto& thread : threads)
            thread.join();

        ASSERT_TRUE(state->error.empty()) << state->error;
        for (const auto& connection : state->connections)
            ASSERT_GT(connection.notifications.load(), 0u);
    }
};

// Several server threads change the device while no RPC is in progress
TEST_F(ConfigCoreEventConcurrencyTest, ServerThreadsChangeDeviceWithoutRpc)
{
    const auto statePtr = createState();
    auto& state = *statePtr;
    connect(state);

    run(statePtr,
        {[&state](uint64_t) { return addAndRemoveDeviceLocally(state); },
         [&state](uint64_t iteration) { return writePropertyLocally(state, iteration); },
         [&state](uint64_t iteration) { return setStatusLocally(state, iteration); }});
}

// A client adds a device while a server thread writes a property of that device
TEST_F(ConfigCoreEventConcurrencyTest, RpcAddsDeviceWhileServerThreadWritesProperty)
{
    const auto statePtr = createState();
    auto& state = *statePtr;
    auto& connection = connect(state);

    run(statePtr,
        {[&state, &connection](uint64_t) { return addAndRemoveDeviceOverRpc(state, connection); },
         [&state](uint64_t iteration) { return writePropertyLocally(state, iteration); }});
}

// A client adds a device while a server thread sets a status of that device
TEST_F(ConfigCoreEventConcurrencyTest, RpcAddsDeviceWhileServerThreadSetsStatus)
{
    const auto statePtr = createState();
    auto& state = *statePtr;
    auto& connection = connect(state);

    run(statePtr,
        {[&state, &connection](uint64_t) { return addAndRemoveDeviceOverRpc(state, connection); },
         [&state](uint64_t iteration) { return setStatusLocally(state, iteration); }});
}

// The same as RpcAddsDeviceWhileServerThreadWritesProperty for a client that gets every core event in its own notification
TEST_F(ConfigCoreEventConcurrencyTest, RpcAddsDeviceWhileServerThreadWritesPropertyWithoutEventBundling)
{
    const auto statePtr = createState();
    auto& state = *statePtr;
    auto& connection = connect(state, 19);

    run(statePtr,
        {[&state, &connection](uint64_t) { return addAndRemoveDeviceOverRpc(state, connection); },
         [&state](uint64_t iteration) { return writePropertyLocally(state, iteration); }});
}

// One client adds a device while another client writes a property of that device
TEST_F(ConfigCoreEventConcurrencyTest, TwoClientsAddDeviceAndWriteProperty)
{
    const auto statePtr = createState();
    auto& state = *statePtr;
    auto& first = connect(state);
    auto& second = connect(state);

    run(statePtr,
        {[&state, &first](uint64_t) { return addAndRemoveDeviceOverRpc(state, first); },
         [&state, &second](uint64_t iteration) { return writePropertyOverRpc(state, second, NewChannelPath, iteration); }});
}

// Two clients write one property at the same time; each client must get one event per write, none lost or sent twice
TEST_F(ConfigCoreEventConcurrencyTest, TwoClientsWritePropertyAndGetEveryEvent)
{
    const auto statePtr = createState();
    auto& state = *statePtr;
    auto& first = connect(state, GetLatestConfigProtocolVersion(), true);
    auto& second = connect(state, GetLatestConfigProtocolVersion(), true);

    // The two clients never write the same value, so every write changes the property and reports one event
    ASSERT_NO_FATAL_FAILURE(run(
        statePtr,
        {[&state, &first](uint64_t iteration) { return writePropertyOverRpc(state, first, RootChannelPath, 2 * iteration); },
         [&state, &second](uint64_t iteration) { return writePropertyOverRpc(state, second, RootChannelPath, 2 * iteration + 1); }}));

    const uint64_t writes = state.workers[0].changes + state.workers[1].changes;
    for (const auto& connection : state.connections)
    {
        uint64_t events = 0;
        for (const auto& json : sentJson(connection))
        {
            const auto items = deserialize(state, json);
            ASSERT_EQ(items.getCount() % 2, 0u) << json;
            for (SizeT i = 0; i < items.getCount(); i += 2)
                ASSERT_EQ(items[i].asPtr<IString>().toStdString(), globalId(state, RootChannelPath));
            events += items.getCount() / 2;
        }
        ASSERT_EQ(events, writes);
    }
}

// A component removed while its ComponentAdded event is serialized reaches the client as added first and removed after
TEST_F(ConfigCoreEventConcurrencyTest, ComponentRemovedWhileItsAddedEventIsSerializedIsSentAsAddedThenRemoved)
{
    const auto statePtr = createState();
    auto& state = *statePtr;
    auto& connection = connect(state, GetLatestConfigProtocolVersion(), true);

    const FolderConfigPtr folder = state.device.getItem("FB");
    const auto type = FunctionBlockType("test", "test", "");
    folder.addItem(createWithImplementation<IFunctionBlock, SelfRemovingFunctionBlock>(type, state.device.getContext(), folder, "fb"));

    ASSERT_FALSE(folder.hasItem("fb"));
    ASSERT_EQ(sentJson(connection).size(), 1u);
    ASSERT_NE(sentPosition(connection, "ComponentAdded"), std::string::npos);
    ASSERT_NE(sentPosition(connection, "ComponentRemoved"), std::string::npos);
    ASSERT_LT(sentPosition(connection, "ComponentAdded"), sentPosition(connection, "ComponentRemoved"));
}

// An event reported while an earlier one is still being serialized is sent after it, so events reach the client in the order they happened
TEST_F(ConfigCoreEventConcurrencyTest, EventsAreSentInTheOrderTheyWereReported)
{
    const auto statePtr = createState();
    auto& state = *statePtr;
    auto& connection = connect(state, GetLatestConfigProtocolVersion(), true);

    SlowSerialization slow;
    {
        std::thread adder = addSlowlySerializedFunctionBlock(state, slow);
        const Finally finishAdding(
            [&]
            {
                slow.proceed = true;
                adder.join();
            });

        state.device.setPropertyValue("StrProp", "written");
        EXPECT_TRUE(sentJson(connection).empty());
    }

    ASSERT_NE(sentPosition(connection, "ComponentAdded"), std::string::npos);
    ASSERT_NE(sentPosition(connection, "PropertyValueChanged"), std::string::npos);
    ASSERT_LT(sentPosition(connection, "ComponentAdded"), sentPosition(connection, "PropertyValueChanged"));
}

// A component is serialized for its update end event without a lock of the server, so that a second such event is not held up behind it
TEST_F(ConfigCoreEventConcurrencyTest, UpdateEndEventIsReportedWhileAnotherIsStillBeingSerialized)
{
    const auto statePtr = createState();
    auto& state = *statePtr;
    connect(state);

    SlowSerialization slow;
    slow.proceed = true;
    addSlowlySerializedFunctionBlock(state, slow).join();
    slow.proceed = false;
    slow.serializing = false;

    const auto reportUpdateEnd = [](const ComponentPtr& component)
    {
        EventPtr<const ComponentPtr, const CoreEventArgsPtr> coreEvent;
        checkErrorInfo(component.getContext()->getOnCoreEvent(&coreEvent));
        const CoreEventArgsPtr args = CoreEventArgsComponentUpdateEnd();
        coreEvent(component, args);
    };

    std::future_status reportStatus;
    {
        std::thread slowReporter(reportUpdateEnd, state.device.findComponent("FB/fb"));
        const Finally finishReporting(
            [&]
            {
                slow.proceed = true;
                slowReporter.join();
            });

        while (!slow.serializing)
            std::this_thread::yield();

        auto reported = std::async(std::launch::async, reportUpdateEnd, state.device.findComponent(RootChannelPath));
        reportStatus = reported.wait_for(std::chrono::seconds(5));
        slow.proceed = true;
    }

    ASSERT_EQ(reportStatus, std::future_status::ready);
}

// The events of an RPC are sent before its reply also when an event that another thread reported earlier is still being serialized
TEST_F(ConfigCoreEventConcurrencyTest, RpcReplyFollowsItsEventsAlsoBehindAnEventStillBeingSerialized)
{
    // A client of a protocol version before 20 gets the events of an RPC one by one, a later one gets them as one notification
    for (const uint16_t protocolVersion : {uint16_t{19}, GetLatestConfigProtocolVersion()})
    {
        const auto statePtr = createState();
        auto& state = *statePtr;
        auto& connection = connect(state, protocolVersion, true);

        SlowSerialization slow;
        std::future_status replyStatus;
        bool eventSentBeforeReply = false;
        {
            std::thread adder = addSlowlySerializedFunctionBlock(state, slow);
            const Finally finishAdding(
                [&]
                {
                    slow.proceed = true;
                    adder.join();
                });

            auto reply = std::async(
                std::launch::async,
                [&]
                {
                    const bool written = writePropertyOverRpc(state, connection, RootChannelPath, 1);
                    return written && sentPosition(connection, "PropertyValueChanged") != std::string::npos;
                });

            replyStatus = reply.wait_for(std::chrono::milliseconds(200));
            EXPECT_TRUE(sentJson(connection).empty()) << protocolVersion;
            slow.proceed = true;
            eventSentBeforeReply = reply.get();
        }

        ASSERT_EQ(replyStatus, std::future_status::timeout) << protocolVersion;
        ASSERT_TRUE(eventSentBeforeReply) << protocolVersion;
        ASSERT_LT(sentPosition(connection, "ComponentAdded"), sentPosition(connection, "PropertyValueChanged")) << protocolVersion;
    }
}

// The core events of an RPC reach the client as one serialized list of component IDs and event arguments
TEST_F(ConfigCoreEventConcurrencyTest, EventsOfAnRpcAreSentAsOneSerializedList)
{
    const auto statePtr = createState();
    auto& state = *statePtr;
    auto& connection = connect(state, GetLatestConfigProtocolVersion(), true);

    const PropertyObjectPtr channel = state.device.findComponent(RootChannelPath);
    channel.getOnPropertyValueWrite("StrProp") +=
        [&state](const PropertyObjectPtr&, const PropertyValueEventArgsPtr&) { state.device.setPropertyValue("StrProp", "written"); };

    ASSERT_TRUE(writePropertyOverRpc(state, connection, RootChannelPath, 1));
    const auto sent = sentJson(connection);
    ASSERT_EQ(sent.size(), 1u);

    // The head of a serialized list of base objects, as clients of every supported protocol version receive it
    const std::string listHead = R"({"__type":"List","itemIntfID":"{9C911F6D-1664-5AA2-97BD-90FE3143E881}","values":[)";
    ASSERT_EQ(sent[0].compare(0, listHead.size(), listHead), 0) << sent[0];
    ASSERT_EQ(sent[0].substr(sent[0].size() - 2), "]}");

    const auto items = deserialize(state, sent[0]);
    ASSERT_EQ(items.getCount(), 4u);
    ASSERT_EQ(items[0].asPtr<IString>().toStdString(), globalId(state));
    ASSERT_EQ(items[2].asPtr<IString>().toStdString(), globalId(state, RootChannelPath));
}
