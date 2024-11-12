#include <opendaq/client_type.h>
#include <coreobjects/property_factory.h>

BEGIN_NAMESPACE_OPENDAQ

void ClientTypeTools::DefineConfigProperties(PropertyObjectPtr obj)
{
    auto clientTypes = Dict<IInteger, IString>();
    clientTypes.set((Int) ClientType::Control, "Control");
    clientTypes.set((Int) ClientType::ExclusiveControl, "Exclusive Control");
    clientTypes.set((Int) ClientType::ViewOnly, "View Only");

    const auto clientTypeProp =
        SparseSelectionPropertyBuilder("ClientType", clientTypes, (Int) ClientType::Control)
            .setDescription("Specifies the client's connection type. Control and Exclusive Control clients can modify the device, while "
                            "View-Only clients can only read from the device. When an Exclusive Control client is connected, no other "
                            "Control or Exclusive Control clients can connect to the same device.")
            .build();

    auto dropOthersProp =
        BoolPropertyBuilder("ExclusiveControlDropOthers", false)
            .setDescription("If enebaled, when connecting as an Exclusive Control client, any existing Control clients will be disconnected.")
            .build();

    obj.addProperty(clientTypeProp);
    obj.addProperty(dropOthersProp);
}

ClientType ClientTypeTools::IntToClientType(Int value)
{
    switch (value)
    {
        case (Int) ClientType::Control:
            return ClientType::Control;
        case (Int) ClientType::ExclusiveControl:
            return ClientType::ExclusiveControl;
        case (Int) ClientType::ViewOnly:
            return ClientType::ViewOnly;
    }

    throw InvalidValueException();
}

END_NAMESPACE_OPENDAQ
