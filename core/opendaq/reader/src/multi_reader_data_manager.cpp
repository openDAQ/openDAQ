#include <coretypes/validation.h>
#include <opendaq/custom_log.h>
#include <opendaq/event_packet_ids.h>
#include <opendaq/event_packet_ptr.h>
#include <opendaq/event_packet_utils.h>
#include <opendaq/multi_reader2_status_impl.h>
#include <opendaq/multi_reader_data_manager.h>
#include <opendaq/reader_utils.h>
#include <opendaq/sample_type_traits.h>

#include <algorithm>
#include <cstring>
#include <limits>

BEGIN_NAMESPACE_OPENDAQ

using namespace multi_reader2;

namespace
{
    // As MultiReader2Impl tells inputs apart
    bool sameComponent(const ComponentPtr& a, const ComponentPtr& b)
    {
        return a.getObject() == b.getObject();
    }

    // The same object, or equal by value. The identity check comes first: comparing by value walks the whole descriptor,
    // and the same object is the usual case, such as the descriptors a connect's event packet repeats
    bool sameDescriptor(const DataDescriptorPtr& a, const DataDescriptorPtr& b)
    {
        if (a.getObject() == b.getObject())
            return true;
        return a.assigned() && b.assigned() && a == b;
    }

    SizeT elementCount(const DataDescriptorPtr& descriptor)
    {
        SizeT count = 1;
        const auto dimensions = descriptor.getDimensions();
        if (dimensions.assigned())
        {
            for (const auto& dimension : dimensions)
                count *= dimension.getSize();
        }
        return count;
    }

    // Bytes of one sample in the read buffer: the elements in the read type, or the input's own sample under Undefined
    SizeT readSampleSize(const DataDescriptorPtr& value, SampleType readType)
    {
        if (readType == SampleType::Undefined)
            return value.getSampleSize();
        return getSampleSize(readType) * elementCount(value);
    }
}

// ================================================================ configuration

ErrCode MultiReaderDataManager::reconfigure(Config newConfig, IMultiReader2Status** status)
{
    const SizeT inputCount = newConfig.inputs.size();
    if (newConfig.connected.size() != inputCount || newConfig.valueDescriptors.size() != inputCount ||
        newConfig.domainDescriptors.size() != inputCount)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, "The connection states and descriptors must have one entry per input");

    std::scoped_lock lock(mutex);
    return daqTry([&]
    {
        // Retained inputs keep what they last saw; new ones start from what the reader read off the input
        std::vector<Slot> newSlots(inputCount);
        for (SizeT i = 0; i < inputCount; ++i)
        {
            const auto it = std::find_if(config.inputs.begin(), config.inputs.end(),
                                         [&](const ComponentPtr& input) { return sameComponent(input, newConfig.inputs[i]); });
            if (it != config.inputs.end())
                newSlots[i] = std::move(slots[it - config.inputs.begin()]);
            else
                newSlots[i].latest = {newConfig.connected[i], newConfig.valueDescriptors[i], newConfig.domainDescriptors[i]};
        }
        config = std::move(newConfig);
        slots = std::move(newSlots);

        // As if newly constructed: the queued events are dropped, as latest already holds what they announced, and no
        // SyncFailed, main input or sent tick is carried over
        for (auto& slot : slots)
        {
            slot.queue.total = &queuedBytes;
            slot.queue.clear();
            slot.current = slot.latest;
            slot.syncFailed = SyncCause::None;
            slot.firstSentTick.reset();
            slot.newestTick.reset();
            slot.converter.reset();  // the read type may have changed
        }
        queuedBytes = 0;
        mainSlot.reset();
        tickedMain.reset();
        tickedMainDomain = nullptr;
        for (SizeT i = 0; i < slots.size(); ++i)
            checkInput(i);
        judgeInputs(false);
        restart();

        afresh = true;
        steadyStatus = nullptr;
        recountShort();
        noteChanges();
        if (status)
            *status = buildStatus().detach();
    });
}

ErrCode MultiReaderDataManager::reevaluate(IMultiReader2Status** status)
{
    OPENDAQ_PARAM_NOT_NULL(status);

    std::scoped_lock lock(mutex);
    return daqTry([&]
    {
        const bool wasValid = valid();
        const auto previousMain = mainSlot;
        const auto previous = errors();

        // Every input that passes the reader's own checks is judged again with its descriptors in force
        for (SizeT i = 0; i < slots.size(); ++i)
            checkInput(i);
        judgeInputs(true);
        afterJudgement(wasValid, previousMain, previous, false, false);
        applyStalls();
        recountShort();

        steadyStatus = nullptr;
        noteChanges();
        *status = buildStatus().detach();
    });
}

// What follows a judgement in a read or reevaluate. The reader becoming invalid or valid again, a moved automatic main
// input or a new main domain is an internal reconfigure. Otherwise inputs set aside lose their data, inputs that rejoin
// resynchronize, and so does a contributing input's gap or new domain (resync)
void MultiReaderDataManager::afterJudgement(bool wasValid,
                                            const std::optional<SizeT>& previousMain,
                                            const std::vector<MultiReader2InputError>& previous,
                                            bool reconfigureNeeded,
                                            bool resync)
{
    if (reconfigureNeeded || valid() != wasValid || mainSlot != previousMain)
    {
        internalReconfigure(wasValid);
        return;
    }

    std::vector<SizeT> rejoined;
    for (SizeT i = 0; i < slots.size(); ++i)
    {
        Slot& slot = slots[i];
        if (!contributes(slot))
        {
            // Set aside or still in error, or the reader is invalid: no data in front of its events
            slot.queue.dropFront();
        }
        else if (previous[i] != MultiReader2InputError::None)
        {
            // Rejoins under Exclude, with a synchronization
            slot.descriptorChanged = true;
            rejoined.push_back(i);
        }
    }
    if (!rejoined.empty())
    {
        dropForRejoin(rejoined);
        resync = true;
    }
    if (resync)
        startSync();
    synchronize();
}

// The configure with the params in force, from a read: every queued event goes onto its input and the data is dropped.
// The inputs whose descriptors or connection changed are judged again, and a healthy main input is kept, with the
// SyncFailed of the others
void MultiReaderDataManager::internalReconfigure(bool wasValid)
{
    for (SizeT i = 0; i < slots.size(); ++i)
    {
        Slot& slot = slots[i];
        bool judge = false;
        while (!slot.queue.empty())
        {
            slot.queue.dropFront();
            if (slot.queue.frontIsEvent())
                judge = applyEvent(slot, slot.queue.popEvent()).judge || judge;
        }
        if (judge)
            checkInput(i);
    }

    // A reader that was invalid picks its automatic main input afresh, as a configure would: the one kept while it was
    // invalid depends on the order the inputs recovered in. Should the fresh choice leave the reader invalid, the kept
    // one stays, with the SyncFailed judged against it, which a move of the main input clears
    if (!wasValid && !config.mainSlot.has_value())
    {
        const auto kept = mainSlot;
        std::vector<SyncCause> keptSyncFailed;
        for (const auto& slot : slots)
            keptSyncFailed.push_back(slot.syncFailed);

        judgeInputs(false);
        if (!valid())
        {
            mainSlot = kept;
            for (SizeT i = 0; i < slots.size(); ++i)
                slots[i].syncFailed = keptSyncFailed[i];
            judgeInputs(true);
        }
    }
    else
    {
        judgeInputs(true);
    }
    restart();
}

