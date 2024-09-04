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


﻿namespace Daq.Core.Types;


/// <summary>
/// The function callback delegate.
/// </summary>
/// <remarks>
/// If the callback expects no parameters, the `params` parameter would be `null`.<br/>
/// If it expects a single parameter, any openDAQ object would be passed as the `params` parameter.<br/>
/// If it expects multiple parameters, an <c>IList&lt;IBaseObject&gt;</c> would be passed as the `params` parameter.
/// </remarks>
/// <param name="params">Parameters passed to the callback.</param>
/// <param name="result">Return value of the callback.</param>
/// <returns><see cref="ErrorCode.OPENDAQ_SUCCESS"/> when no error occurred; otherwise any other <see cref="ErrorCode"/>.</returns>
public delegate ErrorCode FuncCallDelegate(BaseObject @params, out BaseObject result);

/// <summary>
/// The procedure callback delegate.
/// </summary>
/// <remarks>
/// If the callback expects no parameters, the `params` parameter would be `null`.<br/>
/// If it expects a single parameter, any openDAQ object would be passed as the `params` parameter.<br/>
/// If it expects multiple parameters, an <c>IList&lt;IBaseObject&gt;</c> would be passed as the `params` parameter.
/// </remarks>
/// <param name="params">Parameters passed to the callback.</param>
/// <returns><see cref="ErrorCode.OPENDAQ_SUCCESS"/> when no error occurred; otherwise any other <see cref="ErrorCode"/>.</returns>
public delegate ErrorCode ProcCallDelegate(BaseObject @params);


//private (unmanaged) delegates for internal use (e.g. cannot send a managed object to C++)
public static partial class CoreTypesFactory
{
    //typedef ErrCode(*FuncCall)(IBaseObject*, IBaseObject**);
    [UnmanagedFunctionPointer(CallingConvention.StdCall)]
    private delegate ErrorCode FuncCall(IntPtr @params, out IntPtr result);

    //typedef ErrCode(*ProcCall)(IBaseObject*);
    [UnmanagedFunctionPointer(CallingConvention.StdCall)]
    private delegate ErrorCode ProcCall(IntPtr @params);
}
