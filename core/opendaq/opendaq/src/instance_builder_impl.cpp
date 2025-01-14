#include <coreobjects/errors.h>
#include <opendaq/instance_builder_impl.h>
#include <opendaq/instance_builder_ptr.h>
#include <opendaq/instance_ptr.h>
#include <opendaq/custom_log.h>
#include <opendaq/config_provider_factory.h>
#include <coreobjects/authentication_provider_factory.h>

BEGIN_NAMESPACE_OPENDAQ

DictPtr<IString, IBaseObject> InstanceBuilderImpl::GetDefaultOptions()
{
    return Dict<IString, IBaseObject>({
        {"ModuleManager", Dict<IString, IBaseObject>({
                {"ModulesPaths", List<IString>(""),
                }
            })},
        {"Scheduler", Dict<IString, IBaseObject>({
                {"WorkersNum", 0}
            })},
        {"Logging", Dict<IString, IBaseObject>({
                {"GlobalLogLevel", OPENDAQ_LOG_LEVEL_DEFAULT}
            })},
        {"RootDevice", Dict<IString, IBaseObject>({
                {"DefaultLocalId", ""},
                {"ConnectionString", ""}
            })},   
        {"Modules", Dict<IString, IBaseObject>()}
    });
}

InstanceBuilderImpl::InstanceBuilderImpl()
    : componentsLogLevel(Dict<IString, LogLevel>())
    , sinks(List<ILoggerSink>())
    , authenticationProvider(AuthenticationProvider())
    , providers(List<IConfigProvider>())
    , options(GetDefaultOptions())
    , discoveryServers(List<IString>())
{
}

DictPtr<IString, IBaseObject> InstanceBuilderImpl::getModuleManagerOptions()
{
    return options.get("ModuleManager");
}

DictPtr<IString, IBaseObject> InstanceBuilderImpl::getSchedulerOptions()
{
    return options.get("Scheduler");
}

DictPtr<IString, IBaseObject> InstanceBuilderImpl::getLoggingOptions()
{
    return options.get("Logging");
}

DictPtr<IString, IBaseObject> InstanceBuilderImpl::getRootDevice()
{
    return options.get("RootDevice");
}

DictPtr<IString, IBaseObject> InstanceBuilderImpl::getModules()
{
    return options.get("Modules");
}

