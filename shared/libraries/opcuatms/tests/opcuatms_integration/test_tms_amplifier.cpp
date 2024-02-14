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

using namespace daq;

class TMSAmplifierTest : public TmsObjectIntegrationTest
{
protected:

    TypeManagerPtr objManager;

    void SetUp() override
    {
        TmsObjectIntegrationTest::SetUp();

         // create class with name "STGAmplifier"
        auto stgAmplClass =
            PropertyObjectClassBuilder("StgAmp")
                .addProperty(SelectionProperty(
                    "Measurement", List<IString>("Voltage", "Bridge", "Resistance", "Temperature", "Current", "Potentiometer"), 0))
                .addProperty(BoolProperty("DualCore", False))
                .addProperty(ReferenceProperty("Range",
                                               EvalValue("switch($Measurement, 0, %VoltageRange, 1, %BridgeRange, 2, %ResistanceRange, 3, "
                                                         "%TemperatureRange, 4, %CurrentRange, 5, %PotentiometerRange)")))
                .addProperty(SelectionProperty("RangeUnit", List<IString>("Amplifier range", "Scaled range"), 0))
                .addProperty(SelectionProperty("LowPassFilter",
                                               List<IString>("Anti-aliasing filter (IIR)",
                                                             "AAF (zero-phase distortion)",
                                                             "30 kHz",
                                                             "10 kHz",
                                                             "3 kHz",
                                                             "1 khZ",
                                                             "300 Hz",
                                                             "100 Hz",
                                                             "30 Hz",
                                                             "10 Hz"),
                                               0))
                .addProperty(ReferenceProperty(
                    "InputType", EvalValue("switch($Measurement, 0, %VoltageInputType, 3, %TemperatureInputType, 4, %CurrentInputType)")))
                .addProperty(ReferenceProperty(
                    "Excitation",
                    EvalValue("switch($Measurement, 0, %VoltageExcitation, 4, %CurrentExcitation, 5, %PotentiometerExcitation)")))
                .addProperty(ReferenceProperty("ExcitationUnit", EvalValue("switch($Measurement, 0, %VoltageExcitationUnit))")))
                .addProperty(SelectionProperty("Short", List<IString>("On", "Off"), 1))
                .addProperty(FloatProperty("PhysicalScale", 1.0, false))
                .addProperty(FloatProperty("PhysicalOffset", 0.0, false))
                .addProperty(StringProperty("PhysicalUnit", "V", false))
                .addProperty(
                    SelectionPropertyBuilder(
                        "VoltageRange",
                        EvalValue(
                            "[50.0, 10.0, 1.0, 0.1] * if($RangeUnit == 0, 1, $PhysicalScale) + if($RangeUnit == 0, 0, $PhysicalOffset)"),
                        0)
                        .setUnit(EvalValue("if($RangeUnit == 0, Unit('V'), Unit($PhysicalUnit))"))
                        .build())
                .addProperty(SelectionProperty("VoltageInputType", List<IString>("Differential", "Single ended"), 1))
                .addProperty(FloatPropertyBuilder("VoltageExcitation", 6)
                                 .setVisible(EvalValue("$VoltageInputType == 0"))
                                 .setUnit(EvalValue("Unit(%VoltageExcitationUnit:SelectedValue)"))
                                 .setSuggestedValues(EvalValue("If($VoltageExcitationUnit == 0, [20.0, 15.0, 10.0, 5.0, 2.5, 1.0, 0.0], "
                                                               "[60.0, 20.0, 10.0, 5.0, 2.0, 1.0, 0.0])"))
                                 .build())
                .addProperty(SelectionProperty("VoltageExcitationUnit", List<IString>("V", "mA"), 0, EvalValue("$VoltageInputType == 0")))
                .addProperty(
                    SelectionPropertyBuilder(
                        "PotentiometerRange",
                        EvalValue(
                            "[20000.0, 2000.0, 200.0] * if($RangeUnit == 0, 1, $PhysicalScale) + if($RangeUnit == 0, 0, $PhysicalOffset)"),
                        0)
                        .setUnit(EvalValue("if($RangeUnit == 0, Unit('V'), Unit($PhysicalUnit))"))
                        .build())
                .addProperty(FloatPropertyBuilder("PotentiometerExcitation", 6)
                                 .setSuggestedValues(List<Float>(20.0, 15.0, 10.0, 5.0, 2.5, 1.0, 0.0))
                                 .setUnit(Unit("V"))
                                 .build())
                .addProperty(SelectionPropertyBuilder("TemperatureRange", List<IString>("-200 .. 850"), 0)
                                 .setUnit(EvalValue("if($RangeUnit == 0, Unit('C'), Unit($PhysicalUnit))"))
                                 .build())
                .addProperty(SelectionProperty("TemperatureInputType", List<IString>("PT100", "PT200", "PT500", "PT1000", "PT2000"), 0))
                .addProperty(SelectionPropertyBuilder("ResistanceRange",
                                                      EvalValue("[100000.0, 10000.0, 1000.0, 100.0] * if($RangeUnit == 0, 1, "
                                                                "$PhysicalScale) + if($RangeUnit == 0, 0, $PhysicalOffset)"),
                                                      0)
                                 .setUnit(EvalValue("if($RangeUnit == 0, Unit('V'), Unit($PhysicalUnit))"))
                                 .build())
                .addProperty(SelectionPropertyBuilder("BridgeRange", List<Float>(1000.0, 200.0, 20.0, 2.0), 0)
                                 .setUnit(EvalValue("if($RangeUnit == 0, Unit('mV'), Unit($PhysicalUnit))"))
                                 .build())
                .addProperty(SelectionProperty(
                    "BridgeMode", List<IString>("Full", "Half", "Quarter 3-wire", "Quarter 4-wire"), 0, EvalValue("$Measurement == 1")))
                .addProperty(ReferenceProperty(
                    "BridgeResistance",
                    EvalValue("if($BridgeMode == 0 || $BridgeMode == 1, %BridgeCustomResistance, $BridgeFixedResistance)")))
                .addProperty(FloatPropertyBuilder("BridgeCustomResistance", 120.0)
                                 .setVisible(EvalValue("$Measurement == 1"))
                                 .setUnit(Unit("Ohm"))
                                 .setSuggestedValues(List<Float>(120.0, 350.0))
                                 .build())
                .addProperty(SelectionPropertyBuilder("BridgeFixedResistance", List<Float>(120.0, 350.0), 0)
                                 .setVisible( EvalValue("$Measurement == 1"))
                                 .setUnit(Unit("Ohm"))
                                 .build())
                .addProperty(SelectionProperty("BridgeShunt",
                                               List<IString>("Off", "Sns + 100kOhm", "Quarter 3-wire", "Quarter 4-wire"),
                                               0,
                                               EvalValue("$Measurement == 1")))
                .addProperty(FloatPropertyBuilder("BridgeExcitation", 6)
                                 .setSuggestedValues(EvalValue("If($BridgeExcitationUnit == 0, [20.0, 15.0, 10.0, 5.0, 2.5, 1.0, 0.0], "
                                                               "[60.0, 20.0, 10.0, 5.0, 2.0, 1.0, 0.0])"))
                                 .setUnit(EvalValue("Unit(%BridgeExcitationUnit:SelectedValue)"))
                                 .build())
                .addProperty(SelectionProperty("BridgeExcitationUnit", List<IString>("V", "mA"), 0))
                .addProperty(
                    SelectionProperty("Shunt", List<IString>("On", "Off"), 1, EvalValue("($Measurement == 1) && ($BridgeShunt == 1)")))
                .addProperty(FunctionProperty("Balance", ProcedureInfo(), EvalValue("$Measurement == 1")))
                .addProperty(FunctionProperty("Reset", ProcedureInfo(), EvalValue("$Measurement == 1")))
                .addProperty(FloatPropertyBuilder("SensorUnbalance", 0).setVisible(EvalValue("$Measurement == 1")).setUnit(Unit("mv/V")).build())
                .addProperty(SelectionPropertyBuilder("CurrentRange", EvalValue("[1000.0, 200.0, 20.0, 2.0] * $ResistorDivider"), 1)
                                 .setUnit(EvalValue("if($RangeUnit == 0, Unit($ExternalShuntUnit), Unit($PhysicalUnit))"))
                                 .build())
                .addProperty(SelectionProperty("CurrentInputType", List<IString>("Ext. direct shunt", "Ext. loop powered shunt"), 0))
                .addProperty(FloatPropertyBuilder("CurrentExcitation", 6)
                                 .setVisible(EvalValue("$CurrentInputType == 1"))
                                 .setSuggestedValues(List<IFloat>(20.0, 15.0, 10.0, 5.0, 2.5, 1.0, 0.0))
                                 .setUnit(Unit("V"))
                                 .build())
                .addProperty(SelectionProperty("ExternalShunt",
                                               List<IString>("Shunt 1",
                                                             "Shunt 2",
                                                             "Mini-1mR",
                                                             "Mini-500mR",
                                                             "Ato-1mR",
                                                             "Ato-500mR",
                                                             "Maxi-0.2mR",
                                                             "Maxi-500mR",
                                                             "Strip-0.2mR",
                                                             "Strip-500mR",
                                                             "Custom"),
                                               0,
                                               EvalValue("$Measurement == 4")))
                .addProperty(
                    StringProperty("ExternalShuntUnit", EvalValue("if($ExternalShunt == 0 || $ExternalShunt == 1, 'mA', 'A')"), false))
                .addProperty(FloatProperty("ResistorDivider",
                                           EvalValue("switch($ExternalShunt,"
                                                     " 0, 1.0,"
                                                     " 1, 0.5,"
                                                     " 2, 50.0,"
                                                     " 3, 0.1,"
                                                     " 4, 50.0,"
                                                     " 5, 0.1,"
                                                     " 6, 250.0,"
                                                     " 7, 0.1,"
                                                     " 8, 250.0,"
                                                     " 9, 0.1,"
                                                     " 10, 0.0"
                                                     ")"),
                                           false))
                .addProperty(FloatPropertyBuilder("Resistor",
                                                  EvalValue("switch($ExternalShunt,"
                                                            " 0, 50.0,"
                                                            " 1, 0.1,"
                                                            " 2, 0.001,"
                                                            " 3, 0.5,"
                                                            " 4, 0.001,"
                                                            " 5, 0.5,"
                                                            " 6, 0.0002,"
                                                            " 7, 0.5,"
                                                            " 8, 0.0002,"
                                                            " 9, 0.5,"
                                                            " 10, 50.0"
                                                            ")"))
                                 .setVisible( EvalValue("$Measurement == 4"))
                                 .setReadOnly(EvalValue("$ExternalShunt != 10"))
                                 .setUnit(Unit("Ohm"))
                                 .build())
                .addProperty(FloatPropertyBuilder("Pmax",
                                                  EvalValue("switch($ExternalShunt,"
                                                            " 0, 0.25,"
                                                            " 1, 2.5,"
                                                            " 2, 0.4,"
                                                            " 3, 2.0,"
                                                            " 4, 0.9,"
                                                            " 5, 2.0,"
                                                            " 6, 0.32,"
                                                            " 7, 2.0,"
                                                            " 8, 2.0,"
                                                            " 9, 2.0,"
                                                            " 10, 0.125"
                                                            ")"))
                                 .setVisible(EvalValue("$Measurement == 4"))
                                 .setReadOnly(EvalValue("$ExternalShunt != 10"))
                                 .setUnit(Unit("W"))
                                 .build())
                .addProperty(FloatPropertyBuilder("Imax", 0.0)
                                 .setVisible(EvalValue("$Measurement == 4"))
                                 .setReadOnly(true)
                                 .setUnit(EvalValue("Unit($ExternalShuntUnit)"))
                                 .build())
                .build();

        objManager = TypeManager();
        objManager.addType(stgAmplClass);

        // create class with name "LvAmp"
        auto lvAmplClass =
            PropertyObjectClassBuilder("LvAmp")
                .addProperty(StringPropertyBuilder("ModuleName", "XHS LV").setReadOnly(true).build())
                .addProperty(StringPropertyBuilder("SettingRevision", "1,9").setReadOnly(true).build())
                .addProperty(SelectionProperty("Measurement", List<IString>("Voltage", "Bridge", "Current"), 0))
                .addProperty(
                    ReferenceProperty("Range", EvalValue("switch($Measurement, 0, %VoltageRange, 1, %BridgeRange, 2, %CurrentRange)")))
                .addProperty(
                    SelectionPropertyBuilder("VoltageRange", List<Float>(100.0, 50.0, 20.0, 10.0, 5.0, 2.0, 1.0, 0.5, 0.2, 0.1, 0.05), 0)
                        .setUnit(Unit("V"))
                        .build())
                .addProperty(SelectionPropertyBuilder("BridgeRange",
                                                      EvalValue("If($Excitation == 0, [0.0], [100.0, 50.0, 20.0, 10.0, 5.0, 2.0, 1.0, 0.5, "
                                                                "0.2, 0.1, 0.05] * 1000.0 / $Excitation)"),
                                                      0)
                                 .setUnit(Unit("mv/V"))
                                 .build())
                .addProperty(
                    SelectionPropertyBuilder("CurrentRange", List<Float>(100.0, 50.0, 20.0, 10.0, 5.0, 2.0, 1.0, 0.5, 0.2, 0.1, 0.05), 0)
                        .setUnit(Unit("V"))
                        .build())
                .addProperty(SelectionProperty("InputType", List<IString>("SingleEnded", "Differential"), 0))
                .addProperty(ReferenceProperty("Excitation", EvalValue("switch($InputType, 0, %SEExcitation, 1, %DiffExcitation)")))
                .addProperty(FloatPropertyBuilder("SEExcitation", 0.0)
                                 .setUnit(Unit("V"))
                                 .setSuggestedValues(List<Float>(0.0))
                                 .setMinValue(0.0)
                                 .setMaxValue(0.0)
                                 .setReadOnly(true)
                                 .build())
                .addProperty(SelectionProperty("ExcitationType", List<IString>("Unipolar", "Bipolar"), 0, EvalValue("$InputType == 1")))
                .addProperty(ReferenceProperty("DiffExcitation",
                                               EvalValue("switch($ExcitationType, 0, %UnipolarExcitation, 1, %BipolarExcitation)")))
                .addProperty(FloatPropertyBuilder("UnipolarExcitation", 0)
                                 .setSuggestedValues(List<Float>(0.0, 1.0, 2.5, 5.0, 10.0, 12.0, 15.0, 24.0))
                                 .setMinValue(0.0)
                                 .setMaxValue(24.0)
                                 .setUnit(Unit("V"))
                                 .build())
                .addProperty(FloatPropertyBuilder("BipolarExcitation", 2)
                                 .setSuggestedValues(List<Float>(2.0, 2.5, 5.0, 10.0, 12.0, 15.0, 24.0, 30.0))
                                 .setMinValue(0.0)
                                 .setMaxValue(30.0)
                                 .setUnit(Unit("V"))
                                 .build())
                .build();

        objManager.addType(lvAmplClass);
    }

