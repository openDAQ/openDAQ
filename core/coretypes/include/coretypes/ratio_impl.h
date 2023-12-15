/*
 * Copyright 2022-2023 Blueberry d.o.o.
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
#include <coretypes/common.h>
#include <coretypes/ratio.h>
#include <coretypes/convertible.h>
#include <coretypes/serializable.h>
#include <coretypes/deserializer.h>
#include <coretypes/coretype.h>
#include <coretypes/comparable.h>
#include <coretypes/struct_impl.h>

BEGIN_NAMESPACE_OPENDAQ

class RatioImpl : public GenericStructImpl<IRatio, IStruct, IConvertible, IComparable>
{
public:
    RatioImpl(Int numerator, Int denominator);

    // IBaseObject
    ErrCode INTERFACE_FUNC toString(CharPtr* str) override;

    // IRatio
    ErrCode INTERFACE_FUNC getNumerator(Int* numerator) override;
    ErrCode INTERFACE_FUNC getDenominator(Int* denominator) override;
    ErrCode INTERFACE_FUNC simplify(IRatio** simplifiedRatio) override;

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

    static ConstCharPtr SerializeId();
    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* /*context*/, IFunction* /*factoryCallback*/, IBaseObject** obj);

    ErrCode INTERFACE_FUNC equals(IBaseObject* other, Bool* equals) const override;

private:
    Int numerator;
    Int denominator;
};

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(RatioImpl)

END_NAMESPACE_OPENDAQ
