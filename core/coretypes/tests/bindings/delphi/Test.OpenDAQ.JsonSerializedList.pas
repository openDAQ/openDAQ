unit Test.OpenDAQ.JsonSerializedList;

interface

uses
  DunitX.TestFramework, DS.UT.DSUnitTestUnit, OpenDAQ.CoreTypes;

type
  [TestFixture]
  TTest_JsonSerializedList = class(TDSUnitTest)
  private
  public
    [Setup]
    procedure Setup; override;
    [TearDown]
    procedure TearDown; override;
    [Test]
    procedure ReadSerializedObjectInvalidValue;
    [Test]
    procedure ReadSerializedObjectOutOfRange;
    [Test]
    procedure ReadSerializedObjectNull;
    [Test]
    procedure ReadListInvalidType;
    [Test]
    procedure ReadList;
    [Test]
    procedure ReadListOutOfRange;
    [Test]
    procedure ReadSerializedListInvalidType;
    [Test]
    procedure ReadSerializedList;
    [Test]
    procedure ReadSerializedListOutOfRange;
    [Test]
    procedure ReadBoolTrue;
    [Test]
    procedure ReadBoolFalse;
    [Test]
    procedure ReadBoolInvalidType;
    [Test]
    procedure ReadBoolOutOfRange;
    [Test]
    procedure ReadIntPositive;
    [Test]
    procedure ReadIntNegative;
    [Test]
    procedure ReadIntInvalidType;
    [Test]
    procedure ReadIntOutOfRange;
    [Test]
    procedure ReadFloatPositive;
    [Test]
    procedure ReadFloatNegative;
    [Test]
    procedure ReadNonExistentFloat;
    [Test]
    procedure ReadFloatInvalidType;
    [Test]
    procedure ReadString;
    [Test]
    procedure ReadStringInvalidType;
    [Test]
    procedure ReadStringOutOfRange;
    [Test]
    procedure ReadObjectOutOfRange;
    [Test]
    procedure ReadObject;
  end;

  function ObjectFactory(Serialized: ISerializedObject; Context: IBaseObject; out Obj: IBaseObject): ErrCode; cdecl;
  function SerializedObjectFactory(Serialized: ISerializedObject; Context: IBaseObject; out Obj: IBaseObject): ErrCode; cdecl;
  function ListFactory(Serialized: ISerializedObject; Context: IBaseObject; out Obj: IBaseObject): ErrCode; cdecl;
  function BoolFactory(Serialized: ISerializedObject; Context: IBaseObject; out Obj: IBaseObject): ErrCode; cdecl;
  function IntFactory(Serialized: ISerializedObject; Context: IBaseObject; out Obj: IBaseObject): ErrCode; cdecl;
  function FloatFactory(Serialized: ISerializedObject; Context: IBaseObject; out Obj: IBaseObject): ErrCode; cdecl;
  function StringFactory(Serialized: ISerializedObject; Context: IBaseObject; out Obj: IBaseObject): ErrCode; cdecl;
  function SerializedListFactory(Serialized: ISerializedObject; Context: IBaseObject; out Obj: IBaseObject): ErrCode; cdecl;
  function EmptyFactory(Serialized: ISerializedObject; Context: IBaseObject; out Obj: IBaseObject): ErrCode; cdecl;

implementation
uses
  DS.UT.DSUnitTestEngineUnit, WinApi.Windows, OpenDAQ.CoreTypes.Errors, SysUtils;

function ObjectFactory(Serialized: ISerializedObject; Context: IBaseObject; out Obj: IBaseObject): ErrCode;
var
  SerializedList: ISerializedList;
  Res: ErrCode;
  StringObj: IString;
  BaseObj: IBaseObject;
begin
  CreateString(StringObj, 'list');
  Res := Serialized.ReadSerializedList(StringObj, SerializedList);

  if OPENDAQ_FAILED(Res) then
    Exit(Res);

  Res := SerializedList.ReadObject(Context, BaseObj);

  if OPENDAQ_FAILED(Res) then
    Exit(Res);

  Obj := BaseObj;

  Result := OPENDAQ_SUCCESS;
end;

