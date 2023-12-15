#include <coretypes/json_deserializer_impl.h>
#include <coretypes/coretypes.h>
#include <coretypes/json_serialized_object.h>
#include <coretypes/json_serialized_list.h>
#include <coretypes/updatable.h>
#include <coretypes/ctutils.h>
#include <rapidjson/document.h>
#include <memory>

BEGIN_NAMESPACE_OPENDAQ

// static
ErrCode JsonDeserializerImpl::DeserializeTagged(JsonValue& document, IBaseObject* context, IFunction* factoryCallback, IBaseObject** object)
{
    using namespace rapidjson;

    auto jsonObject = document.GetObject();

    if (jsonObject.HasMember("__type"))
    {
        if (jsonObject["__type"].IsString())
        {
            std::string typeId = jsonObject["__type"].GetString();

            SerializedObjectPtr jsonSerObj;
            auto errCode = createObject<ISerializedObject, JsonSerializedObject>(&jsonSerObj, jsonObject);
            if (OPENDAQ_FAILED(errCode))
            {
                return errCode;
            }

            bool constructedFromCallbackFactory = false;

            errCode = daqTry(
                [&factoryCallback, &typeId, &object, &context, &jsonSerObj, &constructedFromCallbackFactory]
                {
                    const auto factoryCallbackPtr = FunctionPtr::Borrow(factoryCallback);
                    if (factoryCallbackPtr.assigned())
                    {
                        *object = factoryCallbackPtr.call(String(typeId), jsonSerObj, context, factoryCallback).detach();
                        constructedFromCallbackFactory = *object != nullptr;
                    }

                    return OPENDAQ_SUCCESS;
                });

            if (OPENDAQ_FAILED(errCode))
            {
                return errCode;
            }

            daqDeserializerFactory factory{};
            if (!constructedFromCallbackFactory)
            {
                errCode = daqGetSerializerFactory(typeId.data(), &factory);

                if (OPENDAQ_FAILED(errCode))
                {
                    return errCode;
                }

                errCode = factory(jsonSerObj, context, factoryCallback, object);
            }

            if (OPENDAQ_FAILED(errCode))
            {
                return errCode;
            }

            return OPENDAQ_SUCCESS;
        }
        return OPENDAQ_ERR_DESERIALIZE_UNKNOWN_TYPE;
    }
    return OPENDAQ_ERR_DESERIALIZE_NO_TYPE;
}

ErrCode JsonDeserializerImpl::DeserializeList(const JsonList& array, IBaseObject* context, IFunction* factoryCallback, IBaseObject** object)
{
    IList* list;
    ErrCode errCode = createList(&list);
    if (OPENDAQ_FAILED(errCode))
    {
        return errCode;
    }

    for (auto& element : array)
    {
        IBaseObject* elementObj;
        errCode = Deserialize(element, context, factoryCallback, &elementObj);

        if (OPENDAQ_FAILED(errCode))
        {
            return errCode;
        }

        errCode = list->moveBack(elementObj);
        if (OPENDAQ_FAILED(errCode))
        {
            return errCode;
        }
    }
    *object = list;

    return OPENDAQ_SUCCESS;
}

// static
ErrCode JsonDeserializerImpl::Deserialize(JsonValue& document, IBaseObject* context, IFunction* factoryCallback, IBaseObject** object)
{
    ErrCode errCode = OPENDAQ_SUCCESS;

    switch (document.GetType())
    {
        case rapidjson::kNullType:
            object = nullptr;
            break;
        case rapidjson::kObjectType:
            errCode = DeserializeTagged(document, context, factoryCallback, object);
            break;
        case rapidjson::kStringType:
        {
            IString* list;
            errCode = createString(&list, document.GetString());
            *object = list;
            break;
        }
        case rapidjson::kNumberType:
            if (document.IsInt())
            {
                IInteger* integer;
                errCode = createInteger(&integer, document.GetInt());
                *object = integer;
            }
            else if (document.IsInt64())
            {
                IInteger* integer;
                errCode = createInteger(&integer, document.GetInt64());
                *object = integer;
            }
            else
            {
                IFloat* floating;
                errCode = createFloat(&floating, document.GetDouble());
                *object = floating;
            }
            break;
        case rapidjson::kArrayType:
            errCode = DeserializeList(document.GetArray(), context, factoryCallback, object);
            break;
        case rapidjson::kFalseType:
        {
            IBoolean* boolean;
            errCode = createBoolean(&boolean, false);
            *object = boolean;
            break;
        }
        case rapidjson::kTrueType:
        {
            IBoolean* boolean;
            errCode = createBoolean(&boolean, true);
            *object = boolean;
            break;
        }
        default:
            *object = nullptr;
            return OPENDAQ_ERR_DESERIALIZE_UNKNOWN_TYPE;
    }
    return errCode;
}

