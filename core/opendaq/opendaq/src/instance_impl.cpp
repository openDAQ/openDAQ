#include <opendaq/client_private.h>
#include <opendaq/context_ptr.h>
#include <opendaq/context_internal_ptr.h>
#include <opendaq/device_info_factory.h>
#include <opendaq/ids_parser.h>
#include <opendaq/instance_factory.h>
#include <opendaq/instance_impl.h>
#include <opendaq/create_device.h>
#include <opendaq/input_port_private_ptr.h>
#include <coretypes/validation.h>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <opendaq/custom_log.h>

BEGIN_NAMESPACE_OPENDAQ

InstanceImpl::InstanceImpl(ContextPtr context, const StringPtr& localId)
    : context(std::move(context))
    , moduleManager(this->context.assigned() ? this->context.asPtr<IContextInternal>().moveModuleManager() : nullptr)
    , rootDeviceSet(false)
{
    auto instanceId = defineLocalId(localId.assigned() ? localId.toStdString() : std::string());
    defaultRootDevice = Client(this->context, instanceId);
    defaultRootDevice.asPtrOrNull<IPropertyObjectInternal>().enableCoreEventTrigger();
    rootDevice = defaultRootDevice;
    loggerComponent = this->context.getLogger().addComponent("Instance");
}

std::string InstanceImpl::defineLocalId(const std::string& localId)
{
    if (!localId.empty())
        return localId;

    auto* env = std::getenv("OPENDAQ_INSTANCE_ID");
    if (env != nullptr)
        return env;

    boost::uuids::random_generator gen;
    const auto uuidBoost = gen();
    return boost::uuids::to_string(uuidBoost);
}

InstanceImpl::~InstanceImpl()
{
    stopServers();
    rootDevice.release();
    defaultRootDevice.release();
}

void InstanceImpl::stopServers()
{
    for (const auto& server : servers)
        server->stop();
}

ComponentPtr InstanceImpl::findComponentInternal(const ComponentPtr& component, const std::string& id)
{
    std::string startStr;
    std::string restStr;
    const bool hasSubComponentStr = IdsParser::splitRelativeId(id, startStr, restStr);
    if (!hasSubComponentStr)
        startStr = id;

    const auto folder = component.asPtrOrNull<IFolder>(true);
    if (!folder.assigned())
        return nullptr;

    if (folder.hasItem(startStr))
    {
        const auto subComponent = folder.getItem(startStr);
        if (hasSubComponentStr)
            return findComponentInternal(subComponent, restStr);

        return subComponent;
    }

    return nullptr;
}

bool InstanceImpl::isDefaultRootDevice()
{
    return rootDevice == defaultRootDevice;
}

