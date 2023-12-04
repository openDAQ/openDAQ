#include <coreobjects/errors.h>
#include <coreobjects/instance_builder_impl.h>
#include <coreobjects/instance_builder_ptr.h>
#include <coreobjects/instance_ptr.h>
#include <utility>

BEGIN_NAMESPACE_OPENDAQ

DictPtr<IString, IBaseObject> InstanceBuilderImpl::GetOptions()
{
    return Dict<IString, IBaseObject>({
        {"ModuleManager", Dict<IString, IBaseObject>({
                {"ModulesPath", ""}
            })},
        {"Scheduler", Dict<IString, IBaseObject>()},
        {"Logging", Dict<IString, IBaseObject>()},
        {"Modules", Dict<IString, IBaseObject>()}
    });
}

InstanceBuilderImpl::InstanceBuilderImpl()
    : options(DefaultOptions())
    , sinks(Dict<ILoggerSink, LogLevel>)
{
}

DictPtr<IString, IBaseObject>& InstanceBuilderImpl::getModuleManagerOptions()
{
    return options["ModuleManager"];
}
DictPtr<IString, IBaseObject>& InstanceBuilderImpl::getSchedulerOptions()
{
    return options["Scheduler"];
}
DictPtr<IString, IBaseObject>& InstanceBuilderImpl::getLoggingOptions()
{
    return options["Logging"];
}
DictPtr<IString, IBaseObject>& InstanceBuilderImpl::getModuleOptions(IString* module)
{
    const auto & modules = options["Modules"]
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
    
    if (!this->logger.assigned()) 
    {
        if (sinks.empty())
            this->logger = Logger();
        else
        {
            auto sinksList = List<ILoggerSink>();
            for (const auto& sink : sinks)
                sinksList.push_back(sink);
            
            this->logger = LoggerWithSinks(sinksList);
        }
    }
    
    if (getLoggingOptions().hasKey("GlobalLogLevel"))
        this->logger.setLevel(LogLevel(getLoggingOptions()["GlobalLogLevel"]));

    for (const auto& [component, logLevel] : componentsLogLevel)
    {
        auto createdComponent = this->logger->getOrAddComponent(component);
        createdComponent.setLevel(logLevel);
    }

    *logger = this->logger.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode InstanceBuilderImpl::setGlobalLogLevel(LogLevel logLevel)
{
    getLoggingOptions()["GlobalLogLevel"] = Uint(logLevel);
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

    componentsLogLevel[component] = logLevel;
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
    
    getModulesOptions()["ModulesPath"] = path;
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
    
    if (!this->moduleManager.assigned())
        this->moduleManager = ModuleManager(getModulesOptions()["ModulesPath"]);
    
    *moduleManager = this->moduleManager.addRefAndReturn();
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
    
    if (!this->scheduler.assigned())
        this->scheduler = Sheduler();
    
    *scheduler = this->scheduler.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode InstanceBuilderImpl::setOption(IString* option, IBaseObject* value)
{
    if (option == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;
    if (value == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

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

ErrCode InstanceBuilderImpl::getRootDevice(IDevice** rootDevice);
{
    if (connectionString == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;
    
    if (!this->rootDeviceConnectionString.assigned())
        this->rootDeviceConnectionString = "";
    
    *connectionString = this->rootDeviceConnectionString.addRefAndReturn();
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

// extern "C" ErrCode PUBLIC_EXPORT createInstanceBuilder(IInstanceBuilder** objTmp)
// {
//     return daq::createObject<IInstanceBuilder, InstanceBuilderImpl>(objTmp);
// }

// extern "C" ErrCode PUBLIC_EXPORT createInstanceBuilderFromExisting(IInstanceBuilder** objTmp, IInstance* instanceToCopy)
// {
//     return daq::createObject<IInstanceBuilder, InstanceBuilderImpl>(objTmp, instanceToCopy);
// }

END_NAMESPACE_OPENDAQ
