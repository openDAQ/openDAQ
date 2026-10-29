/**
 * Trial 2, mixed rates and devices over the network. A reference device runs behind a native server on this machine
 * (loopback, port 7430) and is used as a network device; the configurable Raspberry Pi's ADC joins over OPC UA when it
 * answers; a block statistics block on the client turns two channels into 10 Hz statistics.
 *
 * MultiReader2 reads equal rates only, so the application does what a measurement application does with acquisition
 * groups: it keeps one reader per (sample rate, reference clock) and one CSV per reader, and a supervisor moves a
 * channel to the group of its new rate when its rate changes. Readers are driven by onDataAvailable. The supervisor
 * also watches each device's configuration connection: on a reconnect it applies the application's channel setup again,
 * as the restarted device came back with its defaults.
 */

#include "trial_common.h"

#include <fmt/ranges.h>

using namespace daq;
using trial::Scenario;

namespace
{

struct Group
{
    std::unique_ptr<trial::CsvSink> sink;
    std::unique_ptr<trial::ReaderDriver> driver;
    std::vector<std::string> members;  // global IDs in slot order
};

// (rate in Hz, reference clock); empty while the domain is not known yet
std::string groupKey(const SignalPtr& signal)
{
    const auto domainSignal = signal.getDomainSignal();
    const auto d = domainSignal.assigned() ? domainSignal.getDescriptor() : nullptr;
    if (!d.assigned() || !d.getRule().assigned() || !d.getTickResolution().assigned())
        return "";
    const auto rule = d.getRule().getParameters();
    if (!rule.hasKey("delta"))
        return "";
    const double rate = static_cast<double>(d.getTickResolution().getDenominator()) /
                        (static_cast<double>(d.getTickResolution().getNumerator()) * static_cast<double>(rule.get("delta")));
    const auto info = d.getReferenceDomainInfo();
    return fmt::format("{:g}Hz {}", rate, info.assigned() && info.getReferenceDomainId().assigned() ? info.getReferenceDomainId() : "none");
}

InstancePtr startServer(trial::Log& log)
{
    const InstancePtr server = InstanceBuilder().setModulePath(MODULE_PATH).setGlobalLogLevel(LogLevel::Warn).build();
    auto deviceConfig = server.getAvailableDeviceTypes().get("daqref").createDefaultConfig();
    deviceConfig.setPropertyValue("NumberOfChannels", 4);
    server.setRootDevice("daqref://device0", deviceConfig);
    auto serverConfig = server.getAvailableServerTypes().get("OpenDAQNativeStreaming").createDefaultConfig();
    serverConfig.setPropertyValue("NativeStreamingPort", 7430);
    server.addServer("OpenDAQNativeStreaming", serverConfig);
    log("server: reference device behind a native server on port 7430");
    return server;
}

}

