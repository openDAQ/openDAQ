#include "multi_reader2_harness.h"

#include <opendaq/event_packet_utils.h>
#include <opendaq/logger_sink_base_private.h>
#include <opendaq/signal_impl.h>
#include <opendaq/work_factory.h>
#include <spdlog/details/log_msg.h>

#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <future>
#include <limits>
#include <random>
#include <sstream>
#include <thread>

namespace daq::mr2test
{

namespace
{

constexpr uint8_t Sentinel = 0xA5;
constexpr SizeT BytesPerSample = 128;  // larger than any sample in the suites (Struct[2x3] takes 72)
constexpr int64_t T0 = 1767225600;     // 2026-01-01T00:00:00Z
constexpr int64_t PsPerSecond = 1000000000000;

// A signal whose descriptors the harness sets without sending an event, so every event is the one a step names
class HarnessSignalImpl : public SignalImpl
{
public:
    using SignalImpl::SignalImpl;

    void setDescriptorSilently(const DataDescriptorPtr& descriptor)
    {
        dataDescriptor = descriptor;
    }
};

// A port listener that is neither the reader nor another reader: "listener=block"
class BlockListenerImpl : public ImplementationOfWeak<IInputPortNotifications>  // the port holds its listener weakly
{
public:
    ErrCode INTERFACE_FUNC acceptsSignal(IInputPort*, ISignal*, Bool* accept) override
    {
        *accept = True;
        return OPENDAQ_SUCCESS;
    }
    ErrCode INTERFACE_FUNC connected(IInputPort*) override
    {
        return OPENDAQ_SUCCESS;
    }
    ErrCode INTERFACE_FUNC disconnected(IInputPort*) override
    {
        return OPENDAQ_SUCCESS;
    }
    ErrCode INTERFACE_FUNC packetReceived(IInputPort*) override
    {
        return OPENDAQ_SUCCESS;
    }
};

std::string isoOrigin(int64_t secondsFromT0, int64_t ms = 0)
{
    const std::time_t t = static_cast<std::time_t>(T0 + secondsFromT0);
    std::tm tm{};
#if defined(_WIN32)
    gmtime_s(&tm, &t);
#else
    gmtime_r(&t, &tm);
#endif
    char buf[64];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &tm);
    std::string s = buf;
    if (ms != 0)
    {
        char frac[8];
        std::snprintf(frac, sizeof(frac), ".%03d", static_cast<int>(ms));
        s += frac;
    }
    return s + "Z";
}

// Full-second ISO 8601 origin to seconds since the Unix epoch; nullopt for anything else
std::optional<int64_t> parseOrigin(const std::string& origin)
{
    int y, mo, d, h, mi, s;
    char z = 0;
    if (std::sscanf(origin.c_str(), "%d-%d-%dT%d:%d:%d%c", &y, &mo, &d, &h, &mi, &s, &z) != 7 || z != 'Z')
        return std::nullopt;
    // days from civil, Howard Hinnant's algorithm
    y -= mo <= 2;
    const int64_t era = (y >= 0 ? y : y - 399) / 400;
    const int64_t yoe = y - era * 400;
    const int64_t doy = (153 * (mo + (mo > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    const int64_t doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    const int64_t days = era * 146097 + doe - 719468;
    return days * 86400 + h * 3600 + mi * 60 + s;
}

// A domain the harness can convert ticks through: int64_t, scalar, linear with integer delta and start, full-second origin
struct TickDomain
{
    int64_t originSeconds = T0;
    int64_t resNum = 1;
    int64_t resDen = 1000;
    int64_t delta = 1;
    int64_t start = 0;

    static std::optional<TickDomain> from(const DataDescriptorPtr& desc)
    {
        if (!desc.assigned() || desc.getSampleType() != SampleType::Int64 || desc.getDimensions().getCount() != 0)
            return std::nullopt;
        const auto rule = desc.getRule();
        if (!rule.assigned() || rule.getType() != DataRuleType::Linear)
            return std::nullopt;
        const auto res = desc.getTickResolution();
        const auto origin = desc.getOrigin();
        if (!res.assigned() || !origin.assigned() || !desc.getUnit().assigned() || desc.getUnit().getSymbol() != "s")
            return std::nullopt;
        const auto seconds = parseOrigin(origin.toStdString());
        const NumberPtr deltaNum = rule.getParameters().get("delta");
        const NumberPtr startNum = rule.getParameters().get("start");
        if (!seconds || deltaNum.getFloatValue() != std::floor(deltaNum.getFloatValue()) || deltaNum.getIntValue() <= 0 ||
            startNum.getFloatValue() != std::floor(startNum.getFloatValue()))
            return std::nullopt;
        TickDomain d{*seconds, res.getNumerator(), res.getDenominator(), deltaNum.getIntValue(), startNum.getIntValue()};
        if (PsPerSecond % d.resDen != 0)
            return std::nullopt;
        return d;
    }

    int64_t resolutionPs() const
    {
        return resNum * (PsPerSecond / resDen);
    }

    // Time of a tick as whole seconds since the epoch plus picoseconds
    std::pair<int64_t, int64_t> timeOf(int64_t tick) const
    {
        int64_t q = tick / resDen;
        int64_t r = tick % resDen;
        if (r < 0)
        {
            r += resDen;
            q -= 1;
        }
        int64_t seconds = q * resNum + (r * resNum) / resDen;
        int64_t ps = ((r * resNum) % resDen) * (PsPerSecond / resDen);
        return {seconds + originSeconds, ps};
    }

    // The tick at or before a time
    int64_t tickAt(std::pair<int64_t, int64_t> time) const
    {
        const int64_t resPs = resolutionPs();
        const int64_t rel = time.first - originSeconds;
        int64_t ps = time.second;
        int64_t ticks = rel * (PsPerSecond / resPs);
        ticks += ps / resPs;
        return ticks;
    }
};

TickDomain baseTickDomain()
{
    return TickDomain{};
}

SizeT elementCount(const DataDescriptorPtr& desc)
{
    SizeT n = 1;
    for (const auto& dim : desc.getDimensions())
        n *= dim.getSize();
    return n;
}

SizeT typeSize(SampleType type)
{
    return getSampleSize(type);
}

void writeElement(uint8_t* dst, SampleType type, double value)
{
    switch (type)
    {
        case SampleType::Float32:
        {
            auto v = static_cast<float>(value);
            std::memcpy(dst, &v, sizeof v);
            break;
        }
        case SampleType::Float64:
        {
            std::memcpy(dst, &value, sizeof value);
            break;
        }
        case SampleType::Int8:
        {
            auto v = static_cast<int8_t>(value);
            std::memcpy(dst, &v, sizeof v);
            break;
        }
        case SampleType::UInt8:
        {
            auto v = static_cast<uint8_t>(value);
            std::memcpy(dst, &v, sizeof v);
            break;
        }
        case SampleType::Int16:
        {
            auto v = static_cast<int16_t>(value);
            std::memcpy(dst, &v, sizeof v);
            break;
        }
        case SampleType::UInt16:
        {
            auto v = static_cast<uint16_t>(value);
            std::memcpy(dst, &v, sizeof v);
            break;
        }
        case SampleType::Int32:
        {
            auto v = static_cast<int32_t>(value);
            std::memcpy(dst, &v, sizeof v);
            break;
        }
        case SampleType::UInt32:
        {
            auto v = static_cast<uint32_t>(value);
            std::memcpy(dst, &v, sizeof v);
            break;
        }
        case SampleType::Int64:
        {
            auto v = static_cast<int64_t>(value);
            std::memcpy(dst, &v, sizeof v);
            break;
        }
        case SampleType::UInt64:
        {
            auto v = static_cast<uint64_t>(value);
            std::memcpy(dst, &v, sizeof v);
            break;
        }
        default:
            throw std::runtime_error("harness: unsupported element type");
    }
}

double readElement(const uint8_t* src, SampleType type)
{
    switch (type)
    {
        case SampleType::Float32:
        {
            float v;
            std::memcpy(&v, src, sizeof v);
            return v;
        }
        case SampleType::Float64:
        {
            double v;
            std::memcpy(&v, src, sizeof v);
            return v;
        }
        case SampleType::Int8:
        {
            int8_t v;
            std::memcpy(&v, src, sizeof v);
            return v;
        }
        case SampleType::UInt8:
        {
            uint8_t v;
            std::memcpy(&v, src, sizeof v);
            return v;
        }
        case SampleType::Int16:
        {
            int16_t v;
            std::memcpy(&v, src, sizeof v);
            return v;
        }
        case SampleType::UInt16:
        {
            uint16_t v;
            std::memcpy(&v, src, sizeof v);
            return v;
        }
        case SampleType::Int32:
        {
            int32_t v;
            std::memcpy(&v, src, sizeof v);
            return v;
        }
        case SampleType::UInt32:
        {
            uint32_t v;
            std::memcpy(&v, src, sizeof v);
            return v;
        }
        case SampleType::Int64:
        {
            int64_t v;
            std::memcpy(&v, src, sizeof v);
            return static_cast<double>(v);
        }
        case SampleType::UInt64:
        {
            uint64_t v;
            std::memcpy(&v, src, sizeof v);
            return static_cast<double>(v);
        }
        default:
            throw std::runtime_error("harness: unsupported element type");
    }
}

SampleType rawSampleType(const DataDescriptorPtr& desc)
{
    return desc.getPostScaling().assigned() ? desc.getPostScaling().getInputSampleType() : desc.getSampleType();
}

// Writes one sample's value into every element (and every struct field) of a raw sample
void fillSample(uint8_t* dst, const DataDescriptorPtr& desc, double value)
{
    const SizeT elements = elementCount(desc);
    if (desc.getSampleType() == SampleType::Struct)
    {
        for (SizeT e = 0; e < elements; ++e)
            for (const auto& field : desc.getStructFields())
            {
                writeElement(dst, field.getSampleType(), value);
                dst += typeSize(field.getSampleType());
            }
        return;
    }
    const auto raw = rawSampleType(desc);
    for (SizeT e = 0; e < elements; ++e)
        writeElement(dst + e * typeSize(raw), raw, value);
}

bool sameValue(double a, double b)
{
    return (std::isnan(a) && std::isnan(b)) || a == b;
}

MultiReader2ParamsPtr copyParams(const MultiReader2ParamsPtr& from)
{
    auto params = MultiReader2Params();
    auto inputs = List<IComponent>();
    for (const auto& input : from.getInputs())
        inputs.pushBack(input);
    params.setInputs(inputs);
    params.setMainInput(from.getMainInput());
    params.setAcceptsDescriptor(from.getAcceptsDescriptor());
    params.setValueReadType(from.getValueReadType());
    params.setMinReadCount(from.getMinReadCount());
    params.setErrorPolicy(from.getErrorPolicy());
    return params;
}

std::string errorName(ErrCode error)
{
    switch (error)
    {
        case OPENDAQ_SUCCESS:
            return "SUCCESS";
        case OPENDAQ_ERR_NOTIMPLEMENTED:
            return "NOTIMPLEMENTED";
        case OPENDAQ_ERR_INVALIDPARAMETER:
            return "INVALIDPARAMETER";
        case OPENDAQ_ERR_ARGUMENT_NULL:
            return "ARGUMENT_NULL";
        case OPENDAQ_ERR_INVALIDSTATE:
            return "INVALIDSTATE";
        case OPENDAQ_ERR_ALREADYEXISTS:
            return "ALREADYEXISTS";
        default:
            break;
    }
    std::ostringstream s;
    s << "0x" << std::hex << error;
    return s.str();
}

// An spdlog sink that counts error entries, since openDAQ has no counting sink. Only the sink's pure virtuals are
// implemented: spdlog is compiled into the openDAQ library, and its non-virtual members are not linked here.
class ErrorCountingSink : public spdlog::sinks::sink
{
public:
    void log(const spdlog::details::log_msg& msg) override
    {
        // Only the reader's entries: an error of a port or a signal must not stand in for the one a row expects
        if (msg.level < spdlog::level::err || msg.logger_name != spdlog::string_view_t("MultiReader2"))
            return;
        {
            std::scoped_lock lock(mutex);
            ++errors;
        }
        cv.notify_all();
    }
    void flush() override
    {
    }
    void set_pattern(const std::string&) override
    {
    }
    void set_formatter(std::unique_ptr<spdlog::formatter>) override
    {
    }

    SizeT count()
    {
        std::scoped_lock lock(mutex);
        return errors;
    }

    SizeT waitForAtLeast(SizeT n)
    {
        std::unique_lock lock(mutex);
        cv.wait_for(lock, std::chrono::seconds(2), [&] { return errors >= n; });
        return errors;
    }

private:
    std::mutex mutex;
    std::condition_variable cv;
    SizeT errors = 0;
};

class ErrorCountingSinkImpl : public ImplementationOf<ILoggerSink, ILoggerSinkBasePrivate>
{
public:
    std::shared_ptr<ErrorCountingSink> sink = std::make_shared<ErrorCountingSink>();

    ErrCode INTERFACE_FUNC setLevel(LogLevel) override
    {
        return OPENDAQ_SUCCESS;
    }
    ErrCode INTERFACE_FUNC getLevel(LogLevel* level) override
    {
        *level = LogLevel::Error;
        return OPENDAQ_SUCCESS;
    }
    ErrCode INTERFACE_FUNC shouldLog(LogLevel level, Bool* willLog) override
    {
        *willLog = level >= LogLevel::Error;
        return OPENDAQ_SUCCESS;
    }
    ErrCode INTERFACE_FUNC setPattern(IString*) override
    {
        return OPENDAQ_SUCCESS;
    }
    ErrCode INTERFACE_FUNC flush() override
    {
        return OPENDAQ_SUCCESS;
    }
    ErrCode INTERFACE_FUNC getSinkImpl(SinkPtr* sinkImpl) override
    {
        *sinkImpl = sink;
        return OPENDAQ_SUCCESS;
    }
};

ErrorCountingSink& countingSink(const LoggerSinkPtr& sink)
{
    return *static_cast<ErrorCountingSinkImpl*>(sink.getObject())->sink;
}

// The one-worker scheduler of suites 1 and 2: work runs in the order it was scheduled. The openDAQ scheduler runs work
// scheduled from its worker before older work, so a handler call an ingest triggers could overtake queued packets
class FifoSchedulerImpl : public ImplementationOf<IScheduler>
{
public:
    explicit FifoSchedulerImpl(SchedulerPtr inner)
        : inner(std::move(inner))
    {
    }

    ErrCode INTERFACE_FUNC scheduleFunction(IFunction* function, IAwaitable** awaitable) override
    {
        if (awaitable)
            *awaitable = nullptr;
        const FunctionPtr f = function;
        return push(Work([f] { f.call(); }));
    }
    ErrCode INTERFACE_FUNC scheduleWork(IWork* work) override
    {
        return push(work);
    }
    ErrCode INTERFACE_FUNC scheduleGraph(ITaskGraph* graph, IAwaitable** awaitable) override
    {
        return inner->scheduleGraph(graph, awaitable);
    }
    ErrCode INTERFACE_FUNC stop() override
    {
        return inner->stop();
    }
    ErrCode INTERFACE_FUNC waitAll() override
    {
        return inner->waitAll();
    }
    ErrCode INTERFACE_FUNC isMultiThreaded(Bool* multiThreaded) override
    {
        return inner->isMultiThreaded(multiThreaded);
    }
    ErrCode INTERFACE_FUNC runMainLoop(SizeT loopTime) override
    {
        return inner->runMainLoop(loopTime);
    }
    ErrCode INTERFACE_FUNC isMainLoopSet(Bool* isSet) override
    {
        return inner->isMainLoopSet(isSet);
    }
    ErrCode INTERFACE_FUNC stopMainLoop() override
    {
        return inner->stopMainLoop();
    }
    ErrCode INTERFACE_FUNC runMainLoopIteration() override
    {
        return inner->runMainLoopIteration();
    }
    ErrCode INTERFACE_FUNC scheduleWorkOnMainLoop(IWork* work) override
    {
        return inner->scheduleWorkOnMainLoop(work);
    }

private:
    ErrCode push(const WorkPtr& work)
    {
        std::scoped_lock lock(mutex);
        queue.push_back(work);
        if (draining)
            return OPENDAQ_SUCCESS;
        draining = true;
        return inner->scheduleWork(Work([this] { drain(); }));
    }

    void drain()
    {
        for (;;)
        {
            WorkPtr work;
            {
                std::scoped_lock lock(mutex);
                if (queue.empty())
                {
                    draining = false;
                    return;
                }
                work = queue.front();
                queue.pop_front();
            }
            work->execute();
        }
    }

    SchedulerPtr inner;
    std::mutex mutex;
    std::deque<WorkPtr> queue;
    bool draining = false;
};

// INV7: a run that has not finished within 10 s is a deadlock; the process exits rather than hang the suite, naming the
// test and the seed so the run can be replayed (MR2_SEED)
class Watchdog
{
public:
    explicit Watchdog(const std::atomic<uint64_t>& seed)
        : thread(
              [this, &seed]
              {
                  std::unique_lock lock(mutex);
                  if (!cv.wait_for(lock, std::chrono::seconds(10), [this] { return done; }))
                  {
                      const auto* info = testing::UnitTest::GetInstance()->current_test_info();
                      std::fprintf(stderr,
                                   "INV7: %s.%s did not finish within 10 s (seed %llu)\n",
                                   info ? info->test_suite_name() : "?",
                                   info ? info->name() : "?",
                                   static_cast<unsigned long long>(seed.load()));
                      std::fflush(stderr);
                      std::_Exit(3);
                  }
              })
    {
    }
    ~Watchdog()
    {
        {
            std::scoped_lock lock(mutex);
            done = true;
        }
        cv.notify_all();
        thread.join();
    }

private:
    std::mutex mutex;
    std::condition_variable cv;
    bool done = false;
    std::thread thread;
};

bool isScalarNumeric(const DataDescriptorPtr& desc)
{
    if (!desc.assigned() || desc.getDimensions().getCount() != 0)
        return false;
    const auto type = desc.getSampleType();
    return type >= SampleType::Float32 && type <= SampleType::Int64;
}

std::string unitOf(const DataDescriptorPtr& desc)
{
    return desc.assigned() && desc.getUnit().assigned() ? desc.getUnit().getSymbol().toStdString() : "";
}

thread_local bool inHandler = false;
thread_local BaseObjectPtr handlerSender;
thread_local MultiReader2Ptr handlerReader;

}

// ---------------------------------------------------------------------------------------------------------------------
// Descriptors and signals

Value::Value(SampleType type)
    : type(type)
{
}

Value Value::dims(std::vector<SizeT> sizes) const
{
    Value v = *this;
    v.dimensions = std::move(sizes);
    return v;
}

Value Value::unit(const std::string& symbol) const
{
    Value v = *this;
    v.unitSymbol = symbol;
    return v;
}

Value Value::scaled(SampleType raw, double scale, double offset)
{
    Value v(raw);
    v.scaling = std::make_pair(scale, offset);
    return v;
}

Value Value::structure()
{
    Value v(SampleType::Struct);
    v.kind = Kind::Struct;
    return v;
}

Value Value::null()
{
    Value v;
    v.kind = Kind::Null;
    return v;
}

Value Value::same()
{
    Value v;
    v.kind = Kind::Same;
    return v;
}

Value Value::base()
{
    return Value(SampleType::Float64);
}

DataDescriptorPtr Value::build(const DataDescriptorPtr& last) const
{
    if (kind == Kind::Null)
        return nullptr;
    if (kind == Kind::Same)
        return last;

    auto builder = DataDescriptorBuilder().setName("value");
    if (!unitSymbol.empty() && kind != Kind::Struct)  // openDAQ rejects a unit on a struct descriptor
        builder.setUnit(Unit(unitSymbol));
    if (kind == Kind::Struct)
    {
        builder.setSampleType(SampleType::Struct)
            .setName("MR2TestStruct")
            .setStructFields(List<IDataDescriptor>(DataDescriptorBuilder().setName("a").setSampleType(SampleType::Int32).build(),
                                                   DataDescriptorBuilder().setName("b").setSampleType(SampleType::Float64).build()));
    }
    else if (scaling)
    {
        builder.setSampleType(SampleType::Float64)
            .setPostScaling(LinearScaling(scaling->first, scaling->second, type, ScaledSampleType::Float64));
    }
    else
    {
        builder.setSampleType(type);
    }

    if (!dimensions.empty())
    {
        auto dims = List<IDimension>();
        for (SizeT size : dimensions)
            dims.pushBack(Dimension(LinearDimensionRule(1, 0, size)));
        builder.setDimensions(dims);
    }
    return builder.build();
}

Domain::Domain() = default;

#define MR2_DOMAIN_SETTER(Name, Args, Body) \
    Domain Domain::Name Args const          \
    {                                       \
        Domain d = *this;                   \
        Body;                               \
        return d;                           \
    }

MR2_DOMAIN_SETTER(originMs, (int64_t ms), d.origin = isoOrigin(ms / 1000, ms % 1000))
MR2_DOMAIN_SETTER(originSeconds, (int64_t seconds), d.origin = isoOrigin(seconds))
MR2_DOMAIN_SETTER(originEpoch, (), d.origin = "1970-01-01T00:00:00Z")
MR2_DOMAIN_SETTER(noOrigin, (), d.origin = std::string())
MR2_DOMAIN_SETTER(type, (SampleType t, std::vector<SizeT> dims), d.sampleType = t; d.dimensions = std::move(dims))
MR2_DOMAIN_SETTER(unitMs, (), d.unitIsMs = true)
MR2_DOMAIN_SETTER(explicitRule, (), d.explicitDataRule = true)
MR2_DOMAIN_SETTER(delta, (int64_t v), d.deltaTicks = v)
MR2_DOMAIN_SETTER(start, (double v), d.startTick = v)
MR2_DOMAIN_SETTER(resolution, (int64_t num, int64_t den), d.resNum = num; d.resDen = den)
MR2_DOMAIN_SETTER(rate500Hz, (), d.deltaTicks = 2)

#undef MR2_DOMAIN_SETTER

Domain Domain::null()
{
    Domain d;
    d.kind = Kind::Null;
    return d;
}

Domain Domain::same()
{
    Domain d;
    d.kind = Kind::Same;
    return d;
}

DataDescriptorPtr Domain::build(const DataDescriptorPtr& last) const
{
    if (kind == Kind::Null)
        return nullptr;
    if (kind == Kind::Same)
        return last;

    auto builder = DataDescriptorBuilder()
                       .setName("time")
                       .setSampleType(sampleType)
                       .setTickResolution(Ratio(resNum, resDen))
                       .setUnit(unitIsMs ? Unit("ms", -1, "milliseconds", "time") : Unit("s", -1, "seconds", "time"));
    if (explicitDataRule)
        builder.setRule(ExplicitDataRule());
    else if (startTick == std::floor(startTick))
        builder.setRule(LinearDataRule(deltaTicks, static_cast<int64_t>(startTick)));
    else
        builder.setRule(LinearDataRule(deltaTicks, startTick));
    const std::string o = origin.value_or(isoOrigin(0));
    if (!o.empty())
        builder.setOrigin(o);
    if (!dimensions.empty())
    {
        auto dims = List<IDimension>();
        for (SizeT size : dimensions)
            dims.pushBack(Dimension(LinearDimensionRule(1, 0, size)));
        builder.setDimensions(dims);
    }
    return builder.build();
}

namespace sig
{
Sig base()
{
    return {Value::base(), Domain()};
}
Sig int32()
{
    return {Value(SampleType::Int32), Domain()};
}
Sig int64()
{
    return {Value(SampleType::Int64), Domain()};
}
Sig scaled()
{
    return {Value::scaled(SampleType::Int32, 0.5, 1), Domain()};
}
Sig structure()
{
    return {Value::structure(), Domain()};
}
Sig vector()
{
    return {Value().dims({4}), Domain()};
}
Sig matrix()
{
    return {Value().dims({2, 3}), Domain()};
}
Sig nonSimplified()
{
    return {Value(), Domain().resolution(2, 2000)};
}
Sig fine()
{
    return {Value(), Domain().resolution(1, 2000).delta(2)};
}
Sig start5()
{
    return {Value(), Domain().start(5)};
}
Sig offset1s()
{
    return {Value(), Domain().originSeconds(1)};
}
Sig epochNs()
{
    return {Value(), Domain().resolution(1, 1000000000).delta(1000000).originEpoch()};
}
Sig epochUs()
{
    return {Value(), Domain().resolution(1, 1000000).delta(1000).originEpoch()};
}
Sig epochNonSimplified()
{
    return {Value(), Domain().resolution(1000, 1000000000000).delta(1000000).originEpoch()};
}
Sig pico()
{
    return {Value(), Domain().resolution(1, 1000000000000).delta(1000000000)};
}
Sig delta2()
{
    return {Value(), Domain().delta(2)};
}
Sig rate500Hz()
{
    return {Value(), Domain().rate500Hz()};
}
Sig domainFault(int n)
{
    switch (n)
    {
        case 2:
            return {Value(), Domain().noOrigin()};
        case 3:
            return {Value(), Domain().originMs(500)};
        case 5:
            return {Value(), Domain().type(SampleType::Float64)};
        case 6:
            return {Value(), Domain().type(SampleType::Int64, {2})};
        case 7:
            return {Value(), Domain().unitMs()};
        case 8:
            return {Value(), Domain().explicitRule()};
        case 9:
            return {Value(), Domain().delta(0)};
        case 10:
            return {Value(), Domain().start(1.5)};
        default:
            throw std::invalid_argument("harness: no SIG.DOM." + std::to_string(n));
    }
}
Sig noValue()
{
    return {Value::null(), Domain()};
}
Sig noDomain()
{
    return {Value(), Domain::null()};
}
}

std::string toString(ReadCall call)
{
    switch (call)
    {
        case ReadCall::Read:
            return "Read";
        case ReadCall::Skip:
            return "SkipSamples";
        case ReadCall::ReadWithDomain:
            return "ReadWithDomain";
    }
    return "?";
}

std::string readCallName(const testing::TestParamInfo<ReadCall>& info)
{
    return toString(info.param);
}

ReadArgs nullBuffer(Input input)
{
    ReadArgs args;
    args.nullBuffer = input;
    return args;
}

ReadArgs timeout(SizeT ms)
{
    ReadArgs args;
    args.timeoutMs = ms;
    return args;
}

// ---------------------------------------------------------------------------------------------------------------------
// Fixture state

struct TestSignal
{
    std::string id;
    SignalConfigPtr value;
    SignalConfigPtr domain;
    HarnessSignalImpl* valueImpl = nullptr;
    HarnessSignalImpl* domainImpl = nullptr;
    DataDescriptorPtr valueDesc;
    DataDescriptorPtr domainDesc;
    std::vector<DataDescriptorPtr> valueHistory;
    std::vector<DataDescriptorPtr> domainHistory;
    int64_t nextIndex = 0;
    std::map<int64_t, double> written;  // reference main tick -> raw value
    bool removed = false;
};

struct TestInput
{
    int slot = 0;
    std::string id;
    ContextPtr context;
    std::unique_ptr<TestSignal> own;
    TestSignal* signal = nullptr;  // the connected signal; null when unconnected
    InputPortConfigPtr port;       // input-port kinds only
    BaseObjectPtr otherListener;
    BaseObjectPtr blockListener;
    std::map<int64_t, double> written;  // reference main tick -> raw value, whichever signal wrote it
};

struct MultiReader2Test::Config
{
    int count = 2;
    bool ports = false;
    std::set<int> unconnected;
    std::optional<int> main;
    SizeT minReadCount = 1;
    MultiReader2ErrorPolicy policy = Invalidate;
    SampleType readType = SampleType::Float64;
    std::map<int, Sig> signals;
    std::map<int, Sig> spareSignals;
    enum class Values
    {
        Tick,
        Index,
        NaN
    } values = Values::Tick;
    bool stub = false;
    bool noScheduler = false;
    std::set<int> noSchedulerInputs;
    std::set<int> heldByReader;
    std::set<int> heldByStream;
    std::map<int, PacketReadyNotification> blockListener;
    int packets = 2;
    TickDomain reference;
};

struct MultiReader2Test::Handler
{
    std::function<void()> body;
    bool installed = false;
    std::mutex mutex;
    HandlerLog log;
    std::atomic<int> running{0};
    std::atomic<int> maxRunning{0};
    std::atomic<SizeT> started{0};
    std::atomic<SizeT> startedWithoutReader{0};  // calls that found the reader destroyed
    bool subscribed = false;
    std::atomic<bool> holdNext{false};
    bool held = false;  // under mutex: a call is parked
    std::condition_variable heldCv;
};

struct MultiReader2Test::Owner
{
    std::mutex mutex;
    OwnerReacts reacts;
    std::optional<AcceptsRule> rule;
    std::map<int, DataDescriptorPtr> cache;
    ComponentStatus status = ComponentStatus::Ok;
    std::string message;
    bool paused = false;
    bool acting = false;
};

struct MultiReader2Test::Writers
{
    std::vector<std::thread> threads;
    std::atomic<bool> stop{false};
    std::atomic<int> running{0};
};

struct MultiReader2Test::Stub
{
    std::mutex mutex;
    std::map<int, Verdict> verdicts;
    std::vector<int> asked;
    FunctionPtr function;
};

// ---------------------------------------------------------------------------------------------------------------------
// Params

#define MR2_PARAMS_CHANGE(Name, Args, Body)                                                     \
    Params Params::Name Args const                                                              \
    {                                                                                           \
        Params p = *this;                                                                       \
        p.changes.push_back([=](MultiReader2Test& t, MultiReader2ParamsPtr& params) { Body; }); \
        return p;                                                                               \
    }

MR2_PARAMS_CHANGE(add, (Input input), {
    auto list = params.getInputs();
    list.pushBack(t.slotComponent(input.slot));
})
MR2_PARAMS_CHANGE(addSignal, (Spare spare), {
    auto list = params.getInputs();
    list.pushBack(t.spare(spare.id).value);
})
MR2_PARAMS_CHANGE(remove, (Input input), {
    auto list = List<IComponent>();
    for (const auto& c : params.getInputs())
        if (t.slotOf(c) != input.slot)
            list.pushBack(c);
    params.setInputs(list);
})
MR2_PARAMS_CHANGE(order, (std::vector<Input> inputs), {
    auto list = List<IComponent>();
    for (const auto& i : inputs)
        list.pushBack(t.slotComponent(i.slot));
    params.setInputs(list);
})
MR2_PARAMS_CHANGE(main, (Input input), params.setMainInput(t.slotComponent(input.slot)))
MR2_PARAMS_CHANGE(mainNull, (), params.setMainInput(nullptr))
MR2_PARAMS_CHANGE(readType, (SampleType type), params.setValueReadType(type))
MR2_PARAMS_CHANGE(minReadCount, (SizeT count), params.setMinReadCount(count))
MR2_PARAMS_CHANGE(policy, (MultiReader2ErrorPolicy policy), params.setErrorPolicy(policy))
MR2_PARAMS_CHANGE(newAccepts, (), params.setAcceptsDescriptor(t.newStubFunction()))
MR2_PARAMS_CHANGE(newAccepts, (AcceptsRule rule), t.owner->rule = rule; params.setAcceptsDescriptor(t.newStubFunction()))

#undef MR2_PARAMS_CHANGE

Params same()
{
    return {};
}

// ---------------------------------------------------------------------------------------------------------------------
// Expectations

#define MR2_EXPECT_SETTER(Name, Args, Body) \
    Expect Expect::Name Args const          \
    {                                       \
        Expect e = *this;                   \
        Body;                               \
        return e;                           \
    }

MR2_EXPECT_SETTER(at, (int64_t tick), e.firstTick = tick)
MR2_EXPECT_SETTER(level, (ComponentStatus l), e.expectedLevel = l)
MR2_EXPECT_SETTER(noChanges, (), e.changes = false)
MR2_EXPECT_SETTER(hasChanges, (), e.changes = true)
MR2_EXPECT_SETTER(main, (Input input), e.mainSlot = std::optional<int>(input.slot))
MR2_EXPECT_SETTER(mainNull, (), e.mainSlot = std::optional<int>())
MR2_EXPECT_SETTER(domainChanged, (), e.domainChangedSet = true)
MR2_EXPECT_SETTER(resync, (), e.resyncSet = true)
MR2_EXPECT_SETTER(allDescChanged, (), e.allDesc = true)
MR2_EXPECT_SETTER(descChanged, (Input input), e.descChangedSlots.insert(input.slot))
MR2_EXPECT_SETTER(inError, (Input input, InputError err), e.errors[input.slot] = err)
MR2_EXPECT_SETTER(healthy, (Input input), e.errors[input.slot] = InputError::None)
MR2_EXPECT_SETTER(msg, (const std::string& m), e.message = m)
MR2_EXPECT_SETTER(size, (Input input, SizeT bytes), e.sizes[input.slot] = bytes)
MR2_EXPECT_SETTER(inputs, (std::vector<Input> list), {
    std::vector<int> slots;
    for (const auto& i : list)
        slots.push_back(i.slot);
    e.inputSlots = slots;
})
MR2_EXPECT_SETTER(values, (Input input, std::function<double(int64_t)> value), e.valueOverrides[input.slot] = std::move(value))
MR2_EXPECT_SETTER(stubAsked, (std::vector<Input> list), {
    std::vector<int> slots;
    for (const auto& i : list)
        slots.push_back(i.slot);
    std::sort(slots.begin(), slots.end());
    e.asked = slots;
})
MR2_EXPECT_SETTER(stubNotAsked, (), e.asked = std::vector<int>())
MR2_EXPECT_SETTER(unchanged, (), e.isUnchanged = true)
MR2_EXPECT_SETTER(nullSender, (), e.senderNull = true)
MR2_EXPECT_SETTER(synchronizing, (bool on), e.isSynchronizing = on)

#undef MR2_EXPECT_SETTER

Expect count(SizeT n)
{
    Expect e;
    e.expectedCount = n;
    return e;
}

Expect ok()
{
    return {};
}

Expect fails(ErrCode error)
{
    Expect e;
    e.error = error;
    return e;
}

Expect failsAny()
{
    Expect e;
    e.anyError = true;
    return e;
}

Calls calls(SizeT count, std::vector<Expect> results)
{
    return {count, std::move(results)};
}

// ---------------------------------------------------------------------------------------------------------------------
// Fixture

MultiReader2Test::MultiReader2Test(SizeT workers)
    : config(std::make_unique<Config>())
    , handlerState(std::make_unique<Handler>())
    , stub(std::make_unique<Stub>())
    , workers(workers)
    , owner(std::make_unique<Owner>())
    , writers(std::make_unique<Writers>())
    , fixtureWorkers(workers)
{
    errorSink = createWithImplementation<ILoggerSink, ErrorCountingSinkImpl>();
    logger = LoggerWithSinks(List<ILoggerSink>(errorSink));
}

MultiReader2Test::~MultiReader2Test() = default;

void MultiReader2Test::TearDown()
{
    stopEndlessWriters();
    if (workerHeld)
        releaseWorker();
    readerObject.release();
    otherReaders.clear();
    holders.clear();
    if (scheduler.assigned())
    {
        scheduler.waitAll();
        scheduler.stop();
    }
}

MultiReader2Test::Given::Given(MultiReader2Test& test)
    : test(test)
{
}

MultiReader2Test::Given MultiReader2Test::given()
{
    if (constructed)
        ADD_FAILURE() << "given() after the reader was constructed";
    return Given(*this);
}

#define MR2_GIVEN(Name, Args, Body)                             \
    MultiReader2Test::Given& MultiReader2Test::Given::Name Args \
    {                                                           \
        auto& c = *test.config;                                 \
        (void) c;                                               \
        Body;                                                   \
        return *this;                                           \
    }

MR2_GIVEN(inputs, (int count), c.count = count)
MR2_GIVEN(ports, (), c.ports = true)
MR2_GIVEN(unconnected, (Input input), c.ports = true; c.unconnected.insert(input.slot))
MR2_GIVEN(main, (Input input), c.main = input.slot)
MR2_GIVEN(minReadCount, (SizeT count), c.minReadCount = count)
MR2_GIVEN(policy, (MultiReader2ErrorPolicy p), c.policy = p)
MR2_GIVEN(readType, (SampleType t), c.readType = t)
MR2_GIVEN(signal, (Input input, const Sig& s), c.signals[input.slot] = s)
MR2_GIVEN(signal, (Spare spare, const Sig& s), c.spareSignals[spare.id] = s)
MR2_GIVEN(value, (Input input, const Value& v), {
    auto it = c.signals.emplace(input.slot, sig::base()).first;
    it->second.value = v;
})
MR2_GIVEN(value, (Spare spare, const Value& v), {
    auto it = c.spareSignals.emplace(spare.id, sig::base()).first;
    it->second.value = v;
})
MR2_GIVEN(domain, (Input input, const Domain& d), {
    auto it = c.signals.emplace(input.slot, sig::base()).first;
    it->second.domain = d;
})
MR2_GIVEN(valuesAsIndex, (), c.values = Config::Values::Index)
MR2_GIVEN(valuesNaN, (), c.values = Config::Values::NaN)
MR2_GIVEN(verdict, (Input input, Verdict v), c.stub = true; test.stub->verdicts[input.slot] = v)
MR2_GIVEN(accepts, (), c.stub = true)
MR2_GIVEN(accepts, (AcceptsRule rule), c.stub = true; test.owner->rule = rule)
MR2_GIVEN(noScheduler, (), c.noScheduler = true)
MR2_GIVEN(noScheduler, (Input input), c.noSchedulerInputs.insert(input.slot))
MR2_GIVEN(heldByReader, (Input input), c.ports = true; c.heldByReader.insert(input.slot))
MR2_GIVEN(heldByStreamReader, (Input input), c.ports = true; c.heldByStream.insert(input.slot))
MR2_GIVEN(listenerBlock, (Input input, PacketReadyNotification mode), c.ports = true; c.blockListener[input.slot] = mode)
MR2_GIVEN(onePerPacket, (), c.packets = 1)
MR2_GIVEN(differingPackets, (), c.packets = 3)

#undef MR2_GIVEN

void MultiReader2Test::ensureContext()
{
    if (context.assigned())
        return;
    // Each scheduler logs to a logger of its own: a destroyed scheduler removes its component from its logger, and Suite 3
    // creates a second scheduler in the same fixture
    if (!config->noScheduler)
    {
        // MR2_SCHEDULER=production runs one worker without the FIFO wrapper, as applications do
        const char* kind = std::getenv("MR2_SCHEDULER");
        const bool fifo = workers == 1 && !(kind && std::string(kind) == "production");
        const auto schedulerLogger = LoggerWithSinks(List<ILoggerSink>());
        scheduler = fifo ? createWithImplementation<IScheduler, FifoSchedulerImpl>(Scheduler(schedulerLogger, 1))
                         : Scheduler(schedulerLogger, workers);
    }
    context = Context(scheduler, logger, TypeManager(), nullptr, nullptr);
    contextWithoutScheduler = Context(nullptr, logger, TypeManager(), nullptr, nullptr);

    // Main ticks are those of the case's main input at construction, or SIG.BASE's when that domain cannot convert
    const int refSlot = config->main.value_or(1);
    const auto it = config->signals.find(refSlot);
    const Sig refSig = it != config->signals.end() ? it->second : sig::base();
    config->reference = TickDomain::from(refSig.domain.build(nullptr)).value_or(baseTickDomain());
}

std::unique_ptr<TestSignal> MultiReader2Test::createSignal(const ContextPtr& ctx, const std::string& id, const Sig& s)
{
    auto signal = std::make_unique<TestSignal>();
    signal->id = id;
    signal->value = createWithImplementation<ISignalConfig, HarnessSignalImpl>(ctx, nullptr, nullptr, StringPtr(id), nullptr);
    signal->domain = createWithImplementation<ISignalConfig, HarnessSignalImpl>(ctx, nullptr, nullptr, StringPtr(id + "_domain"), nullptr);
    signal->valueImpl = static_cast<HarnessSignalImpl*>(signal->value.getObject());
    signal->domainImpl = static_cast<HarnessSignalImpl*>(signal->domain.getObject());
    signal->value.asPtr<ISignalPrivate>().enableKeepLastValue(false);
    signal->domain.asPtr<ISignalPrivate>().enableKeepLastValue(false);
    signal->value.setDomainSignal(signal->domain);
    signal->valueDesc = s.value.build(nullptr);
    signal->domainDesc = s.domain.build(nullptr);
    signal->valueHistory.push_back(signal->valueDesc);
    signal->domainHistory.push_back(signal->domainDesc);
    signal->valueImpl->setDescriptorSilently(signal->valueDesc);
    signal->domainImpl->setDescriptorSilently(signal->domainDesc);
    return signal;
}

TestInput& MultiReader2Test::input(int slot)
{
    auto it = inputs.find(slot);
    if (it != inputs.end())
        return *it->second;

    ensureContext();
    auto in = std::make_unique<TestInput>();
    in->slot = slot;
    in->id = "in" + std::to_string(slot);
    in->context = config->noSchedulerInputs.count(slot) ? contextWithoutScheduler : context;
    const auto sit = config->signals.find(slot);
    const Sig s = sit != config->signals.end() ? sit->second : sig::base();
    in->own = createSignal(in->context, config->ports ? "s" + std::to_string(slot) : in->id, s);
    in->signal = in->own.get();

    if (config->ports)
    {
        in->port = InputPort(in->context, nullptr, in->id);
        if (config->unconnected.count(slot))
            in->signal = nullptr;
        else
            in->port.connect(in->own->value);

        if (config->heldByReader.count(slot))
        {
            auto p = MultiReader2Params();
            p.setInputs(List<IComponent>(in->port));
            IMultiReader2* holder = nullptr;
            const ErrCode err = createMultiReader2(&holder, p);
            if (OPENDAQ_SUCCEEDED(err))
                holders.push_back(MultiReader2Ptr::Adopt(holder));
            daqClearErrorInfo();
            in->otherListener = in->port.getListener();
        }
        if (config->heldByStream.count(slot))
        {
            holders.push_back(StreamReaderFromPort(in->port, SampleType::Float64, SampleType::Int64));
            in->otherListener = in->port.getListener();
        }
        const auto block = config->blockListener.find(slot);
        if (block != config->blockListener.end())
        {
            in->blockListener = createWithImplementation<IInputPortNotifications, BlockListenerImpl>();
            in->port.setListener(in->blockListener.asPtr<IInputPortNotifications>());
            in->port.setNotificationMethod(block->second);
        }
    }
    return *inputs.emplace(slot, std::move(in)).first->second;
}

TestSignal& MultiReader2Test::spare(int id)
{
    auto it = spares.find(id);
    if (it != spares.end())
        return *it->second;
    ensureContext();
    const auto sit = config->spareSignals.find(id);
    const Sig s = sit != config->spareSignals.end() ? sit->second : sig::base();
    return *spares.emplace(id, createSignal(context, "s" + std::to_string(id), s)).first->second;
}

ComponentPtr MultiReader2Test::slotComponent(int slot)
{
    auto& in = input(slot);
    if (in.port.assigned())
        return in.port;
    return in.own->value;
}

SignalConfigPtr MultiReader2Test::signalOf(Input in)
{
    auto* signal = input(in.slot).signal;
    return signal ? signal->value : nullptr;
}

int MultiReader2Test::slotOf(const ComponentPtr& component) const
{
    if (!component.assigned())
        return 0;
    for (const auto& [slot, in] : inputs)
    {
        if (in->port.assigned() && component == in->port)
            return slot;
        if (!in->port.assigned() && component == in->own->value)
            return slot;
    }
    return -1;
}

std::vector<int> MultiReader2Test::slotsInForce() const
{
    std::vector<int> slots;
    if (paramsInForce.assigned())
        for (const auto& c : paramsInForce.getInputs())
            slots.push_back(slotOf(c));
    return slots;
}

FunctionPtr MultiReader2Test::newStubFunction()
{
    return Function(
        [this](const BaseObjectPtr& inputObj, const BaseObjectPtr& valueObj, const BaseObjectPtr&, const BaseObjectPtr& valuesObj)
            -> BaseObjectPtr
        {
            const int slot = slotOf(inputObj.asPtrOrNull<IComponent>());
            Verdict v = Verdict::Accept;
            {
                std::scoped_lock lock(stub->mutex);
                stub->asked.push_back(slot);
                const auto it = stub->verdicts.find(slot);
                if (it != stub->verdicts.end())
                    v = it->second;
                else if (owner->rule)
                    v = acceptsByRule(slot, valueObj.asPtrOrNull<IDataDescriptor>(), valuesObj.asPtrOrNull<IList>()) ? Verdict::Accept
                                                                                                                     : Verdict::Reject;
            }
            switch (v)
            {
                case Verdict::Reject:
                    return Boolean(false);
                case Verdict::Throw:
                    throw std::runtime_error("acceptsDescriptor stub throws");
                case Verdict::CallReader:
                {
                    SizeT count = 0;
                    auto r = readerRef.assigned() ? readerRef.getRef() : MultiReader2Ptr();
                    if (!r.assigned())  // asked during construction: there is no reader to call yet
                        return Boolean(true);
                    const ErrCode err = r->getAvailableCount(&count);
                    daqClearErrorInfo();
                    std::scoped_lock lock(stub->mutex);
                    stubSaw.push_back(err);
                    return Boolean(true);
                }
                default:
                    return Boolean(true);
            }
        });
}

MultiReader2ParamsPtr MultiReader2Test::initialParams()
{
    auto params = MultiReader2Params();
    auto list = List<IComponent>();
    for (int slot = 1; slot <= config->count; ++slot)
        list.pushBack(slotComponent(slot));
    params.setInputs(list);
    if (config->main)
        params.setMainInput(slotComponent(*config->main));
    params.setMinReadCount(config->minReadCount);
    params.setErrorPolicy(config->policy);
    params.setValueReadType(config->readType);
    if (config->stub)
    {
        stub->function = newStubFunction();
        params.setAcceptsDescriptor(stub->function);
    }
    return params;
}

MultiReader2Ptr& MultiReader2Test::reader()
{
    if (!constructed)
    {
        constructed = true;
        ensureContext();
        const auto params = initialParams();
        readerObject = MultiReader2(params);
        readerRef = readerObject;
        paramsInForce = copyParams(params);  // a copy: mutate() changes the object the reader was given, not what is in force
        paramsObject = params;
        noteReaderListener();
    }
    // The test thread may release readerObject while a handler call runs
    if (inHandler)
        return handlerReader;
    return readerObject;
}

MultiReader2ParamsPtr MultiReader2Test::buildParams(const Params& changes)
{
    reader();
    auto params = copyParams(paramsInForce);
    for (const auto& change : changes.changes)
        change(*this, params);
    return params;
}

void MultiReader2Test::setupBase(const std::function<void()>& then)
{
    setup(
        [&]
        {
            timeline(Timeline::Base);
            read(0, nullData);
            if (then)
                then();
        });
}

void MultiReader2Test::setupHandler(std::function<void()> body, const std::function<void()>& then)
{
    setup(
        [&]
        {
            handler(std::move(body));
            timeline(Timeline::Base);
            idle();
            if (then)
                then();
        });
}

void MultiReader2Test::setup(const std::function<void()>& steps)
{
    reader();
    muted = true;
    steps();
    muted = false;
}

// ---------------------------------------------------------------------------------------------------------------------
// Scheduler

void MultiReader2Test::holdWorker()
{
    if (!scheduler.assigned() || workerHeld)
        return;
    workerHeld = true;
    workerRunning = false;
    scheduler.scheduleFunction(Function(
        [this]() -> BaseObjectPtr
        {
            std::unique_lock lock(workerMutex);
            workerRunning = true;
            workerCv.notify_all();
            workerCv.wait(lock, [this] { return !workerHeld; });
            return nullptr;
        }));
    std::unique_lock lock(workerMutex);
    workerCv.wait(lock, [this] { return workerRunning; });
}

void MultiReader2Test::releaseWorker()
{
    {
        std::scoped_lock lock(workerMutex);
        workerHeld = false;
    }
    workerCv.notify_all();
}

void MultiReader2Test::flush()
{
    if (scheduler.assigned())
        scheduler.waitAll();
}

void MultiReader2Test::writeLine(const std::function<void()>& send)
{
    reader();
    if (workerBlocked)
    {
        send();
        return;
    }
    holdWorker();
    send();
    releaseWorker();
    flush();
}

void MultiReader2Test::blockWorker()
{
    reader();
    holdWorker();
    workerBlocked = true;
}

void MultiReader2Test::unblockWorker()
{
    workerBlocked = false;
    releaseWorker();
    flush();
}

HandlerLog MultiReader2Test::idle()
{
    SizeT before;
    do
    {
        {
            std::scoped_lock lock(handlerState->mutex);
            before = handlerState->log.calls;
        }
        flush();
    } while (
        [&]
        {
            std::scoped_lock lock(handlerState->mutex);
            return handlerState->log.calls != before;
        }());

    errorsBeforeIdle = errorsAtIdle;
    errorsAtIdle = countingSink(errorSink).count();
    std::scoped_lock lock(handlerState->mutex);
    HandlerLog out = std::move(handlerState->log);
    handlerState->log = {};
    return out;
}

// ---------------------------------------------------------------------------------------------------------------------
// Writes

void MultiReader2Test::write(Input in, int64_t first, int64_t last)
{
    writeLine(sendWrite(in, first, last));
}

void MultiReader2Test::writePackets(Input in, int64_t first, int64_t last)
{
    auto& testInput = input(in.slot);
    TestSignal* signal = testInput.signal;
    if (!signal)
    {
        ADD_FAILURE() << testInput.id << " has no signal to write to";
        return;
    }

    const auto& ref = config->reference;
    const auto own = TickDomain::from(signal->domainDesc);
    const TickDomain dom = own.value_or(baseTickDomain());
    const int64_t u0 = dom.tickAt(ref.timeOf(first));
    const int64_t uEnd = dom.tickAt(ref.timeOf(last));

    std::vector<std::pair<int64_t, int64_t>> samples;  // own tick, reference tick
    for (int64_t u = u0; u <= uEnd; u += dom.delta)
        samples.emplace_back(u, ref.tickAt(dom.timeOf(u)));

    const bool isMain = in.slot == config->main.value_or(1);
    const SizeT perPacket = config->packets == 1 ? 1 : (config->packets == 3 && !isMain ? 7 : 10);

    const auto valueDesc = signal->valueDesc.assigned() ? signal->valueDesc : Value::base().build(nullptr);
    DataDescriptorPtr domainDesc = signal->domainDesc;
    if (!domainDesc.assigned() || domainDesc.getDimensions().getCount() != 0)
        domainDesc = Domain().build(nullptr);
    const bool explicitRule = domainDesc.getRule().getType() == DataRuleType::Explicit;

    for (SizeT i = 0; i < samples.size(); i += perPacket)
    {
        const SizeT n = std::min(perPacket, samples.size() - i);
        DataPacketPtr domainPacket;
        if (explicitRule)
        {
            domainPacket = DataPacket(domainDesc, n);
            auto* ticks = static_cast<int64_t*>(domainPacket.getRawData());
            for (SizeT k = 0; k < n; ++k)
                ticks[k] = samples[i + k].first;
        }
        else
        {
            domainPacket = DataPacket(domainDesc, n, samples[i].first - (own ? own->start : 0));
        }
        auto packet = DataPacketWithDomain(domainPacket, valueDesc, n);
        auto* raw = static_cast<uint8_t*>(packet.getRawData());
        const SizeT sampleSize = valueDesc.getRawSampleSize();
        for (SizeT k = 0; k < n; ++k)
        {
            double v = static_cast<double>(samples[i + k].second);
            if (config->values == Config::Values::Index)
                v = static_cast<double>(signal->nextIndex++);
            else if (config->values == Config::Values::NaN)
                v = std::numeric_limits<double>::quiet_NaN();
            fillSample(raw + k * sampleSize, valueDesc, v);
            signal->written[samples[i + k].second] = v;
            testInput.written[samples[i + k].second] = v;
        }
        signal->value.sendPacket(packet);
    }
}

void MultiReader2Test::writeEmpty(Input in)
{
    writeLine(
        [&]
        {
            auto* signal = input(in.slot).signal;
            const auto valueDesc = signal->valueDesc.assigned() ? signal->valueDesc : Value::base().build(nullptr);
            const auto domainDesc = signal->domainDesc.assigned() ? signal->domainDesc : Domain().build(nullptr);
            signal->value.sendPacket(DataPacketWithDomain(DataPacket(domainDesc, 0, 0), valueDesc, 0));
        });
}

void MultiReader2Test::sendEvent(TestSignal& signal,
                                 const std::optional<DataDescriptorPtr>& value,
                                 const std::optional<DataDescriptorPtr>& domain)
{
    if (value)
    {
        signal.valueDesc = *value;
        signal.valueHistory.push_back(*value);
        signal.valueImpl->setDescriptorSilently(*value);
    }
    if (domain)
    {
        signal.domainDesc = *domain;
        signal.domainHistory.push_back(*domain);
        signal.domainImpl->setDescriptorSilently(*domain);
    }
    signal.value.sendPacket(DataDescriptorChangedEventPacket(value ? descriptorToEventPacketParam(*value) : nullptr,
                                                             domain ? descriptorToEventPacketParam(*domain) : nullptr));
}

void MultiReader2Test::announce(Input in, const Value& value)
{
    writeLine(sendAnnounce(in, value, std::nullopt));
}

void MultiReader2Test::announce(Input in, const Domain& domain)
{
    writeLine(sendAnnounce(in, std::nullopt, domain));
}

void MultiReader2Test::announce(Input in, const Value& value, const Domain& domain)
{
    writeLine(sendAnnounce(in, value, domain));
}

void MultiReader2Test::announce(Spare sp, const Value& value)
{
    writeLine(
        [&]
        {
            auto& s = spare(sp.id);
            sendEvent(s, value.build(s.valueDesc), std::nullopt);
        });
}

void MultiReader2Test::gap(Input in)
{
    writeLine(sendGap(in));
}

void MultiReader2Test::otherEvent(Input in)
{
    writeLine(sendOtherEvent(in));
}

void MultiReader2Test::disconnect(Input in)
{
    writeLine(sendDisconnect(in));
}

void MultiReader2Test::connect(Input in)
{
    writeLine(sendConnect(in));
}

void MultiReader2Test::connect(Input in, Spare sp)
{
    writeLine(
        [&]
        {
            auto& i = input(in.slot);
            ASSERT_TRUE(i.port.assigned()) << "connect needs an input port";
            auto& s = spare(sp.id);
            i.port.connect(s.value);
            i.signal = &s;
        });
}

void MultiReader2Test::removeSignal(Input in)
{
    writeLine(sendRemoval(in));
}

std::function<void()> MultiReader2Test::sendWrite(Input in, int64_t first, int64_t last)
{
    return [this, in, first, last] { writePackets(in, first, last); };
}

std::function<void()> MultiReader2Test::sendAnnounce(Input in, std::optional<Value> value, std::optional<Domain> domain)
{
    return [this, in, value, domain]
    {
        auto& s = *input(in.slot).signal;
        sendEvent(s,
                  value ? std::optional<DataDescriptorPtr>(value->build(s.valueDesc)) : std::nullopt,
                  domain ? std::optional<DataDescriptorPtr>(domain->build(s.domainDesc)) : std::nullopt);
    };
}

std::function<void()> MultiReader2Test::sendGap(Input in)
{
    return [this, in] { input(in.slot).signal->value.sendPacket(ImplicitDomainGapDetectedEventPacket(Integer(1))); };
}

std::function<void()> MultiReader2Test::sendOtherEvent(Input in)
{
    return [this, in] { input(in.slot).signal->value.sendPacket(EventPacket("MR2_TEST_EVENT", Dict<IString, IBaseObject>())); };
}

std::function<void()> MultiReader2Test::sendDisconnect(Input in)
{
    return [this, in]
    {
        auto& i = input(in.slot);
        ASSERT_TRUE(i.port.assigned()) << "disconnect needs an input port";
        i.port.disconnect();
        i.signal = nullptr;
    };
}

std::function<void()> MultiReader2Test::sendConnect(Input in)
{
    return [this, in]
    {
        auto& i = input(in.slot);
        ASSERT_TRUE(i.port.assigned()) << "connect needs an input port";
        i.port.connect(i.own->value);
        i.signal = i.own.get();
    };
}

std::function<void()> MultiReader2Test::sendRemoval(Input in)
{
    return [this, in]
    {
        auto& i = input(in.slot);
        i.signal->value.asPtr<IRemovable>().remove();
        i.signal->removed = true;
        if (i.port.assigned())
            i.signal = nullptr;
    };
}

Script MultiReader2Test::script(Timeline tl)
{
    reader();
    const auto slots = slotsInForce();
    const int pinned = slotOf(paramsInForce.getMainInput());
    const Input m{pinned > 0 ? pinned : slots.front()};
    std::vector<Input> others;
    for (int slot : slots)
        if (slot != m.slot)
            others.push_back(Input{slot});

    Script s;
    const auto one = [&](Input in, int64_t first, int64_t last) { s.push_back({in, sendWrite(in, first, last)}); };
    const auto each = [&](int64_t first, int64_t last, SizeT skipLast = 0)
    {
        for (SizeT i = 0; i + skipLast < others.size(); ++i)
            one(others[i], first, last);
    };
    const int64_t shortLast = static_cast<int64_t>(paramsInForce.getMinReadCount()) - 2;
    const auto& ref = config->reference;
    const int64_t epochFirst = ref.tickAt({T0, 0});
    switch (tl)
    {
        case Timeline::Base:
            for (int slot : slots)
                one(Input{slot}, 0, 99);
            break;  // slot order, as the base setup
        case Timeline::Apart:
            one(m, 0, 99);
            each(1000, 1099);
            break;
        case Timeline::Exact:
            one(m, 0, 99);
            each(2099, 2198);
            break;
        case Timeline::Ahead:
            one(m, 0, 99);
            each(2100, 2199);
            break;
        case Timeline::Behind:
            each(0, 99);
            one(m, 2100, 2199);
            break;
        // [ASSUMPTION] the continuations of S1.SYNC.11 and S1.SYNC.13, with the first non-main input hit
        case Timeline::JumpAhead:
            one(m, 0, 99);
            each(0, 99);
            one(m, 100, 599);
            one(others.front(), 500, 599);
            break;
        case Timeline::JumpBack:
            one(m, 0, 99);
            each(0, 99);
            one(others.front(), 50, 199);
            one(m, 100, 199);
            break;
        case Timeline::Starve1:
            one(m, 0, 99);
            each(0, 99, 1);
            break;
        case Timeline::StarveAll:
            one(m, 0, 99);
            break;
        case Timeline::StarveMain:
            each(0, 99);
            break;
        case Timeline::Short:
            if (shortLast >= 0)
            {
                one(m, 0, shortLast);
                each(0, shortLast);
            }
            break;
        case Timeline::Starve2s:
            one(m, 0, 2001);
            each(0, 2001, 1);
            break;
        case Timeline::CatchUp:
            if (scripted == Timeline::Ahead)
            {
                one(m, 100, 299);
                each(2200, 2299);
                one(m, 300, 2299);
            }
            else
            {
                each(100, 199);
                one(m, 2200, 2299);
                each(2200, 2299);
            }
            break;
        case Timeline::Epoch:
            for (int slot : slots)
                one(Input{slot}, epochFirst, epochFirst + 99 * ref.delta);
            break;
    }
    scripted = tl;
    return s;
}

Script MultiReader2Test::eventScript(Input hit, std::vector<std::function<void()>> event, int64_t restFirst, int64_t restLast)
{
    reader();
    Script s;
    for (int slot : slotsInForce())
    {
        const Input in{slot};
        if (slot != hit.slot)
        {
            s.push_back({in, sendWrite(in, 0, 99)});
            continue;
        }
        s.push_back({in, sendWrite(in, 0, 49)});
        for (auto& send : event)
            s.push_back({in, std::move(send)});
        if (restFirst != NoRest)
            s.push_back({in, sendWrite(in, restFirst, restLast)});
    }
    return s;
}

void MultiReader2Test::timeline(Timeline tl)
{
    runScript(script(tl), false);
}

void MultiReader2Test::runScript(const Script& s, bool concurrent)
{
    if (!concurrent)
    {
        for (const auto& step : s)
            writeLine(step.send);
        return;
    }
    startScript(s);
    joinScript();
}

void MultiReader2Test::startScript(const Script& s)
{
    std::map<int, std::vector<std::function<void()>>> perInput;
    for (const auto& step : s)
        perInput[step.input.slot].push_back(step.send);
    auto gate = std::make_shared<std::promise<void>>();
    std::shared_future<void> start = gate->get_future().share();
    for (auto& [slot, steps] : perInput)
    {
        input(slot);
        ++writers->running;
        writers->threads.emplace_back(
            [this, start, steps = std::move(steps)]
            {
                start.wait();
                for (const auto& send : steps)
                    send();
                --writers->running;
            });
    }
    gate->set_value();
}

bool MultiReader2Test::scriptRunning()
{
    return writers->running > 0;
}

void MultiReader2Test::joinScript()
{
    for (auto& t : writers->threads)
        t.join();
    writers->threads.clear();
}

void MultiReader2Test::startEndlessWriters()
{
    writers->stop = false;
    Script s;
    for (int slot : slotsInForce())
        s.push_back({Input{slot},
                     [this, slot]
                     {
                         for (int64_t t = 0; !writers->stop; t += 10)
                             writePackets(Input{slot}, t, t + 9);
                     }});
    startScript(s);
}

void MultiReader2Test::stopEndlessWriters()
{
    writers->stop = true;
    joinScript();
}

// ---------------------------------------------------------------------------------------------------------------------
// Reader calls

Result MultiReader2Test::record(Result result)
{
    if (result.status.assigned())
    {
        checkInvariants(result);
        {
            std::scoped_lock lock(stateMutex);
            previousStatus = result.status;
        }
        if (result.status.getHasChanges())
            ownerOnStatus(result.status);
    }
    {
        std::scoped_lock lock(stateMutex);
        lastResult = result;
    }
    if (recordingRun &&
        (result.kind == Result::Kind::Read || result.kind == Result::Kind::Skip || result.kind == Result::Kind::ReadWithDomain))
    {
        std::scoped_lock lock(runMutex);
        runLog.push_back(result);
    }
    if (inHandler)
    {
        std::scoped_lock lock(handlerState->mutex);
        handlerState->log.results.push_back(result);
    }
    return result;
}

std::vector<int> MultiReader2Test::takeStubAsked()
{
    std::scoped_lock lock(stub->mutex);
    auto asked = std::move(stub->asked);
    stub->asked.clear();
    std::sort(asked.begin(), asked.end());
    return asked;
}

Result MultiReader2Test::runRead(Result::Kind kind, SizeT count, const ReadArgs& args)
{
    Result r{kind};
    r.args = args;
    r.step = (kind == Result::Kind::Read   ? "read "
              : kind == Result::Kind::Skip ? "skip "
                                           : "readd ") +
             std::to_string(count) + (args.nullData ? " null" : "") +
             (args.nullBuffer ? " null in" + std::to_string(args.nullBuffer->slot) : "") + (args.nullCount ? " null-count" : "") +
             (args.nullStatus ? " null-status" : "") + (args.timeoutMs ? " timeout " + std::to_string(args.timeoutMs) : "");
    auto& rd = reader();  // constructs first, so a CP1 read gets a buffer per input
    {
        std::scoped_lock lock(stateMutex);
        r.previous = previousStatus;
        r.slots = slotsInForce();
    }

    const SizeT bytes = std::max<SizeT>(count, 1) * BytesPerSample;
    std::vector<void*> data;
    if (kind == Result::Kind::ReadWithDomain)
    {
        r.domainBuffer.assign(bytes, Sentinel);  // room past the ticks, so an overrun lands on the sentinel
        data.push_back(r.domainBuffer.data());
    }
    for (int slot : r.slots)
    {
        r.buffers.emplace_back(bytes, Sentinel);
        data.push_back(args.nullBuffer && args.nullBuffer->slot == slot ? nullptr : r.buffers.back().data());
    }

    SizeT n = count;
    SizeT offset = 0;
    IMultiReader2Status* status = nullptr;
    void* dataArg = args.nullData ? nullptr : data.data();
    SizeT* countArg = args.nullCount ? nullptr : &n;
    IMultiReader2Status** statusArg = args.nullStatus ? nullptr : &status;
    switch (kind)
    {
        case Result::Kind::Read:
            r.error = rd->read(dataArg, countArg, args.timeoutMs, &offset, statusArg);
            break;
        case Result::Kind::ReadWithDomain:
            r.error = rd->readWithDomain(dataArg, countArg, args.timeoutMs, statusArg);
            break;
        default:
            r.error = rd->skipSamples(countArg, args.timeoutMs, &offset, statusArg);
            break;
    }
    daqClearErrorInfo();
    r.count = n;
    r.packetOffset = offset;
    r.status = MultiReader2StatusPtr::Adopt(status);
    r.stubAsked = takeStubAsked();
    return record(std::move(r));
}

Result MultiReader2Test::read(SizeT count, const ReadArgs& args)
{
    return runRead(Result::Kind::Read, count, args);
}

Result MultiReader2Test::skip(SizeT count, const ReadArgs& args)
{
    return runRead(Result::Kind::Skip, count, args);
}

Result MultiReader2Test::readWithDomain(SizeT count, const ReadArgs& args)
{
    return runRead(Result::Kind::ReadWithDomain, count, args);
}

Result MultiReader2Test::readingCall(ReadCall call, SizeT count, const ReadArgs& args)
{
    switch (call)
    {
        case ReadCall::Read:
            return read(count, args);
        case ReadCall::Skip:
            return skip(count, args);
        default:
            return readWithDomain(count, args);
    }
}

Result MultiReader2Test::avail()
{
    Result r{Result::Kind::Avail};
    r.step = "avail";
    SizeT n = 0;
    r.error = reader()->getAvailableCount(&n);
    daqClearErrorInfo();
    r.count = n;
    r.stubAsked = takeStubAsked();
    return record(std::move(r));
}

Result MultiReader2Test::configure(const Params& changes)
{
    Result r{Result::Kind::Configure};
    r.step = "configure";
    MultiReader2ParamsPtr params;
    {
        std::scoped_lock lock(stateMutex);
        params = buildParams(changes);
        r.previous = previousStatus;
    }
    IMultiReader2Status* status = nullptr;
    r.error = reader()->configure(params, &status);
    daqClearErrorInfo();
    r.status = MultiReader2StatusPtr::Adopt(status);
    r.stubAsked = takeStubAsked();
    {
        std::scoped_lock lock(stateMutex);
        if (OPENDAQ_SUCCEEDED(r.error))
        {
            paramsInForce = copyParams(params);
            paramsObject = params;
        }
        r.slots = slotsInForce();
    }
    if (OPENDAQ_SUCCEEDED(r.error))
        noteReaderListener();
    return record(std::move(r));
}

Result MultiReader2Test::construct(const Params& changes)
{
    Result r{Result::Kind::Construct};
    r.step = "construct";
    reader();
    auto params = copyParams(initialParams());
    for (const auto& change : changes.changes)
        change(*this, params);
    IMultiReader2* other = nullptr;
    r.error = createMultiReader2(&other, params);
    daqClearErrorInfo();
    r.stubAsked = takeStubAsked();
    if (OPENDAQ_SUCCEEDED(r.error))
        otherReaders.push_back(MultiReader2Ptr::Adopt(other));
    return record(std::move(r));
}

void MultiReader2Test::mutate(const Params& changes)
{
    reader();
    for (const auto& change : changes.changes)
        change(*this, paramsObject);
}

Result MultiReader2Test::inputStatus(Input in)
{
    Result r{Result::Kind::InputStatus};
    r.step = "input-status(in" + std::to_string(in.slot) + ")";
    r.inputSlot = in.slot;
    if (!previousStatus.assigned())
    {
        ADD_FAILURE() << "inputStatus() needs a previous status";
        r.error = OPENDAQ_ERR_INVALIDSTATE;
        return r;
    }
    const Snapshot before = snapshot();
    IMultiReader2InputStatus* status = nullptr;
    r.error = previousStatus->getInputStatus(slotComponent(in.slot), &status);
    daqClearErrorInfo();
    r.input = MultiReader2InputStatusPtr::Adopt(status);
    r.unchanged = snapshot() == before;
    return record(std::move(r));
}

std::vector<Result> MultiReader2Test::drain(ReadCall call)
{
    std::vector<Result> reads;
    for (int guard = 0; guard < 100000; ++guard)
    {
        SizeT n = 0;
        if (OPENDAQ_FAILED(reader()->getAvailableCount(&n)))
        {
            daqClearErrorInfo();
            n = 0;
        }
        reads.push_back(n > 0 ? readingCall(call, n) : readingCall(call, 0, nullData));
        const Result& r = reads.back();
        if (OPENDAQ_FAILED(r.error) || !r.status.assigned())
            return reads;
        if (r.status.getHasChanges() && inHandler)
            ownerActs(r.status);
        else if (r.count == 0 && !r.status.getHasChanges())
            return reads;
    }
    ADD_FAILURE() << "drain did not finish";
    return reads;
}

void MultiReader2Test::sender()
{
    Result r{Result::Kind::Sender};
    r.step = "sender";
    r.sender = handlerSender;
    record(std::move(r));
}

void MultiReader2Test::release()
{
    readerObject.release();
}

void MultiReader2Test::verdict(Input in, Verdict v)
{
    std::scoped_lock lock(stub->mutex);
    stub->verdicts[in.slot] = v;
}

void MultiReader2Test::handler(std::function<void()> body)
{
    handlerState->body = std::move(body);
    if (handlerState->installed)
        return;
    handlerState->installed = true;
    EventPtr<> raw;
    const ErrCode err = reader()->getOnDataAvailable(&raw);
    daqClearErrorInfo();
    if (OPENDAQ_FAILED(err))
    {
        ADD_FAILURE() << "getOnDataAvailable failed: " << errorName(err);
        return;
    }
    Event<BaseObjectPtr, EventArgsPtr<>>(raw) += daq::event(this, &MultiReader2Test::onDataAvailable);
    handlerState->subscribed = true;
}

void MultiReader2Test::onDataAvailable(BaseObjectPtr& sender, EventArgsPtr<>&)
{
    struct Scope
    {
        Scope(MultiReader2Test& t, const BaseObjectPtr& s)
        {
            inHandler = true;
            handlerSender = s;
            handlerReader = t.readerRef.getRef();
            if (!handlerReader.assigned())
                ++t.handlerState->startedWithoutReader;
        }
        ~Scope()
        {
            inHandler = false;
            handlerSender.release();
            handlerReader.release();
        }
    } scope(*this, sender);
    {
        std::scoped_lock lock(handlerState->mutex);
        handlerState->log.calls++;
    }
    ++handlerState->started;
    if (handlerState->holdNext.exchange(false))
    {
        std::unique_lock lock(handlerState->mutex);
        handlerState->held = true;
        handlerState->heldCv.notify_all();
        handlerState->heldCv.wait(lock, [this] { return !handlerState->held; });
    }
    const int running = ++handlerState->running;
    for (int seen = handlerState->maxRunning; running > seen && !handlerState->maxRunning.compare_exchange_weak(seen, running);)
        ;
    struct Leave
    {
        std::atomic<int>& running;
        ~Leave()
        {
            --running;
        }
    } leave{handlerState->running};
    handlerState->body();
}

// ---------------------------------------------------------------------------------------------------------------------
// Expectations

namespace
{
struct State
{
    ComponentStatus level;
    std::string message;
    BaseObjectPtr main;
    DataDescriptorPtr domain;
    std::vector<std::tuple<BaseObjectPtr, InputError, DataDescriptorPtr, SizeT>> inputs;

    static State of(const MultiReader2StatusPtr& s)
    {
        State st{s.getStatus(), s.getStatusMessage().toStdString(), s.getMainInput(), s.getDomainDescriptor()};
        for (const auto& i : s.getInputs())
            st.inputs.emplace_back(i.getInput(), i.getError(), i.getDescriptor(), i.getSampleSize());
        return st;
    }

    bool operator==(const State& o) const
    {
        if (level != o.level || message != o.message || !BaseObjectPtr::Equals(main, o.main) || !BaseObjectPtr::Equals(domain, o.domain) ||
            inputs.size() != o.inputs.size())
            return false;
        for (SizeT i = 0; i < inputs.size(); ++i)
        {
            const auto& [a1, e1, d1, s1] = inputs[i];
            const auto& [a2, e2, d2, s2] = o.inputs[i];
            if (!BaseObjectPtr::Equals(a1, a2) || e1 != e2 || !BaseObjectPtr::Equals(d1, d2) || s1 != s2)
                return false;
        }
        return true;
    }
};
}

void MultiReader2Test::checkInvariants(const Result& result)
{
    if (muted || !statusInvariants)
        return;
    const auto& st = result.status;
    SCOPED_TRACE("status invariants");

    const auto inputsList = st.getInputs();
    std::vector<int> slots;
    for (const auto& i : inputsList)
        slots.push_back(slotOf(i.getInput()));
    EXPECT_EQ(slots, slotsInForce()) << "R-API-36: the status lists the inputs of the params in force";

    const bool anyEvent = st.getDomainDescriptorChanged() || st.getResynchronized() ||
                          std::any_of(inputsList.begin(), inputsList.end(), [](const auto& i) { return i.getDescriptorChanged(); });
    if (result.previous.assigned() && (!(State::of(st) == State::of(result.previous)) || anyEvent))
        EXPECT_TRUE(st.getHasChanges()) << "R-API-31: a status that differs from the previous one has changes";

    const auto level = st.getStatus();
    if (level == Error)
    {
        EXPECT_FALSE(st.getDomainDescriptor().assigned()) << "R-STAT-2: no domain descriptor under Error";
        EXPECT_FALSE(anyEvent) << "R-STAT-8: no event fields under Error";
        for (const auto& i : inputsList)
        {
            EXPECT_FALSE(i.getDescriptor().assigned()) << "R-STAT-2: no descriptor under Error";
            EXPECT_EQ(i.getSampleSize(), 0u) << "R-STAT-2: sample size 0 under Error";
        }
    }

    SizeT inError = 0;
    std::vector<std::string> expectedPrefixes;
    for (const auto& i : inputsList)
        if (i.getError() != InputError::None)
        {
            ++inError;
            expectedPrefixes.push_back(i.getInput().getLocalId().toStdString() + ": ");
            EXPECT_EQ(i.getSampleSize(), 0u) << "R-API-30: an input in error has sample size 0";
            EXPECT_FALSE(i.getDescriptorChanged()) << "R-API-28: an input in error reports no descriptor change";
        }

    const auto policy = paramsInForce.getErrorPolicy();
    const auto pinned = paramsInForce.getMainInput();
    const auto pinnedSlot = slotOf(pinned);
    bool pinnedInError = false;
    for (const auto& i : inputsList)
        if (pinned.assigned() && slotOf(i.getInput()) == pinnedSlot && i.getError() != InputError::None)
            pinnedInError = true;
    if (inError == 0)
        EXPECT_EQ(level, Ok) << "R-STAT-6: Ok exactly when no input is in error";
    else if (policy == Invalidate)
        EXPECT_EQ(level, Error) << "R-POL-1: an input in error invalidates the reader";
    else if (inError == inputsList.getCount() || pinnedInError)
        EXPECT_EQ(level, Error) << "R-POL-3: Error when no input is healthy or the pinned main is in error";
    else
        EXPECT_EQ(level, Warning) << "R-STAT-7: Warning while inputs are set aside";

    const auto main = st.getMainInput();
    if (pinned.assigned())
        EXPECT_EQ(slotOf(main), pinnedSlot) << "R-API-34: a pinned main input is reported";
    else if (level == Error || inputsList.getCount() == 0)
        EXPECT_FALSE(main.assigned()) << "R-MAIN-4: no automatic main under Error or without inputs";
    else
    {
        EXPECT_TRUE(main.assigned()) << "R-MAIN-1: an automatic main input is chosen";
        for (const auto& i : inputsList)
            if (slotOf(i.getInput()) == slotOf(main))
                EXPECT_EQ(i.getError(), InputError::None) << "R-MAIN-1: the automatic main input is healthy";
    }

    const std::string message = st.getStatusMessage().toStdString();
    if (level == Ok)
        EXPECT_EQ(message, "") << "R-API-32: empty message under Ok";
    else
    {
        SizeT pos = 0;
        for (SizeT k = 0; k < expectedPrefixes.size(); ++k)
        {
            EXPECT_EQ(message.compare(pos, expectedPrefixes[k].size(), expectedPrefixes[k]), 0)
                << "R-MSG-1: entry " << k << " of \"" << message << "\" starts with " << expectedPrefixes[k];
            const SizeT next = message.find("; ", pos);
            if (next == std::string::npos)
            {
                EXPECT_EQ(k + 1, expectedPrefixes.size()) << "R-MSG-1: one entry per input in error in \"" << message << "\"";
                break;
            }
            EXPECT_LT(k + 1, expectedPrefixes.size()) << "R-MSG-1: no entry follows the last input in error in \"" << message << "\"";
            pos = next + 2;
        }
    }
}

void MultiReader2Test::expect(const Result& result, const Expect& e)
{
    if (muted)
        return;
    SCOPED_TRACE("expectation " + std::to_string(++expectations) + " of the row: " + result.step);
    checkResult(result, e);
}

void MultiReader2Test::expect(const std::function<Result()>& call, const Expect& e)
{
    const Snapshot before = e.isUnchanged ? snapshot() : Snapshot{};
    const Result result = call();
    Expect checked = e;
    checked.isUnchanged = false;  // asserted here through the snapshots
    expect(result, checked);
    if (e.isUnchanged)
        EXPECT_TRUE(snapshot() == before) << "decision 26: the reader is unchanged by a rejected call";
}

void MultiReader2Test::expect(const HandlerLog& log, const Calls& expected)
{
    EXPECT_EQ(log.calls, expected.count) << "handler calls since the previous idle";
    EXPECT_EQ(log.results.size(), expected.results.size()) << "results of the handler's reader calls";
    for (SizeT i = 0; i < std::min(log.results.size(), expected.results.size()); ++i)
    {
        SCOPED_TRACE("handler result " + std::to_string(i) + ": " + log.results[i].step);
        checkResult(log.results[i], expected.results[i]);
    }
}

void MultiReader2Test::expect(const std::vector<Result>& results, const std::vector<Expect>& expected)
{
    if (muted)
        return;
    SCOPED_TRACE("expectation " + std::to_string(++expectations) + " of the row: drain");
    EXPECT_EQ(results.size(), expected.size()) << "reads of the drain";
    for (SizeT i = 0; i < std::min(results.size(), expected.size()); ++i)
    {
        SCOPED_TRACE("drain read " + std::to_string(i) + ": " + results[i].step);
        checkResult(results[i], expected[i]);
    }
}

void MultiReader2Test::checkResult(const Result& r, const Expect& e)
{
    if (e.error)
        EXPECT_EQ(r.error, *e.error) << "error code, got " << errorName(r.error) << ", expected " << errorName(*e.error);
    else if (e.anyError)
        EXPECT_TRUE(OPENDAQ_FAILED(r.error)) << "expected an error code";
    else
        EXPECT_EQ(r.error, OPENDAQ_SUCCESS) << "error code, got " << errorName(r.error);
    if (r.kind == Result::Kind::InputStatus && e.isUnchanged && r.unchanged)
        EXPECT_TRUE(*r.unchanged) << "decision 26: the reader is unchanged by a rejected call";
    else if (e.isUnchanged)  // only input-status records it; any other call takes its snapshots in expect(call, ...)
        ADD_FAILURE() << "unchanged is asserted only on a call passed as a lambda";
    if (OPENDAQ_FAILED(r.error) || e.error || e.anyError)
        return;
    if (r.kind == Result::Kind::InputStatus)
    {
        if (!r.input.assigned())
        {
            ADD_FAILURE() << "input-status returned no status";
            return;
        }
        for (const auto& [slot, err] : e.errors)
            if (slot == r.inputSlot)
                EXPECT_EQ(r.input.getError(), err) << "error of in" << slot;
        for (const auto& [slot, bytes] : e.sizes)
            if (slot == r.inputSlot)
                EXPECT_EQ(r.input.getSampleSize(), bytes) << "sample size of in" << slot;
        return;
    }

    if (r.kind == Result::Kind::Sender)
    {
        if (e.senderNull)
            EXPECT_FALSE(r.sender.assigned()) << "R-TRIG: the sender is null";
        return;
    }

    if (e.expectedCount)
        EXPECT_EQ(r.count, *e.expectedCount);
    if (e.asked)
        EXPECT_EQ(r.stubAsked, *e.asked) << "acceptsDescriptor asked exactly these inputs";
    if (r.status.assigned())
        checkStatus(r, e);
    else if (e.expectedLevel || e.changes || e.mainSlot || e.domainChangedSet || e.resyncSet || e.allDesc || !e.descChangedSlots.empty() ||
             !e.errors.empty() || e.message || !e.sizes.empty() || e.inputSlots)
        ADD_FAILURE() << "status fields are listed for a call that returns no status";
    if (r.kind == Result::Kind::Read || r.kind == Result::Kind::Skip || r.kind == Result::Kind::ReadWithDomain)
        checkBuffers(r, e);
}

void MultiReader2Test::checkStatus(const Result& r, const Expect& e)
{
    const auto& st = r.status;
    if (e.expectedLevel)
        EXPECT_EQ(st.getStatus(), *e.expectedLevel) << "level";
    else if (r.previous.assigned())
        EXPECT_EQ(st.getStatus(), r.previous.getStatus()) << "an unlisted level equals the previous status's";

    EXPECT_EQ(st.getHasChanges(), e.changes.value_or(true)) << "hasChanges";
    EXPECT_EQ(st.getDomainDescriptorChanged(), e.domainChangedSet) << "domainDescriptorChanged";
    EXPECT_EQ(st.getResynchronized(), e.resyncSet) << "resynchronized";
    if (e.isSynchronizing)
        EXPECT_EQ(st.getSynchronizing(), *e.isSynchronizing) << "synchronizing";

    const auto level = st.getStatus();
    const int mainSlot = slotOf(st.getMainInput());
    if (e.domainChangedSet && mainSlot > 0)
    {
        const auto& history = input(mainSlot).signal ? input(mainSlot).signal->domainHistory : input(mainSlot).own->domainHistory;
        EXPECT_TRUE(
            std::any_of(history.begin(), history.end(), [&](const auto& d) { return BaseObjectPtr::Equals(d, st.getDomainDescriptor()); }))
            << "the reported domain descriptor is one the main input announced";
    }

    for (const auto& i : st.getInputs())
    {
        const int slot = slotOf(i.getInput());
        const bool expectedChanged = e.allDesc ? (i.getError() == InputError::None && level != Error) : e.descChangedSlots.count(slot) > 0;
        EXPECT_EQ(i.getDescriptorChanged(), expectedChanged) << "descriptorChanged of in" << slot;
        if (i.getDescriptorChanged() && slot > 0 && input(slot).signal)
        {
            const auto& history = input(slot).signal->valueHistory;
            EXPECT_TRUE(
                std::any_of(history.begin(), history.end(), [&](const auto& d) { return BaseObjectPtr::Equals(d, i.getDescriptor()); }))
                << "the reported descriptor of in" << slot << " is one it announced";
        }
        // Without domain changed no configure, recovery or internal reconfigure re-reports an unchanged descriptor
        if (i.getDescriptorChanged() && !st.getDomainDescriptorChanged() && r.previous.assigned())
            for (const auto& p : r.previous.getInputs())
                if (slotOf(p.getInput()) == slot && p.getDescriptor().assigned())
                    EXPECT_FALSE(BaseObjectPtr::Equals(p.getDescriptor(), i.getDescriptor()))
                        << "R-API-28: in" << slot << "'s descriptor is new since the previous status";
        const auto err = e.errors.find(slot);
        if (err != e.errors.end())
            EXPECT_EQ(i.getError(), err->second) << "error of in" << slot;
        const auto size = e.sizes.find(slot);
        if (size != e.sizes.end())
            EXPECT_EQ(i.getSampleSize(), size->second) << "sample size of in" << slot;
    }
    std::set<int> named = e.descChangedSlots;
    for (const auto& [slot, err] : e.errors)
        named.insert(slot);
    for (const auto& [slot, bytes] : e.sizes)
        named.insert(slot);
    for (int slot : named)
        if (std::none_of(st.getInputs().begin(), st.getInputs().end(), [&](const auto& i) { return slotOf(i.getInput()) == slot; }))
            ADD_FAILURE() << "in" << slot << " is not on the status";

    if (e.mainSlot)
    {
        if (*e.mainSlot)
            EXPECT_EQ(mainSlot, **e.mainSlot) << "main input";
        else
            EXPECT_FALSE(st.getMainInput().assigned()) << "main input is null";
    }
    if (e.message)
        EXPECT_EQ(st.getStatusMessage().toStdString(), *e.message) << "status message";
    if (e.inputSlots)
    {
        std::vector<int> slots;
        for (const auto& i : st.getInputs())
            slots.push_back(slotOf(i.getInput()));
        EXPECT_EQ(slots, *e.inputSlots) << "inputs";
    }
}

void MultiReader2Test::checkBuffers(const Result& r, const Expect& e)
{
    const auto& prev = r.previous;
    const SizeT n = r.count;
    const bool copies = r.kind != Result::Kind::Skip && !r.args.nullData;

    std::optional<TickDomain> mainDom;
    int64_t mainStart = 0;
    int64_t mainDelta = 1;
    if (prev.assigned() && prev.getDomainDescriptor().assigned())
    {
        mainDom = TickDomain::from(prev.getDomainDescriptor());
        if (mainDom)
        {
            mainStart = mainDom->start;
            mainDelta = mainDom->delta;
        }
    }

    if (e.firstTick && n > 0)
    {
        if (r.kind == Result::Kind::ReadWithDomain)
        {
            int64_t first;
            std::memcpy(&first, r.domainBuffer.data(), sizeof first);
            EXPECT_EQ(first, *e.firstTick) << "first tick in the domain buffer";
        }
        else
            EXPECT_EQ(static_cast<int64_t>(r.packetOffset) + mainStart, *e.firstTick) << "packetOffset plus the main rule's start";
    }
    if (r.kind == Result::Kind::ReadWithDomain && e.firstTick && copies)
        for (SizeT i = 0; i < n; ++i)
        {
            int64_t tick;
            std::memcpy(&tick, r.domainBuffer.data() + i * sizeof tick, sizeof tick);
            EXPECT_EQ(tick, *e.firstTick + static_cast<int64_t>(i) * mainDelta) << "domain buffer sample " << i;
        }
    if (r.kind == Result::Kind::ReadWithDomain && copies)
    {
        const SizeT ticked = e.firstTick ? n * sizeof(int64_t) : 0;
        const auto stray = std::find_if(r.domainBuffer.begin() + ticked, r.domainBuffer.end(), [](uint8_t v) { return v != Sentinel; });
        EXPECT_TRUE(stray == r.domainBuffer.end()) << "R-BUF-1: the domain buffer is written only for the samples read";
    }
    if (!copies)
        return;

    const auto readType = paramsInForce.getValueReadType();
    std::set<int> valued;
    for (SizeT b = 0; b < r.buffers.size(); ++b)
    {
        const int slot = r.slots[b];
        const auto& buffer = r.buffers[b];
        if (r.args.nullBuffer && r.args.nullBuffer->slot == slot)
            continue;
        MultiReader2InputStatusPtr prevInput;
        if (prev.assigned() && prev.getStatus() != Error)
            for (const auto& i : prev.getInputs())
                if (slotOf(i.getInput()) == slot && i.getError() == InputError::None)
                    prevInput = i;

        SizeT filled = 0;
        if (prevInput.assigned() && n > 0 && e.firstTick)
        {
            const auto desc = prevInput.getDescriptor();
            const SampleType type = readType == SampleType::Undefined ? desc.getSampleType() : readType;
            const SizeT elements = elementCount(desc);
            const bool isStruct = type == SampleType::Struct;
            const SizeT sampleSize = isStruct ? desc.getRawSampleSize() * 1 : elements * typeSize(type);
            filled = n * sampleSize;
            valued.insert(slot);
            const auto& written = input(slot).written;  // the samples may come from a signal the port no longer carries
            const auto overrideIt = e.valueOverrides.find(slot);
            double scale = 1, offset = 0;
            if (desc.getPostScaling().assigned())
            {
                const auto params = desc.getPostScaling().getParameters();
                scale = static_cast<NumberPtr>(params.get("scale")).getFloatValue();
                offset = static_cast<NumberPtr>(params.get("offset")).getFloatValue();
            }
            for (SizeT i = 0; i < n && filled <= buffer.size(); ++i)
            {
                const int64_t readerTick = *e.firstTick + static_cast<int64_t>(i) * mainDelta;
                const int64_t refTick = mainDom ? config->reference.tickAt(mainDom->timeOf(readerTick)) : readerTick;
                double expected;
                if (overrideIt != e.valueOverrides.end())
                    expected = overrideIt->second(refTick);
                else
                {
                    if (!written.count(refTick))
                    {
                        ADD_FAILURE() << "in" << slot << ": nothing was written at tick " << refTick;
                        break;
                    }
                    expected = written.at(refTick) * scale + offset;
                }
                const uint8_t* sample = buffer.data() + i * sampleSize;
                if (isStruct)
                {
                    const uint8_t* p = sample;
                    for (SizeT el = 0; el < elements; ++el)
                        for (const auto& field : desc.getStructFields())
                        {
                            const auto ft = field.getSampleType();
                            std::vector<uint8_t> want(typeSize(ft));
                            writeElement(want.data(), ft, expected);
                            EXPECT_TRUE(sameValue(readElement(p, ft), readElement(want.data(), ft)))
                                << "in" << slot << " sample " << i << " struct field";
                            p += typeSize(ft);
                        }
                    continue;
                }
                std::vector<uint8_t> want(typeSize(type));
                writeElement(want.data(), type, expected);
                for (SizeT el = 0; el < elements; ++el)
                {
                    const double got = readElement(sample + el * typeSize(type), type);
                    if (!sameValue(got, readElement(want.data(), type)))
                    {
                        ADD_FAILURE() << "in" << slot << " sample " << i << " element " << el << ": got " << got << ", expected "
                                      << readElement(want.data(), type);
                        break;
                    }
                }
            }
        }
        const auto untouched =
            std::find_if(buffer.begin() + std::min(filled, buffer.size()), buffer.end(), [](uint8_t v) { return v != Sentinel; });
        EXPECT_TRUE(untouched == buffer.end()) << "R-BUF-1: in" << slot << "'s buffer is written only for the samples read";
    }
    for (const auto& [slot, value] : e.valueOverrides)
        if (!valued.count(slot))
            ADD_FAILURE() << "in" << slot << "'s values are listed, but its buffer holds no samples to check";
}

void MultiReader2Test::expectPort(Input in, Listener listener, PacketReadyNotification mode)
{
    const auto state = portState(in.slot);
    EXPECT_EQ(state.first, listener) << "listener of in" << in.slot;
    EXPECT_EQ(state.second, mode) << "notification method of in" << in.slot;
}

void MultiReader2Test::expectPort(Input in, Listener listener)
{
    EXPECT_EQ(portState(in.slot).first, listener) << "listener of in" << in.slot;
}

std::pair<Listener, PacketReadyNotification> MultiReader2Test::portState(int slot)
{
    reader();  // a CP1 row asks the ports of the constructed reader
    auto& i = input(slot);
    if (!i.port.assigned())
        return {Listener::None, PacketReadyNotification::None};
    const auto listener = i.port.getListener();
    Listener kind = Listener::Other;
    if (!listener.assigned())
        kind = Listener::None;
    else if (readerListener.assigned() && BaseObjectPtr::Equals(listener, readerListener))
        kind = Listener::Reader;
    else if (i.blockListener.assigned() && BaseObjectPtr::Equals(listener, i.blockListener))
        kind = Listener::Block;
    else if (i.otherListener.assigned() && BaseObjectPtr::Equals(listener, i.otherListener))
        kind = Listener::Other;
    return {kind, i.port.getNotificationMethod()};
}

void MultiReader2Test::noteReaderListener()
{
    if (readerListener.assigned())
        return;
    for (int slot : slotsInForce())
        if (slot > 0 && input(slot).port.assigned() && input(slot).port.getListener().assigned())
        {
            readerListener = input(slot).port.getListener();
            return;
        }
}

MultiReader2Test::Snapshot MultiReader2Test::snapshot()
{
    Snapshot s;
    SizeT n = 0;
    s.availError = reader().assigned() ? reader()->getAvailableCount(&n) : OPENDAQ_ERR_INVALIDSTATE;
    daqClearErrorInfo();
    s.avail = n;
    for (const auto& [slot, in] : inputs)
        if (in->port.assigned())
            s.ports[slot] = portState(slot);
    return s;
}

bool MultiReader2Test::Snapshot::operator==(const Snapshot& other) const
{
    if (availError != other.availError || avail != other.avail)
        return false;
    for (const auto& [slot, state] : ports)
    {
        const auto it = other.ports.find(slot);
        if (it != other.ports.end() && it->second != state)
            return false;
    }
    return true;
}

void MultiReader2Test::expectDestroyed()
{
    EXPECT_FALSE(readerRef.getRef().assigned()) << "the reader is destroyed";
}

void MultiReader2Test::expectStubSaw(ErrCode error)
{
    std::scoped_lock lock(stub->mutex);
    ASSERT_FALSE(stubSaw.empty()) << "the acceptsDescriptor stub did not call the reader";
    EXPECT_EQ(stubSaw.back(), error);
    stubSaw.clear();
}

void MultiReader2Test::expectLoggedError()
{
    expectLoggedErrors(1);
}

void MultiReader2Test::expectLoggedErrors(SizeT n)
{
    const SizeT seen = countingSink(errorSink).waitForAtLeast(errorsBeforeIdle + n);
    EXPECT_EQ(seen - errorsBeforeIdle, n) << "error entries logged by the handler calls since the previous idle";
}

// ---------------------------------------------------------------------------------------------------------------------
// Suite 2 owner

#define MR2_OWNER_REACTS(Name, Body)      \
    OwnerReacts OwnerReacts::Name() const \
    {                                     \
        OwnerReacts r = *this;            \
        Body;                             \
        return r;                         \
    }

MR2_OWNER_REACTS(cachesDescriptors, r.cache = true)
MR2_OWNER_REACTS(cachesUnlessError, r.cache = true; r.cacheUnderError = false)
MR2_OWNER_REACTS(setsComponentStatus, r.componentStatus = true)
MR2_OWNER_REACTS(warnsWhenOnlyDisconnected, r.componentStatus = true; r.warnDisconnected = true)
MR2_OWNER_REACTS(removesDisconnectedPorts, r.removeDisconnected = true)

#undef MR2_OWNER_REACTS

void MultiReader2Test::ownerReacts(const OwnerReacts& reacts)
{
    std::scoped_lock lock(owner->mutex);
    owner->reacts = reacts;
}

void MultiReader2Test::offerFreePort(Input in)
{
    ASSERT_TRUE(config->ports) << "a free port needs the ports fixture";
    ASSERT_EQ(inputs.count(in.slot), 0u) << "in" << in.slot << " exists";
    config->unconnected.insert(in.slot);
    config->blockListener[in.slot] = PacketReadyNotification::Scheduler;
    input(in.slot);
}

void MultiReader2Test::addPort(Input in)
{
    ASSERT_TRUE(config->ports) << "a port needs the ports fixture";
    ASSERT_EQ(inputs.count(in.slot), 0u) << "in" << in.slot << " exists";
    config->unconnected.insert(in.slot);
    input(in.slot);
}

void MultiReader2Test::removePort(Input in)
{
    auto& i = input(in.slot);
    ASSERT_TRUE(i.port.assigned()) << "removePort needs an input port";
    if (i.signal)
        i.port.disconnect();
    i.signal = nullptr;
    i.port.asPtr<IRemovable>().remove();
}

void MultiReader2Test::pauseRecording()
{
    owner->paused = true;
}

void MultiReader2Test::resumeRecording()
{
    owner->paused = false;
}

void MultiReader2Test::recorderDrain()
{
    drain(owner->paused ? ReadCall::Skip : ReadCall::ReadWithDomain);
}

void MultiReader2Test::expectComponentStatus(ComponentStatus level, const std::string& message)
{
    std::scoped_lock lock(owner->mutex);
    EXPECT_EQ(owner->status, level) << "the owner's component status";
    EXPECT_EQ(owner->message, message) << "the owner's component status message";
}

bool MultiReader2Test::acceptsByRule(int slot, const DataDescriptorPtr& desc, const ListPtr<IDataDescriptor>& values)
{
    std::scoped_lock lock(owner->mutex);
    switch (*owner->rule)
    {
        case AcceptsRule::UnitV:
            return unitOf(desc) == "V";
        case AcceptsRule::Power:
            return slot != 1 || unitOf(desc) == "V" || unitOf(desc).empty();
        case AcceptsRule::Sum:
        {
            std::string reference;
            for (const DataDescriptorPtr d : values)
                if (isScalarNumeric(d))
                {
                    reference = unitOf(d);
                    break;
                }
            return isScalarNumeric(desc) && unitOf(desc) == reference;
        }
    }
    return true;
}

void MultiReader2Test::ownerOnStatus(const MultiReader2StatusPtr& status)
{
    std::scoped_lock lock(owner->mutex);
    const auto level = status.getStatus();
    if (owner->reacts.cache && (owner->reacts.cacheUnderError || level != Error))
        for (const auto& i : status.getInputs())
            if (i.getDescriptor().assigned())
                owner->cache[slotOf(i.getInput())] = i.getDescriptor();
    if (owner->reacts.componentStatus)
    {
        bool onlyDisconnected = true;
        for (const auto& i : status.getInputs())
            if (i.getError() != InputError::None && i.getError() != InputError::Disconnected)
                onlyDisconnected = false;
        owner->status = level == Error && owner->reacts.warnDisconnected && onlyDisconnected ? Warning : level;
        owner->message = status.getStatusMessage().toStdString();
    }
}

void MultiReader2Test::ownerActs(const MultiReader2StatusPtr& status)
{
    OwnerReacts reacts;
    {
        std::scoped_lock lock(owner->mutex);
        if (owner->acting)
            return;
        owner->acting = true;
        reacts = owner->reacts;
    }
    std::vector<int> disconnected;
    for (const auto& i : status.getInputs())
        if (i.getError() == InputError::Disconnected)
            disconnected.push_back(slotOf(i.getInput()));
    if (reacts.removeDisconnected && !disconnected.empty())
    {
        for (int slot : disconnected)
        {
            removePort(Input{slot});
            configure(same().remove(Input{slot}));
            inputStatus(Input{slot});
        }
    }
    std::scoped_lock lock(owner->mutex);
    owner->acting = false;
}

// ---------------------------------------------------------------------------------------------------------------------
// Suite 3

SizeT MultiReader2Test::handlerCallsStarted()
{
    return handlerState->started;
}

void MultiReader2Test::waitUntilHandlerSaw(Input in, InputError error)
{
    const bool seen = waitForHandler(
        [&]
        {
            std::scoped_lock lock(handlerState->mutex);
            for (const auto& r : handlerState->log.results)
                if (r.status.assigned())
                    for (const auto& i : r.status.getInputs())
                        if (slotOf(i.getInput()) == in.slot && i.getError() == error)
                            return true;
            return false;
        });
    EXPECT_TRUE(seen) << "the handler saw in" << in.slot << " in error";
}

bool MultiReader2Test::waitForHandler(const std::function<bool()>& condition)
{
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (handlerState->subscribed && std::chrono::steady_clock::now() < deadline)
    {
        if (condition())
            return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return condition();
}

bool MultiReader2Test::handlerRunning()
{
    return handlerState->running > 0;
}

void MultiReader2Test::holdNextHandlerCall()
{
    handlerState->holdNext = true;
}

void MultiReader2Test::waitForHeldHandlerCall()
{
    std::unique_lock lock(handlerState->mutex);
    if (!handlerState->heldCv.wait_for(lock, std::chrono::seconds(5), [this] { return handlerState->held; }))
        ADD_FAILURE() << "no handler call parked within 5 s";
}

void MultiReader2Test::releaseHandlerCall()
{
    {
        std::scoped_lock lock(handlerState->mutex);
        handlerState->held = false;
    }
    handlerState->heldCv.notify_all();
}

Script MultiReader2Test::eachInputWrites(int64_t first, int64_t last)
{
    reader();
    Script s;
    for (int slot : slotsInForce())
        s.push_back({Input{slot}, sendWrite(Input{slot}, first, last)});
    return s;
}

int64_t MultiReader2Test::random(int64_t low, int64_t high)
{
    if (!rng)
    {
        // MR2_SEED replays a recorded run; otherwise gtest's seed when one is set (--gtest_random_seed, --gtest_shuffle)
        const char* fixed = std::getenv("MR2_SEED");
        const auto gtestSeed = static_cast<uint64_t>(testing::UnitTest::GetInstance()->random_seed());
        const uint64_t seed = fixed ? std::stoull(fixed) : gtestSeed != 0 ? gtestSeed : std::random_device{}();
        RecordProperty("seed", std::to_string(seed));
        recordedSeed = seed;
        rng.emplace(seed);
    }
    return std::uniform_int_distribution<int64_t>(low, high)(*rng);
}

void MultiReader2Test::pause(int64_t ms)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

void MultiReader2Test::restart(SizeT runWorkers)
{
    stopEndlessWriters();
    if (workerHeld)
        releaseWorker();
    if (handlerState)
        releaseHandlerCall();
    readerObject.release();
    otherReaders.clear();
    holders.clear();
    if (scheduler.assigned())
    {
        scheduler.waitAll();
        scheduler.stop();
    }
    inputs.clear();
    spares.clear();
    handlerState = std::make_unique<Handler>();
    {
        std::scoped_lock lock(stub->mutex);
        stub->asked.clear();
        stubSaw.clear();
    }
    {
        std::scoped_lock lock(owner->mutex);
        owner->cache.clear();
    }
    constructed = false;
    readerRef = nullptr;
    readerListener = nullptr;
    paramsInForce = nullptr;
    paramsObject = nullptr;
    previousStatus = nullptr;
    lastResult = Result{Result::Kind::Avail};
    scheduler = nullptr;
    context = nullptr;
    contextWithoutScheduler = nullptr;
    workers = runWorkers;
    errorsAtIdle = errorsBeforeIdle = countingSink(errorSink).count();
}

CaseRun MultiReader2Test::runCase(SizeT runWorkers, bool concurrent, const std::function<void(bool)>& body)
{
    restart(runWorkers);
    Watchdog watchdog(recordedSeed);
    statusInvariants = false;
    muted = true;
    CaseRun run;
    {
        std::scoped_lock lock(runMutex);
        runLog.clear();
    }
    recordingRun = true;
    body(concurrent);
    flush();
    idle();
    if (reader().assigned())
    {
        run.availError = reader()->getAvailableCount(&run.availAfterIdle);
        daqClearErrorInfo();
        const auto final = drain();
        if (!final.empty())
            run.finalRead = final.front();
    }
    recordingRun = false;
    {
        std::scoped_lock lock(runMutex);
        run.reads = std::move(runLog);
        runLog.clear();
    }
    for (const auto& r : run.reads)
        if (r.status.assigned())
            run.last = r.status;
    run.maxOverlap = handlerState->maxRunning;
    run.values = delivered(run.reads, paramsInForce.assigned() ? paramsInForce.getValueReadType() : SampleType::Float64, false, false);
    muted = false;
    return run;
}

void MultiReader2Test::replay(Timeline tl, unsigned invariants)
{
    referenceDelivers = tl != Timeline::Apart && tl != Timeline::Starve1 && tl != Timeline::StarveAll && tl != Timeline::StarveMain &&
                        tl != Timeline::Short;
    replay([this, tl] { return script(tl); }, invariants);
}

void MultiReader2Test::replay(const std::function<Script()>& s, unsigned invariants)
{
    replay(
        [this, s](bool concurrent)
        {
            handler([this] { drain(); });
            runScript(s(), concurrent);
        },
        invariants);
}

void MultiReader2Test::replay(const std::function<void(bool)>& body, unsigned invariants)
{
    const CaseRun reference = runCase(1, false, body);
    const CaseRun run = runCase(fixtureWorkers, true, body);
    checkReplay(reference, run, invariants);
}

void MultiReader2Test::runConcurrently(const std::function<void(bool)>& body, unsigned invariants)
{
    const CaseRun run = runCase(fixtureWorkers, true, body);
    checkReplay(run, run, invariants & ~(INV4 | INV5 | INV9));
}

// The samples each input delivered, in read order. INV1 holds on the way: values rise except on the first delivering
// read after a status with resync. INV10, where values are ticks: a read delivers the same ticks on every input, which
// holds however the ports' packets interleave
std::map<int, std::vector<double>> MultiReader2Test::delivered(const std::vector<Result>& reads,
                                                               SampleType readType,
                                                               bool checkInv1,
                                                               bool checkInv10)
{
    checkInv10 = checkInv10 && config->values == Config::Values::Tick;
    std::map<int, std::vector<double>> values;
    std::map<int, double> lastValue;
    bool resyncPending = false;
    for (const auto& r : reads)
    {
        const bool copies = r.kind != Result::Kind::Skip && !r.args.nullData && OPENDAQ_SUCCEEDED(r.error);
        if (copies && r.count > 0 && r.previous.assigned() && r.previous.getStatus() != Error)
        {
            if (resyncPending)
                lastValue.clear();
            resyncPending = false;
            std::optional<std::vector<double>> aligned;
            for (SizeT b = 0; b < r.buffers.size(); ++b)
            {
                const int slot = r.slots[b];
                MultiReader2InputStatusPtr prevInput;
                for (const auto& i : r.previous.getInputs())
                    if (slotOf(i.getInput()) == slot && i.getError() == InputError::None)
                        prevInput = i;
                if (!prevInput.assigned())
                    continue;
                const auto desc = prevInput.getDescriptor();
                const SampleType type = readType == SampleType::Undefined ? desc.getSampleType() : readType;
                const SizeT size = elementCount(desc) * typeSize(type);
                std::vector<double> got;
                for (SizeT k = 0; k < r.count && (k + 1) * size <= r.buffers[b].size(); ++k)
                {
                    const double v = readElement(r.buffers[b].data() + k * size, type);
                    const auto last = lastValue.find(slot);
                    if (checkInv1 && last != lastValue.end())
                        EXPECT_GT(v, last->second) << "INV1: in" << slot << "'s samples arrive once and in tick order";
                    lastValue[slot] = v;
                    values[slot].push_back(v);
                    got.push_back(v);
                }
                if (checkInv10 && aligned)
                    EXPECT_EQ(got, *aligned) << "INV10: in" << slot << " delivers the ticks of the read's other inputs";
                else if (checkInv10)
                    aligned = std::move(got);
            }
        }
        if (r.status.assigned() && r.status.getResynchronized())
            resyncPending = true;
    }
    return values;
}

namespace
{
int64_t firstTickOf(const Result& r, int64_t start)
{
    if (r.kind == Result::Kind::ReadWithDomain)
    {
        int64_t tick;
        std::memcpy(&tick, r.domainBuffer.data(), sizeof tick);
        return tick;
    }
    return static_cast<int64_t>(r.packetOffset) + start;
}

struct FinalState
{
    ComponentStatus level;
    std::string message;
    int main;
    DataDescriptorPtr domain;
    std::vector<std::tuple<int, InputError, DataDescriptorPtr, SizeT>> inputs;
};
}

void MultiReader2Test::checkReplay(const CaseRun& reference, const CaseRun& run, unsigned inv)
{
    const SampleType readType = paramsInForce.assigned() ? paramsInForce.getValueReadType() : SampleType::Float64;
    for (const auto& r : run.reads)
        if (OPENDAQ_FAILED(r.error))
        {
            ADD_FAILURE() << "a reading call of the run failed: " << r.step << " returned " << errorName(r.error);
            break;
        }
    const auto values = delivered(run.reads, readType, inv & INV1, inv & INV10);

    if (inv & INV2)
    {
        bool resyncPending = true;
        int64_t next = 0;
        for (const auto& r : run.reads)
        {
            if (OPENDAQ_SUCCEEDED(r.error) && r.count > 0)
            {
                int64_t start = 0, delta = 1;
                if (r.previous.assigned() && r.previous.getDomainDescriptor().assigned())
                    if (const auto d = TickDomain::from(r.previous.getDomainDescriptor()))
                    {
                        start = d->start;
                        delta = d->delta;
                    }
                const int64_t first = firstTickOf(r, start);
                if (!resyncPending)
                    EXPECT_EQ(first, next) << "INV2: packetOffset continues the previous read";
                resyncPending = false;
                next = first + static_cast<int64_t>(r.count) * delta;
            }
            if (r.status.assigned() && r.status.getResynchronized())
                resyncPending = true;
        }
    }

    if ((inv & INV3) && run.last.assigned() && run.last.getStatus() != Error)
    {
        for (const auto& i : run.last.getInputs())
        {
            const int slot = slotOf(i.getInput());
            if (i.getError() == InputError::None && input(slot).signal)
                EXPECT_TRUE(BaseObjectPtr::Equals(i.getDescriptor(), input(slot).signal->valueDesc))
                    << "INV3: in" << slot << "'s descriptor is the last one written";
        }
        const int mainSlot = slotOf(run.last.getMainInput());
        if (mainSlot > 0 && input(mainSlot).signal)
            EXPECT_TRUE(BaseObjectPtr::Equals(run.last.getDomainDescriptor(), input(mainSlot).signal->domainDesc))
                << "INV3: the main domain descriptor is the last one written";
    }

    if (inv & INV4)
    {
        EXPECT_EQ(run.availError, OPENDAQ_SUCCESS) << "INV4: avail after idle returned " << errorName(run.availError);
        EXPECT_EQ(run.availAfterIdle, 0u) << "INV4: nothing is left to read once idle returns";
        ASSERT_TRUE(run.finalRead.has_value()) << "INV4: the final drain read";
        EXPECT_EQ(run.finalRead->count, 0u) << "INV4: the final drain's first read returns 0";
        if (run.finalRead->status.assigned() && reference.finalRead && reference.finalRead->status.assigned())
            EXPECT_EQ(run.finalRead->status.getHasChanges(), reference.finalRead->status.getHasChanges())
                << "INV4: the final drain's first read has changes as in the reference run";
    }

    if (inv & INV5)
    {
        const auto stateOf = [this](const MultiReader2StatusPtr& s)
        {
            FinalState st{s.getStatus(), s.getStatusMessage().toStdString(), slotOf(s.getMainInput()), s.getDomainDescriptor(), {}};
            for (const auto& i : s.getInputs())
                st.inputs.emplace_back(slotOf(i.getInput()), i.getError(), i.getDescriptor(), i.getSampleSize());
            return st;
        };
        ASSERT_TRUE(run.last.assigned() && reference.last.assigned()) << "INV5: both runs end with a status";
        const auto a = stateOf(run.last);
        // the reference ran on its own inputs, which map to the same slots by local ID
        FinalState b{
            reference.last.getStatus(), reference.last.getStatusMessage().toStdString(), 0, reference.last.getDomainDescriptor(), {}};
        b.main =
            reference.last.getMainInput().assigned() ? std::stoi(reference.last.getMainInput().getLocalId().toStdString().substr(2)) : 0;
        for (const auto& i : reference.last.getInputs())
            b.inputs.emplace_back(
                std::stoi(i.getInput().getLocalId().toStdString().substr(2)), i.getError(), i.getDescriptor(), i.getSampleSize());
        EXPECT_EQ(a.level, b.level) << "INV5: level";
        EXPECT_EQ(a.message, b.message) << "INV5: message";
        EXPECT_EQ(a.main, b.main) << "INV5: main input";
        EXPECT_TRUE(BaseObjectPtr::Equals(a.domain, b.domain)) << "INV5: domain descriptor";
        ASSERT_EQ(a.inputs.size(), b.inputs.size()) << "INV5: inputs";
        for (SizeT k = 0; k < a.inputs.size(); ++k)
        {
            EXPECT_EQ(std::get<0>(a.inputs[k]), std::get<0>(b.inputs[k])) << "INV5: input slot";
            EXPECT_EQ(std::get<1>(a.inputs[k]), std::get<1>(b.inputs[k])) << "INV5: error of in" << std::get<0>(a.inputs[k]);
            EXPECT_TRUE(BaseObjectPtr::Equals(std::get<2>(a.inputs[k]), std::get<2>(b.inputs[k])))
                << "INV5: descriptor of in" << std::get<0>(a.inputs[k]);
            EXPECT_EQ(std::get<3>(a.inputs[k]), std::get<3>(b.inputs[k])) << "INV5: sample size of in" << std::get<0>(a.inputs[k]);
        }
    }

    if (inv & INV8)
        EXPECT_LE(run.maxOverlap, 1) << "INV8: handler calls never overlap";

    if (inv & INV9)
    {
        // The reference run's inputs are gone by now, so its samples were taken when it ended
        EXPECT_EQ(values, reference.values) << "INV9: each input delivers the samples of the reference run";
        // A reference that delivered nothing would make the comparison above pass on any run
        if (referenceDelivers && reference.last.assigned())
            for (const auto& i : reference.last.getInputs())
                if (i.getError() == InputError::None)
                {
                    const int slot = std::stoi(i.getInput().getLocalId().toStdString().substr(2));
                    const auto it = reference.values.find(slot);
                    EXPECT_TRUE(it != reference.values.end() && !it->second.empty())
                        << "INV9: the reference run delivered nothing on in" << slot;
                }
    }
}

void MultiReader2Test::expectReleasedCleanly(SizeT callsAtRelease, SizeT inFlight)
{
    flush();
    EXPECT_EQ(handlerState->startedWithoutReader.load(), 0u) << "INV6: no handler call runs after the reader was destroyed";
    // A call that took the reader before the release is counted when it enters the body, after the release (R-THR-2)
    EXPECT_LE(handlerCallsStarted(), callsAtRelease + inFlight)
        << "INV6: no handler call starts after the last release but those in flight";
    expectDestroyed();
    for (const auto& [slot, in] : inputs)
        if (in->port.assigned())
        {
            const auto state = portState(slot);
            EXPECT_EQ(state.first, Listener::None) << "INV6: in" << slot << " has no listener";
            EXPECT_EQ(state.second, PacketReadyNotification::None) << "INV6: in" << slot << " has notification method None";
        }
        else  // the port the reader created for a signal input is disconnected with the reader
            EXPECT_EQ(in->own->value.getConnections().getCount(), 0u) << "INV6: in" << slot << "'s signal has no connection left";
}

Result EachReadingCall::reading(SizeT count, const ReadArgs& args)
{
    return readingCall(GetParam(), count, args);
}

}
