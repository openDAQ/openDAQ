#include <gtest/gtest.h>
#include <opendaq/cyclic_ref_check.h>
#include <opendaq/signal_factory.h>
#include <opendaq/input_port_factory.h>
#include <opendaq/context_factory.h>
#include <opendaq/function_block_impl.h>
#include <opendaq/function_block_type_factory.h>

using namespace daq;

using CyclicRefCheckTest = ::testing::Test;

// Simple test function block for testing
class TestFunctionBlock : public FunctionBlockImpl<>
{
public:
    TestFunctionBlock(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId)
        : FunctionBlockImpl(FunctionBlockType("test", "Test FB", "Test function block"), ctx, parent, localId)
    {
        // Create an output signal
        outputSignal = createAndAddSignal("output");
        // Create an input port  
        inputPort = createAndAddInputPort("input", PacketReadyNotification::None);
    }

    SignalConfigPtr getOutputSignal() { return outputSignal; }
    InputPortConfigPtr getInputPort() { return inputPort; }

private:
    SignalConfigPtr outputSignal;
    InputPortConfigPtr inputPort;
};

// Complex test function block with multiple inputs and outputs
class ComplexTestFunctionBlock : public FunctionBlockImpl<>
{
public:
    ComplexTestFunctionBlock(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId)
        : FunctionBlockImpl(FunctionBlockType("complex_test", "Complex Test FB", "Complex test function block"), ctx, parent, localId)
    {
        // Create multiple output signals
        outputSignal1 = createAndAddSignal("output1");
        outputSignal2 = createAndAddSignal("output2");

        // Create multiple input ports  
        inputPort1 = createAndAddInputPort("input1", PacketReadyNotification::None);
        inputPort2 = createAndAddInputPort("input2", PacketReadyNotification::None);
    }

    SignalConfigPtr getOutputSignal1() { return outputSignal1; }
    SignalConfigPtr getOutputSignal2() { return outputSignal2; }
    InputPortConfigPtr getInputPort1() { return inputPort1; }
    InputPortConfigPtr getInputPort2() { return inputPort2; }

private:
    SignalConfigPtr outputSignal1;
    SignalConfigPtr outputSignal2;
    InputPortConfigPtr inputPort1;
    InputPortConfigPtr inputPort2;
};

TEST_F(CyclicRefCheckTest, NoCycleDetection)
{
    const auto context = NullContext();

    // Create two function blocks
    auto fb1 = createWithImplementation<IFunctionBlock, TestFunctionBlock>(context, nullptr, "fb1");
    auto fb2 = createWithImplementation<IFunctionBlock, TestFunctionBlock>(context, nullptr, "fb2");

    auto testFb1 = static_cast<TestFunctionBlock*>(fb1.getObject());
    auto testFb2 = static_cast<TestFunctionBlock*>(fb2.getObject());

    // Test no cycle - should return false
    Bool hasCycle = true;
    ASSERT_EQ(daqHasCyclicReferenceIfConnected(
        testFb1->getOutputSignal(), 
        testFb2->getInputPort(), 
        &hasCycle),
        OPENDAQ_SUCCESS);
    ASSERT_FALSE(hasCycle);
}

TEST_F(CyclicRefCheckTest, CycleDetection)
{
    const auto context = NullContext();

    // Create two function blocks
    auto fb1 = createWithImplementation<IFunctionBlock, TestFunctionBlock>(context, nullptr, "fb1");
    auto fb2 = createWithImplementation<IFunctionBlock, TestFunctionBlock>(context, nullptr, "fb2");

    auto testFb1 = static_cast<TestFunctionBlock*>(fb1.getObject());
    auto testFb2 = static_cast<TestFunctionBlock*>(fb2.getObject());

    // Connect fb1 output to fb2 input
    testFb2->getInputPort().connect(testFb1->getOutputSignal());

    // Test cycle - fb2 output connecting to fb1 input should detect a cycle
    Bool hasCycle = false;
    ASSERT_EQ(daqHasCyclicReferenceIfConnected(
        testFb2->getOutputSignal(), 
        testFb1->getInputPort(), 
        &hasCycle
    ), OPENDAQ_SUCCESS);

    ASSERT_TRUE(hasCycle);
}

