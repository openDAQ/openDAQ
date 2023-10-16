unit Test.OpenDAQ.JsonSerializedObject;

interface

uses
  DunitX.TestFramework, DS.UT.DSUnitTestUnit, OpenDAQ.CoreTypes;

type
  [TestFixture]
  TTest_JsonSerializedObject = class(TDSUnitTest)
  private
    FId: AnsiString;
  public
    [Setup]
    procedure Setup; override;
    [TearDown]
    procedure TearDown; override;
    [Test]
    procedure ReadIntPositive;
    [Test]
    procedure ReadIntNegative;
    [Test]
    procedure ReadIntInvalidType;
    [Test]
    procedure ReadNonExistentInt;
    [Test]
    procedure ReadFloatPositive;
    [Test]
    procedure ReadFloatNegative;
    [Test]
    procedure ReadNonExistentFloat;
    [Test]
    procedure ReadFloatInvalidType;
    [Test]
    procedure ReadBoolTrue;
    [Test]
    procedure ReadBoolFalse;
    [Test]
    procedure ReadBoolInvalidType;
    [Test]
    procedure ReadNonExistentBool;
    [Test]
    procedure ReadString;
    [Test]
    procedure ReadStringInvalidType;
    [Test]
    procedure ReadNonExistentString;
    [Test]
    procedure TestHasKeyTrue;
    [Test]
    procedure TestHasKeyFalse;
    [Test]
    procedure ReadEmptyObjectKeys;
    [Test]
    procedure ReadObjectKeys;
    [Test]
    procedure ReadNonExistentObject;
    [Test]
    procedure ReadSerializedObjectInvalidType;
    [Test]
    procedure ReadNonExistentSerializedObject;
    [Test]
    procedure ReadEmptySerializedList;
    [Test]
    procedure ReadNonExistingSerializedList;
    [Test]
    procedure ReadSerializedListInvalidType;
    [Test]
    procedure ReadEmptyList;
    [Test]
    procedure ReadNonExistingList;
    [Test]
    procedure ReadListInvalidType;
  end;

  function IntFactory(Serialized: ISerializedObject; Context: IBaseObject; out Obj: IBaseObject): ErrCode; cdecl;
  function FloatFactory(Serialized: ISerializedObject; Context: IBaseObject; out Obj: IBaseObject): ErrCode; cdecl;
  function BoolFactory(Serialized: ISerializedObject; Context: IBaseObject; out Obj: IBaseObject): ErrCode; cdecl;
  function StringFactory(Serialized: ISerializedObject; Context: IBaseObject; out Obj: IBaseObject): ErrCode; cdecl;
  function SerializedObjectFactory(Serialized: ISerializedObject; Context: IBaseObject; out Obj: IBaseObject): ErrCode; cdecl;
  function ListFactory(Serialized: ISerializedObject; Context: IBaseObject; out Obj: IBaseObject): ErrCode; cdecl;

  function HasKeyFactory(Serialized: ISerializedObject; Context: IBaseObject; out Obj: IBaseObject): ErrCode; cdecl;
  function KeyFactory(Serialized: ISerializedObject; Context: IBaseObject; out Obj: IBaseObject): ErrCode; cdecl;
  function ObjectFactory(Serialized: ISerializedObject; Context: IBaseObject; out Obj: IBaseObject): ErrCode; cdecl;
  function SerializedObjectErrorFactory(Serialized: ISerializedObject; Context: IBaseObject; out Obj: IBaseObject): ErrCode; cdecl;

implementation
uses  
  WinApi.Windows,
  System.SysUtils,
  OpenDAQ.List,
  OpenDAQ.TString,
  DS.UT.DSUnitTestEngineUnit,
  OpenDAQ.CoreTypes.Errors,
  OpenDAQ.ObjectPtr,
  OpenDAQ.Exceptions,
  OpenDAQ.Deserializer;


function IntFactory(Serialized: ISerializedObject; Context: IBaseObject; out Obj: IBaseObject): ErrCode;
var
  Value: Int64;
  Res: ErrCode;
  StringObj: IString;
  IntObj: IInteger;
