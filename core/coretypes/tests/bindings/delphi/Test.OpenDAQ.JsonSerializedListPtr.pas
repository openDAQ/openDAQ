unit Test.OpenDAQ.JsonSerializedListPtr;

interface

uses
  DunitX.TestFramework, DS.UT.DSUnitTestUnit, OpenDAQ.CoreTypes;

type
  [TestFixture]
  TTest_JsonSerializedListPtr = class(TDSUnitTest)
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

  function ObjectFactory(Serialized: ISerializedObject; Context: IBaseObject; FactoryCallback: IFunction; out Obj: IBaseObject): ErrCode; cdecl;
  function SerializedObjectFactory(Serialized: ISerializedObject; Context: IBaseObject; FactoryCallback: IFunction; out Obj: IBaseObject): ErrCode; cdecl;
  function ListFactory(Serialized: ISerializedObject; Context: IBaseObject; FactoryCallback: IFunction; out Obj: IBaseObject): ErrCode; cdecl;
  function BoolFactory(Serialized: ISerializedObject; Context: IBaseObject; FactoryCallback: IFunction; out Obj: IBaseObject): ErrCode; cdecl;
  function IntFactory(Serialized: ISerializedObject; Context: IBaseObject; FactoryCallback: IFunction; out Obj: IBaseObject): ErrCode; cdecl;
  function FloatFactory(Serialized: ISerializedObject; Context: IBaseObject; FactoryCallback: IFunction; out Obj: IBaseObject): ErrCode; cdecl;
  function StringFactory(Serialized: ISerializedObject; Context: IBaseObject; FactoryCallback: IFunction; out Obj: IBaseObject): ErrCode; cdecl;
  function SerializedListFactory(Serialized: ISerializedObject; Context: IBaseObject; FactoryCallback: IFunction; out Obj: IBaseObject): ErrCode; cdecl;
  function EmptyFactory(Serialized: ISerializedObject; Context: IBaseObject; FactoryCallback: IFunction; out Obj: IBaseObject): ErrCode; cdecl;

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
  OpenDAQ.Serializer,
  OpenDAQ.Deserializer,
  OpenDAQ.SerializedList;

function ObjectFactory(Serialized: ISerializedObject; Context: IBaseObject; FactoryCallback: IFunction; out Obj: IBaseObject): ErrCode;
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

  Res := SerializedList.ReadObject(Context, nil, BaseObj);

  if OPENDAQ_FAILED(Res) then
    Exit(Res);

  Obj := BaseObj;

  Result := OPENDAQ_SUCCESS;
end;

function SerializedObjectFactory(Serialized: ISerializedObject; Context: IBaseObject; FactoryCallback: IFunction; out Obj: IBaseObject): ErrCode;
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

function ListFactory(Serialized: ISerializedObject; Context: IBaseObject; FactoryCallback: IFunction; out Obj: IBaseObject): ErrCode;
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

  Res := SerializedList.ReadList(Context, nil, List);

  if OPENDAQ_FAILED(Res) then
    Exit(Res);

  Obj := List;

  Result := OPENDAQ_SUCCESS;
end;

function BoolFactory(Serialized: ISerializedObject; Context: IBaseObject; FactoryCallback: IFunction; out Obj: IBaseObject): ErrCode;
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

function IntFactory(Serialized: ISerializedObject; Context: IBaseObject; FactoryCallback: IFunction; out Obj: IBaseObject): ErrCode;
var
  SerializedList: ISerializedList;
  Res: ErrCode;
  StringObj: IString;
  Int: DaqInt;
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

function FloatFactory(Serialized: ISerializedObject; Context: IBaseObject; FactoryCallback: IFunction; out Obj: IBaseObject): ErrCode;
var
  SerializedList: ISerializedList;
  Res: ErrCode;
  StringObj: IString;
  Float: DaqFloat;
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

function StringFactory(Serialized: ISerializedObject; Context: IBaseObject; FactoryCallback: IFunction; out Obj: IBaseObject): ErrCode;
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