// The end of every reconfigure, internal or not: the queues are empty of data, everything is reported anew, and
// synchronization starts
void MultiReaderDataManager::restart()
{
    mainDelta = 1;
    mainStart = 0;
    syncLimitTicks = 0;
    if (mainSlot.has_value() && slots[*mainSlot].domain.has_value())
    {
        const TickDomain& main = *slots[*mainSlot].domain;
        mainDelta = main.delta;
        mainStart = main.start;
        syncLimitTicks = main.ticksIn(syncLimitSeconds);
        recoveryLimitTicks = main.ticksIn(recoveryLimitSeconds);
    }

    // The ticks the inputs sent are measured on the main input's grid: they hold while it stays
    const DataDescriptorPtr mainDomain = mainSlot.has_value() ? slots[*mainSlot].current.domain : nullptr;
    const bool sameGrid = mainSlot == tickedMain && mainDomain.getObject() == tickedMainDomain.getObject();
    tickedMain = mainSlot;
    tickedMainDomain = mainDomain;

    for (auto& slot : slots)
    {
        // A new run on every input; the main input may have changed, and with it every main tick
        slot.queue.dropData();
        slot.queue.setStep(mainDelta);
        if (!sameGrid)
        {
            slot.firstSentTick.reset();
            slot.newestTick.reset();
        }
        useLatestDomain(slot);
        slot.descriptorChanged = slot.error == MultiReader2InputError::None;
    }

    // Without a main input there is no stream and no packetOffset to continue
    domainChanged = true;
    resynchronized = mainSlot.has_value();

    // A reader without inputs is valid and has nothing to synchronize
    state = valid() ? State::Syncing : State::Invalid;
    readTick = 0;
    positioned = false;
}

// ================================================================ judgement

// The error the input has on its own, with its descriptors and connection in force, in the order of the error table:
// the owner's verdict once the value descriptor passes the reader's checks, then the domain. The checks against the
// main input are in judgeInputs
void MultiReaderDataManager::checkInput(SizeT i)
{
    Slot& slot = slots[i];
    slot.domain = parseDomain(slot.current.domain);
    slot.rejectedByOwner = false;
    slot.sampleSize = 0;

    if (!slot.current.connected)
        slot.ownError = MultiReader2InputError::Disconnected;
    else if (!slot.current.value.assigned() || !makeConverter(slot))
        slot.ownError = MultiReader2InputError::ValueDescriptorInvalid;
    else if (!askAccept(i))
    {
        slot.ownError = MultiReader2InputError::ValueDescriptorInvalid;
        slot.rejectedByOwner = true;
    }
    else if (!slot.domain.has_value())
        slot.ownError = MultiReader2InputError::DomainDescriptorInvalid;
    else
    {
        slot.ownError = MultiReader2InputError::None;
        slot.sampleSize = readSampleSize(slot.current.value, config.readType);
    }
}

bool MultiReaderDataManager::askAccept(SizeT i)
{
    if (!config.acceptsDescriptor.assigned())
        return true;

    callbackThread = std::this_thread::get_id();
    Finally leave([this] { callbackThread = std::thread::id(); });
    const auto& loggerComponent = config.loggerComponent;
    try
    {
        return config.acceptsDescriptor.call(config.inputs[i], slots[i].current.value, slots[i].current.domain);
    }
    catch (const std::exception& e)
    {
        // An exception counts as a rejection
        daqClearErrorInfo();
        if (loggerComponent.assigned())
            LOG_W("acceptsDescriptor threw for {}: {}", config.inputs[i].getLocalId().toStdString(), e.what());
        return false;
    }
    catch (...)
    {
        daqClearErrorInfo();
        if (loggerComponent.assigned())
            LOG_W("acceptsDescriptor threw for {}", config.inputs[i].getLocalId().toStdString());
        return false;
    }
}

// Chooses the main input from the inputs' own errors, then sets every input's reported error and the reader's level.
// keepMain: a healthy automatic main input stays, as on an internal reconfigure (Main input)
void MultiReaderDataManager::judgeInputs(bool keepMain)
{
    const auto previousMain = mainSlot;
    for (auto& slot : slots)
    {
        // A stall is the input's own, as its other errors: it keeps the input from becoming the main input
        slot.error = slot.ownError;
        if (slot.error == MultiReader2InputError::None && slot.syncFailed == SyncCause::Stalled)
            slot.error = MultiReader2InputError::SyncFailed;
    }

    // Pinned, kept while healthy, or the first input without an error. A move prefers an input that was synchronized
    // with the main input it replaces, so the inputs read together stay the same as far as they can; SyncFailed is
    // judged against that main input, so an input that failed it is taken only when no other input is healthy
    if (config.mainSlot.has_value())
    {
        mainSlot = config.mainSlot;
    }
    else if (!(keepMain && mainSlot.has_value() && slots[*mainSlot].error == MultiReader2InputError::None))
    {
        const auto firstHealthy = [this](bool synchronized) -> std::optional<SizeT>
        {
            for (SizeT i = 0; i < slots.size(); ++i)
            {
                if (slots[i].error == MultiReader2InputError::None && (!synchronized || slots[i].syncFailed == SyncCause::None))
                    return i;
            }
            return std::nullopt;
        };
        mainSlot = firstHealthy(true);
        if (!mainSlot.has_value())
            mainSlot = firstHealthy(false);
    }

    // Every comparison is against the main input; none is possible while it is in error itself
    if (mainSlot.has_value() && slots[*mainSlot].error == MultiReader2InputError::None)
    {
        const TickDomain& main = *slots[*mainSlot].domain;
        for (SizeT i = 0; i < slots.size(); ++i)
        {
            if (i != *mainSlot && slots[i].error == MultiReader2InputError::None && !relate(*slots[i].domain, main).has_value())
                slots[i].error = MultiReader2InputError::DomainDescriptorInvalid;
        }
    }

    // A cause relative to the main input survives only while the main input stays, and the main input never has one
    SizeT inError = 0;
    for (SizeT i = 0; i < slots.size(); ++i)
    {
        Slot& slot = slots[i];
        if ((mainSlot != previousMain || mainSlot == i) && slot.syncFailed != SyncCause::Stalled)
            slot.syncFailed = SyncCause::None;
        if (slot.syncFailed != SyncCause::None && slot.error == MultiReader2InputError::None)
            slot.error = MultiReader2InputError::SyncFailed;
        if (slot.error != MultiReader2InputError::None)
            ++inError;
    }

    // Ok: every input is healthy. Invalidate: any error invalidates. Exclude: only no healthy input, or the pinned
    // main input in error
    if (inError == 0)
        level = ComponentStatus::Ok;
    else if (config.errorPolicy == MultiReader2ErrorPolicy::Invalidate || inError == slots.size() ||
             (config.mainSlot.has_value() && slots[*config.mainSlot].error != MultiReader2InputError::None))
        level = ComponentStatus::Error;
    else
        level = ComponentStatus::Warning;
}