begin
  CreateString(StringObj, 'int');
  Res := Serialized.ReadInt(StringObj, Value);

  if OPENDAQ_FAILED(Res) then
    Exit(Res);

  CreateInteger(IntObj, Value);
  Obj := IntObj;

  Result := OPENDAQ_SUCCESS;
end;

function FloatFactory(Serialized: ISerializedObject; Context: IBaseObject; out Obj: IBaseObject): ErrCode;
var
  Value: Double;
  Res: ErrCode;
  StringObj: IString;
  FloatObj: IFloat;
begin
  CreateString(StringObj, 'float');
  Res := Serialized.ReadFloat(StringObj, Value);

  if OPENDAQ_FAILED(Res) then
    Exit(Res);

  CreateFloat(FloatObj, Value);
  Obj := FloatObj;

  Result := OPENDAQ_SUCCESS;
end;

function BoolFactory(Serialized: ISerializedObject; Context: IBaseObject; out Obj: IBaseObject): ErrCode;
var
  Value: Boolean;
  Res: ErrCode;
  StringObj: IString;
  BoolObj: IBoolean;
begin
  CreateString(StringObj, 'bool');
  Res := Serialized.ReadBool(StringObj, Value);

  if OPENDAQ_FAILED(Res) then
    Exit(Res);

  CreateBoolean(Boolobj, Value);
  Obj := BoolObj;

  Result := OPENDAQ_SUCCESS;
end;

function StringFactory(Serialized: ISerializedObject; Context: IBaseObject; out Obj: IBaseObject): ErrCode;
var
  Value: IString;
  Res: ErrCode;
  StringObj: IString;
begin
  CreateString(StringObj, 'string');
  Res := Serialized.ReadString(StringObj, Value);

  if OPENDAQ_FAILED(Res) then
    Exit(Res);

  Obj := Value;

  Result := OPENDAQ_SUCCESS;
end;

function SerializedObjectFactory(Serialized: ISerializedObject; Context: IBaseObject; out Obj: IBaseObject): ErrCode; cdecl;
var
  List: ISerializedList;
  Res: ErrCode;
  StringObj: IString;
begin
  CreateString(StringObj, 'list');
  Res := Serialized.ReadSerializedList(StringObj, List);

  if OPENDAQ_FAILED(Res) then
  begin
    Obj := nil;
    Exit(Res);
  end;

  Obj := List;

  Result := OPENDAQ_SUCCESS;
end;

function ListFactory(Serialized: ISerializedObject; Context: IBaseObject; out Obj: IBaseObject): ErrCode;
var
  List: IListObject;
  Res: ErrCode;
  StringObj: IString;
begin
  CreateString(StringObj, 'list');
  Res := Serialized.ReadList(StringObj, Context, List);

  if OPENDAQ_FAILED(Res) then
    Exit(Res);

  Obj := List;

  Result := OPENDAQ_SUCCESS;
end;

function HasKeyFactory(Serialized: ISerializedObject; Context: IBaseObject; out Obj: IBaseObject): ErrCode;
var
  HasKey: Boolean;
  Res: ErrCode;
  StringObj: IString;
  BoolObj: IBoolean;
begin
  CreateString(StringObj, 'str');
  Res := Serialized.HasKey(StringObj, HasKey);

  if OPENDAQ_FAILED(Res) then
    Exit(Res);

  CreateBoolean(BoolObj, HasKey);

  Obj := BoolObj;
  Result := OPENDAQ_SUCCESS;
end;

function KeyFactory(Serialized: ISerializedObject; Context: IBaseObject; out Obj: IBaseObject): ErrCode;
var
  Res: ErrCode;
  StringObj: IString;
  SerializedObj: ISerializedObject;
  Keys: IListObject;
begin
  CreateString(StringObj, 'object');
  Res := Serialized.ReadSerializedObject(StringObj, SerializedObj);

  if OPENDAQ_FAILED(Res) then
    Exit(Res);

  Res := SerializedObj.GetKeys(Keys);
  if OPENDAQ_FAILED(Res) then
    Exit(Res);

  Obj := Keys;
  Result := OPENDAQ_SUCCESS;
