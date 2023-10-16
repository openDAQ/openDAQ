unit Test.OpenDAQ.DeserializerPtr;

interface

uses
  DunitX.TestFramework, DS.UT.DSUnitTestUnit, OpenDAQ.CoreTypes;

type
  [TestFixture]
  TTest_DeserializerPtr = class(TDSUnitTest)
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
  WinApi.Windows,
  System.SysUtils,
  OpenDAQ.List,
  DS.UT.DSUnitTestEngineUnit,
  OpenDAQ.CoreTypes.Errors,
  OpenDAQ.ObjectPtr,
  OpenDAQ.TString,
  OpenDAQ.Exceptions,
  OpenDAQ.Deserializer;

function SerializedObjectFactory(SerObj: ISerializedObject; Context: IBaseObject; out Obj: IBaseObject): ErrCode;
begin
  Result := OPENDAQ_SUCCESS;
end;

function ErrorFactory(SerObj: ISerializedObject; Context: IBaseObject; out Obj: IBaseObject): ErrCode;
begin
  Result := OPENDAQ_ERR_GENERALERROR;
end;

{ TTest_BaseObject }

procedure TTest_DeserializerPtr.Setup();
begin
end;

procedure TTest_DeserializerPtr.TearDown();
begin
end;

procedure TTest_DeserializerPtr.DeserializeInvalidJson();
var
  BaseObj: IObjectPtr;
  Deserializer: IDeserializerPtr;
begin
  Deserializer := TDeserializerPtr.Create();

  Assert.WillRaise(procedure()
    begin
      BaseObj := Deserializer.Deserialize('...');
    end,
    ERTDeserializeParseException
  )
end;

procedure TTest_DeserializerPtr.BoolTrue();
var
  Deserializer: IDeserializerPtr<IBoolean>;
begin
  Deserializer := TDeserializerPtr<IBoolean>.Create();
  Assert.IsTrue(Deserializer.Deserialize('true'));
end;

procedure TTest_DeserializerPtr.BoolFalse();
var
  Deserializer: IDeserializerPtr<IBoolean>;
begin
  Deserializer := TDeserializerPtr<IBoolean>.Create();
  Assert.IsFalse(Deserializer.Deserialize('false'));
end;

procedure TTest_DeserializerPtr.FloatZero();
var
  Deserializer: IDeserializerPtr<IFloat>;
  FloatVal: RtFloat;
begin
  Deserializer := TDeserializerPtr<IFloat>.Create();

  FloatVal := Deserializer.Deserialize('0.0');
  Assert.AreEqual<RtFloat>(FloatVal, 0.0);
end;

procedure TTest_DeserializerPtr.FloatMax();
const FloatMax = 1.7976931348623157e308;
var
  Deserializer: IDeserializerPtr<IFloat>;
  FloatVal: RtFloat;
begin
  Deserializer := TDeserializerPtr<IFloat>.Create();

  FloatVal := Deserializer.Deserialize('1.7976931348623157e308');
  Assert.AreEqual<RtFloat>(FloatVal, FloatMax);
end;

procedure TTest_DeserializerPtr.FloatMin();
const FloatMin = 2.2250738585072014e-308;
var
  Deserializer: IDeserializerPtr<IFloat>;
  FloatVal: RtFloat;
begin
  Deserializer := TDeserializerPtr<IFloat>.Create();

  FloatVal := Deserializer.Deserialize('2.2250738585072014e-308');
  Assert.AreEqual<RtFloat>(FloatVal, FloatMin);
end;

procedure TTest_DeserializerPtr.IntZero();
var
  Deserializer: IDeserializerPtr<IInteger>;
  IntVal: RtInt;
begin
  Deserializer := TDeserializerPtr<IInteger>.Create();

  IntVal := Deserializer.Deserialize('0');
  Assert.AreEqual<RtInt>(IntVal, 0);
end;

procedure TTest_DeserializerPtr.IntMax();
const IntMax = 9223372036854775807;
var
  Deserializer: IDeserializerPtr<IInteger>;
  IntVal: RtInt;
begin
  Deserializer := TDeserializerPtr<IInteger>.Create();

  IntVal := Deserializer.Deserialize('9223372036854775807');
  Assert.AreEqual<RtInt>(IntVal, IntMax);
end;