std::vector<MultiReader2InputError> MultiReaderDataManager::errors() const
{
    std::vector<MultiReader2InputError> result;
    result.reserve(slots.size());
    for (const auto& slot : slots)
        result.push_back(slot.error);
    return result;
}

// ================================================================ read

SizeT MultiReaderDataManager::getAvailableCount()
{
    std::scoped_lock lock(mutex);
    return statusOwed ? 0 : frontAvailable();
}

ErrCode MultiReaderDataManager::read(void** data, std::int64_t* domain, SizeT* count, SizeT timeoutMs, SizeT* packetOffset, IMultiReader2Status** status)
{
    OPENDAQ_PARAM_NOT_NULL(count);
    OPENDAQ_PARAM_NOT_NULL(status);

    // The samples are taken off the queues under the mutex and converted after it, so a large read does not hold up
    // ingest, and a conversion that fails leaves every input at the same position. Reused per thread, so a streaming
    // read allocates nothing
    thread_local std::vector<Copy> copies;
    struct ClearCopies
    {
        std::vector<Copy>& copies;
        ~ClearCopies()
        {
            copies.clear();
        }
    } clearCopies{copies};

    decltype(dropped) released;  // freed after the mutex
    {
        std::scoped_lock lock(mutex);
        released.swap(dropped);
        const ErrCode errCode = daqTry([&]
        {
            OPENDAQ_RETURN_IF_FAILED(checkRead(*count, timeoutMs, data, false));
            performRead(data, domain, count, packetOffset, status, &copies);
            return OPENDAQ_SUCCESS;
        });
        OPENDAQ_RETURN_IF_FAILED(errCode);
    }

    const ErrCode errCode = daqTry([&]
    {
        for (const auto& copy : copies)
            runCopy(copy);
    });
    if (OPENDAQ_FAILED(errCode))
    {
        // The samples left the queues, but the caller gets no status: the next one reports everything afresh, so no change
        // the lost status carried goes unreported, and resynchronized says the samples are gone
        releaseRefIfNotNull(*status);
        *status = nullptr;
        std::scoped_lock lock(mutex);
        reportAfresh();
    }
    return errCode;
}

ErrCode MultiReaderDataManager::skipSamples(SizeT* count, SizeT timeoutMs, SizeT* packetOffset, IMultiReader2Status** status)
{
    OPENDAQ_PARAM_NOT_NULL(count);
    OPENDAQ_PARAM_NOT_NULL(status);

    decltype(dropped) released;  // freed after the mutex
    std::scoped_lock lock(mutex);
    released.swap(dropped);
    return daqTry([&]
    {
        OPENDAQ_RETURN_IF_FAILED(checkRead(*count, timeoutMs, nullptr, true));
        performRead(nullptr, nullptr, count, packetOffset, status, nullptr);
        return OPENDAQ_SUCCESS;
    });
}

// count 0 with null data is a status probe; otherwise every contributing input needs a buffer, while the domain buffer
// may be null. skipSamples takes none
ErrCode MultiReaderDataManager::checkRead(SizeT count, SizeT timeoutMs, void** data, bool skip) const
{
    if (timeoutMs != 0)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOTIMPLEMENTED, "A read never waits; timeoutMs must be 0");
    if (count == 0)
        return OPENDAQ_SUCCESS;
    if (count < config.minReadCount)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, "The count is below MinReadCount");
    if (skip || !valid())
        return OPENDAQ_SUCCESS;

    // Buffers are needed only for contributing inputs; a reader without any, such as one without inputs, reads count 0
    for (SizeT i = 0; i < slots.size(); ++i)
    {
        if (slots[i].error == MultiReader2InputError::None && (data == nullptr || data[i] == nullptr))
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, "A contributing input has no buffer");
    }
    return OPENDAQ_SUCCESS;
}

// The read: the samples every contributing input has in front of its first event, unless a status is owed, then the
// events the read reached and the status. skipSamples copies nothing; a domain buffer gets the main ticks
void MultiReaderDataManager::performRead(
    void** data, std::int64_t* domain, SizeT* count, SizeT* packetOffset, IMultiReader2Status** status, std::vector<Copy>* copies)
{
    applyStalls();
    dropShortRun();

    const SizeT n = statusOwed ? 0 : std::min(*count, frontAvailable());
    if (packetOffset)
        *packetOffset = n > 0 ? static_cast<SizeT>(readTick - mainStart) : 0;
    if (n > 0)
    {
        if (domain)
        {
            for (SizeT k = 0; k < n; ++k)
                domain[k] = readTick + static_cast<std::int64_t>(k) * mainDelta;
        }
        for (SizeT i = 0; i < slots.size(); ++i)
        {
            if (slots[i].error == MultiReader2InputError::None)
                take(slots[i], n, data ? data[i] : nullptr, copies);
        }
        readTick += static_cast<std::int64_t>(n) * mainDelta;
        positioned = true;
    }
    *count = n;

    applyEvents();
    recountShort();
    *status = buildStatus().detach();
}

// The samples every healthy input has in front of its first event; 0 while synchronizing or below MinReadCount. While
// streaming, every contributing input's front starts at readTick
SizeT MultiReaderDataManager::frontAvailable() const
{
    if (state != State::Streaming)
        return 0;

    SizeT count = std::numeric_limits<SizeT>::max();
    for (const auto& slot : slots)
    {
        if (slot.error != MultiReader2InputError::None)
            continue;
        count = std::min(count, slot.queue.frontCount());
        if (count == 0)
            return 0;
    }
    return count != std::numeric_limits<SizeT>::max() && count >= config.minReadCount ? count : 0;
}

// frontAvailable() > 0, from the counts: streaming, with at least MinReadCount samples in front of every healthy input
bool MultiReaderDataManager::readable() const
{
    return state == State::Streaming && healthyInputs > 0 && shortInputs == 0;
}

bool MultiReaderDataManager::isShort(const Slot& slot) const
{
    return slot.error == MultiReader2InputError::None && slot.queue.frontCount() < config.minReadCount;
}

// After every change that removes queued data or changes the inputs' errors: a removal can leave an input with nothing
// queued, so the stall check is planned afresh too
void MultiReaderDataManager::recountShort()
{
    stallCheckTick.reset();
    healthyInputs = 0;
    shortInputs = 0;
    waitingInputs = 0;
    for (const auto& slot : slots)
    {
        healthyInputs += slot.error == MultiReader2InputError::None ? 1 : 0;
        shortInputs += isShort(slot) ? 1 : 0;
        waitingInputs += isWaiting(slot) ? 1 : 0;
    }
}

bool MultiReaderDataManager::isWaiting(const Slot& slot) const
{
    return slot.error == MultiReader2InputError::None && slot.queue.frontCount() == 0 &&
           !(slot.queue.frontIsEvent() && slot.queue.frontEvent().kind == Event::Kind::SyncFailed);
}

