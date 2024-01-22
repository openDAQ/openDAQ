#include <opendaq/context_impl.h>
#include <coretypes/validation.h>
#include <coretypes/intfs.h>
#include <opendaq/module_manager_ptr.h>
#include <opendaq/component_private_ptr.h>
#include <opendaq/custom_log.h>

BEGIN_NAMESPACE_OPENDAQ

ContextImpl::ContextImpl(SchedulerPtr scheduler,
                             LoggerPtr logger,
                             TypeManagerPtr typeManager,
                             ModuleManagerPtr moduleManager)
    : logger(std::move(logger))
    , scheduler(std::move(scheduler))
    , moduleManager(std::move(moduleManager))
    , typeManager(std::move(typeManager))
{
    if (!this->logger.assigned())
        throw ArgumentNullException("Logger must not be null");

    if (this->moduleManager.assigned())
    {
        this->moduleManagerWeakRef = this->moduleManager;
        // Have to increment the ref-count with `thisInterface()` so passing it to the module doesn't crashc
        checkErrorInfo(this->moduleManager.asPtr<IModuleManager>()->loadModules(this->thisInterface()));

        // manually remove the reference count without deleting the object (as reference count should drop to 0)
        this->internalReleaseRef();
        assert(this->getReferenceCount() >= 0);
    }

    Event(this->coreEvent) += event(&ContextImpl::componentCoreEventCallback);
}

ContextImpl::~ContextImpl()
{
    Event(this->coreEvent) -= event(&ContextImpl::componentCoreEventCallback);
}

ErrCode ContextImpl::getScheduler(IScheduler** scheduler)
{
    OPENDAQ_PARAM_NOT_NULL(scheduler);

    *scheduler = this->scheduler.addRefAndReturn();

    return OPENDAQ_SUCCESS;
}

ErrCode ContextImpl::getLogger(ILogger** logger)
{
    OPENDAQ_PARAM_NOT_NULL(logger);

    *logger = this->logger.addRefAndReturn();

    return OPENDAQ_SUCCESS;
}

ErrCode ContextImpl::getModuleManager(IBaseObject** manager)
{
    OPENDAQ_PARAM_NOT_NULL(manager);

    return daqTry([&]()
        {
            if (this->moduleManagerWeakRef.assigned())
            {
                *manager = moduleManagerWeakRef.getRef().asPtr<IBaseObject>().detach();
            }
            else
            {
                *manager = nullptr;
            }
            return OPENDAQ_SUCCESS;
        });
}

ErrCode ContextImpl::getTypeManager(ITypeManager** manager)
{
    OPENDAQ_PARAM_NOT_NULL(manager);

    *manager = this->typeManager.addRefAndReturn();

    return OPENDAQ_SUCCESS;
}

ErrCode ContextImpl::getOnCoreEvent(IEvent** event)
{
    OPENDAQ_PARAM_NOT_NULL(event);

    *event = coreEvent.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode ContextImpl::moveModuleManager(IModuleManager** manager)
{
    OPENDAQ_PARAM_NOT_NULL(manager);

    if (this->moduleManager.assigned())
    {
        *manager = moduleManager.detach();
        moduleManager = nullptr;
    }
    else
    {
        *manager = nullptr;
    }

    return OPENDAQ_SUCCESS;
}

void ContextImpl::componentCoreEventCallback(ComponentPtr& component, CoreEventArgsPtr& eventArgs)
{
    try
    {
        component.asPtr<IComponentPrivate>()->triggerComponentCoreEvent(eventArgs);
    }
    catch (const std::exception& e)
    {
        const auto loggerComponent = this->logger.getOrAddComponent("Component");
        LOG_W("Component {} failed while triggering core event {} with: {}", component.getLocalId(), eventArgs.getEventName(), e.what())
    }
    catch (...)
    {
        const auto loggerComponent = this->logger.getOrAddComponent("Component");
        LOG_W("Component {} failed while triggering core event {}", component.getLocalId(), eventArgs.getEventName())
    }

}

OPENDAQ_DEFINE_CLASS_FACTORY(
    LIBRARY_FACTORY,
    Context,
    IScheduler*,
    Scheduler,
    ILogger*,
    Logger,
    ITypeManager*,
    typeManager,
    IModuleManager*,
    moduleManager
    )

END_NAMESPACE_OPENDAQ