    void TearDown() override
    {
        objManager.removeType("StgAmp");
        objManager.removeType("LvAmp");
    }

    RegisteredPropertyObject registerPropertyObject(const PropertyObjectPtr& prop)
    {
        const auto context = Context(nullptr, logger, TypeManager(), nullptr);
        const auto serverProp =
            std::make_shared<TmsServerPropertyObject>(prop, server, context, std::make_shared<TmsServerContext>(context, nullptr));
        const auto nodeId = serverProp->registerOpcUaNode();
        const auto clientProp = TmsClientPropertyObject(Context(nullptr, logger, TypeManager(), nullptr), clientContext, nodeId);
        return {serverProp, clientProp};
    }
};

TEST_F(TMSAmplifierTest, SetUpServerLv)
{
    const auto obj = PropertyObject(objManager, "LvAmp");

    const auto serverProp = std::make_shared<TmsServerPropertyObject>(obj, server, ctx, serverContext);
    const auto nodeId = serverProp->registerOpcUaNode();
}

TEST_F(TMSAmplifierTest, SetUpServerStg)
{
    const auto obj = PropertyObject(objManager, "StgAmp");

    const auto serverProp = std::make_shared<TmsServerPropertyObject>(obj, server, ctx, serverContext);
    const auto nodeId = serverProp->registerOpcUaNode();
}

