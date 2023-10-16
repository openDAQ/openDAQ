unit Test.OpenDAQ.Float;

interface
uses
  DunitX.TestFramework, DS.UT.DSUnitTestUnit, OpenDAQ.CoreTypes;

type
  [TestFixture]
  TTest_Floating = class(TDSUnitTest)
  private
  public
    [Setup]
    procedure Setup; override;
    [TearDown]
    procedure TearDown; override;
    [Test]
    procedure Basic;
    [Test]
    procedure Equality;
    [Test]
    procedure Hashing;
    [Test]
    procedure CastString;
    [Test]
    procedure CastInt;
    [Test]
    procedure CastFloat;
    [Test]
    procedure CastBool;
    [Test]
    procedure CoreType;
  end;

implementation
uses
  DS.UT.DSUnitTestEngineUnit, WinApi.Windows, OpenDAQ.CoreTypes.Errors;

{ TTest_Floating }

procedure TTest_Floating.Basic;
var
  FloatObj: IFloat;
  FloatVal: RtFloat;
  ReferenceValue: RtFloat;

begin
  CreateFloat(FloatObj, 5.5);
  Assert.IsTrue(OPENDAQ_SUCCEEDED(FloatObj.GetValue(FloatVal)));
  ReferenceValue := 5.5;
  Assert.AreEqual(FloatVal, ReferenceValue);
end;

procedure TTest_Floating.Equality;
var
  FloatObj1, FloatObj2, FloatObj3: IFloat;
  ReferenceValue: RtFloat;
  Eq: Boolean;
begin
  CreateFloat(FloatObj1, 3.0);
  CreateFloat(FloatObj2, 3.0);
  CreateFloat(FloatObj3, 4.0);

  Eq := False;
  FloatObj1.EqualsObject(FloatObj1, Eq);
  Assert.IsTrue(Eq);

  FloatObj1.EqualsObject(FloatObj2, Eq);
  Assert.IsTrue(Eq);

  FloatObj1.EqualsObject(FloatObj3, Eq);
  Assert.IsFalse(Eq);

  ReferenceValue := 3.0;
  FloatObj1.EqualsValue(ReferenceValue, Eq);
  Assert.IsTrue(Eq);
end;

procedure TTest_Floating.Hashing;
var
  FloatObj1, FloatObj2: IFloat;
  HashCode1, HashCode2: SizeT;
begin
  CreateFloat(FloatObj1, 3.0);
  CreateFloat(FloatObj2, 4.0);

  FloatObj1.GetHashCodeEx(HashCode1);
  Assert.AreNotEqual(HashCode1, Nativeint(0));
  FloatObj2.GetHashCodeEx(HashCode2);
  Assert.AreNotEqual(HashCode2, Nativeint(0));
  Assert.AreNotEqual(HashCode1, HashCode2);
end;

procedure TTest_Floating.CastString;
var
  FloatObj: IFloat;
  Str: PAnsiChar;
begin
  CreateFloat(FloatObj, 1);
  FloatObj.ToCharPtr(@Str);
  Assert.AreEqual(string(Str), '1');
  DaqFreeMemory(Str);
end;

procedure TTest_Floating.CastInt;
var
  FloatObj: IFloat;
  Int: RtInt;
begin
  CreateFloat(FloatObj, 1);

  Int := BaseObjectToInt(FloatObj);
  Assert.IsTrue(Int = 1);
end;

procedure TTest_Floating.CastFloat;
var
  FloatObj: IFloat;
  Float: RtFloat;
begin
  CreateFloat(FloatObj, 1);

  Float := BaseObjectToFloat(FloatObj);
  Assert.AreEqual(1.0, Float, 1e-7);
end;

procedure TTest_Floating.CastBool;
var
  FloatObj1, FloatObj2: IFloat;
  ByteBoolVar: Boolean;
begin
  CreateFloat(FloatObj1, 1);
  CreateFloat(FloatObj2, 0);

  ByteBoolVar := BaseObjectToBool(FloatObj1);
  Assert.AreEqual(ByteBoolVar, True);

  ByteBoolVar := BaseObjectToBool(FloatObj2);
  Assert.AreEqual(ByteBoolVar, False);
end;

procedure TTest_Floating.CoreType;
var
  FloatObj: IFloat;
  CoreType: TCoreType;
begin
  CreateFloat(FloatObj, 1);
  CoreType := GetCoreType(FloatObj);
  Assert.AreEqual(CoreType, ctFloat);
end;

procedure TTest_Floating.Setup;
begin

end;

procedure TTest_Floating.TearDown;
begin

end;

initialization
  TDSUnitTestEngine.RegisterUnitTest(TTest_Floating);

end.
