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


﻿namespace Daq.Core.Types;


#nullable enable annotations

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
public delegate ErrorCode FuncCallDelegate(BaseObject? @params, out BaseObject? result);

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
public delegate ErrorCode ProcCallDelegate(BaseObject? @params);

#nullable restore annotations


//private (unmanaged) delegates and helper functions for internal use (e.g. cannot send a managed object to C++)
public static partial class CoreTypesFactory
{
    //typedef ErrCode(*FuncCall)(IBaseObject*, IBaseObject**);
    [UnmanagedFunctionPointer(CallingConvention.StdCall)]
    private delegate ErrorCode FuncCall(IntPtr @params, out IntPtr result);

    //typedef ErrCode(*ProcCall)(IBaseObject*);
    [UnmanagedFunctionPointer(CallingConvention.StdCall)]
    private delegate ErrorCode ProcCall(IntPtr @params);


    /// <summary>
    /// Creates a <see cref="FuncCall"/> wrapper for a <see cref="FuncCallDelegate"/> for native use
    /// because managed openDAQ objects cannot be marshaled to C++.
    /// </summary>
    /// <param name="funcCallDelegate">The procedure call delegate.</param>
    /// <returns>The wrapped procedure call delegate for native use.</returns>
    private static FuncCall CreateFuncCallWrapper(FuncCallDelegate funcCallDelegate)
    {
        return (IntPtr @params, out IntPtr result) =>
        {
            BaseObject paramsObject = null;

            if (@params != IntPtr.Zero)
            {
                paramsObject = new BaseObject(@params, true);
            }

            //call the managed callback with the managed parameters object
            var errorCode = funcCallDelegate(paramsObject, out BaseObject resultObject);

            //get the result pointer (if there was a result)
            result = resultObject;

            //prevent from releasing the reference in managed resultObject destruction
            //as we hand it over to C++ in the result above
            resultObject?.SetNativePointerToZero();
            resultObject?.Dispose();

            return errorCode;
        };
    }

    /// <summary>
    /// Creates a <see cref="ProcCall"/> wrapper for a <see cref="ProcCallDelegate"/> for native use
    /// because managed openDAQ objects cannot be marshaled to C++.
    /// </summary>
    /// <param name="procCallDelegate">The procedure call delegate.</param>
    /// <returns>The wrapped procedure call delegate for native use.</returns>
    private static ProcCall CreateProcCallWrapper(ProcCallDelegate procCallDelegate)
    {
        return (IntPtr @params) =>
        {
            BaseObject paramsObject = null;

            if (@params != IntPtr.Zero)
            {
                paramsObject = new BaseObject(@params, true);
            }

            //call the managed callback with the managed parameters object
            return procCallDelegate(paramsObject);
        };
    }
}