TEST_F(TMSAmplifierTest, SetUpLv)
{
    const auto obj = PropertyObject(objManager, "LvAmp");
    auto [serverObj, clientObj] = registerPropertyObject(obj);
}

TEST_F(TMSAmplifierTest, SetUpStg)
{
    const auto obj = PropertyObject(objManager, "StgAmp");
    auto [serverObj, clientObj] = registerPropertyObject(obj);
}

TEST_F(TMSAmplifierTest, TestVisibleProp)
{
    const auto obj = PropertyObject(objManager, "LvAmp");
    auto [serverObj, lvAmpl] = registerPropertyObject(obj);

    auto boundPropInfo = lvAmpl.getProperty("ExcitationType");

    lvAmpl.setPropertyValue("InputType", 0);
    ASSERT_FALSE(lvAmpl.getProperty("ExcitationType").getVisible());
    ASSERT_FALSE(boundPropInfo.getVisible());

    lvAmpl.setPropertyValue("InputType", 1);
    ASSERT_TRUE(lvAmpl.getProperty("ExcitationType").getVisible());
    ASSERT_TRUE(boundPropInfo.getVisible());
}

TEST_F(TMSAmplifierTest, Measurement0)
{
    const auto obj = PropertyObject(objManager, "LvAmp");
    auto [serverObj, lvAmpl] = registerPropertyObject(obj);

    lvAmpl.setPropertyValue("Measurement", 0);

    lvAmpl.setPropertyValue("Range", 0);
    Float rangeValue = lvAmpl.getPropertySelectionValue("Range");
    auto rangeProp = lvAmpl.getProperty("Range");
    ListPtr<Float> rangeValues = rangeProp.getSelectionValues();

    ASSERT_EQ(rangeValues[0], rangeValue);
    
    lvAmpl.setPropertyValue("Measurement", 1);
    rangeValue = lvAmpl.getPropertySelectionValue("Range");
    rangeProp = lvAmpl.getProperty("Range");
    rangeValues = rangeProp.getSelectionValues();

    ASSERT_EQ(rangeValues[0], rangeValue);
}

