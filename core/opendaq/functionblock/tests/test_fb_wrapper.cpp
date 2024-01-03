#include <opendaq/gmock/component.h>
#include <opendaq/gmock/function_block.h>
#include <opendaq/function_block_wrapper_factory.h>
#include <gtest/gtest.h>
#include <coretypes/gmock/mock_ptr.h>
#include <coreobjects/property_object_impl.h>
#include <coreobjects/validator_factory.h>
#include <coreobjects/coercer_factory.h>
#include <coreobjects/property_object_factory.h>
#include <opendaq/context_factory.h>
#include <opendaq/function_block_type_factory.h>
#include <opendaq/input_port_ptr.h>

struct MockInputPort : daq::MockGenericComponent<MockInputPort, daq::IInputPort>
{
    typedef MockPtr<
        daq::IInputPort,
        daq::InputPortPtr,
        MockInputPort
    > Strict;

    MOCK_METHOD(daq::ErrCode, acceptsSignal, (daq::ISignal* signal, daq::Bool* accepts), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, connect, (daq::ISignal* signal), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, disconnect, (), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, getSignal, (daq::ISignal** signal), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, getConnection, (daq::IConnection** connection), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, getRequiresSignal, (daq::Bool* value), (override MOCK_CALL));

    MockInputPort()
        : MockGenericComponent<MockInputPort, daq::IInputPort>()
    {
    }
};

struct MockSignal : daq::MockGenericComponent<MockSignal, daq::ISignal>
{
    typedef MockPtr<daq::ISignal, daq::SignalPtr, MockSignal> Strict;

    MOCK_METHOD(daq::ErrCode, getPublic, (daq::Bool* public_), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, setPublic, (daq::Bool public_), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, getDescriptor, (daq::IDataDescriptor** descriptor), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, getDomainSignal, (daq::ISignal** signal), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, getRelatedSignals, (daq::IList** signals), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, getConnections, (daq::IList** connections), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, setStreamed, (daq::Bool streamed), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, getStreamed, (daq::Bool* streamed), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, getValue, (IBaseObject ** value), (override MOCK_CALL));

    MockSignal()
        : MockGenericComponent<MockSignal, daq::ISignal>()
    {
    }
};

using namespace testing;


MATCHER_P(IsName, name, "")
{
    if (arg.getName() == name)
        return true;
    *result_listener << "whose name is \"" << arg.getName() << "\"";
    return false;
}

MATCHER_P(IsLocalId, localId, "")
{
    if (arg.getLocalId() == localId)
        return true;
    *result_listener << "whose local id is \"" << arg.getLocalId() << "\"";
    return false;
}

MATCHER(IsListEmpty, negation ? "isn't empty" : "is empty")
{
    if (arg.empty())
        return true;
    *result_listener << "whose size is " << arg.getCount();
    return false;
}

class FunctionBlockWrapperTest : public Test
{
protected:
    void SetUp() override
    {
        EXPECT_CALL(fb.mock(), getParent(_)).WillRepeatedly(daq::Get<daq::ComponentPtr>(nullptr));
        EXPECT_CALL(fb.mock(), getLocalId(_)).WillRepeatedly(daq::Get<daq::StringPtr>("testPort"));
        EXPECT_CALL(fb.mock(), getContext(_)).WillRepeatedly(daq::Get(daq::NullContext()));
        EXPECT_CALL(fb.mock(), getFunctionBlockType(_)).WillRepeatedly(daq::Get(daq::FunctionBlockType("test", "test", "test")));

        auto strProp = daq::StringPropertyBuilder("strprop", "-").setValidator(daq::Validator("value != ''")).build();
        fb->addProperty(strProp);

        fb->addProperty(daq::BoolProperty("boolprop", daq::True));
        fb->addProperty(daq::BoolProperty("boolprophidden", daq::True, daq::False));

        fb->addProperty(daq::SelectionProperty("selprop", daq::List<daq::IString>("sel1", "sel2", "sel3", "sel4"), 0));

        auto propObj = daq::PropertyObject();
        propObj->addProperty(daq::BoolProperty("boolprop", daq::True));

        fb->addProperty(daq::ObjectProperty("objprop", daq::PropertyObject()));

        fb->setPropertyValue("objprop", propObj);
    }