end;

function ObjectFactory(Serialized: ISerializedObject; Context: IBaseObject; out Obj: IBaseObject): ErrCode;
var
  Res: ErrCode;
  StringObj: IString;

begin
  CreateString(StringObj, 'doesNotExist');
  Res := Serialized.ReadObject(StringObj, Context, Obj);

  if OPENDAQ_FAILED(Res) then
    Exit(Res);

  Result := OPENDAQ_SUCCESS;
end;

function SerializedObjectErrorFactory(Serialized: ISerializedObject; Context: IBaseObject; out Obj: IBaseObject): ErrCode;
var
  Res: ErrCode;
  StringObj: IString;
  SerializedObj: ISerializedObject;
begin
  CreateString(StringObj, 'object');
  Res := Serialized.ReadSerializedObject(StringObj, SerializedObj); // ReadObject(StringObj, Obj);

  if OPENDAQ_FAILED(Res) then
    Exit(Res);

  Result := OPENDAQ_SUCCESS;
end;

{ TTest_JsonSerializedList }

procedure TTest_JsonSerializedObject.Setup;
begin
  FId := 'test';
end;

procedure TTest_JsonSerializedObject.TearDown;
begin
  DaqUnregisterSerializerFactory(PAnsiChar(FId));
end;

procedure TTest_JsonSerializedObject.ReadIntPositive;
var
  Str: AnsiString;
  StringObj: IString;
  Deserializer: IDeserializer;
  IntObj: IInteger;
  IntVal: Int64;
begin
  CreateJsonDeserializer(Deserializer);
  DaqRegisterSerializerFactory(PAnsiChar(FId), IntFactory);

  Str := '{"__type":"' + FId + '","int":1}';
  CreateString(StringObj, PAnsiChar(Str));
  Deserializer.Deserialize(StringObj, nil, IBaseOBject(IntObj));

  IntObj.GetValue(IntVal);
  Assert.AreEqual(IntVal, Int64(1));
end;

procedure TTest_JsonSerializedObject.ReadIntNegative;
var
  Str: AnsiString;
  StringObj: IString;
  Deserializer: IDeserializer;
  IntObj: IInteger;
  IntVal: Int64;
begin
  CreateJsonDeserializer(Deserializer);
  DaqRegisterSerializerFactory(PAnsiChar(FId), IntFactory);

  Str := '{"__type":"' + FId + '","int":-1}';
  CreateString(StringObj, PAnsiChar(Str));
  Deserializer.Deserialize(StringObj, nil, IBaseOBject(IntObj));

  IntObj.GetValue(IntVal);
  Assert.AreEqual(IntVal, Int64(-1));
end;

procedure TTest_JsonSerializedObject.ReadIntInvalidType;
var
  Str: AnsiString;
  StringObj: IString;
  Deserializer: IDeserializer;
  IntObj: IInteger;
begin
  CreateJsonDeserializer(Deserializer);
  DaqRegisterSerializerFactory(PAnsiChar(FId), IntFactory);

  Str := '{"__type":"' + FId + '","int":1.0}';
  CreateString(StringObj, PAnsiChar(Str));
  Assert.AreEqual(Deserializer.Deserialize(StringObj, nil, IBaseOBject(IntObj)), OPENDAQ_ERR_INVALIDTYPE);

end;

procedure TTest_JsonSerializedObject.ReadNonExistentInt;
var
  Str: AnsiString;
  StringObj: IString;
  Deserializer: IDeserializer;
  IntObj: IInteger;
begin
  CreateJsonDeserializer(Deserializer);
  DaqRegisterSerializerFactory(PAnsiChar(FId), IntFactory);

  Str := '{"__type":"' + FId + '","integer":1}';
  CreateString(StringObj, PAnsiChar(Str));
  Assert.AreEqual(Deserializer.Deserialize(StringObj, nil, IBaseOBject(IntObj)), OPENDAQ_ERR_NOTFOUND);
