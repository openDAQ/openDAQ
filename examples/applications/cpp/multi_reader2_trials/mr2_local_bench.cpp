/**
 * Trial 1, a local test bench: one reference device with four channels and a bridge strain block, read by one
 * MultiReader2 in the application's own acquisition loop (Exclude, automatic main input at first), recorded to CSV.
 *
 * The application locks each column's unit at its first descriptor, as a recorder whose table header must not change
 * under it: a new unit is rejected (ValueDescriptorInvalid) until the operator accepts it, which re-judges the inputs
 * through an equal-params configure and starts a new file segment. A watchdog reports a stall, an input that stopped
 * sending while the reader still waits for it, which the reader itself does not report.
 */

#include "trial_common.h"

using namespace daq;
using trial::Scenario;

int main(int /*argc*/, const char* /*argv*/[])
{
    const auto dir = trial::outputDir("local-bench");
    trial::Log log(dir);
    trial::CsvSink sink(dir, "bench");
    trial::ReaderDriver driver("bench", log, sink);
    trial::PropertyGuard guard(log);

    const InstancePtr instance = Instance(MODULE_PATH);
    auto deviceConfig = instance.getAvailableDeviceTypes().get("daqref").createDefaultConfig();
    deviceConfig.setPropertyValue("NumberOfChannels", 4);
    const auto device = instance.addDevice("daqref://device0", deviceConfig);
    const auto channel = [&](int i) { return device.getChannels()[i]; };
    const auto aiSignal = [&](int i) { return channel(i).getSignals(search::Visible())[0]; };

    // AI2 is a bridge output of ±2 mV, AI3 a 5 V excitation; the bridge block turns them into strain
    channel(2).setPropertyValue("Amplitude", 0.002);
    channel(3).setPropertyValue("Amplitude", 0.0);
    channel(3).setPropertyValue("DC", 5.0);
    const auto bridge = instance.addFunctionBlock("RefFBModuleBridgeReader");
    bridge.getInputPorts()[0].connect(aiSignal(2));
    bridge.getInputPorts()[1].connect(aiSignal(3));
    const SignalPtr strain = bridge.getSignals()[0];

    // The application's judgement: a column keeps the unit it started with until the operator accepts a new one
    std::mutex judgementMutex;
    std::map<std::string, std::string> lockedUnit;
    std::map<std::string, std::string> offeredUnit;
    const auto accepts = Function(
        [&](ComponentPtr input, DataDescriptorPtr value, DataDescriptorPtr, ListPtr<IDataDescriptor>) -> bool
        {
            std::scoped_lock lock(judgementMutex);
            const auto id = input.getGlobalId().toStdString();
            const auto unit = trial::unitOf(value);
            offeredUnit[id] = unit;
            return lockedUnit.emplace(id, unit).first->second == unit;
        });

    std::vector<SignalPtr> inputs = {aiSignal(0), aiSignal(1), strain, aiSignal(3)};
    const std::vector<std::string> labels = {"AI0", "AI1", "Strain", "Excitation"};
    const auto makeParams = [&](const ComponentPtr& main)
    {
        auto params = MultiReader2Params();
        auto list = List<IComponent>();
        for (const auto& s : inputs)
            list.pushBack(s);
        params.setInputs(list);
        params.setErrorPolicy(MultiReader2ErrorPolicy::Exclude);
        params.setAcceptsDescriptor(accepts);
        if (main.assigned())
            params.setMainInput(main);
        return params;
    };
    ComponentPtr pinnedMain;

    std::atomic<bool> running{true};
    driver.start(makeParams(nullptr), labels, false);
    driver.runLoop(running);

    std::vector<Scenario::Step> steps = {
        {"Start: AI0, AI1, Strain and Excitation from one device", nullptr, ComponentStatus::Ok, "",
         "Synchronizes, then Ok; the first status marks every descriptor and the domain new."},
        {"AI1 amplitude 5 V -> 2 V (value range change)",
         [&] { guard.set(channel(1), "Amplitude", 2.0, "AI1"); }, ComponentStatus::Ok, "",
         "descriptorChanged on AI1 after the data in front of it; no resynchronization."},
        {"AI1 on its own rate of 2 kHz",
         [&] {
             guard.set(channel(1), "UseGlobalSampleRate", false, "AI1");
             guard.set(channel(1), "SampleRate", 2000.0, "AI1");
         },
         ComponentStatus::Warning, "AI1: domain descriptor invalid",
         "AI1 DomainDescriptorInvalid (rate differs from the main input); Exclude sets it aside, the rest flows."},
        {"AI1 back on the global rate", [&] { channel(1).setPropertyValue("UseGlobalSampleRate", true); }, ComponentStatus::Ok, "",
         "AI1 rejoins with a synchronization; descriptorChanged on AI1, resynchronized if samples were dropped."},
        {"AI1 rule offset +500 us (half a sample)", [&] { guard.set(channel(1), "Offset", 500, "AI1"); }, ComponentStatus::Warning,
         "AI1: not synchronized, off the main input's grid", "AI1 SyncFailed off grid, found at synchronization; the rest flows."},
        {"AI1 rule offset back to 0", [&] { channel(1).setPropertyValue("Offset", 0); }, ComponentStatus::Ok, "",
         "A new domain descriptor on AI1 clears the off-grid failure; AI1 rejoins."},
        {"Excitation drops to 0.1 V", [&] { guard.set(channel(3), "DC", 0.1, "AI3"); }, ComponentStatus::Ok, "",
         "Reader unaffected (a value change, the range stays); the bridge block writes NaN below MinExcitation."},
        {"Excitation back to 5 V", [&] { channel(3).setPropertyValue("DC", 5.0); }, ComponentStatus::Ok, "", "Unaffected."},
        {"Device global rate 1000 -> 500 Hz", [&] { guard.set(device, "GlobalSampleRate", 500.0, "device0"); }, ComponentStatus::Ok, "",
         "The main input's domain changes: internal reconfigure, domainDescriptorChanged, every descriptor new, resynchronized. "
         "The application starts a new segment."},
        {"Device global rate back to 1000 Hz", [&] { device.setPropertyValue("GlobalSampleRate", 1000.0); }, ComponentStatus::Ok, "",
         "As before; another segment."},
        {"Bridge output quantity Strain -> Ratio (unit um/m -> mV/V)", [&] { guard.set(bridge, "OutputQuantity", 1, "bridge"); },
         ComponentStatus::Warning, "Strain: value descriptor rejected by the owner",
         "The owner's judgement rejects the new unit: ValueDescriptorInvalid, Strain set aside."},
        {"Operator accepts the new unit: re-judge through an equal-params configure",
         [&] {
             {
                 std::scoped_lock lock(judgementMutex);
                 const auto id = strain.getGlobalId().toStdString();
                 lockedUnit[id] = offeredUnit[id];
             }
             driver.reconfigure();
         },
         ComponentStatus::Ok, "", "Equal params skip the reinitialization; the verdict changes and Strain rejoins. New segment (unit)."},
        {"Pin the main input to AI1 (configure)",
         [&] {
             pinnedMain = inputs[1];
             driver.configure(makeParams(pinnedMain), labels);
         },
         ComponentStatus::Ok, "", "configure: a new reader state, main input AI1, every descriptor new."},
        {"Pinned main AI1 on its own 2 kHz rate",
         [&] {
             channel(1).setPropertyValue("UseGlobalSampleRate", false);
             channel(1).setPropertyValue("SampleRate", 2000.0);
         },
         ComponentStatus::Warning, "AI0: domain descriptor invalid",
         "The main input is the reference: its own domain change is an internal reconfigure, after which every other input "
         "breaks the equal-rate constraint and is set aside. AI1 is read alone at 2 kHz; the application starts a segment."},
        {"Pinned main AI1 back on the global rate", [&] { channel(1).setPropertyValue("UseGlobalSampleRate", true); }, ComponentStatus::Ok,
         "", "The reader recovers on its own: internal reconfigure, every descriptor new, resynchronized."},
        {"NumberOfChannels 4 -> 3: the excitation channel goes", [&] { guard.set(device, "NumberOfChannels", 3, "device0"); },
         ComponentStatus::Warning, "Strain: not synchronized, more than 2 s from the main input",
         "AI3 Disconnected and set aside. The bridge loses an input and stops, so Strain sends nothing; once the main input is "
         "more than 2 s ahead, Strain is SyncFailed (late) and set aside, and the rest flows (decision 54)."},
        {"NumberOfChannels back to 4; the application maps AI3 again by global ID and reconnects the bridge",
         [&] {
             device.setPropertyValue("NumberOfChannels", 4);
             channel(3).setPropertyValue("Amplitude", 0.0);
             channel(3).setPropertyValue("DC", 5.0);
             bridge.getInputPorts()[1].connect(aiSignal(3));
             inputs[3] = aiSignal(3);
             driver.configure(makeParams(pinnedMain), labels);
         },
         ComponentStatus::Warning, "AI3: not synchronized, more than 2 s from the main input",
         "configure with the new signal object. The ref device stamps a channel added after its start with the device start "
         "epoch plus the channel's own elapsed time, so the new AI3 lags by its creation delay: SyncFailed (late), for good. "
         "The bridge, also fed by it, stays invalid, so Strain has no sample and fails after 2 s of main data.",
         4000, 3500},
    };

    Scenario scenario(log, driver, dir / "scenario.md");
    scenario.run(steps);

    running = false;
    driver.stop();
    guard.restoreAll();
    log("done: {} samples, {} rows in {} segments, {} mismatches; output in {}", driver.sampleCount(), sink.rowCount(), sink.segmentCount(),
        scenario.mismatches, dir.string());
    return 0;
}
