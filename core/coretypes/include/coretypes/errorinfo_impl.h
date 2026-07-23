/*
 * Copyright 2022-2026 openDAQ d.o.o.
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

// Per-thread holder of the error-guard scope stack. Deliberately trivially destructible: it lives in
// thread-local storage, and on mingw-w64 the emutls block backing a thread_local may be freed/reused
// before a registered thread-local destructor runs at thread exit, so any non-trivial destructor
// would dereference freed memory and crash. With no destructor and raw-pointer members, no thread-exit
// callback is registered; the small scope list is leaked at thread exit (harmless - the thread is
// ending). Tracked error-guard objects are still released eagerly during normal use.
class ErrorInfoHolder
{
public:
    ErrorInfoHolder() = default;

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

    // Returns the current top-of-stack scope, or nullptr if none is active.
    ErrorGuardImpl* currentScope() const;
    // Ensures a scope exists to attach errors to (creating the owned sentinel guard if needed).
    ErrorGuardImpl* getOrCreateBack();
    // Releases and forgets the sentinel guard (if present) once its errors have been consumed.
    void removeInitialGuard();

    // Scope stack of non-owning guard pointers. Created lazily and freed as soon as it becomes empty
    // (see removeScopeEntry), so a thread that exits with a balanced scope stack leaks nothing; the
    // holder itself has no destructor (see class comment). Recreated on next use.
    ContainerT* errorScopeList = nullptr;
    // The sentinel guard collects errors raised outside any explicit guard - the only entry owned by
    // the holder (regular guards are owned by their RAII wrappers). It is released eagerly by
    // removeInitialGuard when its errors are consumed, so it does not linger as a tracked object.
    ErrorGuardImpl* initialGuard = nullptr;
};

END_NAMESPACE_OPENDAQ
