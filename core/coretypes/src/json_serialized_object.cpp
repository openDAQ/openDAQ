#include <coretypes/json_serialized_object.h>
#include <coretypes/coretypes.h>
#include <coretypes/json_deserializer_impl.h>
#include <coretypes/json_serialized_list.h>
#include <coretypes/serialization.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

BEGIN_NAMESPACE_OPENDAQ
JsonSerializedObject::JsonSerializedObject(const JsonObject& obj)
    : object(obj)
    , root(false)
{
}

JsonSerializedObject::JsonSerializedObject(const JsonObject& obj, bool isRoot)
    : object(obj)
    , root(isRoot)
{
}

ErrCode JsonSerializedObject::readSerializedObject(IString* key, ISerializedObject** plainObj)
{
    ConstCharPtr member;
    key->getCharPtr(&member);

    if (!object.HasMember(member))
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOTFOUND);

    auto& value = object[member];
    if (!value.IsObject())
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDTYPE);

    *plainObj = new(std::nothrow) JsonSerializedObject(value.GetObject());
    (*plainObj)->addRef();
    return OPENDAQ_SUCCESS;
}

ErrCode JsonSerializedObject::readSerializedList(IString* key, ISerializedList** list)
{
    using namespace rapidjson;

    ConstCharPtr member;
    key->getCharPtr(&member);

    if (!object.HasMember(member))
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOTFOUND);

    auto& value = object[member];
    if (!value.IsArray())
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDTYPE);

    SerializedListPtr serList;
    ErrCode err = createObject<ISerializedList, JsonSerializedList>(&serList, value.GetArray());
    if (OPENDAQ_FAILED(err))
    {
        return err;
    }

    *list = serList.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode JsonSerializedObject::readList(IString* key, IBaseObject* context, IFunction* factoryCallback, IList** list)
{
    ConstCharPtr member;
    key->getCharPtr(&member);

    if (!object.HasMember(member))
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOTFOUND);

    auto& value = object[member];
    if (!value.IsArray())
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDTYPE);

    return JsonDeserializerImpl::Deserialize(value, context, factoryCallback, reinterpret_cast<IBaseObject**>(list));
}

ErrCode JsonSerializedObject::readObject(IString* key, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj)
{
    ConstCharPtr member;
    key->getCharPtr(&member);

    if (!object.HasMember(member))
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOTFOUND);

    return JsonDeserializerImpl::Deserialize(object[member], context, factoryCallback, obj);
}

ErrCode JsonSerializedObject::readString(IString* key, IString** string)
{
    ConstCharPtr member;
    key->getCharPtr(&member);

    if (!object.HasMember(member))
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOTFOUND);
    
    auto& value = object[member];
    if (!value.IsString())
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDTYPE);

    ErrCode errCode = createString(string, value.GetString());
    if (OPENDAQ_FAILED(errCode))
    {
        return errCode;
    }
    return OPENDAQ_SUCCESS;
}

ErrCode JsonSerializedObject::readBool(IString* key, Bool* boolean)
{
    ConstCharPtr member;
    key->getCharPtr(&member);

    if (!object.HasMember(member))
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOTFOUND);

    auto& value = object[member];
    if (!value.IsBool())
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDTYPE);

    *boolean = value.GetBool();

    return OPENDAQ_SUCCESS;
}

ErrCode JsonSerializedObject::readInt(IString* key, Int* integer)
{
    ConstCharPtr member;
    key->getCharPtr(&member);

    if (!object.HasMember(member))
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOTFOUND);

    auto& value = object[member];
    if (value.IsInt())
    {
        *integer = value.GetInt();
        return OPENDAQ_SUCCESS;
    }

    if (value.IsInt64())
    {
        *integer = value.GetInt64();
        return OPENDAQ_SUCCESS;
    }

    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDTYPE);
}

ErrCode JsonSerializedObject::readFloat(IString* key, Float* real)
{
    ConstCharPtr member;
    key->getCharPtr(&member);

    if (!object.HasMember(member))
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOTFOUND);

    auto& value = object[member];
    if (!value.IsDouble())
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDTYPE);

    *real = value.GetDouble();

    return OPENDAQ_SUCCESS;
}

ErrCode JsonSerializedObject::getKeys(IList** list)
{
    ErrCode errCode = createList(list);
    if (OPENDAQ_FAILED(errCode))
    {
        return errCode;
    }

    for (const auto& prop : object)
    {
        errCode = (*list)->pushBack(String(prop.name.GetString()));

        if (OPENDAQ_FAILED(errCode))
        {
            return errCode;
        }
    }

    return OPENDAQ_SUCCESS;
}

ErrCode JsonSerializedObject::getType(IString* key, CoreType* type)
{
    OPENDAQ_PARAM_NOT_NULL(type);

    ConstCharPtr str;
    key->getCharPtr(&str);

    auto iter = object.FindMember(str);
    if (iter == object.MemberEnd())
    {
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOTFOUND);
    }

    *type = JsonDeserializerImpl::GetCoreType(iter->value);
    return OPENDAQ_SUCCESS;
}

ErrCode JsonSerializedObject::isRoot(Bool* isRoot)
{
    OPENDAQ_PARAM_NOT_NULL(isRoot);

    *isRoot = root;
    return OPENDAQ_SUCCESS;
}

ErrCode JsonSerializedObject::toJson(IString** jsonString)
{
    OPENDAQ_PARAM_NOT_NULL(jsonString);

    *jsonString = objToJson(object).detach();
    return OPENDAQ_SUCCESS;
}

ErrCode JsonSerializedObject::toString(CharPtr* str)
{
    OPENDAQ_PARAM_NOT_NULL(str);

    return daqDuplicateCharPtr("JsonSerializedObject", str);
}

StringPtr JsonSerializedObject::objToJson(const rapidjson::Value& val)
{
    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
    val.Accept(writer);
    std::string s = sb.GetString();
    return s;
}

ErrCode JsonSerializedObject::hasKey(IString* key, Bool* hasKey)
{
    ConstCharPtr ptr;
    key->getCharPtr(&ptr);

    *hasKey = object.HasMember(ptr);

    return OPENDAQ_SUCCESS;
}

END_NAMESPACE_OPENDAQ
