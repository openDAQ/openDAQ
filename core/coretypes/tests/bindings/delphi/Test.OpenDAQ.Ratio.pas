unit Test.OpenDAQ.Ratio;

interface

uses
  DunitX.TestFramework, DS.UT.DSUnitTestUnit, OpenDAQ.CoreTypes;

type
  [TestFixture]
  TTest_Ratio = class(TDSUnitTest)
  private
  public
    [Setup]
    procedure Setup; override;
    [TearDown]
    procedure TearDown; override;
    [Test]
    procedure CreateTest;
    [Test]
    procedure CreateInvalid;
    [Test]
    procedure GetNumerator;
    [Test]
    procedure GetDenominator;
    [Test]
    procedure ConvertFloat;
    [Test]
    procedure ConvertInt;
    [Test]
    procedure ConvertBoolTrue;
    [Test]
    procedure ConvertBoolFalse;
    [Test]
    procedure Serialize;
    [Test]
    procedure Deserialize;
    [Test]
    procedure CoreType;
  end;

implementation

uses
  WinApi.Windows,
  System.SysUtils,
  OpenDAQ.Ratio,
  DS.UT.DSUnitTestEngineUnit,
  OpenDAQ.CoreTypes.Errors;

{ TTest_BaseObject }

procedure TTest_Ratio.Setup;
begin
end;

procedure TTest_Ratio.TearDown();
begin
end;

procedure TTest_Ratio.CreateTest();
var
  RatioObj: IRatio;
  Err : ErrCode;
begin
  Err := CreateRatio(RatioObj, 0, 1);
  Assert.AreEqual(Err, OPENDAQ_SUCCESS);
end;

procedure TTest_Ratio.CreateInvalid();
var
  RatioObj: IRatio;
  Err : ErrCode;
begin
  Err := CreateRatio(RatioObj, 1, 0);
  Assert.AreEqual(Err, OPENDAQ_ERR_INVALIDPARAMETER);
end;

procedure TTest_Ratio.GetNumerator();
var
  RatioObj: IRatio;
  Err : ErrCode;
  Numerator : RtInt;
begin
  CreateRatio(RatioObj, 1, 2);

  Err := RatioObj.GetNumerator(Numerator);
  Assert.AreEqual(Err, OPENDAQ_SUCCESS);
  Assert.AreEqual<RtInt>(Numerator, 1);
end;

procedure TTest_Ratio.GetDenominator();
var
  RatioObj: IRatio;
  Err : ErrCode;
  Denominator : RtInt;
begin
  CreateRatio(RatioObj, 1, 2);

  Err := RatioObj.GetDenominator(Denominator);
  Assert.AreEqual(Err, OPENDAQ_SUCCESS);
  Assert.AreEqual<RtInt>(Denominator, 2);
end;

procedure TTest_Ratio.ConvertFloat();
var
  RatioObj: IRatio;
  FloatVal : RtFloat;
begin
  CreateRatio(RatioObj, 1, 2);
  FloatVal := BaseObjectToFloat(RatioObj);

  Assert.AreEqual<RtFloat>(FloatVal, 1 / 2.0);
end;

procedure TTest_Ratio.ConvertInt();
var
  RatioObj: IRatio;
  IntVal : RtInt;
begin
  CreateRatio(RatioObj, 1, 2);
  IntVal := BaseObjectToInt(RatioObj);

  Assert.AreEqual<RtInt>(IntVal, 1);
end;

procedure TTest_Ratio.ConvertBoolTrue();
var
  RatioObj: IRatio;
  BoolVal : Boolean;
begin
  CreateRatio(RatioObj, 1, 2);
  BoolVal := BaseObjectToBool(RatioObj);

  Assert.IsTrue(BoolVal);
end;

procedure TTest_Ratio.ConvertBoolFalse();
var
  RatioObj: IRatio;
  BoolVal : Boolean;
begin
  CreateRatio(RatioObj, 0, 2);
  BoolVal := BaseObjectToBool(RatioObj);

  Assert.IsFalse(BoolVal);
end;

procedure TTest_Ratio.Serialize();
var
  Serializer : ISerializer;
  RatioObj : IRatio;
  Err : ErrCode;

  OutputObj : IString;
  OutputStr : string;
  ExpectedStr : string;

  SerializeId : PAnsiChar;
  Serializable : ISerializable;
begin
  CreateRatio(RatioObj, 1, 2);
  Err := CreateJsonSerializer(Serializer);
  Assert.AreEqual(Err, OPENDAQ_SUCCESS);

  Serializable := GetSerializableInterface(RatioObj);
  Err := Serializable.GetSerializeId(@SerializeId);
  Assert.AreEqual(Err, OPENDAQ_SUCCESS);

  ExpectedStr := '{"__type":"' + string(SerializeId) + '","num":1,"den":2}';

  Err := Serializable.Serialize(Serializer);
  Assert.AreEqual(Err, OPENDAQ_SUCCESS);

  Err := Serializer.GetOutput(OutputObj);
  Assert.AreEqual(Err, OPENDAQ_SUCCESS);

  OutputStr := RtToString(OutputObj);
  Assert.AreEqual(OutputStr, ExpectedStr);
end;

procedure TTest_Ratio.Deserialize();
var
  Serializer : ISerializer;
  Deserializer : IDeserializer;
  RatioObj : IRatio;
  Err : ErrCode;

  OutputObj : IString;
  Serializable : ISerializable;

  DeserializedObj : IBaseObject;
  DeserializedRatio : IRatio;
  Numerator : RtInt;
  Denominator : RtInt;
begin
  CreateRatio(RatioObj, 1, 2);
  CreateJsonSerializer(Serializer);

  Serializable := GetSerializableInterface(RatioObj);
  Serializable.Serialize(Serializer);
  Serializer.GetOutput(OutputObj);

  Err := CreateJsonDeserializer(Deserializer);
  Assert.AreEqual(Err, OPENDAQ_SUCCESS);

  Err := Deserializer.Deserialize(OutputObj, nil, DeserializedObj);
  Assert.AreEqual(Err, OPENDAQ_SUCCESS);
  Assert.AreNotEqual<IBaseObject>(DeserializedObj, nil);

  Assert.WillNotRaiseAny(procedure()
    begin
      DeserializedRatio := DeserializedObj as IRatio;
    end
  );

  DeserializedRatio.GetNumerator(Numerator);
  Assert.AreEqual<RtInt>(Numerator, 1);

  DeserializedRatio.GetDenominator(Denominator);
  Assert.AreEqual<RtInt>(Denominator, 2);
end;

procedure TTest_Ratio.CoreType();
var
  RatioObj: IRatio;
  CoreTypeVal : TCoreType;
begin
  CreateRatio(RatioObj, 1, 1);
  CoreTypeVal := GetCoreType(RatioObj);

  Assert.AreEqual(CoreTypeVal, ctRatio);
end;

initialization
  TDSUnitTestEngine.RegisterUnitTest(TTest_Ratio);

end.