TEST_F(TMSAmplifierTest, Measurement1)
{
    const auto obj = PropertyObject(objManager, "LvAmp");
    auto [serverObj, lvAmpl] = registerPropertyObject(obj);

    lvAmpl.setPropertyValue("Measurement", 1);
    auto exc = lvAmpl.getPropertyValue("Excitation");
    auto it = lvAmpl.getPropertyValue("InputType");
    auto se = lvAmpl.getPropertyValue("SEExcitation");

    auto a = lvAmpl.getPropertyValue("BridgeRange");
    auto c = lvAmpl.getPropertyValue("Range");

    auto aProp = lvAmpl.getProperty("BridgeRange");
    ListPtr<Int> bridgeValues = aProp.getSelectionValues();

    auto cProp = lvAmpl.getProperty("Range");
    ListPtr<Int> rangeValues = cProp.getSelectionValues();
}

TEST_F(TMSAmplifierTest, Measurement2)
{
    const auto obj = PropertyObject(objManager, "LvAmp");
    auto [serverObj, lvAmpl] = registerPropertyObject(obj);

    lvAmpl.setPropertyValue("Measurement", 2);

    auto a = lvAmpl.getPropertyValue("BridgeRange");
    auto c = lvAmpl.getPropertyValue("Range");

    auto aProp = lvAmpl.getProperty("BridgeRange");
    auto bridgeValues = aProp.getSelectionValues();

    auto cProp = lvAmpl.getProperty("Range");
    auto rangeValues = cProp.getSelectionValues();
}

