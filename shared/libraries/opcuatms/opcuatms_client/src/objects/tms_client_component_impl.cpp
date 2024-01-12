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
    try
    {
        *active = this->template readValue<IBoolean>("Active");
        return OPENDAQ_SUCCESS;
    }
    catch(...)
    {
        auto loggerComponent = getLogger();
        StringPtr nameObj;
        this->getName(&nameObj);
        LOG_W("Failed to get active of component \"{}\"", nameObj);
    }
    return OPENDAQ_IGNORED;
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
        auto loggerComponent = getLogger();
        StringPtr nameObj;
        this->getName(&nameObj);
        LOG_W("Failed to set active of component \"{}\"", nameObj);
    }
    return OPENDAQ_IGNORED;
}

template <class Impl>
ErrCode TmsClientComponentBaseImpl<Impl>::getTags(ITagsConfig** tags)
{
    try
    {
        ListPtr<IString> tagValues = this->template readList<IString>("Tags");
        auto tagsObj = Tags();
        for (auto tag : tagValues)
            tagsObj.add(tag);
        tagsObj.freeze();
        *tags = tagsObj.detach();
        return OPENDAQ_SUCCESS;
    }
    catch(...)
    {
        auto loggerComponent = getLogger();
        StringPtr nameObj;
        this->getName(&nameObj);
        LOG_W("Failed to get tags of component \"{}\"", nameObj);
    }
    return OPENDAQ_IGNORED;
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
        nameObj = "unknown name";
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
        auto loggerComponent = getLogger();
        StringPtr nameObj;
        this->getName(&nameObj);
        LOG_W("Failed to set name of component \"{}\"", nameObj);
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
        return OPENDAQ_SUCCESS;
    }
    catch(...)
    {
        auto loggerComponent = getLogger();
        StringPtr nameObj;
        this->getName(&nameObj);
        LOG_W("Failed to get description of component \"{}\"", nameObj);
    }
    return OPENDAQ_IGNORED;
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
        auto loggerComponent = getLogger();
        StringPtr descObj;
        this->getName(&descObj);
        LOG_W("Failed to set description of component \"{}\"", descObj);
    }

    return OPENDAQ_IGNORED;
}
template <class Impl>
LoggerComponentPtr TmsClientComponentBaseImpl<Impl>::getLogger()
{
    return this->daqContext.getLogger().getOrAddComponent("OpcUaClientComponent"); 
}

template class TmsClientComponentBaseImpl<ComponentImpl<>>;
template class TmsClientComponentBaseImpl<FolderImpl<IFolderConfig>>;
template class TmsClientComponentBaseImpl<IoFolderImpl>;
template class TmsClientComponentBaseImpl<Device>;
template class TmsClientComponentBaseImpl<FunctionBlock>;
template class TmsClientComponentBaseImpl<Channel>;
template class TmsClientComponentBaseImpl<MirroredSignal>;
template class TmsClientComponentBaseImpl<InputPortImpl>;

END_NAMESPACE_OPENDAQ_OPCUA_TMS
