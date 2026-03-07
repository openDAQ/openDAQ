#include <opendaq/device_update_options_impl.h>
#include <rapidjson/error/en.h>

BEGIN_NAMESPACE_OPENDAQ

DeviceUpdateOptionsImpl::DeviceUpdateOptionsImpl()
{
    isRoot = false;
    children = List<IDeviceUpdateOptions>();
    manufacturer = "";
    serialNumber = "";
    connectionString = "";
    mode = DeviceUpdateMode::Load;
}

DeviceUpdateOptionsImpl::DeviceUpdateOptionsImpl(const StringPtr& setupString)
    : DeviceUpdateOptionsImpl()
{
    SizeT length;
    setupString->getLength(&length);

    ConstCharPtr ptr;
    setupString->getCharPtr(&ptr);

#ifdef _WIN32
    constexpr size_t dataPaddingSize = 0;
#else
    //heap-buffer-overflow in _mm_load_si128 https://github.com/Tencent/rapidjson/issues/1723 
    constexpr size_t dataPaddingSize = 16;
#endif 

    char* buffer = new(std::nothrow) char[length + 1 + dataPaddingSize * 2];
    if (!buffer)
        throw NoMemoryException{"Failed to allocate buffer for JSON setup parsing."};

    rapidjson::Document document;
    strcpy(&buffer[dataPaddingSize], ptr);

    if (document.ParseInsitu(&buffer[dataPaddingSize]).HasParseError())
    {
        rapidjson::ParseErrorCode errorCode = document.GetParseError();
        size_t errorOffset = document.GetErrorOffset();
        auto errorMsg = fmt::format(R"(Failed to parse json configuration on {} position. Error: {})", errorOffset, rapidjson::GetParseError_En(errorCode));

        delete[] buffer;
        throw DeserializeException{"Failed to parse setup document!"};
    }

    internalAddRef();

    isRoot = true;
    if (!read("", document, getNodeType(document)))
        throw DeserializeException{"Failed to read setup!"};
}

