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
                {"ForceSingleThread", 0}
            })},
        {"Logging", Dict<IString, IBaseObject>()},
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

    // Configire logger
    if (!this->logger.assigned()) 
    {
        if (sinks.empty())
            this->logger = Logger();
        else
        {
            auto sinksList = List<ILoggerSink>();
            for (const auto& sink : sinks)
                sinksList.pushBack(sink);
            
            this->logger = LoggerWithSinks(sinksList);
        }
    }
    
    if (getLoggingOptions().hasKey("GlobalLogLevel"))
        this->logger.setLevel(LogLevel(getLoggingOptions()["GlobalLogLevel"]));

    for (const auto& [component, logLevel] : componentsLogLevel)
    {
        auto createdComponent = this->logger.getOrAddComponent(component);
        createdComponent.setLevel(logLevel);
    }

    // Configure scheduler
    if (!this->scheduler.assigned())
        this->scheduler = Scheduler(this->logger, getSchedulerOptions()["ForceSingleThread"]);

    // Configure moduleManager
    if (!this->moduleManager.assigned())
        this->moduleManager = ModuleManager(getModuleManagerOptions()["ModulesPath"]);

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

ErrCode InstanceBuilderImpl::setSinkLogLevel(ILoggerSink* sink, LogLevel logLevel)
{
    if (sink == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;
    
    sink->setLevel(logLevel);
    sinks.insert(sink);
    return OPENDAQ_SUCCESS;
}

ErrCode InstanceBuilderImpl::setModulePath(IString* path)
{
    if (path == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;
    
    getModuleManagerOptions()["ModulesPath"] = path;
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
    getSchedulerOptions()["ForceSingleThread"] = numWorkers;
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

ErrCode InstanceBuilderImpl::setOption(IString* option, IBaseObject* value)
{
    if (option == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;
    if (value == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;
    
    auto optionPtr = StringPtr::Borrow(option);
    if (optionPtr == "ModuleManager")
        return OPENDAQ_ERR_INVALIDPROPERTY;
    if (optionPtr == "Scheduler")
        return OPENDAQ_ERR_INVALIDPROPERTY;
    if (optionPtr == "Logging")
        return OPENDAQ_ERR_INVALIDPROPERTY;
    if (optionPtr == "Modules")
        return OPENDAQ_ERR_INVALIDPROPERTY;

    this->options[option] = value;
    return OPENDAQ_SUCCESS;
}

ErrCode InstanceBuilderImpl::getOptions(IDict** options)
{
    if (options == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;
    
    *options = this->options.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode InstanceBuilderImpl::setRootDevice(IDevice* rootDevice)
{
    if (rootDevice == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;
    
    this->rootDevice = rootDevice;
    return OPENDAQ_SUCCESS;
}

ErrCode InstanceBuilderImpl::getRootDevice(IDevice** rootDevice)
{
    if (rootDevice == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;
    
    if (this->rootDevice.assigned())
        *rootDevice = this->rootDevice.addRefAndReturn();
    else
        *rootDevice = nullptr;
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
    
    *localId = this->localId.addRefAndReturn();
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
