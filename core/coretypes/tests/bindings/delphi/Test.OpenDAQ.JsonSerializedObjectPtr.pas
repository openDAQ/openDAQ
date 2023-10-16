unit Test.OpenDAQ.JsonSerializedObjectPtr;

interface

uses
  DunitX.TestFramework, DS.UT.DSUnitTestUnit, OpenDAQ.CoreTypes;

type
  [TestFixture]
  TTest_JsonSerializedObjectPtr = class(TDSUnitTest)
  private
    FFactoryId: string;
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
  DS.UT.DSUnitTestEngineUnit,
  OpenDAQ.CoreTypes.Errors,
  OpenDAQ.Exceptions,
  OpenDAQ.Serializer,
  OpenDAQ.Deserializer;

function IntFactory(Serialized: ISerializedObject; Context: IBaseObject; out Obj: IBaseObject): ErrCode;
var
  Value: RtInt;
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
  Value: RtFloat;
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

procedure TTest_JsonSerializedObjectPtr.Setup();
begin
  FFactoryId := 'test';
end;

procedure TTest_JsonSerializedObjectPtr.TearDown();
begin
  DaqUnregisterSerializerFactory(FFactoryId);
end;

procedure TTest_JsonSerializedObjectPtr.ReadIntPositive();
var
  Deserializer: IDeserializerPtr<IInteger>;
  IntVal: RtInt;
begin
  Deserializer := TDeserializerPtr<IInteger>.Create();
  DaqRegisterSerializerFactory(FFactoryId, IntFactory);

  IntVal := Deserializer.Deserialize('{"__type":"' + FFactoryId + '","int":1}');
  Assert.AreEqual<RtInt>(IntVal, 1);
end;

procedure TTest_JsonSerializedObjectPtr.ReadIntNegative();
var
  Deserializer: IDeserializerPtr<IInteger>;
  IntVal: RtInt;
begin
  Deserializer := TDeserializerPtr<IInteger>.Create();
  DaqRegisterSerializerFactory(FFactoryId, IntFactory);

  IntVal := Deserializer.Deserialize('{"__type":"' + FFactoryId + '","int":-1}');
  Assert.AreEqual<RtInt>(IntVal, -1);
end;

procedure TTest_JsonSerializedObjectPtr.ReadIntInvalidType();
var
  Deserializer: IDeserializerPtr<IInteger>;
  IntVal: RtInt;
begin
  Deserializer := TDeserializerPtr<IInteger>.Create();
  DaqRegisterSerializerFactory(FFactoryId, IntFactory);

  Assert.WillRaise(procedure()
    begin
      IntVal := Deserializer.Deserialize('{"__type":"' + FFactoryId + '","int":1.0}');
    end,
    ERTInvalidTypeException
  );
end;

procedure TTest_JsonSerializedObjectPtr.ReadNonExistentInt();
var
  Deserializer: IDeserializerPtr<IInteger>;
  IntVal: RtInt;
begin
  Deserializer := TDeserializerPtr<IInteger>.Create();
  DaqRegisterSerializerFactory(FFactoryId, IntFactory);

  Assert.WillRaise(procedure()
    begin
      IntVal := Deserializer.Deserialize('{"__type":"' + FFactoryId + '","integer":1}')
    end,
    ERTNotFoundException
  );
end;

procedure TTest_JsonSerializedObjectPtr.ReadFloatPositive();
var
  Deserializer: IDeserializerPtr<IFloat>;
  FloatVal: RtFloat;
begin
  Deserializer := TDeserializerPtr<IFloat>.Create();
  DaqRegisterSerializerFactory(FFactoryId, FloatFactory);

  FloatVal := Deserializer.Deserialize('{"__type":"' + FFactoryId + '","float":1.5}');
  Assert.AreEqual<RtFloat>(FloatVal, 1.5);
end;

procedure TTest_JsonSerializedObjectPtr.ReadFloatNegative();
var
  Deserializer: IDeserializerPtr<IFloat>;
  FloatVal: RtFloat;