function SerializedListFactory(Serialized: ISerializedObject; Context: IBaseObject; FactoryCallback: IFunction; out Obj: IBaseObject): ErrCode;
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

function EmptyFactory(Serialized: ISerializedObject; Context: IBaseObject; FactoryCallback: IFunction; out Obj: IBaseObject): ErrCode;
begin
  Obj := nil;
  Result := OPENDAQ_SUCCESS;
end;

{ TTest_JsonSerializedListPtr }

procedure TTest_JsonSerializedListPtr.Setup();
begin
end;

procedure TTest_JsonSerializedListPtr.TearDown();
begin
end;

procedure TTest_JsonSerializedListPtr.ReadSerializedObjectInvalidValue();
var
  Id: string;
  BaseObj: IBaseObject;
  Deserializer: IDeserializerPtr;
begin
  Id := 'test';
  Deserializer := TDeserializerPtr.Create();
  DaqRegisterSerializerFactory(Id, SerializedObjectFactory);

  Assert.WillRaise(procedure()
    begin
      BaseObj := Deserializer.Deserialize('{"__type":"test","list":[false]}', nil, nil);
    end,
    ERTInvalidTypeException
  );

  DaqUnregisterSerializerFactory(Id);
end;

procedure TTest_JsonSerializedListPtr.ReadSerializedObjectOutOfRange();
var
  Id: string;
  BaseObj: IBaseObject;
  Deserializer: IDeserializerPtr;
begin
  Id := 'test';
  DaqRegisterSerializerFactory(Id, serializedObjectFactory);
  Deserializer := TDeserializerPtr.Create();

  Assert.WillRaise(procedure()
    begin
      BaseObj := Deserializer.Deserialize('{"__type":"test","list":[]}', nil, nil);
    end,
    ERTOutOfRangeException
  );

  DaqUnregisterSerializerFactory(Id);
end;

procedure TTest_JsonSerializedListPtr.ReadSerializedObjectNull();
var
  Id: string;
  BaseObj: ISerializedObjectPtr;
  Deserializer: IDeserializerPtr;
begin
  Id := 'test';
  DaqRegisterSerializerFactory(Id, SerializedObjectFactory);
  Deserializer := TDeserializerPtr.Create();

  BaseObj := Deserializer.Deserialize('{"__type":"test","list":[{"__type":1}]}', nil, nil).AsPtr<ISerializedObjectPtr>;
  Assert.IsTrue(Assigned(BaseObj));

  DaqUnregisterSerializerFactory(Id);
end;

procedure TTest_JsonSerializedListPtr.ReadListInvalidType();
var
  Id: string;
  BaseObj: IListPtr<IBaseObject>;
  Deserializer: IDeserializerPtr;
begin
  Id := 'test';
  DaqRegisterSerializerFactory(Id, ListFactory);
  Deserializer := TDeserializerPtr.Create();

  Assert.WillRaise(procedure()
    begin
      BaseObj := Deserializer.Deserialize('{"__type":"test","list":[false]}', nil, nil).AsPtr<IListPtr<IBaseObject>>;
    end,
    ERTInvalidTypeException
  );

  DaqUnregisterSerializerFactory(Id);
end;

procedure TTest_JsonSerializedListPtr.ReadList();
var
  Id: string;
  List: IListPtr<IBaseObject>;
  Deserializer: IDeserializerPtr;
begin
  Id := 'test';
  DaqRegisterSerializerFactory(Id, ListFactory);
  Deserializer := TDeserializerPtr.Create();

  List := Deserializer.Deserialize('{"__type":"test","list":[[]]}', nil, nil).AsPtr<IListPtr<IBaseObject>>;
  Assert.AreEqual<SizeT>(List.GetCount(), 0);

  DaqUnregisterSerializerFactory(Id);
end;

procedure TTest_JsonSerializedListPtr.ReadListOutOfRange();
var
  Id: string;
  List: IListPtr<IBaseObject>;
  Deserializer: IDeserializerPtr;
