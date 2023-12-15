#include <opendaq/tags_impl.h>
#include <opendaq/tags_ptr.h>
#include <coretypes/validation.h>
#include <coreobjects/eval_value_factory.h>

BEGIN_NAMESPACE_OPENDAQ

TagsImpl::TagsImpl()
    : frozen(false)
{
}

TagsImpl::TagsImpl(const TagsPtr& tagsToCopy)
    : frozen(false)
{
    for (const auto& tag : tagsToCopy.getList())
    {
        tags.emplace(tag);
    }
}

ErrCode TagsImpl::getList(IList** value)
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

ErrCode TagsImpl::add(IString* name)
{
    OPENDAQ_PARAM_NOT_NULL(name);
    if (frozen)
        return OPENDAQ_ERR_FROZEN;

    auto str = StringPtr::Borrow(name).toStdString();

    if (tags.count(str) > 0)
        return OPENDAQ_ERR_DUPLICATEITEM;

    tags.emplace(str);
    return OPENDAQ_SUCCESS;
}

ErrCode TagsImpl::remove(IString* name)
{
    OPENDAQ_PARAM_NOT_NULL(name);
    if (frozen)
        return OPENDAQ_ERR_FROZEN;

    const auto str = StringPtr::Borrow(name).toStdString();
    if (tags.count(str) == 0)
        return OPENDAQ_ERR_NOTFOUND;

    tags.erase(str);
    return OPENDAQ_SUCCESS;
}

ErrCode TagsImpl::contains(IString* name, Bool* value)
{
    OPENDAQ_PARAM_NOT_NULL(value);
    OPENDAQ_PARAM_NOT_NULL(name);

    const auto str = StringPtr::Borrow(name).toStdString();
    *value = tags.count(str) > 0;
    return OPENDAQ_SUCCESS;
}

ErrCode TagsImpl::query(IString* query, Bool* value)
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

ErrCode TagsImpl::freeze()
{
    if (frozen)
        return  OPENDAQ_IGNORED;

    frozen = true;
    return OPENDAQ_SUCCESS;
}

ErrCode TagsImpl::isFrozen(Bool* isFrozen) const
{
    OPENDAQ_PARAM_NOT_NULL(isFrozen);

    *isFrozen = frozen;
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC TagsImpl::equals(IBaseObject* other, Bool* equals) const
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

ErrCode TagsImpl::serialize(ISerializer* serializer)
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

ErrCode TagsImpl::getSerializeId(ConstCharPtr* id) const
{
    OPENDAQ_PARAM_NOT_NULL(id);

    *id = SerializeId();

    return OPENDAQ_SUCCESS;
}

ConstCharPtr TagsImpl::SerializeId()
{
    return "Tags";
}

ErrCode TagsImpl::Deserialize(ISerializedObject* serialized, IBaseObject*, IFunction*, IBaseObject** obj)
{
    OPENDAQ_PARAM_NOT_NULL(serialized);
    OPENDAQ_PARAM_NOT_NULL(obj);

    ObjectPtr<ITagsConfig> tags;
    auto errCode = createObject<ITagsConfig, TagsImpl>(&tags);
    if (OPENDAQ_FAILED(errCode))
        return errCode;

    const auto serializedObj = SerializedObjectPtr::Borrow(serialized);

    const auto list = serializedObj.readList<IString>("list");
    for (const auto& tag : list)
        tags->add(tag);

    *obj = tags.detach();

    return OPENDAQ_SUCCESS;
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, Tags, ITagsConfig)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, Tags, ITagsConfig, createTagsFromExisting,
    ITags*, tagsToCopy
)

END_NAMESPACE_OPENDAQ