end;

procedure TTest_JsonSerializedObject.ReadFloatPositive;
var
  Str: AnsiString;
  StringObj: IString;
  Deserializer: IDeserializer;
  FloatObj: IFloat;
  FloatVal: Double;
begin
  CreateJsonDeserializer(Deserializer);
  DaqRegisterSerializerFactory(PAnsiChar(FId), FloatFactory);

  Str := '{"__type":"' + FId + '","float":1.5}';
  CreateString(StringObj, PAnsiChar(Str));
  Deserializer.Deserialize(StringObj, nil, IBaseOBject(FloatObj));

  FloatObj.GetValue(FloatVal);
  Assert.AreEqual(FloatVal, Double(1.5));

end;

procedure TTest_JsonSerializedObject.ReadFloatNegative;
var
  Str: AnsiString;
  StringObj: IString;
  Deserializer: IDeserializer;
  FloatObj: IFloat;
  FloatVal: Double;
begin
  CreateJsonDeserializer(Deserializer);
  DaqRegisterSerializerFactory(PAnsiChar(FId), FloatFactory);

  Str := '{"__type":"' + FId + '","float":-1.5}';
  CreateString(StringObj, PAnsiChar(Str));
  Deserializer.Deserialize(StringObj, nil, IBaseOBject(FloatObj));

  FloatObj.GetValue(FloatVal);
  Assert.AreEqual(FloatVal, Double(-1.5));
end;

procedure TTest_JsonSerializedObject.ReadNonExistentFloat;
var
  Str: AnsiString;
  StringObj: IString;
  Deserializer: IDeserializer;
  FloatObj: IFloat;
begin
  CreateJsonDeserializer(Deserializer);
  DaqRegisterSerializerFactory(PAnsiChar(FId), FloatFactory);

  Str := '{"__type":"' + FId + '","floating":1.0}';
  CreateString(StringObj, PAnsiChar(Str));
  Assert.AreEqual(Deserializer.Deserialize(StringObj, nil, IBaseObject(FloatObj)), OPENDAQ_ERR_NOTFOUND);
end;

procedure TTest_JsonSerializedObject.ReadFloatInvalidType;
var
  Str: AnsiString;
  StringObj: IString;
  Deserializer: IDeserializer;
  FloatObj: IFloat;
begin
  CreateJsonDeserializer(Deserializer);
  DaqRegisterSerializerFactory(PAnsiChar(FId), FloatFactory);

  Str := '{"__type":"' + FId + '","float":1}';
  CreateString(StringObj, PAnsiChar(Str));
  Assert.AreEqual(Deserializer.Deserialize(StringObj, nil, IBaseOBject(FloatObj)), OPENDAQ_ERR_INVALIDTYPE);
end;

procedure TTest_JsonSerializedObject.ReadBoolTrue;
var
  Str: AnsiString;
  StringObj: IString;
  Deserializer: IDeserializer;
  BoolObj: IBoolean;
  BoolVal: Boolean;
begin
  CreateJsonDeserializer(Deserializer);
  DaqRegisterSerializerFactory(PAnsiChar(FId), BoolFactory);

  Str := '{"__type":"' + FId + '","bool":true}';
  CreateString(StringObj, PAnsiChar(Str));
  Deserializer.Deserialize(StringObj, nil, IBaseOBject(BoolObj));

  BoolObj.GetValue(BoolVal);
  Assert.AreEqual(BoolVal, True);
end;

procedure TTest_JsonSerializedObject.ReadBoolFalse;
var
  Str: AnsiString;
  StringObj: IString;
  Deserializer: IDeserializer;
  BoolObj: IBoolean;
  BoolVal: Boolean;
begin
  CreateJsonDeserializer(Deserializer);
  DaqRegisterSerializerFactory(PAnsiChar(FId), BoolFactory);

  Str := '{"__type":"' + FId + '","bool":false}';
  CreateString(StringObj, PAnsiChar(Str));
  Deserializer.Deserialize(StringObj, nil, IBaseOBject(BoolObj));

  BoolObj.GetValue(BoolVal);
  Assert.AreEqual(BoolVal, False);