begin
  Id := 'test';
  DaqRegisterSerializerFactory(Id, ListFactory);
  Deserializer := TDeserializerPtr.Create();

  Assert.WillRaise(procedure()
    begin
      List := Deserializer.Deserialize('{"__type":"test","list":[]}', nil, nil).AsPtr<IListPtr<IBaseObject>>;
    end,
    ERTOutOfRangeException
  );

  DaqUnregisterSerializerFactory(Id);
end;

procedure TTest_JsonSerializedListPtr.ReadSerializedListInvalidType();
var
  Id: string;
  List: ISerializedListPtr;
  Deserializer: IDeserializerPtr;
begin
  Id := 'test';
  DaqRegisterSerializerFactory(Id, SerializedListFactory);
  Deserializer := TDeserializerPtr.Create();

  Assert.WillRaise(procedure()
    begin
      List := Deserializer.Deserialize('{"__type":"test","list":[{}]}', nil, nil).AsPtr<ISerializedListPtr>;
    end,
    ERTInvalidTypeException
  );

  DaqUnregisterSerializerFactory(Id);
end;

procedure TTest_JsonSerializedListPtr.ReadSerializedList();
var
  Id: string;
  List: ISerializedListPtr;
  Deserializer: IDeserializerPtr;
begin
  Id := 'test';
  DaqRegisterSerializerFactory(Id, SerializedListFactory);
  Deserializer := TDeserializerPtr.Create();

  List := Deserializer.Deserialize('{"__type":"test","list":[[]]}', nil, nil).AsPtr<ISerializedListPtr>;
  Assert.AreEqual<SizeT>(List.GetCount(), 0);

  DaqUnregisterSerializerFactory(Id);
end;

procedure TTest_JsonSerializedListPtr.ReadSerializedListOutOfRange();
var
  Id: string;
  List: ISerializedListPtr;
  Deserializer: IDeserializerPtr;
begin
  Id := 'test';
  DaqRegisterSerializerFactory(Id, SerializedListFactory);
  Deserializer := TDeserializerPtr.Create();

  Assert.WillRaise(procedure()
    begin
      List := Deserializer.Deserialize('{"__type":"test","list":[]}', nil, nil).AsPtr<ISerializedListPtr>;
    end,
    ERTOutOfRangeException
  );

  DaqUnregisterSerializerFactory(Id);
end;

procedure TTest_JsonSerializedListPtr.ReadBoolTrue();
var
  Id: string;
  Deserializer: IDeserializerPtr;
  BoolVal: Boolean;
begin
  Id := 'test';
  DaqRegisterSerializerFactory(Id, BoolFactory);
  Deserializer := TDeserializerPtr.Create();

  BoolVal := Deserializer.Deserialize('{"__type":"test","list":[true]}', nil, nil);
  Assert.IsTrue(BoolVal);

  DaqUnregisterSerializerFactory(Id);
end;

procedure TTest_JsonSerializedListPtr.ReadBoolFalse();
var
  Id: string;
  Deserializer: IDeserializerPtr;
  BoolVal: Boolean;
begin
  Id := 'test';
  DaqRegisterSerializerFactory(Id, BoolFactory);
  Deserializer := TDeserializerPtr.Create();

  BoolVal := Deserializer.Deserialize('{"__type":"test","list":[false]}', nil, nil);
  Assert.IsFalse(BoolVal);

  DaqUnregisterSerializerFactory(Id);
end;

procedure TTest_JsonSerializedListPtr.ReadBoolInvalidType();
var
  Id: string;
  BoolVal: Boolean;
  Deserializer: IDeserializerPtr;
begin
  Id := 'test';
  DaqRegisterSerializerFactory(Id, BoolFactory);
  Deserializer := TDeserializerPtr.Create();

  Assert.WillRaise(procedure()
    begin
      BoolVal := Deserializer.Deserialize('{"__type":"test","list":[1]}', nil, nil)
    end,
    ERTInvalidTypeException
  );

  DaqUnregisterSerializerFactory(Id);
end;

procedure TTest_JsonSerializedListPtr.ReadBoolOutOfRange();
var
  Id: string;
  BoolVal: Boolean;
  Deserializer: IDeserializerPtr;