    daq::MockFunctionBlock::Strict fb;
};

TEST_F(FunctionBlockWrapperTest, Create)
{
    auto fbw = daq::FunctionBlockWrapper(fb, true, true, true, true);

    ASSERT_EQ(fbw.asPtr<daq::IFunctionBlockWrapper>().getWrappedFunctionBlock(), *fb);
}

TEST_F(FunctionBlockWrapperTest, GetInputPorts)
{
    auto inputPorts = daq::List<daq::IInputPort>();

    MockInputPort::Strict ip0;
    EXPECT_CALL(ip0.mock(), getLocalId(_)).WillRepeatedly(daq::Get{daq::String("ip0")});
    inputPorts.pushBack(ip0);

    MockInputPort::Strict ip1;
    EXPECT_CALL(ip1.mock(), getLocalId(_)).WillRepeatedly(daq::Get<daq::StringPtr>{"ip1"});
    inputPorts.pushBack(ip1);

    EXPECT_CALL(fb.mock(), getInputPorts(_)).WillRepeatedly(daq::Get{inputPorts});

    auto fbw = daq::FunctionBlockWrapper(fb, true, true, true, true);

    ASSERT_THAT(fbw.getInputPorts(), ElementsAre(IsLocalId("ip0"), IsLocalId("ip1")));

    auto fbwCtrl = fbw.asPtr<daq::IFunctionBlockWrapper>();

    fbwCtrl.excludeInputPort("ip0");
    ASSERT_THAT(fbw.getInputPorts(), ElementsAre(IsLocalId("ip1")));

    ASSERT_EQ(fbwCtrl->excludeInputPort(daq::String("ip0")), OPENDAQ_IGNORED);

    fbwCtrl.includeInputPort("ip0");
    ASSERT_THAT(fbw.getInputPorts(), ElementsAre(IsLocalId("ip0"), IsLocalId("ip1")));

    ASSERT_EQ(fbwCtrl->includeInputPort(daq::String("ip0")), OPENDAQ_IGNORED);

    fbw = daq::FunctionBlockWrapper(fb, false, true, true, true);
    ASSERT_THAT(fbw.getInputPorts(), IsListEmpty());

    fbwCtrl = fbw.asPtr<daq::IFunctionBlockWrapper>();

    fbwCtrl.includeInputPort("ip0");
    ASSERT_THAT(fbw.getInputPorts(), ElementsAre(IsLocalId("ip0")));

    fbwCtrl.excludeInputPort("ip0");
    ASSERT_THAT(fbw.getInputPorts(), IsListEmpty());
}

TEST_F(FunctionBlockWrapperTest, GetSignals)
{
    auto signals = daq::List<daq::ISignal>();

    MockSignal::Strict sig1;
    EXPECT_CALL(sig1.mock(), getLocalId(_)).WillRepeatedly(daq::Get<daq::StringPtr>("sig1"));
    signals.pushBack(sig1);

    MockSignal::Strict sig2;
    EXPECT_CALL(sig2.mock(), getLocalId(_)).WillRepeatedly(daq::Get<daq::StringPtr>("sig2"));
    signals.pushBack(sig2);

    EXPECT_CALL(fb.mock(), getSignals(_)).WillRepeatedly(daq::Get{signals});

    auto fbw = daq::FunctionBlockWrapper(fb, true, true, true, true);

    ASSERT_THAT(fbw.getSignals(), ElementsAre(IsLocalId("sig1"), IsLocalId("sig2")));

    auto fbwCtrl = fbw.asPtr<daq::IFunctionBlockWrapper>();

    fbwCtrl.excludeSignal("sig1");
    ASSERT_THAT(fbw.getSignals(), ElementsAre(IsLocalId("sig2")));

    ASSERT_EQ(fbwCtrl->excludeSignal(daq::String("sig1")), OPENDAQ_IGNORED);

    fbwCtrl.includeSignal("sig1");
    ASSERT_THAT(fbw.getSignals(), ElementsAre(IsLocalId("sig1"), IsLocalId("sig2")));

    ASSERT_EQ(fbwCtrl->includeSignal(daq::String("sig1")), OPENDAQ_IGNORED);

    fbw = daq::FunctionBlockWrapper(fb, true, false, true, true);
    ASSERT_THAT(fbw.getSignals(), IsListEmpty());

    fbwCtrl = fbw.asPtr<daq::IFunctionBlockWrapper>();

    fbwCtrl.includeSignal("sig1");
    ASSERT_THAT(fbw.getSignals(), ElementsAre(IsLocalId("sig1")));

    fbwCtrl.excludeSignal("sig1");
    ASSERT_THAT(fbw.getSignals(), IsListEmpty());
}

