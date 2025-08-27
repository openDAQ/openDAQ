#pragma once
#include <coretypes/coretypes.h>
#include "coreobjects/mutex.h"
#include <coretypes/objectptr.h>

BEGIN_NAMESPACE_OPENDAQ

class MutexPtr;

template <>
struct InterfaceToSmartPtr<daq::IMutex>
{
    using SmartPtr = daq::MutexPtr;
};

class MutexPtr : public daq::ObjectPtr<IMutex>
{
public:
    using daq::ObjectPtr<IMutex>::ObjectPtr;


    MutexPtr()
        : daq::ObjectPtr<IMutex>()

    {
    }

    MutexPtr(daq::ObjectPtr<IMutex>&& ptr)
        : daq::ObjectPtr<IMutex>(std::move(ptr))

    {
    }

    MutexPtr(const daq::ObjectPtr<IMutex>& ptr)
        : daq::ObjectPtr<IMutex>(ptr)

    {
    }

    MutexPtr(const MutexPtr& other)
        : daq::ObjectPtr<IMutex>(other)

    {
    }

    MutexPtr(MutexPtr&& other) noexcept
        : daq::ObjectPtr<IMutex>(std::move(other))

    {
    }
    
    MutexPtr& operator=(const MutexPtr& other)
    {
        daq::ObjectPtr<IMutex>::operator =(other);


        return *this;
    }

    MutexPtr& operator=(MutexPtr&& other) noexcept
    {

        daq::ObjectPtr<IMutex>::operator =(std::move(other));

        return *this;
    }

    void lock() const
    {
        if (this->object == nullptr)
            DAQ_THROW_EXCEPTION(daq::InvalidParameterException);

        auto errCode = this->object->lock();
        daq::checkErrorInfo(errCode);
    }

    bool try_lock() const
    {
        if (this->object == nullptr)
            DAQ_THROW_EXCEPTION(daq::InvalidParameterException);

        Bool lock;
        auto errCode = this->object->tryLock(&lock);
        daq::checkErrorInfo(errCode);

        return lock;
    }

    void unlock() const
    {
        if (this->object == nullptr)
            DAQ_THROW_EXCEPTION(daq::InvalidParameterException);

        auto errCode = this->object->lock();
        daq::checkErrorInfo(errCode);
    }
};

END_NAMESPACE_OPENDAQ
