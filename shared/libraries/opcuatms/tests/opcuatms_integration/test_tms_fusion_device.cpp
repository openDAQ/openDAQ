#include <coreobjects/property_object_class_factory.h>
#include "coreobjects/callable_info_factory.h"
#include "coreobjects/property_object_factory.h"
#include "coreobjects/unit_factory.h"
#include "coretypes/type_manager_factory.h"
#include "gtest/gtest.h"
#include "opcuaclient/opcuaclient.h"
#include "opcuatms_client/objects/tms_client_property_object_factory.h"
#include "opcuatms_client/objects/tms_client_property_object_impl.h"
#include "opcuatms_server/objects/tms_server_property_object.h"
#include "opcuatms_client/objects/tms_client_signal_factory.h"
#include "opcuatms_server/objects/tms_server_signal.h"
#include "tms_object_integration_test.h"
#include "opendaq/instance_factory.h"

using namespace daq;
using namespace opcua::tms;
using namespace opcua;
using namespace std::chrono_literals;

struct RegisteredPropertyObject
{
    TmsServerPropertyObjectPtr serverProp;
    PropertyObjectPtr clientProp;
};

class TmsFusionDevice : public TmsObjectIntegrationTest
{
protected:

    TypeManagerPtr objManager;

    SignalPtr createSignal(const std::string& id)
    {
        SignalPtr signal = Signal(NullContext(), nullptr, id);
        signal->setActive(false);
        return signal;
    }

    void SetUp() override
    {
        TmsObjectIntegrationTest::SetUp();

         // create class with name "STGAmplifier"
        auto stgAmplClass =
            PropertyObjectClassBuilder("StgAmp")
                .addProperty(SelectionProperty(
                    "Measurement", List<IString>("Voltage", "Bridge", "Resistance", "Temperature", "Current", "Potentiometer"), 0))
                
                .build();

        objManager = TypeManager();
        objManager.addType(stgAmplClass);

       
    }

    void TearDown() override
    {
        objManager.removeType("StgAmp");
    }

    RegisteredPropertyObject registerPropertyObject(const PropertyObjectPtr& prop)
    {
        const auto logger = Logger();
        const auto context = Context(nullptr, logger, TypeManager(), nullptr);
        const auto serverProp =
            std::make_shared<TmsServerPropertyObject>(prop, server, context, std::make_shared<TmsServerContext>(context));
        const auto nodeId = serverProp->registerOpcUaNode();
        const auto clientProp = TmsClientPropertyObject(Context(nullptr, logger, TypeManager(), nullptr), clientContext, nodeId);
        return {serverProp, clientProp};
    }
};

TEST_F(TmsFusionDevice, IntegrationTest)
{

    SignalPtr daqServerSignal = createSignal("id");
    daqServerSignal.addProperty(FloatProperty("SampleRate", 1.0, false));

    auto serverSignal = TmsServerSignal(daqServerSignal, this->getServer(), ctx, serverContext);
    auto nodeId = serverSignal.registerOpcUaNode();

    SignalPtr clientSignal = TmsClientSignal(NullContext(), nullptr, "sig", clientContext, nodeId);

    ASSERT_TRUE(clientSignal.getPublic());

    ASSERT_NO_THROW(clientSignal.getPropertyValue("SampleRate"));
}