procedure TTest_DeserializerPtr.IntMin();
const IntMin = -9223372036854775808;
var
  Deserializer: IDeserializerPtr<IInteger>;
  IntVal: RtInt;
begin
  Deserializer := TDeserializerPtr<IInteger>.Create();

  IntVal := Deserializer.Deserialize('-9223372036854775808');
  Assert.AreEqual<RtInt>(IntVal, IntMin);
end;

procedure TTest_DeserializerPtr.AsciiStr();
const Expected = ' !"#$%&''()*+''-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijklmnopqrstuvwxyz{|}~';
var
  Deserializer: IDeserializerPtr<IString>;
  Str: string;
begin
  Deserializer := TDeserializerPtr<IString>.Create();

  Str := Deserializer.Deserialize('" !\"#$%&''()*+''-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~"');
  Assert.AreEqual(Str, Expected);
end;


procedure TTest_DeserializerPtr.EmptyList();
var
  Deserializer: IDeserializerPtr<IListObject>;
  ListObj: IListPtr<IBaseObject>;
begin
  Deserializer := TDeserializerPtr<IListObject>.Create();

  ListObj := Deserializer.Deserialize('[]').AsPtr<IListPtr<IBaseObject>>();
  Assert.AreEqual<SizeT>(ListObj.GetCount(), 0);
end;

procedure TTest_DeserializerPtr.StringListOne();
var
  Deserializer: IDeserializerPtr<IListObject>;
  ListObj: IListPtr<IString>;
begin
  Deserializer := TDeserializerPtr<IListObject>.Create();
  ListObj := Deserializer.Deserialize('["Item1"]').AsPtr<IListPtr<IString>>();

  Assert.AreEqual<SizeT>(ListObj.GetCount(), 1);
  Assert.AreEqual<string>(ListObj[0], 'Item1');
end;

procedure TTest_DeserializerPtr.StringListMultiple();
var
  Deserializer: IDeserializerPtr<IListObject>;
  ListObj: IListPtr<IString>;
begin
  Deserializer := TDeserializerPtr<IListObject>.Create();
  ListObj := Deserializer.Deserialize('["Item1", "Item2"]').AsPtr<IListPtr<IString>>();

  Assert.AreEqual<SizeT>(ListObj.GetCount(), 2);

  Assert.AreEqual<string>(ListObj[0], 'Item1');
  Assert.AreEqual<string>(ListObj[1], 'Item2');
end;

procedure TTest_DeserializerPtr.BoolListTrue();
var
  Deserializer: IDeserializerPtr<IListObject>;
  ListObj: IListPtr<IBoolean>;
begin
  Deserializer := TDeserializerPtr<IListObject>.Create();
  ListObj := Deserializer.Deserialize('[true]').AsPtr<IListPtr<IBoolean>>();

  Assert.AreEqual<SizeT>(ListObj.GetCount(), 1);
  Assert.IsTrue(ListObj[0]);
end;

procedure TTest_DeserializerPtr.BoolListFalse();
var
  Deserializer: IDeserializerPtr<IListObject>;
  ListObj: IListPtr<IBoolean>;
begin
  Deserializer := TDeserializerPtr<IListObject>.Create();
  ListObj := Deserializer.Deserialize('[false]').AsPtr<IListPtr<IBoolean>>();

  Assert.AreEqual<SizeT>(ListObj.GetCount(), 1);
  Assert.IsFalse(ListObj[0]);
end;

procedure TTest_DeserializerPtr.BoolList();
var
  Deserializer: IDeserializerPtr<IListObject>;
  ListObj: IListPtr<IBoolean>;
begin
  Deserializer := TDeserializerPtr<IListObject>.Create();
  ListObj := Deserializer.Deserialize('[false,true]').AsPtr<IListPtr<IBoolean>>();

  Assert.AreEqual<SizeT>(ListObj.GetCount(), 2);
  Assert.IsFalse(ListObj[0]);
  Assert.IsTrue(ListObj[1]);
end;

procedure TTest_DeserializerPtr.FloatListOne();
var
  Deserializer: IDeserializerPtr<IListObject>;
  ListObj: IListPtr<IFloat>;
