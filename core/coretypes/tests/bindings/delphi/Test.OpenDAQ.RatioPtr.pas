unit Test.OpenDAQ.RatioPtr;

interface

uses
  DunitX.TestFramework, DS.UT.DSUnitTestUnit, OpenDAQ.CoreTypes;

type
  [TestFixture]
  TTest_RatioPtr = class(TDSUnitTest)
  private
  public
    [Setup]
    procedure Setup; override;
    [TearDown]
    procedure TearDown; override;
    [Test]
    procedure CreateNull;
    [Test]
    procedure CreateTest;
    [Test]
    procedure CreateInvalid;
    [Test]
    procedure GetNumerator;
    [Test]
    procedure GetDenominator;
    [Test]
    procedure ConvertFloatPtr;
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
  OpenDAQ.CoreTypes.Errors,
  OpenDAQ.Exceptions,
  OpenDAQ.Convertible;

{ TTest_BaseObject }

procedure TTest_RatioPtr.Setup;
begin
end;

procedure TTest_RatioPtr.TearDown();
begin
end;

procedure TTest_RatioPtr.CreateTest();
var
  RatioObj: IRatioPtr;
begin
  Assert.WillNotRaiseAny(procedure()
    begin
      RatioObj := TRatioPtr.Create(0, 1);
    end
  );
end;

procedure TTest_RatioPtr.CreateNull();
var
  RatioObj: IRatioPtr;
  Obj: IBaseObject;
begin
  RatioObj := TRatioPtr.Create(Obj);
  Assert.IsFalse(RatioObj.IsAssigned);
end;

procedure TTest_RatioPtr.CreateInvalid();
var
  RatioObj: IRatioPtr;
begin
  Assert.WillRaise(procedure()
    begin
      RatioObj := TRatioPtr.Create(1, 0);
    end,
    ERTInvalidParameterException
  );
end;

procedure TTest_RatioPtr.GetNumerator();
var
  RatioObj: IRatioPtr;
begin
  RatioObj := TRatioPtr.Create(1, 2);
  Assert.AreEqual<RtInt>(RatioObj.GetNumerator(), 1);
end;

procedure TTest_RatioPtr.GetDenominator();
var
  RatioObj: IRatioPtr;
begin
  RatioObj := TRatioPtr.Create(1, 2);
  Assert.AreEqual<RtInt>(RatioObj.GetDenominator(), 2);
end;

procedure TTest_RatioPtr.ConvertFloatPtr();
var
  RatioObj: IRatioPtr;
  FloatVal : RtFloat;
begin
  RatioObj := TRatioPtr.Create(1, 2);
  FloatVal := (RatioObj as IConvertiblePtr).ToFloat();

  Assert.AreEqual<RtFloat>(FloatVal, 1 / 2.0);
end;

procedure TTest_RatioPtr.ConvertFloat();
var
  RatioObj: IRatioPtr;
  FloatVal : RtFloat;
begin
  RatioObj := TRatioPtr.Create(1, 2);
  (RatioObj as IConvertible).ToFloat(FloatVal);

  Assert.AreEqual<RtFloat>(FloatVal, 1 / 2.0);
end;

procedure TTest_RatioPtr.ConvertInt();
var
  RatioObj: IRatioPtr;
  IntVal : RtInt;
begin
  RatioObj := TRatioPtr.Create(1, 2);
  IntVal := (RatioObj as IConvertiblePtr).ToInt();

  Assert.AreEqual<RtInt>(IntVal, 1);
end;

procedure TTest_RatioPtr.ConvertBoolTrue();
var
  RatioObj: IRatioPtr;
  BoolVal : Boolean;
begin
  RatioObj := TRatioPtr.Create(1, 2);
  BoolVal := (RatioObj as IConvertiblePtr).ToBool();

  Assert.IsTrue(BoolVal);
end;

procedure TTest_RatioPtr.ConvertBoolFalse();
var
  RatioObj: IRatioPtr;
  BoolVal : Boolean;
begin
  RatioObj := TRatioPtr.Create(0, 2);
  BoolVal := (RatioObj as IConvertiblePtr).ToBool();

  Assert.IsFalse(BoolVal);
end;

procedure TTest_RatioPtr.Serialize();
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

procedure TTest_RatioPtr.Deserialize();
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

procedure TTest_RatioPtr.CoreType();
var
  RatioObj: IRatioPtr;
  CoreTypeVal : TCoreType;
begin
  RatioObj := TRatioPtr.Create(1, 1);
  CoreTypeVal := GetCoreType(RatioObj);

  Assert.AreEqual(CoreTypeVal, ctRatio);
end;

initialization
  TDSUnitTestEngine.RegisterUnitTest(TTest_RatioPtr);

end.
