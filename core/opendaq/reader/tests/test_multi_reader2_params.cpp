/*
 * MultiReader2 params: every getter and setter, what each validates and the error it returns.
 */
#include "test_multi_reader2_common.h"

#include <opendaq/folder_factory.h>
#include <opendaq/multi_reader2_factory.h>

using namespace daq;
using namespace testing;

class MultiReader2ParamsTest : public MultiReader2Test
{
public:
    static MultiReader2ParamsPtr freshParams()
    {
        return createWithImplementation<IMultiReader2Params, MultiReader2ParamsImpl>();
    }

    void addSignals(SizeT count)
    {
        readSignals.reserve(count);
        for (SizeT i = 0; i < count; i++)
            addSignal(0, 10, createDomainSignal());
    }
};

// ---------------------------------------------------------------- defaults and factory

TEST_F(MultiReader2ParamsTest, FactoryCreatesDefaults)
{
    auto p = MultiReader2Params();
    ASSERT_TRUE(p.getInputs().assigned());
    ASSERT_EQ(p.getInputs().getCount(), 0u);
    ASSERT_FALSE(p.getMainInput().assigned());
    ASSERT_FALSE(p.getAcceptsDescriptor().assigned());
    ASSERT_EQ(p.getValueReadType(), SampleType::Float64);
    ASSERT_EQ(p.getMinReadCount(), 1u);
    ASSERT_EQ(p.getErrorPolicy(), MultiReader2ErrorPolicy::Invalidate);
}

TEST_F(MultiReader2ParamsTest, GettersRejectNullOutput)
{
    addSignals(1);
    auto p = freshParams();
    p.setInputs(signalsToList());
    ASSERT_EQ(p->getInputs(nullptr), OPENDAQ_ERR_ARGUMENT_NULL);
    daqClearErrorInfo();
    ASSERT_EQ(p->getMainInput(nullptr), OPENDAQ_ERR_ARGUMENT_NULL);
    daqClearErrorInfo();
    ASSERT_EQ(p->getValueReadType(nullptr), OPENDAQ_ERR_ARGUMENT_NULL);
    daqClearErrorInfo();
    ASSERT_EQ(p->getMinReadCount(nullptr), OPENDAQ_ERR_ARGUMENT_NULL);
    daqClearErrorInfo();
    ASSERT_EQ(p->getErrorPolicy(nullptr), OPENDAQ_ERR_ARGUMENT_NULL);
    daqClearErrorInfo();
    ASSERT_EQ(p->getAcceptsDescriptor(nullptr), OPENDAQ_ERR_ARGUMENT_NULL);
    daqClearErrorInfo();
}

TEST_F(MultiReader2ParamsTest, SettersRejectNullInput)
{
    auto p = freshParams();
    ASSERT_EQ(p->setInputs(nullptr), OPENDAQ_ERR_ARGUMENT_NULL);
    daqClearErrorInfo();
}

// ---------------------------------------------------------------- inputs

TEST_F(MultiReader2ParamsTest, SetInputsKeepsOrderOfSignals)
{
    addSignals(3);
    auto p = freshParams();
    p.setInputs(signalsToList());
    const auto inputs = p.getInputs();
    ASSERT_EQ(inputs.getCount(), 3u);
    for (SizeT i = 0; i < 3; i++)
        ASSERT_EQ(inputs[i].getGlobalId(), readSignals[i].signal.getGlobalId());
}

TEST_F(MultiReader2ParamsTest, SetInputsKeepsOrderOfPorts)
{
    addSignals(3);
    auto p = freshParams();
    p.setInputs(portsList());
    const auto inputs = p.getInputs();
    ASSERT_EQ(inputs.getCount(), 3u);
    for (SizeT i = 0; i < 3; i++)
        ASSERT_EQ(inputs[i].getGlobalId(), ports[i].getGlobalId());
}

TEST_F(MultiReader2ParamsTest, SingleInputIsAccepted)
{
    addSignals(1);
    auto p = freshParams();
    ASSERT_EQ(p->setInputs(signalsToList()), OPENDAQ_SUCCESS);
    ASSERT_EQ(p.getInputs().getCount(), 1u);
}

