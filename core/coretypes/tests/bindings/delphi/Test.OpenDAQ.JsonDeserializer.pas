unit Test.OpenDAQ.JsonDeserializer;

interface

uses
  DunitX.TestFramework, DS.UT.DSUnitTestUnit, OpenDAQ.CoreTypes;

type
  [TestFixture]
  TTest_JsonDeserializer = class(TDSUnitTest)
  private
  public
    [Setup]
    procedure Setup; override;
    [TearDown]
    procedure TearDown; override;
    [Test]
    procedure DeserializeInvalidJson;
    [Test]
    procedure BoolTrue;
    [Test]
    procedure BoolFalse;
    [Test]
    procedure FloatZero;
    [Test]
    procedure FloatMax;
    [Test]
    procedure FloatMin;
    [Test]
    procedure IntZero;
    [Test]
    procedure IntMax;
    [Test]
    procedure IntMin;
    [Test]
    procedure AsciiStr;
    [Test]
    procedure EmptyList;
    [Test]
    procedure StringListOne;
    [Test]
    procedure StringListMultiple;
    [Test]
    procedure BoolListTrue;
    [Test]
    procedure BoolListFalse;
    [Test]
    procedure BoolList;
    [Test]
    procedure FloatListOne;
    [Test]
    procedure FloatListMultiple;
    [Test]
    procedure IntListOne;
    [Test]
    procedure IntListMultiple;
    [Test]
    procedure MixedList;
    [Test]
    procedure UnknownObjectType;
    [Test]
    procedure ObjectTypeTagNotInt;
    [Test]
    procedure NoObjectType;
    [Test]
    procedure DeserializeNullString;
    [Test]
    procedure RegisterFactory;
    [Test]
    procedure RegisterExistingFactory;
    [Test]
    procedure GetFactory;
    [Test]
    procedure UnregisterFactory;
    [Test]
    procedure UnregisterNonExistingFactory;
    [Test]
    procedure GetNonExistingFactory;
    [Test]
    procedure FactoryReturnsError;
  end;

function SerializedObjectFactory(SerObj: ISerializedObject; Context: IBaseObject; out Obj: IBaseObject): ErrCode; cdecl;
function ErrorFactory(SerObj: ISerializedObject; Context: IBaseObject; out Obj: IBaseObject): ErrCode; cdecl;

implementation

uses
  DS.UT.DSUnitTestEngineUnit, WinApi.Windows, OpenDAQ.CoreTypes.Errors, SysUtils;


function SerializedObjectFactory(SerObj: ISerializedObject; Context: IBaseObject; out Obj: IBaseObject): ErrCode;
begin
  Result := OPENDAQ_SUCCESS;
end;

function ErrorFactory(SerObj: ISerializedObject; Context: IBaseObject; out Obj: IBaseObject): ErrCode;
begin
  Result := OPENDAQ_ERR_GENERALERROR;
end;

{ TTest_BaseObject }

procedure TTest_JsonDeserializer.Setup;
begin
end;

procedure TTest_JsonDeserializer.TearDown;
begin
end;

procedure TTest_JsonDeserializer.DeserializeInvalidJson;
var
  StringObj: IString;
  BaseObj: IBaseObject;
  Deserializer: IDeserializer;
begin
  CreateJsonDeserializer(Deserializer);
  CreateString(StringObj, '...');
  Assert.AreEqual(Deserializer.Deserialize(StringObj, nil, BaseObj), OPENDAQ_ERR_DESERIALIZE_PARSE_ERROR);
end;

procedure TTest_JsonDeserializer.BoolTrue;
var
  StringObj: IString;
  Deserializer: IDeserializer;
  BoolObj: IBoolean;
  BoolVal: Boolean;
begin
  CreateJsonDeserializer(Deserializer);
  CreateString(StringObj, 'true');
  Deserializer.Deserialize(StringObj, nil, IBaseObject(BoolObj));
  BoolObj.GetValue(BoolVal);
  Assert.IsTrue(BoolVal);
end;

procedure TTest_JsonDeserializer.BoolFalse;
var
  StringObj: IString;
  Deserializer: IDeserializer;
  BoolObj: IBoolean;
  BoolVal: Boolean;
