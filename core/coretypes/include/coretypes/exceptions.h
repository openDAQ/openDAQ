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
#include <coretypes/bb_exception.h>
#include <coretypes/common.h>
#include <coretypes/error_code_to_exception.h>
#include <coretypes/errors.h>
#include <cstdio>
#include <stdexcept>
#include <string>

BEGIN_NAMESPACE_OPENDAQ

#define DEFINE_EXCEPTION_BASE(excBase, excName, errCode, excMsg)                                 \
    class excName##Exception : public excBase                                                    \
    {                                                                                            \
    public:                                                                                      \
        explicit excName##Exception()                                                            \
            : excBase(true, errCode, excMsg)                                                     \
        {                                                                                        \
        }                                                                                        \
        template <typename... Params>                                                            \
        explicit excName##Exception(const std::string& msg, Params&&... params)                  \
            : excBase(errCode, msg, std::forward<Params>(params)...)                             \
        {                                                                                        \
        }                                                                                        \
        template <typename... Params>                                                            \
        explicit excName##Exception(daq::ErrCode err, const std::string& msg, Params&&... params) \
            : excBase(errCode, msg, std::forward<Params>(params)...)                             \
        {                                                                                        \
        }                                                                                        \
    };                                                                                           \
    OPENDAQ_REGISTER_ERRCODE_EXCEPTION(errCode, excName##Exception)

#define OPENDAQ_REGISTER_ERRCODE_EXCEPTION(errCode, type)                                                \
    class Exception##type##Factory                                                                  \
    {                                                                                               \
    public:                                                                                         \
        Exception##type##Factory()                                                                  \
        {                                                                                           \
            daq::ErrorCodeToException::GetInstance()->registerException<type>(errCode);              \
        }                                                                                           \
    };                                                                                              \
                                                                                                    \
    namespace Detail::Initialization                                                                \
    {                                                                                               \
        inline Exception##type##Factory exception##type##Factory;                                   \
    }

#define DEFINE_EXCEPTION(excName, errCode, excMsg) DEFINE_EXCEPTION_BASE(daq::DaqException, excName, errCode, excMsg)

#define OPENDAQ_TRY(expression)            \
    try                                    \
    {                                      \
        expression                         \
    }                                      \
    catch (const DaqException& e)          \
    {                                      \
        return errorFromException(e);      \
    }                                      \
    catch (const std::exception& e)        \
    {                                      \
        return errorFromException(e);      \
    }                                      \
    catch (...)                            \
    {                                      \
        return OPENDAQ_ERR_GENERALERROR;   \
    }

/*
 * Should be in the order of the error's numerical value
 */

DEFINE_EXCEPTION(NoMemory, OPENDAQ_ERR_NOMEMORY, "No memory")
DEFINE_EXCEPTION(InvalidParameter, OPENDAQ_ERR_INVALIDPARAMETER, "Invalid parameter")
DEFINE_EXCEPTION(NoInterface, OPENDAQ_ERR_NOINTERFACE, "Invalid cast. The object does not implement this interface.")
DEFINE_EXCEPTION(SizeTooSmall, OPENDAQ_ERR_SIZETOOSMALL, "Size too small")
DEFINE_EXCEPTION(ConversionFailed, OPENDAQ_ERR_CONVERSIONFAILED, "Conversion failed")
DEFINE_EXCEPTION(OutOfRange, OPENDAQ_ERR_OUTOFRANGE, "Out of range")
DEFINE_EXCEPTION(NotFound, OPENDAQ_ERR_NOTFOUND, "Not found")
DEFINE_EXCEPTION(AlreadyExists, OPENDAQ_ERR_ALREADYEXISTS, "Already exists")
DEFINE_EXCEPTION(NotAssigned, OPENDAQ_ERR_NOTASSIGNED, "Not assigned")
DEFINE_EXCEPTION(CallFailed, OPENDAQ_ERR_CALLFAILED, "Call failed")
DEFINE_EXCEPTION(ParseFailed, OPENDAQ_ERR_PARSEFAILED, "Parse failed")
DEFINE_EXCEPTION(InvalidValue, OPENDAQ_ERR_INVALIDVALUE, "Invalid value")
DEFINE_EXCEPTION(ResolveFailed, OPENDAQ_ERR_RESOLVEFAILED, "Resolve failed")
DEFINE_EXCEPTION(InvalidType, OPENDAQ_ERR_INVALIDTYPE, "Invalid type")
DEFINE_EXCEPTION(AccessDenied, OPENDAQ_ERR_ACCESSDENIED, "Access denied")
DEFINE_EXCEPTION(NotEnabled, OPENDAQ_ERR_NOTENABLED, "Not enabled")
// DEFINE_EXCEPTION(CalcFailed, OPENDAQ_ERR_CALCFAILED, "Calculation failed")
DEFINE_EXCEPTION(NotImplemented, OPENDAQ_ERR_NOTIMPLEMENTED, "Not Implemented")
DEFINE_EXCEPTION(Frozen, OPENDAQ_ERR_FROZEN, "Object frozen")
DEFINE_EXCEPTION(NotFrozen, OPENDAQ_ERR_NOT_FROZEN, "Object is not frozen")
DEFINE_EXCEPTION(NotSerializable, OPENDAQ_ERR_NOT_SERIALIZABLE, "Not serializable")
DEFINE_EXCEPTION(Deserialize, OPENDAQ_ERR_DESERIALIZE_PARSE_ERROR, "Error when parsing or deserializing")
DEFINE_EXCEPTION(InvalidProperty, OPENDAQ_ERR_INVALIDPROPERTY, "Invalid property")
DEFINE_EXCEPTION(DuplicateItem, OPENDAQ_ERR_DUPLICATEITEM, "Duplicate item")
DEFINE_EXCEPTION(ArgumentNull, OPENDAQ_ERR_ARGUMENT_NULL, "Argument must not be NULL.")
DEFINE_EXCEPTION(InvalidOperation, OPENDAQ_ERR_INVALID_OPERATION, "Operation in not valid for the current type or state.")
DEFINE_EXCEPTION(Uninitialized, OPENDAQ_ERR_UNINITIALIZED, "The operation requires initialization")
DEFINE_EXCEPTION(InvalidState, OPENDAQ_ERR_INVALIDSTATE, "Invalid state")
DEFINE_EXCEPTION(ValidateFailed, OPENDAQ_ERR_VALIDATE_FAILED, "Validate failed")
DEFINE_EXCEPTION(NotUpdatable, OPENDAQ_ERR_NOT_UPDATABLE, "Not updatable")
DEFINE_EXCEPTION(NotCompatibleVersion, OPENDAQ_ERR_NO_COMPATIBLE_VERSION, "Not compatible version")
DEFINE_EXCEPTION(Locked, OPENDAQ_ERR_LOCKED, "Locked")
DEFINE_EXCEPTION(SizeTooLarge, OPENDAQ_ERR_SIZETOOLARGE, "Size too large")
DEFINE_EXCEPTION(BufferFull, OPENDAQ_ERR_BUFFERFULL, "Buffer full")
DEFINE_EXCEPTION(EmptyScalingTable, OPENDAQ_ERR_EMPTY_SCALING_TABLE, "Scaling table must not be empty")
DEFINE_EXCEPTION(EmptyRange, OPENDAQ_ERR_EMPTY_RANGE, "Scaling range must not be empty")
DEFINE_EXCEPTION(CreateFailed, OPENDAQ_ERR_CREATE_FAILED, "Failed to create object")
DEFINE_EXCEPTION(GeneralError, OPENDAQ_ERR_GENERALERROR, "General error")
DEFINE_EXCEPTION(DiscoveryFailed, OPENDAQ_ERR_DISCOVERY_FAILED, "Device discovery failed")
DEFINE_EXCEPTION(CoerceFailed, OPENDAQ_ERR_COERCE_FAILED, "Coercing failed")
DEFINE_EXCEPTION(NotSupported, OPENDAQ_ERR_NOT_SUPPORTED, "The operation or type is not supported")
DEFINE_EXCEPTION(ListNotHomogeneous, OPENDAQ_ERR_LIST_NOT_HOMOGENEOUS, "List is not homogeneous")
DEFINE_EXCEPTION(FactoryNotRegistered, OPENDAQ_ERR_FACTORY_NOT_REGISTERED, "Factory not registered")

extern void checkErrorInfo(ErrCode errCode);

[[noreturn]] inline void throwExceptionFromErrorCode(ErrCode errCode, const std::string& msg = "")
{
    IExceptionFactory* fact = ErrorCodeToException::GetInstance()->getExceptionFactory(errCode);
    fact->throwException(errCode, msg);

    // Unreachable code but prevents GCC from emitting a warning that a [[noreturn]] function actually returns
    throw std::runtime_error(msg + " (" + std::to_string(errCode) + ")");
}

END_NAMESPACE_OPENDAQ