TEST_F(MultiReader2ParamsTest, LargeInputListIsAccepted)
{
    addSignals(64);
    auto p = freshParams();
    ASSERT_EQ(p->setInputs(signalsToList()), OPENDAQ_SUCCESS);
    ASSERT_EQ(p.getInputs().getCount(), 64u);
}

TEST_F(MultiReader2ParamsTest, SetInputsAcceptsAnEmptyList)
{
    auto p = freshParams();
    ASSERT_EQ(p->setInputs(List<IComponent>()), OPENDAQ_SUCCESS);
    ASSERT_EQ(p.getInputs().getCount(), 0u);
}

TEST_F(MultiReader2ParamsTest, SetInputsRejectsNullElement)
{
    addSignals(1);
    auto list = List<IComponent>();
    list.pushBack(readSignals[0].signal);
    list.pushBack(ComponentPtr());
    auto p = freshParams();
    ASSERT_EQ(p->setInputs(list), OPENDAQ_ERR_INVALIDPARAMETER);
    daqClearErrorInfo();
}

TEST_F(MultiReader2ParamsTest, SetInputsRejectsComponentThatIsNeitherSignalNorPort)
{
    addSignals(1);
    auto folder = Folder(context, nullptr, "folder");
    auto p = freshParams();
    ASSERT_EQ(p->setInputs(List<IComponent>(folder)), OPENDAQ_ERR_INVALIDPARAMETER);
    daqClearErrorInfo();
    ASSERT_EQ(p->setInputs(List<IComponent>(readSignals[0].signal, folder)), OPENDAQ_ERR_INVALIDPARAMETER);
    daqClearErrorInfo();
}

TEST_F(MultiReader2ParamsTest, SetInputsRejectsPortAfterSignal)
{
    addSignals(2);
    portsList();
    auto p = freshParams();
    ASSERT_EQ(p->setInputs(List<IComponent>(readSignals[0].signal, ports[1])), OPENDAQ_ERR_INVALIDPARAMETER);
    daqClearErrorInfo();
}

TEST_F(MultiReader2ParamsTest, SetInputsRejectsSignalAfterPort)
{
    addSignals(2);
    portsList();
    auto p = freshParams();
    ASSERT_EQ(p->setInputs(List<IComponent>(ports[0], readSignals[1].signal)), OPENDAQ_ERR_INVALIDPARAMETER);
    daqClearErrorInfo();
}

TEST_F(MultiReader2ParamsTest, SetInputsRejectsDuplicateSignal)
{
    addSignals(2);
    auto p = freshParams();
    ASSERT_EQ(p->setInputs(List<IComponent>(readSignals[0].signal, readSignals[1].signal, readSignals[0].signal)),
              OPENDAQ_ERR_DUPLICATEITEM);
    daqClearErrorInfo();
}

TEST_F(MultiReader2ParamsTest, SetInputsRejectsDuplicatePort)
{
    addSignals(2);
    portsList();
    auto p = freshParams();
    ASSERT_EQ(p->setInputs(List<IComponent>(ports[0], ports[0])), OPENDAQ_ERR_DUPLICATEITEM);
    daqClearErrorInfo();
}

TEST_F(MultiReader2ParamsTest, RejectedSetInputsKeepsThePreviousList)
{
    addSignals(2);
    auto p = freshParams();
    p.setInputs(List<IComponent>(readSignals[0].signal));
    ASSERT_EQ(p->setInputs(List<IComponent>(readSignals[1].signal, readSignals[1].signal)), OPENDAQ_ERR_DUPLICATEITEM);
    daqClearErrorInfo();
    ASSERT_EQ(p.getInputs().getCount(), 1u);
    ASSERT_EQ(p.getInputs()[0].getGlobalId(), readSignals[0].signal.getGlobalId());
}

// ---------------------------------------------------------------- the judgement

TEST_F(MultiReader2ParamsTest, AcceptsDescriptorIsNullByDefaultAndStoredAsGiven)
{
    auto p = freshParams();
    ASSERT_FALSE(p.getAcceptsDescriptor().assigned());
    const auto judgement = rejectingAll();
    ASSERT_EQ(p->setAcceptsDescriptor(judgement), OPENDAQ_SUCCESS);
    ASSERT_EQ(p.getAcceptsDescriptor(), judgement);
    ASSERT_EQ(p->setAcceptsDescriptor(nullptr), OPENDAQ_SUCCESS);
    ASSERT_FALSE(p.getAcceptsDescriptor().assigned());
}