begin
  CreateJsonDeserializer(Deserializer);
  CreateString(StringObj, 'false');
  Deserializer.Deserialize(StringObj, nil, IBaseObject(BoolObj));
  BoolObj.GetValue(BoolVal);
  Assert.IsFalse(BoolVal);
end;

procedure TTest_JsonDeserializer.FloatZero;
var
  StringObj: IString;
  Deserializer: IDeserializer;
  FloatObj: IFloat;
  FloatVal: Double;
begin
  CreateJsonDeserializer(Deserializer);
  CreateString(StringObj, '0.0');
  Deserializer.Deserialize(StringObj, nil, IBaseObject(FloatObj));
  FloatObj.GetValue(FloatVal);
  Assert.AreEqual(FloatVal, Double(0.0));
end;

procedure TTest_JsonDeserializer.FloatMax;
const FloatMax = 1.7976931348623157e308;
var
  StringObj: IString;
  Deserializer: IDeserializer;
  FloatObj: IFloat;
  FloatVal: Double;
begin
  CreateJsonDeserializer(Deserializer);
  CreateString(StringObj, '1.7976931348623157e308');
  Deserializer.Deserialize(StringObj, nil, IBaseObject(FloatObj));
  FloatObj.GetValue(FloatVal);
  Assert.AreEqual(FloatVal, Double(FloatMax));
end;

procedure TTest_JsonDeserializer.FloatMin;
const FloatMin = 2.2250738585072014e-308;
var
  StringObj: IString;
  Deserializer: IDeserializer;
  FloatObj: IFloat;
  FloatVal: Double;
begin
  CreateJsonDeserializer(Deserializer);
  CreateString(StringObj, '2.2250738585072014e-308');
  Deserializer.Deserialize(StringObj, nil, IBaseObject(FloatObj));
  FloatObj.GetValue(FloatVal);
  Assert.AreEqual(FloatVal, Double(FloatMin));
end;

procedure TTest_JsonDeserializer.IntZero;
var
  StringObj: IString;
  Deserializer: IDeserializer;
  IntObj: IInteger;
  IntVal: Int64;
begin
  CreateJsonDeserializer(Deserializer);
  CreateString(StringObj, '0');
  Deserializer.Deserialize(StringObj, nil, IBaseObject(IntObj));
  Intobj.GetValue(IntVal);
  Assert.AreEqual(IntVal, Int64(0));
end;

procedure TTest_JsonDeserializer.IntMax;
const IntMax = 9223372036854775807;
var
  StringObj: IString;
  Deserializer: IDeserializer;
  IntObj: IInteger;
  IntVal: Int64;
begin
  CreateJsonDeserializer(Deserializer);
  CreateString(StringObj, '9223372036854775807');
  Deserializer.Deserialize(StringObj, nil, IBaseObject(IntObj));
  Intobj.GetValue(IntVal);
  Assert.AreEqual(IntVal, IntMax);
end;

procedure TTest_JsonDeserializer.IntMin;
const IntMin = -9223372036854775808;
var
  StringObj: IString;
  Deserializer: IDeserializer;
  IntObj: IInteger;
  IntVal: Int64;
begin
  CreateJsonDeserializer(Deserializer);
  CreateString(StringObj, '-9223372036854775808');
  Deserializer.Deserialize(StringObj, nil, IBaseObject(IntObj));
  Intobj.GetValue(IntVal);
  Assert.AreEqual(IntVal, IntMin);
end;

procedure TTest_JsonDeserializer.AsciiStr;
const Expected = ' !"#$%&''()*+''-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijklmnopqrstuvwxyz{|}~';
var
  StringObj: IString;
  Deserializer: IDeserializer;
  StrObj: IString;
  Str: PAnsiChar;
begin
  CreateJsonDeserializer(Deserializer);
  CreateString(StringObj, '" !\"#$%&''()*+''-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~"');
  Deserializer.Deserialize(StringObj, nil, IBaseObject(StrObj));
  StrObj.GetCharPtr(@Str);
  Assert.AreEqual(string(Str), Expected);
end;


procedure TTest_JsonDeserializer.EmptyList;
var
  StringObj: IString;
  Deserializer: IDeserializer;
  ListObj: IListObject;
  Count: SizeT;
