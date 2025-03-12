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

#pragma once
#include <functional>
#include <coretypes/common.h>
#include <coretypes/mem.h>
#include <regex>
#include <sstream>
#include <utility>
#include <coretypes/errorinfo.h>
#include <coretypes/baseobject.h>
#include <coretypes/stringobject.h>
#include <numeric>

#include "xxhash.h"

template <>
struct std::hash<daq::IntfID>
{
    using argument_type = daq::IntfID;
    using result_type = std::size_t;

    static constexpr XXH32_hash_t Seed = 123456789;

    result_type operator()(const argument_type& id) const noexcept
    {
        return sizeof(std::size_t) == 4 ? XXH32(&id, sizeof(daq::IntfID), Seed)
                                        : XXH3_64bits_withSeed(&id, sizeof(daq::IntfID), Seed);
    }
};

BEGIN_NAMESPACE_OPENDAQ

class Finally
{
public:
    Finally() = delete;
    Finally(const Finally& other) = delete;

    Finally(std::function<void()> finalizer)
        : finalizer(std::move(finalizer))
    {
    }

    ~Finally()
    {
        if (finalizer)
            finalizer();
    }

private:
    std::function<void()> finalizer;
};

inline std::string objectToString(IBaseObject* object)
{
    CharPtr s;
    std::ostringstream stream;

    if (object == nullptr)
    {
        stream << "null";
    }
    else
    {
        auto errCode = object->toString(&s);
        if (OPENDAQ_SUCCEEDED(errCode))
        {
            stream << s;
            daqFreeMemory(s);
        }
        else
            stream << "Unknown";
    }
    return stream.str();
}

inline std::ostringstream& ErrorFormat(std::ostringstream& ss, IErrorInfo* errorInfo)
{
    if (errorInfo == nullptr)
        return ss;

#ifndef NDEBUG
    ConstCharPtr fileNameCharPtr;
    Int fileLine = -1;
    
    errorInfo->getFileName(&fileNameCharPtr);
    errorInfo->getFileLine(&fileLine);

    if (fileNameCharPtr != nullptr)
    {
        ss << "[ " << fileNameCharPtr;
        if (fileLine != -1)
            ss << ":" << fileLine;
        ss << " ] : ";
    }
#endif

    IString* message;
    errorInfo->getMessage(&message);

    if (message != nullptr)
    {
        ConstCharPtr msgCharPtr;
        message->getCharPtr(&msgCharPtr);

        if (msgCharPtr != nullptr)
            ss << msgCharPtr;
        
        message->releaseRef();
    }
    
    return ss;
}

inline void checkErrorInfo(ErrCode errCode)
{
    if (OPENDAQ_FAILED(errCode))
    {
        IList* errorInfoList;
        daqGetErrorInfoList(&errorInfoList);

        std::ostringstream ss;
        if (errorInfoList != nullptr)
        {
            SizeT count = 0;
            errorInfoList->getCount(&count);

            // print last error
            {
                IBaseObject* errorInfoObj;
                errorInfoList->getItemAt(count - 1, &errorInfoObj);

                IErrorInfo* errorInfo;
                errorInfoObj->borrowInterface(IErrorInfo::Id, reinterpret_cast<void**>(&errorInfo));
            
                if (errorInfo != nullptr)
                {
                    IString* message;
                    errorInfo->getMessage(&message);
                    if (message != nullptr)
                    {
                        ConstCharPtr msgCharPtr;
                        message->getCharPtr(&msgCharPtr);
                        if (msgCharPtr != nullptr)
                            ss << msgCharPtr;
                        message->releaseRef();
                    }
                }
                if (errorInfoObj != nullptr)
                    errorInfoObj->releaseRef();
            }
    
            // print traceback
            ss << "\n\nTraceback (most recent call last):";
            for (SizeT i = 0; i < count; ++i)
            {
                IBaseObject* errorInfoObj;
                errorInfoList->getItemAt(i, &errorInfoObj);

                IErrorInfo* errorInfo;
                errorInfoObj->borrowInterface(IErrorInfo::Id, reinterpret_cast<void**>(&errorInfo));
            
                if (errorInfo != nullptr)
                {
                    ss << "\n";
                    ErrorFormat(ss, errorInfo);
                }
                if (errorInfoObj != nullptr)
                    errorInfoObj->releaseRef();
            }
        }

        if (errorInfoList != nullptr)
            errorInfoList->releaseRef();

        throwExceptionFromErrorCode(errCode, ss.str());
    }
}

