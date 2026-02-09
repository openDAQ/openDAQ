#include <opendaq/cyclic_ref_check.h>
#include <opendaq/function_block_ptr.h>
#include <opendaq/signal_ptr.h>
#include <opendaq/input_port_ptr.h>
#include <opendaq/connection_ptr.h>
#include <coretypes/listptr.h>
#include <opendaq/input_port_config_ptr.h>
#include <opendaq/input_port_notifications_ptr.h>
#include <unordered_set>
#include <opendaq/search_filter_factory.h>

BEGIN_NAMESPACE_OPENDAQ

using RefCheckDict = std::unordered_set<FunctionBlockPtr, ObjectPtrHash<IFunctionBlock>, ObjectPtrEqualTo<IFunctionBlock>>;

static bool checkCyclicReferenceRecursive(const SignalPtr& targetSignal, 
                                          const InputPortNotificationsPtr& listener,
                                          RefCheckDict& visitedFunctionBlocks)
{
    if (!listener.assigned())
        return false;

    // Try to cast the component to a function block
    const auto functionBlock = listener.asPtrOrNull<IFunctionBlock>(true);
    if (!functionBlock.assigned())
        return false;

    // Check if we've already visited this component to prevent infinite recursion
    if (visitedFunctionBlocks.find(functionBlock) != visitedFunctionBlocks.end())
        return false;

    visitedFunctionBlocks.insert(functionBlock);

    // Get all output signals from this function block
    auto outputSignals = functionBlock.getSignals(daq::search::Any());

    for (const auto& outputSignal : outputSignals)
    {
        // Check if this output signal is the same as the target signal
        if (outputSignal == targetSignal)
            return true; // Cycle detected

        // Get connections from this output signal
        const auto connections = outputSignal.getConnections();
        for (const auto& connection: connections)
        {
            if (!connection.assigned())
                continue;

            // Get the input port from the connection
            InputPortPtr inputPort = connection.getInputPort();
            if (!inputPort.assigned())
                continue;

            const auto innerListener = inputPort.asPtr<IInputPortConfig>(true).getListener();
            if (checkCyclicReferenceRecursive(targetSignal, innerListener, visitedFunctionBlocks))
                return true; // Cycle detected
        }
    }

    return false;
}

ErrCode daqHasCyclicReferenceIfConnected(ISignal* signal, IInputPort* inputPort, Bool* cyclicReference)
{
    if (!signal || !inputPort || !cyclicReference)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *cyclicReference = false;

    return daqTry([&]() {
        const SignalPtr signalPtr = SignalPtr::Borrow(signal);
        const InputPortPtr inputPortPtr = InputPortPtr::Borrow(inputPort);

        // Get the listener of the input port (should be a function block)
        const auto listener = inputPortPtr.asPtr<IInputPortConfig>(true).getListener();

        // Use a set to track visited components to prevent infinite loops
        RefCheckDict visitedFunctionBlocks;

        // Check for cyclic reference starting from the parent component
        *cyclicReference = checkCyclicReferenceRecursive(signalPtr, listener, visitedFunctionBlocks);
    });
}

END_NAMESPACE_OPENDAQ