TEST_F(MultiReader2ParamsTest, JudgementIsCalledWithTheInputAndBothDescriptors)
{
    addSignals(2);
    std::vector<std::string> seen;
    auto p = params(signalsToList());
    p.setAcceptsDescriptor(Function(
        [&seen](ComponentPtr input, DataDescriptorPtr value, DataDescriptorPtr domain) -> bool
        {
            seen.push_back(input.getGlobalId().toStdString() + " " + std::to_string(static_cast<int>(value.getSampleType())) + " " +
                           (domain.assigned() ? domain.getOrigin().toStdString() : "no domain"));
            return true;
        }));
    auto reader = createReader(p);
    scheduler.waitAll();
    auto status = probe(reader);
    ASSERT_TRUE(status.getValid());

    // Once per input at the configure, with the signal's value descriptor and its domain descriptor
    ASSERT_EQ(seen.size(), 2u);
    for (SizeT i = 0; i < 2; i++)
        ASSERT_EQ(seen[i],
                  readSignals[i].signal.getGlobalId().toStdString() + " " + std::to_string(static_cast<int>(SampleType::Float64)) +
                      " 2022-09-27T00:02:03+00:00");
}

TEST_F(MultiReader2ParamsTest, JudgementIsAskedAgainOnEveryDescriptorChange)
{
    addSignals(2);
    std::atomic<int> calls{0};
    auto p = params(signalsToList());
    p.setErrorPolicy(MultiReader2ErrorPolicy::Exclude);
    // The owner rejects volts
    p.setAcceptsDescriptor(Function(
        [&calls](ComponentPtr, DataDescriptorPtr value, DataDescriptorPtr) -> bool
        {
            calls++;
            return !value.getUnit().assigned() || value.getUnit().getSymbol() != "V";
        }));
    auto reader = createReaderProbed(p);
    ASSERT_EQ(calls.load(), 2);

    readSignals[1].signal.setDescriptor(DataDescriptorBuilder().setSampleType(SampleType::Float64).setUnit(Unit("V")).build());
    scheduler.waitAll();
    auto status = probe(reader);
    ASSERT_EQ(calls.load(), 3);
    ASSERT_TRUE(status.getHasChanges());
    ASSERT_TRUE(status.getValid());
    ASSERT_EQ(input(status, 1).getError(), MultiReader2InputError::ValueDescriptorInvalid);
    ASSERT_FALSE(input(status, 1).getDescriptor().assigned());  // judged and set aside, not reported

    readSignals[1].signal.setDescriptor(setupDescriptor(SampleType::Float64));
    scheduler.waitAll();
    status = probe(reader);
    ASSERT_EQ(calls.load(), 4);
    ASSERT_EQ(input(status, 1).getError(), MultiReader2InputError::None);
    ASSERT_TRUE(input(status, 1).getDescriptorChanged());  // new to the consumer again
}

TEST_F(MultiReader2ParamsTest, RejectedInputUnderExcludeDeliversNothingAndTheOthersStream)
{
    addSignals(3);
    auto domain = readSignals[0].signal.getDomainSignal();
    for (auto& read : readSignals)
        read.signal.setDomainSignal(domain);
    auto p = params(signalsToList());
    p.setErrorPolicy(MultiReader2ErrorPolicy::Exclude);
    p.setAcceptsDescriptor(rejecting({readSignals[1].signal}));
    auto reader = createReaderProbed(p);
    sendPackets(0);
    ASSERT_EQ(reader.getAvailableCount(), 10u);
    std::array<std::array<double, 10>, 3> values{};
    for (auto& v : values)
        v.fill(-1.0);
    SizeT count = 10;
    auto status = read(reader, values, count);
    ASSERT_EQ(count, 10u);
    ASSERT_EQ(values[0][9], 9.0);
    ASSERT_EQ(values[1][0], -1.0);  // untouched
    ASSERT_EQ(values[2][9], 9.0);
    ASSERT_EQ(input(status, 1).getError(), MultiReader2InputError::ValueDescriptorInvalid);
}