TEST_F(FunctionBlockWrapperTest, GetFunctionBlocks)
{
    auto fbs = daq::List<daq::IFunctionBlock>();

    daq::MockFunctionBlock::Strict fb1;
    EXPECT_CALL(fb1.mock(), getLocalId(_)).WillRepeatedly(daq::Get<daq::StringPtr>("fb1"));
    fbs.pushBack(fb1);

    daq::MockFunctionBlock::Strict fb2;
    EXPECT_CALL(fb2.mock(), getLocalId(_)).WillRepeatedly(daq::Get<daq::StringPtr>("fb2"));
    fbs.pushBack(fb2);

    EXPECT_CALL(fb.mock(), getFunctionBlocks(_)).WillRepeatedly(daq::Get{fbs});

    auto fbw = daq::FunctionBlockWrapper(fb, true, true, true, true);

    ASSERT_THAT(fbw.getFunctionBlocks(), ElementsAre(IsLocalId("fb1"), IsLocalId("fb2")));

    auto fbwCtrl = fbw.asPtr<daq::IFunctionBlockWrapper>();

    fbwCtrl.excludeFunctionBlock("fb1");
    ASSERT_THAT(fbw.getFunctionBlocks(), ElementsAre(IsLocalId("fb2")));

    ASSERT_EQ(fbwCtrl->excludeFunctionBlock(daq::String("fb1")), OPENDAQ_IGNORED);

    fbwCtrl.includeFunctionBlock("fb1");
    ASSERT_THAT(fbw.getFunctionBlocks(), ElementsAre(IsLocalId("fb1"), IsLocalId("fb2")));

    ASSERT_EQ(fbwCtrl->includeFunctionBlock(daq::String("fb1")), OPENDAQ_IGNORED);

    fbw = daq::FunctionBlockWrapper(fb, true, true, true, false);
    ASSERT_THAT(fbw.getFunctionBlocks(), IsListEmpty());

    fbwCtrl = fbw.asPtr<daq::IFunctionBlockWrapper>();

    fbwCtrl.includeFunctionBlock("fb1");
    ASSERT_THAT(fbw.getFunctionBlocks(), ElementsAre(IsLocalId("fb1")));

    fbwCtrl.excludeFunctionBlock("fb1");
    ASSERT_THAT(fbw.getFunctionBlocks(), IsListEmpty());
}