TEST_F(TMSAmplifierTest, ReadRefPropertyValues)
{
    const auto obj = PropertyObject(objManager, "StgAmp");
    auto [serverObj, stgAmpl] = registerPropertyObject(obj);

    auto rangeProp = stgAmpl.getProperty("Range");
    ListPtr<Float> rangeValues = rangeProp.getSelectionValues();
    ASSERT_EQ(rangeValues.getCount(), 4u);
    ASSERT_EQ(rangeValues.getItemAt(0), 50.0);
}

TEST_F(TMSAmplifierTest, SetAndGetRefProperty)
{
    const auto obj = PropertyObject(objManager, "StgAmp");
    auto [serverObj, stgAmpl] = registerPropertyObject(obj);

    stgAmpl.setPropertyValue("Measurement", 0);

    auto rangeProp = stgAmpl.getProperty("Range");
    ListPtr<Float> rangeValues = rangeProp.getSelectionValues();

    ASSERT_EQ(rangeValues.getCount(), 4u);
    ASSERT_EQ(rangeValues.getItemAt(0), 50.0);

    stgAmpl.setPropertyValue("Range", 1);
    stgAmpl.setPropertyValue("Measurement", 1);

    rangeProp = stgAmpl.getProperty("Range");
    rangeValues = rangeProp.getSelectionValues();

    ASSERT_EQ(rangeValues.getCount(), 4u);
    ASSERT_EQ(rangeValues.getItemAt(0), 1000.0);

    stgAmpl.setPropertyValue("Range", 2);
    stgAmpl.setPropertyValue("Measurement", 0);

    ASSERT_EQ(stgAmpl.getPropertyValue("Range"), 1);
}

TEST_F(TMSAmplifierTest, CheckVoltage)
{
    const auto obj = PropertyObject(objManager, "StgAmp");
    auto [serverObj, stgAmpl] = registerPropertyObject(obj);

    stgAmpl.setPropertyValue("PhysicalUnit", "Pa");

    // check if by default is in single ended input mode
    ASSERT_EQ(stgAmpl.getPropertyValue("InputType"), 1);

    // switch to input type to differential mode
    stgAmpl.setPropertyValue("InputType", 0);
    auto props = stgAmpl.getVisibleProperties();

    // TODO: function properties metadata is not transferred from server
    ASSERT_EQ(props.getCount(), 13u);

    // TODO: props are out of order
    //ASSERT_EQ(props.getItemAt(0).getName(), "Measurement");
    //ASSERT_EQ(props.getItemAt(1).getName(), "DualCore");
    //ASSERT_EQ(props.getItemAt(2).getName(), "Range");
    //ASSERT_EQ(props.getItemAt(3).getName(), "RangeUnit");
    //ASSERT_EQ(props.getItemAt(4).getName(), "LowPassFilter");
    //ASSERT_EQ(props.getItemAt(5).getName(), "InputType");
    //ASSERT_EQ(props.getItemAt(6).getName(), "Excitation");
    //ASSERT_EQ(props.getItemAt(7).getName(), "ExcitationUnit");
    //ASSERT_EQ(props.getItemAt(8).getName(), "Short");

    // check if by default excitation unit is V
    ASSERT_EQ(stgAmpl.getPropertyValue("ExcitationUnit"), 0);

    ASSERT_EQ(stgAmpl.getProperty("Excitation").getUnit(), Unit("V"));

    // check if excitation values correct
    ListPtr<Float> excitationValues = stgAmpl.getProperty("Excitation").getSuggestedValues();
    ASSERT_EQ(excitationValues.getCount(), 7u);
    ASSERT_EQ(excitationValues.getItemAt(0), 20.0);
    ASSERT_EQ(excitationValues.getItemAt(1), 15.0);

    // switch to excitation unit to mA
    stgAmpl.setPropertyValue("ExcitationUnit", 1);
    props = stgAmpl.getVisibleProperties();
    ASSERT_EQ(stgAmpl.getProperty("Excitation").getUnit(), Unit("mA"));

    // check if excitation values correct
    excitationValues = stgAmpl.getProperty("Excitation").getSuggestedValues();
    ASSERT_EQ(excitationValues.getCount(), 7u);
    ASSERT_EQ(excitationValues.getItemAt(0), 60.0);
    ASSERT_EQ(excitationValues.getItemAt(1), 20.0);

    // switch to single ended input type
    // TODO: Clear not supported
    //stgAmpl.clearPropertyValue("InputType");
    stgAmpl.setPropertyValue("InputType", 1);
    props = stgAmpl.getVisibleProperties();

    // TODO: function properties metadata is not transferred from server
    ASSERT_EQ(props.getCount(), 11u);
    
    // TODO: props are out of order
    //ASSERT_EQ(props.getItemAt(0).getName(), "Measurement");
    //ASSERT_EQ(props.getItemAt(1).getName(), "DualCore");
    //ASSERT_EQ(props.getItemAt(2).getName(), "Range");
    //ASSERT_EQ(props.getItemAt(3).getName(), "RangeUnit");
    //ASSERT_EQ(props.getItemAt(4).getName(), "LowPassFilter");
    //ASSERT_EQ(props.getItemAt(5).getName(), "InputType");
    //ASSERT_EQ(props.getItemAt(6).getName(), "Short");

    // check voltage range
    ASSERT_EQ(stgAmpl.getPropertyValue("RangeUnit"), 0);
    ASSERT_EQ(stgAmpl.getProperty("Range").getUnit(), Unit("V"));
    ASSERT_EQ(stgAmpl.getProperty("Range").getSelectionValues().asPtr<IList>().getItemAt(0), 50.0);

    stgAmpl.setPropertyValue("PhysicalScale", 2.0);
    stgAmpl.setPropertyValue("PhysicalOffset", 10.0);

    // change to scaled range unit
    stgAmpl.setPropertyValue("RangeUnit", 1);
    props = stgAmpl.getVisibleProperties();
    ASSERT_EQ(stgAmpl.getProperty("Range").getUnit(), Unit("Pa"));
    ASSERT_EQ(stgAmpl.getProperty("Range").getSelectionValues().asPtr<IList>().getItemAt(0), 110.0);
}