begin
  Id := 'test';
  DaqRegisterSerializerFactory(Id, BoolFactory);
  Deserializer := TDeserializerPtr.Create();

  Assert.WillRaise(procedure()
    begin
      BoolVal := Deserializer.Deserialize('{"__type":"test","list":[]}', nil, nil)
    end,
    ERTOutOfRangeException
  );

  DaqUnregisterSerializerFactory(Id);
end;

procedure TTest_JsonSerializedListPtr.ReadIntPositive();
var
  Id: string;
  IntVal: DaqInt;
  Deserializer: IDeserializerPtr;
begin
  Id := 'test';
  DaqRegisterSerializerFactory(Id, IntFactory);
  Deserializer := TDeserializerPtr.Create();

  IntVal := Deserializer.Deserialize('{"__type":"test","list":[1]}', nil, nil);
  Assert.AreEqual<DaqInt>(IntVal, 1);

  DaqUnregisterSerializerFactory(Id);
end;

procedure TTest_JsonSerializedListPtr.ReadIntNegative();
var
  Id: string;
  IntVal: DaqInt;
  Deserializer: IDeserializerPtr;
begin
  Id := 'test';
  DaqRegisterSerializerFactory(Id, IntFactory);
  Deserializer := TDeserializerPtr.Create();

  IntVal := Deserializer.Deserialize('{"__type":"test","list":[-1]}', nil, nil);
  Assert.AreEqual<DaqInt>(IntVal, -1);

  DaqUnregisterSerializerFactory(Id);
end;

procedure TTest_JsonSerializedListPtr.ReadIntInvalidType();
var
  Id: string;
  IntVal: DaqInt;
  Deserializer: IDeserializerPtr;
begin
  Id := 'test';
  DaqRegisterSerializerFactory(Id, IntFactory);
  Deserializer := TDeserializerPtr.Create();

  Assert.WillRaise(procedure()
    begin
      IntVal := Deserializer.Deserialize('{"__type":"test","list":[1.0]}', nil, nil);
    end,
    ERTInvalidTypeException
  );

  DaqUnregisterSerializerFactory(Id);
end;

procedure TTest_JsonSerializedListPtr.ReadIntOutOfRange();
var
  Id: string;
  IntVal: DaqInt;
  Deserializer: IDeserializerPtr;
begin
  Id := 'test';
  DaqRegisterSerializerFactory(Id, IntFactory);
  Deserializer := TDeserializerPtr.Create();

  Assert.WillRaise(procedure()
    begin
      IntVal := Deserializer.Deserialize('{"__type":"test","list":[]}', nil, nil);
    end,
    ERTOutOfRangeException
  );

  DaqUnregisterSerializerFactory(Id);
end;

procedure TTest_JsonSerializedListPtr.ReadFloatPositive();
var
  Id: string;
  FloatVal: DaqFloat;
  Deserializer: IDeserializerPtr;
begin
  Id := 'test';
  DaqRegisterSerializerFactory(Id, FloatFactory);
  Deserializer := TDeserializerPtr.Create();

  FloatVal := Deserializer.Deserialize('{"__type":"test","list":[1.5]}', nil, nil);
  Assert.AreEqual<DaqFloat>(FloatVal, 1.5);

  DaqUnregisterSerializerFactory(Id);
end;

procedure TTest_JsonSerializedListPtr.ReadFloatNegative();
var
  Id: string;
  FloatVal: DaqFloat;
  Deserializer: IDeserializerPtr;
begin
  Id := 'test';
  DaqRegisterSerializerFactory(Id, FloatFactory);
  Deserializer := TDeserializerPtr.Create();

  FloatVal := Deserializer.Deserialize('{"__type":"test","list":[-1.5]}', nil, nil);
  Assert.AreEqual<DaqFloat>(FloatVal, -1.5);

  DaqUnregisterSerializerFactory(Id);
end;

procedure TTest_JsonSerializedListPtr.ReadNonExistentFloat();
var
  Id: string;
  FloatVal: DaqFloat;
  Deserializer: IDeserializerPtr;