TEST_F(CyclicRefCheckTest, NullParameterHandling)
{
    const auto context = NullContext();

    // Create a function block for testing
    auto fb1 = createWithImplementation<IFunctionBlock, TestFunctionBlock>(context, nullptr, "fb1");
    auto testFb1 = static_cast<TestFunctionBlock*>(fb1.getObject());

    Bool hasCycle = false;

    // Test null signal parameter
    ASSERT_EQ(daqHasCyclicReferenceIfConnected(nullptr, testFb1->getInputPort(), &hasCycle), OPENDAQ_ERR_ARGUMENT_NULL);

    // Test null input port parameter
    ASSERT_EQ(daqHasCyclicReferenceIfConnected(testFb1->getOutputSignal(), nullptr, &hasCycle), OPENDAQ_ERR_ARGUMENT_NULL);

    // Test null output parameter
    ASSERT_EQ(daqHasCyclicReferenceIfConnected(testFb1->getOutputSignal(), testFb1->getInputPort(), nullptr), OPENDAQ_ERR_ARGUMENT_NULL);
}

TEST_F(CyclicRefCheckTest, SelfLoopDetection)
{
    const auto context = NullContext();

    // Create a single function block
    auto fb1 = createWithImplementation<IFunctionBlock, TestFunctionBlock>(context, nullptr, "fb1");
    auto testFb1 = static_cast<TestFunctionBlock*>(fb1.getObject());

    // Test connecting function block output to its own input (self-loop)
    Bool hasCycle = false;
    ASSERT_EQ(daqHasCyclicReferenceIfConnected(
        testFb1->getOutputSignal(), 
        testFb1->getInputPort(), 
        &hasCycle), OPENDAQ_SUCCESS);
    ASSERT_TRUE(hasCycle);
}

TEST_F(CyclicRefCheckTest, ComplexLinearChain)
{
    const auto context = NullContext();

    // Create three complex function blocks
    auto fb1 = createWithImplementation<IFunctionBlock, ComplexTestFunctionBlock>(context, nullptr, "fb1");
    auto fb2 = createWithImplementation<IFunctionBlock, ComplexTestFunctionBlock>(context, nullptr, "fb2");
    auto fb3 = createWithImplementation<IFunctionBlock, ComplexTestFunctionBlock>(context, nullptr, "fb3");

    auto testFb1 = static_cast<ComplexTestFunctionBlock*>(fb1.getObject());
    auto testFb2 = static_cast<ComplexTestFunctionBlock*>(fb2.getObject());
    auto testFb3 = static_cast<ComplexTestFunctionBlock*>(fb3.getObject());

    // Create linear chain: FB1 -> FB2 -> FB3
    testFb2->getInputPort1().connect(testFb1->getOutputSignal1());
    testFb3->getInputPort1().connect(testFb2->getOutputSignal1());

    // Test that adding another linear connection doesn't create a cycle
    Bool hasCycle = true;
    ASSERT_EQ(daqHasCyclicReferenceIfConnected(
        testFb1->getOutputSignal2(), 
        testFb2->getInputPort2(), 
        &hasCycle), OPENDAQ_SUCCESS);
    ASSERT_FALSE(hasCycle);

    // Test that connecting backwards from FB3 to FB1 would create a cycle
    ASSERT_EQ(daqHasCyclicReferenceIfConnected(
        testFb3->getOutputSignal1(), 
        testFb1->getInputPort1(), 
        &hasCycle), OPENDAQ_SUCCESS);
    ASSERT_TRUE(hasCycle);
}