function SerializedObjectFactory(Serialized: ISerializedObject; Context: IBaseObject; out Obj: IBaseObject): ErrCode;
var
  List: ISerializedList;
  Res: ErrCode;
  StringObj: IString;
  SerObj: ISerializedObject;
begin
  CreateString(StringObj, 'list');
  Res := Serialized.ReadSerializedList(StringObj, List);

  if OPENDAQ_FAILED(Res) then
    Exit(Res);

  Res := List.ReadSerializedObject(SerObj);

  if OPENDAQ_FAILED(Res) then
  begin
    Obj := nil;
    Exit(Res);
  end;

  Obj := SerObj;
  Result := OPENDAQ_SUCCESS;
end;

function ListFactory(Serialized: ISerializedObject; Context: IBaseObject; out Obj: IBaseObject): ErrCode;
var
  SerializedList: ISerializedList;
  Res: ErrCode;
  StringObj: IString;
  List: IListObject;
begin
  CreateString(StringObj, 'list');
  Res := Serialized.ReadSerializedList(StringObj, SerializedList);

  if OPENDAQ_FAILED(Res) then
    Exit(Res);

  Res := SerializedList.ReadList(nil, List);

  if OPENDAQ_FAILED(Res) then
    Exit(Res);

  Obj := List;

  Result := OPENDAQ_SUCCESS;
end;

function BoolFactory(Serialized: ISerializedObject; Context: IBaseObject; out Obj: IBaseObject): ErrCode;
var
  SerializedList: ISerializedList;
  Res: ErrCode;
  StringObj: IString;
  Bool: Boolean;
  BoolObj: IBoolean;
begin
  CreateString(StringObj, 'list');
  Res := Serialized.ReadSerializedList(StringObj, SerializedList);

  if OPENDAQ_FAILED(Res) then
    Exit(Res);

  Res := SerializedList.ReadBool(Bool);

  if OPENDAQ_FAILED(Res) then
    Exit(Res);

  CreateBoolean(BoolObj, Bool);
  Obj := BoolObj;

  Result := OPENDAQ_SUCCESS;
end;

function IntFactory(Serialized: ISerializedObject; Context: IBaseObject; out Obj: IBaseObject): ErrCode;
var
  SerializedList: ISerializedList;
  Res: ErrCode;
  StringObj: IString;
  Int: Int64;
  IntObj: IInteger;
begin
  CreateString(StringObj, 'list');
  Res := Serialized.ReadSerializedList(StringObj, SerializedList);

  if OPENDAQ_FAILED(Res) then
    Exit(Res);

  Res := SerializedList.ReadInt(Int);

  if OPENDAQ_FAILED(Res) then
    Exit(Res);

  CreateInteger(IntObj, Int);
  Obj := IntObj;

  Result := OPENDAQ_SUCCESS;
end;

function FloatFactory(Serialized: ISerializedObject; Context: IBaseObject; out Obj: IBaseObject): ErrCode;
var
  SerializedList: ISerializedList;
  Res: ErrCode;
  StringObj: IString;
  Float: Double;
  FloatObj: IFloat;
begin
  CreateString(StringObj, 'list');
  Res := Serialized.ReadSerializedList(StringObj, SerializedList);

  if OPENDAQ_FAILED(Res) then
    Exit(Res);

  Res := SerializedList.ReadFloat(Float);

  if OPENDAQ_FAILED(Res) then
    Exit(Res);

  CreateFloat(FloatObj, Float);
  Obj := FloatObj;

  Result := OPENDAQ_SUCCESS;
end;

function StringFactory(Serialized: ISerializedObject; Context: IBaseObject; out Obj: IBaseObject): ErrCode;
var
  SerializedList: ISerializedList;
  Res: ErrCode;
  StringObj: IString;
  StrObj: IString;
begin
  CreateString(StringObj, 'list');
  Res := Serialized.ReadSerializedList(StringObj, SerializedList);

  if OPENDAQ_FAILED(Res) then
    Exit(Res);

  Res := SerializedList.ReadString(StrObj);

  if OPENDAQ_FAILED(Res) then
    Exit(Res);

  Obj := StrObj;

  Result := OPENDAQ_SUCCESS;
end;