ErrCode InstanceBuilderImpl::build(IInstance** instance)
{
    if (instance == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    const auto builderPtr = this->borrowPtr<InstanceBuilderPtr>();
    return daqTry([&]()
    {
        *instance = InstanceFromBuilder(builderPtr).detach();
        return OPENDAQ_SUCCESS;
    });
}

ErrCode InstanceBuilderImpl::addConfigProvider(IConfigProvider* configProvider)
{
    if (configProvider == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;
    
    providers.pushBack(configProvider);
    return OPENDAQ_SUCCESS;

    auto configProviderPtr = ConfigProviderPtr::Borrow(configProvider);
    
    return OPENDAQ_IGNORED;
}

ErrCode INTERFACE_FUNC InstanceBuilderImpl::setContext(IContext* context)
{
    this->context = context;
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC InstanceBuilderImpl::getContext(IContext** context)
{
    if (context == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *context = this->context.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode InstanceBuilderImpl::setLogger(ILogger* logger)
{
    this->logger = logger;
    return OPENDAQ_SUCCESS;
}

ErrCode InstanceBuilderImpl::getLogger(ILogger** logger)
{
    if (logger == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;
    
    *logger = this->logger.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode InstanceBuilderImpl::setGlobalLogLevel(LogLevel logLevel)
{
    getLoggingOptions().set("GlobalLogLevel", UInt(logLevel));
    return OPENDAQ_SUCCESS;
}

ErrCode InstanceBuilderImpl::getGlobalLogLevel(LogLevel* logLevel)
{
    if (logLevel == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *logLevel = LogLevel(getLoggingOptions()["GlobalLogLevel"]);
    return OPENDAQ_SUCCESS;
}

ErrCode InstanceBuilderImpl::setComponentLogLevel(IString* component, LogLevel logLevel)
{
    if (component == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    componentsLogLevel.set(component, UInt(logLevel));
    return OPENDAQ_SUCCESS;
}

ErrCode InstanceBuilderImpl::getComponentsLogLevel(IDict** components)
{
    if (components == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *components = componentsLogLevel.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode InstanceBuilderImpl::addLoggerSink(ILoggerSink* sink)
{
    if (sink == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;
    
    for (const auto & s: sinks)
    {
        if (s == sink)
            return OPENDAQ_IGNORED;
    }

    sinks.pushBack(sink);
    return OPENDAQ_SUCCESS;
}

ErrCode InstanceBuilderImpl::setSinkLogLevel(ILoggerSink* sink, LogLevel logLevel)
{
    if (sink == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;
    
    sink->setLevel(logLevel);

    for (const auto & s: sinks)
    {
        if (s == sink)
            return OPENDAQ_SUCCESS;
    }
    sinks.pushBack(sink);

    return OPENDAQ_SUCCESS;
}

ErrCode InstanceBuilderImpl::getLoggerSinks(IList** sinks)
{
    if (sinks == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *sinks = this->sinks.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode InstanceBuilderImpl::setModulePath(IString* path)
{
    if (path == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    getModuleManagerOptions().set("ModulesPaths", List<IString>(path));
    return OPENDAQ_SUCCESS;
}

ErrCode InstanceBuilderImpl::getModulePath(IString** path)
{
    if (path == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    auto paths = getModuleManagerOptions().get("ModulesPaths").asPtr<IList>();
    if (paths.empty()) 
    {
        *path = String("").detach();
    } 
    else 
    {
        *path = paths[0].asPtr<IString>().addRefAndReturn();
    }

    return OPENDAQ_SUCCESS;
}

ErrCode InstanceBuilderImpl::addModulePath(IString* path)
{
    if (path == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    getModuleManagerOptions().get("ModulesPaths").asPtr<IList>().pushBack(path);
    return OPENDAQ_SUCCESS;
}

ErrCode InstanceBuilderImpl::getModulePathsList(IList** paths)
{
    if (paths == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *paths = getModuleManagerOptions().get("ModulesPaths").asPtr<IList>().addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode InstanceBuilderImpl::setModuleManager(IModuleManager* moduleManager)
{
    if (moduleManager == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;
    
    this->moduleManager = moduleManager;
    return OPENDAQ_SUCCESS;
}

ErrCode InstanceBuilderImpl::getModuleManager(IModuleManager** moduleManager)
{
    if (moduleManager == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;
    
    *moduleManager = this->moduleManager.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC InstanceBuilderImpl::setAuthenticationProvider(IAuthenticationProvider* authenticationProvider)
{
    if (authenticationProvider == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    this->authenticationProvider = authenticationProvider;
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC InstanceBuilderImpl::getAuthenticationProvider(IAuthenticationProvider** authenticationProvider)
{
    if (authenticationProvider == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *authenticationProvider = this->authenticationProvider.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode InstanceBuilderImpl::setSchedulerWorkerNum(SizeT numWorkers)
{
    getSchedulerOptions().set("WorkersNum", numWorkers);
    return OPENDAQ_SUCCESS;
}

ErrCode InstanceBuilderImpl::getSchedulerWorkerNum(SizeT* numWorkers)
{
    if (numWorkers == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *numWorkers = getSchedulerOptions()["WorkersNum"];
    return OPENDAQ_SUCCESS;
}

ErrCode InstanceBuilderImpl::setScheduler(IScheduler* scheduler)
{
    if (scheduler == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;
    
    this->scheduler = scheduler;
    return OPENDAQ_SUCCESS;
}

ErrCode InstanceBuilderImpl::getScheduler(IScheduler** scheduler)
{
    if (scheduler == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;
    
    *scheduler = this->scheduler.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode InstanceBuilderImpl::setDefaultRootDeviceLocalId(IString* localId)
{
    if (localId == nullptr)
        getRootDevice().set("DefaultLocalId", "");
    else
        getRootDevice().set("DefaultLocalId", localId);
    return OPENDAQ_SUCCESS;
}

ErrCode InstanceBuilderImpl::getDefaultRootDeviceLocalId(IString** localId)
{
    if (localId == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;
    
    *localId = getRootDevice().get("DefaultLocalId").asPtr<IString>().addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode InstanceBuilderImpl::setRootDevice(IString* connectionString, IPropertyObject* config)
{
    if (connectionString == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    getRootDevice().set("ConnectionString", connectionString);
    this->rootDeviceConfig = config;
    return OPENDAQ_SUCCESS;
}

ErrCode InstanceBuilderImpl::getRootDevice(IString** connectionString)
{
    if (connectionString == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;
    
    *connectionString = getRootDevice().get("ConnectionString").asPtr<IString>().addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC InstanceBuilderImpl::getRootDeviceConfig(IPropertyObject** config)
{
    if (config == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *config = this->rootDeviceConfig.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode InstanceBuilderImpl::setDefaultRootDeviceInfo(IDeviceInfo* deviceInfo)
{
    if (deviceInfo == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    this->defaultRootDeviceInfo = deviceInfo;
    return OPENDAQ_SUCCESS;
}

ErrCode InstanceBuilderImpl::getDefaultRootDeviceInfo(IDeviceInfo** deviceInfo)
{
    if (deviceInfo == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;
    
    *deviceInfo = this->defaultRootDeviceInfo.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode InstanceBuilderImpl::getOptions(IDict** options)
{
    if (options == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    auto logger = this->logger;
    if (!logger.assigned())
        logger = Logger();
    auto loggerComponent = logger.getOrAddComponent("InstanceBuilder");

    if (useStandardProviders)
    {
        try
        {
            auto provider = JsonConfigProvider();
            provider.populateOptions(this->options);
        }
        catch (const DaqException& e)
        {
            LOG_I("Failed to populate instance builder options with given provider. Error message: {}", e.what());
        }
        catch (...)
        {
        }
    }

    for (const auto& provider : providers)
    {
        try
        {
            provider.populateOptions(this->options);
        }
        catch (const DaqException& e)
        {
            LOG_I("Failed to populate instance builder options with given provider. Error message: {}", e.what());
        }
        catch (...)
        {
        }
    }
    
    *options = this->options.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode InstanceBuilderImpl::enableStandardProviders(Bool flag)
{
    useStandardProviders = flag;
    return OPENDAQ_SUCCESS;
}

ErrCode InstanceBuilderImpl::getDiscoveryServers(IList** serverNames)
{
    if (serverNames == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *serverNames = discoveryServers.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode InstanceBuilderImpl::addDiscoveryServer(IString* serverName)
{
    if (serverName == nullptr)
        return OPENDAQ_IGNORED;

    discoveryServers.pushBack(serverName);
    return OPENDAQ_SUCCESS;
}

/////////////////////
////
//// FACTORIES
////
////////////////////
extern "C" ErrCode PUBLIC_EXPORT createInstanceBuilder(IInstanceBuilder** objTmp)
{
    return daq::createObject<IInstanceBuilder, InstanceBuilderImpl>(objTmp);
}

END_NAMESPACE_OPENDAQ