begin
  CreateJsonDeserializer(Deserializer);
  CreateString(StringObj, '[]');
  Deserializer.Deserialize(StringObj, nil, IBaseObject(ListObj));
  ListObj.GetCount(Count);
  Assert.AreEqual(Count, SizeT(0));
end;

procedure TTest_JsonDeserializer.StringListOne;
var
  StringObj: IString;
  Deserializer: IDeserializer;
  ListObj: IListObject;
  Count: SizeT;
  StrObj1: IString;
  Str: PAnsiChar;
begin
  CreateJsonDeserializer(Deserializer);
  CreateString(StringObj, '["Item1"]');
  Deserializer.Deserialize(StringObj, nil, IBaseObject(ListObj));
  ListObj.GetCount(Count);
  Assert.AreEqual(Count, SizeT(1));

  ListObj.GetItemAt(0, IBaseObject(StrObj1));

  StrObj1.GetCharPtr(@Str);
  Assert.AreEqual(string(Str), 'Item1');
end;

procedure TTest_JsonDeserializer.StringListMultiple;
var
  StringObj: IString;
  Deserializer: IDeserializer;
  ListObj: IListObject;
  Count: SizeT;
  StrObj1, StrObj2: IString;
  Str: PAnsiChar;
begin
  CreateJsonDeserializer(Deserializer);
  CreateString(StringObj, '["Item1", "Item2"]');
  Deserializer.Deserialize(StringObj, nil, IBaseObject(ListObj));
  ListObj.GetCount(Count);
  Assert.AreEqual(Count, SizeT(2));

  ListObj.GetItemAt(0, IBaseObject(StrObj1));
  ListObj.GetItemAt(1, IBaseObject(StrObj2));

  StrObj1.GetCharPtr(@Str);
  Assert.AreEqual(string(Str), 'Item1');

  StrObj2.GetCharPtr(@Str);
  Assert.AreEqual(string(Str), 'Item2');
end;

procedure TTest_JsonDeserializer.BoolListTrue;
var
  StringObj: IString;
  Deserializer: IDeserializer;
  ListObj: IListObject;
  Count: SizeT;
  BoolObj: IBoolean;
  BoolVal: Boolean;
begin
  CreateJsonDeserializer(Deserializer);
  CreateString(StringObj, '[true]');
  Deserializer.Deserialize(StringObj, nil, IBaseObject(ListObj));
  ListObj.GetCount(Count);
  Assert.AreEqual(Count, SizeT(1));

  ListObj.GetItemAt(0, IBaseObject(BoolObj));
  BoolObj.GetValue(BoolVal);
  Assert.IsTrue(BoolVal);
end;

procedure TTest_JsonDeserializer.BoolListFalse;
var
  StringObj: IString;
  Deserializer: IDeserializer;
  ListObj: IListObject;
  Count: SizeT;
  BoolObj: IBoolean;
  BoolVal: Boolean;
begin
  CreateJsonDeserializer(Deserializer);
  CreateString(StringObj, '[false]');
  Deserializer.Deserialize(StringObj, nil, IBaseObject(ListObj));
  ListObj.GetCount(Count);
  Assert.AreEqual(Count, SizeT(1));

  ListObj.GetItemAt(0, IBaseObject(BoolObj));
  BoolObj.GetValue(BoolVal);
  Assert.IsFalse(BoolVal);
end;

procedure TTest_JsonDeserializer.BoolList;
var
  StringObj: IString;
  Deserializer: IDeserializer;
  ListObj: IListObject;
  Count: SizeT;
  BoolObj1, BoolObj2: IBoolean;
  BoolVal: Boolean;
begin
  CreateJsonDeserializer(Deserializer);
  CreateString(StringObj, '[false,true]');
  Deserializer.Deserialize(StringObj, nil, IBaseObject(ListObj));
  ListObj.GetCount(Count);
  Assert.AreEqual(Count, SizeT(2));

  ListObj.GetItemAt(0, IBaseObject(BoolObj1));
  ListObj.GetItemAt(1, IBaseObject(BoolObj2));

  BoolObj1.GetValue(BoolVal);
  Assert.IsFalse(BoolVal);
  BoolObj2.GetValue(BoolVal);
  Assert.IsTrue(BoolVal);
