unit Test.OpenDAQ.StringObject;

interface
uses
  DunitX.TestFramework, DS.UT.DSUnitTestUnit, OpenDAQ.CoreTypes;

type
  [TestFixture]
  TTest_StringObject = class(TDSUnitTest)
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
    [Test]
    procedure Ptr;
    [Test]
    procedure DelphiString;
  end;

implementation
uses
  DS.UT.DSUnitTestEngineUnit, WinApi.Windows, OpenDAQ.CoreTypes.Errors, SysUtils,
  OpenDAQ.TString;

{ TTest_StringObject }

procedure TTest_StringObject.Basic;
var
  StringObj: IString;
  Str: PAnsiChar;
  Size: SizeT;
begin
  CreateString(StringObj, 'Test');
  Assert.IsTrue(OPENDAQ_SUCCEEDED(StringObj.GetCharPtr(@Str)));

  Assert.AreEqual(string(Str), 'Test');
  Assert.IsTrue(OPENDAQ_SUCCEEDED(StringObj.GetLength(Size)));

  Assert.AreEqual<SizeT>(Size, 4);
end;

procedure TTest_StringObject.Empty;
var
  StringObj: IString;
  Str: PAnsiChar;
  Size: SizeT;
begin
  CreateString(StringObj, '');
	StringObj.GetCharPtr(@Str);
  Assert.AreEqual(string(Str), '');

  Assert.IsTrue(OPENDAQ_SUCCEEDED(StringObj.GetLength(Size)));
  Assert.AreEqual<SizeT>(Size, 0);
end;

procedure TTest_StringObject.Equality;
var
  StringObj1, StringObj2, StringObj3, StringObj4, StringObj5: IString;
  Eq: Boolean;
begin
  CreateString(StringObj1, 'Test12');
  CreateString(StringObj2, 'Test12');
  CreateString(StringObj3, 'Test3');
  CreateString(StringObj4, '');
  CreateString(StringObj5, '');

  Eq := False;
  StringObj1.EqualsObject(StringObj2, Eq);
  Assert.IsTrue(Eq);

  StringObj1.EqualsObject(StringObj3, Eq);
  Assert.IsFalse(Eq);

  StringObj1.EqualsObject(StringObj4, Eq);
  Assert.IsFalse(Eq);

  StringObj4.EqualsObject(StringObj5, Eq);
  Assert.IsTrue(Eq);
end;

procedure TTest_StringObject.Hashing;
var
  StringObj1, StringObj2, StringObj3: IString;
  HashCode1, HashCode2: SizeT;
begin
  CreateString(StringObj1, 'Test2');
  CreateString(StringObj2, 'Test1');

  StringObj1.GetHashCodeEx(HashCode1);
  Assert.AreNotEqual<SizeT>(HashCode1, 0);

  StringObj2.GetHashCodeEx(HashCode2);
  Assert.AreNotEqual<SizeT>(HashCode2, 0);

  Assert.AreNotEqual<SizeT>(HashCode1, HashCode2);

  CreateString(StringObj3, '');
  StringObj3.GetHashCodeEx(HashCode1);
  Assert.AreEqual<SizeT>(HashCode1, 0);
end;

procedure TTest_StringObject.CastString;
var
  StringObj: IString;
  Str: PAnsiChar;
begin
  CreateString(StringObj, '1');
  StringObj.ToCharPtr(@Str);
  Assert.AreEqual(string(Str), '1');
  DaqFreeMemory(Str);
end;

procedure TTest_StringObject.CastInt;
var
  StringObj1, StringObj2: IString;
  Int: Int64;
  TempMethod: TTestLocalMethod;
begin
  CreateString(StringObj1, '1');
  Int := BaseObjectToInt(StringObj1);
  Assert.IsTrue(Int = 1);

  CreateString(StringObj2, 'a');
  TempMethod := procedure
  begin
    BaseObjectToInt(StringObj2);
  end;;

  Assert.WillRaise(TempMethod, EConvertError);
end;

procedure TTest_StringObject.CastFloat;
var
  StringObj: IString;
  Float: Double;
begin
  CreateString(StringObj, '1');
  Float := BaseObjectToFloat(StringObj);
  Assert.AreEqual(1.0, Float, 1e-7);
end;

procedure TTest_StringObject.CastBool;
var
  StringObj1, StringObj2, StringObj3, StringObj4, StringObj5: IString;
  ByteBoolVar: ByteBool;
begin
  CreateString(StringObj1, 'True');
  ByteBoolVar := BaseObjectToBool(StringObj1);
  Assert.AreEqual(ByteBoolVar, True);

  CreateString(StringObj2, 'False');
  ByteBoolVar := BaseObjectToBool(StringObj2);
  Assert.AreEqual(ByteBoolVar, False);

  CreateString(StringObj3, '1');
  ByteBoolVar := BaseObjectToBool(StringObj3);
  Assert.AreEqual(ByteBoolVar, True);

  CreateString(StringObj4, '0');
  ByteBoolVar := BaseObjectToBool(StringObj4);
  Assert.AreEqual(ByteBoolVar, False);

  CreateString(StringObj5, 'axy');
  ByteBoolVar := BaseObjectToBool(StringObj5);
  Assert.AreEqual(ByteBoolVar, False);
end;

procedure TTest_StringObject.CoreType;
var
  StringObj: IString;
  CoreType: TCoreType;
begin
  CreateString(StringObj, '1');

  CoreType := GetCoreType(StringObj);
  Assert.AreEqual(CoreType, ctString);
end;

procedure TTest_StringObject.DelphiString;
var
  StringObj: IString;
  Str: PAnsiChar;
  Err: ErrCode;
begin
  StringObj := CreateStringFromDelphiString('Test');
  Assert.IsTrue(OPENDAQ_SUCCEEDED(StringObj.GetCharPtr(@Str)));
  Assert.AreEqual(string(UTF8String(Str)), 'Test');

  Err := CreateStringFromDelphiString(StringObj, 'Test1');
  Assert.IsTrue(OPENDAQ_SUCCEEDED(Err));
  Assert.IsTrue(OPENDAQ_SUCCEEDED(StringObj.GetCharPtr(@Str)));
  Assert.AreEqual(string(UTF8String(Str)), 'Test1');
end;

procedure TTest_StringObject.Ptr;
var
  StrPtr : IStringPtr;
  DelphiString : string;
begin
  StrPtr := TStringPtr.Create(CreateStringFromDelphiString('Test'));
  DelphiString := StrPtr.ToString();

  Assert.AreEqual('Test', DelphiString);
  Assert.AreEqual<SizeT>(Length('Test'), StrPtr.GetLength());
end;

procedure TTest_StringObject.Setup;
begin

end;

procedure TTest_StringObject.TearDown;
begin

end;

initialization
  TDSUnitTestEngine.RegisterUnitTest(TTest_StringObject);

end.
