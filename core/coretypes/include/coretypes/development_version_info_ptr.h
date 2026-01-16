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
#include <coretypes/coretypes.h>
#include <coretypes/development_version_info.h>

BEGIN_NAMESPACE_OPENDAQ

class DevelopmentVersionInfoPtr;

template<>
struct InterfaceToSmartPtr<IDevelopmentVersionInfo>
{
    typedef DevelopmentVersionInfoPtr SmartPtr;
};

/*!
 * @addtogroup types_version_info
 * @{
 */

class DevelopmentVersionInfoPtr : public daq::ObjectPtr<IDevelopmentVersionInfo>
{
public:
    using daq::ObjectPtr<IDevelopmentVersionInfo>::ObjectPtr;
//    using daq::ObjectPtr<IVersionInfo>::operator=;

    DevelopmentVersionInfoPtr()
        : daq::ObjectPtr<IDevelopmentVersionInfo>()
    {
    }

    DevelopmentVersionInfoPtr(daq::ObjectPtr<IDevelopmentVersionInfo>&& ptr)
        : daq::ObjectPtr<IDevelopmentVersionInfo>(std::move(ptr))
    {
    }

    DevelopmentVersionInfoPtr(const daq::ObjectPtr<IDevelopmentVersionInfo>& ptr)
        : daq::ObjectPtr<IDevelopmentVersionInfo>(ptr)
    {
    }

    DevelopmentVersionInfoPtr(const DevelopmentVersionInfoPtr& other)
        : daq::ObjectPtr<IDevelopmentVersionInfo>(other)
    {
    }

    DevelopmentVersionInfoPtr(DevelopmentVersionInfoPtr&& other) noexcept
        : daq::ObjectPtr<IDevelopmentVersionInfo>(std::move(other))
    {
    }

    DevelopmentVersionInfoPtr& operator=(const DevelopmentVersionInfoPtr& other)
    {
        if (this == &other)
            return *this;

        daq::ObjectPtr<IDevelopmentVersionInfo>::operator =(other);
        return *this;
    }

    DevelopmentVersionInfoPtr& operator=(DevelopmentVersionInfoPtr&& other) noexcept
    {
        if (this == std::addressof(other))
        {
            return *this;
        }

        daq::ObjectPtr<IDevelopmentVersionInfo>::operator =(std::move(other));
        return *this;
    }

    SizeT getMajor() const
    {
        if (this->object == nullptr)
            DAQ_THROW_EXCEPTION(daq::InvalidParameterException);

        SizeT major;
        auto errCode = this->object->getMajor(&major);
        daq::checkErrorInfo(errCode);

        return major;
    }

    SizeT getMinor() const
    {
        if (this->object == nullptr)
            DAQ_THROW_EXCEPTION(daq::InvalidParameterException);

        SizeT minor;
        auto errCode = this->object->getMinor(&minor);
        daq::checkErrorInfo(errCode);

        return minor;
    }

    SizeT getPatch() const
    {
        if (this->object == nullptr)
            DAQ_THROW_EXCEPTION(daq::InvalidParameterException);

        SizeT patch;
        auto errCode = this->object->getPatch(&patch);
        daq::checkErrorInfo(errCode);

        return patch;
    }

    SizeT getTweak() const
    {
        if (this->object == nullptr)
            DAQ_THROW_EXCEPTION(daq::InvalidParameterException);

        SizeT tweak;
        auto errCode = this->object->getTweak(&tweak);
        daq::checkErrorInfo(errCode);

        return tweak;
    }

    StringPtr getBranchName() const
    {
        if (this->object == nullptr)
            DAQ_THROW_EXCEPTION(daq::InvalidParameterException);

        StringPtr branchName;
        auto errCode = this->object->getBranchName(&branchName);
        daq::checkErrorInfo(errCode);

        return branchName;
    }

    StringPtr getHashDigest() const
    {
        if (this->object == nullptr)
            DAQ_THROW_EXCEPTION(daq::InvalidParameterException);

        StringPtr hashDigest;
        auto errCode = this->object->getHashDigest(&hashDigest);
        daq::checkErrorInfo(errCode);

        return hashDigest;
    }
};

/*!
 * @}
 */

END_NAMESPACE_OPENDAQ