end;

procedure TTest_JsonDeserializer.FloatListOne;
var
  StringObj: IString;
  Deserializer: IDeserializer;
  ListObj: IListObject;
  Count: SizeT;
  FloatVal: Double;
  FloatObj: IFloat;
begin
  CreateJsonDeserializer(Deserializer);
  CreateString(StringObj, '[0.0]');
  Deserializer.Deserialize(StringObj, nil, IBaseObject(ListObj));
  ListObj.GetCount(Count);
  Assert.AreEqual(Count, SizeT(1));

  ListObj.GetItemAt(0, IBaseObject(FloatObj));
  FloatObj.GetValue(FloatVal);
  Assert.AreEqual(FloatVal, Double(0));
end;

procedure TTest_JsonDeserializer.FloatListMultiple;
const
  FloatMax = 1.7976931348623157e308;
  FloatMin = 2.2250738585072014e-308;
var
  StringObj: IString;
  Deserializer: IDeserializer;
  ListObj: IListObject;
  Count: SizeT;
  FloatVal: Double;
  FloatObj1, FloatObj2, FloatObj3: IFloat;
begin
  CreateJsonDeserializer(Deserializer);
  CreateString(StringObj, '[0.0,2.2250738585072014e-308,1.7976931348623157e308]');
  Deserializer.Deserialize(StringObj, nil, IBaseObject(ListObj));
  ListObj.GetCount(Count);
  Assert.AreEqual(Count, SizeT(3));

  ListObj.GetItemAt(0, IBaseObject(FloatObj1));
  ListObj.GetItemAt(1, IBaseObject(FloatObj2));
  ListObj.GetItemAt(2, IBaseObject(FloatObj3));
  FloatObj1.GetValue(FloatVal);
  Assert.AreEqual(FloatVal, Double(0));
  FloatObj2.GetValue(FloatVal);
  Assert.AreEqual(FloatVal, Double(FloatMin));
  FloatObj3.GetValue(FloatVal);
  Assert.AreEqual(FloatVal, Double(FloatMax));
end;

procedure TTest_JsonDeserializer.IntListOne;
var
  StringObj: IString;
  Deserializer: IDeserializer;
  ListObj: IListObject;
  Count: SizeT;
  IntVal: Int64;
  IntObj: IInteger;
begin
  CreateJsonDeserializer(Deserializer);
  CreateString(StringObj, '[0]');
  Deserializer.Deserialize(StringObj, nil, IBaseObject(ListObj));
  ListObj.GetCount(Count);
  Assert.AreEqual(Count, SizeT(1));

  ListObj.GetItemAt(0, IBaseObject(IntObj));
  IntObj.GetValue(IntVal);
  Assert.AreEqual(IntVal, Int64(0));
end;

procedure TTest_JsonDeserializer.IntListMultiple;
const
  IntMax = 9223372036854775807;
  IntMin = -9223372036854775808;
var
  StringObj: IString;
  Deserializer: IDeserializer;
  ListObj: IListObject;
  Count: SizeT;
  IntVal: Int64;
  IntObj1, IntObj2, IntObj3: IInteger;
begin
  CreateJsonDeserializer(Deserializer);
  CreateString(StringObj, '[0,-9223372036854775808,9223372036854775807]');
  Deserializer.Deserialize(StringObj, nil, IBaseObject(ListObj));
  ListObj.GetCount(Count);
  Assert.AreEqual(Count, SizeT(3));

  ListObj.GetItemAt(0, IBaseObject(IntObj1));
  ListObj.GetItemAt(1, IBaseObject(IntObj2));
  ListObj.GetItemAt(2, IBaseObject(IntObj3));
  IntObj1.GetValue(IntVal);
  Assert.AreEqual(IntVal, Int64(0));
  IntObj2.GetValue(IntVal);
  Assert.AreEqual(IntVal, Int64(IntMin));
  IntObj3.GetValue(IntVal);
  Assert.AreEqual(IntVal, Int64(IntMax));
end;