// Fewer than MinReadCount samples in front of an event can never be read: their count, when the contributing inputs'
// shortest front run is such a run. A run without an event behind it can still grow and is kept
std::optional<SizeT> MultiReaderDataManager::shortRun() const
{
    if (state != State::Streaming)
        return std::nullopt;

    std::optional<SizeT> least;
    bool closed = false;
    for (const auto& slot : slots)
    {
        if (slot.error != MultiReader2InputError::None)
            continue;
        const SizeT front = slot.queue.frontCount();
        const bool hasEvent = slot.queue.eventCount() > 0;
        if (!least.has_value() || front < *least)
        {
            least = front;
            closed = hasEvent;
        }
        else if (front == *least)
            closed = closed || hasEvent;
    }
    if (!least.has_value() || *least == 0 || *least >= config.minReadCount || !closed)
        return std::nullopt;
    return least;
}

// The read drops a short run on every contributing input, so it reaches the event behind it
void MultiReaderDataManager::dropShortRun()
{
    const auto count = shortRun();
    if (!count.has_value())
        return;

    for (auto& slot : slots)
    {
        if (slot.error == MultiReader2InputError::None)
            slot.queue.skip(*count);
    }
    readTick += static_cast<std::int64_t>(*count) * mainDelta;
    markResynchronized();
}

// Takes count samples off the input's front run, queueing their copy into out unless it is null
void MultiReaderDataManager::take(Slot& slot, SizeT count, void* out, std::vector<Copy>* copies)
{
    if (!out || !copies)
    {
        slot.queue.skip(count);
        return;
    }

    const SizeT size = slot.sampleSize;
    slot.queue.take(count,
                     [&](const DataPacketPtr& packet, SizeT first, SizeT taken)
                     {
                         copies->push_back({packet, first, taken, out, slot.converter, size});
                         out = static_cast<std::uint8_t*>(out) + taken * size;
                     });
}

// The readers' own conversion to the read type, made when a value descriptor takes effect. It also judges whether the
// value type converts, so the reader applies the same rule as the other readers. Under Undefined the samples are copied
// as they are. TODO: TypedReadingUtils::readData of openDAQ/openDAQ#1215 can replace the reader here (todo file)
bool MultiReaderDataManager::makeConverter(Slot& slot) const
{
    slot.converter.reset();
    if (config.readType == SampleType::Undefined)
        return true;

    std::shared_ptr<Reader> converter = createReaderForType(config.readType, nullptr);
    DataDescriptorPtr descriptor = slot.current.value;
    if (!converter->handleDescriptorChanged(descriptor, ReadMode::Scaled))
        return false;
    slot.converter = std::move(converter);
    return true;
}

// Runs outside the mutex: the packet and the converter are the copy's own
void MultiReaderDataManager::runCopy(const Copy& copy)
{
    auto* input = static_cast<std::uint8_t*>(copy.packet.getData());
    if (!copy.converter)
    {
        std::memcpy(copy.out, input + copy.first * copy.sampleSize, copy.count * copy.sampleSize);
        return;
    }

    void* out = copy.out;
    checkErrorInfo(copy.converter->readData(input, copy.first, &out, copy.count));
}

// Whether a read applies an event on the input: a contributing input the one in front of it, which the read reached;
// any other input every queued one
bool MultiReaderDataManager::reachesEvent(const Slot& slot, bool contributing)
{
    return contributing ? slot.queue.frontIsEvent() : slot.queue.eventCount() > 0;
}

// What a read changes after its samples: the events it reached, the judgement, and the internal
// reconfigure, set-aside, rejoin or resynchronization that follows from it
void MultiReaderDataManager::applyEvents()
{
    // A read that reaches no event changes nothing: the streaming read returns here. A round can put a new event in front of
    // an input, such as the synchronization failure it found, with no data before it: the same read applies it
    while (std::any_of(slots.begin(), slots.end(), [this](const Slot& slot) { return reachesEvent(slot, contributes(slot)); }))
        applyReachedEvents();
}

// One round of applyEvents
void MultiReaderDataManager::applyReachedEvents()
{
    const bool wasValid = valid();
    const auto previousMain = mainSlot;
    const auto previous = errors();

    bool reconfigureNeeded = false;
    bool resync = false;
    for (SizeT i = 0; i < slots.size(); ++i)
    {
        Slot& slot = slots[i];

        // A contributing input applies the events in front of it, which the read reached. Any other input delivers
        // nothing: it applies all of them and keeps only the data behind the last
        const bool contributing = contributes(slot);
        bool judge = false;
        while (reachesEvent(slot, contributing))
        {
            slot.queue.dropFront();
            const Applied applied = applyEvent(slot, slot.queue.popEvent());
            judge = judge || applied.judge;
            if (applied.domain && mainSlot == i)
                reconfigureNeeded = true;
            else if (contributing && (applied.domain || applied.gap))
                resync = true;
        }
        if (judge)
            checkInput(i);
    }

    judgeInputs(true);
    afterJudgement(wasValid, previousMain, previous, reconfigureNeeded, resync);
    noteChanges();
}

// One event onto its input's state in force
MultiReaderDataManager::Applied MultiReaderDataManager::applyEvent(Slot& slot, const Event& event)
{
    Applied applied;
    switch (event.kind)
    {
        case Event::Kind::Descriptor:
        case Event::Kind::Connected:
            applied.judge = true;
            applied.domain = !sameDescriptor(event.domain, slot.current.domain);
            if (!sameDescriptor(event.value, slot.current.value))
                slot.descriptorChanged = true;
            slot.current.value = event.value;
            slot.current.domain = event.domain;
            if (event.kind == Event::Kind::Connected)
                slot.current.connected = true;

            // A new domain descriptor or a connect clears an off-grid input
            if (slot.syncFailed == SyncCause::OffGrid && (applied.domain || event.kind == Event::Kind::Connected))
                slot.syncFailed = SyncCause::None;
            break;
        case Event::Kind::Disconnected:
            applied.judge = true;
            slot.current.connected = false;
            break;
        case Event::Kind::Gap:
            // A gap clears an off-grid input too; on any other failed input it changes nothing
            applied.gap = true;
            if (slot.syncFailed == SyncCause::OffGrid)
                slot.syncFailed = SyncCause::None;
            break;
        case Event::Kind::SyncFailed:
            slot.syncFailed = event.cause;
            break;
        case Event::Kind::Rejoined:
            if (slot.syncFailed == SyncCause::Ahead || slot.syncFailed == SyncCause::Stalled)
                slot.syncFailed = SyncCause::None;
            break;
    }
    return applied;
}

// ================================================================ ingest

bool MultiReaderDataManager::connected(SizeT slot, const DataDescriptorPtr& value, const DataDescriptorPtr& domain)
{
    std::scoped_lock lock(mutex);
    return claim(queueConnect(slot, value, domain));
}

bool MultiReaderDataManager::disconnected(SizeT slot)
{
    std::scoped_lock lock(mutex);
    return claim(queueDisconnect(slot));
}

std::unique_lock<std::mutex> MultiReaderDataManager::lockIngest(bool wait)
{
    return wait ? std::unique_lock<std::mutex>(mutex) : std::unique_lock<std::mutex>(mutex, std::try_to_lock);
}

