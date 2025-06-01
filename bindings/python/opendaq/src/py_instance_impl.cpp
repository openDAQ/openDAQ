#include "py_opendaq/py_instance_impl.h"
#include <opendaq/instance_builder_ptr.h>
#include <opendaq/instance_impl.h>
#include <opendaq/context_factory.h>
#include <opendaq/logger_factory.h>
#include <opendaq/scheduler_factory.h>
#include <opendaq/module_manager_factory.h>
#include <opendaq/discovery_server_factory.h>
#include <opendaq/discovery_server_factory.h>
#include "py_core_types/py_opendaq_daq.h"

BEGIN_NAMESPACE_OPENDAQ

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
    for (const auto& serverName : builderPtr.getDiscoveryServers())
    {
        auto server = InstanceImpl::createDiscoveryServer(serverName, logger);
        if (server.assigned())
            discoveryServers.set(serverName, server);
    }

    return Context(scheduler, logger, typeManager, moduleManager, authenticationProvider, options, discoveryServers);
}

PythonInstanceImpl::PythonInstanceImpl(IInstanceBuilder* instanceBuilder)
    : InstanceImpl(ContextFromInstanceBuilder(instanceBuilder), 
                   InstanceBuilderPtr::Borrow(instanceBuilder).getDefaultRootDeviceLocalId(),
                   InstanceBuilderPtr::Borrow(instanceBuilder).getDefaultRootDeviceInfo(),
                   InstanceBuilderPtr::Borrow(instanceBuilder).getRootDevice(),
                   InstanceBuilderPtr::Borrow(instanceBuilder).getRootDeviceConfig())
{
}

END_NAMESPACE_OPENDAQ

PyDaqIntf<daq::IInstance, daq::IDevice> declareIPythonInstance(pybind11::module_ m)
{
    return wrapInterface<daq::IInstance, daq::IDevice>(m, "IInstance");
}