TEST_F(MultiReader2ParamsTest, ExceptionInTheJudgementRejects)
{
    addSignals(2);
    auto p = params(signalsToList());
    p.setAcceptsDescriptor(Function(
        [](ComponentPtr input, DataDescriptorPtr, DataDescriptorPtr) -> bool
        {
            if (input.getLocalId() == "sig1")
                throw std::runtime_error("no");
            return true;
        }));
    auto reader = createReader(p);
    scheduler.waitAll();
    auto status = probe(reader);
    ASSERT_FALSE(status.getValid());
    ASSERT_EQ(input(status, 0).getError(), MultiReader2InputError::None);
    ASSERT_EQ(input(status, 1).getError(), MultiReader2InputError::ValueDescriptorInvalid);
}

TEST_F(MultiReader2ParamsTest, AnIntegerVerdictCounts)
{
    addSignals(2);
    auto p = params(signalsToList());
    p.setAcceptsDescriptor(
        Function([](ComponentPtr input, DataDescriptorPtr, DataDescriptorPtr) -> Int { return input.getLocalId() == "sig0" ? 1 : 0; }));
    auto reader = createReader(p);
    scheduler.waitAll();
    auto status = probe(reader);
    ASSERT_FALSE(status.getValid());
    ASSERT_EQ(input(status, 0).getError(), MultiReader2InputError::None);
    ASSERT_EQ(input(status, 1).getError(), MultiReader2InputError::ValueDescriptorInvalid);
}

TEST_F(MultiReader2ParamsTest, ReaderCallsFromInsideTheJudgementFail)
{
    addSignals(2);
    MultiReader2Ptr reader;
    std::vector<ErrCode> seen;
    auto p = params(signalsToList());
    p.setAcceptsDescriptor(Function(
        [&](ComponentPtr, DataDescriptorPtr, DataDescriptorPtr) -> bool
        {
            if (!reader.assigned())
                return true;  // the construction's own configure
            SizeT available = 0;
            seen.push_back(reader->getAvailableCount(&available));
            daqClearErrorInfo();
            IString* id = nullptr;
            seen.push_back(reader->getMainInput(&id));
            daqClearErrorInfo();
            SizeT count = 0;
            IMultiReader2Status* status = nullptr;
            seen.push_back(reader->read(nullptr, &count, nullptr, &status));
            daqClearErrorInfo();
            seen.push_back(reader->configure(p));
            daqClearErrorInfo();
            return true;
        }));
    reader = createReaderProbed(p);

    readSignals[0].signal.setDescriptor(DataDescriptorBuilder().setSampleType(SampleType::Float64).setUnit(Unit("V")).build());
    scheduler.waitAll();
    auto status = probe(reader);
    ASSERT_EQ(seen.size(), 4u);
    for (const auto code : seen)
        ASSERT_EQ(code, OPENDAQ_ERR_INVALIDSTATE);
    // The reader is untouched by the refused calls
    ASSERT_TRUE(status.getValid());
    sendPackets(0);
    ASSERT_EQ(reader.getAvailableCount(), 10u);
}

TEST_F(MultiReader2ParamsTest, RejectedPinnedMainInvalidatesTheReader)
{
    addSignals(2);
    auto p = params(signalsToList());
    p.setMainInput(readSignals[1].signal);
    p.setAcceptsDescriptor(rejecting({readSignals[1].signal}));
    auto reader = createReader(p);
    scheduler.waitAll();
    auto status = probe(reader);
    ASSERT_FALSE(status.getValid());
    ASSERT_EQ(input(status, 1).getError(), MultiReader2InputError::ValueDescriptorInvalid);
    ASSERT_EQ(reader.getMainInput(), readSignals[1].signal.getGlobalId());  // a pin is always named

    p.setAcceptsDescriptor(nullptr);
    reader.configure(p);
    scheduler.waitAll();
    status = probe(reader);
    ASSERT_TRUE(status.getValid());
    sendPackets(0);
    ASSERT_EQ(reader.getAvailableCount(), 10u);
}

