#include <opendaq/context_ptr.h>
#include <opendaq/context_internal_ptr.h>
#include <opendaq/device_info_factory.h>
#include <opendaq/ids_parser.h>
#include <opendaq/instance_factory.h>
#include <opendaq/instance_impl.h>
#include <opendaq/input_port_private_ptr.h>
#include <coretypes/validation.h>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <opendaq/custom_log.h>
#include <opendaq/device_private.h>

#include <opendaq/module_manager_utils_ptr.h>
#include <opendaq/discovery_server_factory.h>

BEGIN_NAMESPACE_OPENDAQ

static StringPtr DefineLocalId(const StringPtr& localId);
static ContextPtr ContextFromInstanceBuilder(IInstanceBuilder* instanceBuilder);
static std::string GetErrorMessage();

InstanceImpl::InstanceImpl(ContextPtr context, const StringPtr& localId)
    : context(std::move(context))
    , moduleManager(this->context.assigned() ? this->context.asPtr<IContextInternal>().moveModuleManager() : nullptr)
    , rootDeviceSet(false)
{
    auto instanceId = DefineLocalId(localId);
    rootDevice = Client(this->context, instanceId);
    rootDevice.asPtr<IPropertyObjectInternal>().enableCoreEventTrigger();
    loggerComponent = this->context.getLogger().addComponent("Instance");
}

InstanceImpl::InstanceImpl(IInstanceBuilder* instanceBuilder)
    : context(ContextFromInstanceBuilder(instanceBuilder))
    , moduleManager(this->context.assigned() ? this->context.asPtr<IContextInternal>().moveModuleManager() : nullptr)
    , rootDeviceSet(false)
{
    const auto builderPtr = InstanceBuilderPtr::Borrow(instanceBuilder);
    loggerComponent = this->context.getLogger().getOrAddComponent("Instance");

    auto connectionString = builderPtr.getRootDevice();
    auto rootDeviceConfig = builderPtr.getRootDeviceConfig();

    if (connectionString.assigned() && connectionString.getLength())
    {
        rootDevice = moduleManager.asPtr<IModuleManagerUtils>().createDevice(connectionString, nullptr, rootDeviceConfig);
        const auto devicePrivate = rootDevice.asPtrOrNull<IDevicePrivate>();
        if (devicePrivate.assigned())
            devicePrivate->setAsRoot();
        LOG_I("Root device set to {}", connectionString)
        rootDeviceSet = true;
    }
    else
    {
        auto localId = builderPtr.getDefaultRootDeviceLocalId();
        auto instanceId = DefineLocalId(localId);
        rootDevice = Client(this->context, instanceId, builderPtr.getDefaultRootDeviceInfo());
    }
    rootDevice.asPtr<IPropertyObjectInternal>().enableCoreEventTrigger();
}

InstanceImpl::~InstanceImpl()
{
    stopAndRemoveServers();
    rootDevice.release();
}

static StringPtr DefineLocalId(const StringPtr& localId)
{
    if (localId.assigned() && localId.getLength())
        return localId;
    return "ClientDevice";
}

static DiscoveryServerPtr createDiscoveryServer(const StringPtr& serviceName, const LoggerPtr& logger)
{
    if (serviceName == "mdns")
        return MdnsDiscoveryServer(logger);
    return nullptr;
}

static ContextPtr ContextFromInstanceBuilder(IInstanceBuilder* instanceBuilder)
{
    const auto builderPtr = InstanceBuilderPtr::Borrow(instanceBuilder);
    const auto context = builderPtr.getContext();

    if (context.assigned())
        return context;

    auto logger = builderPtr.getLogger();
    auto scheduler = builderPtr.getScheduler();
    auto moduleManager = builderPtr.getModuleManager();
    auto typeManager = TypeManager();
    auto authenticationProvider = builderPtr.getAuthenticationProvider();
    auto options = builderPtr.getOptions();

    // Configure logger
    if (!logger.assigned())
    {
        auto sinks = builderPtr.getLoggerSinks();
        if (sinks.empty())
            logger = Logger();
        else
            logger = LoggerWithSinks(sinks);
    }
    logger.setLevel(builderPtr.getGlobalLogLevel());

    for (const auto& [component, logLevel] : builderPtr.getComponentsLogLevel())
    {
        auto createdComponent = logger.getOrAddComponent(component);
        createdComponent.setLevel(LogLevel(logLevel));
    }

    // Configure scheduler
    if (!scheduler.assigned())
        scheduler = Scheduler(logger, builderPtr.getSchedulerWorkerNum());

    // Configure moduleManager
    if (!moduleManager.assigned())
        moduleManager = ModuleManagerMultiplePaths(builderPtr.getModulePathsList());

    auto discoveryServers = Dict<IString, IDiscoveryServer>();
    for (const auto& serviceName : builderPtr.getDiscoveryServers())
    {
        auto service = createDiscoveryServer(serviceName, logger);
        if (service.assigned())
            discoveryServers.set(serviceName, service);
    }

    return Context(scheduler, logger, typeManager, moduleManager, authenticationProvider, options, discoveryServers);
}

