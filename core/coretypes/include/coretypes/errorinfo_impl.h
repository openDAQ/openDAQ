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
#include <coretypes/errorinfo.h>
#include <coretypes/error_guard.h>
#include <coretypes/freezable.h>
#include <coretypes/listobject.h>
#include <coretypes/intfs.h>
#include <list>

BEGIN_NAMESPACE_OPENDAQ

class ErrorInfoImpl : public ImplementationOf<IErrorInfo, IFreezable>
{
public:
    ErrorInfoImpl();
    ~ErrorInfoImpl() override;

    ErrCode INTERFACE_FUNC setMessage(IString* message) override;
    ErrCode INTERFACE_FUNC getMessage(IString** message) override;
    ErrCode INTERFACE_FUNC setSource(IString* source) override;
    ErrCode INTERFACE_FUNC getSource(IString** source) override;
    ErrCode INTERFACE_FUNC setFileName(ConstCharPtr fileName) override;
    ErrCode INTERFACE_FUNC getFileName(CharPtr* fileName) override;
    ErrCode INTERFACE_FUNC setFileLine(Int line) override;
    ErrCode INTERFACE_FUNC getFileLine(Int* line) override;
    ErrCode INTERFACE_FUNC setErrorCode(ErrCode errorCode) override;
    ErrCode INTERFACE_FUNC getErrorCode(ErrCode* errorCode) override;

    ErrCode INTERFACE_FUNC setCausedByPrevious(ErrCode prevErrCode) override;
    ErrCode INTERFACE_FUNC getCausedByPrevious(Bool* caused) override;

    ErrCode INTERFACE_FUNC getFormatMessage(IString** message) override;
    
    // IFreezable interface
    ErrCode INTERFACE_FUNC freeze() override;
    ErrCode INTERFACE_FUNC isFrozen(Bool* frozen) const override;

private:
    IString* message;
    IString* source;
    CharPtr fileName;
    Int fileLine;
    ErrCode errorCode;
    ErrCode prevErrCode;

    Bool frozen;
};

class ErrorInfoWrapper
{
public:
    ErrorInfoWrapper(IErrorInfo*);
    ~ErrorInfoWrapper();

    IErrorInfo* get() const;
    IErrorInfo* borrow() const;
private:
    IErrorInfo* errorInfo;
};

class ErrorGuardImpl : public ImplementationOf<IErrorGuard>
{
public:
    ErrorGuardImpl(ConstCharPtr filename, int fileLine);
    ~ErrorGuardImpl();

    ErrCode INTERFACE_FUNC getErrorInfos(IList** errorInfos) override;
    ErrCode INTERFACE_FUNC getFormatMessage(IString** message) override;

    void setErrorInfo(IErrorInfo* errorInfo);
    void extendErrorInfo(IErrorInfo* errorInfo, ErrCode prevErrCode);
    IErrorInfo* getErrorInfo() const;
    void clearLastErrorInfo(ErrCode errCode);
    bool empty() const;

private:
    ConstCharPtr filename;
    int fileLine;

    std::list<ErrorInfoWrapper> errorInfoList;
};

class ErrorInfoHolder
{
public:
    
    ErrorInfoHolder() = default;
    ~ErrorInfoHolder();

    void setErrorInfo(IErrorInfo* errorInfo);
    void extendErrorInfo(IErrorInfo* errorInfo, ErrCode prevErrCode);
    void clearErrorInfo(ErrCode errorCode);
    IErrorInfo* getErrorInfo() const;

    IList* getErrorInfoList();
    IString* getFormatMessage() const;

    void setScopeEntry(ErrorGuardImpl* entry);
    void removeScopeEntry(ErrorGuardImpl* entry);

private:
    using ContainerT = std::list<ErrorGuardImpl*>;
    ContainerT* getOrCreateList();

    std::unique_ptr<ContainerT> errorScopeList;
};

END_NAMESPACE_OPENDAQ
