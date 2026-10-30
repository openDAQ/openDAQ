/*
 * Copyright 2022-2026 openDAQ d.o.o.
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

// Harness for the MultiReader2 suites. Each call is named after a step of the test matrix
// (reviews/0007-multi-reader-2-test-matrix.md in the knowledge base), so a test body reads like its row.

#pragma once
#include <gtest/gtest.h>
#include <opendaq/logger_sink_last_message_private_ptr.h>
#include <opendaq/multi_reader2_factory.h>
#include <opendaq/opendaq.h>

#include <condition_variable>
#include <functional>
#include <map>
#include <mutex>
#include <optional>
#include <random>
#include <set>
#include <string>
#include <vector>

namespace daq::mr2test
{

class MultiReader2Test;

// An input slot of the fixture: inK is the K-th input, local ID "inK"
struct Input
{
    int slot;
    bool operator<(const Input& other) const
    {
        return slot < other.slot;
    }
    bool operator==(const Input& other) const
    {
        return slot == other.slot;
    }
};
inline constexpr Input in1{1}, in2{2}, in3{3}, in4{4}, in5{5}, in6{6}, in7{7}, in8{8}, in9{9}, in10{10};

// A further SIG.BASE signal that no input carries, for "connect s3"
struct Spare
{
    int id;
};
inline constexpr Spare s3{3}, s4{4}, s5{5};

inline constexpr ComponentStatus Ok = ComponentStatus::Ok;
inline constexpr ComponentStatus Warning = ComponentStatus::Warning;
inline constexpr ComponentStatus Error = ComponentStatus::Error;
using InputError = MultiReader2InputError;
inline constexpr MultiReader2ErrorPolicy Invalidate = MultiReader2ErrorPolicy::Invalidate;
inline constexpr MultiReader2ErrorPolicy Exclude = MultiReader2ErrorPolicy::Exclude;

// A value descriptor as the matrix writes it after "value="
class Value
{
public:
    Value(SampleType type = SampleType::Float64);
    Value dims(std::vector<SizeT> sizes) const;
    Value unit(const std::string& symbol) const;
    static Value scaled(SampleType raw, double scale, double offset);
    static Value structure();
    static Value null();
    static Value same();
    static Value base();

    // The descriptor, or null for Value::null(); `last` resolves Value::same()
    DataDescriptorPtr build(const DataDescriptorPtr& last) const;

private:
    enum class Kind
    {
        Plain,
        Struct,
        Null,
        Same
    } kind = Kind::Plain;
    SampleType type;
    std::vector<SizeT> dimensions;
    std::string unitSymbol = "V";
    std::optional<std::pair<double, double>> scaling;
};

// A domain descriptor as the matrix writes it after "domain=": SIG.BASE's domain plus changes
class Domain
{
public:
    Domain();
    Domain originMs(int64_t ms) const;
    Domain originSeconds(int64_t seconds) const;
    Domain originEpoch() const;
    Domain noOrigin() const;
    Domain type(SampleType type, std::vector<SizeT> dims = {}) const;
    Domain unitMs() const;
    Domain explicitRule() const;
    Domain delta(int64_t delta) const;
    Domain start(double start) const;
    Domain resolution(int64_t num, int64_t den) const;
    Domain rate500Hz() const;
    static Domain null();
    static Domain same();

    DataDescriptorPtr build(const DataDescriptorPtr& last) const;

private:
    enum class Kind
    {
        Plain,
        Null,
        Same
    } kind = Kind::Plain;
    SampleType sampleType = SampleType::Int64;
    std::vector<SizeT> dimensions;
    bool unitIsMs = false;
    bool explicitDataRule = false;
    int64_t deltaTicks = 1;
    double startTick = 0;
    int64_t resNum = 1, resDen = 1000;
    std::optional<std::string> origin;
};

// A signal of the suite document's Test signals table
struct Sig
{
    Value value;
    Domain domain;
};
namespace sig
{
Sig base();
Sig int32();               // SIG.INT
Sig int64();               // SIG.INT64
Sig scaled();              // SIG.SCALED
Sig structure();           // SIG.STRUCT
Sig vector();              // SIG.VEC
Sig matrix();              // SIG.MAT
Sig nonSimplified();       // SIG.NONSIMPL
Sig fine();                // SIG.FINE
Sig start5();              // SIG.START
Sig offset1s();            // SIG.OFFSET
Sig epochNs();             // SIG.EPOCH.NS
Sig epochUs();             // SIG.EPOCH.US
Sig epochNonSimplified();  // SIG.EPOCH.NONSIMPL
Sig pico();                // SIG.PICO
Sig delta2();              // SIG.DELTA2
Sig rate500Hz();           // SIG.RATE
Sig domainFault(int n);    // SIG.DOM.2 to SIG.DOM.10
Sig noValue();             // SIG.NOVALUE
Sig noDomain();            // SIG.NODOMAIN
}

enum class Verdict
{
    Accept,
    Reject,
    Throw,
    CallReader
};

// The timelines of the suite document, written by name ("setup: TL.AHEAD"). CatchUp is the TL.CATCHUP part that follows
// TL.AHEAD or TL.BEHIND, whichever the case wrote before it.
enum class Timeline
{
    Base,
    Apart,
    Exact,
    Ahead,
    Behind,
    JumpAhead,
    JumpBack,
    Starve1,
    StarveAll,
    StarveMain,
    Short,
    Starve2s,
    CatchUp,
    Epoch
};

// The acceptsDescriptor rules of the Suite 2 owners: "accepts=unit V", "accepts=sum", "accepts=power"
enum class AcceptsRule
{
    UnitV,  // unit V
    Sum,    // a scalar numeric sample type in the unit of the first input in slot order whose value in force is scalar numeric
    Power   // in1: unit V or no unit; every other input: any
};
enum class Listener
{
    Reader,
    Other,
    Block,
    None
};
enum class ReadCall
{
    Read,
    Skip,
    ReadWithDomain
};
std::string toString(ReadCall call);
inline void PrintTo(ReadCall call, std::ostream* os)
{
    *os << toString(call);
}

// Params of a configure, construct or mutate: the params in force plus changes
class Params
{
public:
    Params add(Input input) const;
    Params addSignal(Spare spare) const;
    Params remove(Input input) const;
    Params order(std::vector<Input> inputs) const;
    Params main(Input input) const;
    Params mainNull() const;
    Params readType(SampleType type) const;
    Params minReadCount(SizeT count) const;
    Params policy(MultiReader2ErrorPolicy policy) const;
    Params newAccepts() const;
    Params newAccepts(AcceptsRule rule) const;

private:
    friend class MultiReader2Test;
    std::vector<std::function<void(MultiReader2Test&, MultiReader2ParamsPtr&)>> changes;
};
Params same();

// Variants of a reading call: "read N null in2", "read 0 null", "null-count", "null-status", "timeout 10"
struct ReadArgs
{
    bool nullData = false;
    std::optional<Input> nullBuffer;
    bool nullCount = false;
    bool nullStatus = false;
    SizeT timeoutMs = 0;
};
inline const ReadArgs nullData{true};
inline const ReadArgs nullCount{false, std::nullopt, true};
inline const ReadArgs nullStatus{false, std::nullopt, false, true};
ReadArgs nullBuffer(Input input);
ReadArgs timeout(SizeT ms);

// What one call returned; recorded so expectations can run later, as for handler calls
struct Result
{
    enum class Kind
    {
        Read,
        Skip,
        ReadWithDomain,
        Avail,
        Configure,
        Construct,
        InputStatus,
        Sender
    } kind;
    MultiReader2InputStatusPtr input;  // input-status: the status it returned
    int inputSlot = 0;                 // input-status: the slot it asked for
    std::optional<bool> unchanged;     // input-status: avail and the port getters returned the same after it
    ErrCode error = OPENDAQ_SUCCESS;
    SizeT count = 0;
    SizeT packetOffset = 0;
    MultiReader2StatusPtr status;
    MultiReader2StatusPtr previous;
    BaseObjectPtr sender;
    std::vector<std::vector<uint8_t>> buffers;  // per input of the params in force, in slot order
    std::vector<int> slots;                     // the input slot of each buffer
    std::vector<uint8_t> domainBuffer;
    ReadArgs args;
    std::vector<int> stubAsked;  // the stub's asks since the previous call step, construction included
    std::string step;
};

// An expected result in the matrix's Expectations form
class Expect
{
public:
    Expect at(int64_t tick) const;
    Expect level(ComponentStatus level) const;
    Expect noChanges() const;
    Expect hasChanges() const;
    Expect main(Input input) const;
    Expect mainNull() const;
    Expect domainChanged() const;
    Expect resync() const;
    Expect allDescChanged() const;
    Expect descChanged(Input input) const;
    Expect inError(Input input, InputError error) const;
    Expect healthy(Input input) const;
    Expect msg(const std::string& message) const;
    Expect size(Input input, SizeT bytes) const;
    Expect inputs(std::vector<Input> inputs) const;
    Expect values(Input input, std::function<double(int64_t tick)> value) const;
    Expect stubAsked(std::vector<Input> inputs) const;
    Expect stubNotAsked() const;
    Expect unchanged() const;
    Expect nullSender() const;
    Expect synchronizing(bool on = true) const;

private:
    friend Expect count(SizeT);
    friend Expect ok();
    friend Expect fails(ErrCode);
    friend Expect failsAny();
    friend class MultiReader2Test;

    std::optional<ErrCode> error;
    bool anyError = false;
    bool isUnchanged = false;
    bool senderNull = false;
    std::optional<bool> isSynchronizing;  // checked only when listed
    std::optional<SizeT> expectedCount;
    std::optional<int64_t> firstTick;
    std::optional<ComponentStatus> expectedLevel;
    std::optional<bool> changes;
    std::optional<std::optional<int>> mainSlot;
    bool domainChangedSet = false;
    bool resyncSet = false;
    bool allDesc = false;
    std::set<int> descChangedSlots;
    std::map<int, InputError> errors;
    std::optional<std::string> message;
    std::map<int, SizeT> sizes;
    std::optional<std::vector<int>> inputSlots;
    std::map<int, std::function<double(int64_t)>> valueOverrides;
    std::optional<std::vector<int>> asked;
};
Expect count(SizeT count);  // "100", "0": a reading call or avail returning that count
Expect ok();                // "ok": a configure or construct succeeding
Expect fails(ErrCode error);
Expect failsAny();  // "error": any error code (decision 1)

// "calls K: <results>": the handler calls since the previous idle
struct Calls
{
    SizeT count;
    std::vector<Expect> results;
};
Calls calls(SizeT count, std::vector<Expect> results = {});

struct HandlerLog
{
    SizeT calls = 0;
    std::vector<Result> results;
};

// What the test-local owner of Suite 2 does on each status with changes it receives
class OwnerReacts
{
public:
    OwnerReacts cachesDescriptors() const;          // keeps each input's last reported descriptor
    OwnerReacts cachesUnlessError() const;          // the same, but not from a status under Error
    OwnerReacts setsComponentStatus() const;        // its own status: the reader's level and message
    OwnerReacts warnsWhenOnlyDisconnected() const;  // Warning instead of an Error whose inputs in error are all Disconnected
    OwnerReacts removesDisconnectedPorts() const;   // in the handler: removes the port, configure without it, input-status

private:
    friend class MultiReader2Test;
    bool cache = false, cacheUnderError = true, componentStatus = false, warnDisconnected = false, removeDisconnected = false;
};

// Suite 3 oracle: the invariants a replay asserts
enum Inv : unsigned
{
    INV1 = 1u << 1,
    INV2 = 1u << 2,
    INV3 = 1u << 3,
    INV4 = 1u << 4,
    INV5 = 1u << 5,
    INV6 = 1u << 6,
    INV7 = 1u << 7,
    INV8 = 1u << 8,
    INV9 = 1u << 9,
    INV10 = 1u << 10
};

// Suite 3: the write steps of a case in timeline order. A concurrent run gives each input its own writer thread, which
// sends that input's steps in order.
struct WriteStep
{
    Input input;
    std::function<void()> send;
};
using Script = std::vector<WriteStep>;
inline constexpr int64_t NoRest = -1;  // eventScript: nothing written on the input hit after the event

// Suite 3: what one run of a case recorded (not Run, which testing::Test declares)
struct CaseRun
{
    std::vector<Result> reads;   // every reading call, handler and user thread, in order
    MultiReader2StatusPtr last;  // the last status of the run
    SizeT availAfterIdle = 0;
    ErrCode availError = OPENDAQ_SUCCESS;
    std::optional<Result> finalRead;            // the first read of the final drain
    int maxOverlap = 0;                         // the most handler calls that ran at once
    std::map<int, std::vector<double>> values;  // each input's delivered samples, taken while the run's inputs exist
};

struct TestInput;
struct TestSignal;

class MultiReader2Test : public testing::Test
{
public:
    explicit MultiReader2Test(SizeT workers = 1);
    ~MultiReader2Test() override;

protected:
    // given: fixture and params at construction
    class Given
    {
    public:
        Given& inputs(int count);
        Given& ports();
        Given& unconnected(Input input);
        Given& main(Input input);
        Given& minReadCount(SizeT count);
        Given& policy(MultiReader2ErrorPolicy policy);
        Given& readType(SampleType type);
        Given& signal(Input input, const Sig& sig);
        Given& signal(Spare spare, const Sig& sig);
        Given& value(Input input, const Value& value);
        Given& value(Spare spare, const Value& value);
        Given& domain(Input input, const Domain& domain);
        Given& valuesAsIndex();
        Given& valuesNaN();
        Given& verdict(Input input, Verdict verdict);
        Given& accepts();  // CFG.ACCEPT.2 to 6: a stub that accepts until a verdict says otherwise
        Given& accepts(AcceptsRule rule);
        Given& noScheduler();
        Given& noScheduler(Input input);
        Given& heldByReader(Input input);
        Given& heldByStreamReader(Input input);
        Given& listenerBlock(Input input, PacketReadyNotification mode = PacketReadyNotification::Scheduler);
        Given& onePerPacket();      // DATA.PACKET.1
        Given& differingPackets();  // DATA.PACKET.3: 10 on the main input, 7 on the others

    private:
        friend class MultiReader2Test;
        explicit Given(MultiReader2Test& test);
        MultiReader2Test& test;
    };
    Given given();

    // Setup: setupBase() is the base case (TL.BASE written, the first status consumed); setup(steps) replaces it.
    // setupBase(then) runs further setup steps after the base case; setupHandler(body, then) installs the handler,
    // writes TL.BASE and idles. Calls inside setup are not asserted.
    void setupBase(const std::function<void()>& then = {});
    void setupHandler(std::function<void()> body, const std::function<void()>& then = {});
    void setup(const std::function<void()>& steps);

    // Writes, each flushed as one write line unless the worker is blocked
    void write(Input input, int64_t first, int64_t last);  // main ticks first..last
    void writeEmpty(Input input);
    void announce(Input input, const Value& value);
    void announce(Input input, const Domain& domain);
    void announce(Input input, const Value& value, const Domain& domain);
    void announce(Spare spare, const Value& value);
    void gap(Input input);
    void otherEvent(Input input);
    void disconnect(Input input);
    void connect(Input input);
    void connect(Input input, Spare spare);
    void removeSignal(Input input);
    void timeline(Timeline timeline);  // M is the main input of the params in force, N every other input

    // The Suite 2 owner: ports it hands to the reader or takes back, and how it reacts to statuses
    void offerFreePort(Input input);  // an unconnected port outside the reader, with the owner's own listener
    void addPort(Input input);        // an unconnected port outside the reader
    void removePort(Input input);
    void ownerReacts(const OwnerReacts& reacts);
    void pauseRecording();  // the recorder drains with skip instead of readd
    void resumeRecording();
    void recorderDrain();
    void expectComponentStatus(ComponentStatus level, const std::string& message);

    // Reader calls
    Result read(SizeT count, const ReadArgs& args = {});
    Result skip(SizeT count, const ReadArgs& args = {});
    Result readWithDomain(SizeT count, const ReadArgs& args = {});
    Result readingCall(ReadCall call, SizeT count, const ReadArgs& args = {});
    Result avail();
    Result configure(const Params& params);
    Result construct(const Params& params);
    void mutate(const Params& params);
    Result inputStatus(Input input);
    std::vector<Result> drain(ReadCall call = ReadCall::Read);  // reads avail until a read returns 0 with no changes
    void sender();
    void release();

    // Handler, scheduler and stub
    void handler(std::function<void()> body);
    HandlerLog idle();
    void blockWorker();
    void unblockWorker();
    void verdict(Input input, Verdict verdict);

    // Expectations
    void expect(const Result& result, const Expect& expected);
    void expect(const std::function<Result()>& call, const Expect& expected);
    void expect(const HandlerLog& log, const Calls& expected);
    void expect(const std::vector<Result>& results, const std::vector<Expect>& expected);  // "drain -> r1; r2"
    void expectPort(Input input, Listener listener, PacketReadyNotification mode);
    void expectPort(Input input, Listener listener);  // "port(in3) → listener other": the method is not asserted
    void expectDestroyed();
    void expectStubSaw(ErrCode error);
    void expectLoggedError();  // exactly one error entry of the reader since the idle before the latest one
    void expectLoggedErrors(SizeT count);

    // Suite 3: a run is the reference (one worker, writes flushed in timeline order) or concurrent (one writer thread per
    // input, the workers of the fixture). replay runs both and checks the invariants.
    Script script(Timeline timeline);
    Script eventScript(Input hit, std::vector<std::function<void()>> event, int64_t restFirst = 50, int64_t restLast = 99);
    std::function<void()> sendWrite(Input input, int64_t first, int64_t last);
    std::function<void()> sendAnnounce(Input input, std::optional<Value> value, std::optional<Domain> domain);
    std::function<void()> sendGap(Input input);
    std::function<void()> sendOtherEvent(Input input);
    std::function<void()> sendDisconnect(Input input);
    std::function<void()> sendConnect(Input input);
    std::function<void()> sendRemoval(Input input);
    void runScript(const Script& script, bool concurrent);
    void startScript(const Script& script);  // one writer thread per input, started together
    bool scriptRunning();
    void joinScript();
    void waitUntilHandlerSaw(Input input, InputError error);
    bool waitForHandler(const std::function<bool()>& condition);  // false after 5 s, or at once when no handler is subscribed
    bool handlerRunning();
    void holdNextHandlerCall();     // the next handler call parks before its body until releaseHandlerCall
    void waitForHeldHandlerCall();  // until that call is parked; a failure after 5 s
    void releaseHandlerCall();
    Script eachInputWrites(int64_t first, int64_t last);  // every input in force, in slot order
    int64_t random(int64_t low, int64_t high);            // uniform; the seed is the test property "seed", MR2_SEED replays it
    void pause(int64_t ms);
    void runConcurrently(const std::function<void(bool concurrent)>& body, unsigned invariants);  // no reference run
    void replay(Timeline timeline, unsigned invariants);
    void replay(const std::function<Script()>& script, unsigned invariants);
    void replay(const std::function<void(bool concurrent)>& body, unsigned invariants);
    void startEndlessWriters();  // every input writes packets of 10 without end, one thread each
    void stopEndlessWriters();
    SizeT handlerCallsStarted();
    void expectReleasedCleanly(SizeT callsAtRelease, SizeT inFlight);  // INV6: at most callsAtRelease + inFlight calls started

    void TearDown() override;

    MultiReader2Ptr& reader();
    TestInput& input(int slot);
    TestSignal& spare(int id);
    ComponentPtr slotComponent(int slot);
    SignalConfigPtr signalOf(Input input);  // the signal the input carries now
    SchedulerPtr scheduler;
    ContextPtr context;

private:
    friend class Params;
    struct Config;
    struct Handler;
    struct Stub;
    struct Snapshot
    {
        ErrCode availError = OPENDAQ_SUCCESS;
        SizeT avail = 0;
        std::map<int, std::pair<Listener, PacketReadyNotification>> ports;
        bool operator==(const Snapshot& other) const;  // ports compared where both have them: a call may create inputs
    };

    void ensureContext();
    std::unique_ptr<TestSignal> createSignal(const ContextPtr& ctx, const std::string& id, const Sig& sig);
    MultiReader2ParamsPtr buildParams(const Params& changes);
    MultiReader2ParamsPtr initialParams();
    FunctionPtr newStubFunction();
    void writeLine(const std::function<void()>& send);
    void writePackets(Input input, int64_t first, int64_t last);
    void sendEvent(TestSignal& signal, const std::optional<DataDescriptorPtr>& value, const std::optional<DataDescriptorPtr>& domain);
    void ownerOnStatus(const MultiReader2StatusPtr& status);
    void ownerActs(const MultiReader2StatusPtr& status);
    bool acceptsByRule(int slot, const DataDescriptorPtr& descriptor, const ListPtr<IDataDescriptor>& values);
    CaseRun runCase(SizeT runWorkers, bool concurrent, const std::function<void(bool concurrent)>& body);
    void restart(SizeT runWorkers);
    void checkReplay(const CaseRun& reference, const CaseRun& run, unsigned invariants);
    std::map<int, std::vector<double>> delivered(const std::vector<Result>& reads, SampleType readType, bool checkInv1, bool checkInv10);
    Result runRead(Result::Kind kind, SizeT count, const ReadArgs& args);
    Result record(Result result);
    std::vector<int> takeStubAsked();
    void onDataAvailable(BaseObjectPtr& sender, EventArgsPtr<>& args);
    void checkInvariants(const Result& result);
    void checkResult(const Result& result, const Expect& expected);
    void checkStatus(const Result& result, const Expect& expected);
    void checkBuffers(const Result& result, const Expect& expected);
    int slotOf(const ComponentPtr& component) const;
    std::vector<int> slotsInForce() const;
    std::pair<Listener, PacketReadyNotification> portState(int slot);
    void noteReaderListener();
    Snapshot snapshot();
    void holdWorker();
    void releaseWorker();
    void flush();

    std::unique_ptr<Config> config;
    std::unique_ptr<Handler> handlerState;
    std::unique_ptr<Stub> stub;
    SizeT workers;
    LoggerPtr logger;
    LoggerSinkPtr errorSink;  // counts the reader's error entries
    ContextPtr contextWithoutScheduler;
    std::map<int, std::unique_ptr<TestInput>> inputs;
    std::map<int, std::unique_ptr<TestSignal>> spares;
    bool constructed = false;
    MultiReader2Ptr readerObject;
    WeakRefPtr<IMultiReader2> readerRef;
    BaseObjectPtr readerListener;  // the listener the reader sets on its ports, so "listener reader" means this reader
    std::mutex stateMutex;         // paramsInForce, paramsObject, previousStatus and lastResult: the handler and test threads share them
    MultiReader2ParamsPtr paramsInForce;
    MultiReader2ParamsPtr paramsObject;
    std::vector<MultiReader2Ptr> otherReaders;
    std::vector<BaseObjectPtr> holders;
    MultiReader2StatusPtr previousStatus;
    Result lastResult{Result::Kind::Avail};
    bool muted = false;
    int expectations = 0;
    bool workerBlocked = false;
    std::mutex workerMutex;
    std::condition_variable workerCv;
    bool workerHeld = false;
    bool workerRunning = false;
    std::vector<ErrCode> stubSaw;

    struct Owner;
    std::unique_ptr<Owner> owner;
    struct Writers;
    std::unique_ptr<Writers> writers;
    bool statusInvariants = true;  // suites 1 and 2 only
    SizeT fixtureWorkers;
    SizeT errorsAtIdle = 0;
    SizeT errorsBeforeIdle = 0;
    Timeline scripted = Timeline::Base;
    bool referenceDelivers = true;  // INV9: false on timelines that align nothing readable
    std::mutex runMutex;
    std::vector<Result> runLog;  // Suite 3: every reading call of the current run, any thread
    std::atomic<bool> recordingRun{false};
    std::optional<std::mt19937_64> rng;
    std::atomic<uint64_t> recordedSeed{0};  // the seed random() drew from, 0 before; the INV7 watchdog prints it
};

// A row whose Values name OP.READ.*: runs once per reading call
class EachReadingCall : public MultiReader2Test, public testing::WithParamInterface<ReadCall>
{
protected:
    Result reading(SizeT count, const ReadArgs& args = {});
};

// A step with a buffer variant ("read* 100 null in2") leaves skipSamples, which takes no buffers, out of the row's runs
#define MR2_SKIP_CALL_WITHOUT_BUFFERS()             \
    if (GetParam() == daq::mr2test::ReadCall::Skip) \
    GTEST_SKIP() << "skipSamples takes no buffers"

std::string readCallName(const testing::TestParamInfo<ReadCall>& info);

#define MR2_INSTANTIATE_EACH_READING_CALL(Fixture)                                                                           \
    INSTANTIATE_TEST_SUITE_P(                                                                                                \
        MultiReader2,                                                                                                        \
        Fixture,                                                                                                             \
        testing::Values(daq::mr2test::ReadCall::Read, daq::mr2test::ReadCall::Skip, daq::mr2test::ReadCall::ReadWithDomain), \
        daq::mr2test::readCallName)

}
