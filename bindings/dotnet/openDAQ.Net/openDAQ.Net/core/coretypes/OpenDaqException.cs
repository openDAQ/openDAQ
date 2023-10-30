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


[Serializable]
public class OpenDaqException : Exception
{
    private ErrorCode _errorCode = ErrorCode.OPENDAQ_ERR_GENERALERROR;

    public OpenDaqException() { }
    public OpenDaqException(string message) : base(message) { }
    public OpenDaqException(string message, Exception inner) : base(message, inner) { }
    protected OpenDaqException(System.Runtime.Serialization.SerializationInfo info,
                               System.Runtime.Serialization.StreamingContext context) : base(info, context) { }
    public OpenDaqException(ErrorCode errorCode) : this(errorCode, null, null) { }
    public OpenDaqException(ErrorCode errorCode, string message) : this(errorCode, message, null) { }
    public OpenDaqException(ErrorCode errorCode, string message, Exception inner) : base(message, inner)
    {
        this._errorCode = errorCode;
    }

    public override string Message => this.ToString();

    public override string ToString()
    {
        if (string.IsNullOrWhiteSpace(base.Message))
            return _errorCode.ToString();

        return $"{_errorCode}: {base.Message}";
    }
}
