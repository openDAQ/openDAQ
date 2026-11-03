// The fixture's own state, which the harness files share

#pragma once
#include "multi_reader2_harness.h"
#include "multi_reader2_harness_detail.h"

#include <opendaq/signal_impl.h>

namespace daq::mr2test
{

namespace detail
{

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

}

using namespace detail;

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
    bool fifo = false;
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
    std::string referenceUnit;  // AcceptsRule::Sum: the main input's unit; empty accepts every unit
    bool rejudge = false;       // the reference moved: the owner has every input judged again
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

}