// Under lockIngest's lock
MultiReaderDataManager::Ingested MultiReaderDataManager::addPackets(const std::unique_lock<std::mutex>&,
                                                                    SizeT slot,
                                                                    const PacketPtr* packets,
                                                                    SizeT count)
{
    Ingested ingested;
    for (SizeT i = 0; i < count; ++i)
        ingested |= ingestPacket(slot, packets[i]);
    ingested.dropped = !dropped.empty();
    return ingested;
}

void MultiReaderDataManager::releaseDropped()
{
    decltype(dropped) released;
    std::scoped_lock lock(mutex);
    released.swap(dropped);
}

// After a sender's packets or an ingest run, under lockIngest's lock: an event or a synchronization outcome is owed a call,
// a data packet only when samples are readable
bool MultiReaderDataManager::claimWake(const std::unique_lock<std::mutex>&, const Ingested& ingested)
{
    return claim(ingested.trigger || (ingested.data && readable()) || (retryCall && triggerHolds()));
}

bool MultiReaderDataManager::queueConnect(SizeT i, const DataDescriptorPtr& value, const DataDescriptorPtr& domain)
{
    Slot& slot = slots[i];

    // A connect the reader already has, such as one configure read before it took the port over
    if (slot.latest.connected)
        return queueDescriptor(i, value, domain);

    // A disconnect and reconnect with nothing between them cancel out. The run continues, so a jump to the new signal's
    // data is a gap
    slot.latest.connected = true;
    if (slot.queue.cancelTrailingDisconnect())
        return queueDescriptor(i, value, domain);

    // Possibly another signal: a new run, which continues nothing
    slot.latest.value = value;
    slot.latest.domain = domain;
    useLatestDomain(slot);
    slot.queue.pushEvent({Event::Kind::Connected, value, domain});
    return true;
}

bool MultiReaderDataManager::queueDisconnect(SizeT i)
{
    Slot& slot = slots[i];
    if (!slot.latest.connected)
        return false;

    slot.latest.connected = false;
    slot.queue.pushEvent({Event::Kind::Disconnected});
    return true;
}

// Queues a descriptor event unless both descriptors equal the ones last seen
bool MultiReaderDataManager::queueDescriptor(SizeT i, const DataDescriptorPtr& value, const DataDescriptorPtr& domain)
{
    Slot& slot = slots[i];
    if (sameDescriptor(value, slot.latest.value) && sameDescriptor(domain, slot.latest.domain))
        return false;

    const bool newDomain = !sameDescriptor(domain, slot.latest.domain);
    slot.latest.value = value;
    slot.latest.domain = domain;
    if (newDomain)
        useLatestDomain(slot);
    slot.queue.pushEvent({Event::Kind::Descriptor, value, domain});
    return true;
}

MultiReaderDataManager::Ingested MultiReaderDataManager::ingestPacket(SizeT i, const PacketPtr& packet)
{
    switch (packet.getType())
    {
        case PacketType::Event:
        {
            // The gap event packet is ignored with the others: the continuity check finds the same jump
            const auto eventPacket = packet.asPtr<IEventPacket>(true);
            if (eventPacket.getEventId() != event_packet_id::DATA_DESCRIPTOR_CHANGED)
                return {};
            const auto [valueNew, domainNew, value, domain] = parseDataDescriptorEventPacket(eventPacket);
            const Slot& slot = slots[i];
            return {queueDescriptor(i, valueNew ? value : slot.latest.value, domainNew ? domain : slot.latest.domain), false};
        }
        case PacketType::Data:
        {
            const auto dataPacket = packet.asPtr<IDataPacket>(true);
            if (dataPacket.getSampleCount() == 0)
                return {};
            return {ingestData(i, dataPacket), true};
        }
        default:
            return {};
    }
}

// Queues a data packet, or drops it while the input does not contribute and has no event queued; returns whether it
// queued an event or moved the synchronization
bool MultiReaderDataManager::ingestData(SizeT i, const DataPacketPtr& packet)
{
    Slot& slot = slots[i];
    const SizeT count = packet.getSampleCount();

    std::optional<std::int64_t> offset;
    const auto domainPacket = packet.getDomainPacket();
    if (domainPacket.assigned())
    {
        if (const auto packetOffset = domainPacket.getOffset(); packetOffset.assigned())
            offset = packetOffset.getIntValue();
    }

    // Continuity in the input's own ticks, so it holds without a main input
    const bool gap = offset.has_value() && slot.nextOffset.has_value() && *offset != *slot.nextOffset;
    slot.nextOffset.reset();
    if (offset.has_value() && slot.latestDomain.has_value())
    {
        if (const auto span = checkedMul(static_cast<std::int64_t>(count), slot.latestDomain->delta))
            slot.nextOffset = checkedAdd(*offset, *span);
    }

    // The ticks it sent, dropped or not, on the main input's grid
    const auto firstTick = offset.has_value() && slot.toMain.has_value() ? slot.toMain->firstTick(*offset) : std::nullopt;
    if (firstTick.has_value())
    {
        if (!slot.firstSentTick.has_value())
        {
            slot.firstSentTick = firstTick;
            // An input that sent nothing is measured from the senders' first sample, which this one may move earlier
            if (stallCheckTick.has_value())
            {
                const auto from = checkedSub(*firstTick, mainDelta);
                const auto due = from.has_value() ? checkedAdd(*from, syncLimitTicks) : std::nullopt;
                stallCheckTick = std::min(*stallCheckTick, due.value_or(std::numeric_limits<std::int64_t>::min()));
            }
        }
        if (const auto span = checkedMul(static_cast<std::int64_t>(count - 1), mainDelta))
            slot.newestTick = checkedAdd(*firstTick, *span);
    }

    // While the reader is invalid or the input is in error, data with no event ahead of it is dropped
    bool keep = slot.queue.eventCount() > 0 || contributes(slot);
    const bool wasShort = isShort(slot);
    const bool wasWaiting = isWaiting(slot);
    const bool frontWasEmpty = slot.queue.frontCount() == 0;

    // A gap ends the run. On an off-grid input it clears the failure, so it is queued even while the data is dropped, and
    // the data behind it waits for the read that applies it
    bool trigger = false;
    if (gap && (keep || slot.syncFailed == SyncCause::OffGrid))
    {
        slot.queue.pushEvent({Event::Kind::Gap});
        keep = true;
        trigger = true;
    }

    bool droppedAll = false;
    if (keep)
    {
        // Past the critical limit the inputs are read too slowly: every queue goes. Past the sync limit nothing goes, and
        // the read this calls for judges the inputs for a stall
        const SizeT bytes = packet.getRawDataSize() + (domainPacket.assigned() ? domainPacket.getRawDataSize() : 0);
        const bool wasOverSync = over(slot, slot.syncLimitSamples, syncLimitBytes, 0, 0);
        if (over(slot, slot.criticalLimitSamples, criticalLimitBytes, count, bytes) ||
            (queuedBytes > 0 && queuedBytes + bytes > totalLimitBytes))
        {
            dropAll(i);
            droppedAll = true;
            trigger = true;
        }
        slot.queue.pushData(packet, count, firstTick, bytes);
        trigger = (!wasOverSync && over(slot, slot.syncLimitSamples, syncLimitBytes, 0, 0)) || trigger;
    }

    if (droppedAll)
        recountShort();
    else
    {
        if (isShort(slot) != wasShort)
            shortInputs = wasShort ? shortInputs - 1 : shortInputs + 1;
        if (isWaiting(slot) != wasWaiting)
            waitingInputs = wasWaiting ? waitingInputs - 1 : waitingInputs + 1;
    }

    trigger = checkRejoin(i) || trigger;
    if (state == State::Syncing)
    {
        // The common start moves, and the synchronization can finish, only when an input's front run starts: the latest
        // first tick is the start, and an input whose data lies before it is emptied, so its next packet starts a run. The
        // pass over every input runs when the main input starts a run, which every input is judged against, and when no
        // input is left to wait for, so the start can be found; any other input is judged on its own when its run starts.
        // A pass for every new input cost a reader of 6,144 inputs more than a second at its start. The pass may drop data
        // in front of any input and leave one that sent nothing empty
        if (frontWasEmpty && slot.queue.frontCount() > 0)
        {
            if (mainSlot == i || waitingInputs == 0)
            {
                trigger = synchronize() || trigger;
                recountShort();
            }
            else if (judgeStart(i))
            {
                trigger = true;
                recountShort();
            }
        }
        // A stall that is due is owed a call, which applies it. Only a contributing input's samples measure a stall
        if (!stallCheckTick.has_value() ||
            (contributes(slot) && slot.newestTick.has_value() && *slot.newestTick > *stallCheckTick))
            trigger = checkStall() || trigger;
    }
    return trigger;
}

