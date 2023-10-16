unit Test.OpenDAQ.StringPtr;

interface
uses
  DunitX.TestFramework, DS.UT.DSUnitTestUnit, OpenDAQ.CoreTypes;

type
  [TestFixture]
  TTest_StringPtr = class(TDSUnitTest)
  private
  public
    [Setup]
    procedure Setup; override;
    [TearDown]
    procedure TearDown; override;
    [Test]
    procedure Basic;
    [Test]
    procedure Empty;
    [Test]
    procedure ValidInterface;
    [Test]
    procedure InvalidInterface;
    [Test]
    procedure Equality;
    [Test]
    procedure Hashing;
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
  SysUtils,
  WinApi.Windows,
  OpenDAQ.TString,
  DS.UT.DSUnitTestEngineUnit,
  OpenDAQ.CoreTypes.Errors,
  OpenDAQ.Exceptions;

{ TTest_StringPtr }

procedure TTest_StringPtr.Basic();
var
  StringObj: IStringPtr;
begin
  StringObj := TStringPtr.Create('Test');
  Assert.AreEqual(StringObj.ToString(), 'Test');

  Assert.AreEqual<SizeT>(StringObj.GetLength(), 4);
end;

procedure TTest_StringPtr.ValidInterface();
var
  IStr : IBaseObject;
  StrPtr : IStringPtr;
begin
  IStr := CreateStringFromDelphiString('Test');

  Assert.WillNotRaiseAny(procedure()
    begin
      StrPtr := TStringPtr.Create(IStr);
    end
  );
end;

procedure TTest_StringPtr.InvalidInterface();
var
  BaseObj : IBaseObject;
  StrPtr : IStringPtr;
begin
  CreateBaseObject(BaseObj);

  Assert.WillRaise(procedure()
    begin
      StrPtr := TStringPtr.Create(BaseObj);
    end,
    ERTNoInterfaceException
  );
end;

procedure TTest_StringPtr.Empty();
var
  StringObj: IStringPtr;
begin
  StringObj := TStringPtr.Create('');
  Assert.AreEqual(StringObj.ToString(), '');

  Assert.AreEqual<SizeT>(StringObj.GetLength(), 0);
end;

procedure TTest_StringPtr.Equality();
var
  StringObj1, StringObj2, StringObj3, StringObj4, StringObj5: IStringPtr;
begin
  StringObj1 := TStringPtr.Create('Test12');
  StringObj2 := TStringPtr.Create('Test12');
  StringObj3 := TStringPtr.Create('Test3');
  StringObj4 := TStringPtr.Create('');
  StringObj5 := TStringPtr.Create('');

  Assert.IsTrue(StringObj1.EqualsObject(StringObj2));
  Assert.IsFalse(StringObj1.EqualsObject(StringObj3));
  Assert.IsFalse(StringObj1.EqualsObject(StringObj4));
  Assert.IsTrue(StringObj4.EqualsObject(StringObj5));
end;

procedure TTest_StringPtr.Hashing();
var
  StringObj1, StringObj2, StringObj3: IStringPtr;
  HashCode1, HashCode2: SizeT;
begin
  StringObj1 := TStringPtr.Create('Test2');
  StringObj2 := TStringPtr.Create('Test1');

  HashCode1 := StringObj1.GetHashCodeEx();
  Assert.AreNotEqual<SizeT>(HashCode1, 0);

  HashCode2 := StringObj2.GetHashCodeEx();
  Assert.AreNotEqual<SizeT>(HashCode2, 0);

  Assert.AreNotEqual(HashCode1, HashCode2);

  StringObj3 := TStringPtr.Create('');
  HashCode1 := StringObj3.GetHashCodeEx();
  Assert.AreEqual<SizeT>(HashCode1, 0);
end;

procedure TTest_StringPtr.CastInt();
var
  StringObj1, StringObj2: IStringPtr;
  Int: RtInt;
  TempMethod: TTestLocalMethod;
begin
  StringObj1 := TStringPtr.Create('1');
  Int := BaseObjectToInt(StringObj1);
  Assert.IsTrue(Int = 1);

  StringObj2 := TStringPtr.Create('a');
  TempMethod := procedure
    begin
      BaseObjectToInt(StringObj2);
    end;

  Assert.WillRaise(TempMethod, EConvertError);
end;

procedure TTest_StringPtr.CastFloat();
var
  StringObj: IStringPtr;
  Float: Double;
begin
  StringObj := TStringPtr.Create('1');
  Float := BaseObjectToFloat(StringObj);
  Assert.AreEqual(1.0, Float, 1e-7);
end;

procedure TTest_StringPtr.CastBool();
var
  StringObj1, StringObj2, StringObj3, StringObj4, StringObj5: IStringPtr;
begin
  StringObj1 := TStringPtr.Create('True');

  StringObj2 := TStringPtr.Create('False');
  Assert.IsFalse(BaseObjectToBool(StringObj2));

  StringObj3 := TStringPtr.Create('1');
  Assert.IsTrue(BaseObjectToBool(StringObj3));

  StringObj4 := TStringPtr.Create('0');
  Assert.IsFalse(BaseObjectToBool(StringObj4));

  StringObj5 := TStringPtr.Create('axy');
  Assert.IsFalse(BaseObjectToBool(StringObj5));
end;

procedure TTest_StringPtr.CoreType();
var
  StringObj: IStringPtr;
begin
  StringObj := TStringPtr.Create('1');

  Assert.AreEqual(GetCoreType(StringObj), ctString);
end;

procedure TTest_StringPtr.Setup;
begin

end;

procedure TTest_StringPtr.TearDown;
begin

end;

initialization
  TDSUnitTestEngine.RegisterUnitTest(TTest_StringPtr);

end.