begin
  Deserializer := TDeserializerPtr<IListObject>.Create();
  ListObj := Deserializer.Deserialize('[0.0]').AsPtr<IListPtr<IFloat>>();

  Assert.AreEqual<SizeT>(ListObj.GetCount(), 1);
  Assert.AreEqual<RtFloat>(ListObj[0], 0.0);
end;

procedure TTest_DeserializerPtr.FloatListMultiple();
const
  FloatMax = 1.7976931348623157e308;
  FloatMin = 2.2250738585072014e-308;
var
  Deserializer: IDeserializerPtr<IListObject>;
  ListObj: IListPtr<IFloat>;
begin
  Deserializer := TDeserializerPtr<IListObject>.Create();
  ListObj := Deserializer.Deserialize('[0.0,2.2250738585072014e-308,1.7976931348623157e308]').AsPtr<IListPtr<IFloat>>();

  Assert.AreEqual<SizeT>(ListObj.GetCount(), 3);
  Assert.AreEqual<RtFloat>(ListObj[0], 0);
  Assert.AreEqual<RtFloat>(ListObj[1], FloatMin);
  Assert.AreEqual<RtFloat>(ListObj[2], FloatMax);
end;

procedure TTest_DeserializerPtr.IntListOne();
var
  Deserializer: IDeserializerPtr<IListObject>;
  ListObj: IListPtr<IInteger>;
begin
  Deserializer := TDeserializerPtr<IListObject>.Create();
  ListObj := Deserializer.Deserialize('[0]').AsPtr<IListPtr<IInteger>>();

  Assert.AreEqual<SizeT>(ListObj.GetCount(), 1);
  Assert.AreEqual<RtInt>(ListObj.GetItemAt(0), 0);
end;

procedure TTest_DeserializerPtr.IntListMultiple();
const
  IntMax = 9223372036854775807;
  IntMin = -9223372036854775808;
var
  Deserializer: IDeserializerPtr<IListObject>;
  ListObj: IListPtr<IInteger>;
begin
  Deserializer := TDeserializerPtr<IListObject>.Create();

  ListObj := Deserializer.Deserialize('[0,-9223372036854775808,9223372036854775807]').AsPtr<IListPtr<IInteger>>();
  Assert.AreEqual<SizeT>(ListObj.GetCount(), 3);

  Assert.AreEqual<RtInt>(ListObj[0], 0);
  Assert.AreEqual<RtInt>(ListObj[1], IntMin);
  Assert.AreEqual<RtInt>(ListObj[2], IntMax);
end;

procedure TTest_DeserializerPtr.MixedList();
const
  IntMax = 9223372036854775807;
  IntMin = -9223372036854775808;
  FloatMax = 1.7976931348623157e308;
  FloatMin = 2.2250738585072014e-308;
var
  Deserializer: IDeserializerPtr<IListObject>;
  ListObj: IListPtr<IBaseObject>;
begin
  Deserializer := TDeserializerPtr<IListObject>.Create();
  ListObj := Deserializer.Deserialize('[0.0,0,-2.5,1.5,1,-2,2.2250738585072014e-308,1.7976931348623157e308,-9223372036854775808,9223372036854775807,"Test1"]')
                         .AsPtr<IListPtr<IBaseObject>>();

  Assert.AreEqual<SizeT>(ListObj.GetCount(), 11);

  Assert.AreEqual<RtFloat>(ListObj[0], 0.0);
  Assert.AreEqual<RtInt>(ListObj[1], 0);
  Assert.AreEqual<RtFloat>(ListObj[2], -2.5);
  Assert.AreEqual<RtFloat>(ListObj[3], 1.5);
  Assert.AreEqual<RtInt>(ListObj[4], 1);
  Assert.AreEqual<RtInt>(ListObj[5], -2);
  Assert.AreEqual<RtFloat>(ListObj[6], FloatMin);
  Assert.AreEqual<RtFloat>(ListObj[7], FloatMax);
  Assert.AreEqual<RtInt>(ListObj[8], IntMin);
  Assert.AreEqual<RtInt>(ListObj[9], IntMax);
  Assert.AreEqual<string>(ListObj[10], 'Test1');
end;

procedure TTest_DeserializerPtr.UnknownObjectType();
var
  Deserializer: IDeserializerPtr<IBaseObject>;
  BaseObj: IObjectPtr;
