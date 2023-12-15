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
#include <opendaq/range.h>
#include <coretypes/number_ptr.h>
#include <coretypes/serializable.h>
#include <coretypes/serialized_object.h>
#include <coretypes/deserializer.h>
#include <coretypes/struct_impl.h>

BEGIN_NAMESPACE_OPENDAQ

class RangeImpl : public GenericStructImpl<IRange, IStruct>
{
public:
    explicit RangeImpl(NumberPtr lowValue, NumberPtr highValue);

    ErrCode INTERFACE_FUNC getLowValue(INumber** value) override;
    ErrCode INTERFACE_FUNC getHighValue(INumber** value) override;

    ErrCode INTERFACE_FUNC equals(IBaseObject* other, Bool* equals) const override;

    // ISerializable
    ErrCode INTERFACE_FUNC serialize(ISerializer* serializer) override;
    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override;

    static ConstCharPtr SerializeId();
    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* /*context*/, IFunction* /*factoryCallback*/, IBaseObject** obj);

private:
    NumberPtr low;
    NumberPtr high;
};

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(RangeImpl)

END_NAMESPACE_OPENDAQ
