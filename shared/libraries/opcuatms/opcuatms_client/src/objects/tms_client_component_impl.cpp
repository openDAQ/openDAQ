#include "opcuatms_client/objects/tms_client_component_impl.h"
#include "opendaq/device_impl.h"
#include "opendaq/folder_impl.h"
#include "opendaq/io_folder_impl.h"
#include "opendaq/mirrored_signal_impl.h"
#include "opendaq/input_port_impl.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

using namespace daq::opcua;

template <class Impl>
ErrCode TmsClientComponentBaseImpl<Impl>::getActive(Bool* active)
{
    return daqTry([&]() {
        *active = this->template readValue<IBoolean>("Active");
        return OPENDAQ_SUCCESS;
    });
}

template <class Impl>
ErrCode TmsClientComponentBaseImpl<Impl>::setActive(Bool active)
{
    return daqTry([&]() {
        this->template writeValue<IBoolean>("Active", active);
        return OPENDAQ_SUCCESS;
    });
}

template <class Impl>
void TmsClientComponentBaseImpl<Impl>::initComponent()
{
    try
    {
        ListPtr<IString> tagValues = this->template readList<IString>("Tags");
        auto tagsObj = Tags();
        for (auto tag : tagValues)
            tagsObj.add(tag);
        tagsObj.freeze();
        this->tags = tagsObj.detach();
    }
    catch([[maybe_unused]] const std::exception& e)
    {
        const auto loggerComponent = this->daqContext.getLogger().getOrAddComponent("OpcUaClient");
        LOG_D("OPC UA Component {} failed to fetch tags: {}", this->localId, e.what());
    }
    catch(...)
    {
        const auto loggerComponent = this->daqContext.getLogger().getOrAddComponent("OpcUaClient");
        LOG_D("OPC UA Component {} failed to fetch tags.", this->localId);
    }
}

template <class Impl>
ErrCode TmsClientComponentBaseImpl<Impl>::getName(IString** name)
{
    OPENDAQ_PARAM_NOT_NULL(name);

    return daqTry([&]
    {
        StringPtr nameObj =this->client->readDisplayName(this->nodeId);
        *name = nameObj.detach();
        return OPENDAQ_SUCCESS;
    });
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
        auto loggerComponent = this->daqContext.getLogger().getOrAddComponent("OpcUaClientComponent");
        LOG_W("Failed to set name of component \"{}\"", this->localId);
    }

    return OPENDAQ_IGNORED;
}

template <class Impl>
ErrCode TmsClientComponentBaseImpl<Impl>::getDescription(IString** description)
{
    OPENDAQ_PARAM_NOT_NULL(description);

    return daqTry([&]
    {
        StringPtr descObj = this->client->readDescription(this->nodeId);
        *description = descObj.detach();
        return OPENDAQ_SUCCESS;
    });
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
        auto loggerComponent = this->daqContext.getLogger().getOrAddComponent("OpcUaClientComponent");
        LOG_W("Failed to set description of component \"{}\"", this->localId);
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
        const auto loggerComponent = this->daqContext.getLogger().getOrAddComponent("OpcUaClient");
        LOG_D("OPC UA Component {} failed to fetch \"Visible\" state.", this->localId);
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
        const auto loggerComponent = this->daqContext.getLogger().getOrAddComponent("OpcUaClient");
        LOG_D("OPC UA Component {} failed to set \"Active\" state.", this->localId);
    }

    return OPENDAQ_IGNORED;
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