TEST_F(CyclicRefCheckTest, ComplexMultipleConnectionsCycle)
{
    const auto context = NullContext();

    // Create three complex function blocks
    auto fb1 = createWithImplementation<IFunctionBlock, ComplexTestFunctionBlock>(context, nullptr, "fb1");
    auto fb2 = createWithImplementation<IFunctionBlock, ComplexTestFunctionBlock>(context, nullptr, "fb2");
    auto fb3 = createWithImplementation<IFunctionBlock, ComplexTestFunctionBlock>(context, nullptr, "fb3");

    auto testFb1 = static_cast<ComplexTestFunctionBlock*>(fb1.getObject());
    auto testFb2 = static_cast<ComplexTestFunctionBlock*>(fb2.getObject());
    auto testFb3 = static_cast<ComplexTestFunctionBlock*>(fb3.getObject());

    // Create complex connections
    // FB1.out1 -> FB2.in1
    testFb2->getInputPort1().connect(testFb1->getOutputSignal1());
    // FB2.out1 -> FB3.in1  
    testFb3->getInputPort1().connect(testFb2->getOutputSignal1());
    // FB1.out2 -> FB3.in2
    testFb3->getInputPort2().connect(testFb1->getOutputSignal2());

    // Test that connecting FB3.out1 -> FB1.in1 would create a cycle
    Bool hasCycle = false;
    ASSERT_EQ(daqHasCyclicReferenceIfConnected(
        testFb3->getOutputSignal1(), 
        testFb1->getInputPort1(), 
        &hasCycle), OPENDAQ_SUCCESS);
    ASSERT_TRUE(hasCycle);

    // Test that connecting FB3.out2 -> FB2.in2 would also create a cycle
    ASSERT_EQ(daqHasCyclicReferenceIfConnected(
        testFb3->getOutputSignal2(), 
        testFb2->getInputPort2(), 
        &hasCycle), OPENDAQ_SUCCESS);
    ASSERT_TRUE(hasCycle);
}

TEST_F(CyclicRefCheckTest, ComplexParallelPaths)
{
    const auto context = NullContext();

    // Create four complex function blocks
    auto fb1 = createWithImplementation<IFunctionBlock, ComplexTestFunctionBlock>(context, nullptr, "fb1");
    auto fb2 = createWithImplementation<IFunctionBlock, ComplexTestFunctionBlock>(context, nullptr, "fb2");
    auto fb3 = createWithImplementation<IFunctionBlock, ComplexTestFunctionBlock>(context, nullptr, "fb3");
    auto fb4 = createWithImplementation<IFunctionBlock, ComplexTestFunctionBlock>(context, nullptr, "fb4");

    auto testFb1 = static_cast<ComplexTestFunctionBlock*>(fb1.getObject());
    auto testFb2 = static_cast<ComplexTestFunctionBlock*>(fb2.getObject());
    auto testFb3 = static_cast<ComplexTestFunctionBlock*>(fb3.getObject());
    auto testFb4 = static_cast<ComplexTestFunctionBlock*>(fb4.getObject());

    // Create parallel paths:
    // Path 1: FB1.out1 -> FB2.in1 -> FB4.in1
    // Path 2: FB1.out2 -> FB3.in1 -> FB4.in2
    testFb2->getInputPort1().connect(testFb1->getOutputSignal1());
    testFb3->getInputPort1().connect(testFb1->getOutputSignal2());
    testFb4->getInputPort1().connect(testFb2->getOutputSignal1());
    testFb4->getInputPort2().connect(testFb3->getOutputSignal1());

    // Test that connecting FB4 back to FB1 would create a cycle
    Bool hasCycle = false;
    ASSERT_EQ(daqHasCyclicReferenceIfConnected(
        testFb4->getOutputSignal1(), 
        testFb1->getInputPort1(), 
        &hasCycle), OPENDAQ_SUCCESS);
    ASSERT_TRUE(hasCycle);

    // Test that connecting FB2 to FB3 would not create a cycle
    ASSERT_EQ(daqHasCyclicReferenceIfConnected(
        testFb2->getOutputSignal2(), 
        testFb3->getInputPort2(), 
        &hasCycle), OPENDAQ_SUCCESS);
    ASSERT_FALSE(hasCycle);
}
