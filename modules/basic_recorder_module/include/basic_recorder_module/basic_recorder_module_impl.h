#pragma once

#include <opendaq/module_impl.h>
#include <opendaq/opendaq.h>

#include <basic_recorder_module/common.h>

BEGIN_NAMESPACE_OPENDAQ_BASIC_RECORDER_MODULE

/**
 * An openDAQ module which exposes a single `FunctionBlock` object, BasicRecorderImpl.
 */
class BasicRecorderModule final : public daq::Module
{
    public:

        /**
         * The name of this module.
         */
        static constexpr const char *MODULE_NAME = "BasicRecorderModule";

        /**
         * Constructs a new module.
         *
         * @param context The openDAQ context object.
         */
        BasicRecorderModule(daq::ContextPtr context);

        /**
         * Returns a dictionary of the function block types supported by this module.
         */
        daq::DictPtr<daq::IString, daq::IFunctionBlockType> onGetAvailableFunctionBlockTypes() override;

        /**
         * Creates and returns a new function block.
         *
         * @param id The identifier of the type of function block to create.
         * @param parent The component object which will contain the function block.
         * @param localId The local identifier of the function block.
         * @param config A property object containing configuration data for the function block.
         *
         * @throws daq::NotFoundException This module does not support the specified function
         *     block type.
         */
        daq::FunctionBlockPtr onCreateFunctionBlock(
            const daq::StringPtr& id,
            const daq::ComponentPtr& parent,
            const daq::StringPtr& localId,
            const daq::PropertyObjectPtr& config) override;

    private:

        daq::FunctionBlockTypePtr basicRecorderType;
};

END_NAMESPACE_OPENDAQ_BASIC_RECORDER_MODULE
