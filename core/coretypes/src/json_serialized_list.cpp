#include <coretypes/json_serialized_list.h>
#include <coretypes/json_deserializer_impl.h>
#include <coretypes/json_serialized_object.h>

BEGIN_NAMESPACE_OPENDAQ

JsonSerializedList::JsonSerializedList(const JsonList& list)
    : index(0)
    , length(list.Size())
    , array(list)
{
}

ErrCode JsonSerializedList::readSerializedList(ISerializedList** list)
{
    OPENDAQ_PARAM_NOT_NULL(list);

    if (index >= length)
    {
        return OPENDAQ_ERR_OUTOFRANGE;
    }

    if (array[index].IsArray())
    {
        auto serList = new(std::nothrow) JsonSerializedList(array[index++].GetArray());
        if (!serList)
        {
            return OPENDAQ_ERR_NOMEMORY;
        }

        serList->addRef();
        *list = serList;

        return OPENDAQ_SUCCESS;
    }
    return OPENDAQ_ERR_INVALIDTYPE;
}

ErrCode JsonSerializedList::readList(IBaseObject* context, IFunction* factoryCallback, IList** list)
{
    OPENDAQ_PARAM_NOT_NULL(list);

    if (index >= length)
    {
        return OPENDAQ_ERR_OUTOFRANGE;
    }

    auto& genericValue = array[index];
    if (genericValue.IsArray())
    {
        IBaseObject* object;
        ErrCode errCode = JsonDeserializerImpl::Deserialize(array[index++], context, factoryCallback, &object);

        if (OPENDAQ_FAILED(errCode))
        {
            return errCode;
        }

        *list = static_cast<IList*>(object);
        return OPENDAQ_SUCCESS;
    }

    if (genericValue.IsNull())
    {
        *list = nullptr;
        return OPENDAQ_SUCCESS;
    }

    return OPENDAQ_ERR_INVALIDTYPE;
}

ErrCode JsonSerializedList::readSerializedObject(ISerializedObject** plainObj)
{
    OPENDAQ_PARAM_NOT_NULL(plainObj);

    if (index >= length)
    {
        return OPENDAQ_ERR_OUTOFRANGE;
    }

    using namespace rapidjson;

    auto& genericValue = array[index];
    if (genericValue.IsObject())
    {
        auto serObj = new(std::nothrow) JsonSerializedObject(array[index++].GetObject());
        if (!serObj)
        {
            return OPENDAQ_ERR_NOMEMORY;
        }

        serObj->addRef();
        *plainObj = serObj;

        return OPENDAQ_SUCCESS;
    }

    if (genericValue.IsNull())
    {
        *plainObj = nullptr;
        return OPENDAQ_SUCCESS;
    }
    return OPENDAQ_ERR_INVALIDTYPE;
}

ErrCode JsonSerializedList::readObject(IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj)
{
    OPENDAQ_PARAM_NOT_NULL(obj);

    if (index >= length)
    {
        return OPENDAQ_ERR_OUTOFRANGE;
    }

    return JsonDeserializerImpl::Deserialize(array[index++], context, factoryCallback, obj);
}

ErrCode JsonSerializedList::readString(IString** obj)
{
    OPENDAQ_PARAM_NOT_NULL(obj);

    if (index >= length)
    {
        return OPENDAQ_ERR_OUTOFRANGE;
    }

    auto& genericValue = array[index];
    if (genericValue.IsString())
    {
        createString(obj, array[index++].GetString());

        return OPENDAQ_SUCCESS;
    }

    if (genericValue.IsNull())
    {
        *obj = nullptr;
        return OPENDAQ_SUCCESS;
    }

    return OPENDAQ_ERR_INVALIDTYPE;
}

ErrCode JsonSerializedList::readBool(Bool* obj)
{
    OPENDAQ_PARAM_NOT_NULL(obj);

    if (index >= length)
    {
        return OPENDAQ_ERR_OUTOFRANGE;
    }

    if (array[index].IsBool())
    {
        *obj = array[index++].GetBool();
        return OPENDAQ_SUCCESS;
    }

    return OPENDAQ_ERR_INVALIDTYPE;
}

ErrCode JsonSerializedList::readInt(Int* obj)
{
    OPENDAQ_PARAM_NOT_NULL(obj);

    if (index >= length)
    {
        return OPENDAQ_ERR_OUTOFRANGE;
    }

    if (array[index].IsInt())
    {
        *obj = array[index++].GetInt();
        return OPENDAQ_SUCCESS;
    }

    return OPENDAQ_ERR_INVALIDTYPE;
}

ErrCode JsonSerializedList::readFloat(Float* obj)
{
    OPENDAQ_PARAM_NOT_NULL(obj);

    if (index >= length)
    {
        return OPENDAQ_ERR_OUTOFRANGE;
    }

    if (array[index].IsDouble())
    {
        *obj = array[index++].GetDouble();
        return OPENDAQ_SUCCESS;
    }

    return OPENDAQ_ERR_INVALIDTYPE;
}

ErrCode JsonSerializedList::getCount(SizeT* size)
{
    OPENDAQ_PARAM_NOT_NULL(size);

    *size = length;

    return OPENDAQ_SUCCESS;
}

ErrCode JsonSerializedList::getCurrentItemType(CoreType* size)
{
    OPENDAQ_PARAM_NOT_NULL(size);

    if (index >= length)
    {
        return OPENDAQ_ERR_OUTOFRANGE;
    }

    *size = JsonDeserializerImpl::GetCoreType(array[index]);
    return OPENDAQ_SUCCESS;
}

ErrCode JsonSerializedList::toString(CharPtr* str)
{
    OPENDAQ_PARAM_NOT_NULL(str);

    return daqDuplicateCharPtr("JsonSerializedList", str);
}

END_NAMESPACE_OPENDAQ