int main(int /*argc*/, const char* /*argv*/[])
{
    const auto dir = trial::outputDir("network-rates");
    trial::Log log(dir);
    trial::PropertyGuard guard(log);

    InstancePtr server = startServer(log);
    const InstancePtr client = Instance(MODULE_PATH);

    // Heartbeat monitoring, so a server that goes away without closing is noticed
    auto addConfig = client.createDefaultAddDeviceConfig();
    PropertyObjectPtr deviceOptions = addConfig.getPropertyValue("Device");
    PropertyObjectPtr native = deviceOptions.getPropertyValue("OpenDAQNativeConfiguration");
    PropertyObjectPtr transport = native.getPropertyValue("TransportLayerConfig");
    transport.setPropertyValue("MonitoringEnabled", True);
    const auto remote = client.addDevice("daq.nd://127.0.0.1:7430/", addConfig);
    const auto channel = [&](int i) { return remote.getChannels()[i]; };
    const auto ai = [&](int i) { return channel(i).getSignals(search::Visible())[0]; };

    // The application's channel setup: AI2 and AI3 at 500 Hz, applied again after every reconnect
    const auto applySetup = [&]
    {
        for (int i : {2, 3})
        {
            channel(i).setPropertyValue("UseGlobalSampleRate", false);
            channel(i).setPropertyValue("SampleRate", 500.0);
        }
        log("setup applied: AI2 and AI3 at 500 Hz");
    };
    applySetup();

    // Statistics on the client, at 10 Hz from the two 1 kHz channels
    const auto stats = client.addFunctionBlock("RefFBModuleBlockStatisticsReader");
    stats.getInputPorts()[0].connect(ai(0));
    stats.getInputPorts()[1].connect(ai(1));

    // The configurable Pi's ADC, if one answers over OPC UA
    DevicePtr pi;
    for (const auto& info : client.getAvailableDevices())
        for (const auto& cap : info.getServerCapabilities())
            if (!pi.assigned() && cap.getProtocolId() == "OpenDAQOPCUAConfiguration" && info.getModel().toStdString().find("Raspberry") != std::string::npos)
                try
                {
                    pi = client.addDevice(cap.getConnectionString());
                    log("Pi {} joined over {}", info.getSerialNumber(), cap.getConnectionString());
                }
                catch (const std::exception& e)
                {
                    log("Pi {} over OPC UA failed: {}", info.getSerialNumber(), e.what());
                }

    const auto tracked = [&]
    {
        std::vector<SignalPtr> out = {ai(0), ai(1), ai(2), ai(3)};
        for (const auto& s : stats.getSignals())
            out.push_back(s);
        if (pi.assigned())
            for (const auto& s : pi.getSignals(search::Recursive(search::Visible())))
                if (s.getLocalId() == "AI" && s.getDomainSignal().assigned())
                    out.push_back(s);
        return out;
    };
    const auto label = [](const SignalPtr& s)
    {
        const std::string parent = s.getParent().assigned() && s.getParent().getParent().assigned() ? s.getParent().getParent().getLocalId().toStdString() : "";
        return parent + "." + s.getLocalId().toStdString();
    };

    // The supervisor: one reader per (rate, clock); a channel follows its rate to the matching group
    std::recursive_mutex groupsMutex;
    std::map<std::string, Group> groups;
    const auto regroup = [&]
    {
        std::scoped_lock lock(groupsMutex);
        std::map<std::string, std::vector<SignalPtr>> wanted;
        for (const auto& s : tracked())
            if (const auto key = groupKey(s); !key.empty())
                wanted[key].push_back(s);
        for (auto& [key, group] : groups)
            if (!wanted.count(key) && !group.members.empty())
            {
                group.members.clear();
                group.driver->configure(MultiReader2Params(), {});
                log("group {} is empty", key);
            }
        for (auto& [key, signals] : wanted)
        {
            // Members already in the group keep their slots and newcomers go last, so the automatic main input, the
            // first healthy slot, stays with an incumbent and a channel passing through never becomes the reference
            const auto& old = groups[key].members;
            const auto rank = [&](const SignalPtr& s)
            { return std::find(old.begin(), old.end(), s.getGlobalId().toStdString()) - old.begin(); };
            std::stable_sort(signals.begin(), signals.end(), [&](const SignalPtr& a, const SignalPtr& b) { return rank(a) < rank(b); });

            std::vector<std::string> ids;
            std::vector<std::string> labels;
            auto list = List<IComponent>();
            for (const auto& s : signals)
            {
                ids.push_back(s.getGlobalId());
                labels.push_back(label(s));
                list.pushBack(s);
            }
            auto params = MultiReader2Params();
            params.setInputs(list);
            params.setErrorPolicy(MultiReader2ErrorPolicy::Exclude);
            auto& group = groups[key];
            if (!group.driver)
            {
                std::string stem;
                for (const char c : key)
                    stem += std::isalnum(static_cast<unsigned char>(c)) ? c : '_';
                group.sink = std::make_unique<trial::CsvSink>(dir, stem);
                group.driver = std::make_unique<trial::ReaderDriver>(key, log, *group.sink);
                group.members = ids;
                group.driver->start(params, labels, true);
                log("group {} started: {}", key, fmt::join(labels, ", "));
            }
            else if (group.members != ids)
            {
                group.members = ids;
                group.driver->configure(params, labels);
                log("group {} now: {}", key, fmt::join(labels, ", "));
            }
        }
    };

    std::atomic<bool> running{true};
    std::string lastConnection = "Connected";
    std::thread supervisor(
        [&]
        {
            while (running)
            {
                try
                {
                    const std::string connection = remote.getConnectionStatusContainer().getStatus("ConfigurationStatus").getValue();
                    if (connection != lastConnection)
                    {
                        log("device connection {} -> {}", lastConnection, connection);
                        if (connection == "Connected")
                            applySetup();
                        lastConnection = connection;
                    }
                    if (connection == "Connected")
                        regroup();
                }
                catch (const std::exception& e)
                {
                    log("supervisor: {}", e.what());
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
            }
        });

    // The scenario follows the 1 kHz group of the reference device
    std::this_thread::sleep_for(std::chrono::seconds(1));
    trial::ReaderDriver* main = nullptr;
    {
        std::scoped_lock lock(groupsMutex);
        for (auto& [key, group] : groups)
            if (key.rfind("1000Hz", 0) == 0 && std::find(group.members.begin(), group.members.end(), ai(0).getGlobalId().toStdString()) != group.members.end())
                main = group.driver.get();
    }
    if (!main)
    {
        log("no 1 kHz group of the reference device; stopping");
        running = false;
        supervisor.join();
        return 1;
    }

    std::vector<Scenario::Step> steps = {
        {"Start: groups by rate and clock, readers driven by onDataAvailable", nullptr, ComponentStatus::Ok, "",
         "Each group synchronizes and streams; the 1 kHz group holds AI0 and AI1 of the network device."},
        {"AI1 to 500 Hz on the network device", [&] {
             guard.set(channel(1), "UseGlobalSampleRate", false, "AI1");
             guard.set(channel(1), "SampleRate", 500.0, "AI1");
         },
         ComponentStatus::Ok, "",
         "AI1 DomainDescriptorInvalid in the 1 kHz group (Warning) until the supervisor moves it to the 500 Hz group; both "
         "groups configure, each with a new segment. The statistics block reads AI0 and AI1 itself and sets AI1 aside.",
         4000, 2500},
        {"AI1 back to 1 kHz", [&] { channel(1).setPropertyValue("SampleRate", 1000.0); }, ComponentStatus::Ok, "",
         "The reverse move; the statistics block takes AI1 back with a resynchronization.", 4000, 2500},
        {"Statistics BlockSize 100 -> 50: its outputs go from 10 Hz to 20 Hz", [&] { guard.set(stats, "BlockSize", 50, "statistics"); },
         ComponentStatus::Ok, "",
         "The statistics group's main domain changes: internal reconfigure; the supervisor then renames it to the 20 Hz group "
         "(a configure). The 1 kHz group is untouched.",
         4000, 2500},
        {"The server goes away", [&] { server = nullptr; }, ComponentStatus::Ok, "",
         "Every input of the group stops at once: no input is behind another, so nothing fails and no read returns data; the "
         "application shows the device connection as lost.",
         2000, 4000},
        {"The server comes back with a fresh device",
         [&] { server = startServer(log); }, ComponentStatus::Ok, "",
         "The client reconnects; the ticks continue at the new device's time, a jump: resynchronized, and the application marks "
         "a gap of the outage. The supervisor applies the setup again, as the device came back with its defaults.",
         15000, 4000},
    };

    Scenario scenario(log, *main, dir / "scenario.md");
    try
    {
        scenario.run(steps);
    }
    catch (const std::exception& e)
    {
        log("scenario aborted: {}", e.what());
    }

    running = false;
    supervisor.join();
    guard.restoreAll();
    {
        std::scoped_lock lock(groupsMutex);
        for (auto& [key, group] : groups)
        {
            group.driver->stop();
            log("group {}: {} samples, {} rows in {} segments, level {} {}",
                key,
                group.driver->sampleCount(),
                group.sink->rowCount(),
                group.sink->segmentCount(),
                trial::levelName(group.driver->level()),
                group.driver->message());
        }
    }
    log("done: {} mismatches; output in {}", scenario.mismatches, dir.string());
    return 0;
}