function SerializedListFactory(Serialized: ISerializedObject; Context: IBaseObject; out Obj: IBaseObject): ErrCode;
var
  SerializedList: ISerializedList;
  Res: ErrCode;
  StringObj: IString;
  InnerList: ISerializedList;
begin
  CreateString(StringObj, 'list');
  Res := Serialized.ReadSerializedList(StringObj, SerializedList);

  if OPENDAQ_FAILED(Res) then
    Exit(Res);

  Res := SerializedList.ReadSerializedList(InnerList);

  if OPENDAQ_FAILED(Res) then
    Exit(Res);

  Obj := InnerList;

  Result := OPENDAQ_SUCCESS;
end;

function EmptyFactory(Serialized: ISerializedObject; Context: IBaseObject; out Obj: IBaseObject): ErrCode;
begin
  Obj := nil;
  Result := OPENDAQ_SUCCESS;
end;

{ TTest_JsonSerializedList }

procedure TTest_JsonSerializedList.Setup;
begin
end;

procedure TTest_JsonSerializedList.TearDown;
begin
end;

procedure TTest_JsonSerializedList.ReadSerializedObjectInvalidValue;
var
  Id: AnsiString;
  Str: AnsiString;
  StringObj: IString;
  Deserializer: IDeserializer;
  BaseObj: IBaseObject;
  Res: ErrCode;
begin
  Id := 'test';
  CreateJsonDeserializer(Deserializer);
  Res := DaqRegisterSerializerFactory(PAnsiChar(Id), SerializedObjectFactory);
  Assert.AreEqual(Res, OPENDAQ_SUCCESS);

  Str := '{"__type":"test","list":[false]}';
  CreateString(StringObj, PAnsiChar(Str));
  Assert.AreEqual(Deserializer.Deserialize(StringObj, nil, BaseObj), OPENDAQ_ERR_INVALIDTYPE);
  DaqUnregisterSerializerFactory(PAnsiChar(Id));
end;

procedure TTest_JsonSerializedList.ReadSerializedObjectOutOfRange;
var
  Id: AnsiString;
  Str: AnsiString;
  StringObj: IString;
  Deserializer: IDeserializer;
  BaseObj: IBaseObject;
begin
  Id := 'test';
  CreateJsonDeserializer(Deserializer);
  DaqRegisterSerializerFactory(PAnsiChar(Id), serializedObjectFactory);

  Str := '{"__type":"test","list":[]}';
  CreateString(StringObj, PAnsiChar(Str));
  Assert.AreEqual(Deserializer.Deserialize(StringObj, nil, BaseObj), OPENDAQ_ERR_OUTOFRANGE);
  DaqUnregisterSerializerFactory(PAnsiChar(Id));
end;

procedure TTest_JsonSerializedList.ReadSerializedObjectNull;
var
  Id: AnsiString;
  Str: AnsiString;
  StringObj: IString;
  Deserializer: IDeserializer;
  BaseObj: IBaseObject;
begin
  Id := 'test';
  CreateJsonDeserializer(Deserializer);
  DaqRegisterSerializerFactory(PAnsiChar(Id), SerializedObjectFactory);

  Str := '{"__type":"test","list":[{"__type":1}]}';
  CreateString(StringObj, PAnsiChar(Str));
  Deserializer.Deserialize(StringObj, nil, BaseObj);

  DaqUnregisterSerializerFactory(PAnsiChar(Id));
  Assert.IsTrue(Assigned(BaseObj));
end;

procedure TTest_JsonSerializedList.ReadListInvalidType;
var
  Id: AnsiString;
  Str: AnsiString;
  StringObj: IString;
  Deserializer: IDeserializer;
  BaseObj: IBaseObject;
begin
  Id := 'test';
  CreateJsonDeserializer(Deserializer);
  DaqRegisterSerializerFactory(PAnsiChar(Id), ListFactory);

  Str := '{"__type":"test","list":[false]}';
  CreateString(StringObj, PAnsiChar(Str));
  Assert.AreEqual(Deserializer.Deserialize(StringObj, nil, BaseObj), OPENDAQ_ERR_INVALIDTYPE);

  DaqUnregisterSerializerFactory(PAnsiChar(Id));
end;