ErrCode InstanceImpl::getContext(IContext** context)
{
    OPENDAQ_PARAM_NOT_NULL(context);

    *context = this->context.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode InstanceImpl::getModuleManager(IModuleManager** manager)
{
    OPENDAQ_PARAM_NOT_NULL(manager);

    *manager = this->moduleManager.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode InstanceImpl::getAvailableServerTypes(IDict** servers)
{
    OPENDAQ_PARAM_NOT_NULL(servers);

    auto result = Dict<IString, IServerType>();
    for (const auto module_ : moduleManager.getModules())
    {
        DictPtr<IString, IServerType> moduleServerTypes;

        try
        {
            moduleServerTypes = module_.getAvailableServerTypes();
        }
        catch (NotImplementedException&)
        {
            moduleServerTypes = nullptr;
        }

        if (!moduleServerTypes.assigned())
            continue;

        for (const auto& [id, serverType] : moduleServerTypes)
            result[id] = serverType;
    }

    *servers = result.detach();

    return OPENDAQ_SUCCESS;
}

ErrCode InstanceImpl::addServer(IString* serverTypeId, IPropertyObject* serverConfig, IServer** server)
{
    OPENDAQ_PARAM_NOT_NULL(serverTypeId);
    OPENDAQ_PARAM_NOT_NULL(server);

    auto typeId = toStdString(serverTypeId);

    for (const auto module : moduleManager.getModules())
    {
        DictPtr<IString, IServerType> serverTypes;
        try
        {
            serverTypes = module.getAvailableServerTypes();
        }
        catch (NotImplementedException&)
        {
            serverTypes = nullptr;
        }

        if (!serverTypes.assigned())
            continue;

        for (const auto& [id, serverType] : serverTypes)
        {
            if (id == typeId)
            {
                // Use root device instead of Instance(this) to prevent cycling reference.
                auto createdServer = module.createServer(typeId, rootDevice, serverConfig);

                std::scoped_lock lock(configSync);
                servers.push_back(createdServer);
                *server = createdServer.detach();
                return OPENDAQ_SUCCESS;
            }
        }
    }

    return OPENDAQ_ERR_NOTFOUND;
}

ErrCode InstanceImpl::addStandardServers(IList** servers)
{
    OPENDAQ_PARAM_NOT_NULL(servers);

    auto serversPtr = List<IServer>();
    ErrCode errCode = OPENDAQ_SUCCESS;

#if defined(OPENDAQ_ENABLE_NATIVE_STREAMING)
    ServerPtr nativeStreamingServer;
    errCode = addServer(String("openDAQ Native Streaming"), nullptr, &nativeStreamingServer);
    if (OPENDAQ_FAILED(errCode))
        return errCode;
    serversPtr.pushBack(nativeStreamingServer);
#elif defined(OPENDAQ_ENABLE_WEBSOCKET_STREAMING)
    ServerPtr websocketServer;
    errCode = addServer(String("openDAQ WebsocketTcp Streaming"), nullptr, &websocketServer);
    if (OPENDAQ_FAILED(errCode))
        return errCode;
    serversPtr.pushBack(websocketServer);
#endif

    ServerPtr opcUaServer;
    errCode = addServer(String("openDAQ OpcUa"), nullptr, &opcUaServer);
    if (OPENDAQ_FAILED(errCode))
        return errCode;
    serversPtr.pushBack(opcUaServer);

    *servers = serversPtr.detach();

    return OPENDAQ_SUCCESS;
}

ErrCode InstanceImpl::removeServer(IServer* server)
{
    OPENDAQ_PARAM_NOT_NULL(server);

    std::scoped_lock lock(configSync);

    auto it = std::find(servers.begin(), servers.end(), server);
    if (it == servers.end())
        return OPENDAQ_ERR_NOTFOUND;

    auto errCode = it->getObject()->stop();

    servers.erase(it);

    return errCode;
}

ErrCode InstanceImpl::getServers(IList** servers)
{
    OPENDAQ_PARAM_NOT_NULL(servers);

    ListPtr<IServer> serversPtr{ this->servers };
    *servers = serversPtr.detach();

    return OPENDAQ_SUCCESS;
}

ErrCode InstanceImpl::getRootDevice(IDevice** rootDevice)
{
    OPENDAQ_PARAM_NOT_NULL(rootDevice);

    *rootDevice = this->rootDevice.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode InstanceImpl::setRootDevice(IString* connectionString, IPropertyObject* config)
{
    OPENDAQ_PARAM_NOT_NULL(rootDevice);

    if (rootDeviceSet)
        return makeErrorInfo(OPENDAQ_ERR_INVALIDSTATE, "Root device already set.");

    if (defaultRootDevice.getFunctionBlocks().getCount() > 0)
        return makeErrorInfo(OPENDAQ_ERR_INVALIDSTATE, "Cannot set root device if function blocks already added");

    if (defaultRootDevice.getDevices().getCount() > 0)
        return makeErrorInfo(OPENDAQ_ERR_INVALIDSTATE, "Cannot set root device if devices are already added");

    if (!servers.empty())
        return makeErrorInfo(OPENDAQ_ERR_INVALIDSTATE, "Cannot set root device if servers are already added");

    const auto newRootDevice = detail::createDevice(connectionString, config, nullptr, moduleManager, loggerComponent);

    auto errCode = defaultRootDevice.asPtr<IClientPrivate>(true)->setRootDevice(newRootDevice);
    if (OPENDAQ_FAILED(errCode))
        return errCode;

    this->rootDevice = newRootDevice;
    rootDeviceSet = true;

    this->rootDevice.asPtrOrNull<IPropertyObjectInternal>().enableCoreEventTrigger();
    return OPENDAQ_SUCCESS;
}

// IDevice

ErrCode InstanceImpl::getInfo(IDeviceInfo** info)
{
    return rootDevice->getInfo(info);
}

ErrCode InstanceImpl::getDomain(IDeviceDomain** deviceDomain)
{
    return rootDevice->getDomain(deviceDomain);
}

ErrCode InstanceImpl::getInputsOutputsFolder(IFolder** inputsOutputsFolder)
{
    return rootDevice->getInputsOutputsFolder(inputsOutputsFolder);
}

ErrCode InstanceImpl::getCustomComponents(IList** customFolders)
{
    return rootDevice->getCustomComponents(customFolders);
}

ErrCode InstanceImpl::getSignals(IList** signals)
{
    return rootDevice->getSignals(signals);
}

ErrCode InstanceImpl::getSignalsRecursive(IList** signals)
{
    return rootDevice->getSignalsRecursive(signals);
}

// IDeviceDomain

ErrCode InstanceImpl::getTickResolution(IRatio** resolution)
{
    const auto deviceDomain = rootDevice.asPtrOrNull<IDeviceDomain>();
    if (deviceDomain.assigned())
        return deviceDomain->getTickResolution(resolution);

    return makeErrorInfo(OPENDAQ_ERR_NOINTERFACE, "Root device does not contain a device domain.");
}

ErrCode InstanceImpl::getTicksSinceOrigin(uint64_t* ticks)
{
    const auto deviceDomain = rootDevice.asPtrOrNull<IDeviceDomain>();
    if (deviceDomain.assigned())
        return deviceDomain->getTicksSinceOrigin(ticks);

    return makeErrorInfo(OPENDAQ_ERR_NOINTERFACE, "Root device does not contain a device domain.");
}

ErrCode InstanceImpl::getOrigin(IString** origin)
{
    const auto deviceDomain = rootDevice.asPtrOrNull<IDeviceDomain>();
    if (deviceDomain.assigned())
        return deviceDomain->getOrigin(origin);

    return makeErrorInfo(OPENDAQ_ERR_NOINTERFACE, "Root device does not contain a device domain.");
}

ErrCode InstanceImpl::getUnit(IUnit** unit)
{
    const auto deviceDomain = rootDevice.asPtrOrNull<IDeviceDomain>();
    if (deviceDomain.assigned())
        return deviceDomain->getUnit(unit);

    return makeErrorInfo(OPENDAQ_ERR_NOINTERFACE, "Root device does not contain a device domain.");
}

ErrCode InstanceImpl::getLocalId(IString** localId)
{
    return rootDevice->getLocalId(localId);
}

ErrCode InstanceImpl::getGlobalId(IString** globalId)
{
    return rootDevice->getGlobalId(globalId);
}

ErrCode InstanceImpl::getActive(Bool* active)
{
    return rootDevice->getActive(active);
}

ErrCode InstanceImpl::setActive(Bool active)
{
    return rootDevice->setActive(active);
}

ErrCode INTERFACE_FUNC InstanceImpl::getParent(IComponent** parent)
{
    OPENDAQ_PARAM_NOT_NULL(parent);

    *parent = nullptr;

    return OPENDAQ_SUCCESS;
}

ErrCode InstanceImpl::getName(IString** name)
{
    return rootDevice->getName(name);
}

ErrCode InstanceImpl::setName(IString* name)
{
    return rootDevice->setName(name);
}

ErrCode InstanceImpl::getDescription(IString** description)
{
    return rootDevice->getDescription(description);
}

ErrCode InstanceImpl::setDescription(IString* description)
{
    return rootDevice->setDescription(description);
}

ErrCode InstanceImpl::getTags(ITagsConfig** tags)
{
    return rootDevice->getTags(tags);
}

ErrCode InstanceImpl::getItems(IList** items)
{
    return rootDevice->getItems(items);
}

ErrCode InstanceImpl::getItem(IString* localId, IComponent** item)
{
    return rootDevice->getItem(localId, item);
}

ErrCode INTERFACE_FUNC InstanceImpl::isEmpty(Bool* empty)
{
    return rootDevice->isEmpty(empty);
}

ErrCode InstanceImpl::hasItem(IString* localId, Bool* value)
{
    return rootDevice->hasItem(localId, value);
}

ErrCode INTERFACE_FUNC InstanceImpl::findComponent(IComponent* component, IString* id, IComponent** outComponent)
{
    OPENDAQ_PARAM_NOT_NULL(id);
    OPENDAQ_PARAM_NOT_NULL(outComponent);

    ComponentPtr componentPtr = component;
    if (!componentPtr.assigned())
        componentPtr = rootDevice;

    return daqTry(
        [&outComponent, &componentPtr, &id]()
        {
            *outComponent = findComponentInternal(componentPtr, StringPtr(id)).detach();

            return *outComponent == nullptr ? OPENDAQ_NOTFOUND : OPENDAQ_SUCCESS;
        });
}

ErrCode InstanceImpl::getAvailableDevices(IList** availableDevices)
{
    return rootDevice->getAvailableDevices(availableDevices);
}

ErrCode InstanceImpl::getAvailableDeviceTypes(IDict** deviceTypes)
{
    return rootDevice->getAvailableDeviceTypes(deviceTypes);
}

ErrCode InstanceImpl::addDevice(IDevice** device, IString* connectionString, IPropertyObject* config)
{
    return rootDevice->addDevice(device, connectionString, config);
}

ErrCode InstanceImpl::removeDevice(IDevice* device)
{
    return rootDevice->removeDevice(device);
}

ErrCode InstanceImpl::getDevices(IList** devices)
{
    return rootDevice->getDevices(devices);
}

ErrCode InstanceImpl::getAvailableFunctionBlockTypes(IDict** functionBlockTypes)
{
    if (isDefaultRootDevice())
        return rootDevice->getAvailableFunctionBlockTypes(functionBlockTypes);

    OPENDAQ_PARAM_NOT_NULL(functionBlockTypes);

    DictPtr<IString, IFunctionBlockType> rootDeviceFbs;
    auto errCode = rootDevice->getAvailableFunctionBlockTypes(&rootDeviceFbs);
    if (OPENDAQ_FAILED(errCode))
    {
        if (errCode == OPENDAQ_ERR_NOTIMPLEMENTED)
            daqClearErrorInfo();
        else
            return errCode;
    }

    DictPtr<IString, IFunctionBlockType> daqClientFbs;
    errCode = defaultRootDevice->getAvailableFunctionBlockTypes(&daqClientFbs);
    if (OPENDAQ_FAILED(errCode))
    {
        if (errCode == OPENDAQ_ERR_NOTIMPLEMENTED)
            daqClearErrorInfo();
        else
            return errCode;
    }

    auto availableTypes = Dict<IString, IFunctionBlockType>();
    for (const auto& [id, fbType] : rootDeviceFbs)
        availableTypes.set(id, fbType);
    for (const auto& [id, fbType] : daqClientFbs)
        availableTypes.set(id, fbType);

    *functionBlockTypes = availableTypes.detach();
    return OPENDAQ_SUCCESS;
}

ErrCode InstanceImpl::addFunctionBlock(IFunctionBlock** functionBlock, IString* typeId, IPropertyObject* config)
{
    if (isDefaultRootDevice())
        return rootDevice->addFunctionBlock(functionBlock, typeId, config);

    auto errCode = rootDevice->addFunctionBlock(functionBlock, typeId, config);
    if (OPENDAQ_SUCCEEDED(errCode))
    {
        if (*functionBlock != nullptr)
            return errCode; // success
    }
    else if (errCode != OPENDAQ_ERR_NOTFOUND && errCode != OPENDAQ_ERR_NOTIMPLEMENTED)
        return errCode;

    daqClearErrorInfo();

    return defaultRootDevice->addFunctionBlock(functionBlock, typeId, config);
}

ErrCode InstanceImpl::removeFunctionBlock(IFunctionBlock* functionBlock)
{
    if (isDefaultRootDevice())
        return rootDevice->removeFunctionBlock(functionBlock);

    auto errCode = rootDevice->removeFunctionBlock(functionBlock);
    if (OPENDAQ_SUCCEEDED(errCode))
        return errCode;

    if (errCode != OPENDAQ_ERR_NOTFOUND && errCode != OPENDAQ_ERR_NOTIMPLEMENTED)
        return errCode;

    daqClearErrorInfo();

    return defaultRootDevice->removeFunctionBlock(functionBlock);
}

ErrCode InstanceImpl::getFunctionBlocks(IList** functionBlocks)
{
    if (isDefaultRootDevice())
        return rootDevice->getFunctionBlocks(functionBlocks);

    OPENDAQ_PARAM_NOT_NULL(functionBlocks);

    ListPtr<IFunctionBlock> rootDeviceFbs;
    auto errCode = rootDevice->getFunctionBlocks(&rootDeviceFbs);
    if (OPENDAQ_FAILED(errCode))
    {
        if (errCode == OPENDAQ_ERR_NOTIMPLEMENTED)
            daqClearErrorInfo();
        else
            return errCode;
    }

    ListPtr<IFunctionBlock> daqClientFbs;
    errCode = defaultRootDevice->getFunctionBlocks(&daqClientFbs);
    if (OPENDAQ_FAILED(errCode))
    {
        if (errCode == OPENDAQ_ERR_NOTIMPLEMENTED)
            daqClearErrorInfo();
        else
            return errCode;
    }

    auto list = List<IFunctionBlock>();
    for (const auto& fb : rootDeviceFbs)
        list.pushBack(fb);
    for (const auto& fb : daqClientFbs)
        list.pushBack(fb);

    *functionBlocks = list.detach();

    return OPENDAQ_SUCCESS;
}

ErrCode InstanceImpl::getChannels(IList** channels)
{
    return rootDevice->getChannels(channels);
}

ErrCode InstanceImpl::getChannelsRecursive(IList** channels)
{
    return rootDevice->getChannelsRecursive(channels);
}

ErrCode InstanceImpl::saveConfiguration(IString** configuration)
{
    OPENDAQ_PARAM_NOT_NULL(configuration);

    return daqTry(
        [this, &configuration]()
        {
            auto serializer = JsonSerializer(True);

            checkErrorInfo(this->serialize(serializer));

            auto str = serializer.getOutput();

            *configuration = str.detach();

            return OPENDAQ_SUCCESS;
        });
}

ErrCode InstanceImpl::loadConfiguration(IString* configuration)
{
    OPENDAQ_PARAM_NOT_NULL(configuration);

    return daqTry(
        [this, &configuration]()
        {
            const auto deserializer = JsonDeserializer();

            auto updatable = this->template borrowInterface<IUpdatable>();

            deserializer.update(updatable, configuration);

            return OPENDAQ_SUCCESS;
        });
}

// IPropertyObject

ErrCode InstanceImpl::getClassName(IString** className)
{
    return rootDevice->getClassName(className);
}

ErrCode InstanceImpl::setPropertyValue(IString* propertyName, IBaseObject* value)
{
    return rootDevice->setPropertyValue(propertyName, value);
}

ErrCode InstanceImpl::getPropertyValue(IString* propertyName, IBaseObject** value)
{
    return rootDevice->getPropertyValue(propertyName, value);
}

ErrCode InstanceImpl::getPropertySelectionValue(IString* propertyName, IBaseObject** value)
{
    return rootDevice->getPropertySelectionValue(propertyName, value);
}

ErrCode InstanceImpl::clearPropertyValue(IString* propertyName)
{
    return rootDevice->clearPropertyValue(propertyName);
}

ErrCode InstanceImpl::getProperty(IString* propertyName, IProperty** property)
{
    return rootDevice->getProperty(propertyName, property);
}

ErrCode InstanceImpl::addProperty(IProperty* property)
{
    return rootDevice->addProperty(property);
}

ErrCode InstanceImpl::removeProperty(IString* propertyName)
{
    return rootDevice->removeProperty(propertyName);
}

ErrCode InstanceImpl::getVisibleProperties(IList** properties)
{
    return rootDevice->getVisibleProperties(properties);
}

ErrCode InstanceImpl::getAllProperties(IList** properties)
{
    return rootDevice->getAllProperties(properties);
}

ErrCode InstanceImpl::setPropertyOrder(IList* orderedPropertyNames)
{
    return rootDevice->setPropertyOrder(orderedPropertyNames);
}

ErrCode InstanceImpl::getOnPropertyValueWrite(IString* propertyName, IEvent** event)
{
    return rootDevice->getOnPropertyValueWrite(propertyName, event);
}

ErrCode InstanceImpl::getOnPropertyValueRead(IString* propertyName, IEvent** event)
{
    return rootDevice->getOnPropertyValueRead(propertyName, event);
}

ErrCode InstanceImpl::beginUpdate()
{
    return rootDevice->beginUpdate();
}

ErrCode InstanceImpl::endUpdate()
{
    return rootDevice->endUpdate();
}

ErrCode InstanceImpl::getOnEndUpdate(IEvent** event)
{
    return rootDevice->endUpdate();
}

ErrCode InstanceImpl::hasProperty(IString* propertyName, Bool* hasProperty)
{
    return rootDevice->hasProperty(propertyName, hasProperty);
}

ErrCode InstanceImpl::serialize(ISerializer* serializer)
{
    OPENDAQ_PARAM_NOT_NULL(serializer);

    serializer->startTaggedObject(this);
    {
        serializer->key("rootDevice");
        serializer->startObject();
        {            
            serializer->key(rootDevice.getLocalId().getCharPtr());
            rootDevice.serialize(serializer);
        }
        serializer->endObject();
    }
    serializer->endObject();

    return OPENDAQ_SUCCESS;
}

ErrCode InstanceImpl::getSerializeId(ConstCharPtr* id) const
{
    OPENDAQ_PARAM_NOT_NULL(id);

    *id = SerializeId();

    return OPENDAQ_SUCCESS;
}

ConstCharPtr InstanceImpl::SerializeId()
{
    return "Instance";
}

ErrCode InstanceImpl::Deserialize(ISerializedObject* serialized, IBaseObject*, IBaseObject** obj)
{
    OPENDAQ_PARAM_NOT_NULL(serialized);
    OPENDAQ_PARAM_NOT_NULL(obj);

    return OPENDAQ_ERR_NOTIMPLEMENTED;
}

ErrCode INTERFACE_FUNC InstanceImpl::update(ISerializedObject* obj)
{
    const auto objPtr = SerializedObjectPtr::Borrow(obj);

    return daqTry([&objPtr, this]()
        {
            objPtr.checkObjectType("Instance");

            const auto rootDeviceWrapperPtr = objPtr.readSerializedObject("rootDevice");
            const auto rootDeviceWrapperKeysPtr = rootDeviceWrapperPtr.getKeys();
            if (rootDeviceWrapperKeysPtr.getCount() != 1)
                throw InvalidValueException("Invalid root device object");

            const auto rootDevicePtr = rootDeviceWrapperPtr.readSerializedObject(rootDeviceWrapperKeysPtr[0]);
            rootDevicePtr.checkObjectType("Device");
            
            auto rootDeviceUpdatable = this->rootDevice.asPtr<IUpdatable>(true);
            rootDeviceUpdatable.update(rootDevicePtr);

            connectInputPorts();

            return OPENDAQ_SUCCESS;
        });
}

void InstanceImpl::connectInputPorts()
{
    forEachComponent(rootDevice, [this](const ComponentPtr& component)
        {
            if (component.supportsInterface<IFolder>() && component.getLocalId() == "ip")
                return false;

            if (component.supportsInterface<IFunctionBlock>())
            {
                const auto fb = component.asPtr<IFunctionBlock>();
                const auto inputPorts = fb.getInputPorts();
                for (const auto& inputPort: inputPorts)
                {
                    const auto inputPortPrivate = inputPort.asPtr<IInputPortPrivate>(true);
                    auto str = inputPortPrivate.getSerializedSignalId();
                    const std::string signalId = str.assigned() ? str.toStdString() : std::string {};

                    if (!signalId.empty())
                    {
                        const SignalPtr sig = findComponentInternal(rootDevice, signalId);
                        if (sig.assigned())
                        {
                            try
                            {
                                inputPort.connect(sig);
                            }
                            catch (const DaqException&)
                            {
                                LOG_W("Failed to connect signal: {}", signalId);
                            }
                        }
                        else
                        {
                            LOG_W("Signal not found: {}", signalId);
                        }
                    }

                    inputPortPrivate.finishUpdate();
                }
            }

            return true;
        });
}

template <class F>
void InstanceImpl::forEachComponent(const ComponentPtr& component, F&& callback)
{
    const auto checkSubComponents = callback(component);

    if (checkSubComponents)
    {
        const auto folder = component.asPtrOrNull<IFolder>(true);
        if (folder.assigned())
        {
            for (const auto item : folder.getItems())
                forEachComponent(item, std::forward<F>(callback));
        }
    }
}

OPENDAQ_DEFINE_CLASS_FACTORY(
    LIBRARY_FACTORY,
    Instance,
    IContext*, context,
    IString*, localId)

END_NAMESPACE_OPENDAQ