TEST_F(FunctionBlockWrapperTest, Properties)
{
    auto fbw = daq::FunctionBlockWrapper(fb, true, true, true, true);

    ASSERT_THAT(fbw.getVisibleProperties(), ElementsAre(IsName("strprop"), IsName("boolprop"), IsName("selprop"), IsName("objprop")));
    ASSERT_THAT(fbw.getAllProperties(), ElementsAre(IsName("strprop"), IsName("boolprop"), IsName("boolprophidden"), IsName("selprop"), IsName("objprop")));
    ASSERT_TRUE(fbw.hasProperty("boolprop"));

    auto fbwCtrl = fbw.asPtr<daq::IFunctionBlockWrapper>();

    fbwCtrl.excludeProperty("boolprop");
    ASSERT_THAT(fbw.getVisibleProperties(), ElementsAre(IsName("strprop"), IsName("selprop"), IsName("objprop")));
    ASSERT_THAT(fbw.getAllProperties(), ElementsAre(IsName("strprop"), IsName("boolprophidden"), IsName("selprop"), IsName("objprop")));
    ASSERT_FALSE(fbw.hasProperty("boolprop"));

    ASSERT_EQ(fbwCtrl->excludeProperty(daq::String("boolprop")), OPENDAQ_IGNORED);

    fbwCtrl.includeProperty("boolprop");
    ASSERT_THAT(fbw.getVisibleProperties(), ElementsAre(IsName("strprop"), IsName("boolprop"), IsName("selprop"), IsName("objprop")));
    ASSERT_THAT(fbw.getAllProperties(), ElementsAre(IsName("strprop"), IsName("boolprop"), IsName("boolprophidden"), IsName("selprop"), IsName("objprop")));

    ASSERT_EQ(fbwCtrl->includeSignal(daq::String("sig1")), OPENDAQ_IGNORED);

    fbw = daq::FunctionBlockWrapper(fb, true, true, false, true);
    ASSERT_THAT(fbw.getVisibleProperties(), IsListEmpty());
    ASSERT_THAT(fbw.getAllProperties(), IsListEmpty());
    ASSERT_FALSE(fbw.hasProperty("boolprop"));

    fbwCtrl = fbw.asPtr<daq::IFunctionBlockWrapper>();

    fbwCtrl.includeProperty("boolprop");
    ASSERT_THAT(fbw.getVisibleProperties(), ElementsAre(IsName("boolprop")));
    ASSERT_THAT(fbw.getAllProperties(), ElementsAre(IsName("boolprop")));
    ASSERT_TRUE(fbw.hasProperty("boolprop"));

    fbwCtrl.excludeProperty("boolprop");
    ASSERT_THAT(fbw.getVisibleProperties(), IsListEmpty());
    ASSERT_THAT(fbw.getAllProperties(), IsListEmpty());
    ASSERT_FALSE(fbw.hasProperty("boolprop"));
}

TEST_F(FunctionBlockWrapperTest, PropertyValue)
{
    auto fbw = daq::FunctionBlockWrapper(fb, true, true, true, true);

    ASSERT_EQ(fbw.getPropertyValue("strprop"), "-");
    ASSERT_NO_THROW(fbw.setPropertyValue("strprop", "a"));
    ASSERT_THROW(fbw.setPropertyValue("strprop", ""), daq::ValidateFailedException);

    auto fbwCtrl = fbw.asPtr<daq::IFunctionBlockWrapper>();
    fbwCtrl.excludeProperty("strprop");

    ASSERT_THROW(fbw.setPropertyValue("strprop", "a"), daq::NotFoundException);
    ASSERT_THROW(fbw.setPropertyValue("strprop", ""), daq::NotFoundException);
    ASSERT_THROW(fbw.getPropertyValue("strprop"), daq::NotFoundException);
    ASSERT_THROW(fbw.clearPropertyValue("strprop"), daq::NotFoundException);

    fbw.setPropertyValue("objprop.boolprop", daq::False);
    fbwCtrl.excludeProperty("objprop");
    ASSERT_THROW(fbw.setPropertyValue("objprop.boolprop", daq::True), daq::NotFoundException);
}

TEST_F(FunctionBlockWrapperTest, PropertyValidator)
{
    auto fbw = daq::FunctionBlockWrapper(fb, true, true, true, true);

    auto fbwCtrl = fbw.asPtr<daq::IFunctionBlockWrapper>();

    ASSERT_THROW(fbwCtrl.setPropertyValidator("temp", daq::Validator("value == False")), daq::NotFoundException);

    fbwCtrl.setPropertyValidator("strprop", daq::Validator("value != 'val'"));
    ASSERT_THROW(fbw.setPropertyValue("strprop", ""), daq::ValidateFailedException);
    ASSERT_THROW(fbw.setPropertyValue("strprop", "val"), daq::ValidateFailedException);
    ASSERT_NO_THROW(fbw.setPropertyValue("strprop", "a"));
    ASSERT_EQ(fbw.getPropertyValue("strprop"), "a");

    fbwCtrl.setPropertyValidator("strprop", nullptr);
    ASSERT_NO_THROW(fbw.setPropertyValue("strprop", "val"));
    ASSERT_EQ(fbw.getPropertyValue("strprop"), "val");
}