ErrCode DeviceUpdateOptionsImpl::getLocalId(IString** localId)
{
    OPENDAQ_PARAM_NOT_NULL(localId);
    
    *localId = this->localId.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode DeviceUpdateOptionsImpl::getManufacturer(IString** manufacturer)
{
    OPENDAQ_PARAM_NOT_NULL(manufacturer);
    
    *manufacturer = this->manufacturer.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode DeviceUpdateOptionsImpl::getSerialNumber(IString** serialNumber)
{
    OPENDAQ_PARAM_NOT_NULL(serialNumber);
    
    *serialNumber = this->serialNumber.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}


ErrCode DeviceUpdateOptionsImpl::getConnectionString(IString** connectionString)
{
    OPENDAQ_PARAM_NOT_NULL(connectionString);
    
    *connectionString = this->connectionString.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode DeviceUpdateOptionsImpl::setNewManufacturer(IString* manufacturer)
{
    if (isRoot)
        return makeErrorInfo(OPENDAQ_ERR_ACCESSDENIED, "Root device setup options can not be modified.");

    this->newManufacturer = manufacturer;
    return OPENDAQ_SUCCESS;
}

ErrCode DeviceUpdateOptionsImpl::getNewManufacturer(IString** manufacturer)
{    
    OPENDAQ_PARAM_NOT_NULL(manufacturer);
    
    *manufacturer = this->newManufacturer.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode DeviceUpdateOptionsImpl::setNewSerialNumber(IString* serialNumber)
{
    if (isRoot)
        return makeErrorInfo(OPENDAQ_ERR_ACCESSDENIED, "Root device setup options can not be modified.");

    this->newSerialNumber = serialNumber;
    return OPENDAQ_SUCCESS;
}

ErrCode DeviceUpdateOptionsImpl::getNewSerialNumber(IString** serialNumber)
{
    OPENDAQ_PARAM_NOT_NULL(serialNumber);
    
    *serialNumber = this->newSerialNumber.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode DeviceUpdateOptionsImpl::setNewConnectionString(IString* connectionString)
{
    if (isRoot)
        return makeErrorInfo(OPENDAQ_ERR_ACCESSDENIED, "Root device setup options can not be modified.");

    this->newConnectionString = connectionString;
    return OPENDAQ_SUCCESS;
}

ErrCode DeviceUpdateOptionsImpl::getNewConnectionString(IString** connectionString)
{
    OPENDAQ_PARAM_NOT_NULL(connectionString);
    
    *connectionString = this->newConnectionString.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode DeviceUpdateOptionsImpl::getUpdateMode(DeviceUpdateMode* mode)
{
    OPENDAQ_PARAM_NOT_NULL(mode);
    
    *mode = this->mode;
    return OPENDAQ_SUCCESS;
}

ErrCode DeviceUpdateOptionsImpl::setUpdateMode(DeviceUpdateMode mode)
{
    this->mode = mode;
    return OPENDAQ_SUCCESS;
}

ErrCode DeviceUpdateOptionsImpl::getChildDeviceOptions(IList** childDeviceOptions)
{
    OPENDAQ_PARAM_NOT_NULL(childDeviceOptions);
    
    *childDeviceOptions = this->children.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

NodeType DeviceUpdateOptionsImpl::getNodeType(const rapidjson::Value& value)
{
    auto obj = value.GetObject();
    if (!obj.HasMember("__type"))
        return NodeType::Unknown;

    if (!obj["__type"].IsString())
        return NodeType::Unknown;

    std::string nodeTypeString = obj["__type"].GetString();
    
    if (nodeTypeString == "Instance")
        return NodeType::Instance;
    else if (nodeTypeString == "Folder")
        return NodeType::Folder;
    else if (nodeTypeString == "Device")
        return NodeType::Device;
    else
        return NodeType::Unknown;
}

bool DeviceUpdateOptionsImpl::read(const StringPtr& localId, const rapidjson::Value& value, NodeType nodeType)
{
    if (value.GetType() != rapidjson::kObjectType)
        return false;
        
    if (nodeType == NodeType::Unknown)
        return false;
    
    auto jsonObject = value.GetObject();

    if (nodeType == NodeType::Instance)
    {
        if (!jsonObject.HasMember("rootDevice"))
            return false;

        const auto& rootDeviceWrapper = jsonObject["rootDevice"];
        if (rootDeviceWrapper.MemberCount() != 1)
            return false;

        auto rootDevice = rootDeviceWrapper.MemberBegin();
        return readDevice(rootDevice->name.GetString(), rootDevice->value);
    }

    if (nodeType == NodeType::Device)
        return readDevice(localId, value);

    return false;
}

bool DeviceUpdateOptionsImpl::readFolder(const rapidjson::Value& document)
{        
    auto jsonObject = document.GetObject();
    if (!jsonObject.HasMember("items"))
        return true; // Allow empty device items folder
       
    const auto items = jsonObject["items"].GetObject();
    for (const auto& member : items)
    {
        auto type = member.value.GetType();
        if (type != rapidjson::kObjectType)
            continue;

        if (getNodeType(member.value) != NodeType::Device)
            continue;

        auto child = createWithImplementation<IDeviceUpdateOptions, DeviceUpdateOptionsImpl>();
        auto thisPtr = this->borrowPtr<DeviceUpdateOptionsPtr>();
        if (dynamic_cast<DeviceUpdateOptionsImpl*>(child.getObject())->readDevice(member.name.GetString(), member.value))
            children.pushBack(child);
    }

    return true;
}

bool DeviceUpdateOptionsImpl::readDevice(const StringPtr& localId, const rapidjson::Value& document)
{
    auto jsonObject = document.GetObject();
    if (jsonObject.HasMember("manufacturer") && jsonObject["manufacturer"].IsString())
        manufacturer = jsonObject["manufacturer"].GetString();
    if (jsonObject.HasMember("serialNumber") && jsonObject["serialNumber"].IsString())
        serialNumber = jsonObject["serialNumber"].GetString();
    if (jsonObject.HasMember("connectionString") && jsonObject["connectionString"].IsString())
        connectionString = jsonObject["connectionString"].GetString();

    this->localId = localId;

    if (jsonObject.HasMember("Dev") && !readFolder(jsonObject["Dev"]))
        return false;

    children.freeze();
    return true;
}

OPENDAQ_DEFINE_CLASS_FACTORY(LIBRARY_FACTORY, DeviceUpdateOptions, IString*, setupString);

END_NAMESPACE_OPENDAQ
