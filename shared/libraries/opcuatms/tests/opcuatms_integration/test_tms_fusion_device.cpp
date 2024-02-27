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
#include <opcuatms_client/tms_client.h>

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
        objManager = TypeManager();

        //Add Enumeration type to Type Manager
        const auto enumExcitationType = EnumerationType("ExcitationTypeEnumeration", List<IString>("DoNotCare", "DCVoltage", "ACVoltage", "ACVoltageRectangle", "ACVoltageSinewave"));
        objManager.addType(enumExcitationType);

        // create class with name "FusionAmp"
        auto fusionAmpClass =
            PropertyObjectClassBuilder("FusionAmp")
                .addProperty(SelectionProperty(
                    "Measurement", List<IString>("Voltage", "FullBridge", "HalfBridge", "QuarterBridge"), 0))
                .addProperty(StructProperty(
                    "AdjustmentPoint", Struct("AdjustmentPointScalingStructure", Dict<IString, IBaseObject>({{"Index", 1}, {"Factor", 2.1}, {"Offset", 3.0}}), objManager))
                )
                .addProperty(StructProperty(
                    "Scaler", Struct("GainScalingStructure", Dict<IString, IBaseObject>({{"Factor", 2.1}, {"Offset", 3.0}}), objManager))
                )
                .addProperty(EnumerationProperty(
                    "ExcitationType", Enumeration("ExcitationTypeEnumeration", "DCVoltage", objManager))
                )
                .build();
        objManager.addType(fusionAmpClass);

        std::cout << "DEBUG: List of types :" << std::endl;
        int i = 0;
        for (const auto& item : objManager.getTypes())
        {
            std::cout << "\t" << i << " : " << item << std::endl;
            i++;
        }
    }

    void TearDown() override
    {
        objManager.removeType("FusionAmp");
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

TEST_F(TmsFusionDevice, DISABLED_SampleRateTest)
{

    SignalPtr daqServerSignal = createSignal("id");
    daqServerSignal.addProperty(FloatProperty("SampleRate", 1.0, false));

    auto serverSignal = TmsServerSignal(daqServerSignal, this->getServer(), ctx, serverContext);
    auto nodeId = serverSignal.registerOpcUaNode();

    SignalPtr clientSignal = TmsClientSignal(NullContext(), nullptr, "sig", clientContext, nodeId);
    //std::cin.get();

    ASSERT_TRUE(clientSignal.getPublic());
    ASSERT_NO_THROW(clientSignal.getPropertyValue("SampleRate"));
}

TEST_F(TmsFusionDevice, DISABLED_StructTest)
{
    const auto obj = PropertyObject(objManager, "FusionAmp");
    auto [serverObj, fusionAmp] = registerPropertyObject(obj);

    // Test struct with int and float values
    const auto adjustmentPoint =  StructBuilder(fusionAmp.getPropertyValue("AdjustmentPoint"));
    adjustmentPoint.set("Index", 10);
    adjustmentPoint.set("Factor", 3.1);
    fusionAmp.setPropertyValue("AdjustmentPoint", adjustmentPoint.build());

    const auto adjustmentPointManipulated =  StructBuilder(fusionAmp.getPropertyValue("AdjustmentPoint"));
    ASSERT_EQ(adjustmentPointManipulated.get("Index"), 10);
    ASSERT_FLOAT_EQ(adjustmentPointManipulated.get("Factor"), (float) 3.1);

    // Test struct with double values
    const auto scaler =  StructBuilder(fusionAmp.getPropertyValue("Scaler"));
    scaler.set("Factor", 3.62);
    scaler.set("Offset", 3.1);
    fusionAmp.setPropertyValue("Scaler", scaler.build());

    const auto scalerManipulated =  StructBuilder(fusionAmp.getPropertyValue("Scaler"));
    ASSERT_DOUBLE_EQ(scalerManipulated.get("Factor"), (double) 3.62);
    ASSERT_DOUBLE_EQ(scalerManipulated.get("Offset"), (double) 3.1);
}

TEST_F(TmsFusionDevice, DISABLED_EnumTest)
{
    //Move this test to coreobjects after has it tests PropertyObject implementation for Enumgeration type properties

    const auto obj = PropertyObject(objManager, "FusionAmp");

    // Test enum
    obj.setPropertyValue("ExcitationType", Enumeration("ExcitationType", "ACVoltage", objManager));
    ASSERT_EQ(obj.getPropertyValue("ExcitationType"), Enumeration("ExcitationType", "ACVoltage", objManager));
}

TEST_F(TmsFusionDevice, EnumTest2)
{
    const auto obj = PropertyObject(objManager, "FusionAmp");
    const auto logger = Logger();
    const auto context = Context(nullptr, logger, TypeManager(), nullptr);
    const auto serverProp =
        std::make_shared<TmsServerPropertyObject>(obj, server, context, std::make_shared<TmsServerContext>(context));
    const auto nodeId = serverProp->registerOpcUaNode();
    while(true){}
    //const auto clientProp = TmsClientPropertyObject(Context(nullptr, logger, TypeManager(), nullptr), clientContext, nodeId);
}

TEST_F(TmsFusionDevice, DISABLED_EnumTest3)
{
    const auto obj = PropertyObject(objManager, "FusionAmp");
    auto [serverObj, fusionAmp] = registerPropertyObject(obj);

    // Test enum
    fusionAmp.setPropertyValue("ExcitationType", Enumeration("ExcitationTypeEnumeration", "ACVoltage", objManager));
    ASSERT_EQ(fusionAmp.getPropertyValue("ExcitationType"), Enumeration("ExcitationTypeEnumeration", "ACVoltage", objManager));
}

TEST_F(TmsFusionDevice, DISABLED_SimulatorTest)
{
    // this test should pass, if you have Fusion simulator running

    const std::string connectionString = "opc.tcp://10.0.210.201";

    TmsClient tmsClient(NullContext(), nullptr, connectionString, nullptr);
    auto clientDevice = tmsClient.connect();

    ASSERT_TRUE(clientDevice.assigned());
}