// The input's latest domain, which its next packets carry: its ticks, their map onto the main input's domain in force (none
// without a main input, or when the two cannot be read together), and its limits in samples. A new domain
// starts a new run, which continues nothing
void MultiReaderDataManager::useLatestDomain(Slot& slot) const
{
    slot.latestDomain = parseDomain(slot.latest.domain);
    constexpr auto unlimited = std::numeric_limits<std::int64_t>::max();
    slot.syncLimitSamples = slot.latestDomain.has_value() ? slot.latestDomain->samplesIn(syncLimitSeconds) : unlimited;
    slot.criticalLimitSamples = slot.latestDomain.has_value() ? slot.latestDomain->samplesIn(criticalLimitSeconds) : unlimited;
    slot.toMain.reset();
    if (mainSlot.has_value() && slot.latestDomain.has_value() && slots[*mainSlot].domain.has_value())
        slot.toMain = relate(*slot.latestDomain, *slots[*mainSlot].domain);
    slot.nextOffset.reset();
}

// Whether the input's queue, with addedCount samples and addedBytes more, holds more than samples of its data or more than
// bytes. The time counts the queued samples at the input's own rate, so a gap among them adds none. A packet alone never
// passes a limit, so the newest packet always fits
bool MultiReaderDataManager::over(const Slot& slot, std::int64_t samples, SizeT bytes, SizeT addedCount, SizeT addedBytes)
{
    if (!slot.queue.hasData())
        return false;
    return slot.queue.byteCount() + addedBytes > bytes || slot.queue.sampleCount() + addedCount > static_cast<std::uint64_t>(samples);
}

// Past the critical limit, found on input i: the inputs are read too slowly. Every input that holds data loses it and has a
// gap queued in its place, so each reports the loss and synchronizes afresh from the newest data; an input that held nothing
// loses nothing, and stays free to stall. The packets are freed after the mutex
void MultiReaderDataManager::dropAll(SizeT i)
{
    const auto& loggerComponent = config.loggerComponent;
    if (loggerComponent.assigned())
        LOG_W("{}: past the critical limit of {} s or {} MB queued, or {} MB over all inputs; the queued samples of every input "
              "are dropped",
              config.inputs[i].getLocalId().toStdString(), criticalLimitSeconds, criticalLimitBytes / 1000000,
              totalLimitBytes / 1000000);
    for (auto& slot : slots)
    {
        if (!slot.queue.hasData())
            continue;
        dropped.push_back(slot.queue.dropData());
        slot.queue.pushEvent({Event::Kind::Gap});
    }
}

// An input that rejoins with no data queued has nothing that could align with what the others queued: what it sent while
// set aside was dropped, and what it sends next is newer. Their data goes too, events kept, so the synchronization judges
// only what arrives after the rejoin, and an input is not behind for a backlog its consumer left unread
void MultiReaderDataManager::dropForRejoin(const std::vector<SizeT>& rejoined)
{
    if (std::all_of(rejoined.begin(), rejoined.end(), [this](SizeT i) { return slots[i].queue.hasData(); }))
        return;
    for (auto& slot : slots)
        slot.queue.dropData();
}

// ================================================================ synchronization

// A synchronization trigger while valid, such as a rejoin under Exclude
void MultiReaderDataManager::startSync()
{
    if (state == State::Invalid)
        return;
    state = State::Syncing;
}

// The check mechanism over the inputs' front runs, on every ingest and after a read applies events, while
// synchronizing: the grid, the inputs ahead, the common start. Returns whether it reached the common start or failed an input
bool MultiReaderDataManager::synchronize()
{
    if (state != State::Syncing || !mainSlot.has_value())
        return false;

    // While the main input's front is an event, nobody is judged
    Slot& main = slots[*mainSlot];
    const auto mainFront = main.queue.frontTick();
    if (!mainFront.has_value())
        return false;
    const std::int64_t mainFirst = *mainFront;
    const std::int64_t mainLatest = main.queue.frontLastTick();
    const auto judged = [this](SizeT i)
    { return i != *mainSlot && slots[i].error == MultiReader2InputError::None && slots[i].queue.frontCount() > 0; };

    // Grid first (step 1), as an off-grid first tick cannot be the common start; then ahead (step 2); the inputs that
    // pass propose the common start (step 3). A failed input's marker is an event in front, so it takes no further part
    bool reported = false;
    std::int64_t common = mainFirst;
    for (SizeT i = 0; i < slots.size(); ++i)
    {
        if (!judged(i))
            continue;
        Slot& slot = slots[i];
        if (const auto cause = startFailure(slot, mainFirst, mainLatest))
        {
            failSync(slot, *cause);
            reported = true;
        }
        else
            common = std::max(common, *slot.queue.frontTick());
    }

    // Discard in front of the common start (step 5); an input whose data is all behind it is left empty
    main.queue.discardBefore(common);
    for (SizeT i = 0; i < slots.size(); ++i)
    {
        if (judged(i))
            slots[i].queue.discardBefore(common);
    }

    // An input with no data at the common start is waited for; one that stays silent is stalled at a read (applyStalls)

    // Streaming begins once every contributing input is at the common start or has failed
    for (const auto& slot : slots)
    {
        if (slot.error != MultiReader2InputError::None)
            continue;
        if (slot.queue.frontIsEvent() ? slot.queue.frontEvent().kind != Event::Kind::SyncFailed : slot.queue.frontTick() != common)
            return reported;
    }

    // A start other than where the last read that delivered samples ended is a jump in packetOffset; without such a read
    // there is nothing to continue. The restart reports its own
    if (positioned && common != readTick)
        markResynchronized();
    state = State::Streaming;
    readTick = common;
    return true;
}