void InstanceImpl::stopAndRemoveServers()
{
    for (const auto& server : rootDevice.getServers())
    {
        server.stop();
        // Manually remove servers from the root device to clean-up circular references to root device held by servers
        rootDevice.removeServer(server);
    }
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

ErrCode InstanceImpl::addServer(IString* typeId, IPropertyObject* config, IServer** server)
{
    return rootDevice->addServer(typeId, config, server);
}

std::string GetErrorMessage()
{
    std::string errorMessage;

    ErrorInfoPtr errorInfo;
    daqGetErrorInfo(&errorInfo);
    if (errorInfo.assigned())
    {
        StringPtr message;
        errorInfo->getMessage(&message);

        if (message.assigned())
        {
            errorMessage = message.toStdString();
        }
    }

    return errorMessage;
}

ErrCode InstanceImpl::addStandardServers(IList** standardServers)
{
    OPENDAQ_PARAM_NOT_NULL(standardServers);

    auto serversPtr = List<IServer>();
    ErrCode errCode = OPENDAQ_SUCCESS;

    StringPtr serverName;
    
#if defined(OPENDAQ_ENABLE_NATIVE_STREAMING)
    ServerPtr nativeStreamingServer;
    serverName = "OpenDAQNativeStreaming";
    errCode = addServer(serverName, nullptr, &nativeStreamingServer);
    if (OPENDAQ_FAILED(errCode))
    {
        LOG_E(R"(AddStandardServers called but could not add "{}" module: {} [{:#x}])", serverName, GetErrorMessage(), errCode);
        return errCode;
    }
    serversPtr.pushBack(nativeStreamingServer);

#elif defined(OPENDAQ_ENABLE_WEBSOCKET_STREAMING)

    ServerPtr websocketServer;
    serverName = "OpenDAQLTStreaming";
    errCode = addServer(serverName, nullptr, &websocketServer);
    if (OPENDAQ_FAILED(errCode))
    {
        LOG_E(R"(AddStandardServers called but could not add "{}" module: {} [{:#x}])", serverName, GetErrorMessage(), errCode);
        return errCode;
    }
    serversPtr.pushBack(websocketServer);
#endif

    ServerPtr opcUaServer;
    serverName = "OpenDAQOPCUA";
    errCode = addServer(serverName, nullptr, &opcUaServer);
    if (OPENDAQ_FAILED(errCode))
    {
        LOG_E(R"(AddStandardServers called but could not add "{}" module: {} [{:#x}])", serverName, GetErrorMessage(), errCode);
        return errCode;
    }
    serversPtr.pushBack(opcUaServer);

    *standardServers = serversPtr.detach();

    return OPENDAQ_SUCCESS;
}

ErrCode InstanceImpl::removeServer(IServer* server)
{
    return rootDevice->removeServer(server);
}

ErrCode InstanceImpl::getServers(IList** instanceServers)
{
    return rootDevice->getServers(instanceServers);
}

ErrCode INTERFACE_FUNC InstanceImpl::lock()
{
    return rootDevice->lock();
}

ErrCode INTERFACE_FUNC InstanceImpl::unlock()
{
    return rootDevice->unlock();
}

ErrCode INTERFACE_FUNC InstanceImpl::isLocked(Bool* locked)
{
    return rootDevice->isLocked(locked);
}

ErrCode INTERFACE_FUNC InstanceImpl::getLogFileInfos(IList** logFileInfos)
{
    return rootDevice->getLogFileInfos(logFileInfos);
}

ErrCode INTERFACE_FUNC InstanceImpl::getLog(IString** log, IString* id, Int size, Int offset)
{
    return rootDevice->getLog(log, id, size, offset);
}

ErrCode InstanceImpl::getRootDevice(IDevice** currentRootDevice)
{
    OPENDAQ_PARAM_NOT_NULL(currentRootDevice);

    *currentRootDevice = this->rootDevice.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode InstanceImpl::setRootDevice(IString* connectionString, IPropertyObject* config)
{
    OPENDAQ_PARAM_NOT_NULL(connectionString);

    const auto connectionStringPtr = StringPtr::Borrow(connectionString);

    if (rootDeviceSet)
        return makeErrorInfo(OPENDAQ_ERR_INVALIDSTATE, "Root device already set.");

    if (rootDevice.getFunctionBlocks().getCount() > 0)
        return makeErrorInfo(OPENDAQ_ERR_INVALIDSTATE, "Cannot set root device if function blocks already added");

    if (rootDevice.getDevices().getCount() > 0)
        return makeErrorInfo(OPENDAQ_ERR_INVALIDSTATE, "Cannot set root device if devices are already added");

    if (rootDevice.getServers().getCount() > 0)
        return makeErrorInfo(OPENDAQ_ERR_INVALIDSTATE, "Cannot set root device if servers are already added");

    const auto newRootDevice = moduleManager.asPtr<IModuleManagerUtils>().createDevice(connectionString, nullptr, config);

    this->rootDevice = newRootDevice;
    rootDeviceSet = true;

    const auto devicePrivate = rootDevice.asPtrOrNull<IDevicePrivate>();
    if (devicePrivate.assigned())
        devicePrivate->setAsRoot();

    LOG_I("Root device explicitly set to {}", connectionStringPtr);

    this->rootDevice.asPtr<IPropertyObjectInternal>().enableCoreEventTrigger();
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

ErrCode InstanceImpl::getSignals(IList** signals, ISearchFilter* searchFilter)
{
    return rootDevice->getSignals(signals, searchFilter);
}

ErrCode InstanceImpl::getSignalsRecursive(IList** signals, ISearchFilter* searchFilter)
{
    return rootDevice->getSignalsRecursive(signals, searchFilter);
}

ErrCode InstanceImpl::getTicksSinceOrigin(uint64_t* ticks)
{
    return rootDevice->getTicksSinceOrigin(ticks);
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

ErrCode InstanceImpl::getTags(ITags** tags)
{
    return rootDevice->getTags(tags);
}

ErrCode InstanceImpl::getVisible(Bool* visible)
{
    return rootDevice->getVisible(visible);
}

ErrCode InstanceImpl::setVisible(Bool visible)
{
    return rootDevice->setVisible(visible);
}

ErrCode InstanceImpl::getLockedAttributes(IList** attributes)
{
    return rootDevice->getLockedAttributes(attributes);
}

ErrCode InstanceImpl::getOnComponentCoreEvent(IEvent** event)
{
    return rootDevice->getOnComponentCoreEvent(event);
}

ErrCode InstanceImpl::getStatusContainer(IComponentStatusContainer** statusContainer)
{
    return rootDevice->getStatusContainer(statusContainer);
}

ErrCode InstanceImpl::getItems(IList** items, ISearchFilter* searchFilter)
{
    return rootDevice->getItems(items, searchFilter);
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

ErrCode INTERFACE_FUNC InstanceImpl::findComponent(IString* id, IComponent** outComponent)
{
    return rootDevice->findComponent(id, outComponent);
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

ErrCode InstanceImpl::getDevices(IList** devices, ISearchFilter* searchFilter)
{
    return rootDevice->getDevices(devices, searchFilter);
}

ErrCode InstanceImpl::addStreaming(IStreaming** streaming, IString* connectionString, IPropertyObject* config)
{
    return rootDevice->addStreaming(streaming, connectionString, config);
}

ErrCode InstanceImpl::getSyncComponent(ISyncComponent** syncComponent)
{
    return rootDevice->getSyncComponent(syncComponent);
}

ErrCode InstanceImpl::createDefaultAddDeviceConfig(IPropertyObject** defaultConfig)
{
    return rootDevice->createDefaultAddDeviceConfig(defaultConfig);
}

ErrCode InstanceImpl::getAvailableFunctionBlockTypes(IDict** functionBlockTypes)
{
    return rootDevice->getAvailableFunctionBlockTypes(functionBlockTypes);
}

ErrCode InstanceImpl::addFunctionBlock(IFunctionBlock** functionBlock, IString* typeId, IPropertyObject* config)
{
    return rootDevice->addFunctionBlock(functionBlock, typeId, config);
}

ErrCode InstanceImpl::removeFunctionBlock(IFunctionBlock* functionBlock)
{
    return rootDevice->removeFunctionBlock(functionBlock);
}

ErrCode InstanceImpl::getFunctionBlocks(IList** functionBlocks, ISearchFilter* searchFilter)
{
    return rootDevice->getFunctionBlocks(functionBlocks, searchFilter);
}

ErrCode InstanceImpl::getChannels(IList** channels, ISearchFilter* searchFilter)
{
    return rootDevice->getChannels(channels, searchFilter);
}

ErrCode InstanceImpl::getChannelsRecursive(IList** channels, ISearchFilter* searchFilter)
{
    return rootDevice->getChannelsRecursive(channels, searchFilter);
}

ErrCode InstanceImpl::saveConfiguration(IString** configuration)
{
    OPENDAQ_PARAM_NOT_NULL(configuration);

    return daqTry(
        [this, &configuration]()
        {
            auto serializer = JsonSerializer(True);

            checkErrorInfo(this->serializeForUpdate(serializer));

            auto str = serializer.getOutput();

            *configuration = str.detach();

            return OPENDAQ_SUCCESS;
        });
}

ErrCode InstanceImpl::loadConfiguration(IString* configuration, IUpdateParameters* config)
{
    OPENDAQ_PARAM_NOT_NULL(configuration);
    auto configPtr = BaseObjectPtr(config);

    return daqTry([this, &configuration, &configPtr]
    {
        const auto deserializer = JsonDeserializer();

        auto updatable = this->template borrowInterface<IUpdatable>();

        deserializer.update(updatable, configuration, configPtr);

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

ErrCode InstanceImpl::getUpdating(Bool* updating)
{
    return rootDevice->getUpdating(updating);
}

ErrCode InstanceImpl::getOnEndUpdate(IEvent** event)
{
    return rootDevice->endUpdate();
}

ErrCode INTERFACE_FUNC InstanceImpl::getPermissionManager(IPermissionManager** permissionManager)
{
    OPENDAQ_PARAM_NOT_NULL(permissionManager);

    *permissionManager = rootDevice.getPermissionManager().addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode InstanceImpl::hasProperty(IString* propertyName, Bool* hasProperty)
{
    return rootDevice->hasProperty(propertyName, hasProperty);
}

ErrCode InstanceImpl::serialize(ISerializer* serializer)
{
    return daqTry([this, &serializer] {
            rootDevice.asPtr<ISerializable>(true).serialize(serializer);
        });
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

ErrCode InstanceImpl::updateInternal(ISerializedObject* obj, IBaseObject* context)
{
    return this->makeErrorInfo(OPENDAQ_ERR_INVALID_OPERATION, "UpdateInternal is not permitted for Instance. Use update instead.");
}

ErrCode InstanceImpl::update(ISerializedObject* obj, IBaseObject* config)
{
    const auto objPtr = SerializedObjectPtr::Borrow(obj);

    return daqTry([&objPtr, &config, this]
    {
        objPtr.checkObjectType("Instance");

        const auto rootDeviceWrapperPtr = objPtr.readSerializedObject("rootDevice");
        const auto rootDeviceWrapperKeysPtr = rootDeviceWrapperPtr.getKeys();
        if (rootDeviceWrapperKeysPtr.getCount() != 1)
            throw InvalidValueException("Invalid root device object");

        const auto rootDevicePtr = rootDeviceWrapperPtr.readSerializedObject(rootDeviceWrapperKeysPtr[0]);
        rootDevicePtr.checkObjectType("Device");

        auto rootDeviceUpdatable = this->rootDevice.asPtr<IUpdatable>(true);
        rootDeviceUpdatable.update(rootDevicePtr, config);

        return OPENDAQ_SUCCESS;
    });
}

ErrCode InstanceImpl::updateEnded(IBaseObject* /* context */)
{
    return OPENDAQ_SUCCESS;
}

ErrCode InstanceImpl::serializeForUpdate(ISerializer* serializer)
{
    OPENDAQ_PARAM_NOT_NULL(serializer);

    serializer->startTaggedObject(this);
    {
        serializer->key("rootDevice");
        serializer->startObject();
        {
            serializer->key(rootDevice.getLocalId().getCharPtr());
            const auto updatableRootDevice = rootDevice.asPtr<IUpdatable>(true);
            updatableRootDevice.serializeForUpdate(serializer);
        }
        serializer->endObject();
    }
    serializer->endObject();

    return OPENDAQ_SUCCESS;
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

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, Instance,
    IInstance, createInstanceFromBuilder,
    IInstanceBuilder*, builder
)

END_NAMESPACE_OPENDAQ
