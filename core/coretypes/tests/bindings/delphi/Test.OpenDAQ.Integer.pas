unit Test.OpenDAQ.Integer;

interface
uses
  DunitX.TestFramework, DS.UT.DSUnitTestUnit, OpenDAQ.CoreTypes;

type
  [TestFixture]
  TTest_Integer = class(TDSUnitTest)
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

{ TTest_Integer }

procedure TTest_Integer.Basic;
var
  IntObj: IInteger;
  IntVal: RtInt;
begin
  CreateInteger(IntObj, 5);

  Assert.IsTrue(OPENDAQ_SUCCEEDED(IntObj.GetValue(IntVal)));
  Assert.IsTrue(IntVal = 5);
end;

procedure TTest_Integer.Equality;
var
  IntObj1, IntObj2, IntObj3: IInteger;
  Eq: Boolean;
begin
  CreateInteger(IntObj1, 3);
  CreateInteger(IntObj2, 3);
  CreateInteger(IntObj3, 4);

  Eq := False;
  IntObj1.EqualsObject(IntObj1, Eq);
  Assert.IsTrue(Eq);

  IntObj1.EqualsObject(IntObj2, Eq);
  Assert.IsTrue(Eq);

  IntObj1.EqualsObject(IntObj3, Eq);
  Assert.IsFalse(Eq);

  IntObj1.EqualsValue(3, Eq);
  Assert.IsTrue(Eq);
end;

procedure TTest_Integer.Hashing;
var
  IntObj1, IntObj2: IInteger;
  HashCode1, HashCode2: SizeT;
begin
  CreateInteger(IntObj1, 3);
  CreateInteger(IntObj2, 4);

  IntObj1.GetHashCodeEx(HashCode1);
  Assert.AreNotEqual(HashCode1, SizeT(0));
  IntObj2.GetHashCodeEx(HashCode2);
  Assert.AreNotEqual(HashCode2, SizeT(0));
  Assert.AreNotEqual(HashCode1, HashCode2);
end;

procedure TTest_Integer.CastString;
var
  IntObj: IInteger;
  Str: PAnsiChar;
begin
  CreateInteger(IntObj, 1);
  IntObj.ToCharPtr(@Str);
  Assert.AreEqual(string(Str), '1');
  DaqFreeMemory(Str);
end;

procedure TTest_Integer.CastInt;
var
  IntObj: IInteger;
  Int: Int64;
begin
  CreateInteger(IntObj, 1);

  Int := BaseObjectToInt(IntObj);
  Assert.IsTrue(Int = 1);
end;

procedure TTest_Integer.CastFloat;
var
  IntObj: IInteger;
  Float: Double;
begin
  CreateInteger(IntObj, 1);

  Float := BaseObjectToFloat(IntObj);
  Assert.AreEqual(1.0, Float, 1e-7);
end;

procedure TTest_Integer.CastBool;
var
  IntObj: IInteger;
  ByteBoolVar: ByteBool;
begin
  CreateInteger(IntObj, 1);

  ByteBoolVar := BaseObjectToBool(IntObj);
  Assert.AreEqual(ByteBoolVar, True);
end;


procedure TTest_Integer.CoreType;
var
  IntObj: IInteger;
  CoreType: TCoreType;
begin
  CreateInteger(IntObj, 1);
  CoreType := GetCoreType(IntObj);
  Assert.AreEqual(CoreType, ctInt);
end;

procedure TTest_Integer.Setup;
begin

end;

procedure TTest_Integer.TearDown;
begin

end;

initialization
  TDSUnitTestEngine.RegisterUnitTest(TTest_Integer);

end.
