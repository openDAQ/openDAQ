unit Test.OpenDAQ.Boolean;

interface
uses
  DunitX.TestFramework, DS.UT.DSUnitTestUnit, OpenDAQ.CoreTypes;

type
  [TestFixture]
  TTest_Boolean = class(TDSUnitTest)
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

{ TTest_Boolean }

procedure TTest_Boolean.Basic;
var
  BoolObj: IBoolean;
  BoolVal: Boolean;
begin
  CreateBoolean(BoolObj, True);

  Assert.IsTrue(OPENDAQ_SUCCEEDED(BoolObj.GetValue(BoolVal)));
  Assert.AreEqual(BoolVal, True);
end;

procedure TTest_Boolean.Equality;
var
  BoolObj1, BoolObj2, BoolObj3: IBoolean;
  Eq: Boolean;
begin
  CreateBoolean(BoolObj1, True);
  CreateBoolean(BoolObj2, True);
  CreateBoolean(BoolObj3, False);

  Eq := False;
  BoolObj1.EqualsObject(BoolObj1, Eq);
  Assert.IsTrue(Eq);

  BoolObj1.EqualsObject(BoolObj2, Eq);
  Assert.IsTrue(Eq);

  BoolObj1.EqualsObject(BoolObj3, Eq);
  Assert.IsFalse(Eq);

  BoolObj1.EqualsValue(True, Eq);
  Assert.IsTrue(Eq);
end;

procedure TTest_Boolean.Hashing;
var
  BoolObj1, BoolObj2: IBoolean;
  HashCode1, HashCode2: NativeInt;
begin
  CreateBoolean(BoolObj1, True);
  CreateBoolean(BoolObj2, False);

  BoolObj1.GetHashCodeEx(HashCode1);
  Assert.AreNotEqual(HashCode1, Nativeint(0));
  BoolObj2.GetHashCodeEx(HashCode2);
  Assert.AreEqual(HashCode2, Nativeint(0));
  Assert.AreNotEqual(HashCode1, HashCode2);
end;

procedure TTest_Boolean.CastString;
var
  BoolObj: IBoolean;
  Str: PAnsiChar;
begin
  CreateBoolean(BoolObj, True);
  BoolObj.ToCharPtr(@Str);
  Assert.AreEqual(string(Str), 'True');
  DaqFreeMemory(Str);
end;

procedure TTest_Boolean.CastInt;
var
  BoolObj: IBoolean;
  Int: Int64;
begin
  CreateBoolean(BoolObj, True);

  Int := BaseObjectToInt(BoolObj);
  Assert.IsTrue(Int = 1);
end;

procedure TTest_Boolean.CastFloat;
var
  BoolObj: IBoolean;
  Float: Double;
begin
  CreateBoolean(BoolObj, True);

  Float := BaseObjectToFloat(BoolObj);
  Assert.AreEqual(1.0, Float, 1e-7);
end;

procedure TTest_Boolean.CastBool;
var
  BoolObj: IBoolean;
  ByteBoolVar: ByteBool;
begin
  CreateBoolean(BoolObj, True);

  ByteBoolVar := BaseObjectToBool(BoolObj);
  Assert.AreEqual(ByteBoolVar, True);
end;

procedure TTest_Boolean.CoreType;
var
  BoolObj: IBoolean;
  CoreType: TCoreType;
begin
  CreateBoolean(BoolObj, True);
  CoreType := GetCoreType(BoolObj);
  Assert.AreEqual(CoreType, ctBool);
end;

procedure TTest_Boolean.Setup;
begin

end;

procedure TTest_Boolean.TearDown;
begin

end;

initialization
  TDSUnitTestEngine.RegisterUnitTest(TTest_Boolean);

end.
