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
#include <coretypes/objectptr.h>
#include <coretypes/string_ptr.h>
#include <coretypes/error_guard.h>
#include <coretypes/list_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @addtogroup types_event_args
 * @{
 */


class ErrorGuardPtr;

template <>
struct InterfaceToSmartPtr<IErrorGuard>
{
    using SmartPtr = ErrorGuardPtr;
};


class ErrorGuardPtr : public ObjectPtr<IErrorGuard>
{
public:
    using ObjectPtr<IErrorGuard>::ObjectPtr;

    ErrorGuardPtr(ObjectPtr<IErrorGuard>&& ptr)
        : ObjectPtr<IErrorGuard>(std::move(ptr))

    {
    }

    ErrorGuardPtr(const ObjectPtr<IErrorGuard>& ptr)
        : ObjectPtr<IErrorGuard>(ptr)
    {
    }

    ErrorGuardPtr(const ErrorGuardPtr& other)
        : ObjectPtr<IErrorGuard>(other)
    {
    }

    ErrorGuardPtr(ErrorGuardPtr&& other) noexcept
        : ObjectPtr<IErrorGuard>(std::move(other))
    {
    }

    ErrorGuardPtr& operator=(const ErrorGuardPtr& other)
    {
        ObjectPtr<IErrorGuard>::operator =(other);
        return *this;
    }

    ErrorGuardPtr& operator=(ErrorGuardPtr&& other) noexcept
    {
        ObjectPtr<IErrorGuard>::operator =(std::move(other));
        return *this;
    }

    ListPtr<IErrorInfo> getErrorInfos() const
    {
        if (this->object == nullptr)
            DAQ_THROW_EXCEPTION(InvalidParameterException);

        ListPtr<IErrorInfo> errorInfos;
        const ErrCode errCode = this->object->getErrorInfos(&errorInfos);
        checkErrorInfo(errCode);

        return errorInfos;
    }

    StringPtr getFormatMessage() const
    {
        if (this->object == nullptr)
            DAQ_THROW_EXCEPTION(InvalidParameterException);

        StringPtr message;
        const ErrCode errCode = this->object->getFormatMessage(&message);
        checkErrorInfo(errCode);

        return message;
    }
};

/*!
 * @}
 */

END_NAMESPACE_OPENDAQ