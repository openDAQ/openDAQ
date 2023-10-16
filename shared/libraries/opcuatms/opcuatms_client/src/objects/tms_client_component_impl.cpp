#include "opcuatms_client/objects/tms_client_component_impl.h"
#include "opendaq/device_impl.h"
#include "opendaq/folder_impl.h"
#include "opendaq/io_folder_impl.h"
#include "opendaq/signal_remote_impl.h"
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
ErrCode TmsClientComponentBaseImpl<Impl>::getTags(ITagsConfig** tags)
{
    return daqTry([&]() {
        ListPtr<IString> tagValues = this->template readList<IString>("Tags");
        auto tagsObj = Tags();
        for (auto tag : tagValues)
            tagsObj.add(tag);
        tagsObj.freeze();
        *tags = tagsObj.detach();
        return OPENDAQ_SUCCESS;
    });
}

template class TmsClientComponentBaseImpl<ComponentImpl<>>;
template class TmsClientComponentBaseImpl<FolderImpl<IFolderConfig>>;
template class TmsClientComponentBaseImpl<IoFolderImpl>;
template class TmsClientComponentBaseImpl<Device>;
template class TmsClientComponentBaseImpl<FunctionBlock>;
template class TmsClientComponentBaseImpl<Channel>;
template class TmsClientComponentBaseImpl<SignalRemote<SignalStandardProps::Skip>>;
template class TmsClientComponentBaseImpl<InputPortImpl>;

END_NAMESPACE_OPENDAQ_OPCUA_TMS
