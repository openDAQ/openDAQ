/*
 * Copyright 2022-2024 Blueberry d.o.o.
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
#include <mutex>
#include <atomic>
#include <memory>
#include <unordered_map>
#include <coretypes/common.h>
#include <coretypes/bb_exception.h>

BEGIN_NAMESPACE_OPENDAQ

struct IExceptionFactory
{
    virtual ~IExceptionFactory() = default;

    virtual void destroy() const = 0;

    [[noreturn]] virtual void throwException(ErrCode errCode, const std::string& msg) const = 0;
};

template <typename TException>
struct GenericExceptionFactory : IExceptionFactory
{
    using ExceptionType = TException;

    [[noreturn]] void throwException(ErrCode /*errCode*/, const std::string& msg) const override
    {
        if (msg.empty())
        {
            throw TException();
        }
        throw TException(msg);
    }

    void destroy() const override
    {
        delete this;
    }
};

template <>
struct GenericExceptionFactory<DaqException> : IExceptionFactory
{
    [[noreturn]] void throwException(ErrCode errCode, const std::string& msg) const override
    {
        throw DaqException(errCode, msg);
    }

    void destroy() const override
    {
        delete this;
    }
};

struct ExceptionFactoryDeleter
{
    void operator()(IExceptionFactory* factory) const
    {
        factory->destroy();
    }
};

class ErrorCodeToException;

template <typename T>
class PointerView
{
public:
    explicit PointerView(T* ptr) : ptr(ptr)
    {
    }

    T* operator->() const
    {
        return ptr;
    }

    explicit operator bool() const
    {
        return ptr;
    }

private:
    T* ptr;
};

class ErrorCodeToException
{
private:
    // So that we can have a public constructor
    // and still only be able to construct inside the class
    struct Key
    {
        explicit Key() = default;
    };

public:
    explicit ErrorCodeToException(Key) : unloaded(false)
    {
    }

    ~ErrorCodeToException()
    {
        unloaded = true;

#if defined(__clang__) && !defined(_WIN32)
        for (auto& [errCode, factory] : map)
        {
            factory.release();
        }
#endif

    }

    ErrorCodeToException(const ErrorCodeToException&) = delete;
    ErrorCodeToException(ErrorCodeToException&&) = delete;
    ErrorCodeToException& operator=(const ErrorCodeToException&) = delete;
    ErrorCodeToException& operator=(ErrorCodeToException&&) = delete;

    static PointerView<ErrorCodeToException> GetInstance()
    {
        static std::unique_ptr<ErrorCodeToException> singleton = std::make_unique<ErrorCodeToException>(Key{});
        return PointerView(singleton.get());
    }

    bool registerRtException(ErrCode errCode, IExceptionFactory* factory)
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (map.find(errCode) != map.cend())
        {
            factory->destroy();
            return false;
        }

        map[errCode] = ExceptionFactoryPtr(factory);
        return true;
    }

    template <typename TException>
    bool registerException(ErrCode errCode)
    {
        return registerRtException(errCode, new GenericExceptionFactory<TException>());
    }

    void unregisterException(ErrCode errCode)
    {
        if (unloaded)
        {
            return;
        }

        std::lock_guard<std::mutex> lock(mutex);
        map.erase(errCode);
    }

    IExceptionFactory* getExceptionFactory(ErrCode errCode) const
    {
        static GenericExceptionFactory<DaqException> defaultFactory;

        std::lock_guard<std::mutex> lock(mutex);
        const ErrorCodeToExceptionMap::const_iterator iterator = map.find(errCode);
        if (iterator == map.cend())
        {
            return &defaultFactory;
        }
        return iterator->second.get();
    }

private:
    using ExceptionFactoryPtr = std::unique_ptr<IExceptionFactory, ExceptionFactoryDeleter>;
    using ErrorCodeToExceptionMap = std::unordered_map<ErrCode, ExceptionFactoryPtr>;

    std::atomic<bool> unloaded;
    mutable std::mutex mutex;
    ErrorCodeToExceptionMap map;
};

END_NAMESPACE_OPENDAQ