end;

procedure TTest_JsonSerializedObject.ReadBoolInvalidType;
var
  Str: AnsiString;
  StringObj: IString;
  Deserializer: IDeserializer;
  BoolObj: IBoolean;
begin
  CreateJsonDeserializer(Deserializer);
  DaqRegisterSerializerFactory(PAnsiChar(FId), BoolFactory);

  Str := '{"__type":"'+ FId + '","bool":1}';
  CreateString(StringObj, PAnsiChar(Str));
  Assert.AreEqual(Deserializer.Deserialize(StringObj, nil, IBaseObject(BoolObj)), OPENDAQ_ERR_INVALIDTYPE);
end;

procedure TTest_JsonSerializedObject.ReadNonExistentBool;
var
  Str: AnsiString;
  StringObj: IString;
  Deserializer: IDeserializer;
  BoolObj: IBoolean;
begin
  CreateJsonDeserializer(Deserializer);
  DaqRegisterSerializerFactory(PAnsiChar(FId), BoolFactory);

  Str := '{"__type":"' + FId + '","boolean":true}';
  CreateString(StringObj, PAnsiChar(Str));
  Assert.AreEqual(Deserializer.Deserialize(StringObj, nil, IBaseOBject(BoolObj)), OPENDAQ_ERR_NOTFOUND);
end;

procedure TTest_JsonSerializedObject.ReadString;
var
  Str: AnsiString;
  StringObj: IString;
  Deserializer: IDeserializer;
  StrObj: IString;
  StrVal: PAnsiChar;
begin
  CreateJsonDeserializer(Deserializer);
  DaqRegisterSerializerFactory(PAnsiChar(FId), StringFactory);

  Str := '{"__type":"' + FId + '","string":"Test"}';
  CreateString(StringObj, PAnsiChar(Str));
  Deserializer.Deserialize(StringObj, nil, IBaseOBject(StrObj));

  StrObj.GetCharPtr(@StrVal);
  Assert.AreEqual(string(StrVal), 'Test');
end;

procedure TTest_JsonSerializedObject.ReadStringInvalidType;
var
  Str: AnsiString;
  StringObj: IString;
  Deserializer: IDeserializer;
  StrObj: IString;
begin
  CreateJsonDeserializer(Deserializer);
  DaqRegisterSerializerFactory(PAnsiChar(FId), StringFactory);

  Str := '{"__type":"' + FId + '","string":0}';
  CreateString(StringObj, PAnsiChar(Str));

  Assert.AreEqual(Deserializer.Deserialize(StringObj, nil, IBaseObject(StrObj)), OPENDAQ_ERR_INVALIDTYPE);
end;

procedure TTest_JsonSerializedObject.ReadNonExistentString;
var
  Str: AnsiString;
  StringObj: IString;
  Deserializer: IDeserializer;
  StrObj: IString;
begin
  CreateJsonDeserializer(Deserializer);
  DaqRegisterSerializerFactory(PAnsiChar(FId), StringFactory);

  Str := '{"__type":"' + FId + '","str":"Test"}';
  CreateString(StringObj, PAnsiChar(Str));

  Assert.AreEqual(Deserializer.Deserialize(StringObj, nil, IBaseObject(StrObj)), OPENDAQ_ERR_NOTFOUND);
end;

procedure TTest_JsonSerializedObject.TestHasKeyTrue;
var
  Str: AnsiString;
  StringObj: IString;
  Deserializer: IDeserializer;
  BoolObj: IBoolean;
  BoolVal: Boolean;

begin
  CreateJsonDeserializer(Deserializer);
  DaqRegisterSerializerFactory(PAnsiChar(FId), HasKeyFactory);

  Str := '{"__type":"' + FId + '","str":"Test"}';
  CreateString(StringObj, PAnsiChar(Str));
  Deserializer.Deserialize(StringObj, nil, IBaseObject(BoolObj));
  BoolObj.GetValue(BoolVal);

  Assert.AreEqual(BoolVal, True);
end;

