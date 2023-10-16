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
 * @defgroup types_function Function
 * @{
 */

/// Function pointer to callback.
typedef ErrCode (*FuncCall)(IBaseObject*, IBaseObject**);

static constexpr IntfID FuncGuid = { 0xCD7DE87D, 0xC267, 0x5736, { { 0xA6, 0xD0, 0x03, 0xC5, 0x6A, 0x9E, 0x20, 0x8A } } };

/*!
 * @brief Holds a callback function.
 *
 * Represents a callable object. The openDAQ SDK uses this interface when it needs to make
 * a call back to the client.
 *
 * Although the implementation of this interface is provided by openDAQ, C++ and other
 * bindings provide their implementation which allows passing function as a lambda
 * functions and other constructs.
 *  
 * Available factories:
 * @code
 * // Creates a new Function object. Throws exception if not successful.
 * IFunction* Function_Create(FuncCall value)
 *
 * // Creates a new Function object. Returns error code if not successful.
 * ErrCode createFunction(IFuncObject** obj, FuncCall value)
 * @endcode
 */
DECLARE_OPENDAQ_INTERFACE_EX(IFunction, IBaseObject)
{
    DEFINE_EXTERNAL_INTFID(FuncGuid)

    /*!
     * @brief Calls the stored callback.
     * @param params Parameters passed to the callback.
     * @param[out] result Return value of the callback.
     *
     * If the callback expects no parameters, the `params` parameter has to be `null`. If it
     * expects a single parameter, pass any openDAQ object as the `params` parameter.
     * If it expects multiple parameters, pass an IList<IBaseObject> as the `params` parameter.
     *
     */
    virtual ErrCode INTERFACE_FUNC call(IBaseObject* params, IBaseObject** result) = 0;
};

/*!@}*/

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(LIBRARY_FACTORY, Function, IFunction, createFunction, FuncCall, value)

END_NAMESPACE_OPENDAQ