TEST_F(MultiReader2ParamsTest, ANewJudgementObjectReconfiguresTheSameOneDoesNot)
{
    addSignals(2);
    auto domain = readSignals[0].signal.getDomainSignal();
    readSignals[1].signal.setDomainSignal(domain);
    auto p = params(signalsToList());
    p.setAcceptsDescriptor(Function([](ComponentPtr, DataDescriptorPtr, DataDescriptorPtr) -> bool { return true; }));
    auto reader = createReaderProbed(p);
    sendPackets(0);
    ASSERT_EQ(reader.getAvailableCount(), 10u);

    ASSERT_EQ(reader->configure(p), OPENDAQ_SUCCESS);
    ASSERT_EQ(reader.getAvailableCount(), 10u);  // the same function object: nothing to do

    p.setAcceptsDescriptor(Function([](ComponentPtr, DataDescriptorPtr, DataDescriptorPtr) -> bool { return true; }));
    ASSERT_EQ(reader->configure(p), OPENDAQ_SUCCESS);
    scheduler.waitAll();
    ASSERT_EQ(reader.getAvailableCount(), 0u);  // another judgement: every descriptor is judged anew
    ASSERT_TRUE(probe(reader).getHasChanges());
}

TEST_F(MultiReader2ParamsTest, EmptyInputListIsAValidReaderWithoutAMain)
{
    addSignals(2);
    auto p = freshParams();
    p.setInputs(List<IComponent>());
    auto reader = createReader(p);
    auto status = probe(reader);
    ASSERT_TRUE(status.getValid());
    ASSERT_TRUE(status.getHasChanges());
    ASSERT_EQ(status.getInputs().getCount(), 0u);
    ASSERT_EQ(reader.getMainInput(), "");
    ASSERT_FALSE(status.getDomainDescriptor().assigned());
    ASSERT_EQ(reader.getAvailableCount(), 0u);

    // Inputs arrive later through configure
    p.setInputs(signalsToList());
    reader.configure(p);
    scheduler.waitAll();
    status = probe(reader);
    ASSERT_TRUE(status.getValid());
    ASSERT_EQ(status.getInputs().getCount(), 2u);
    ASSERT_EQ(reader.getMainInput(), readSignals[0].signal.getGlobalId());
}

// ---------------------------------------------------------------- main input

TEST_F(MultiReader2ParamsTest, SetMainInputAcceptsAnyComponentAndNull)
{
    addSignals(2);
    auto p = freshParams();
    p.setInputs(List<IComponent>(readSignals[0].signal));
    // A main input outside the list is a configure-time error, not a params error
    ASSERT_EQ(p->setMainInput(readSignals[1].signal), OPENDAQ_SUCCESS);
    ASSERT_EQ(p.getMainInput().getGlobalId(), readSignals[1].signal.getGlobalId());
    ASSERT_EQ(p->setMainInput(nullptr), OPENDAQ_SUCCESS);
    ASSERT_FALSE(p.getMainInput().assigned());
}

TEST_F(MultiReader2ParamsTest, MainInputOutsideTheListIsRejectedAtCreate)
{
    addSignals(2);
    auto p = params(List<IComponent>(readSignals[0].signal));
    p.setMainInput(readSignals[1].signal);
    ASSERT_THROW(createReader(p), InvalidParameterException);
}

// ---------------------------------------------------------------- read type, min read count, policy

TEST_F(MultiReader2ParamsTest, SetValueReadTypeAcceptsEveryNumericTypeAndUndefined)
{
    const SampleType accepted[]{SampleType::Undefined,
                                SampleType::Float32,
                                SampleType::Float64,
                                SampleType::UInt8,
                                SampleType::Int8,
                                SampleType::UInt16,
                                SampleType::Int16,
                                SampleType::UInt32,
                                SampleType::Int32,
                                SampleType::UInt64,
                                SampleType::Int64};
    auto p = freshParams();
    for (const auto type : accepted)
    {
        ASSERT_EQ(p->setValueReadType(type), OPENDAQ_SUCCESS) << static_cast<int>(type);
        ASSERT_EQ(p.getValueReadType(), type);
    }
}

