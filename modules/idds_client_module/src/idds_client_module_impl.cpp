#include <idds_client_module/idds_client_module_impl.h>
#include <idds_client_module/version.h>
#include <coretypes/version_info_factory.h>
#include <chrono>
#include <coreobjects/callable_info_factory.h>
#include <coreobjects/argument_info_factory.h>
#include <opendaq/device_type_factory.h>
#include <opendaq/address_info_factory.h>
#include <opendaq/device_impl.h>

BEGIN_NAMESPACE_OPENDAQ_IDDS_CLIENT_MODULE

using namespace daq::idds;

iDDSClientModule::iDDSClientModule(ContextPtr context)
    : Module("OpenDAQiDDSClientModule",
            daq::VersionInfo(IDDS_CL_MODULE_MAJOR_VERSION, IDDS_CL_MODULE_MINOR_VERSION, IDDS_CL_MODULE_PATCH_VERSION),
            std::move(context),
            "OpenDAQiDDSClientModule"),
      iDDSWrapper()
{
}

DictPtr<IString, IDeviceType> iDDSClientModule::onGetAvailableDeviceTypes()
{
    auto result = Dict<IString, IDeviceType>();

    //TBD: Add device types here

    return result;
}

DevicePtr iDDSClientModule::onCreateDevice(const StringPtr& connectionString,
                                                         const ComponentPtr& parent,
                                                         const PropertyObjectPtr& /*config*/)
{
    DevicePtr obj(createWithImplementation<IDevice, Device>(context, parent, "iDDSDevice"));

    //iDDSWrapper.addTopic("openDaq");
    //iDDSWrapper.start()

    ProcedurePtr sendMessage = []() {
        //iDDSWrapper.sendMessage("openDaq", "Hello from openDaq");
    };
    PropertyPtr sendMessageProperty = FunctionProperty("SendMessage", ProcedureInfo(List<IArgumentInfo>(
            ArgumentInfo("message", ctString)
    )));
    obj->addProperty(sendMessageProperty);

    return obj;
}

END_NAMESPACE_OPENDAQ_IDDS_CLIENT_MODULE
