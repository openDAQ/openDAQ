/*
 * Copyright 2022-2025 openDAQ d.o.o.
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

#pragma once

#include <opendaq/module_impl.h>
#include <opendaq/opendaq.h>

#include <basic_recorder_module/common.h>

BEGIN_NAMESPACE_OPENDAQ_BASIC_RECORDER_MODULE

/**
 * An openDAQ module which exposes a single `FunctionBlock` object, BasicRecorderImpl.
 */
class BasicRecorderModule final : public Module
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
        BasicRecorderModule(ContextPtr context);

        /**
         * Returns a dictionary of the function block types supported by this module.
         */
        DictPtr<IString, IFunctionBlockType> onGetAvailableFunctionBlockTypes() override;

        /**
         * Creates and returns a new function block.
         *
         * @param id The identifier of the type of function block to create.
         * @param parent The component object which will contain the function block.
         * @param localId The local identifier of the function block.
         * @param config A property object containing configuration data for the function block.
         *
         * @throws NotFoundException This module does not support the specified function block
         *     type.
         */
        FunctionBlockPtr onCreateFunctionBlock(
            const StringPtr& id,
            const ComponentPtr& parent,
            const StringPtr& localId,
            const PropertyObjectPtr& config) override;

    private:

        FunctionBlockTypePtr basicRecorderType;
};

END_NAMESPACE_OPENDAQ_BASIC_RECORDER_MODULE