TEST_F(TMSAmplifierTest, CheckPotentiometer)
{
    const auto obj = PropertyObject(objManager, "StgAmp");
    auto [serverObj, stgAmpl] = registerPropertyObject(obj);

    stgAmpl.setPropertyValue("PhysicalUnit", "Pa");

    stgAmpl.setPropertyValue("Measurement", 5);

    auto props = stgAmpl.getVisibleProperties();
    // TODO: function properties metadata is not transferred from server
    ASSERT_EQ(props.getCount(), 11u);

    // TODO: props are out of order
    //ASSERT_EQ(props.getItemAt(0).getName(), "Measurement");
    //ASSERT_EQ(props.getItemAt(1).getName(), "DualCore");
    //ASSERT_EQ(props.getItemAt(2).getName(), "Range");
    //ASSERT_EQ(props.getItemAt(3).getName(), "RangeUnit");
    //ASSERT_EQ(props.getItemAt(4).getName(), "LowPassFilter");
    //ASSERT_EQ(props.getItemAt(5).getName(), "Excitation");
    //ASSERT_EQ(props.getItemAt(6).getName(), "Short");

    // check if by default excitation unit is V
    ASSERT_EQ(stgAmpl.getProperty("Excitation").getUnit(), Unit("V"));

    // check if excitation values correct
    ListPtr<Float> excitationValues = stgAmpl.getProperty("Excitation").getSuggestedValues();
    ASSERT_EQ(excitationValues.getCount(), 7u);
    ASSERT_EQ(excitationValues.getItemAt(0), 20.0);
    ASSERT_EQ(excitationValues.getItemAt(1), 15.0);

    // check potentiometer range
    ASSERT_EQ(stgAmpl.getPropertyValue("RangeUnit"), 0);
    ASSERT_EQ(stgAmpl.getProperty("Range").getUnit(), Unit("V"));
    ASSERT_EQ(stgAmpl.getProperty("Range").getSelectionValues().asPtr<IList>().getItemAt(0), 20000.0);

    stgAmpl.setPropertyValue("PhysicalScale", 2.0);
    stgAmpl.setPropertyValue("PhysicalOffset", 10.0);

    // change to scaled range unit
    stgAmpl.setPropertyValue("RangeUnit", 1);
    props = stgAmpl.getVisibleProperties();
    ASSERT_EQ(stgAmpl.getProperty("Range").getUnit(), Unit("Pa"));
    ASSERT_EQ(stgAmpl.getProperty("Range").getSelectionValues().asPtr<IList>().getItemAt(0), 40010.0);
}

TEST_F(TMSAmplifierTest, CheckResistance)
{
    const auto obj = PropertyObject(objManager, "StgAmp");
    auto [serverObj, stgAmpl] = registerPropertyObject(obj);

    stgAmpl.setPropertyValue("PhysicalUnit", "Pa");

    stgAmpl.setPropertyValue("Measurement", 2);

    auto props = stgAmpl.getVisibleProperties();
    // TODO: function properties metadata is not transferred from server
    ASSERT_EQ(props.getCount(), 10u);

    // TODO: props are out of order
    //ASSERT_EQ(props.getItemAt(0).getName(), "Measurement");
    //ASSERT_EQ(props.getItemAt(1).getName(), "DualCore");
    //ASSERT_EQ(props.getItemAt(2).getName(), "Range");
    //ASSERT_EQ(props.getItemAt(3).getName(), "RangeUnit");
    //ASSERT_EQ(props.getItemAt(4).getName(), "LowPassFilter");
    //ASSERT_EQ(props.getItemAt(5).getName(), "Short");

    // check resistance range
    ASSERT_EQ(stgAmpl.getPropertyValue("RangeUnit"), 0);
    ASSERT_EQ(stgAmpl.getProperty("Range").getUnit(), Unit("V"));
    ASSERT_EQ(stgAmpl.getProperty("Range").getSelectionValues().asPtr<IList>().getItemAt(0), 100000.0);

    stgAmpl.setPropertyValue("PhysicalScale", 2.0);
    stgAmpl.setPropertyValue("PhysicalOffset", 10.0);

    // change to scaled range unit
    stgAmpl.setPropertyValue("RangeUnit", 1);
    ASSERT_EQ(stgAmpl.getProperty("Range").getUnit(), Unit("Pa"));
    ASSERT_EQ(stgAmpl.getProperty("Range").getSelectionValues().asPtr<IList>().getItemAt(0), 200010.0);
}

