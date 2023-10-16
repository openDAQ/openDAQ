/*
 * Copyright 2022-2023 Blueberry d.o.o.
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
#include <coretypes/common.h>
#include <coretypes/baseobject.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup types_functions
 * @defgroup types_procedure Procedure
 * @{
 */

/// Function pointer to callback without return value.
typedef ErrCode (*ProcCall)(IBaseObject*);

// Interface name changed, but GUID has to stay the same for backwards compatibility (Generated from IFuncObject.Core.RT.Dewesoft)
static constexpr IntfID ProcGuid = { 0xEB405ABE, 0x0DF0, 0x5808, { { 0x86, 0x42, 0xC0, 0x20, 0x69, 0x56, 0x7A, 0xDF } } };

/*!
 * @brief Holds a callback function without return value.
 *
 * Represents a callable object without return value. The openDAQ SDK uses this interface when
 * it needs to make a call back to the client.
 *
 * Although the implementation of this interface is provided by openDAQ, C++ and other
 * bindings provide their implementation which allows passing function as a lambda
 * functions and other constructs.
 *
 * Available factories:
 * @code
 * // Creates a new Procedure object. Throws exception if not successful.
 * IFunction* Procedure_Create(ProcCall value)
 *
 * // Creates a new Procedure object. Returns error code if not successful.
 * ErrCode createProcedure(IFuncObject** obj, ProcCall value)
 * @endcode
 */
DECLARE_OPENDAQ_INTERFACE_EX(IProcedure, IBaseObject)
{
    DEFINE_EXTERNAL_INTFID(ProcGuid)

    /*!
     * @brief Calls the stored callback.
     * @param params Parameters passed to the callback.
     *
     * If the callback expects no parameters, the `params` parameter has to be `nullptr`. If it
     * expects a single parameter, pass any openDAQ object as the `params` parameter.
     * If it expects multiple parameters, pass an IList<IBaseObject> as the `params` parameter.
     *
     */
    virtual ErrCode INTERFACE_FUNC dispatch(IBaseObject * params) = 0;
};

/*!@}*/

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(LIBRARY_FACTORY, Procedure, IProcedure, createProcedure, ProcCall, value)

END_NAMESPACE_OPENDAQ