TEST_F(MultiReader2ParamsTest, SetValueReadTypeRejectsEveryOtherType)
{
    const SampleType rejected[]{SampleType::RangeInt64,
                                SampleType::ComplexFloat32,
                                SampleType::ComplexFloat64,
                                SampleType::Binary,
                                SampleType::String,
                                SampleType::Struct,
                                SampleType::Null,
                                static_cast<SampleType>(200)};
    auto p = freshParams();
    for (const auto type : rejected)
    {
        ASSERT_EQ(p->setValueReadType(type), OPENDAQ_ERR_INVALIDPARAMETER) << static_cast<int>(type);
        daqClearErrorInfo();
        ASSERT_EQ(p.getValueReadType(), SampleType::Float64);
    }
}

TEST_F(MultiReader2ParamsTest, SetMinReadCountRejectsZeroAndKeepsTheOldValue)
{
    auto p = freshParams();
    p.setMinReadCount(16);
    ASSERT_EQ(p->setMinReadCount(0), OPENDAQ_ERR_INVALIDPARAMETER);
    daqClearErrorInfo();
    ASSERT_EQ(p.getMinReadCount(), 16u);
}

TEST_F(MultiReader2ParamsTest, SetMinReadCountAcceptsOneAndLargeValues)
{
    auto p = freshParams();
    ASSERT_EQ(p->setMinReadCount(1), OPENDAQ_SUCCESS);
    ASSERT_EQ(p.getMinReadCount(), 1u);
    ASSERT_EQ(p->setMinReadCount(SizeT{1} << 40), OPENDAQ_SUCCESS);
    ASSERT_EQ(p.getMinReadCount(), SizeT{1} << 40);
}

TEST_F(MultiReader2ParamsTest, SetErrorPolicyAcceptsBothAndRejectsUnknown)
{
    auto p = freshParams();
    ASSERT_EQ(p->setErrorPolicy(MultiReader2ErrorPolicy::Exclude), OPENDAQ_SUCCESS);
    ASSERT_EQ(p.getErrorPolicy(), MultiReader2ErrorPolicy::Exclude);
    ASSERT_EQ(p->setErrorPolicy(static_cast<MultiReader2ErrorPolicy>(7)), OPENDAQ_ERR_INVALIDPARAMETER);
    daqClearErrorInfo();
    ASSERT_EQ(p.getErrorPolicy(), MultiReader2ErrorPolicy::Exclude);
    ASSERT_EQ(p->setErrorPolicy(MultiReader2ErrorPolicy::Invalidate), OPENDAQ_SUCCESS);
    ASSERT_EQ(p.getErrorPolicy(), MultiReader2ErrorPolicy::Invalidate);
}

// ---------------------------------------------------------------- params and readers

TEST_F(MultiReader2ParamsTest, EditingParamsDoesNotTouchTheReaderUntilConfigure)
{
    addSignals(2);
    auto p = params(signalsToList());
    auto reader = createReader(p);
    ASSERT_EQ(reader.getMainInput(), readSignals[0].signal.getGlobalId());

    p.setMainInput(readSignals[1].signal);
    ASSERT_EQ(reader.getMainInput(), readSignals[0].signal.getGlobalId());
    reader.configure(p);
    ASSERT_EQ(reader.getMainInput(), readSignals[1].signal.getGlobalId());
}

TEST_F(MultiReader2ParamsTest, OneParamsObjectServesTwoReaders)
{
    addSignals(2);
    auto domain = readSignals[0].signal.getDomainSignal();
    auto p = params(signalsToList());
    auto first = createReaderProbed(p);
    auto second = createReaderProbed(p);
    sendPackets(0);
    ASSERT_EQ(first.getAvailableCount(), 10u);
    ASSERT_EQ(second.getAvailableCount(), 10u);
}

TEST_F(MultiReader2ParamsTest, FactoryParamsCreateAReader)
{
    addSignals(2);
    auto p = MultiReader2Params();
    p.setInputs(signalsToList());
    p.setValueReadType(SampleType::Float64);
    auto reader = MultiReader2(p);
    scheduler.waitAll();
    auto status = probe(reader);
    ASSERT_TRUE(status.getValid());
    ASSERT_EQ(status.getInputs().getCount(), 2u);
}