begin
  Deserializer := TDeserializerPtr<IFloat>.Create();
  DaqRegisterSerializerFactory(FFactoryId, FloatFactory);

  FloatVal := Deserializer.Deserialize('{"__type":"' + FFactoryId + '","float":-1.5}');

  Assert.AreEqual<RtFloat>(FloatVal, -1.5);
end;

procedure TTest_JsonSerializedObjectPtr.ReadNonExistentFloat();
var
  Deserializer: IDeserializerPtr<IFloat>;
  FloatVal: RtFloat;
begin
  Deserializer := TDeserializerPtr<IFloat>.Create();
  DaqRegisterSerializerFactory(FFactoryId, FloatFactory);

  Assert.WillRaise(procedure()
    begin
      FloatVal := Deserializer.Deserialize('{"__type":"' + FFactoryId + '","floating":1.0}');
    end,
    ERTNotFoundException
  );
end;

procedure TTest_JsonSerializedObjectPtr.ReadFloatInvalidType();
var
  Deserializer: IDeserializerPtr<IFloat>;
  FloatVal: RtFloat;
begin
  Deserializer := TDeserializerPtr<IFloat>.Create();
  DaqRegisterSerializerFactory(FFactoryId, FloatFactory);

  Assert.WillRaise(procedure()
    begin
      FloatVal := Deserializer.Deserialize('{"__type":"' + FFactoryId + '","float":1}');
    end,
    ERTInvalidTypeException
  );
end;

procedure TTest_JsonSerializedObjectPtr.ReadBoolTrue();
var
  Deserializer: IDeserializerPtr<IBoolean>;
  BoolVal: Boolean;
begin
  Deserializer := TDeserializerPtr<IBoolean>.Create();
  DaqRegisterSerializerFactory(FFactoryId, BoolFactory);

  BoolVal := Deserializer.Deserialize('{"__type":"' + FFactoryId + '","bool":true}');
  Assert.AreEqual(BoolVal, True);
end;

procedure TTest_JsonSerializedObjectPtr.ReadBoolFalse();
var
  Deserializer: IDeserializerPtr<IBoolean>;
  BoolVal: Boolean;
begin
  Deserializer := TDeserializerPtr<IBoolean>.Create();
  DaqRegisterSerializerFactory(FFactoryId, BoolFactory);

  BoolVal := Deserializer.Deserialize('{"__type":"' + FFactoryId + '","bool":false}');
  Assert.AreEqual(BoolVal, False);
end;

procedure TTest_JsonSerializedObjectPtr.ReadBoolInvalidType();
var
  Deserializer: IDeserializerPtr<IBoolean>;
  BoolVal: Boolean;
begin
  Deserializer := TDeserializerPtr<IBoolean>.Create();
  DaqRegisterSerializerFactory(FFactoryId, BoolFactory);

  Assert.WillRaise(procedure()
    begin
        BoolVal := Deserializer.Deserialize('{"__type":"'+ FFactoryId + '","bool":1}');
    end,
    ERTInvalidTypeException
  );
end;

procedure TTest_JsonSerializedObjectPtr.ReadNonExistentBool();
var
  Deserializer: IDeserializerPtr<IBoolean>;
  BoolVal: Boolean;
begin
  Deserializer := TDeserializerPtr<IBoolean>.Create();
  DaqRegisterSerializerFactory(FFactoryId, BoolFactory);

  Assert.WillRaise(procedure()
    begin
      BoolVal := Deserializer.Deserialize('{"__type":"' + FFactoryId + '","boolean":true}');
    end,
    ERTNotFoundException
  );
end;

procedure TTest_JsonSerializedObjectPtr.ReadString();
var
  Deserializer: IDeserializerPtr<IString>;
  StrVal: string;
begin
  Deserializer := TDeserializerPtr<IString>.Create();
  DaqRegisterSerializerFactory(FFactoryId, StringFactory);

  StrVal := Deserializer.Deserialize('{"__type":"' + FFactoryId + '","string":"Test"}');
  Assert.AreEqual(StrVal, 'Test');
end;

procedure TTest_JsonSerializedObjectPtr.ReadStringInvalidType();
var
  Deserializer: IDeserializerPtr<IString>;
  StrVal: string;
