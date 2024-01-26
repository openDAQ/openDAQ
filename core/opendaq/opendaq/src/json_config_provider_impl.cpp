#include <coreobjects/errors.h>
#include <opendaq/custom_log.h>
#include <opendaq/json_config_provider_impl.h>
#include <coretypes/validation.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <utility>
#include <coretypes/ctutils.h>
#include <rapidjson/error/en.h>
#include <coretypes/coretypes.h>
#include <rapidjson/filereadstream.h> 

BEGIN_NAMESPACE_OPENDAQ

JsonConfigProviderImpl::JsonConfigProviderImpl(const StringPtr& filename)
{
    if (!filename.assigned())
        throw ArgumentNullException("filename for json config provider is not assigned");

    this->filename = filename; 
}

StringPtr JsonConfigProviderImpl::GetDataFromFile(const StringPtr& filename)
{
    std::ifstream file (filename.toStdString());

    if (!file)
        throw NotFoundException("Json config file not found");

    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

BaseObjectPtr JsonConfigProviderImpl::HandleNumber(const rapidjson::Value& value)
{
    if (value.IsInt()) 
    {
        return Integer(value.GetInt());
    } 
    if (value.IsUint()) 
    {
        return Integer(value.GetUint());
    } 
    if (value.IsInt64()) 
    {
        return Integer(value.GetInt64());
    } 
    if (value.IsUint64()) 
    {
        return Integer(value.GetUint64());
    } 
    if (value.IsDouble()) 
    {
        return Floating(value.GetDouble());
    } 
    throw InvalidTypeException("json value type have to be number"); 
}
BaseObjectPtr JsonConfigProviderImpl::HandlePrimitive(const rapidjson::Value& value)
{
    switch (value.GetType())
    {
        case (rapidjson::Type::kNullType):
        {
            return {};
        }
        case (rapidjson::Type::kFalseType):
        case (rapidjson::Type::kTrueType):
        {
            return Boolean(value.GetBool());
        }
        case (rapidjson::Type::kStringType):
        {
            return String(value.GetString());
        }
        case (rapidjson::Type::kNumberType):
        {
            return HandleNumber(value);
        }
        default:
        {
            throw InvalidTypeException("json value type have to be primitive"); 
        }
    };
}
void JsonConfigProviderImpl::HandleArray(const BaseObjectPtr& options, const rapidjson::Value& value)
{
    if (!value.IsArray())
        throw InvalidTypeException("json value type have to be array"); 
    
    if (!options.assigned())
        throw ArgumentNullException("options is null");    
    
    auto optionsPtr = options.asPtrOrNull<IList>();
    if (!optionsPtr.assigned())
        throw InvalidTypeException("json type mistamatch");

    optionsPtr.clear();
    for (auto & el : value.GetArray())
    {
        BaseObjectPtr optionValue;
        if (el.IsObject()) 
        {
            optionValue = Dict<IString, IBaseObject>();
            HandleObject(optionValue, el);
        } 
        else if (el.IsArray()) 
        {
            optionValue = List<IBaseObject>();
            HandleArray(optionValue, el);
        }
        else
        {
            optionValue = HandlePrimitive(el);
        }
        optionsPtr.pushBack(optionValue);
    }
}
void JsonConfigProviderImpl::HandleObject(const BaseObjectPtr& options, const rapidjson::Value& value)
{
    if (!value.IsObject())
        throw InvalidTypeException("json value type have to be object"); 
    
    if (!options.assigned())
        throw ArgumentNullException("options is null");    
    
    auto optionsPtr = options.asPtrOrNull<IDict, DictPtr<IString, IBaseObject>>();
    if (!optionsPtr.assigned())
        throw InvalidTypeException("json type mistamatch");
    
    for (auto & el : value.GetObject())
    {
        BaseObjectPtr optionValue;
        if (optionsPtr.hasKey(el.name.GetString()))
            optionValue = optionsPtr.get(el.name.GetString());

        if (el.value.IsObject()) 
        {
            if (!optionValue.assigned())
                optionValue = Dict<IString, IBaseObject>();

            HandleObject(optionValue, el.value);
        } 
        else if (el.value.IsArray()) 
        {
            if (!optionValue.assigned())
                optionValue = List<IBaseObject>();

            HandleArray(optionValue, el.value);
        }
        else
        {
            auto parsedValue = HandlePrimitive(el.value);
            if (optionValue.assigned() && optionValue.getCoreType() != parsedValue.getCoreType())
                throw InvalidTypeException("json primtive type mistamatch");
            optionValue = parsedValue;
        }
        optionsPtr.set(el.name.GetString(), optionValue);
    }
}

ErrCode JsonConfigProviderImpl::populateOptions(IDict* options)
{
    OPENDAQ_PARAM_NOT_NULL(options);

    auto jsonStr = GetDataFromFile(filename);

    rapidjson::Document document;
    if (document.Parse(jsonStr.toStdString().c_str()).HasParseError())
    {
        rapidjson::ParseErrorCode errorCode = document.GetParseError();
        size_t errorOffset = document.GetErrorOffset();

        auto errorMsg = fmt::format(R"(Failed to parse json configuration on {} position. Error: {})", errorOffset, rapidjson::GetParseError_En(errorCode));
        return this->makeErrorInfo(OPENDAQ_ERR_DESERIALIZE_PARSE_ERROR, errorMsg);
    }

    HandleObject(BaseObjectPtr::Borrow(options), document);

    return OPENDAQ_SUCCESS;
}

/////////////////////
////
//// FACTORIES
////
////////////////////

extern "C" ErrCode PUBLIC_EXPORT createJsonConfigProvider(IConfigProvider** objTmp, IString* filename)
{
    return daq::createObject<IConfigProvider, JsonConfigProviderImpl>(objTmp, filename);
}


END_NAMESPACE_OPENDAQ
