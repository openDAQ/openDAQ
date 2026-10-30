/*
 * Copyright 2022-2026 openDAQ d.o.o.
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
#include <coretypes/baseobject.h>
#include <coretypes/function.h>
#include <coretypes/listobject.h>
#include <opendaq/component.h>
#include <opendaq/sample_type.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_readers
 * @addtogroup opendaq_multi_reader2 Multi reader 2
 * @{
 */

/*!
 * @brief What an input in error does to the reader.
 */
enum class MultiReader2ErrorPolicy : EnumType
{
    Invalidate = 0,  ///< The reader is invalid while any input is in error.
    Exclude          ///< An input in error is set aside until healthy; the reader is invalid only when none is healthy.
};

/*!
 * @brief The setup a reader applies at construction and on `configure`. The reader keeps no reference to the object.
 */
DECLARE_OPENDAQ_INTERFACE(IMultiReader2Params, IBaseObject)
{
    // [templateType(inputs, IComponent)]
    /*!
     * @brief Gets the inputs in slot order: all signals or all input ports, without duplicates.
     *
     * An empty list is a valid reader without a main input: its status is Ok, and every read returns count 0. The reader
     * is the listener of every listed port, so the owner's listener receives no connect or disconnect for them; a
     * disconnect shows as Disconnected on the next status. A port the owner wants to hear about stays outside the list
     * until a signal is connected to it.
     */
    virtual ErrCode INTERFACE_FUNC getInputs(IList** inputs) = 0;
    // [templateType(inputs, IComponent)]
    virtual ErrCode INTERFACE_FUNC setInputs(IList* inputs) = 0;

    /*!
     * @brief Gets the input everything aligns to.
     *
     * Null: the reader picks the first input without an error and keeps it until it errors or the reader is
     * configured. Set: pinned; it must be in the list, and its error invalidates the reader.
     */
    virtual ErrCode INTERFACE_FUNC getMainInput(IComponent** input) = 0;
    virtual ErrCode INTERFACE_FUNC setMainInput(IComponent* input) = 0;

    /*!
     * @brief Gets the owner's judgement of an input's descriptors; null accepts all.
     *
     * Called with (input, value descriptor, domain descriptor, value descriptors) and returns a Bool; a rejection, or an
     * exception, is ValueDescriptorInvalid. The last argument holds every input's value descriptor in force, in slot
     * order, null for an input that is not connected, so a verdict can depend on the other inputs. Asked inside `read`
     * and `configure` on the caller's thread whenever a descriptor takes effect, and for every other input when a value
     * descriptor or a connection changes. A verdict that depends on the owner's own state needs a `configure` from
     * outside the judgement when that state changes. It runs while the reader holds its internal lock, and
     * during `configure` the intake locks of newly added ports: it must not call the reader (INVALIDSTATE) or wait on
     * a lock that a thread calling the reader may hold.
     */
    virtual ErrCode INTERFACE_FUNC getAcceptsDescriptor(IFunction** acceptsDescriptor) = 0;
    virtual ErrCode INTERFACE_FUNC setAcceptsDescriptor(IFunction* acceptsDescriptor) = 0;

    /*!
     * @brief Gets the sample type of the read buffers; Float64 by default.
     *
     * Every input converts element-wise with post scaling applied, and one that cannot is ValueDescriptorInvalid.
     * Undefined delivers each input scaled, in its own sample type.
     */
    virtual ErrCode INTERFACE_FUNC getValueReadType(SampleType* valueReadType) = 0;
    virtual ErrCode INTERFACE_FUNC setValueReadType(SampleType valueReadType) = 0;

    /*!
     * @brief Gets the smallest count a read returns: at least 1, 1 by default.
     */
    virtual ErrCode INTERFACE_FUNC getMinReadCount(SizeT* count) = 0;
    virtual ErrCode INTERFACE_FUNC setMinReadCount(SizeT count) = 0;

    /*!
     * @brief Gets the error policy; Invalidate by default.
     *
     * The reader is valid while it has a healthy main input: a pinned main input must be healthy, and an automatic
     * one can be any healthy input. Under Invalidate every input must be healthy as well.
     */
    virtual ErrCode INTERFACE_FUNC getErrorPolicy(MultiReader2ErrorPolicy* policy) = 0;
    virtual ErrCode INTERFACE_FUNC setErrorPolicy(MultiReader2ErrorPolicy policy) = 0;
};

/*!@}*/

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, MultiReader2Params, IMultiReader2Params)

END_NAMESPACE_OPENDAQ