TEST_F(FunctionBlockWrapperTest, PropertyCoercer)
{
    auto fbw = daq::FunctionBlockWrapper(fb, true, true, true, true);

    auto fbwCtrl = fbw.asPtr<daq::IFunctionBlockWrapper>();

    ASSERT_THROW(fbwCtrl.setPropertyCoercer("boolhidden", daq::Coercer("True")), daq::NotFoundException);

    ASSERT_NO_THROW(fbw.setPropertyValue("boolprop", daq::False));
    ASSERT_EQ(fbw.getPropertyValue("boolprop"), daq::False);
    ASSERT_NO_THROW(fbw.setPropertyValue("boolprop", daq::True));
    ASSERT_EQ(fbw.getPropertyValue("boolprop"), daq::True);

    fbwCtrl.setPropertyCoercer("boolprop", daq::Coercer("True"));
    ASSERT_NO_THROW(fbw.setPropertyValue("boolprop", daq::False));
    ASSERT_EQ(fbw.getPropertyValue("boolprop"), daq::True);

    fbwCtrl.setPropertyCoercer("boolprop", nullptr);
    ASSERT_NO_THROW(fbw.setPropertyValue("boolprop", daq::False));
    ASSERT_EQ(fbw.getPropertyValue("boolprop"), daq::False);
}

TEST_F(FunctionBlockWrapperTest, AddAndRemoveProperty)
{
    auto fbw = daq::FunctionBlockWrapper(fb, true, true, true, true);

    ASSERT_THROW(fbw.addProperty(nullptr), daq::AccessDeniedException);
    ASSERT_THROW(fbw.removeProperty(nullptr), daq::AccessDeniedException);
}

TEST_F(FunctionBlockWrapperTest, PropertySelectionValue)
{
    using DictElem = std::pair<daq::IntegerPtr, daq::StringPtr>;

    auto fbw = daq::FunctionBlockWrapper(fb, true, true, true, true);

    auto prop = fbw.getProperty("selprop");
    daq::ListPtr<daq::IString> selValues = prop.getSelectionValues();
    ASSERT_THAT(selValues, ElementsAre("sel1", "sel2", "sel3", "sel4"));

    auto fbwCtrl = fbw.asPtr<daq::IFunctionBlockWrapper>();

    fbwCtrl.setPropertySelectionValues("selprop", daq::List<daq::IInteger>(0, 1, 2));

    prop = fbw.getProperty("selprop");
    daq::DictPtr<daq::IInteger, daq::IString> selDict = prop.getSelectionValues();
    ASSERT_THAT(selDict, ElementsAre(DictElem(0, "sel1"), DictElem(1, "sel2"), DictElem(2, "sel3")));

    ASSERT_THROW(fbw.setPropertyValue("selprop", 3), daq::NotFoundException);

    fbwCtrl.setPropertySelectionValues("selprop", nullptr);
    ASSERT_NO_THROW(fbw.setPropertyValue("selprop", 3));
}

TEST_F(FunctionBlockWrapperTest, WrapperProperties)
{
    auto fbw = daq::FunctionBlockWrapper(fb, true, true, true, true);

    auto props = fbw.getAllProperties();

    const auto prop = *std::find_if(props.begin(), props.end(), [](const daq::PropertyPtr& p) { return p.getName() == "selprop"; });

    ASSERT_EQ(prop.getName(), "selprop");
    ASSERT_EQ(prop.getValueType(), daq::ctInt);
    ASSERT_EQ(prop.getItemType(), daq::ctString);
    ASSERT_EQ(prop.getDefaultValue(), 0);
    ASSERT_EQ(prop.getVisible(), daq::True);
    ASSERT_THAT(prop.getSelectionValues().asPtr<daq::IList>(), ElementsAre("sel1", "sel2", "sel3", "sel4"));
    ASSERT_EQ(prop.getReadOnly(), daq::False);
}