begin
  Deserializer := TDeserializerPtr<IBaseObject>.Create();

  Assert.WillRaise(procedure()
    begin
      BaseObj := Deserializer.Deserialize('{"__type":"unknown"}');
    end,
    ERTDeserializeFactoryNotRegisteredException
  );
end;

procedure TTest_DeserializerPtr.ObjectTypeTagNotInt();
var
  Deserializer: IDeserializerPtr<IBaseObject>;
  BaseObj: IObjectPtr;
begin
  Deserializer := TDeserializerPtr<IBaseObject>.Create();

  Assert.WillRaise(procedure()
    begin
      BaseObj := Deserializer.Deserialize('{"__type":0.0}');
    end,
    ERTDeserializeInvalidTypeTagException
  );
end;

procedure TTest_DeserializerPtr.NoObjectType();
var
  Deserializer: IDeserializerPtr<IBaseObject>;
  BaseObj: IBaseObject;
begin
  Deserializer := TDeserializerPtr<IBaseObject>.Create();

  Assert.WillRaise(procedure()
    begin
      BaseObj := Deserializer.Deserialize('{"test":0}');
    end,
    ERTDeserializeNoTypeTagException
  );
end;

procedure TTest_DeserializerPtr.DeserializeNullString();
var
  Deserializer: IDeserializerPtr<IBaseObject>;
  BaseObj: IBaseObject;
begin
  Deserializer := TDeserializerPtr<IBaseObject>.Create();

  Assert.WillRaise(procedure()
    begin
       BaseObj := Deserializer.Deserialize(nil);
    end,
    ERTArgumentNullException
  );
end;

procedure TTest_DeserializerPtr.RegisterFactory();
var
  Id: string;
begin
  Id := 'test';
  Assert.WillNotRaiseAny(procedure()
    begin
      DaqRegisterSerializerFactory(Id, SerializedObjectFactory);
    end
  );
  DaqUnregisterSerializerFactory(Id);
end;

procedure TTest_DeserializerPtr.RegisterExistingFactory();
var
  Id: string;
begin
  Id := 'test';
  DaqRegisterSerializerFactory(Id, SerializedObjectFactory);
  Assert.IsTrue(DaqRegisterSerializerFactory(Id, SerializedObjectFactory));

  DaqUnregisterSerializerFactory(Id);
end;

procedure TTest_DeserializerPtr.GetFactory();
var
  Id: string;
  Factory: TDSRTDeserializerFactory;
begin
  Id := 'test';
  DaqRegisterSerializerFactory(Id, SerializedObjectFactory);

  Assert.WillNotRaiseAny(procedure()
    begin
      Factory := DaqGetSerializerFactory(Id);
    end
  );

  DaqUnregisterSerializerFactory(Id);
end;

procedure TTest_DeserializerPtr.UnregisterFactory();
var
  Id: string;
begin
  Id := 'test';
  DaqRegisterSerializerFactory(Id, SerializedObjectFactory);

  Assert.WillNotRaiseAny(procedure()
    begin
      DaqUnregisterSerializerFactory(Id);
    end
  );
end;

procedure TTest_DeserializerPtr.UnregisterNonExistingFactory();
begin
  Assert.IsFalse(DaqUnregisterSerializerFactory('test'));
end;

procedure TTest_DeserializerPtr.GetNonExistingFactory();
var
  Id: string;
  Factory: TDSRTDeserializerFactory;
begin
  Id := 'test';

  Assert.WillRaise(procedure()
    begin
      Factory := DaqGetSerializerFactory(Id);
    end,
    ERTDeserializeFactoryNotRegisteredException
  );
end;

procedure TTest_DeserializerPtr.FactoryReturnsError();
var
  Id: string;
  Deserializer: IDeserializerPtr;
  StringObj: IStringPtr;
  BaseObj: IBaseObject;
begin
  Id := 'test';
  DaqRegisterSerializerFactory(Id, ErrorFactory);
  Deserializer := TDeserializerPtr.Create();

  StringObj := TStringPtr.Create();

  Assert.WillRaise(procedure()
    begin
      BaseObj := Deserializer.Deserialize('{"__type":"' + Id + '"}');
    end,
    ERTException
  );

  DaqUnregisterSerializerFactory(Id);
end;

initialization
  TDSUnitTestEngine.RegisterUnitTest(TTest_DeserializerPtr);

end.
