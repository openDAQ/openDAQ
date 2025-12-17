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
#include <coretypes/serializable.h>
#include <coretypes/deserializer.h>

BEGIN_NAMESPACE_OPENDAQ

class ErrorInfoImpl : public ImplementationOf<IErrorInfo, IFreezable, ISerializable>
{
public:
    ErrorInfoImpl();
    ~ErrorInfoImpl() override;

    // IErrorInfo
    ErrCode INTERFACE_FUNC setMessage(IString* message) override;
    ErrCode INTERFACE_FUNC getMessage(IString** message) override;
    ErrCode INTERFACE_FUNC setSource(IString* source) override;
    ErrCode INTERFACE_FUNC getSource(IString** source) override;
    ErrCode INTERFACE_FUNC setFileName(ConstCharPtr fileName) override;
    ErrCode INTERFACE_FUNC getFileName(ConstCharPtr* fileName) override;
    ErrCode INTERFACE_FUNC setFileLine(Int line) override;
    ErrCode INTERFACE_FUNC getFileLine(Int* line) override;
    ErrCode INTERFACE_FUNC setErrorCode(ErrCode errorCode) override;
    ErrCode INTERFACE_FUNC getErrorCode(ErrCode* errorCode) override;
    ErrCode INTERFACE_FUNC setPreviousErrorCode(ErrCode prevErrCode) override;
    ErrCode INTERFACE_FUNC getPreviousErrorCode(ErrCode* prevErrCode) override;

    ErrCode INTERFACE_FUNC getFormattedMessage(IString** message) override;
    
    // IFreezable interface
    ErrCode INTERFACE_FUNC freeze() override;
    ErrCode INTERFACE_FUNC isFrozen(Bool* frozen) const override;

    // ISerializable
    ErrCode INTERFACE_FUNC serialize(ISerializer* serializer) override;
    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override;

    static ConstCharPtr SerializeId();
    static ErrCode Deserialize(ISerializedObject* ser, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);

private:
    IString* message;
    IString* source;
    CharPtr fileName;
    Int fileLine;
    ErrCode errorCode;
    ErrCode prevErrCode;

    Bool frozen;
    IString* cachedMessage;
    IString* fileNameObject;

    void setFileNameObject(IString* fileNameObj);
};

class ErrorInfoWrapper
{
public:
    ErrorInfoWrapper(IErrorInfo*);
    ~ErrorInfoWrapper();

    IErrorInfo* getAddRef() const;
    IErrorInfo* borrow() const;
private:
    IErrorInfo* errorInfo;
};

class ErrorGuardImpl : public ImplementationOf<IErrorGuard>
{
public:
    ErrorGuardImpl(ConstCharPtr filename, Int fileLine);
    ~ErrorGuardImpl();

    ErrCode INTERFACE_FUNC getLastErrorInfo(IErrorInfo** errorInfo) const override;
    ErrCode INTERFACE_FUNC getErrorInfoList(IList** errorInfos) override;
    ErrCode INTERFACE_FUNC getFormattedMessage(IString** message) const override;
    ErrCode INTERFACE_FUNC toString(CharPtr* str) override;

    virtual bool isInitial() const { return false; }
    void setErrorInfo(IErrorInfo* errorInfo);
    void extendErrorInfo(IErrorInfo* errorInfo);
    virtual ErrCode clearLastErrorInfo();
    bool empty() const;

protected:
    ConstCharPtr filename;
    Int fileLine;

    std::list<ErrorInfoWrapper> errorInfoList;
};

class ErrorInfoHolder
{
public:
    ErrorInfoHolder() = default;
    ~ErrorInfoHolder();

    void setErrorInfo(IErrorInfo* errorInfo);
    void extendErrorInfo(IErrorInfo* errorInfo);
    ErrCode clearErrorInfo();
    ErrCode getErrorInfo(IErrorInfo** errorInfo) const;

    IList* getErrorInfoList();
    ErrCode getFormattedMessage(IString** message) const;

    void setScopeEntry(ErrorGuardImpl* entry);
    void removeScopeEntry(ErrorGuardImpl* entry);

private:
    using ContainerT = std::list<ErrorGuardImpl*>;
    ContainerT* getOrCreateList();

    std::unique_ptr<ContainerT> errorScopeList;
};

END_NAMESPACE_OPENDAQ
