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
#include <opendaq/tags_config.h>
#include <opendaq/tags_ptr.h>
#include <coretypes/intfs.h>
#include <coretypes/freezable.h>

BEGIN_NAMESPACE_OPENDAQ

class TagsImpl : public ImplementationOf<ITagsConfig, IFreezable, ISerializable>
{
public:
    explicit TagsImpl();
    explicit TagsImpl(const TagsPtr& tags);

    // ITags

    ErrCode INTERFACE_FUNC getList(IList** value) override;
    ErrCode INTERFACE_FUNC add(IString* name) override;
    ErrCode INTERFACE_FUNC remove(IString* name) override;
    ErrCode INTERFACE_FUNC contains(IString* name, Bool* value) override;
    ErrCode INTERFACE_FUNC query(IString* query, Bool* value) override;

    // IFreezable

    ErrCode INTERFACE_FUNC freeze() override;
    ErrCode INTERFACE_FUNC isFrozen(Bool* isFrozen) const override;

    // IBaseObject

    ErrCode INTERFACE_FUNC equals(IBaseObject* other, Bool* equals) const override;

    // ISerializable
    ErrCode INTERFACE_FUNC serialize(ISerializer* serializer) override;
    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override;

    static ConstCharPtr SerializeId();
    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* /*context*/, IFunction* /*factoryCallback*/, IBaseObject** obj);

private:
    std::unordered_set<std::string> tags;
    bool frozen;
};

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(TagsImpl)

END_NAMESPACE_OPENDAQ
