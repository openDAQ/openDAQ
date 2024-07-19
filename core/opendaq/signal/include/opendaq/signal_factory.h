/*
 * Copyright 2022-2024 openDAQ d.o.o.
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
#include <opendaq/signal_config_ptr.h>
#include <opendaq/context_ptr.h>
#include <opendaq/data_descriptor_factory.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_signal
 * @addtogroup opendaq_signal_factories Factories
 * @{
 */

/*!
 * @brief Creates a new Signal with a given Context and Descriptor, as well as an optional uniqueId.
 * @param context The Context. Most often the creating function-block/device passes its own Context to the Signal.
 * @param descriptor The Signal-descriptor.
 * @param parent <description-missing>
 * @param localId <description-missing>
 * @param className <description-missing>
 */
inline SignalConfigPtr SignalWithDescriptor(const ContextPtr& context,
                                            const DataDescriptorPtr& descriptor,
                                            const ComponentPtr& parent,
                                            const StringPtr& localId,
                                            const StringPtr& className = nullptr)
{
    SignalConfigPtr obj(SignalWithDescriptor_Create(context, descriptor, parent, localId, className));
    return obj;
}

/*!
 * @brief Creates a new Signal with a given Context, as well as an optional uniqueId. The created Signal has no Descriptor.
 * @param context The Context. Most often the creating function-block/device passes its own Context to the Signal.
 * @param parent <description-missing>
 * @param localId <description-missing>
 * @param className <description-missing>
 */
inline SignalConfigPtr Signal(const ContextPtr& context,
                              const ComponentPtr& parent,
                              const StringPtr& localId,
                              const StringPtr& className = nullptr)
{
    SignalConfigPtr obj(Signal_Create(context, parent, localId, className));
    return obj;
}

/*!@}*/

END_NAMESPACE_OPENDAQ