procedure TTest_JsonSerializedList.ReadList;
var
  Id: AnsiString;
  Str: AnsiString;
  StringObj: IString;
  Deserializer: IDeserializer;
  List: IListObject;
  Count: SizeT;
begin
  Id := 'test';
  CreateJsonDeserializer(Deserializer);
  DaqRegisterSerializerFactory(PAnsiChar(Id), ListFactory);

  Str := '{"__type":"test","list":[[]]}';
  CreateString(StringObj, PAnsiChar(Str));

  Deserializer.Deserialize(StringObj, nil, IBaseObject(List));
  List.GetCount(Count);
  Assert.AreEqual(Count, SizeT(0));

  DaqUnregisterSerializerFactory(PAnsiChar(Id));
end;

procedure TTest_JsonSerializedList.ReadListOutOfRange;
var
  Id: AnsiString;
  Str: AnsiString;
  StringObj: IString;
  Deserializer: IDeserializer;
  List: IListObject;
begin
  Id := 'test';
  CreateJsonDeserializer(Deserializer);
  DaqRegisterSerializerFactory(PAnsiChar(Id), ListFactory);

  Str := '{"__type":"test","list":[]}';
  CreateString(StringObj, PAnsiChar(Str));

  Assert.AreEqual(Deserializer.Deserialize(StringObj, nil, IBaseObject(List)), OPENDAQ_ERR_OUTOFRANGE);

  DaqUnregisterSerializerFactory(PAnsiChar(Id));
end;

procedure TTest_JsonSerializedList.ReadSerializedListInvalidType;
var
  Id: AnsiString;
  Str: AnsiString;
  StringObj: IString;
  Deserializer: IDeserializer;
  List: ISerializedList;
begin
  Id := 'test';
  CreateJsonDeserializer(Deserializer);
  DaqRegisterSerializerFactory(PAnsiChar(Id), SerializedListFactory);

  Str := '{"__type":"test","list":[{}]}';
  CreateString(StringObj, PAnsiChar(Str));

  Deserializer.Deserialize(StringObj, nil, IBaseObject(List));

  DaqUnregisterSerializerFactory(PAnsiChar(Id));
end;

procedure TTest_JsonSerializedList.ReadSerializedList;
var
  Id: AnsiString;
  Str: AnsiString;
  StringObj: IString;
  Deserializer: IDeserializer;
  List: ISerializedList;
  Count: SizeT;
begin
  Id := 'test';
  CreateJsonDeserializer(Deserializer);
  DaqRegisterSerializerFactory(PAnsiChar(Id), SerializedListFactory);

  Str := '{"__type":"test","list":[[]]}';
  CreateString(StringObj, PAnsiChar(Str));

  Deserializer.Deserialize(StringObj, nil, IBaseObject(List));
  List.GetCount(Count);
  Assert.AreEqual(Count, SizeT(0));

  DaqUnregisterSerializerFactory(PAnsiChar(Id));
end;

procedure TTest_JsonSerializedList.ReadSerializedListOutOfRange;
var
  Id: AnsiString;
  Str: AnsiString;
  StringObj: IString;
  Deserializer: IDeserializer;
  List: ISerializedList;
begin
  Id := 'test';
  CreateJsonDeserializer(Deserializer);
  DaqRegisterSerializerFactory(PAnsiChar(Id), SerializedListFactory);

  Str := '{"__type":"test","list":[]}';
  CreateString(StringObj, PAnsiChar(Str));

  Assert.AreEqual(Deserializer.Deserialize(StringObj, nil, IBaseObject(List)), OPENDAQ_ERR_OUTOFRANGE);

  DaqUnregisterSerializerFactory(PAnsiChar(Id));
end;

procedure TTest_JsonSerializedList.ReadBoolTrue;
var
  Id: AnsiString;
  Str: AnsiString;
  StringObj: IString;
  Deserializer: IDeserializer;
  Bool: IBoolean;
  BoolVal: Boolean;
begin
  Id := 'test';
  CreateJsonDeserializer(Deserializer);
  DaqRegisterSerializerFactory(PAnsiChar(Id), BoolFactory);

  Str := '{"__type":"test","list":[true]}';
  CreateString(StringObj, PAnsiChar(Str));

  Deserializer.Deserialize(StringObj, nil, IBaseObject(Bool));
  Bool.GetValue(BoolVal);

  Assert.AreEqual(BoolVal, True);
  DaqUnregisterSerializerFactory(PAnsiChar(Id));
