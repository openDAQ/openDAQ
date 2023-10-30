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


namespace Daq.Core.Types;


public enum ErrorCode : uint
{
    OPENDAQ_SUCCESS                      = 0x00000000u,
    OPENDAQ_TRUE                         = 0x00000000u,
    OPENDAQ_FALSE                        = 0x00000001u,
    OPENDAQ_LOWER                        = 0x00000002u,
    OPENDAQ_EQUAL                        = 0x00000003u,
    OPENDAQ_GREATER                      = 0x00000004u,
    OPENDAQ_NO_MORE_ITEMS                = 0x00000005u,
    OPENDAQ_IGNORED                      = 0x00000006u,
    OPENDAQ_ERR_NOMEMORY                 = 0x80000000u,
    OPENDAQ_ERR_INVALIDPARAMETER         = 0x80000001u,
    OPENDAQ_ERR_NOINTERFACE              = 0x80004002u,
    OPENDAQ_ERR_SIZETOOSMALL             = 0x80000003u,
    OPENDAQ_ERR_CONVERSIONFAILED         = 0x80000004u,
    OPENDAQ_ERR_OUTOFRANGE               = 0x80000005u,
    OPENDAQ_ERR_NOTFOUND                 = 0x80000006u,
    OPENDAQ_ERR_NOMOREITEMS              = 0x80000009u,
    OPENDAQ_ERR_ALREADYEXISTS            = 0x8000000Au,
    OPENDAQ_ERR_NOTASSIGNED              = 0x8000000Bu,
    OPENDAQ_ERR_CALLFAILED               = 0x8000000Cu,
    OPENDAQ_ERR_PARSEFAILED              = 0x8000000Du,
    OPENDAQ_ERR_INVALIDVALUE             = 0x8000000Eu,
    OPENDAQ_ERR_RESOLVEFAILED            = 0x80000010u,
    OPENDAQ_ERR_INVALIDTYPE              = 0x80000011u,
    OPENDAQ_ERR_ACCESSDENIED             = 0x80000012u,
    OPENDAQ_ERR_NOTENABLED               = 0x80000013u,
    OPENDAQ_ERR_GENERALERROR             = 0x80000014u,
    OPENDAQ_ERR_CALCFAILED               = 0x80000015u,
    OPENDAQ_ERR_NOTIMPLEMENTED           = 0x80000016u,
    OPENDAQ_ERR_FROZEN                   = 0x80000017u,
    OPENDAQ_ERR_NOT_SERIALIZABLE         = 0x80000018u,
    OPENDAQ_ERR_FACTORY_NOT_REGISTERED   = 0x80000020u,
    OPENDAQ_ERR_DESERIALIZE_PARSE_ERROR  = 0x80000021u,
    OPENDAQ_ERR_DESERIALIZE_UNKNOWN_TYPE = 0x80000022u,
    OPENDAQ_ERR_DESERIALIZE_NO_TYPE      = 0x80000023u,
    OPENDAQ_ERR_INVALIDPROPERTY          = 0x80000024u,
    OPENDAQ_ERR_DUPLICATEITEM            = 0x80000025u,
    OPENDAQ_ERR_ARGUMENT_NULL            = 0x80000026u,
    OPENDAQ_ERR_INVALID_OPERATION        = 0x80000027u,
    OPENDAQ_ERR_UNINITIALIZED            = 0x80000028u,
    OPENDAQ_ERR_INVALIDSTATE             = 0x80000029u,
    OPENDAQ_ERR_VALIDATE_FAILED          = 0x80000030u,
    OPENDAQ_ERR_NOT_UPDATABLE            = 0x80000031u,
    OPENDAQ_ERR_NO_COMPATIBLE_VERSION    = 0x80000032u,
    OPENDAQ_ERR_LOCKED                   = 0x80000033u,
    OPENDAQ_ERR_SIZETOOLARGE             = 0x80000034u,
    OPENDAQ_ERR_BUFFERFULL               = 0x80000035u,
    OPENDAQ_ERR_CREATE_FAILED            = 0x80000036u,
    OPENDAQ_ERR_EMPTY_SCALING_TABLE      = 0x80000037u,
    OPENDAQ_ERR_EMPTY_RANGE              = 0x80000038u,
    OPENDAQ_ERR_DISCOVERY_FAILED         = 0x80000039u,
    OPENDAQ_ERR_COERCE_FAILED            = 0x80000040u,
    OPENDAQ_ERR_NOT_SUPPORTED            = 0x80000041u
}

public static class Result
{
    public static bool Succeeded(ErrorCode errorCode)
    {
        return (uint)errorCode < 0x80000000u;
    }

    public static bool Failed(ErrorCode errorCode)
    {
        return !Succeeded(errorCode);
    }
}