procedure TTest_JsonSerializedObject.TestHasKeyFalse;
var
  Str: AnsiString;
  StringObj: IString;
  Deserializer: IDeserializer;
  BoolObj: IBoolean;
  BoolVal: Boolean;

begin
  CreateJsonDeserializer(Deserializer);
  DaqRegisterSerializerFactory(PAnsiChar(FId), HasKeyFactory);

  Str := '{"__type":"' + FId + '","string":"Test"}';
  CreateString(StringObj, PAnsiChar(Str));
  Deserializer.Deserialize(StringObj, nil, IBaseObject(BoolObj));
  BoolObj.GetValue(BoolVal);

  Assert.AreEqual(BoolVal, False);
end;

procedure TTest_JsonSerializedObject.ReadEmptyObjectKeys;
var
  Str: AnsiString;
  StringObj: IString;
  Deserializer: IDeserializer;
  Keys: IListObject;
  Count: SizeT;
begin
  CreateJsonDeserializer(Deserializer);
  DaqRegisterSerializerFactory(PAnsiChar(FId), KeyFactory);

  Str := '{"__type":"' + FId + '","object":{}}';
  CreateString(StringObj, PAnsiChar(Str));
  Deserializer.Deserialize(StringObj, nil, IBaseObject(Keys));
  Keys.GetCount(Count);

  Assert.AreEqual(Count, SizeT(0));
end;

procedure TTest_JsonSerializedObject.ReadObjectKeys;
var
  Str: AnsiString;
  StringObj: IString;
  Deserializer: IDeserializer;
  Keys: IListObject;
  Count: SizeT;
begin
  CreateJsonDeserializer(Deserializer);
  DaqRegisterSerializerFactory(PAnsiChar(FId), KeyFactory);

  Str := '{"__type":"' + FId + '","object":{"key1":1,"key2":0.0,"key3":false,"key4":"string","key5":[],"key6":{}}}';
  CreateString(StringObj, PAnsiChar(Str));
  Deserializer.Deserialize(StringObj, nil, IBaseObject(Keys));
  Keys.GetCount(Count);

  Assert.AreEqual(Count, SizeT(6));
end;

procedure TTest_JsonSerializedObject.ReadNonExistentObject;
var
  Str: AnsiString;
  StringObj: IString;
  Deserializer: IDeserializer;
  Obj: IBaseObject;
begin
  CreateJsonDeserializer(Deserializer);
  DaqRegisterSerializerFactory(PAnsiChar(FId), ObjectFactory);

  Str := '{"__type":"' + FId + '","object":{}}';
  CreateString(StringObj, PAnsiChar(Str));
  Assert.AreEqual(Deserializer.Deserialize(StringObj, nil, Obj), OPENDAQ_ERR_NOTFOUND);
end;

procedure TTest_JsonSerializedObject.ReadSerializedObjectInvalidType;
var
  Str: AnsiString;
  StringObj: IString;
  Deserializer: IDeserializer;
  Obj: IBaseObject;
begin
  CreateJsonDeserializer(Deserializer);
  DaqRegisterSerializerFactory(PAnsiChar(FId), SerializedObjectErrorFactory);

  Str := '{"__type":"' + FId + '","object":[]}';
  CreateString(StringObj, PAnsiChar(Str));
  Assert.AreEqual(Deserializer.Deserialize(StringObj, nil, Obj), OPENDAQ_ERR_INVALIDTYPE);
end;

procedure TTest_JsonSerializedObject.ReadNonExistentSerializedObject;
var
  Str: AnsiString;
  StringObj: IString;
  Deserializer: IDeserializer;
  Obj: IBaseObject;
begin
  CreateJsonDeserializer(Deserializer);
  DaqRegisterSerializerFactory(PAnsiChar(FId), SerializedObjectErrorFactory);

  Str := '{"__type":"' + FId + '","obj":{}}';
  CreateString(StringObj, PAnsiChar(Str));
  Assert.AreEqual(Deserializer.Deserialize(StringObj, nil, Obj), OPENDAQ_ERR_NOTFOUND);