end;

procedure TTest_JsonSerializedList.ReadBoolFalse;
var
  Id: AnsiString;
  Str: AnsiString;
  StringObj: IString;
  Deserializer: IDeserializer;
  Bool: IBoolean;
  BoolVal: Boolean;
begin
  Id := 'test';
  CreateJsonDeserializer(Deserializer);
  DaqRegisterSerializerFactory(PAnsiChar(Id), BoolFactory);

  Str := '{"__type":"test","list":[false]}';
  CreateString(StringObj, PAnsiChar(Str));

  Deserializer.Deserialize(StringObj, nil, IBaseObject(Bool));
  Bool.GetValue(BoolVal);

  Assert.AreEqual(BoolVal, False);
  DaqUnregisterSerializerFactory(PAnsiChar(Id));
end;

procedure TTest_JsonSerializedList.ReadBoolInvalidType;
var
  Id: AnsiString;
  Str: AnsiString;
  StringObj: IString;
  Deserializer: IDeserializer;
  Bool: IBoolean;
begin
  Id := 'test';
  CreateJsonDeserializer(Deserializer);
  DaqRegisterSerializerFactory(PAnsiChar(Id), BoolFactory);

  Str := '{"__type":"test","list":[1]}';
  CreateString(StringObj, PAnsiChar(Str));

  Assert.AreEqual(Deserializer.Deserialize(StringObj, nil, IBaseObject(Bool)), OPENDAQ_ERR_INVALIDTYPE);

  DaqUnregisterSerializerFactory(PAnsiChar(Id));
end;

procedure TTest_JsonSerializedList.ReadBoolOutOfRange;
var
  Id: AnsiString;
  Str: AnsiString;
  StringObj: IString;
  Deserializer: IDeserializer;
  Bool: IBoolean;
begin
  Id := 'test';
  CreateJsonDeserializer(Deserializer);
  DaqRegisterSerializerFactory(PAnsiChar(Id), BoolFactory);

  Str := '{"__type":"test","list":[]}';
  CreateString(StringObj, PAnsiChar(Str));

  Assert.AreEqual(Deserializer.Deserialize(StringObj, nil, IBaseObject(Bool)), OPENDAQ_ERR_OUTOFRANGE);

  DaqUnregisterSerializerFactory(PAnsiChar(Id));
end;

procedure TTest_JsonSerializedList.ReadIntPositive;
var
  Id: AnsiString;
  Str: AnsiString;
  StringObj: IString;
  Deserializer: IDeserializer;
  Int: IInteger;
  IntVal: Int64;
begin
  Id := 'test';
  CreateJsonDeserializer(Deserializer);
  DaqRegisterSerializerFactory(PAnsiChar(Id), IntFactory);

  Str := '{"__type":"test","list":[1]}';
  CreateString(StringObj, PAnsiChar(Str));

  Deserializer.Deserialize(StringObj, nil, IBaseObject(Int));
  Int.GetValue(IntVal);

  Assert.AreEqual(IntVal, Int64(1));
  DaqUnregisterSerializerFactory(PAnsiChar(Id));
end;

procedure TTest_JsonSerializedList.ReadIntNegative;
var
  Id: AnsiString;
  Str: AnsiString;
  StringObj: IString;
  Deserializer: IDeserializer;
  Int: IInteger;
  IntVal: Int64;
begin
  Id := 'test';
  CreateJsonDeserializer(Deserializer);
  DaqRegisterSerializerFactory(PAnsiChar(Id), IntFactory);

  Str := '{"__type":"test","list":[-1]}';
  CreateString(StringObj, PAnsiChar(Str));

  Deserializer.Deserialize(StringObj, nil, IBaseObject(Int));
  Int.GetValue(IntVal);

  Assert.AreEqual(IntVal, Int64(-1));
  DaqUnregisterSerializerFactory(PAnsiChar(Id));
end;