TEST_F(TMSAmplifierTest, CheckTemperature)
{
    const auto obj = PropertyObject(objManager, "StgAmp");
    auto [serverObj, stgAmpl] = registerPropertyObject(obj);

    stgAmpl.setPropertyValue("PhysicalUnit", "F");

    stgAmpl.setPropertyValue("Measurement", 3);

    auto props = stgAmpl.getVisibleProperties();
    // TODO: function properties metadata is not transferred from server
    ASSERT_EQ(props.getCount(), 11u);

    // TODO: props are out of order
    //ASSERT_EQ(props.getItemAt(0).getName(), "Measurement");
    //ASSERT_EQ(props.getItemAt(1).getName(), "DualCore");
    //ASSERT_EQ(props.getItemAt(2).getName(), "Range");
    //ASSERT_EQ(props.getItemAt(3).getName(), "RangeUnit");
    //ASSERT_EQ(props.getItemAt(4).getName(), "LowPassFilter");
    //ASSERT_EQ(props.getItemAt(5).getName(), "InputType");
    //ASSERT_EQ(props.getItemAt(6).getName(), "Short");

    // check temperature range
    ASSERT_EQ(stgAmpl.getPropertyValue("RangeUnit"), 0);
    ASSERT_EQ(stgAmpl.getProperty("Range").getUnit(), Unit("C"));
    ASSERT_EQ(stgAmpl.getProperty("Range").getSelectionValues().asPtr<IList>().getItemAt(0), "-200 .. 850");

    stgAmpl.setPropertyValue("PhysicalScale", 2.0);
    stgAmpl.setPropertyValue("PhysicalOffset", 10.0);

    // change to scaled range unit
    stgAmpl.setPropertyValue("RangeUnit", 1);
    props = stgAmpl.getVisibleProperties();
    ASSERT_EQ(stgAmpl.getProperty("Range").getUnit(), Unit("F"));
    // ASSERT_EQ(props.getItemAt(2).getSelectionValues().getItemAt(0), 200010.0);
}

