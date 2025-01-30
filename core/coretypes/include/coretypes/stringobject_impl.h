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
#include <coretypes/stringobject.h>
#include <coretypes/intfs.h>
#include <coretypes/convertible.h>
#include <coretypes/coretype.h>
#include <coretypes/comparable.h>
#include <coretypes/serializable.h>

BEGIN_NAMESPACE_OPENDAQ

class StringImpl : public ImplementationOf<IString, IConvertible, ICoreType, IComparable, ISerializable>
{
public:
    StringImpl(ConstCharPtr str);
    StringImpl(ConstCharPtr data, SizeT length);

    ~StringImpl() override;

    // IBaseObject
    ErrCode INTERFACE_FUNC getHashCode(SizeT* hashCode) override;
    ErrCode INTERFACE_FUNC equals(IBaseObject* other, Bool* equals) const override;
    ErrCode INTERFACE_FUNC toString(CharPtr* str) override;

    // IString
    ErrCode INTERFACE_FUNC getCharPtr(ConstCharPtr* value) override;
    ErrCode INTERFACE_FUNC getLength(SizeT* size) override;

    // IConvertible
    ErrCode INTERFACE_FUNC toFloat(Float* val) override;
    ErrCode INTERFACE_FUNC toInt(Int* val) override;
    ErrCode INTERFACE_FUNC toBool(Bool* val) override;

    // ICoreType
    ErrCode INTERFACE_FUNC getCoreType(CoreType* coreType) override;

    // IComparable
    ErrCode INTERFACE_FUNC compareTo(IBaseObject* obj) override;

    // ISerializable
    ErrCode INTERFACE_FUNC serialize(ISerializer* serializer) override;
    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override;

private:
    char* str;
    SizeT hashCode;
    bool hashCalculated;
    SizeT length;
};

END_NAMESPACE_OPENDAQ
