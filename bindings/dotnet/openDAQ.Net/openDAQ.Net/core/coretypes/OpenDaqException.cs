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

﻿

namespace Daq.Core.Types;


/// <inheritdoc/>
/// <remarks>
/// <u>Example:</u>
/// <code>
/// try
/// {
///     //failing function call on an openDAQ object
/// }
/// catch (OpenDaqException ex)
/// {
///     Debug.Print("An {0} error occurred - {1}", ex.ErrorCode, ex.ToString());
///     Debug.Print("Message: {0}", ex.ErrorInfo?.GetMessage() ?? ex.Message ?? "not specified");
/// }
/// </code>
/// </remarks>
[Serializable]
public class OpenDaqException : Exception
{
    private ErrorCode _errorCode = ErrorCode.OPENDAQ_ERR_GENERALERROR;

    /// <inheritdoc/>
    public OpenDaqException() { }
    /// <inheritdoc/>
    public OpenDaqException(string message) : base(message) { }
    /// <inheritdoc/>
    public OpenDaqException(string message, Exception inner) : base(message, inner) { }
    /// <inheritdoc/>
    protected OpenDaqException(System.Runtime.Serialization.SerializationInfo info,
                               System.Runtime.Serialization.StreamingContext context) : base(info, context) { }
    public OpenDaqException(ErrorCode errorCode) : this(errorCode, CoreTypesFactory.DaqGetErrorInfo()?.GetMessage() ?? "n/a", null) { }
    public OpenDaqException(ErrorCode errorCode, string message) : this(errorCode, message, null) { }
    public OpenDaqException(ErrorCode errorCode, string message, Exception inner) : base(message, inner)
    {
        this._errorCode = errorCode;
    }

    /// <inheritdoc/>
    public override string Message => this.ToString();

    /// <inheritdoc/>
    public override string ToString()
    {
        if (string.IsNullOrWhiteSpace(base.Message))
            return _errorCode.ToString();

        return $"{_errorCode}: {base.Message}";
    }

    /// <summary>
    /// Gets the error code.
    /// </summary>
    /// <value>
    /// The <see cref="Daq.Core.Types.ErrorCode"/>.
    /// </value>
    public ErrorCode ErrorCode => _errorCode;

    /// <summary>
    /// Gets the internal error information from the SDK.
    /// </summary>
    /// <value>
    /// The error information or <c>null</c> if not available.
    /// </value>
    public ErrorInfo ErrorInfo => CoreTypesFactory.DaqGetErrorInfo();
}