begin
  Deserializer := TDeserializerPtr<IString>.Create();
  DaqRegisterSerializerFactory(FFactoryId, StringFactory);

  Assert.WillRaise(procedure()
    begin
      StrVal := Deserializer.Deserialize('{"__type":"' + FFactoryId + '","string":0}');
    end,
    ERTInvalidTypeException
  );
end;

procedure TTest_JsonSerializedObjectPtr.ReadNonExistentString();
var
  Deserializer: IDeserializerPtr<IString>;
  StrVal: string;
begin
  Deserializer := TDeserializerPtr<IString>.Create();
  DaqRegisterSerializerFactory(FFactoryId, StringFactory);

  Assert.WillRaise(procedure()
    begin
      StrVal := Deserializer.Deserialize('{"__type":"' + FFactoryId + '","str":"Test"}');
    end,
    ERTNotFoundException
  );
end;

procedure TTest_JsonSerializedObjectPtr.TestHasKeyTrue();
var
  Deserializer: IDeserializerPtr<IBoolean>;
  BoolVal: Boolean;
begin
  Deserializer := TDeserializerPtr<IBoolean>.Create();
  DaqRegisterSerializerFactory(FFactoryId, HasKeyFactory);

  BoolVal := Deserializer.Deserialize('{"__type":"' + FFactoryId + '","str":"Test"}');
  Assert.AreEqual(BoolVal, True);
end;

procedure TTest_JsonSerializedObjectPtr.TestHasKeyFalse();
var
  Deserializer: IDeserializerPtr<IBoolean>;
  BoolVal: Boolean;
begin
  Deserializer := TDeserializerPtr<IBoolean>.Create();
  DaqRegisterSerializerFactory(FFactoryId, HasKeyFactory);

  BoolVal := Deserializer.Deserialize('{"__type":"' + FFactoryId + '","string":"Test"}');

  Assert.AreEqual(BoolVal, False);
end;

procedure TTest_JsonSerializedObjectPtr.ReadEmptyObjectKeys();
var
  Deserializer: IDeserializerPtr<IListObject>;
  Keys: IListPtr<IBaseObject>;
begin
  Deserializer := TDeserializerPtr<IListObject>.Create();
  DaqRegisterSerializerFactory(FFactoryId, KeyFactory);

  Keys := Deserializer.Deserialize('{"__type":"' + FFactoryId + '","object":{}}').AsPtr<IListPtr<IBaseObject>>();

  Assert.AreEqual<SizeT>(Keys.GetCount(), 0);
end;

procedure TTest_JsonSerializedObjectPtr.ReadObjectKeys();
var
  Deserializer: IDeserializerPtr<IListObject>;
  Keys: IListPtr<IBaseObject>;
begin
  Deserializer := TDeserializerPtr<IListObject>.Create();
  DaqRegisterSerializerFactory(FFactoryId, KeyFactory);

  Keys := Deserializer.Deserialize('{"__type":"' + FFactoryId + '","object":{"key1":1,"key2":0.0,"key3":false,"key4":"string","key5":[],"key6":{}}}')
                      .AsPtr<IListPtr<IBaseObject>>;

  Assert.AreEqual<SizeT>(Keys.GetCount(), 6);
end;

procedure TTest_JsonSerializedObjectPtr.ReadNonExistentObject();
var
  Deserializer: IDeserializerPtr;
  Obj: IBaseObject;
begin
  Deserializer := TDeserializerPtr.Create();
  DaqRegisterSerializerFactory(FFactoryId, ObjectFactory);

  Assert.WillRaise(procedure()
    begin
      Obj := Deserializer.Deserialize('{"__type":"' + FFactoryId + '","object":{}}');
    end,
    ERTNotFoundException
  );
end;

procedure TTest_JsonSerializedObjectPtr.ReadSerializedObjectInvalidType();
var
  Deserializer: IDeserializerPtr;
  Obj: IBaseObject;
begin
  Deserializer := TDeserializerPtr.Create();
  DaqRegisterSerializerFactory(FFactoryId, SerializedObjectErrorFactory);

  Assert.WillRaise(procedure()
    begin
      Obj := Deserializer.Deserialize('{"__type":"' + FFactoryId + '","object":[]}');
    end,
    ERTInvalidTypeException
  );
end;

