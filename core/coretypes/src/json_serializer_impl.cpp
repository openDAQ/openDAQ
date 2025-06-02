#include <coretypes/stringobject_factory.h>
#include <coretypes/json_serializer_impl.h>
#include <rapidjson/prettywriter.h>

BEGIN_NAMESPACE_OPENDAQ

template <typename TWriter>
JsonSerializerImpl<TWriter>::JsonSerializerImpl()
    : writer(buffer)
    , version(3)
{
}

template <typename TWriter>
JsonSerializerImpl<TWriter>::JsonSerializerImpl(Int version)
    : writer(buffer)
    , version(version)
{
}

template <typename TWriter>
ErrCode JsonSerializerImpl<TWriter>::startTaggedObject(ISerializable* serializable)
{
    OPENDAQ_PARAM_NOT_NULL(serializable);

    ConstCharPtr id;
    ErrCode errCode = serializable->getSerializeId(&id);
    OPENDAQ_RETURN_IF_FAILED(errCode);

    writer.StartObject();
    writer.Key("__type");
    writer.String(id);

    return OPENDAQ_SUCCESS;
}

template <typename TWriter>
ErrCode JsonSerializerImpl<TWriter>::startObject()
{
    bool boolean = writer.StartObject();

    return boolean ? OPENDAQ_SUCCESS :  OPENDAQ_ERR_GENERALERROR;
}

template <typename TWriter>
ErrCode JsonSerializerImpl<TWriter>::startList()
{
    writer.StartArray();
    return OPENDAQ_SUCCESS;
}

template <typename TWriter>
ErrCode JsonSerializerImpl<TWriter>::endList()
{
    writer.EndArray();
    return OPENDAQ_SUCCESS;
}

inline ErrCode getCharLen(ConstCharPtr string, SizeT& length)
{
    OPENDAQ_PARAM_NOT_NULL(string);
    
    length = strlen(string);
    return OPENDAQ_SUCCESS;
}

template <typename TWriter>
ErrCode JsonSerializerImpl<TWriter>::keyRaw(ConstCharPtr string, SizeT length)
{
    OPENDAQ_PARAM_NOT_NULL(string);

    if (length <= 0)
    {
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER);
    }

    writer.Key(string, static_cast<rapidjson::SizeType>(length));

    return OPENDAQ_SUCCESS;
}

template <typename TWriter>
ErrCode JsonSerializerImpl<TWriter>::key(ConstCharPtr string)
{
    SizeT length;
    ErrCode errCode = getCharLen(string, length);
    OPENDAQ_RETURN_IF_FAILED(errCode);

    if (length == 0)
    {
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER);
    }

    writer.Key(string, static_cast<rapidjson::SizeType>(length));

    return OPENDAQ_SUCCESS;
}

template <typename TWriter>
ErrCode JsonSerializerImpl<TWriter>::keyStr(IString* name)
{
    OPENDAQ_PARAM_NOT_NULL(name);

    ConstCharPtr str;
    ErrCode errCode = name->getCharPtr(&str);
    OPENDAQ_RETURN_IF_FAILED(errCode);

    OPENDAQ_PARAM_NOT_NULL(str);

    SizeT length;
    errCode = name->getLength(&length);

    if (length == 0)
    {
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER);
    }

    writer.Key(str, static_cast<rapidjson::SizeType>(length));

    return errCode;
}

template <typename TWriter>
ErrCode JsonSerializerImpl<TWriter>::writeInt(Int integer)
{
    writer.Int64(integer);

    return OPENDAQ_SUCCESS;
}

template <typename TWriter>
ErrCode JsonSerializerImpl<TWriter>::writeBool(Bool boolean)
{
    writer.Bool(boolean == True);

    return OPENDAQ_SUCCESS;
}

template <typename TWriter>
ErrCode JsonSerializerImpl<TWriter>::writeFloat(Float real)
{
    writer.Double(real);

    return OPENDAQ_SUCCESS;
}

template <typename TWriter>
ErrCode JsonSerializerImpl<TWriter>::writeNull()
{
    writer.Null();

    return OPENDAQ_SUCCESS;
}

template <typename TWriter>
ErrCode JsonSerializerImpl<TWriter>::reset()
{
    buffer.Clear();
    writer.Reset(buffer);

    return OPENDAQ_SUCCESS;
}

template <typename TWriter>
ErrCode JsonSerializerImpl<TWriter>::isComplete(Bool* complete)
{
    *complete = writer.IsComplete();

    return OPENDAQ_SUCCESS;
}

template <typename TWriter>
ErrCode JsonSerializerImpl<TWriter>::endObject()
{
    writer.EndObject();

    return OPENDAQ_SUCCESS;
}

template <typename TWriter>
ErrCode JsonSerializerImpl<TWriter>::writeString(ConstCharPtr string, SizeT length)
{
    if (length == 0)
    {
        writer.String("", 0u);
    }
    else
    {
        writer.String(string, static_cast<rapidjson::SizeType>(length));
    }

    return OPENDAQ_SUCCESS;
}

template <typename TWriter>
ErrCode INTERFACE_FUNC JsonSerializerImpl<TWriter>::getUser(daq::IBaseObject** user)
{
    OPENDAQ_PARAM_NOT_NULL(user);

    *user = this->userContext.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

template <typename TWriter>
ErrCode INTERFACE_FUNC JsonSerializerImpl<TWriter>::setUser(daq::IBaseObject* user)
{
    this->userContext = user;
    return OPENDAQ_SUCCESS;
}

template <typename TWriter>
ErrCode JsonSerializerImpl<TWriter>::getVersion(Int* version)
{
    OPENDAQ_PARAM_NOT_NULL(version);

    *version = this->version;
    return OPENDAQ_SUCCESS;
}

template <typename TWriter>
ErrCode JsonSerializerImpl<TWriter>::getOutput(IString** output)
{
    *output = String_Create(buffer.GetString());

    return OPENDAQ_SUCCESS;
}

// createJsonSerializer
extern "C"
ErrCode PUBLIC_EXPORT createJsonSerializer(ISerializer** jsonSerializer, Bool pretty = False)
{
    OPENDAQ_PARAM_NOT_NULL(jsonSerializer);

    ISerializer* object;
    if (pretty)
    {
        object = new(std::nothrow) PrettyJsonSerializer();
    }
    else
    {
        object = new(std::nothrow) JsonSerializerImpl<>();
    }

    if (!object)
    {
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOMEMORY);
    }

    object->addRef();
    *jsonSerializer = object;
    return OPENDAQ_SUCCESS;
}

// createJsonSerializer
extern "C"
ErrCode PUBLIC_EXPORT createJsonSerializerWithVersion(ISerializer** jsonSerializer, Int version, Bool pretty = False)
{
    OPENDAQ_PARAM_NOT_NULL(jsonSerializer);

    ISerializer* object;
    if (pretty)
    {
        object = new(std::nothrow) PrettyJsonSerializer(version);
    }
    else
    {
        object = new(std::nothrow) JsonSerializerImpl<>(version);
    }

    if (!object)
    {
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOMEMORY);
    }

    object->addRef();
    *jsonSerializer = object;
    return OPENDAQ_SUCCESS;
}

END_NAMESPACE_OPENDAQ
