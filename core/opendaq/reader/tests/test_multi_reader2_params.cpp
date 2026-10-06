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
    ASSERT_TRUE(p.getUsed());
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
    ASSERT_EQ(p->getUsed(nullptr), OPENDAQ_ERR_ARGUMENT_NULL);
    daqClearErrorInfo();
    ASSERT_EQ(p->getValueReadType(nullptr), OPENDAQ_ERR_ARGUMENT_NULL);
    daqClearErrorInfo();
    ASSERT_EQ(p->getMinReadCount(nullptr), OPENDAQ_ERR_ARGUMENT_NULL);
    daqClearErrorInfo();
    ASSERT_EQ(p->getErrorPolicy(nullptr), OPENDAQ_ERR_ARGUMENT_NULL);
    daqClearErrorInfo();
    ASSERT_EQ(p->getInputUsed(readSignals[0].signal, nullptr), OPENDAQ_ERR_ARGUMENT_NULL);
    daqClearErrorInfo();
    daq::Bool used;
    ASSERT_EQ(p->getInputUsed(nullptr, &used), OPENDAQ_ERR_ARGUMENT_NULL);
    daqClearErrorInfo();
}

TEST_F(MultiReader2ParamsTest, SettersRejectNullInput)
{
    auto p = freshParams();
    ASSERT_EQ(p->setInputs(nullptr), OPENDAQ_ERR_ARGUMENT_NULL);
    daqClearErrorInfo();
    ASSERT_EQ(p->setInputUsed(nullptr, True), OPENDAQ_ERR_ARGUMENT_NULL);
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

TEST_F(MultiReader2ParamsTest, SetInputsRejectsEmptyList)
{
    auto p = freshParams();
    ASSERT_EQ(p->setInputs(List<IComponent>()), OPENDAQ_ERR_INVALIDPARAMETER);
    daqClearErrorInfo();
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

TEST_F(MultiReader2ParamsTest, ReplacingTheListForgetsFlagsOfRemovedInputs)
{
    addSignals(3);
    auto p = freshParams();
    p.setInputs(List<IComponent>(readSignals[0].signal, readSignals[1].signal));
    p.setInputUsed(readSignals[0].signal, false);

    // Signal 0 leaves the list and comes back: it is used again
    p.setInputs(List<IComponent>(readSignals[1].signal, readSignals[2].signal));
    p.setInputs(List<IComponent>(readSignals[0].signal, readSignals[1].signal));
    ASSERT_TRUE(p.getInputUsed(readSignals[0].signal));
}

TEST_F(MultiReader2ParamsTest, ReplacingTheListKeepsFlagsOfRetainedPorts)
{
    addSignals(3);
    portsList();
    auto p = freshParams();
    p.setInputs(List<IComponent>(ports[0], ports[1]));
    p.setInputUsed(ports[1], false);
    p.setInputs(List<IComponent>(ports[1], ports[2]));
    ASSERT_FALSE(p.getInputUsed(ports[1]));
    ASSERT_TRUE(p.getInputUsed(ports[2]));
}

// ---------------------------------------------------------------- used flags

TEST_F(MultiReader2ParamsTest, InputUsedDefaultsToTrueForEveryInput)
{
    addSignals(4);
    auto p = freshParams();
    p.setInputs(signalsToList());
    for (const auto& read : readSignals)
        ASSERT_TRUE(p.getInputUsed(read.signal));
}

TEST_F(MultiReader2ParamsTest, SetInputUsedIsIdempotent)
{
    addSignals(1);
    auto p = freshParams();
    p.setInputs(signalsToList());
    ASSERT_EQ(p->setInputUsed(readSignals[0].signal, False), OPENDAQ_SUCCESS);
    ASSERT_EQ(p->setInputUsed(readSignals[0].signal, False), OPENDAQ_SUCCESS);
    ASSERT_FALSE(p.getInputUsed(readSignals[0].signal));
    ASSERT_EQ(p->setInputUsed(readSignals[0].signal, True), OPENDAQ_SUCCESS);
    ASSERT_EQ(p->setInputUsed(readSignals[0].signal, True), OPENDAQ_SUCCESS);
    ASSERT_TRUE(p.getInputUsed(readSignals[0].signal));
}

TEST_F(MultiReader2ParamsTest, SetInputUsedRejectsForeignInput)
{
    addSignals(2);
    auto p = freshParams();
    p.setInputs(List<IComponent>(readSignals[0].signal));
    ASSERT_EQ(p->setInputUsed(readSignals[1].signal, False), OPENDAQ_ERR_INVALIDPARAMETER);
    daqClearErrorInfo();
}

TEST_F(MultiReader2ParamsTest, GetInputUsedRejectsForeignInput)
{
    addSignals(2);
    auto p = freshParams();
    p.setInputs(List<IComponent>(readSignals[0].signal));
    daq::Bool used;
    ASSERT_EQ(p->getInputUsed(readSignals[1].signal, &used), OPENDAQ_ERR_INVALIDPARAMETER);
    daqClearErrorInfo();
}

TEST_F(MultiReader2ParamsTest, UsedFlagsBeforeAnyListAreForeign)
{
    addSignals(1);
    auto p = freshParams();
    ASSERT_EQ(p->setInputUsed(readSignals[0].signal, False), OPENDAQ_ERR_INVALIDPARAMETER);
    daqClearErrorInfo();
}

TEST_F(MultiReader2ParamsTest, PinnedMainCannotBeSetUnused)
{
    addSignals(2);
    auto p = freshParams();
    p.setInputs(signalsToList());
    p.setMainInput(readSignals[1].signal);
    ASSERT_EQ(p->setInputUsed(readSignals[1].signal, False), OPENDAQ_ERR_INVALIDPARAMETER);
    daqClearErrorInfo();
    ASSERT_TRUE(p.getInputUsed(readSignals[1].signal));
    // The other input is free to change
    ASSERT_EQ(p->setInputUsed(readSignals[0].signal, False), OPENDAQ_SUCCESS);
}

TEST_F(MultiReader2ParamsTest, PinnedMainCanBeSetUsedAgain)
{
    addSignals(2);
    auto p = freshParams();
    p.setInputs(signalsToList());
    p.setMainInput(readSignals[1].signal);
    ASSERT_EQ(p->setInputUsed(readSignals[1].signal, True), OPENDAQ_SUCCESS);
}

TEST_F(MultiReader2ParamsTest, PinningAnUnusedInputIsRejectedAtCreate)
{
    addSignals(2);
    auto p = params(signalsToList());
    p.setInputUsed(readSignals[1].signal, false);
    // The params take the pin; the reader rejects the combination
    ASSERT_EQ(p->setMainInput(readSignals[1].signal), OPENDAQ_SUCCESS);
    ASSERT_THROW(createReader(p), InvalidParameterException);
}

TEST_F(MultiReader2ParamsTest, SetUsedToggles)
{
    auto p = freshParams();
    p.setUsed(false);
    ASSERT_FALSE(p.getUsed());
    p.setUsed(true);
    ASSERT_TRUE(p.getUsed());
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
