#include <opendaq/context_impl.h>
#include <coretypes/validation.h>
#include <coretypes/intfs.h>
#include <opendaq/module_manager_ptr.h>
#include <opendaq/component_private_ptr.h>
#include <opendaq/custom_log.h>
#include <coretypes/type_manager_private.h>
#include <coreobjects/core_event_args_factory.h>
#include <coreobjects/authentication_provider_factory.h>
#include <coreobjects/property_object_class_factory.h>
#include <coreobjects/property_object_factory.h>
#include <coreobjects/coreobjects.h>

BEGIN_NAMESPACE_OPENDAQ

ContextImpl::ContextImpl(SchedulerPtr scheduler,
                         LoggerPtr logger,
                         TypeManagerPtr typeManager,
                         ModuleManagerPtr moduleManager,
                         AuthenticationProviderPtr authenticationProvider,
                         DictPtr<IString, IBaseObject> options,
                         DictPtr<IString, IDiscoveryServer> discoveryServices)
    : logger(std::move(logger))
    , scheduler(std::move(scheduler))
    , moduleManager(std::move(moduleManager))
    , typeManager(std::move(typeManager))
    , authenticationProvider(std::move(authenticationProvider))
    , options(std::move(options))
    , discoveryServices(std::move(discoveryServices))
{
    if (!this->logger.assigned())
        throw ArgumentNullException("Logger must not be null");

    if (!this->authenticationProvider.assigned())
        this->authenticationProvider = AuthenticationProvider();

    if (this->moduleManager.assigned())
    {
        this->moduleManagerWeakRef = this->moduleManager;
        // Have to increment the ref-count with `thisInterface()` so passing it to the module doesn't crash
        checkErrorInfo(this->moduleManager.asPtr<IModuleManager>()->loadModules(this->thisInterface()));

        // manually remove the reference count without deleting the object (as reference count should drop to 0)
        this->internalReleaseRef();
        assert(this->getReferenceCount() >= 0);
    }

    const ProcedurePtr typeManagerCallback = [this](const BaseObjectPtr& val)
    {
        if (val.supportsInterface(IString::Id))
        {
            auto args = CoreEventArgsTypeRemoved(val);
            ComponentPtr ptr;
            coreEvent.trigger(ptr, args);
        }
        else
        {
            auto args = CoreEventArgsTypeAdded(val);
            ComponentPtr ptr;
            coreEvent.trigger(ptr, args);
        }
    };

    if (this->typeManager.assigned())
        this->typeManager.asPtr<ITypeManagerPrivate>()->setCoreEventCallback(typeManagerCallback);
    Event(this->coreEvent) += event(&ContextImpl::componentCoreEventCallback);

    registerOpenDaqTypes();
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

ErrCode INTERFACE_FUNC ContextImpl::getAuthenticationProvider(IAuthenticationProvider** authenticationProvider)
{
    OPENDAQ_PARAM_NOT_NULL(authenticationProvider);

    *authenticationProvider = this->authenticationProvider.addRefAndReturn();
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

ErrCode ContextImpl::getOptions(IDict** options)
{
    OPENDAQ_PARAM_NOT_NULL(options);

    if (!this->options.assigned())
    {
        *options = Dict<IString, IBaseObject>().detach();
        return OPENDAQ_SUCCESS;
    }

    *options = this->options.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode ContextImpl::getModuleOptions(IString* moduleId, IDict** options)
{
    OPENDAQ_PARAM_NOT_NULL(moduleId);
    OPENDAQ_PARAM_NOT_NULL(options);

    if (this->options.assigned() && this->options.hasKey("Modules"))
    {
        DictPtr<IString, IBaseObject> modules = this->options.get("Modules");
        if (modules.assigned() && modules.hasKey(moduleId))
        {
            DictPtr<IString, IBaseObject> moduleOptions = modules.get(moduleId);
            if (moduleOptions.assigned())
            {
                *options = moduleOptions.addRefAndReturn();
                return OPENDAQ_SUCCESS;
            }
        }
    }

    *options = Dict<IString, IBaseObject>().detach();
    return OPENDAQ_SUCCESS;
}

void ContextImpl::componentCoreEventCallback(ComponentPtr& component, CoreEventArgsPtr& eventArgs)
{
    if (!component.assigned())
        return;

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

ErrCode ContextImpl::getDiscoveryServers(IDict** services)
{
    OPENDAQ_PARAM_NOT_NULL(services);
    if (!this->discoveryServices.assigned())
    {
        *services = Dict<IString, IDiscoveryServer>().detach();
        return OPENDAQ_SUCCESS;
    }
    *services = this->discoveryServices.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

void ContextImpl::registerOpenDaqTypes()
{
    if (typeManager == nullptr)
        return;

    //Sync Component Interfaces
    auto syncInterfaceBase = PropertyObjectClassBuilder(typeManager, "SyncInterfaceBase")
                                    .addProperty(SelectionProperty("Mode", List<IString>("Input", "Output", "Auto", "Off"), 4))
                                    .build();
    typeManager->addType(syncInterfaceBase);

    PropertyObjectPtr InterfaceClockSyncStatusProperty = PropertyObject();
    InterfaceClockSyncStatusProperty.addProperty(SelectionProperty("State", List<IString>("Ok", "Error", "Warning"), 0));

    auto interfaceClockSync = PropertyObjectClassBuilder(typeManager, "InterfaceClockSync")
                                    .setParentName("SyncInterfaceBase")
                                    .addProperty(SelectionProperty("Mode", List<IString>("Input", "Output", "Auto", "Off"), 4))
                                    .addProperty(ObjectProperty("Status", InterfaceClockSyncStatusProperty))
                                    .build();
    typeManager->addType(interfaceClockSync);

    PropertyObjectPtr PtpSyncInterfaceStatus = PropertyObject();
    PtpSyncInterfaceStatus.addProperty(SelectionProperty("State", List<IString>("Ok", "Error", "Warning"), 0));
    PtpSyncInterfaceStatus.addProperty(StringProperty("Grandmaster", ""));

    //Ptp Enumerations
    const auto enumClockType = EnumerationType(
        "PtpClockTypeEnumeration", List<IString>("Transparent", "OrdinaryBoundary", "SlaveOnly", "MasterOnly"));
    const auto enumStepFlag = EnumerationType(
        "PtpStepFlagEnumeration", List<IString>("One", "Two"));
    const auto enumTransportProtocol = EnumerationType(
        "PtpProtocolEnumeration", List<IString>("IEEE802_3", "UDP_IPV4", "UDP_IPV6", "UDP6_SCOPE"));
    const auto enumDelayMechanism = EnumerationType(
        "PtpDelayMechanismEnumeration", List<IString>("P2P", "E2E"));
    const auto enumProfiles = EnumerationType(
        "PtpProfileEnumeration", List<IString>("I558", "802_1AS"));

    typeManager->addType(enumClockType);
    typeManager->addType(enumStepFlag);
    typeManager->addType(enumTransportProtocol);
    typeManager->addType(enumDelayMechanism);
    typeManager->addType(enumProfiles);

    PropertyObjectPtr ports = PropertyObject();
    // ports.addProperty(BoolProperty("Port1", true));

    PropertyObjectPtr parameters = PropertyObject();
    parameters.addProperty(StructProperty("PtpConfigurationStructure",
                                        Struct("PtpConfigurationStructure",
                                                Dict<IString, IBaseObject>({{"ClockType", Enumeration("PtpClockTypeEnumeration", "Transparent", typeManager)},
                                                            {"TransportProtocol", Enumeration("PtpProtocolEnumeration", "IEEE802_3", typeManager)},
                                                            {"StepFlag", Enumeration("PtpStepFlagEnumeration", "One", typeManager)},
                                                            {"DomainNumber", 0},
                                                            {"LeapSeconds", 0},
                                                            {"DelayMechanism", Enumeration("PtpDelayMechanismEnumeration", "P2P", typeManager)},
                                                            {"Priority1", 0},
                                                            {"Priority2", 0},
                                                            {"Profiles", Enumeration("PtpProfileEnumeration", "I558", typeManager)}}),
                                                            typeManager)));
    parameters.addProperty(ObjectProperty("Ports", ports));

    auto ptpSyncInterface = PropertyObjectClassBuilder(typeManager, "PtpSyncInterface")
                                    .setParentName("SyncInterfaceBase")
                                    .addProperty(SelectionProperty("Mode", List<IString>("Input", "Output", "Auto", "Off"), 4))
                                    .addProperty(ObjectProperty("Status", PtpSyncInterfaceStatus))
                                    .addProperty(ObjectProperty("Parameters", parameters))
                                    .build();

    typeManager->addType(ptpSyncInterface);
}

OPENDAQ_DEFINE_CLASS_FACTORY(
    LIBRARY_FACTORY,
    Context,
    IScheduler*, Scheduler,
    ILogger*, Logger,
    ITypeManager*, typeManager,
    IModuleManager*, moduleManager,
    IAuthenticationProvider*, authenticationProvider,
    IDict*, options,
    IDict*, discoveryServices
)

END_NAMESPACE_OPENDAQ
