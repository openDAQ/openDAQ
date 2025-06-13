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
    ErrCode INTERFACE_FUNC getFileName(ConstCharPtr* fileName) override;
    ErrCode INTERFACE_FUNC setFileLine(Int line) override;
    ErrCode INTERFACE_FUNC getFileLine(Int* line) override;

    ErrCode INTERFACE_FUNC getFormatMessage(IString** message) override;
    
    ErrCode INTERFACE_FUNC freeze() override;
    ErrCode INTERFACE_FUNC isFrozen(Bool* frozen) const override;

private:
    IString* message;
    IString* source;
    ConstCharPtr fileName;
    Int fileLine;
    Bool frozen;
};


class ErrorInfoWrapper;

class ErrorGuardImpl : public ImplementationOf<IErrorGuard>
{
public:
    ErrorGuardImpl(ConstCharPtr filename, int fileLine);
    ~ErrorGuardImpl();

    ErrCode INTERFACE_FUNC getFileName(ConstCharPtr* fileName) override;
    ErrCode INTERFACE_FUNC getFileLine(Int* fileLine) override;
    ErrCode INTERFACE_FUNC getErrorInfos(IList** errorInfos) override;
    ErrCode INTERFACE_FUNC getFormatMessage(IString** message) override;

    std::list<ErrorInfoWrapper>::iterator getIterator();

private:
    friend class ErrorInfoHolder;

    ConstCharPtr filename;
    Int fileLine;
    ErrorGuardImpl* prevScopeEntry;
    std::list<ErrorInfoWrapper>::iterator it;
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

class ErrorInfoHolder
{
public:
    using ContainerT = std::list<ErrorInfoWrapper>;
    
    ErrorInfoHolder() = default;

    void setErrorInfo(IErrorInfo* errorInfo);
    IErrorInfo* getErrorInfo() const;
    IList* moveErrorInfoList();

private:
    friend class ErrorGuardImpl;

    ContainerT* getList();

    std::unique_ptr<ContainerT> errorInfoList;
    ErrorGuardImpl* scopeEntry = nullptr;
};

END_NAMESPACE_OPENDAQ
