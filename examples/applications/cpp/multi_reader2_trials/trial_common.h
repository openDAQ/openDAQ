/*
 * Shared code of the MultiReader2 trial applications: a CSV sink, a reader driver that applies the application's
 * policy to each status, a guard that restores every device property it changed, and a scenario runner that drives
 * the devices and records the reader state each step left.
 */

#pragma once
#include <opendaq/opendaq.h>
#include <opendaq/multi_reader2_factory.h>

#include <atomic>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace trial
{

using namespace daq;
using SteadyClock = std::chrono::steady_clock;

inline const char* errorName(MultiReader2InputError error)
{
    switch (error)
    {
        case MultiReader2InputError::None:
            return "None";
        case MultiReader2InputError::Disconnected:
            return "Disconnected";
        case MultiReader2InputError::ValueDescriptorInvalid:
            return "ValueDescriptorInvalid";
        case MultiReader2InputError::DomainDescriptorInvalid:
            return "DomainDescriptorInvalid";
        case MultiReader2InputError::SyncFailed:
            return "SyncFailed";
    }
    return "?";
}

inline const char* levelName(ComponentStatus status)
{
    switch (status)
    {
        case ComponentStatus::Ok:
            return "Ok";
        case ComponentStatus::Warning:
            return "Warning";
        case ComponentStatus::Error:
            return "Error";
    }
    return "?";
}

inline std::string unitOf(const DataDescriptorPtr& descriptor)
{
    const auto unit = descriptor.assigned() ? descriptor.getUnit() : nullptr;
    return unit.assigned() && unit.getSymbol().assigned() ? unit.getSymbol().toStdString() : "";
}

// Timestamped console log, mirrored to events.log in the output directory
class Log
{
public:
    explicit Log(const std::filesystem::path& dir)
        : file(dir / "events.log")
        , start(SteadyClock::now())
    {
    }

    template <typename... Args>
    void operator()(fmt::format_string<Args...> format, Args&&... args)
    {
        const auto line = fmt::format("[{:8.3f}] {}", seconds(), fmt::format(format, std::forward<Args>(args)...));
        std::scoped_lock lock(mutex);
        fmt::println("{}", line);
        file << line << '\n';
        file.flush();
    }

    double seconds() const
    {
        return std::chrono::duration<double>(SteadyClock::now() - start).count();
    }

private:
    std::mutex mutex;
    std::ofstream file;
    SteadyClock::time_point start;
};

// One column per reader input. A recording is split into segments: a new file starts whenever the time base or the
// column set (names, units) changes, as a recorder must not mix setups in one table. An input that does not contribute
// leaves its cells empty. Markers (pauses, resynchronizations, gaps, setup changes) go to a sidecar events file, so the
// data file stays a plain table
class CsvSink
{
public:
    struct Column
    {
        std::string name;
        std::string unit;
    };

    CsvSink(const std::filesystem::path& dir, std::string stem)
        : dir(dir)
        , stem(std::move(stem))
        , events(dir / (this->stem + "_events.csv"))
    {
        events << "tick,time_s,kind,text\n";
    }

    // Starts a new segment when the layout differs from the current one
    void layout(const DataDescriptorPtr& domainDescriptor, const std::vector<Column>& columns)
    {
        const auto rule = domainDescriptor.getRule().getParameters();
        delta = rule.get("delta");
        start = rule.get("start");
        num = domainDescriptor.getTickResolution().getNumerator();
        den = domainDescriptor.getTickResolution().getDenominator();
        const auto info = domainDescriptor.getReferenceDomainInfo();
        referenceOffset = info.assigned() && info.getReferenceDomainOffset().assigned() ? static_cast<Int>(info.getReferenceDomainOffset()) : 0;

        std::string key = fmt::format("{}|{}/{}|{}", delta, num, den, domainDescriptor.getOrigin());
        for (const auto& c : columns)
            key += "|" + c.name + "[" + c.unit + "]";
        if (key == layoutKey && file.is_open())
            return;
        layoutKey = key;
        file.close();
        currentPath = dir / fmt::format("{}_seg{:02}.csv", stem, ++segment);
        file.open(currentPath);
        file << "tick,time_s_since_" << domainDescriptor.getOrigin();
        for (const auto& c : columns)
            file << "," << c.name << (c.unit.empty() ? "" : " [" + c.unit + "]");
        file << '\n';
        width = columns.size();
        marker(-1, "segment", currentPath.filename().string());
    }

    // `buffers` in column order; a null buffer leaves the column empty
    void write(SizeT count, SizeT packetOffset, const std::vector<void*>& buffers)
    {
        if (!file.is_open())
            return;
        std::string row;
        for (SizeT i = 0; i < count; ++i)
        {
            const Int tick = static_cast<Int>(packetOffset) + static_cast<Int>(i) * delta + start;
            row = fmt::format("{},{}", tick, timeOf(tick));
            for (SizeT c = 0; c < width; ++c)
                row += buffers[c] ? fmt::format(",{:.9g}", static_cast<const double*>(buffers[c])[i]) : ",";
            file << row << '\n';
        }
        rows += count;
    }

    void marker(Int packetOffset, const std::string& kind, const std::string& text)
    {
        const Int tick = packetOffset < 0 ? 0 : packetOffset + start;
        events << (packetOffset < 0 ? std::string() : std::to_string(tick)) << "," << (packetOffset < 0 ? std::string() : timeOf(tick)) << ","
               << kind << ",\"" << text << "\"\n";
        events.flush();
    }

    void flush()
    {
        file.flush();
    }

    SizeT rowCount() const
    {
        return rows;
    }

    int segmentCount() const
    {
        return segment;
    }

private:
    // Seconds since the origin, exact to the tick, with the reference domain offset applied
    std::string timeOf(Int tick) const
    {
        const Int total = (tick + referenceOffset) * num;
        const Int whole = total / den;
        const Int frac = (total % den) * 1000000 / den;
        return fmt::format("{}.{:06}", whole, frac < 0 ? -frac : frac);
    }

    std::filesystem::path dir;
    std::string stem;
    std::ofstream file;
    std::ofstream events;
    std::filesystem::path currentPath;
    std::string layoutKey;
    int segment = 0;
    SizeT width = 0;
    SizeT rows = 0;
    Int delta = 1, start = 0, num = 1, den = 1, referenceOffset = 0;
};

// Owns one MultiReader2 and applies the application's policy to its statuses:
//   Error: recording pauses, the cause is shown, nothing is written until the reader recovers on its own;
//   Warning: recording goes on, the inputs set aside show empty cells and are named in the status line;
//   resynchronized: a gap marker with the ticks the next read skipped;
//   new domain or a new column unit/name: a new file segment.
class ReaderDriver
{
public:
    struct Transition
    {
        double time;
        std::string text;
    };

    ReaderDriver(std::string name, Log& log, CsvSink& sink)
        : name(std::move(name))
        , log(log)
        , sink(sink)
    {
    }

    // `labels` names each input's column, in slot order
    void start(const MultiReader2ParamsPtr& readerParams, std::vector<std::string> columnLabels, bool callback)
    {
        std::scoped_lock lock(mutex);
        params = readerParams;
        labels = std::move(columnLabels);
        reader = MultiReader2(params);
        if (callback)
            reader.getOnDataAvailable() += [this](BaseObjectPtr&, EventArgsPtr<>&) { drain(); };
        SizeT count = 0;
        SizeT offset = 0;
        handle(reader.read(nullptr, &count, 0, offset));
    }

    void configure(const MultiReader2ParamsPtr& readerParams, std::vector<std::string> columnLabels)
    {
        std::scoped_lock lock(mutex);
        params = readerParams;
        labels = std::move(columnLabels);
        cached.clear();
        handle(reader.configure(params));
    }

    // Equal params: every input is judged again by acceptsDescriptor, the queued data stays
    void reconfigure()
    {
        std::scoped_lock lock(mutex);
        handle(reader.configure(params));
    }

    // Reads until nothing is deliverable and nothing changed: the samples of each read first, then its status
    SizeT drain()
    {
        std::scoped_lock lock(mutex);
        if (!reader.assigned())
            return 0;
        SizeT total = 0;
        for (;;)
        {
            SizeT count = reader.getAvailableCount();
            const auto inputs = params.getInputs();
            storage.resize(inputs.getCount());
            buffers.assign(inputs.getCount(), nullptr);
            for (SizeT i = 0; i < inputs.getCount(); ++i)
                if (cached.count(inputs[i].getGlobalId().toStdString()))
                {
                    storage[i].resize(count);
                    buffers[i] = storage[i].data();
                }
            SizeT offset = 0;
            const auto status = reader.read(count > 0 ? buffers.data() : nullptr, &count, 0, offset);
            if (count > 0)
            {
                if (gapPending && expectedNext >= 0)
                {
                    const Int skipped = static_cast<Int>(offset) - expectedNext;
                    sink.marker(static_cast<Int>(offset), "gap", fmt::format("{} ticks skipped ({} samples)", skipped, delta ? skipped / delta : 0));
                    note(fmt::format("resumed after a gap of {} ticks ({} samples)", skipped, delta ? skipped / delta : 0));
                }
                gapPending = false;
                sink.write(count, offset, buffers);
                expectedNext = static_cast<Int>(offset) + static_cast<Int>(count) * delta;
                total += count;
                samples += count;
            }
            if (status.getHasChanges())
                handle(status);
            else if (count == 0)
                break;
        }
        return total;
    }

    // A reader loop on its own thread, as an acquisition thread would run it: a read never waits, so it sleeps
    void runLoop(std::atomic<bool>& running, std::chrono::milliseconds period = std::chrono::milliseconds(10))
    {
        loop = std::thread([this, &running, period] {
            while (running)
            {
                drain();
                std::this_thread::sleep_for(period);
            }
        });
    }

    void stop()
    {
        if (loop.joinable())
            loop.join();
        std::scoped_lock lock(mutex);
        reader.release();
        sink.flush();
    }

    ComponentStatus level() const
    {
        std::scoped_lock lock(mutex);
        return currentLevel;
    }

    std::string message() const
    {
        std::scoped_lock lock(mutex);
        return currentMessage;
    }

    std::string mainInput() const
    {
        std::scoped_lock lock(mutex);
        return currentMain;
    }

    SizeT sampleCount() const
    {
        std::scoped_lock lock(mutex);
        return samples;
    }

    // Transitions noted since `since`
    std::vector<Transition> transitionsSince(double since) const
    {
        std::scoped_lock lock(mutex);
        std::vector<Transition> out;
        for (const auto& t : transitions)
            if (t.time >= since)
                out.push_back(t);
        return out;
    }

    const std::string name;

private:
    void note(const std::string& text)
    {
        transitions.push_back({log.seconds(), text});
        log("{}: {}", name, text);
    }

    void handle(const MultiReader2StatusPtr& status)
    {
        const auto level = status.getStatus();
        const std::string message = status.getStatusMessage();
        if (level != currentLevel || message != currentMessage)
        {
            note(fmt::format("{} -> {}{}", levelName(currentLevel), levelName(level), message.empty() ? "" : " (" + message + ")"));
            if (level == ComponentStatus::Error)
                sink.marker(expectedNext, "paused", message);
            else if (currentLevel == ComponentStatus::Error)
                sink.marker(-1, "resumed", message);
        }
        currentLevel = level;
        currentMessage = message;

        const auto main = status.getMainInput();
        const std::string mainId = main.assigned() ? main.getGlobalId().toStdString() : "";
        if (mainId != currentMain)
        {
            note(fmt::format("main input {} -> {}", currentMain.empty() ? "none" : currentMain, mainId.empty() ? "none" : mainId));
            currentMain = mainId;
        }

        const auto inputStatuses = status.getInputs();
        for (SizeT i = 0; i < inputStatuses.getCount(); ++i)
        {
            const MultiReader2InputStatusPtr in = inputStatuses[i];
            const auto id = in.getInput().getGlobalId().toStdString();
            const auto error = in.getError();
            if (errors[id] != error)
            {
                note(fmt::format("input {} {} -> {}", label(i), errorName(errors[id]), errorName(error)));
                errors[id] = error;
            }
        }
        if (level == ComponentStatus::Error)
            return;  // no descriptors under Error; recording waits

        if (status.getDomainDescriptorChanged())
        {
            domain = status.getDomainDescriptor();
            delta = domain.getRule().getParameters().get("delta");
            expectedNext = -1;  // a new time base: the next offset is not a gap against the old one
            note(fmt::format("domain: delta {} ticks at {}/{} s, origin {}",
                             delta,
                             domain.getTickResolution().getNumerator(),
                             domain.getTickResolution().getDenominator(),
                             domain.getOrigin()));
        }
        for (SizeT i = 0; i < inputStatuses.getCount(); ++i)
        {
            const MultiReader2InputStatusPtr in = inputStatuses[i];
            const auto id = in.getInput().getGlobalId().toStdString();
            if (in.getError() != MultiReader2InputError::None)
                cached.erase(id);
            else if (in.getDescriptorChanged())
                cached[id] = in.getDescriptor();
        }
        if (status.getResynchronized())
        {
            gapPending = true;
            note("resynchronized");
        }

        std::vector<CsvSink::Column> columns;
        const auto inputs = params.getInputs();
        for (SizeT i = 0; i < inputs.getCount(); ++i)
        {
            const auto it = cached.find(inputs[i].getGlobalId().toStdString());
            columns.push_back({label(i), it == cached.end() ? lastUnit[i] : (lastUnit[i] = unitOf(it->second))});
        }
        if (domain.assigned())
            sink.layout(domain, columns);
    }

    std::string label(SizeT i) const
    {
        return i < labels.size() ? labels[i] : fmt::format("in{}", i);
    }

    Log& log;
    CsvSink& sink;
    mutable std::recursive_mutex mutex;  // the handler may run while configure holds it on the same thread
    MultiReader2ParamsPtr params;
    MultiReader2Ptr reader;
    std::vector<std::string> labels;
    std::vector<std::vector<double>> storage;
    std::vector<void*> buffers;
    std::map<std::string, DataDescriptorPtr> cached;
    std::map<std::string, MultiReader2InputError> errors;
    std::map<SizeT, std::string> lastUnit;
    DataDescriptorPtr domain;
    Int delta = 1;
    Int expectedNext = -1;
    bool gapPending = false;
    ComponentStatus currentLevel = ComponentStatus::Ok;
    std::string currentMessage;
    std::string currentMain;
    SizeT samples = 0;
    std::vector<Transition> transitions;
    std::thread loop;
};

// Records each property's original value at its first change and restores them in reverse order
class PropertyGuard
{
public:
    explicit PropertyGuard(Log& log)
        : log(log)
    {
    }

    void set(const PropertyObjectPtr& owner, const std::string& property, const BaseObjectPtr& value, const std::string& ownerName)
    {
        const auto original = owner.getPropertyValue(property);
        if (std::none_of(changes.begin(), changes.end(), [&](const auto& c) { return c.owner == owner && c.property == property; }))
            changes.push_back({owner, property, original, ownerName});
        log("set {}.{}: {} -> {}", ownerName, property, original, value);
        owner.setPropertyValue(property, value);
    }

    void restoreAll()
    {
        for (auto it = changes.rbegin(); it != changes.rend(); ++it)
        {
            try
            {
                it->owner.setPropertyValue(it->property, it->original);
                log("restored {}.{} = {} (now {})", it->ownerName, it->property, it->original, it->owner.getPropertyValue(it->property));
            }
            catch (const std::exception& e)
            {
                log("FAILED to restore {}.{} = {}: {}", it->ownerName, it->property, it->original, e.what());
            }
        }
        changes.clear();
    }

    struct Change
    {
        PropertyObjectPtr owner;
        std::string property;
        BaseObjectPtr original;
        std::string ownerName;
    };

private:
    Log& log;
    std::vector<Change> changes;
};

// A step changes something, waits until the driver reaches the expected level and message or the timeout passes, and
// records what the reader went through on the way
class Scenario
{
public:
    struct Step
    {
        std::string name;
        std::function<void()> action;
        ComponentStatus expectLevel;
        std::string expectMessage;  // a substring; empty matches any message
        std::string spec;           // what the spec predicts, for the report
        int timeoutMs = 4000;
        int holdMs = 1500;          // keep reading after the match, so samples flow in the new state
    };

    Scenario(Log& log, ReaderDriver& driver, std::filesystem::path report)
        : log(log)
        , driver(driver)
        , reportPath(std::move(report))
    {
    }

    void run(const std::vector<Step>& steps)
    {
        std::ofstream report(reportPath);
        report << "| # | Step | Spec predicts | Expected | Observed | Transitions | Samples | Match |\n";
        report << "| - | ---- | ------------- | -------- | -------- | ----------- | ------- | ----- |\n";
        int index = 0;
        for (const auto& step : steps)
        {
            ++index;
            log("=== step {}: {}", index, step.name);
            const double t0 = log.seconds();
            const SizeT samples0 = driver.sampleCount();
            std::string error;
            try
            {
                if (step.action)
                    step.action();
            }
            catch (const std::exception& e)
            {
                error = e.what();
                log("step action threw: {}", error);
            }
            const auto deadline = SteadyClock::now() + std::chrono::milliseconds(step.timeoutMs);
            bool match = false;
            while (SteadyClock::now() < deadline)
            {
                match = driver.level() == step.expectLevel &&
                        (step.expectMessage.empty() || driver.message().find(step.expectMessage) != std::string::npos);
                if (match)
                    break;
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(step.holdMs));
            match = driver.level() == step.expectLevel &&
                    (step.expectMessage.empty() || driver.message().find(step.expectMessage) != std::string::npos);

            std::string transitions;
            for (const auto& t : driver.transitionsSince(t0))
                transitions += fmt::format("+{:.2f}s {}<br>", t.time - t0, t.text);
            const auto observed = fmt::format("{}{}", levelName(driver.level()), driver.message().empty() ? "" : ": " + driver.message());
            const auto expected = fmt::format("{}{}", levelName(step.expectLevel), step.expectMessage.empty() ? "" : ": …" + step.expectMessage + "…");
            log("step {} {}: observed {}", index, match ? "MATCH" : "MISMATCH", observed);
            report << fmt::format("| {} | {}{} | {} | {} | {} | {} | {} | {} |\n",
                                  index,
                                  step.name,
                                  error.empty() ? "" : " (threw: " + error + ")",
                                  step.spec,
                                  expected,
                                  observed,
                                  transitions,
                                  driver.sampleCount() - samples0,
                                  match ? "yes" : "**no**");
            report.flush();
            mismatches += match ? 0 : 1;
        }
    }

    int mismatches = 0;

private:
    Log& log;
    ReaderDriver& driver;
    std::filesystem::path reportPath;
};

// The value signals of a component tree that have a domain signal, in tree order
inline std::vector<SignalPtr> valueSignals(const ComponentPtr& root)
{
    std::vector<SignalPtr> out;
    ListPtr<ISignal> signals;
    if (const auto device = root.asPtrOrNull<IDevice>(); device.assigned())
        signals = device.getSignals(search::Recursive(search::Visible()));
    else
        signals = root.asPtr<IFunctionBlock>().getSignals(search::Recursive(search::Visible()));
    for (const auto& s : signals)
        if (s.getDomainSignal().assigned())
            out.push_back(s);
    return out;
}

inline std::filesystem::path outputDir(const std::string& app)
{
    const auto stamp = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    char buf[32];
    std::strftime(buf, sizeof buf, "%Y%m%d-%H%M%S", std::localtime(&stamp));
    auto dir = std::filesystem::current_path() / "mr2-trials" / fmt::format("{}-{}", app, buf);
    std::filesystem::create_directories(dir);
    return dir;
}

}