procedure TTest_JsonSerializedObjectPtr.ReadNonExistentSerializedObject();
var
  Deserializer: IDeserializerPtr;
  Obj: IBaseObject;
begin
  Deserializer := TDeserializerPtr.Create();
  DaqRegisterSerializerFactory(FFactoryId, SerializedObjectErrorFactory);

  Assert.WillRaise(procedure()
    begin
      Obj := Deserializer.Deserialize('{"__type":"' + FFactoryId + '","obj":{}}');
    end,
    ERTNotFoundException
  );
end;

procedure TTest_JsonSerializedObjectPtr.ReadEmptySerializedList();
var
  Deserializer: IDeserializerPtr<ISerializedList>;
  SerializedList: ISerializedListPtr;
begin
  Deserializer := TDeserializerPtr<ISerializedList>.Create();
  DaqRegisterSerializerFactory(FFactoryId, SerializedObjectFactory);

  SerializedList := Deserializer.Deserialize('{"__type":"' + FFactoryId + '","list":[]}').AsPtr<ISerializedListPtr>();
  Assert.AreEqual<SizeT>(SerializedList.GetCount(), 0);
end;

procedure TTest_JsonSerializedObjectPtr.ReadNonExistingSerializedList();
var
  Deserializer: IDeserializerPtr<ISerializedList>;
  SerializedList: ISerializedListPtr;
begin
  Deserializer := TDeserializerPtr<ISerializedList>.Create();
  DaqRegisterSerializerFactory(FFactoryId, SerializedObjectFactory);

  Assert.WillRaise(procedure()
    begin
      SerializedList := Deserializer.Deserialize('{"__type":"' + FFactoryId + '","array":[]}').AsPtr<ISerializedListPtr>();
    end,
    ERTNotFoundException
  );
end;

procedure TTest_JsonSerializedObjectPtr.ReadSerializedListInvalidType();
var
  Deserializer: IDeserializerPtr<ISerializedList>;
  SerializedList: ISerializedListPtr;
begin
  Deserializer := TDeserializerPtr<ISerializedList>.Create();
  DaqRegisterSerializerFactory(FFactoryId, SerializedObjectFactory);

  Assert.WillRaise(procedure()
    begin
      SerializedList := Deserializer.Deserialize('{"__type":"' + FFactoryId + '","list":false}').AsPtr<ISerializedListPtr>();;
    end,
    ERTInvalidTypeException
  );
end;

procedure TTest_JsonSerializedObjectPtr.ReadEmptyList();
var
  Deserializer: IDeserializerPtr<IListObject>;
  List: IListPtr<IBaseObject>;
begin
  Deserializer := TDeserializerPtr<IListObject>.Create();
  DaqRegisterSerializerFactory(FFactoryId, ListFactory);

  List := Deserializer.Deserialize('{"__type":"' + FFactoryId + '","list":[]}').AsPtr<IListPtr<IBaseObject>>;
  
  Assert.AreEqual<SizeT>(List.GetCount(), 0);
end;

procedure TTest_JsonSerializedObjectPtr.ReadNonExistingList();
var
  Deserializer: IDeserializerPtr<IListObject>;
  List: IListObject;
begin
  Deserializer := TDeserializerPtr<IListObject>.Create();
  DaqRegisterSerializerFactory(FFactoryId, ListFactory);

  Assert.WillRaise(procedure()
    begin
      List := Deserializer.Deserialize('{"__type":"' + FFactoryId + '","array":[]}');
    end,
    ERTNotFoundException
  );
end;

procedure TTest_JsonSerializedObjectPtr.ReadListInvalidType();
var
  Deserializer: IDeserializerPtr<IListObject>;
  List: IListObject;
begin
  Deserializer := TDeserializerPtr<IListObject>.Create();
  DaqRegisterSerializerFactory(FFactoryId, ListFactory);

  Assert.WillRaise(procedure()
    begin
      List := Deserializer.Deserialize('{"__type":"' + FFactoryId + '","list":false}');
    end,
    ERTInvalidTypeException
  );
end;

initialization
  TDSUnitTestEngine.RegisterUnitTest(TTest_JsonSerializedObjectPtr);
end.