TEST_F(TMSAmplifierTest, CheckCurrent)
{
    const auto obj = PropertyObject(objManager, "StgAmp");
    auto [serverObj, stgAmpl] = registerPropertyObject(obj);

    stgAmpl.setPropertyValue("PhysicalUnit", "Pa");

    // switch to current mode
    stgAmpl.setPropertyValue("Measurement", 4);
    auto allProps = stgAmpl.getAllProperties();
    auto props = stgAmpl.getVisibleProperties();

    // TODO: function properties metadata is not transferred from server
    ASSERT_EQ(props.getCount(), 15u);

    // TODO: props are out of order
    //ASSERT_EQ(props.getItemAt(0).getName(), "Measurement");
    //ASSERT_EQ(props.getItemAt(1).getName(), "DualCore");
    //ASSERT_EQ(props.getItemAt(2).getName(), "Range");
    //ASSERT_EQ(props.getItemAt(3).getName(), "RangeUnit");
    //ASSERT_EQ(props.getItemAt(4).getName(), "LowPassFilter");
    //ASSERT_EQ(props.getItemAt(5).getName(), "InputType");
    //ASSERT_EQ(props.getItemAt(6).getName(), "Short");
    //ASSERT_EQ(props.getItemAt(7).getName(), "BridgeExcitation");
    //ASSERT_EQ(props.getItemAt(8).getName(), "BridgeExcitationUnit");
    //ASSERT_EQ(props.getItemAt(9).getName(), "ExternalShunt");
    //ASSERT_EQ(props.getItemAt(10).getName(), "Resistor");
    //ASSERT_EQ(props.getItemAt(11).getName(), "Pmax");
    //ASSERT_EQ(props.getItemAt(12).getName(), "Imax");

    // check range
    auto currRange = stgAmpl.getProperty("CurrentRange");
    auto rangeProp = stgAmpl.getProperty("Range");
    auto item = rangeProp.getSelectionValues().asPtr<IList>().getItemAt(0);
    ASSERT_EQ(rangeProp.getSelectionValues().asPtr<IList>().getItemAt(0), 1000.0);

    ASSERT_EQ(rangeProp.getUnit(), Unit("mA"));

    auto iMaxProp = stgAmpl.getProperty("Imax");
    ASSERT_EQ(iMaxProp.getUnit(), Unit("mA"));

    stgAmpl.setPropertyValue("ExternalShunt", 1);

    // check range
    ASSERT_EQ(rangeProp.getSelectionValues().asPtr<IList>().getItemAt(0), 500.0);

    // check shunt values
    ASSERT_EQ(stgAmpl.getPropertyValue("Resistor"), 0.1);
    ASSERT_EQ(stgAmpl.getPropertyValue("Pmax"), 2.5);

    stgAmpl.setPropertyValue("InputType", 1);
    props = stgAmpl.getVisibleProperties();

    // TODO: function properties metadata is not transferred from server
    ASSERT_EQ(props.getCount(), 16u);

    // TODO: props are out of order
    //ASSERT_EQ(props.getItemAt(0).getName(), "Measurement");
    //ASSERT_EQ(props.getItemAt(1).getName(), "DualCore");
    //ASSERT_EQ(props.getItemAt(2).getName(), "Range");
    //ASSERT_EQ(props.getItemAt(3).getName(), "RangeUnit");
    //ASSERT_EQ(props.getItemAt(4).getName(), "LowPassFilter");
    //ASSERT_EQ(props.getItemAt(5).getName(), "InputType");
    //ASSERT_EQ(props.getItemAt(6).getName(), "Excitation");
    //ASSERT_EQ(props.getItemAt(7).getName(), "Short");
    //ASSERT_EQ(props.getItemAt(8).getName(), "BridgeExcitation");
    //ASSERT_EQ(props.getItemAt(9).getName(), "BridgeExcitationUnit");
    //ASSERT_EQ(props.getItemAt(10).getName(), "ExternalShunt");
    //ASSERT_EQ(props.getItemAt(11).getName(), "Resistor");
    //ASSERT_EQ(props.getItemAt(12).getName(), "Pmax");
    //ASSERT_EQ(props.getItemAt(13).getName(), "Imax");

    // set custom shunt
    stgAmpl.setPropertyValue("ExternalShunt", 10);

    // check shunt values
    ASSERT_EQ(stgAmpl.getPropertyValue("Resistor"), 50.0);
    ASSERT_EQ(stgAmpl.getPropertyValue("Pmax"), 0.125);
}


TEST_F(TMSAmplifierTest, GetRefPropAfterChange)
{
    const auto obj = PropertyObject(objManager, "StgAmp");
    auto [serverObj, stgAmpl] = registerPropertyObject(obj);

    auto rangeProp = stgAmpl.getProperty("Range");

    auto voltageRangeProp = stgAmpl.getProperty("VoltageRange");
    stgAmpl.setPropertyValue("Measurement", 0);
    ASSERT_EQ(rangeProp.getReferencedProperty().getName(), voltageRangeProp.getName());

    auto bridgeRangeProp = stgAmpl.getProperty("BridgeRange");
    stgAmpl.setPropertyValue("Measurement", 1);
    ASSERT_EQ(rangeProp.getReferencedProperty().getName(), bridgeRangeProp.getName());
}

TEST_F(TMSAmplifierTest, GetRefPropSelectionValuesAfterChange)
{
    const auto obj = PropertyObject(objManager, "StgAmp");
    auto [serverObj, stgAmpl] = registerPropertyObject(obj);

    auto rangeProp = stgAmpl.getProperty("Range");

    auto voltageRangeValues = stgAmpl.getProperty("VoltageRange").getSelectionValues();
    stgAmpl.setPropertyValue("Measurement", 0);
    ASSERT_EQ(rangeProp.getSelectionValues(), voltageRangeValues);

    auto bridgeRangeValues = stgAmpl.getProperty("BridgeRange").getSelectionValues();
    stgAmpl.setPropertyValue("Measurement", 1);
    ASSERT_EQ(rangeProp.getSelectionValues(), bridgeRangeValues);
}

TEST_F(TMSAmplifierTest, PropertyOrder1)
{
    const auto obj = PropertyObject(objManager, "StgAmp");
    auto [serverObj, stgAmpl] = registerPropertyObject(obj);

    auto serverProps = obj.getAllProperties();
    auto clientProps = stgAmpl.getAllProperties();

    ASSERT_EQ(serverProps.getCount(), clientProps.getCount());

    for (SizeT i = 0; i < serverProps.getCount(); ++i)
        ASSERT_EQ(serverProps[i].getName(), clientProps[i].getName());
}

TEST_F(TMSAmplifierTest, PropertyOrder2)
{
    const auto obj = PropertyObject(objManager, "LvAmp");
    auto [serverObj, lvAmpl] = registerPropertyObject(obj);

    auto serverProps = obj.getAllProperties();
    auto clientProps = lvAmpl.getAllProperties();

    ASSERT_EQ(serverProps.getCount(), clientProps.getCount());

    for (SizeT i = 0; i < serverProps.getCount(); ++i)
        ASSERT_EQ(serverProps[i].getName(), clientProps[i].getName());
}
