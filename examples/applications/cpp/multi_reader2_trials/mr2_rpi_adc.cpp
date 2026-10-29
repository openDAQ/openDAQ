/**
 * Trial 3, Raspberry Pis with ADS1256 ADCs on the office network. The application discovers the Pis over mDNS and
 * connects to each the way a measurement application must when the devices are not all alike: native configuration
 * first, then OPC UA, and when neither works, native streaming alone, which reads but cannot configure.
 *
 * Channels are grouped by clock: one MultiReader2 per reference domain ID, each recorded to its own CSV with absolute
 * time, as a recorder never interleaves samples of independent clocks row by row. A further reader puts both ADCs in
 * one table on purpose, with the configurable Pi's ADC as the pinned main input, to record what the reader does with
 * two clocks. Every property changed on a Pi is restored at the end.
 */

#include "trial_common.h"

using namespace daq;
using trial::Scenario;

namespace
{

struct Pi
{
    std::string address;
    std::string serial;
    std::string protocols;
    std::string connectedWith;
    std::string why;
    DevicePtr device;
    DevicePtr adcDevice;    // the ADS1256 sub-device; only over a configuration protocol
    ChannelPtr adcChannel;  // its AI channel
    SignalPtr adc;          // the aligned AI output
    SignalPtr temperature;  // the system monitor's 1 Hz temperature
};

std::string remoteId(const SignalPtr& signal)
{
    // A pseudo device flattens the remote global ID into its local ID; a configuration device keeps the tree
    std::string id = signal.getGlobalId();
    const auto pos = id.find("/Sig/*");
    if (pos != std::string::npos)
        id = id.substr(pos + 5);
    for (auto& c : id)
        if (c == '*')
            c = '/';
    return id;
}

bool endsWith(const std::string& s, const std::string& suffix)
{
    return s.size() >= suffix.size() && s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

void findSignals(Pi& pi)
{
    pi.adc = nullptr;
    pi.temperature = nullptr;
    for (const auto& s : pi.device.getSignals(search::Recursive(search::Any())))
    {
        const auto id = remoteId(s);
        if (endsWith(id, "/IO/AI/Sig/AI"))
            pi.adc = s;
        else if (endsWith(id, "RpiSystemMonitorChannel/Sig/temperature"))
            pi.temperature = s;
    }
    for (const auto& sub : pi.device.getDevices())
        if (sub.getInfo().getModel() == "ADS1256")
        {
            pi.adcDevice = sub;
            pi.adcChannel = sub.getChannels()[0];
        }
}

std::string domainText(const SignalPtr& signal)
{
    if (!signal.assigned() || !signal.getDomainSignal().assigned())
        return "-";
    const auto d = signal.getDomainSignal().getDescriptor();
    const auto rule = d.getRule().getParameters();
    const auto info = d.getReferenceDomainInfo();
    return fmt::format("delta {} start {} res {}/{} ref {} offset {}",
                       rule.get("delta"),
                       rule.get("start"),
                       d.getTickResolution().getNumerator(),
                       d.getTickResolution().getDenominator(),
                       info.assigned() ? info.getReferenceDomainId() : "-",
                       info.assigned() && info.getReferenceDomainOffset().assigned() ? info.getReferenceDomainOffset() : 0);
}

std::string clockOf(const SignalPtr& signal)
{
    const auto info = signal.getDomainSignal().getDescriptor().getReferenceDomainInfo();
    return info.assigned() && info.getReferenceDomainId().assigned() ? info.getReferenceDomainId().toStdString() : "none";
}

MultiReader2ParamsPtr paramsOf(const std::vector<SignalPtr>& signals, MultiReader2ErrorPolicy policy, const SignalPtr& main = nullptr)
{
    auto params = MultiReader2Params();
    auto list = List<IComponent>();
    for (const auto& s : signals)
        list.pushBack(s);
    params.setInputs(list);
    params.setErrorPolicy(policy);
    if (main.assigned())
        params.setMainInput(main);
    return params;
}

}

int main(int /*argc*/, const char* /*argv*/[])
{
    const auto dir = trial::outputDir("rpi-adc");
    trial::Log log(dir);
    trial::PropertyGuard guard(log);
    const InstancePtr instance = Instance(MODULE_PATH);

    // Discovery and connection, best protocol first
    std::vector<Pi> pis;
    for (const auto& info : instance.getAvailableDevices())
    {
        if (info.getModel().toStdString().find("Raspberry Pi") == std::string::npos)
            continue;
        Pi pi;
        pi.serial = info.getSerialNumber().toStdString();
        std::map<std::string, std::string> byProtocol;
        for (const auto& cap : info.getServerCapabilities())
        {
            byProtocol[cap.getProtocolId().toStdString()] = cap.getConnectionString().toStdString();
            pi.protocols += (pi.protocols.empty() ? "" : ", ") + cap.getProtocolId().toStdString();
            if (pi.address.empty() && cap.getAddresses().getCount() > 0)
                pi.address = StringPtr(cap.getAddresses()[0]).toStdString();
        }
        for (const auto& protocol : {"OpenDAQNativeConfiguration", "OpenDAQOPCUAConfiguration", "OpenDAQNativeStreaming"})
        {
            if (!byProtocol.count(protocol))
                continue;
            try
            {
                pi.device = instance.addDevice(byProtocol[protocol]);
                pi.connectedWith = byProtocol[protocol];
                break;
            }
            catch (const std::exception& e)
            {
                // The module manager keeps only its own message; the module's own reason is worth recording
                std::string reason = e.what();
                for (const auto& module : instance.getModuleManager().getModules())
                    try
                    {
                        module.createDevice(byProtocol[protocol], instance.getRootDevice());
                    }
                    catch (const NotFoundException&)
                    {
                    }
                    catch (const std::exception& inner)
                    {
                        // Modules that do not handle the connection string reject it as a string or a parameter
                        const std::string text = inner.what();
                        if (text.find("connection string") == std::string::npos && text != "Invalid parameter")
                            reason = text;
                    }
                pi.why += fmt::format("{} failed: {}; ", protocol, reason);
            }
        }
        if (!pi.device.assigned())
        {
            log("Pi {} ({}): no connection: {}", pi.serial, pi.address, pi.why);
            continue;
        }
        findSignals(pi);
        log("Pi {} at {} [{}] connected with {}{}; ADS1256 {}; ADC signal {}; ADC domain {}; temperature domain {}",
            pi.serial,
            pi.address,
            pi.protocols,
            pi.connectedWith,
            pi.why.empty() ? "" : " (" + pi.why + ")",
            pi.adcDevice.assigned() ? "configurable" : (pi.adc.assigned() ? "streamed only" : "absent"),
            pi.adc.assigned() ? remoteId(pi.adc) : "-",
            domainText(pi.adc),
            domainText(pi.temperature));
        pis.push_back(std::move(pi));
    }

    Pi* configurable = nullptr;  // the Pi whose ADC the application can configure: the cross reader's pinned main
    Pi* streamed = nullptr;      // a Pi whose ADC is read only
    for (auto& pi : pis)
    {
        if (pi.adc.assigned() && pi.adcChannel.assigned() && !configurable)
            configurable = &pi;
        else if (pi.adc.assigned() && !streamed)
            streamed = &pi;
    }
    log("{} Pis answered, {} with an ADC", pis.size(), std::count_if(pis.begin(), pis.end(), [](const Pi& p) { return p.adc.assigned(); }));
    if (!configurable || !streamed)
    {
        log("need one configurable and one more ADC for the scenario; stopping");
        return 1;
    }

    // One reader per clock, each to its own file, as a recorder groups channels of one clock
    std::map<std::string, std::vector<Pi*>> clocks;
    for (auto& pi : pis)
        if (pi.adc.assigned())
            clocks[clockOf(pi.adc)].push_back(&pi);
    std::vector<std::unique_ptr<trial::CsvSink>> sinks;
    std::vector<std::unique_ptr<trial::ReaderDriver>> groups;
    std::atomic<bool> running{true};
    for (const auto& [clock, members] : clocks)
    {
        std::string stem = "clock";
        for (const char c : clock)
            stem += std::isalnum(static_cast<unsigned char>(c)) ? c : '_';
        sinks.push_back(std::make_unique<trial::CsvSink>(dir, stem));
        groups.push_back(std::make_unique<trial::ReaderDriver>(clock, log, *sinks.back()));
        std::vector<SignalPtr> signals;
        std::vector<std::string> labels;
        for (const Pi* pi : members)
        {
            signals.push_back(pi->adc);
            labels.push_back(pi->serial + ".AI");
        }
        groups.back()->start(paramsOf(signals, MultiReader2ErrorPolicy::Invalidate), labels, false);
        groups.back()->runLoop(running);
    }

    // Both ADCs in one table, against the advice above, with the configurable one pinned as the reference
    trial::CsvSink crossSink(dir, "cross");
    trial::ReaderDriver cross("cross", log, crossSink);
    const auto crossLabels = std::vector<std::string>{configurable->serial + ".AI", streamed->serial + ".AI"};
    const auto crossParams = [&] { return paramsOf({configurable->adc, streamed->adc}, MultiReader2ErrorPolicy::Exclude, configurable->adc); };
    cross.start(crossParams(), crossLabels, false);
    cross.runLoop(running);

    // Every Pi's 1 Hz temperature in one table: a slow monitoring view across devices. The operator's choice of master
    // clock is a PTP-disciplined input, pinned as the main input; the automatic choice would take the first in the list
    trial::CsvSink monitorSink(dir, "monitor");
    trial::ReaderDriver monitor("monitor", log, monitorSink);
    const auto monitorSetup = [&](bool start)
    {
        std::vector<SignalPtr> signals;
        std::vector<std::string> labels;
        SignalPtr master;
        for (const auto& pi : pis)
            if (pi.temperature.assigned())
            {
                signals.push_back(pi.temperature);
                labels.push_back(pi.serial + ".temperature");
                if (!master.assigned() && clockOf(pi.temperature).rfind("ptp:", 0) == 0)
                    master = pi.temperature;
            }
        if (start)
            monitor.start(paramsOf(signals, MultiReader2ErrorPolicy::Exclude, master), labels, false);
        else
            monitor.configure(paramsOf(signals, MultiReader2ErrorPolicy::Exclude, master), labels);
    };
    monitorSetup(true);
    monitor.runLoop(running);

    const auto adcChannel = configurable->adcChannel;
    const auto streamedAddress = streamed->connectedWith;
    const std::string aiName = configurable->serial + ".AI";
    std::vector<Scenario::Step> steps = {
        {"Start: both ADCs in one reader, the configurable Pi's pinned", nullptr, ComponentStatus::Warning, "AI: not synchronized",
         "The ticks count from each Pi's boot; with the reference domain offsets applied (decision 55) both ADCs are on wall "
         "time, within ms of each other, but each free-running ADC grid has its own phase: the streamed one is SyncFailed "
         "(off grid), set aside. Without the offsets it is late by the boot-time difference.",
         8000, 3000},
        {"Configurable Pi: ADC channel SampleRate 1000 -> 500 Hz", [&] { guard.set(adcChannel, "SampleRate", 500, aiName); },
         ComponentStatus::Warning, "AI: domain descriptor invalid",
         "The pinned main input's domain changes: internal reconfigure. The other ADC breaks the equal-rate constraint: "
         "DomainDescriptorInvalid, set aside.",
         8000, 3000},
        {"Configurable Pi: ADC channel SampleRate back to 1000 Hz", [&] { adcChannel.setPropertyValue("SampleRate", 1000); },
         ComponentStatus::Warning, "AI: not synchronized", "Back to the start state: the other ADC fails synchronization again.", 8000, 3000},
        {"Configurable Pi: ADC InputRange (PGA) to the next setting",
         [&] {
             const Int range = adcChannel.getPropertyValue("InputRange");
             const DictPtr<IInteger, IBaseObject> choices = adcChannel.getProperty("InputRange").getSelectionValues();
             Int next = range;
             for (const auto& key : choices.getKeyList())
                 if (static_cast<Int>(key) != range)
                 {
                     next = key;
                     break;
                 }
             guard.set(adcChannel, "InputRange", next, aiName);
         },
         ComponentStatus::Warning, "AI: not synchronized",
         "A value descriptor change on the main input when the range changes: descriptorChanged, no resynchronization.", 6000, 3000},
        {"Streamed Pi: the device is removed",
         [&] {
             instance.removeDevice(streamed->device);
             streamed->device = nullptr;
         },
         ComponentStatus::Warning, "AI: not connected",
         "Removing the device removes its signals: the port loses its signal, Disconnected, set aside. Its clock's own reader "
         "is invalid (Invalidate) and pauses.",
         6000, 2000},
        {"Streamed Pi: added again; the application maps its ADC by remote ID and configures the readers",
         [&] {
             streamed->device = instance.addDevice(streamedAddress);
             findSignals(*streamed);
             cross.configure(crossParams(), crossLabels);
             monitorSetup(false);
             for (auto& g : groups)
                 if (g->name == clockOf(streamed->adc))
                     g->configure(paramsOf({streamed->adc}, MultiReader2ErrorPolicy::Invalidate), {streamed->serial + ".AI"});
         },
         ComponentStatus::Warning, "AI: not synchronized",
         "configure with the new signal object: the other ADC fails synchronization again; its clock's reader recovers.", 10000, 3000},
    };

    Scenario scenario(log, cross, dir / "scenario.md");
    try
    {
        scenario.run(steps);
    }
    catch (const std::exception& e)
    {
        log("scenario aborted: {}", e.what());
    }
    guard.restoreAll();

    // How far each device's newest sample is behind the PC's clock, on the reference domain: a packet's last tick plus
    // the reference domain offset its descriptor carries, against the system clock when the packet is read
    for (auto& pi : pis)
        for (const auto& signal : {pi.adc, pi.temperature})
        {
            if (!signal.assigned())
                continue;
            const auto reader = PacketReader(signal);
            std::this_thread::sleep_for(std::chrono::milliseconds(2500));
            DataPacketPtr last;
            while (reader.getAvailableCount() > 0)
                if (const auto data = reader.read().asPtrOrNull<IDataPacket>(); data.assigned())
                    last = data;
            const auto now = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
            if (!last.assigned())
            {
                log("clock check {} {}: no data in 2.5 s", pi.serial, signal.getLocalId());
                continue;
            }
            const auto domain = last.getDomainPacket();
            const auto descriptor = domain.getDataDescriptor();
            const auto rule = descriptor.getRule().getParameters();
            const Int delta = rule.get("delta");
            const Int start = rule.get("start");
            const auto info = descriptor.getReferenceDomainInfo();
            const Int offset = info.assigned() && info.getReferenceDomainOffset().assigned() ? static_cast<Int>(info.getReferenceDomainOffset()) : 0;
            const auto res = descriptor.getTickResolution();
            const Int lastTick = static_cast<Int>(domain.getOffset()) + start + static_cast<Int>(last.getSampleCount() - 1) * delta + offset;
            const double lastUs = static_cast<double>(lastTick) * 1e6 * res.getNumerator() / res.getDenominator();
            log("clock check {} {}: newest sample {:.3f} s behind the PC clock (tick {}, reference {} offset {})",
                pi.serial,
                signal.getLocalId(),
                (static_cast<double>(now) - lastUs) / 1e6,
                lastTick,
                info.assigned() ? info.getReferenceDomainId() : "-",
                offset);
        }
    std::this_thread::sleep_for(std::chrono::seconds(2));

    running = false;
    for (auto& g : groups)
        g->stop();
    cross.stop();
    monitor.stop();
    for (auto& g : groups)
        log("clock reader {}: {} samples, level {} {}", g->name, g->sampleCount(), trial::levelName(g->level()), g->message());
    log("monitor reader: {} samples, level {} {}", monitor.sampleCount(), trial::levelName(monitor.level()), monitor.message());
    log("done: cross {} samples, {} mismatches; output in {}", cross.sampleCount(), scenario.mismatches, dir.string());
    return 0;
}