procedure TTest_JsonDeserializer.MixedList;
const
  IntMax = 9223372036854775807;
  IntMin = -9223372036854775808;
  FloatMax = 1.7976931348623157e308;
  FloatMin = 2.2250738585072014e-308;

var
  StringObj: IString;
  Deserializer: IDeserializer;
  ListObj: IListObject;
  Count: SizeT;
  FloatVal: Double;
  FloatObj1, FloatObj2, FloatObj3, FloatObj4, FloatObj5: IFloat;
  IntVal: Int64;
  IntObj1, IntObj2, IntObj3, IntObj4, IntObj5: IInteger;
  StrObj: IString;
  Str: PAnsiChar;

begin
  CreateJsonDeserializer(Deserializer);
  CreateString(StringObj, '[0.0,0,-2.5,1.5,1,-2,2.2250738585072014e-308,1.7976931348623157e308,-9223372036854775808,9223372036854775807,"Test1"]');
  Deserializer.Deserialize(StringObj, nil, IBaseObject(ListObj));
  ListObj.GetCount(Count);
  Assert.AreEqual(Count, SizeT(11));

  ListObj.GetItemAt(0, IBaseObject(FloatObj1));
  FloatObj1.GetValue(FloatVal);
  Assert.AreEqual(FloatVal, Double(0.0));

  ListObj.GetItemAt(1, IBaseObject(IntObj1));
  IntObj1.GetValue(IntVal);
  Assert.AreEqual(IntVal, Int64(0));

  ListObj.GetItemAt(2, IBaseObject(FloatObj2));
  FloatObj2.GetValue(FloatVal);
  Assert.AreEqual(FloatVal, Double(-2.5));

  ListObj.GetItemAt(3, IBaseObject(FloatObj3));
  FloatObj3.GetValue(FloatVal);
  Assert.AreEqual(FloatVal, Double(1.5));

  ListObj.GetItemAt(4, IBaseObject(IntObj2));
  IntObj2.GetValue(IntVal);
  Assert.AreEqual(IntVal, Int64(1));

  ListObj.GetItemAt(5, IBaseObject(IntObj3));
  IntObj3.GetValue(IntVal);
  Assert.AreEqual(IntVal, Int64(-2));

  ListObj.GetItemAt(6, IBaseObject(FloatObj4));
  FloatObj4.GetValue(FloatVal);
  Assert.AreEqual(FloatVal, Double(FloatMin));

  ListObj.GetItemAt(7, IBaseObject(FloatObj5));
  FloatObj5.GetValue(FloatVal);
  Assert.AreEqual(FloatVal, Double(FloatMax));

  ListObj.GetItemAt(8, IBaseObject(IntObj4));
  IntObj4.GetValue(IntVal);
  Assert.AreEqual(IntVal, Int64(IntMin));

  ListObj.GetItemAt(9, IBaseObject(IntObj5));
  IntObj5.GetValue(IntVal);
  Assert.AreEqual(IntVal, Int64(IntMax));

  ListObj.GetItemAt(10, IBaseObject(StrObj));
  StrObj.GetCharPtr(@Str);
  Assert.AreEqual(string(Str), 'Test1');
end;

procedure TTest_JsonDeserializer.UnknownObjectType;
var
  Deserializer: IDeserializer;
  BaseObj: IBaseObject;
  StringObj: IString;
begin
  CreateJsonDeserializer(Deserializer);
  CreateString(StringObj, '{"__type":"unknown"}');
  Assert.AreEqual(Deserializer.Deserialize(StringObj, nil, BaseObj), OPENDAQ_ERR_FACTORY_NOT_REGISTERED);
end;

procedure TTest_JsonDeserializer.ObjectTypeTagNotInt;
var
  Deserializer: IDeserializer;
  BaseObj: IBaseObject;
  StringObj: IString;
begin
  CreateJsonDeserializer(Deserializer);
  CreateString(StringObj, '{"__type":0.0}');
  Assert.AreEqual(Deserializer.Deserialize(StringObj, nil, BaseObj), OPENDAQ_ERR_DESERIALIZE_UNKNOWN_TYPE);
end;

procedure TTest_JsonDeserializer.NoObjectType;
var
  Deserializer: IDeserializer;
  BaseObj: IBaseObject;
  StringObj: IString;
