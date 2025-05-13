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
    return Dict<IString, IBaseObject>({{"ModuleManager", Dict<IString, IBaseObject>({
                                            {"ModulesPaths", List<IString>("")}, {"AddDeviceRescanTimer", 5000}
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
    OPENDAQ_PARAM_NOT_NULL(instance);

    const auto builderPtr = this->borrowPtr<InstanceBuilderPtr>();
    return daqTry([&]()
    {
        *instance = InstanceFromBuilder(builderPtr).detach();
        return OPENDAQ_SUCCESS;
    });
}

ErrCode InstanceBuilderImpl::addConfigProvider(IConfigProvider* configProvider)
{
    OPENDAQ_PARAM_NOT_NULL(configProvider);
    
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
    OPENDAQ_PARAM_NOT_NULL(context);

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
    OPENDAQ_PARAM_NOT_NULL(logger);
    
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
    OPENDAQ_PARAM_NOT_NULL(logLevel);

    *logLevel = LogLevel(getLoggingOptions()["GlobalLogLevel"]);
    return OPENDAQ_SUCCESS;
}

ErrCode InstanceBuilderImpl::setComponentLogLevel(IString* component, LogLevel logLevel)
{
    OPENDAQ_PARAM_NOT_NULL(component);

    componentsLogLevel.set(component, UInt(logLevel));
    return OPENDAQ_SUCCESS;
}

ErrCode InstanceBuilderImpl::getComponentsLogLevel(IDict** components)
{
    OPENDAQ_PARAM_NOT_NULL(components);

    *components = componentsLogLevel.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode InstanceBuilderImpl::addLoggerSink(ILoggerSink* sink)
{
    OPENDAQ_PARAM_NOT_NULL(sink);
    
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
    OPENDAQ_PARAM_NOT_NULL(sink);
    
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
    OPENDAQ_PARAM_NOT_NULL(sinks);

    *sinks = this->sinks.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode InstanceBuilderImpl::setModulePath(IString* path)
{
    OPENDAQ_PARAM_NOT_NULL(path);

    getModuleManagerOptions().set("ModulesPaths", List<IString>(path));
    return OPENDAQ_SUCCESS;
}

ErrCode InstanceBuilderImpl::getModulePath(IString** path)
{
    OPENDAQ_PARAM_NOT_NULL(path);

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
    OPENDAQ_PARAM_NOT_NULL(path);

    getModuleManagerOptions().get("ModulesPaths").asPtr<IList>().pushBack(path);
    return OPENDAQ_SUCCESS;
}

ErrCode InstanceBuilderImpl::getModulePathsList(IList** paths)
{
    OPENDAQ_PARAM_NOT_NULL(paths);

    *paths = getModuleManagerOptions().get("ModulesPaths").asPtr<IList>().addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode InstanceBuilderImpl::setModuleManager(IModuleManager* moduleManager)
{
    OPENDAQ_PARAM_NOT_NULL(moduleManager);
    
    this->moduleManager = moduleManager;
    return OPENDAQ_SUCCESS;
}

ErrCode InstanceBuilderImpl::getModuleManager(IModuleManager** moduleManager)
{
    OPENDAQ_PARAM_NOT_NULL(moduleManager);
    
    *moduleManager = this->moduleManager.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC InstanceBuilderImpl::setAuthenticationProvider(IAuthenticationProvider* authenticationProvider)
{
    OPENDAQ_PARAM_NOT_NULL(authenticationProvider);

    this->authenticationProvider = authenticationProvider;
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC InstanceBuilderImpl::getAuthenticationProvider(IAuthenticationProvider** authenticationProvider)
{
    OPENDAQ_PARAM_NOT_NULL(authenticationProvider);

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
    OPENDAQ_PARAM_NOT_NULL(numWorkers);

    *numWorkers = getSchedulerOptions()["WorkersNum"];
    return OPENDAQ_SUCCESS;
}

ErrCode InstanceBuilderImpl::setScheduler(IScheduler* scheduler)
{
    OPENDAQ_PARAM_NOT_NULL(scheduler);
    
    this->scheduler = scheduler;
    return OPENDAQ_SUCCESS;
}

ErrCode InstanceBuilderImpl::getScheduler(IScheduler** scheduler)
{
    OPENDAQ_PARAM_NOT_NULL(scheduler);
    
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
    OPENDAQ_PARAM_NOT_NULL(localId);
    
    *localId = getRootDevice().get("DefaultLocalId").asPtr<IString>().addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode InstanceBuilderImpl::setRootDevice(IString* connectionString, IPropertyObject* config)
{
    OPENDAQ_PARAM_NOT_NULL(connectionString);

    getRootDevice().set("ConnectionString", connectionString);
    this->rootDeviceConfig = config;
    return OPENDAQ_SUCCESS;
}

ErrCode InstanceBuilderImpl::getRootDevice(IString** connectionString)
{
    OPENDAQ_PARAM_NOT_NULL(connectionString);
    
    *connectionString = getRootDevice().get("ConnectionString").asPtr<IString>().addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC InstanceBuilderImpl::getRootDeviceConfig(IPropertyObject** config)
{
    OPENDAQ_PARAM_NOT_NULL(config);

    *config = this->rootDeviceConfig.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode InstanceBuilderImpl::setDefaultRootDeviceInfo(IDeviceInfo* deviceInfo)
{
    OPENDAQ_PARAM_NOT_NULL(deviceInfo);

    this->defaultRootDeviceInfo = deviceInfo;
    return OPENDAQ_SUCCESS;
}

ErrCode InstanceBuilderImpl::getDefaultRootDeviceInfo(IDeviceInfo** deviceInfo)
{
    OPENDAQ_PARAM_NOT_NULL(deviceInfo);
    
    *deviceInfo = this->defaultRootDeviceInfo.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode InstanceBuilderImpl::getOptions(IDict** options)
{
    OPENDAQ_PARAM_NOT_NULL(options);

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
            LOG_I("Failed to populate instance builder options with given provider. Error message: {}", e.getErrorMessage());
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
            LOG_I("Failed to populate instance builder options with given provider. Error message: {}", e.getErrorMessage());
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
    OPENDAQ_PARAM_NOT_NULL(serverNames);

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
