/*
 * Copyright 2022-2024 openDAQ d.o.o.
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
#include <opendaq/tags_ptr.h>
#include <opendaq/tags_private_ptr.h>
#include <opendaq/component_ptr.h>
#include <coretypes/intfs.h>
#include <coretypes/validation.h>
#include <coreobjects/eval_value_factory.h>
#include <coreobjects/core_event_args_impl.h>

BEGIN_NAMESPACE_OPENDAQ

class TagsImpl : public ImplementationOf<ITags, ITagsPrivate, ISerializable>
{
public:
    explicit TagsImpl();
    explicit TagsImpl(const ProcedurePtr& coreEventCallback);

    // ITags

    ErrCode INTERFACE_FUNC getList(IList** value) override;
    ErrCode INTERFACE_FUNC add(IString* name) override;
    ErrCode INTERFACE_FUNC remove(IString* name) override;
    ErrCode INTERFACE_FUNC replace(IList* tags) override;
    ErrCode INTERFACE_FUNC contains(IString* name, Bool* value) override;
    ErrCode INTERFACE_FUNC query(IString* query, Bool* value) override;
    
    // IBaseObject

    ErrCode INTERFACE_FUNC equals(IBaseObject* other, Bool* equals) const override;

    // ISerializable

    ErrCode INTERFACE_FUNC serialize(ISerializer* serializer) override;
    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override;

    static ConstCharPtr SerializeId();
    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* /*context*/, IFunction* /*factoryCallback*/, IBaseObject** obj);

protected:
    std::unordered_set<std::string> tags;

private:
    ProcedurePtr triggerCoreEvent;
};


inline TagsImpl::TagsImpl()
{
}

inline TagsImpl::TagsImpl(const ProcedurePtr& coreEventCallback)
    : triggerCoreEvent(coreEventCallback)
{
}

inline ErrCode TagsImpl::getList(IList** value)
{
    OPENDAQ_PARAM_NOT_NULL(value);

    auto list = List<IString>();

    for (auto& item: tags)
    {
        list.pushBack(item);
    }

    *value = list.detach();
    return OPENDAQ_SUCCESS;
}

inline ErrCode TagsImpl::add(IString* name)
{
    OPENDAQ_PARAM_NOT_NULL(name);

    auto str = StringPtr::Borrow(name).toStdString();

    if (tags.count(str) > 0)
        return OPENDAQ_IGNORED;

    tags.emplace(str);

    if (triggerCoreEvent.assigned())
    {
        const auto thisPtr = this->borrowPtr<TagsPtr>();
        const CoreEventArgsPtr args = createWithImplementation<ICoreEventArgs, CoreEventArgsImpl>(
            CoreEventId::TagsChanged, Dict<IString, IBaseObject>({{"Tags", thisPtr}}));
        triggerCoreEvent(args);
    }

    return OPENDAQ_SUCCESS;
}

inline ErrCode TagsImpl::remove(IString* name)
{
    OPENDAQ_PARAM_NOT_NULL(name);

    const auto str = StringPtr::Borrow(name).toStdString();
    if (tags.count(str) == 0)
        return OPENDAQ_IGNORED;

    tags.erase(str);

    if (triggerCoreEvent.assigned())
    {
        const auto thisPtr = this->borrowPtr<TagsPtr>();
        const CoreEventArgsPtr args = createWithImplementation<ICoreEventArgs, CoreEventArgsImpl>(
            CoreEventId::TagsChanged, Dict<IString, IBaseObject>({{"Tags", thisPtr}}));
        triggerCoreEvent(args);
    }

    return OPENDAQ_SUCCESS;
}

inline ErrCode TagsImpl::replace(IList* tags)
{
    OPENDAQ_PARAM_NOT_NULL(tags);
    this->tags.clear();

    const auto tagsPtr = ListPtr<IString>::Borrow(tags);
    for (const auto& tag : tagsPtr)
        this->tags.insert(tag);
    
    if (triggerCoreEvent.assigned())
    {
        const auto thisPtr = this->borrowPtr<TagsPtr>();
        const CoreEventArgsPtr args = createWithImplementation<ICoreEventArgs, CoreEventArgsImpl>(
            CoreEventId::TagsChanged, Dict<IString, IBaseObject>({{"Tags", thisPtr}}));
        triggerCoreEvent(args);
    }

    return OPENDAQ_SUCCESS;
}

inline ErrCode TagsImpl::contains(IString* name, Bool* value)
{
    OPENDAQ_PARAM_NOT_NULL(value);
    OPENDAQ_PARAM_NOT_NULL(name);

    const auto str = StringPtr::Borrow(name).toStdString();
    *value = tags.count(str) > 0;
    return OPENDAQ_SUCCESS;
}

inline ErrCode TagsImpl::query(IString* query, Bool* value)
{
    OPENDAQ_PARAM_NOT_NULL(value);
    OPENDAQ_PARAM_NOT_NULL(query);

    const auto tagEvalValue = EvalValueFunc(query,
                                      [this](const std::string& tagStr)
                                      {
                                          return this->tags.count(tagStr) > 0 ? True : False;
                                      });

    *value = tagEvalValue;
    return OPENDAQ_SUCCESS;
}

inline ErrCode INTERFACE_FUNC TagsImpl::equals(IBaseObject* other, Bool* equals) const
{
    OPENDAQ_PARAM_NOT_NULL(equals);
    *equals = false;

    if (other == nullptr)
        return OPENDAQ_SUCCESS;

    auto tagsOther = BaseObjectPtr::Borrow(other).asPtrOrNull<ITags>();
    if (tagsOther == nullptr)
        return OPENDAQ_SUCCESS;

    const auto listOther = tagsOther.getList();
    if (listOther.getCount() != tags.size())
        return OPENDAQ_SUCCESS;

    for (const auto& tag : listOther)
    {
        if (tags.count(tag) == 0)
            return OPENDAQ_SUCCESS;
    }

    *equals = true;
    return OPENDAQ_SUCCESS;
}

inline ErrCode TagsImpl::serialize(ISerializer* serializer)
{
    OPENDAQ_PARAM_NOT_NULL(serializer);

    serializer->startTaggedObject(this);
    {
        serializer->key("list");
        serializer->startList();
        for (const auto& tag : tags)
            serializer->writeString(tag.c_str(), tag.size());
        serializer->endList();
    }
    serializer->endObject();

    return OPENDAQ_SUCCESS;
}

inline ErrCode TagsImpl::getSerializeId(ConstCharPtr* id) const
{
    OPENDAQ_PARAM_NOT_NULL(id);

    *id = SerializeId();

    return OPENDAQ_SUCCESS;
}

inline ConstCharPtr TagsImpl::SerializeId()
{
    return "Tags";
}

inline ErrCode TagsImpl::Deserialize(ISerializedObject* serialized, IBaseObject*, IFunction*, IBaseObject** obj)
{
    OPENDAQ_PARAM_NOT_NULL(serialized);
    OPENDAQ_PARAM_NOT_NULL(obj);

    ObjectPtr<ITagsPrivate> tags;
    auto errCode = createObject<ITagsPrivate, TagsImpl>(&tags);
    if (OPENDAQ_FAILED(errCode))
        return errCode;

    const auto serializedObj = SerializedObjectPtr::Borrow(serialized);

    const auto list = serializedObj.readList<IString>("list");
    for (const auto& tag : list)
        tags->add(tag);

    *obj = tags.detach();

    return OPENDAQ_SUCCESS;
}

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(TagsImpl)

END_NAMESPACE_OPENDAQ