begin
  CreateJsonDeserializer(Deserializer);
  CreateString(StringObj, '{"test":0}');
  Assert.AreEqual(Deserializer.Deserialize(StringObj, nil, BaseObj), OPENDAQ_ERR_DESERIALIZE_NO_TYPE);
end;

procedure TTest_JsonDeserializer.DeserializeNullString;
var
  Deserializer: IDeserializer;
  BaseObj: IBaseObject;
begin
  CreateJsonDeserializer(Deserializer);
  Assert.AreEqual(Deserializer.Deserialize(nil, nil, BaseObj), OPENDAQ_ERR_ARGUMENT_NULL);
end;

procedure TTest_JsonDeserializer.RegisterFactory;
var
  Res: ErrCode;
  Id: AnsiString;
begin
  Id := 'test';
  Res := DaqRegisterSerializerFactory(PAnsiChar(Id), SerializedObjectFactory);
  Assert.AreEqual(Res, OPENDAQ_SUCCESS);
  DaqUnregisterSerializerFactory(PAnsiChar(Id));
end;

procedure TTest_JsonDeserializer.RegisterExistingFactory;
var
  Res: ErrCode;
  Id: AnsiString;
begin
  Id := 'test';
  DaqRegisterSerializerFactory(PAnsiChar(Id), SerializedObjectFactory);
  Res := DaqRegisterSerializerFactory(PAnsiChar(Id), SerializedObjectFactory);
  Assert.AreEqual(Res, OPENDAQ_SUCCESS);
end;

procedure TTest_JsonDeserializer.GetFactory;
var
  Id: AnsiString;
  Factory: TDSRTDeserializerFactory;
  Res: ErrCode;
begin
  Id := 'test';
  DaqRegisterSerializerFactory(PAnsiChar(Id), SerializedObjectFactory);

  Res := DaqGetSerializerFactory(PAnsiChar(Id), Factory);
  Assert.AreEqual(Res, OPENDAQ_SUCCESS);
  DaqUnregisterSerializerFactory(PAnsiChar(Id));
end;


procedure TTest_JsonDeserializer.UnregisterFactory;
var
  Res: ErrCode;
  Id: AnsiString;
begin
  Id := 'test';
  DaqRegisterSerializerFactory(PAnsiChar(Id), SerializedObjectFactory);
  Res := DaqUnregisterSerializerFactory(PAnsiChar(Id));
  Assert.AreEqual(Res, OPENDAQ_SUCCESS);
  DaqUnregisterSerializerFactory(PAnsiChar(Id));
end;

procedure TTest_JsonDeserializer.UnregisterNonExistingFactory;
var
  Res: ErrCode;
  Id: AnsiString;
begin
  Id := 'test';
  Res := DaqUnregisterSerializerFactory(PAnsiChar(Id));
  Assert.AreEqual(Res, OPENDAQ_ERR_FACTORY_NOT_REGISTERED);
end;

procedure TTest_JsonDeserializer.GetNonExistingFactory;
var
  Res: ErrCode;
  Id: AnsiString;
  Factory: TDSRTDeserializerFactory;
begin
  Id := 'test';
  Res := DaqGetSerializerFactory(PAnsiChar(Id), Factory);
  Assert.AreEqual(Res, OPENDAQ_ERR_FACTORY_NOT_REGISTERED);
end;

procedure TTest_JsonDeserializer.FactoryReturnsError;
var
  Id: AnsiString;
  Deserializer: IDeserializer;
  StringObj: IString;
  BaseObj: IBaseObject;
  Str: AnsiString;

begin
  Id := 'test';
  CreateJsonDeserializer(Deserializer);
  DaqRegisterSerializerFactory(PAnsiChar(Id), ErrorFactory);

  Str := '{"__type":"' + Id + '"}';
  CreateString(StringObj, PAnsiChar(Str));
  Assert.AreEqual(Deserializer.Deserialize(StringObj, nil, BaseObj), OPENDAQ_ERR_GENERALERROR);
  DaqUnregisterSerializerFactory(PAnsiChar(Id));
end;

initialization
  TDSUnitTestEngine.RegisterUnitTest(TTest_JsonDeserializer);

end.