procedure TTest_JsonSerializedList.ReadIntInvalidType;
var
  Id: AnsiString;
  Str: AnsiString;
  StringObj: IString;
  Deserializer: IDeserializer;
  Int: IInteger;
begin
  Id := 'test';
  CreateJsonDeserializer(Deserializer);
  DaqRegisterSerializerFactory(PAnsiChar(Id), IntFactory);

  Str := '{"__type":"test","list":[1.0]}';
  CreateString(StringObj, PAnsiChar(Str));

  Assert.AreEqual(Deserializer.Deserialize(StringObj, nil, IBaseObject(Int)), OPENDAQ_ERR_INVALIDTYPE);

  DaqUnregisterSerializerFactory(PAnsiChar(Id));
end;

procedure TTest_JsonSerializedList.ReadIntOutOfRange;
var
  Id: AnsiString;
  Str: AnsiString;
  StringObj: IString;
  Deserializer: IDeserializer;
  Int: IInteger;
begin
  Id := 'test';
  CreateJsonDeserializer(Deserializer);
  DaqRegisterSerializerFactory(PAnsiChar(Id), IntFactory);

  Str := '{"__type":"test","list":[]}';
  CreateString(StringObj, PAnsiChar(Str));

  Assert.AreEqual(Deserializer.Deserialize(StringObj, nil, IBaseObject(Int)), OPENDAQ_ERR_OUTOFRANGE);

  DaqUnregisterSerializerFactory(PAnsiChar(Id));
end;

procedure TTest_JsonSerializedList.ReadFloatPositive;
var
  Id: AnsiString;
  Str: AnsiString;
  StringObj: IString;
  Deserializer: IDeserializer;
  Float: IFloat;
  FloatVal: Double;
begin
  Id := 'test';
  CreateJsonDeserializer(Deserializer);
  DaqRegisterSerializerFactory(PAnsiChar(Id), FloatFactory);

  Str := '{"__type":"test","list":[1.5]}';
  CreateString(StringObj, PAnsiChar(Str));

  Deserializer.Deserialize(StringObj, nil, IBaseObject(Float));
  Float.GetValue(FloatVal);

  Assert.AreEqual(FloatVal, Double(1.5));
  DaqUnregisterSerializerFactory(PAnsiChar(Id));
end;

procedure TTest_JsonSerializedList.ReadFloatNegative;
var
  Id: AnsiString;
  Str: AnsiString;
  StringObj: IString;
  Deserializer: IDeserializer;
  Float: IFloat;
  FloatVal: Double;
begin
  Id := 'test';
  CreateJsonDeserializer(Deserializer);
  DaqRegisterSerializerFactory(PAnsiChar(Id), FloatFactory);

  Str := '{"__type":"test","list":[-1.5]}';
  CreateString(StringObj, PAnsiChar(Str));

  Deserializer.Deserialize(StringObj, nil, IBaseObject(Float));
  Float.GetValue(FloatVal);

  Assert.AreEqual(FloatVal, Double(-1.5));
  DaqUnregisterSerializerFactory(PAnsiChar(Id));
end;

procedure TTest_JsonSerializedList.ReadNonExistentFloat;
var
  Id: AnsiString;
  Str: AnsiString;
  StringObj: IString;
  Deserializer: IDeserializer;
  Float: IFloat;
begin
  Id := 'test';
  CreateJsonDeserializer(Deserializer);
  DaqRegisterSerializerFactory(PAnsiChar(Id), FloatFactory);

  Str := '{"__type":"test","list":[]}';
  CreateString(StringObj, PAnsiChar(Str));

  Assert.AreEqual(Deserializer.Deserialize(StringObj, nil, IBaseObject(Float)), OPENDAQ_ERR_OUTOFRANGE);

  DaqUnregisterSerializerFactory(PAnsiChar(Id));
end;

procedure TTest_JsonSerializedList.ReadFloatInvalidType;
var
  Id: AnsiString;
  Str: AnsiString;
  StringObj: IString;
  Deserializer: IDeserializer;
  Float: IFloat;
