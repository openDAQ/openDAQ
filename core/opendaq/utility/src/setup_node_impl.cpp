#include <opendaq/setup_node_impl.h>
#include <rapidjson/error/en.h>
#include <coreobjects/property_object_factory.h>

BEGIN_NAMESPACE_OPENDAQ

SetupNodeImpl::SetupNodeImpl(const StringPtr& setupString)
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
    if (!read("", document, nullptr, getNodeType(document)))
        throw DeserializeException{"Failed to read setup!"};
}

ErrCode SetupNodeImpl::getLocalId(IString** id)
{
    OPENDAQ_PARAM_NOT_NULL(id);

    *id = localId.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode SetupNodeImpl::getType(NodeType* type)
{
    OPENDAQ_PARAM_NOT_NULL(type);

    *type = nodeType;
    return OPENDAQ_SUCCESS;
}

ErrCode SetupNodeImpl::getOptions(IPropertyObject** options)
{
    OPENDAQ_PARAM_NOT_NULL(options);

    *options = this->options.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode SetupNodeImpl::getChildNodes(IList** childNodes)
{
    OPENDAQ_PARAM_NOT_NULL(children);
    
    *childNodes = this->children.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

namespace
{
    void collectTyped(const ListPtr<ISetupNode>& input, NodeType type, ListPtr<ISetupNode>& result)
    {
        for (const auto& node : input)
        {
            if (node.getType() == type)
                result.pushBack(node);
        }
    }

    void collectTypedRecursive(const ListPtr<ISetupNode>& input, NodeType type, ListPtr<ISetupNode>& result)
    {
        for (const auto& child : input)
        {
            if (child.getType() == type)
                result.pushBack(child);

            collectTypedRecursive(child.getChildNodes(), type, result);
        }
    }
}


ErrCode SetupNodeImpl::getChildNodesWithType(IList** childNodes, NodeType type, Bool recursive)
{
    OPENDAQ_PARAM_NOT_NULL(childNodes);
    
    ListPtr<ISetupNode> childNodesList = List<ISetupNode>();
    try
    {
        if (recursive)
            collectTypedRecursive(children, type, childNodesList);
        else
            collectTyped(children, type, childNodesList);
    }
    catch (std::exception& e)
    {
        return errorFromException(e);
    }
    
    *childNodes = childNodesList.detach();
    return OPENDAQ_SUCCESS;
}

ErrCode SetupNodeImpl::getParent(ISetupNode** parent)
{
    OPENDAQ_PARAM_NOT_NULL(parent);

    *parent = this->parent.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

PropertyObjectPtr SetupNodeImpl::createOptions(const rapidjson::Value& value, NodeType nt)
{
    return PropertyObject();
}

NodeType SetupNodeImpl::getNodeType(const rapidjson::Value& value)
{
    auto obj = value.GetObject();
    if (!obj.HasMember("__type"))
        return NodeType::Unknown;

    if (!obj["__type"].IsString())
        return NodeType::Unknown;

    std::string nodeTypeString = obj["__type"].GetString();

    if (nodeTypeString == "Device")
        return NodeType::Device;
    else if (nodeTypeString == "FunctionBlock")
        return NodeType::FunctionBlock;
    else if (nodeTypeString == "Signal")
        return NodeType::Signal;
    else if (nodeTypeString == "InputPort")
        return NodeType::InputPort;
    else if (nodeTypeString == "Server")
        return NodeType::Server;
    else if (nodeTypeString == "IoFolder")
        return NodeType::IOFolder;
    else if (nodeTypeString == "Channel")
        return NodeType::Channel;
    else if (nodeTypeString == "SyncComponent")
        return NodeType::SyncComponent;
    else if (nodeTypeString == "Component")
        return NodeType::Component;
    else if (nodeTypeString == "Folder")
        return NodeType::Folder;
    else if (nodeTypeString == "Instance")
        return NodeType::Instance;
    else
        return NodeType::Unknown;
}

bool SetupNodeImpl::read(const StringPtr& localId, const rapidjson::Value& value, const SetupNodePtr& parent, NodeType nodeType)
{
    switch (value.GetType())
    {
        case rapidjson::kObjectType:
            break;
        case rapidjson::kNullType:
        case rapidjson::kFalseType:
        case rapidjson::kTrueType:
        case rapidjson::kArrayType:
        case rapidjson::kStringType:
        case rapidjson::kNumberType:
            return false;
    }
        
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
        return read(rootDevice->name.GetString(), rootDevice->value, nullptr, NodeType::Device);
    }

    if (nodeType == NodeType::Folder || nodeType == NodeType::IOFolder)
    {
        if (jsonObject.HasMember("items"))
            return read(localId, jsonObject["items"], parent, nodeType);
    }

    this->nodeType = nodeType;
    options = createOptions(value, nodeType);
    this->localId = localId;
    this->parent = parent;
    children = List<ISetupNode>();
   
    for (const auto& member : jsonObject)
    {
        auto type = member.value.GetType();
        if (type != rapidjson::kObjectType)
            continue;

        auto childNodeType = getNodeType(member.value);
        if (childNodeType == NodeType::Unknown)
            continue;

        auto child = createWithImplementation<ISetupNode, SetupNodeImpl>();
        auto thisPtr = this->borrowPtr<SetupNodePtr>();
        if (dynamic_cast<SetupNodeImpl*>(child.getObject())->read(member.name.GetString(), member.value, thisPtr, childNodeType))
            children.pushBack(child);
    }

    if (nodeType == NodeType::Folder && !children.getCount())
        return false;

    children.freeze();
    return true;
}

OPENDAQ_DEFINE_CLASS_FACTORY(LIBRARY_FACTORY, SetupNode, IString*, setupString);

END_NAMESPACE_OPENDAQ
