#pragma once

#include <map>
#include <memory>

#include <opendaq/function_block_impl.h>
#include <opendaq/opendaq.h>

#include <basic_recorder_module/basic_recorder_signal.h>
#include <basic_recorder_module/common.h>

BEGIN_NAMESPACE_OPENDAQ_BASIC_RECORDER_MODULE

/**
 * A basic recorder function block which records data from its input signals into a CSV file. A
 * separate CSV file is created for each recorded signal, in a directory specified by the user via
 * a property. Recording can be started and stopped by calling member functions or by setting a
 * property. Signals can be dynamically connected and disconnected. Initially, the function block
 * has a single input port named "Value1"; additional input ports "Value2" etc. are created so
 * that at least one unconnected input port is always available. Signals can be connected or
 * disconnected while the recording is active. Doing so does not disturb ongoing recording of
 * other signals.
 */
class BasicRecorderImpl final : public FunctionBlockImpl<IFunctionBlock, IRecorder>
{
    public:

        /**
         * The type ID of this function block.
         */
        static constexpr const char *TYPE_ID = "BasicRecorder";

        /**
         * Contains constants for the names of tags assigned to this function block.
         */
        struct Tags
        {
            /**
             * A tag identifying this function block as a recorder.
             */
            static constexpr const char *RECORDER = "Recorder";
        };

        /**
         * Contains constants for the names of properties supported by this function block.
         */
        struct Props
        {
            /**
             * A boolean which specifies whether the recording is currently active. This property
             * can be written, which has the same effect as calling the startRecording() and
             * stopRecording() functions.
             */
            static constexpr const char *RECORDING_ACTIVE = "RecordingActive";

            /**
             * The absolute path to the directory where CSV files should be written. A separate
             * CSV file is written for each recorded signal. The current implementation interprets
             * relative paths with respect to the current working directory of the process, but
             * this behavior is not guaranteed.
             */
            static constexpr const char *PATH = "Path";
        };

        /**
         * Creates and returns a type object describing this function block.
         *
         * @param return A populated function block type object.
         */
        static FunctionBlockTypePtr createType();

        /**
         * Creates a new function block.
         *
         * @param context The openDAQ context object.
         * @param parent The component object which will contain this function block.
         * @param localId The local identifier of this function block.
         * @param config A property object containing configuration data for this function block.
         */
        BasicRecorderImpl(
            const ContextPtr& context,
            const ComponentPtr& parent,
            const StringPtr& localId,
            const PropertyObjectPtr& config);

        /**
         * Starts the recording. This has the same effect as (and is implemented in terms of)
         * setting the `RecordingActive` property to `true`.
         */
        ErrCode INTERFACE_FUNC startRecording() override;

        /**
         * Stops the recording. This has the same effect as (and is implemented in terms of)
         * setting the `RecordingActive` property to `false`.
         */
        ErrCode INTERFACE_FUNC stopRecording() override;

        /**
         * When a signal is connected to an input port, a new input port is dynamically added,
         * so that one is always available to be connected.
         *
         * @param inputPort The input port that was connected.
         */
        void onConnected(const InputPortPtr& port) override;

        /**
         * When a signal is disconnected from the second-to-last input port, the last input
         * port is removed, so that only one unconnected port is always present at the end (it
         * is however possible for additional ports to be disconnected if there are still
         * higher-numbered ports in use).
         *
         * @param port The input port that was disconnected.
         */
        void onDisconnected(const InputPortPtr& port) override;

        /**
         * Records a packet.
         *
         * @param port The input port on which the packet was received.
         */
        void onPacketReceived(const InputPortPtr& port) override;

    private:

        void addProperties();
        void addInputPort();
        void reconfigure();

        std::shared_ptr<BasicRecorderSignal> findSignal(IInputPort *port);

        std::map<IInputPort *, std::shared_ptr<BasicRecorderSignal>> signals;

        unsigned portCount = 0;
};

END_NAMESPACE_OPENDAQ_BASIC_RECORDER_MODULE