// How an input's front run fails against the main input's, if it does: grid first, as an off-grid first tick cannot be the
// common start, then more than the sync limit past the main input's newest sample
std::optional<SyncCause> MultiReaderDataManager::startFailure(const Slot& slot, std::int64_t mainFirst, std::int64_t mainLatest) const
{
    const auto first = slot.queue.frontTick();
    const auto distance = first.has_value() ? checkedSub(*first, mainFirst) : std::nullopt;
    if (!distance.has_value() || *distance % mainDelta != 0)
        return SyncCause::OffGrid;
    if (exceeds(*first, mainLatest, syncLimitTicks))
        return SyncCause::Ahead;
    return std::nullopt;
}

// A non-main input whose front run starts while synchronizing, judged against the main input on its own, as the pass over
// every input would judge it. Nobody is judged while the main input has nothing in front: its next run judges everyone.
// Returns whether the input failed
bool MultiReaderDataManager::judgeStart(SizeT i)
{
    if (!mainSlot.has_value() || i == *mainSlot || slots[i].error != MultiReader2InputError::None)
        return false;
    const Slot& main = slots[*mainSlot];
    const auto mainFront = main.queue.frontTick();
    if (!mainFront.has_value())
        return false;
    const auto cause = startFailure(slots[i], *mainFront, main.queue.frontLastTick());
    if (!cause.has_value())
        return false;
    failSync(slots[i], *cause);
    return true;
}

// Drops the input's data and queues the failure in front of it, where the next read applies it
void MultiReaderDataManager::failSync(Slot& slot, SyncCause cause)
{
    slot.queue.dropFront();
    slot.queue.pushFrontEvent({Event::Kind::SyncFailed, nullptr, nullptr, cause});
}

// A stalled or ahead input with nothing queued in front of its data rejoins once its newest sample is within the recovery
// limit of the newest sample of the other healthy inputs, or on its own packet when none of them has sent. The marker is
// applied by the next read; the data behind it is kept
bool MultiReaderDataManager::checkRejoin(SizeT i)
{
    Slot& slot = slots[i];
    if ((slot.syncFailed != SyncCause::Stalled && slot.syncFailed != SyncCause::Ahead) || slot.queue.eventCount() > 0)
        return false;

    std::optional<std::int64_t> newest;
    for (SizeT j = 0; j < slots.size(); ++j)
    {
        if (j != i && slots[j].error == MultiReader2InputError::None && slots[j].newestTick.has_value())
            newest = std::max(newest.value_or(*slots[j].newestTick), *slots[j].newestTick);
    }
    if (newest.has_value() &&
        (!slot.newestTick.has_value() || exceeds(*newest, *slot.newestTick, recoveryLimitTicks) ||
         exceeds(*slot.newestTick, *newest, recoveryLimitTicks)))
        return false;

    slot.queue.pushEvent({Event::Kind::Rejoined});
    return true;
}

// ================================================================ stalls

// Over the contributing inputs that sent a sample: the newest tick, the input that holds it, the newest of the others, and
// the earliest first tick. One pass, so an input's silence needs no pass over the others
struct MultiReaderDataManager::Senders
{
    std::optional<std::int64_t> newest;
    std::optional<std::int64_t> second;
    std::optional<std::int64_t> first;
    const Slot* newestSlot = nullptr;
};

MultiReaderDataManager::Senders MultiReaderDataManager::senders() const
{
    Senders found;
    for (const auto& slot : slots)
    {
        if (!contributes(slot) || !slot.newestTick.has_value())
            continue;
        if (!found.newest.has_value() || *slot.newestTick > *found.newest)
        {
            found.second = found.newest;
            found.newest = slot.newestTick;
            found.newestSlot = &slot;
        }
        else if (!found.second.has_value() || *slot.newestTick > *found.second)
            found.second = slot.newestTick;
        if (slot.firstSentTick.has_value())
            found.first = std::min(found.first.value_or(*slot.firstSentTick), *slot.firstSentTick);
    }
    return found;
}

// How far, in main ticks, the other inputs sent past a contributing input with nothing queued: past its newest sample, or past
// the senders' first sample when it sent none. nullopt for an input with anything queued
std::optional<std::int64_t> MultiReaderDataManager::silence(const Slot& slot, const Senders& found) const
{
    if (!contributes(slot) || !slot.queue.empty())
        return std::nullopt;
    const auto& others = &slot == found.newestSlot ? found.second : found.newest;
    if (!others.has_value())
        return std::nullopt;
    const auto from = slot.newestTick.has_value() ? slot.newestTick : (found.first.has_value() ? checkedSub(*found.first, mainDelta) : std::nullopt);
    if (!from.has_value())
        return std::nullopt;
    return checkedSub(*others, *from).value_or(std::numeric_limits<std::int64_t>::max());
}

// A contributing input with nothing queued while the others sent more than the sync limit past it: it holds back every read
bool MultiReaderDataManager::stallDue() const
{
    if (!valid() || !mainSlot.has_value() || !slots[*mainSlot].domain.has_value())
        return false;
    if (std::none_of(slots.begin(), slots.end(), [this](const Slot& slot) { return contributes(slot) && slot.queue.empty(); }))
        return false;

    const Senders found = senders();
    return std::any_of(slots.begin(), slots.end(), [&](const Slot& slot)
    {
        const auto ticks = silence(slot, found);
        return ticks.has_value() && *ticks > syncLimitTicks;
    });
}

// The stall check at ingest: whether a stall is due, else the tick an input must send past before one can be. A due stall
// waits for the read it calls for, which plans the check again
bool MultiReaderDataManager::checkStall()
{
    if (stallDue())
    {
        stallCheckTick = std::numeric_limits<std::int64_t>::max();
        return true;
    }
    planStallCheck();
    return false;
}

// The earliest main tick past which another input's newest sample puts an input with nothing queued more than the sync
// limit behind. Only ingest adds data, so until a removal (recountShort) or a new sender, no input stalls before it
void MultiReaderDataManager::planStallCheck()
{
    std::int64_t next = std::numeric_limits<std::int64_t>::max();
    if (valid() && mainSlot.has_value() && slots[*mainSlot].domain.has_value())
    {
        const Senders found = senders();
        for (const auto& slot : slots)
        {
            if (!contributes(slot) || !slot.queue.empty())
                continue;
            const auto from = slot.newestTick.has_value()
                                  ? slot.newestTick
                                  : (found.first.has_value() ? checkedSub(*found.first, mainDelta) : std::nullopt);
            if (from.has_value())
                next = std::min(next, checkedAdd(*from, syncLimitTicks).value_or(std::numeric_limits<std::int64_t>::max()));
        }
    }
    stallCheckTick = next;
}