begin
  Id := 'test';
  DaqRegisterSerializerFactory(Id, FloatFactory);
  Deserializer := TDeserializerPtr.Create();

  Assert.WillRaise(procedure()
    begin
      FloatVal := Deserializer.Deserialize('{"__type":"test","list":[]}', nil, nil);
    end,
    ERTOutOfRangeException
  );

  DaqUnregisterSerializerFactory(Id);
end;

procedure TTest_JsonSerializedListPtr.ReadFloatInvalidType();
var
  Id: string;
  FloatVal: DaqFloat;
  Deserializer: IDeserializerPtr;
begin
  Id := 'test';
  DaqRegisterSerializerFactory(Id, FloatFactory);
  Deserializer := TDeserializerPtr.Create();

  Assert.WillRaise(procedure()
    begin
      FloatVal := Deserializer.Deserialize('{"__type":"test","list":[1]}', nil, nil);
    end,
    ERTInvalidTypeException
  );

  DaqUnregisterSerializerFactory(Id);
end;

procedure TTest_JsonSerializedListPtr.ReadString();
var
  Id: string;
  StrVal : string;
  Deserializer: IDeserializerPtr;
begin
  Id := 'test';
  DaqRegisterSerializerFactory(Id, StringFactory);
  Deserializer := TDeserializerPtr.Create();

  StrVal := Deserializer.Deserialize('{"__type":"test","list":["Test"]}', nil, nil);
  Assert.AreEqual(StrVal, 'Test');

  DaqUnregisterSerializerFactory(Id);
end;

procedure TTest_JsonSerializedListPtr.ReadStringInvalidType();
var
  Id: string;
  StrVal: string;
  Deserializer: IDeserializerPtr;
begin
  Id := 'test';
  DaqRegisterSerializerFactory(Id, StringFactory);
  Deserializer := TDeserializerPtr.Create();

  Assert.WillRaise(procedure()
    begin
      StrVal := Deserializer.Deserialize('{"__type":"test","list":[0]}', nil, nil);
    end,
    ERTInvalidTypeException
  );

  DaqUnregisterSerializerFactory(Id);
end;

procedure TTest_JsonSerializedListPtr.ReadStringOutOfRange();
var
  Id: string;
  Obj: IStringPtr;
  Deserializer: IDeserializerPtr;
begin
  Id := 'test';
  DaqRegisterSerializerFactory(Id, StringFactory);
  Deserializer := TDeserializerPtr.Create();

  Assert.WillRaise(procedure()
    begin
      Obj := Deserializer.Deserialize('{"__type":"test","list":[]}', nil, nil).AsPtr<IStringPtr>;
    end,
    ERTOutOfRangeException
  );

  DaqUnregisterSerializerFactory(Id);
end;

procedure TTest_JsonSerializedListPtr.ReadObjectOutOfRange();
var
  Id: string;
  Obj: IBaseObject;
  Deserializer: IDeserializerPtr;
begin
  Id := 'test';
  DaqRegisterSerializerFactory(Id, ObjectFactory);
  Deserializer := TDeserializerPtr.Create();

  Assert.WillRaise(procedure()
    begin
      Obj := Deserializer.Deserialize('{"__type":"' + Id + '","list":[]}', nil, nil);
    end,
    ERTOutOfRangeException
  );

  DaqUnregisterSerializerFactory(Id);
end;

procedure TTest_JsonSerializedListPtr.ReadObject();
var
  Id: string;
  Obj: IBaseObject;
  Deserializer: IDeserializerPtr;
begin
  Id := 'null';
  DaqRegisterSerializerFactory(Id, EmptyFactory);
  Deserializer := TDeserializerPtr.Create();

  Obj := Deserializer.Deserialize('{"__type":"' + Id + '","list":[{"__type":"null"}]}', nil, nil);
  Assert.IsFalse(Assigned(Obj));

  DaqUnregisterSerializerFactory(Id);
end;

initialization
  TDSUnitTestEngine.RegisterUnitTest(TTest_JsonSerializedListPtr);

end.
