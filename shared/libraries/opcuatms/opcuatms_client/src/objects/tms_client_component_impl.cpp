#include "opcuatms_client/objects/tms_client_component_impl.h"
#include "opendaq/device_impl.h"
#include "opendaq/folder_impl.h"
#include "opendaq/io_folder_impl.h"
#include "opendaq/mirrored_signal_impl.h"
#include "opendaq/input_port_impl.h"
#include "opcuatms_client/objects/tms_client_tags_factory.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

using namespace daq::opcua;

template <class Impl>
ErrCode TmsClientComponentBaseImpl<Impl>::getActive(Bool* active)
{   
    try
    {
        *active = this->template readValue<IBoolean>("Active");
    }
    catch(...)
    {
        *active = true;
        auto loggerComponent = getLoggerComponent();
        LOG_D("Failed to get active of component \"{}\". The default value was returned \"true\"", this->globalId);
    }
    return OPENDAQ_SUCCESS;
}

template <class Impl>
ErrCode TmsClientComponentBaseImpl<Impl>::setActive(Bool active)
{
    try
    {
        this->template writeValue<IBoolean>("Active", active);
        return OPENDAQ_SUCCESS;
    }
    catch(...)
    {
        auto loggerComponent = getLoggerComponent();
        LOG_D("Failed to set active of component \"{}\"", this->globalId);
    }
    return OPENDAQ_IGNORED;
}

template <class Impl>
void TmsClientComponentBaseImpl<Impl>::initComponent()
{

    try
    {
        this->tags = TmsClientTags(this->daqContext, this->clientContext, this->getNodeId("Tags"));
    }
    catch([[maybe_unused]] const std::exception& e)
    {
        const auto loggerComponent = getLoggerComponent();
        LOG_D("OpcUA Component {} failed to initialize: {}", this->globalId, e.what());
    }
    catch(...)
    {
        const auto loggerComponent = getLoggerComponent();
        LOG_D("OpcUA Component {} failed to initialize", this->globalId);
    }
}

template <class Impl>
ErrCode TmsClientComponentBaseImpl<Impl>::getName(IString** name)
{
    OPENDAQ_PARAM_NOT_NULL(name);

    StringPtr nameObj;
    try
    {
        nameObj = this->client->readDisplayName(this->nodeId);
    }
    catch(...)
    {
        nameObj = this->localId;
        auto loggerComponent = getLoggerComponent();
        LOG_D("Failed to get name of component \"{}\". The default value was returned \"{}\" (local id)", this->globalId, nameObj);
    }
    *name = nameObj.detach();
    return OPENDAQ_SUCCESS;
}

template <class Impl>
ErrCode TmsClientComponentBaseImpl<Impl>::setName(IString* name)
{
    OPENDAQ_PARAM_NOT_NULL(name);

    try
    {
        StringPtr nameObj = name;
        this->client->writeDisplayName(this->nodeId, nameObj);
        return OPENDAQ_SUCCESS;
    }
    catch(...)
    {
        auto loggerComponent = getLoggerComponent();
        LOG_D("Failed to set name of component \"{}\"", this->globalId);
    }

    return OPENDAQ_IGNORED;
}

template <class Impl>
ErrCode TmsClientComponentBaseImpl<Impl>::getDescription(IString** description)
{
    OPENDAQ_PARAM_NOT_NULL(description);

    try
    {
        StringPtr descObj = this->client->readDescription(this->nodeId);
        *description = descObj.detach();
    }
    catch(...)
    {
        *description = StringPtr("").detach();
        auto loggerComponent = getLoggerComponent();
        LOG_D("Failed to get description of component \"{}\". The default value was returned \"\"", this->globalId);
    }
    return OPENDAQ_SUCCESS;
}

template <class Impl>
ErrCode TmsClientComponentBaseImpl<Impl>::setDescription(IString* description)
{
    OPENDAQ_PARAM_NOT_NULL(description);

    try
    {
        StringPtr descriptionObj = description;
        this->client->writeDescription(this->nodeId, descriptionObj);
        return OPENDAQ_SUCCESS;
    }
    catch(...)
    {
        auto loggerComponent = getLoggerComponent();
        LOG_D("Failed to set description of component \"{}\"", this->globalId);
    }

    return OPENDAQ_IGNORED;
}

template <class Impl>
ErrCode TmsClientComponentBaseImpl<Impl>::getVisible(Bool* visible)
{
    try
    {
        *visible = this->template readValue<IBoolean>("Visible");
    }
    catch(...)
    {
        *visible = true;
        const auto loggerComponent = getLoggerComponent();
        LOG_D("OpcUA Component {} failed to fetch \"Visible\" state. The default value was returned \"true\"", this->globalId);
    }

    return OPENDAQ_SUCCESS;
}

template <class Impl>
ErrCode TmsClientComponentBaseImpl<Impl>::setVisible(Bool visible)
{
    try
    {
        this->template writeValue<IBoolean>("Visible", visible);
        return OPENDAQ_SUCCESS;
    }
    catch (...)
    {
        const auto loggerComponent = getLoggerComponent();
        LOG_D("OpcUA Component {} failed to set \"Active\" state.", this->globalId);
    }

    return OPENDAQ_IGNORED;
}

template <class Impl>
LoggerComponentPtr TmsClientComponentBaseImpl<Impl>::getLoggerComponent()
{
    return this->daqContext.getLogger().getOrAddComponent("OpcUaClientComponent"); 
}
template class TmsClientComponentBaseImpl<ComponentImpl<>>;
template class TmsClientComponentBaseImpl<FolderImpl<IFolderConfig>>;
template class TmsClientComponentBaseImpl<IoFolderImpl<>>;
template class TmsClientComponentBaseImpl<Device>;
template class TmsClientComponentBaseImpl<FunctionBlock>;
template class TmsClientComponentBaseImpl<Channel>;
template class TmsClientComponentBaseImpl<MirroredSignal>;
template class TmsClientComponentBaseImpl<InputPortImpl>;

END_NAMESPACE_OPENDAQ_OPCUA_TMS