// At the start of a read: the silent inputs become SyncFailed (Stalled), set aside or invalidating the reader as their error
// policy says, the main input included; they rejoin within the recovery limit (checkRejoin)
void MultiReaderDataManager::applyStalls()
{
    if (!stallDue())
        return;

    const bool wasValid = valid();
    const auto previousMain = mainSlot;
    const auto previous = errors();
    const Senders found = senders();
    std::vector<SizeT> silent;
    for (SizeT i = 0; i < slots.size(); ++i)
    {
        const auto ticks = silence(slots[i], found);
        if (ticks.has_value() && *ticks > syncLimitTicks)
            silent.push_back(i);
    }
    for (const SizeT i : silent)
        slots[i].syncFailed = SyncCause::Stalled;

    judgeInputs(true);
    afterJudgement(wasValid, previousMain, previous, false, false);
    noteChanges();
}

// Samples were dropped or the start moved: the next packetOffset does not continue the last read. Only while valid
void MultiReaderDataManager::markResynchronized()
{
    resynchronized = true;
    statusOwed = true;
}

// After a read whose status the caller never got: everything is reported anew, as after a restart
void MultiReaderDataManager::reportAfresh()
{
    for (auto& slot : slots)
        slot.descriptorChanged = slot.error == MultiReader2InputError::None;
    domainChanged = true;
    resynchronized = mainSlot.has_value();
    afresh = true;
    steadyStatus = nullptr;
    noteChanges();
}

// ================================================================ status

bool MultiReaderDataManager::Shown::operator==(const Shown& other) const
{
    return level == other.level && errors == other.errors && message == other.message && mainSlot == other.mainSlot &&
           domain.getObject() == other.domain.getObject();
}

// The state fields a status would show now. An invalid reader keeps a healthy automatic main input internally, but
// reports none; a pinned one always shows
MultiReaderDataManager::Shown MultiReaderDataManager::capture() const
{
    Shown now;
    now.level = level;
    now.errors = errors();
    for (SizeT i = 0; i < slots.size(); ++i)
    {
        if (slots[i].error == MultiReader2InputError::None)
            continue;
        if (!now.message.empty())
            now.message += "; ";
        now.message += config.inputs[i].getLocalId().toStdString() + ": " + describeError(slots[i]);
    }
    if (mainSlot.has_value() && (valid() || config.mainSlot.has_value()))
        now.mainSlot = mainSlot;
    if (mainSlot.has_value() && valid())
        now.domain = slots[*mainSlot].current.domain;
    return now;
}

// After a judgement: a status is owed when it will show something the previous one did not. The event fields count only
// on a valid reader, and a descriptor only on a healthy input
void MultiReaderDataManager::noteChanges()
{
    bool events = false;
    if (valid())
    {
        events = domainChanged || resynchronized;
        for (const auto& slot : slots)
            events = events || (slot.error == MultiReader2InputError::None && slot.descriptorChanged);
    }
    statusOwed = afresh || events || !(capture() == shown);
}

// The text of the input's entry in the status message (Status message)
std::string MultiReaderDataManager::describeError(const Slot& slot) const
{
    switch (slot.error)
    {
        case MultiReader2InputError::Disconnected:
            return "not connected";
        case MultiReader2InputError::ValueDescriptorInvalid:
            if (!slot.current.value.assigned())
                return "no value descriptor";
            if (slot.rejectedByOwner)
                return "value descriptor rejected by the owner";
            return "value type not convertible to " + convertSampleTypeToString(config.readType);
        case MultiReader2InputError::DomainDescriptorInvalid:
            return "domain descriptor invalid";
        case MultiReader2InputError::SyncFailed:
            if (slot.syncFailed == SyncCause::OffGrid)
                return "not synchronized, off the main input's grid";
            if (slot.syncFailed == SyncCause::Stalled)
                return "not synchronized, no data for 3 s";
            return "not synchronized, more than 3 s ahead of the main input";
        default:
            return "";
    }
}

// Consumes the event fields, which are never reported under Error. A status without changes equals the previous one and
// is returned again
MultiReader2StatusPtr MultiReaderDataManager::buildStatus()
{
    if (!statusOwed && steadyStatus.assigned())
    {
        domainChanged = false;
        resynchronized = false;
        return steadyStatus;
    }

    Shown now = capture();
    const bool isValid = valid();
    auto inputs = List<IMultiReader2InputStatus>();
    for (SizeT i = 0; i < slots.size(); ++i)
    {
        Slot& slot = slots[i];
        const bool reported = isValid && slot.error == MultiReader2InputError::None;
        inputs.pushBack(createWithImplementation<IMultiReader2InputStatus, MultiReader2InputStatusImpl>(
            config.inputs[i], slot.error, reported && slot.descriptorChanged, reported ? slot.current.value : nullptr,
            reported ? slot.sampleSize : 0));
        slot.descriptorChanged = false;
    }

    auto result = createWithImplementation<IMultiReader2Status, MultiReader2StatusImpl>(
        statusOwed,
        level,
        String(now.message),
        isValid && mainSlot.has_value() && domainChanged,
        now.domain,
        isValid && resynchronized,
        now.mainSlot.has_value() ? config.inputs[*now.mainSlot] : nullptr,
        inputs);

    steadyStatus = statusOwed ? nullptr : result;
    shown = std::move(now);
    statusOwed = false;
    afresh = false;
    domainChanged = false;
    resynchronized = false;
    return result;
}

// ================================================================ wake

void MultiReaderDataManager::callStarted()
{
    std::scoped_lock lock(mutex);
    arrivedDuringCall = false;
}

bool MultiReaderDataManager::callFinished(bool failed)
{
    std::scoped_lock lock(mutex);
    callPending = false;

    // Calls announce new arrivals only: the handler reads everything available, and what it leaves is not announced
    // again. So another call follows only for an arrival during this one that is still unread
    const bool again = arrivedDuringCall && triggerHolds();

    // A failed call is followed by another for an arrival during it, like any call; after it, the next arrival calls again
    // for whatever is still pending, as a trigger that fires once per cause, such as a rejoin, is not announced again
    retryCall = failed;
    if (again)
        callPending = true;
    return again;
}

void MultiReaderDataManager::callAborted()
{
    std::scoped_lock lock(mutex);
    callPending = false;
}

// An arrival with something new for the user: a call is claimed only when none is pending or running
bool MultiReaderDataManager::claim(bool owed)
{
    if (!owed)
        return false;
    arrivedDuringCall = true;
    if (callPending)
        return false;
    callPending = true;
    retryCall = false;
    return true;
}

// The next read would change something: samples, a status with changes, a stall, or an event it applies.
// The event part asks the read's own rules, so an arrival the next read does not reach calls no one
bool MultiReaderDataManager::triggerHolds() const
{
    if (statusOwed || frontAvailable() > 0 || shortRun().has_value() || stallDue())
        return true;

    return std::any_of(slots.begin(), slots.end(), [this](const Slot& slot) { return reachesEvent(slot, contributes(slot)); });
}

bool MultiReaderDataManager::inCallback() const
{
    return callbackThread.load() == std::this_thread::get_id();
}

END_NAMESPACE_OPENDAQ