template <typename... Params>
ErrCode makeErrorInfo(ErrCode errCode, const std::string& message, IBaseObject* source, Params... params)
{
    setErrorInfoWithSource(source, message, std::forward<Params>(params)...);
    return errCode;
}

#ifdef NDEBUG
    #define DAQ_MAKE_ERROR_INFO(errCode, message, ...) \
        daq::makeErrorInfo(errCode, message, nullptr, ##__VA_ARGS__)
#else
    template <typename... Params>
    ErrCode makeErrorInfo(ConstCharPtr fileName, Int fileLine, ErrCode errCode, const std::string& message, IBaseObject* source, Params... params)
    {
        setErrorInfoWithSource(fileName, fileLine, source, message, std::forward<Params>(params)...);
        return errCode;
    }
    #define DAQ_MAKE_ERROR_INFO(errCode, message, ...) \
        daq::makeErrorInfo(__FILE__, __LINE__, errCode, message, nullptr, ##__VA_ARGS__)
#endif

inline ErrCode errorFromException(const DaqException& e, IBaseObject* source = nullptr)
{
#ifdef NDEBUG
    return makeErrorInfo(e.getErrCode(), e.what(), source);
#else
    return makeErrorInfo(e.getFileName(), e.getFileLine(), e.getErrCode(), e.what(), source);
#endif
}

inline ErrCode errorFromException(const std::exception& e, IBaseObject* source = nullptr, ErrCode errCode = OPENDAQ_ERR_GENERALERROR)
{
    return makeErrorInfo(errCode, e.what(), source);
}

#ifdef NDEBUG
    #define DAQ_ERROR_FROM_STD_EXCEPTION(e, source, errCode) daq::errorFromException(e, source, errCode)
#else
    inline ErrCode errorFromException(ConstCharPtr fileName, Int fileLine, const std::exception& e, IBaseObject* source = nullptr, ErrCode errCode = OPENDAQ_ERR_GENERALERROR)
    {
        return makeErrorInfo(fileName, fileLine, errCode, e.what(), source);
    }
    #define DAQ_ERROR_FROM_STD_EXCEPTION(e, source, errCode) daq::errorFromException(__FILE__, __LINE__, e, source, errCode)
#endif

template <typename... Params>
void setErrorInfoWithSource(IBaseObject* source, const std::string& message, Params... params)
{
    IErrorInfo* errorInfo;
    auto err = createErrorInfoObjectWithSource(&errorInfo, source, message, std::forward<Params>(params)...);
    if (OPENDAQ_FAILED(err))
        return;

    daqSetErrorInfo(errorInfo);
    errorInfo->releaseRef();
}

#ifndef NDEBUG
    template <typename... Params>
    void setErrorInfoWithSource(ConstCharPtr fileName, Int fileLine, IBaseObject* source, const std::string& message, Params... params)
    {
        IErrorInfo* errorInfo;
        auto err = createErrorInfoObjectWithSource(&errorInfo, fileName, fileLine, source, message, std::forward<Params>(params)...);
        if (OPENDAQ_FAILED(err))
            return;

        daqSetErrorInfo(errorInfo);
        errorInfo->releaseRef();
    }
#endif

template <typename... Params>
ErrCode static createErrorInfoObjectWithSource(IErrorInfo** errorInfo, IBaseObject* sourceObj, const std::string& message, Params... params)
{
    if (errorInfo == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

#if defined(__GNUC__)
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wformat-security"
#endif

    IErrorInfo* errorInfo_ = nullptr;
    IString* msg = nullptr;
    IString* source = nullptr;
    Finally final([&errorInfo_, &msg, &source]
    {
        if (errorInfo_ != nullptr)
            errorInfo_->releaseRef();
        if (msg != nullptr)
            msg->releaseRef();
        if (source != nullptr)
            source->releaseRef();
    });

    auto err = createErrorInfo(&errorInfo_);
    if (OPENDAQ_FAILED(err))
        return err;

    if constexpr (sizeof...(params) == 0)
    {
        err = createString(&msg, message.c_str());
    }
    else
    {
        char errorMsg[1024];
#if defined(__STDC_SECURE_LIB__) || defined(__STDC_LIB_EXT1__)
        sprintf_s(errorMsg, sizeof(errorMsg) / sizeof(char), message.c_str(), params...);
#else
        snprintf(errorMsg, sizeof(errorMsg) / sizeof(char), message.c_str(), params...);
#endif

        err = createString(&msg, errorMsg);
    }

    if (OPENDAQ_FAILED(err))
        return err;

    err = errorInfo_->setMessage(msg);
    if (OPENDAQ_FAILED(err))
        return err;

    if (sourceObj)
    {
        err = createString(&source, objectToString(sourceObj).c_str());
        if (OPENDAQ_FAILED(err))
            return err;

        err = errorInfo_->setSource(source);
        if (OPENDAQ_FAILED(err))
            return err;
    }

    errorInfo_->addRef();
    *errorInfo = errorInfo_;

#if defined(__GNUC__)
    #pragma GCC diagnostic pop
#endif

    return OPENDAQ_SUCCESS;
}

#ifndef NDEBUG
    template <typename... Params>
    ErrCode static createErrorInfoObjectWithSource(IErrorInfo** errorInfo, ConstCharPtr fileName, Int fileLine, IBaseObject* sourceObj, const std::string& message, Params... params)
    {
        ErrCode errCode = createErrorInfoObjectWithSource(errorInfo, sourceObj, message, std::forward<Params>(params)...);
        if (OPENDAQ_FAILED(errCode))
            return errCode;

        IErrorInfo* errorInfo_ = *errorInfo;

        errCode = errorInfo_->setFileName(fileName);
        if (OPENDAQ_FAILED(errCode))
            return errCode;

        errCode = errorInfo_->setFileLine(fileLine);
        if (OPENDAQ_FAILED(errCode))
            return errCode;

        return OPENDAQ_SUCCESS;
    }
#endif

inline std::string toStdString(IString* rtStr)
{
    if (rtStr == nullptr)
        DAQ_THROW_EXCEPTION(InvalidParameterException, "Parameter must not be null");

    ConstCharPtr ptr;
    ErrCode err = rtStr->getCharPtr(&ptr);
    checkErrorInfo(err);

    return { ptr };
}

template <typename U, typename OBJ_TYPE = IBaseObject>
inline U* asOrNull(OBJ_TYPE* ptr, bool borrow = false)
{
    if (ptr == nullptr)
    {
        return nullptr;
    }

    U* out;
    ErrCode err;
    if (borrow)
    {
        err = ptr->borrowInterface(U::Id, reinterpret_cast<void**>(&out));
    }
    else
    {
        err = ptr->queryInterface(U::Id, reinterpret_cast<void**>(&out));
    }

    if (!OPENDAQ_SUCCEEDED(err))
    {
        return nullptr;
    }
    return out;
}

inline bool simplify(Int& num, Int& den)
{
    const Int gcd = std::gcd(num, den);
    if (gcd <= 1)
        return false;

    num /= gcd;
    den /= gcd;
    return true;
}

template <typename Handler, typename... Params>
ErrCode wrapHandler(Handler handler, Params... params)
{
    using ResultType = std::invoke_result_t<Handler, Params...>;
    static_assert(std::is_same_v<ResultType, void> || std::is_same_v<ResultType, ErrCode>, "Return type must be void or daq::ErrCode");

    try
    {
        if constexpr (std::is_same_v<ResultType, void>)
        {
            (handler)(params...);
            return OPENDAQ_SUCCESS;
        }
        else
        {
            return (handler)(params...);
        }
    }
    catch (const DaqException& e)
    {
        return errorFromException(e);
    }
    catch (const std::exception& e)
    {
        return DAQ_ERROR_FROM_STD_EXCEPTION(e, nullptr, OPENDAQ_ERR_GENERALERROR);
    }
    catch (...)
    {
        return OPENDAQ_ERR_GENERALERROR;
    }
}

template <typename Handler, typename TReturn, typename... Params>
ErrCode wrapHandlerReturn(Handler handler, TReturn& output, Params... params)
{
    try
    {
        output = (handler)(params...);
        return OPENDAQ_SUCCESS;
    }
    catch (const DaqException& e)
    {
        return errorFromException(e);
    }
    catch (const std::exception& e)
    {
        return DAQ_ERROR_FROM_STD_EXCEPTION(e, nullptr, OPENDAQ_ERR_GENERALERROR);
    }
    catch (...)
    {
        return OPENDAQ_ERR_GENERALERROR;
    }
}

template <typename Object, typename Handler, typename... Params>
ErrCode wrapHandler(Object* object, Handler handler, Params... params)
{
    using ResultType = decltype((object->*handler)(params...));
    static_assert(std::is_same_v<ResultType, void> || std::is_same_v<ResultType, ErrCode>, "Return type must be void or daq::ErrCode");

    try
    {
        if constexpr (std::is_same_v<ResultType, void>)
        {
            (object->*handler)(params...);
            return OPENDAQ_SUCCESS;
        }
        else
        {
            return (object->*handler)(params...);
        }
    }
    catch (const DaqException& e)
    {
        IBaseObject* baseObject;
        object->borrowInterface(IBaseObject::Id, reinterpret_cast<void**>(&baseObject));
        return errorFromException(e, baseObject);
    }
    catch (const std::exception& e)
    {
        IBaseObject* baseObject;
        object->borrowInterface(IBaseObject::Id, reinterpret_cast<void**>(&baseObject));
        return DAQ_ERROR_FROM_STD_EXCEPTION(e, baseObject, OPENDAQ_ERR_GENERALERROR);
    }
    catch (...)
    {
        return OPENDAQ_ERR_GENERALERROR;
    }
}

template <typename Object, typename Handler, typename TReturn, typename... Params>
ErrCode wrapHandlerReturn(Object* object, Handler handler, TReturn& output, Params... params)
{
    try
    {
        output = (object->*handler)(params...);
        return OPENDAQ_SUCCESS;
    }
    catch (const DaqException& e)
    {
        IBaseObject* baseObject;
        object->borrowInterface(IBaseObject::Id, reinterpret_cast<void**>(&baseObject));
        return errorFromException(e, baseObject);
    }
    catch (const std::exception& e)
    {
        IBaseObject* baseObject;
        object->borrowInterface(IBaseObject::Id, reinterpret_cast<void**>(&baseObject));
        return DAQ_ERROR_FROM_STD_EXCEPTION(e, baseObject, OPENDAQ_ERR_GENERALERROR);
    }
    catch (...)
    {
        return OPENDAQ_ERR_GENERALERROR;
    }
}

template <typename T>
class ShowType;

template <class F>
ErrCode daqTry(const IBaseObject* context, F&& func)
{
    try
    {
        if constexpr (std::is_same_v<std::invoke_result_t<F>, void>)
        {
            func();
            return OPENDAQ_SUCCESS;
        }
        else
        {
            return func();            
        }

    }
    catch (const DaqException& e)
    {
        IBaseObject* baseObject = nullptr;
        if (context)
            context->borrowInterface(IBaseObject::Id, reinterpret_cast<void**>(&baseObject));
        return errorFromException(e, baseObject);
    }
    catch (const std::exception& e)
    {
        IBaseObject* baseObject = nullptr;
        if (context)
            context->borrowInterface(IBaseObject::Id, reinterpret_cast<void**>(&baseObject));
        return DAQ_ERROR_FROM_STD_EXCEPTION(e, baseObject, OPENDAQ_ERR_GENERALERROR);
    }
    catch (...)
    {
        return OPENDAQ_ERR_GENERALERROR;
    }
}

template <typename Interface>
inline std::string daqInterfaceIdString()
{
    char iid[39];
    daqInterfaceIdToString(Interface::Id, iid);

    return iid;
}

template <class F>
ErrCode daqTry(const F& func)
{
    return daqTry(nullptr, func);
}

template <class F>
ErrCode daqTry(F&& func)
{
    return daqTry(nullptr, std::move(func));
}

template <typename TPtr, typename TFunc>
TPtr callNotNull(TPtr ptr, TFunc func)
{
    if (!ptr.assigned())
    {
        return ptr;
    }

    return std::invoke(func, std::forward<TPtr>(ptr));
}

inline bool validateTypeName(ConstCharPtr typeName)
{
    const std::regex validatorRegex("^[a-zA-Z_]+[a-zA-Z0-9_]*$");
    return std::regex_match(typeName, validatorRegex);
}

template <typename T>
using IsEnumTypeEnum = std::enable_if<std::is_enum_v<T> && std::is_same_v<std::underlying_type_t<T>, EnumType>, int>;

template <typename T, typename IsEnumTypeEnum<T>::type = 0>
T operator|(T lhs, T rhs)
{
    return T(EnumType(lhs) | EnumType(rhs));
}

template <typename T, typename IsEnumTypeEnum<T>::type = 0>
bool operator&(T lhs, T rhs)
{
    return EnumType(lhs) & EnumType(rhs);
}

END_NAMESPACE_OPENDAQ
