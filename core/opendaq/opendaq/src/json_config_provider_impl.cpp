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
#include <cctype>

BEGIN_NAMESPACE_OPENDAQ

JsonConfigProviderImpl::JsonConfigProviderImpl(const StringPtr& filename)
    :filename(filename)
{
    if (!this->filename.assigned() || this->filename.getLength() == 0)
        this->filename = GetEnvironmentVariableValue("OPENDAQ_CONFIG_PATH", "opendaq-config.json");
}

std::string JsonConfigProviderImpl::ToLowerCase(const std::string &input) 
{
    std::string result = input;
    for (char &c : result)
        c = std::tolower(static_cast<unsigned char>(c));

    return result;
}

StringPtr JsonConfigProviderImpl::GetEnvironmentVariableValue(StringPtr variableName, StringPtr defaultValue)
{
    if (!variableName.assigned() || variableName.getLength() == 0)
        return defaultValue;
    
    const char* value = std::getenv(variableName.toStdString().c_str());

    if (value)
        return String(value);
    else
        return defaultValue;
}

StringPtr JsonConfigProviderImpl::GetDataFromFile(const StringPtr& filename)
{
    std::ifstream file (filename.toStdString());

    if (!file)
        throw NotFoundException(fmt::format("Json config file \"{}\" not found", filename));

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

    // json value type have to be number;
    return {};
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
            // json value type have to be primitive
            return {};
        }
    };
}

void JsonConfigProviderImpl::HandleArray(const BaseObjectPtr& options, const rapidjson::Value& value)
{
    if (!value.IsArray())
    {
        // json value type have to be array;
        return;
    }
    
    if (!options.assigned())
        return;  
    
    auto optionsPtr = options.asPtrOrNull<IList>();
    if (!optionsPtr.assigned())
    {
        // options node can not be converted to list
        return;
    }

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
    {
        // json value type have to be object
        return;
    }
    
    if (!options.assigned())
        return;   
    
    auto optionsPtr = options.asPtrOrNull<IDict, DictPtr<IString, IBaseObject>>();
    if (!optionsPtr.assigned())
    {
        // options node can not be converted to dict
        return;
    }
    
    for (auto & el : value.GetObject())
    {
        auto optionName = ToLowerCase(el.name.GetString());
        BaseObjectPtr optionValue;
        if (optionsPtr.hasKey(optionName))
            optionValue = optionsPtr.get(optionName);

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
            if (optionValue.assigned() && (!parsedValue.assigned() || optionValue.getCoreType() != parsedValue.getCoreType()))
            {
                // json primtive type mistamatch
                continue;
            }
                
            optionValue = parsedValue;
        }
        optionsPtr.set(optionName, optionValue);
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
