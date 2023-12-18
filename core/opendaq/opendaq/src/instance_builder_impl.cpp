#include <coreobjects/errors.h>
#include <opendaq/instance_builder_impl.h>
#include <opendaq/instance_builder_ptr.h>
#include <opendaq/instance_ptr.h>
#include <utility>

BEGIN_NAMESPACE_OPENDAQ

DictPtr<IString, IBaseObject> InstanceBuilderImpl::GetOptions()
{
    return Dict<IString, IBaseObject>({
        {"ModuleManager", Dict<IString, IBaseObject>({
                {"ModulesPath", ""}
            })},
        {"Scheduler", Dict<IString, IBaseObject>({
                {"WorkersNum", 0}
            })},
        {"Logging", Dict<IString, IBaseObject>({
                {"GlobalLogLevel", OPENDAQ_LOG_LEVEL_DEFAULT}
            })},
        {"Modules", Dict<IString, IBaseObject>()}
    });
}

InstanceBuilderImpl::InstanceBuilderImpl()
    : componentsLogLevel(Dict<IString, LogLevel>())
    , options(GetOptions())
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
DictPtr<IString, IBaseObject> InstanceBuilderImpl::getModuleOptions(IString* module)
{
    DictPtr<IString, IBaseObject> modules = options.get("Modules").asPtr<IDict>();
    if (!modules.hasKey(module)) 
    {   
        auto moduleOptions = Dict<IString, IBaseObject>();
        modules[module] = moduleOptions;
        return moduleOptions;
    }
    return modules[module];
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

ErrCode InstanceBuilderImpl::setLogger(ILogger* logger)
{
    this->logger = logger;    
    return OPENDAQ_SUCCESS;
}

ErrCode InstanceBuilderImpl::getLogger(ILogger** logger)
{
    if (logger == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;
    
    if (this->logger.assigned()) 
        *logger = this->logger.addRefAndReturn();
    else
        *logger = nullptr;
    return OPENDAQ_SUCCESS;
}

ErrCode InstanceBuilderImpl::setGlobalLogLevel(LogLevel logLevel)
{
    getLoggingOptions()["GlobalLogLevel"] = UInt(logLevel);
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

    sinks.insert(sink);
    return OPENDAQ_SUCCESS;
}

ErrCode InstanceBuilderImpl::setSinkLogLevel(ILoggerSink* sink, LogLevel logLevel)
{
    if (sink == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;
    
    sink->setLevel(logLevel);
    sinks.insert(sink);
    return OPENDAQ_SUCCESS;
}

ErrCode InstanceBuilderImpl::getLoggerSinks(IList** sinks)
{
    if (sinks == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    auto result = List<ILoggerSink>();
    for (auto & sink: this->sinks)
        result.pushBack(sink);
    
    *sinks = result.detach();
    return OPENDAQ_SUCCESS;
}

ErrCode InstanceBuilderImpl::setModulePath(IString* path)
{
    if (path == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;
    
    getModuleManagerOptions()["ModulesPath"] = path;
    return OPENDAQ_SUCCESS;
}

ErrCode InstanceBuilderImpl::getModulePath(IString** path)
{
    if (path == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *path = getModuleManagerOptions().get("ModulesPath").asPtr<IString>().addRefAndReturn();
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
    
    if (this->moduleManager.assigned())
        *moduleManager = this->moduleManager.addRefAndReturn();
    else
        *moduleManager = nullptr;    
    
    return OPENDAQ_SUCCESS;
}

ErrCode InstanceBuilderImpl::setSchedulerWorkerNum(SizeT numWorkers)
{
    getSchedulerOptions()["WorkersNum"] = numWorkers;
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
    
    if (this->scheduler.assigned())
        *scheduler = this->scheduler.addRefAndReturn();
    else
        *scheduler = nullptr;
    return OPENDAQ_SUCCESS;
}

ErrCode InstanceBuilderImpl::setDefaultRootDeviceName(IString* localId)
{
    if (localId == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    this->localId = localId;
    return OPENDAQ_SUCCESS;
}

ErrCode InstanceBuilderImpl::getDefaultRootDeviceName(IString** localId)
{
    if (localId == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;
    
    if(this->localId.assigned())
        *localId = this->localId.addRefAndReturn();
    else
        *localId = nullptr;
    return OPENDAQ_SUCCESS;
}

ErrCode InstanceBuilderImpl::setRootDevice(IString* connectionString)
{
    if (connectionString == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    this->connectionString = connectionString;
    return OPENDAQ_SUCCESS;
}

ErrCode InstanceBuilderImpl::getRootDevice(IString** connectionString)
{
    if (connectionString == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;
    
    if (this->connectionString.assigned())
        *connectionString = this->connectionString.addRefAndReturn();
    else
        *connectionString = nullptr;
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
    
    if (!this->defaultRootDeviceInfo.assigned())
        *deviceInfo = nullptr; 
    else
        *deviceInfo = this->defaultRootDeviceInfo.addRefAndReturn();

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
