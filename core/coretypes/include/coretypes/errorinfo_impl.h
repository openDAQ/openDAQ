/*
 * Copyright 2022-2024 openDAQ d. o. o.
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
#include <coretypes/errorinfo.h>
#include <coretypes/freezable.h>
#include <coretypes/intfs.h>

BEGIN_NAMESPACE_OPENDAQ

class ErrorInfoImpl : public ImplementationOf<IErrorInfo, IFreezable>
{
public:
    ErrorInfoImpl();
    ~ErrorInfoImpl() override;

    ErrCode INTERFACE_FUNC setMessage(IString* message) override;
    ErrCode INTERFACE_FUNC getMessage(IString** message) override;
    ErrCode INTERFACE_FUNC getSource(IString** source) override;
    ErrCode INTERFACE_FUNC setSource(IString* source) override;

    ErrCode INTERFACE_FUNC freeze() override;
    ErrCode INTERFACE_FUNC isFrozen(Bool* frozen) const override;

private:
    IString* message;
    IString* source;
    Bool frozen;
};

class ErrorInfoHolder
{
public:
    ErrorInfoHolder();
#ifndef __MINGW32__
    ~ErrorInfoHolder();
#endif

    void setErrorInfo(IErrorInfo* errorInfo);
    IErrorInfo* getErrorInfo() const;
private:
    IErrorInfo* errorInfo;
};

END_NAMESPACE_OPENDAQ
