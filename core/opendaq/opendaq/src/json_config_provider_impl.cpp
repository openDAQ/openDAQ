#include <coreobjects/errors.h>
#include <opendaq/custom_log.h>
#include <opendaq/json_config_provider_impl.h>
#include <coretypes/validation.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <utility>

#include <rapidjson/error/en.h>
#include <rapidjson/document.h>
#include <rapidjson/filereadstream.h> 

BEGIN_NAMESPACE_OPENDAQ

JsonConfigProviderImpl::JsonConfigProviderImpl(const StringPtr& filename)
{
    if (!filename.assigned())
        throw ArgumentNullException("filename for json config provider is not assigned");

    this->filename = filename; 
}

StringPtr JsonConfigProviderImpl::getDataFromFile(const StringPtr& filename)
{
    std::ifstream file (filename.toStdString());

    if (!file)
        throw NotFoundException("Json config file not found");

    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

ErrCode JsonConfigProviderImpl::populateOptions(IDict* options)
{
    OPENDAQ_PARAM_NOT_NULL(options);
    DictPtr<IString, IBaseObject> optionsPtr = options;

    auto jsonStr = getDataFromFile(filename);

    rapidjson::Document document;
    if (document.Parse(jsonStr.toStdString().c_str()).HasParseError())
    {
        // rapidjson::ParseErrorCode errorCode = document.GetParseError();
        // size_t errorOffset = document.GetErrorOffset();
        // LOG_E("Failed to parse json configuration on {} position. Error: {}", errorOffset, rapidjson::GetParseError_En(errorCode));
        return OPENDAQ_ERR_DESERIALIZE_PARSE_ERROR;
    }

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