ErrCode JsonDeserializerImpl::deserialize(IString* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** object)
{
    if (serialized == nullptr)
    {
        return OPENDAQ_ERR_ARGUMENT_NULL;
    }

    SizeT length;
    serialized->getLength(&length);

    ConstCharPtr ptr;
    serialized->getCharPtr(&ptr);

#if defined(_WIN32)
    constexpr size_t dataPaddingSize = 0;
#else
//heap-buffer-overflow in _mm_load_si128 https://github.com/Tencent/rapidjson/issues/1723 
    constexpr size_t dataPaddingSize = 16;
#endif 

    char* buffer = new(std::nothrow) char[length + 1 + dataPaddingSize * 2];
    if (!buffer)
    {
        return OPENDAQ_ERR_NOMEMORY;
    }

    JsonDocument document;
    strcpy(&buffer[dataPaddingSize], ptr);

    if (document.ParseInsitu(&buffer[dataPaddingSize]).HasParseError())
    {
        delete[] buffer;
        return OPENDAQ_ERR_DESERIALIZE_PARSE_ERROR;
    }

    ErrCode errCode = Deserialize(document, context, factoryCallback, object);
    delete[] buffer;

    return errCode;
}

ErrCode JsonDeserializerImpl::update(IUpdatable* updatable, IString* serialized)
{
    if (serialized == nullptr || updatable == nullptr)
    {
        return OPENDAQ_ERR_ARGUMENT_NULL;
    }

    SizeT length;
    ErrCode err = serialized->getLength(&length);

    if (!OPENDAQ_SUCCEEDED(err))
    {
        return err;
    }

    ConstCharPtr ptr;
    err = serialized->getCharPtr(&ptr);
    if (!OPENDAQ_SUCCEEDED(err))
    {
        return err;
    }

    std::unique_ptr<char[]> buffer(new(std::nothrow) char[length + 1]);
    if (!buffer)
    {
        return OPENDAQ_ERR_NOMEMORY;
    }

    JsonDocument document;
    strcpy(buffer.get(), ptr);

    if (document.ParseInsitu(buffer.get()).HasParseError())
    {
        return OPENDAQ_ERR_DESERIALIZE_PARSE_ERROR;
    }

    if (document.GetType() != rapidjson::kObjectType)
    {
        return OPENDAQ_ERR_INVALIDTYPE;
    }

    SerializedObjectPtr jsonSerObj;
    ErrCode errCode = createObject<ISerializedObject, JsonSerializedObject>(&jsonSerObj, document.GetObject());
    if (OPENDAQ_FAILED(errCode))
    {
        return errCode;
    }

    return updatable->update(jsonSerObj);
}

ErrCode JsonDeserializerImpl::toString(CharPtr* str)
{
    if (str == nullptr)
    {
        return OPENDAQ_ERR_ARGUMENT_NULL;
    }

    return daqDuplicateCharPtr("JsonDeserializer", str);
}

CoreType JsonDeserializerImpl::GetCoreType(const JsonValue& value) noexcept
{
    switch (value.GetType())
    {
        case rapidjson::kNullType:
            return ctObject;
        case rapidjson::kFalseType:
        case rapidjson::kTrueType:
            return ctBool;
        case rapidjson::kObjectType:
            return ctObject;
        case rapidjson::kArrayType:
            return ctList;
        case rapidjson::kStringType:
            return ctString;
        case rapidjson::kNumberType:
            if (value.IsInt() || value.IsInt64())
            {
                return ctInt;
            }
            return ctFloat;
        default:
            return ctUndefined;
    }
}

// createJsonDeserializer
extern "C"
ErrCode PUBLIC_EXPORT createJsonDeserializer(IDeserializer** jsonDeserializer)
{
    if (!jsonDeserializer)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    IDeserializer* object = new(std::nothrow) JsonDeserializerImpl();

    if (!object)
        return OPENDAQ_ERR_NOMEMORY;

    object->addRef();

    *jsonDeserializer = object;
    return OPENDAQ_SUCCESS;
}

END_NAMESPACE_OPENDAQ