end;

procedure TTest_JsonSerializedObject.ReadEmptySerializedList;
var
  Str: AnsiString;
  StringObj: IString;
  Deserializer: IDeserializer;
  SerializedList: ISerializedList;
  Count: SizeT;
begin
  CreateJsonDeserializer(Deserializer);
  DaqRegisterSerializerFactory(PAnsiChar(FId), SerializedObjectFactory);

  Str := '{"__type":"' + FId + '","list":[]}';
  CreateString(StringObj, PAnsiChar(Str));
  Deserializer.Deserialize(StringObj, nil, IBaseObject(SerializedList));
  SerializedList.GetCount(Count);
  Assert.AreEqual(Count, SizeT(0));
end;

procedure TTest_JsonSerializedObject.ReadNonExistingSerializedList;
var
  Str: AnsiString;
  StringObj: IString;
  Deserializer: IDeserializer;
  SerializedList: ISerializedList;
begin
  CreateJsonDeserializer(Deserializer);
  DaqRegisterSerializerFactory(PAnsiChar(FId), SerializedObjectFactory);

  Str := '{"__type":"' + FId + '","array":[]}';
  CreateString(StringObj, PAnsiChar(Str));
  Assert.AreEqual(Deserializer.Deserialize(StringObj, nil, IBaseObject(SerializedList)), OPENDAQ_ERR_NOTFOUND);
end;

procedure TTest_JsonSerializedObject.ReadSerializedListInvalidType;
var
  Str: AnsiString;
  StringObj: IString;
  Deserializer: IDeserializer;
  SerializedList: ISerializedList;
begin
  CreateJsonDeserializer(Deserializer);
  DaqRegisterSerializerFactory(PAnsiChar(FId), SerializedObjectFactory);

  Str := '{"__type":"' + FId + '","list":false}';
  CreateString(StringObj, PAnsiChar(Str));
  Assert.AreEqual(Deserializer.Deserialize(StringObj, nil, IBaseObject(SerializedList)), OPENDAQ_ERR_INVALIDTYPE);
end;

procedure TTest_JsonSerializedObject.ReadEmptyList;
var
  Str: AnsiString;
  StringObj: IString;
  Deserializer: IDeserializer;
  List: IListObject;
  Count: SizeT;
begin
  CreateJsonDeserializer(Deserializer);
  DaqRegisterSerializerFactory(PAnsiChar(FId), ListFactory);

  Str := '{"__type":"' + FId + '","list":[]}';
  CreateString(StringObj, PAnsiChar(Str));
  Deserializer.Deserialize(StringObj, nil, IBaseObject(List));
  List.GetCount(Count);
  Assert.AreEqual(Count, SizeT(0));
end;

procedure TTest_JsonSerializedObject.ReadNonExistingList;
var
  Str: AnsiString;
  StringObj: IString;
  Deserializer: IDeserializer;
  List: IListObject;
begin
  CreateJsonDeserializer(Deserializer);
  DaqRegisterSerializerFactory(PAnsiChar(FId), ListFactory);

  Str := '{"__type":"' + FId + '","array":[]}';
  CreateString(StringObj, PAnsiChar(Str));
  Assert.AreEqual(Deserializer.Deserialize(StringObj, nil, IBaseObject(List)), OPENDAQ_ERR_NOTFOUND);
end;

procedure TTest_JsonSerializedObject.ReadListInvalidType;
var
  Str: AnsiString;
  StringObj: IString;
  Deserializer: IDeserializer;
  List: IListObject;
begin
  CreateJsonDeserializer(Deserializer);
  DaqRegisterSerializerFactory(PAnsiChar(FId), ListFactory);

  Str := '{"__type":"' + FId + '","list":false}';
  CreateString(StringObj, PAnsiChar(Str));
  Assert.AreEqual(Deserializer.Deserialize(StringObj, nil, IBaseObject(List)), OPENDAQ_ERR_INVALIDTYPE);
end;

initialization
  TDSUnitTestEngine.RegisterUnitTest(TTest_JsonSerializedObject);
end.