begin
  Id := 'test';
  CreateJsonDeserializer(Deserializer);
  DaqRegisterSerializerFactory(PAnsiChar(Id), FloatFactory);

  Str := '{"__type":"test","list":[1]}';
  CreateString(StringObj, PAnsiChar(Str));

  Assert.AreEqual(Deserializer.Deserialize(StringObj, nil, IBaseObject(Float)), OPENDAQ_ERR_INVALIDTYPE);

  DaqUnregisterSerializerFactory(PAnsiChar(Id));
end;

procedure TTest_JsonSerializedList.ReadString;
var
  Id: AnsiString;
  Str: AnsiString;
  StringObj: IString;
  Deserializer: IDeserializer;
  StrObj: IString;
  StrVal: PAnsiChar;
begin
  Id := 'test';
  CreateJsonDeserializer(Deserializer);
  DaqRegisterSerializerFactory(PAnsiChar(Id), StringFactory);

  Str := '{"__type":"test","list":["Test"]}';
  CreateString(StringObj, PAnsiChar(Str));

  Deserializer.Deserialize(StringObj, nil, IBaseObject(StrObj));
  StrObj.GetCharPtr(@StrVal);

  Assert.AreEqual(string(StrVal), 'Test');
  DaqUnregisterSerializerFactory(PAnsiChar(Id));
end;

procedure TTest_JsonSerializedList.ReadStringInvalidType;
var
  Id: AnsiString;
  Str: AnsiString;
  StringObj: IString;
  Deserializer: IDeserializer;
  StrObj: IString;
begin
  Id := 'test';
  CreateJsonDeserializer(Deserializer);
  DaqRegisterSerializerFactory(PAnsiChar(Id), StringFactory);

  Str := '{"__type":"test","list":[0]}';
  CreateString(StringObj, PAnsiChar(Str));

  Assert.AreEqual(Deserializer.Deserialize(StringObj, nil, IBaseObject(StrObj)), OPENDAQ_ERR_INVALIDTYPE);

  DaqUnregisterSerializerFactory(PAnsiChar(Id));
end;

procedure TTest_JsonSerializedList.ReadStringOutOfRange;
var
  Id: AnsiString;
  Str: AnsiString;
  StringObj: IString;
  Deserializer: IDeserializer;
  StrObj: IString;
begin
  Id := 'test';
  CreateJsonDeserializer(Deserializer);
  DaqRegisterSerializerFactory(PAnsiChar(Id), StringFactory);

  Str := '{"__type":"test","list":[]}';
  CreateString(StringObj, PAnsiChar(Str));

  Assert.AreEqual(Deserializer.Deserialize(StringObj, nil, IBaseObject(StrObj)), OPENDAQ_ERR_OUTOFRANGE);

  DaqUnregisterSerializerFactory(PAnsiChar(Id));
end;

procedure TTest_JsonSerializedList.ReadObjectOutOfRange;
var
  Id: AnsiString;
  Str: AnsiString;
  StringObj: IString;
  Deserializer: IDeserializer;
  Obj: IBaseObject;
begin
  Id := 'test';
  CreateJsonDeserializer(Deserializer);
  DaqRegisterSerializerFactory(PAnsiChar(Id), ObjectFactory);

  Str := '{"__type":"'+ Id +'","list":[]}';
  CreateString(StringObj, PAnsiChar(Str));

  Assert.AreEqual(Deserializer.Deserialize(StringObj, nil, Obj), OPENDAQ_ERR_OUTOFRANGE);

  DaqUnregisterSerializerFactory(PAnsiChar(Id));
end;

procedure TTest_JsonSerializedList.ReadObject;
var
  Id: AnsiString;
  Str: AnsiString;
  StringObj: IString;
  Deserializer: IDeserializer;
  Obj: IBaseObject;
begin
  Id := 'null';
  CreateJsonDeserializer(Deserializer);
  DaqRegisterSerializerFactory(PAnsiChar(Id), EmptyFactory);

  Str := '{"__type":"'+ Id +'","list":[{"__type":"null"}]}';
  CreateString(StringObj, PAnsiChar(Str));

  Deserializer.Deserialize(StringObj, nil, Obj);
  Assert.IsFalse(Assigned(Obj));

  DaqUnregisterSerializerFactory(PAnsiChar(Id));
end;

initialization
  TDSUnitTestEngine.RegisterUnitTest(TTest_JsonSerializedList);

end.
