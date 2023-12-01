#include <coreobjects/errors.h>
#include <coreobjects/instance_builder_impl.h>
#include <coreobjects/instance_builder_ptr.h>
#include <coreobjects/instance_ptr.h>
#include <utility>

BEGIN_NAMESPACE_OPENDAQ

InstanceBuilderImpl::InstanceBuilderImpl()
    : options(Dict<IString, IBaseObject>())
    , sinks(Dict<ILoggerSink, LogLevel>)
    , logger(Logger())
{
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
    
    *logger = this->logger.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode InstanceBuilderImpl::setGlobalLogLevel(LogLevel logLevel)
{
    this->logger.setLevel(level);
    return OPENDAQ_SUCCESS;
}

ErrCode InstanceBuilderImpl::getGlobalLogLevel(LogLevel* logLevel)
{
    if (logLevel == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *logLevel = this->logger.getLevel();
    return OPENDAQ_SUCCESS;
}

ErrCode InstanceBuilderImpl::setComponentLogLevel(IString* component, LogLevel logLevel)
{
    if (component == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    componentsLogLevel[component] = logLevel;
    return OPENDAQ_SUCCESS;
}

ErrCode InstanceBuilderImpl::getComponenstLogLevel(IDictPtr** componentsLogLevel)
{
    if (componentsLogLevel == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;
    
    *componentsLogLevel = this->componentsLogLevel.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode InstanceBuilderImpl::setSinkLogLevel(ILoggerSink* sink, LogLevel logLevel)
{
    if (sink == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;
    
    sinks[sink] = logLevel;
    return OPENDAQ_SUCCESS;
}

ErrCode InstanceBuilderImpl::setModulePath(IString* path)
{
    if (path == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;
    
    this->modulePath = path;
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

ErrCode InstanceBuilderImpl::setRootDevice(IString* connectionString)
{
    if (connectionString == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;
    
    // this->rootDevice = 
    return OPENDAQ_SUCCESS;
}

ErrCode InstanceBuilderImpl::getRootDevice(IDevice** rootDevice)
{
    if (rootDevice == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;
    
    *rootDevice = this->rootDevice.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode InstanceBuilderImpl::setDefaultRootDeviceInfo(IDeviceInfo* deviceInfo)
{
    if (deviceInfo == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;
    
    return OPENDAQ_SUCCESS;
}

ErrCode InstanceBuilderImpl::getDefaultRootDeviceInfo(IDevice** defaultDevice)
{
    if (defaultDevice == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;
    
    *defaultDevice = this->defaultDevice.addRefAndReturn();
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
