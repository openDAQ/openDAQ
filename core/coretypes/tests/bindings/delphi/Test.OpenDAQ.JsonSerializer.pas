unit Test.OpenDAQ.JsonSerializer;

interface

uses
  DunitX.TestFramework, DS.UT.DSUnitTestUnit, OpenDAQ.CoreTypes;

type
  [TestFixture]
  TTest_JsonSerializer = class(TDSUnitTest)
  private
  public
    [Setup]
    procedure Setup; override;
    [TearDown]
    procedure TearDown; override;
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
    procedure FloatListOne;
    [Test]
    procedure FloatListMultiple;
    [Test]
    procedure IntListOne;
    [Test]
    procedure IntListMultiple;
    [Test]
    procedure IntFloatList;
    [Test]
    procedure MixedList;
    [Test]
    procedure StartTaggedNull;
    [Test]
    procedure IsCompleteFalse;
    [Test]
    procedure IsCompleteEmpty;
    [Test]
    procedure IsCompleteEmptyObject;
    [Test]
    procedure Reset;
    [Test]
    procedure SimpleObject;
    [Test]
    procedure SimpleObjectKeyLength;
    [Test]
    procedure ObjectZeroLengthKey;
    [Test]
    procedure ObjectNullKeyPtrInterface;
    [Test]
    procedure ObjectKeyNullSizeInterface;
  end;

implementation
uses
  DS.UT.DSUnitTestEngineUnit, WinApi.Windows, OpenDAQ.CoreTypes.Errors, SysUtils;

{ TTest_JsonSerializer }

procedure TTest_JsonSerializer.Setup;
begin
end;

procedure TTest_JsonSerializer.TearDown;
begin
end;

procedure TTest_JsonSerializer.BoolTrue;
var
  BoolObj: IBoolean;
  Serializable: ISerializable;
  Serializer: ISerializer;
  StrObj: IString;
  Str: PAnsiChar;
begin
  CreateBoolean(BoolObj, True);
  CreateJsonSerializer(Serializer);

  Serializable := GetSerializableInterface(BoolObj);
  Serializable.Serialize(Serializer);

  Serializer.GetOutput(StrObj);
  StrObj.GetCharPtr(@Str);
  Assert.AreEqual(string(Str), 'true');
end;

procedure TTest_JsonSerializer.BoolFalse;
var
  BoolObj: IBoolean;
  Serializable: ISerializable;
  Serializer: ISerializer;
  StrObj: IString;
  Str: PAnsiChar;
begin
  CreateBoolean(BoolObj, False);
  CreateJsonSerializer(Serializer);

  Serializable := GetSerializableInterface(BoolObj);
  Serializable.Serialize(Serializer);

  Serializer.GetOutput(StrObj);
  StrObj.GetCharPtr(@Str);
  Assert.AreEqual(string(Str), 'false');
end;

procedure TTest_JsonSerializer.FloatZero;
var
  FloatObj: IFloat;
  Serializable: ISerializable;
  Serializer: ISerializer;
  StrObj: IString;
  Str: PAnsiChar;
begin
  CreateFloat(FloatObj, 0);
  CreateJsonSerializer(Serializer);

  Serializable := GetSerializableInterface(FloatObj);
  Serializable.Serialize(Serializer);

  Serializer.GetOutput(StrObj);
  StrObj.GetCharPtr(@Str);
  Assert.AreEqual(string(Str), '0.0');
end;

procedure TTest_JsonSerializer.FloatMax;
const FloatMax = 1.7976931348623157e308;
var
  FloatObj: IFloat;
  Serializable: ISerializable;
  Serializer: ISerializer;
  StrObj: IString;
  Str: PAnsiChar;
begin
  CreateFloat(FloatObj, FloatMax);
  CreateJsonSerializer(Serializer);

  Serializable := GetSerializableInterface(FloatObj);
  Serializable.Serialize(Serializer);

  Serializer.GetOutput(StrObj);
  StrObj.GetCharPtr(@Str);
  Assert.AreEqual(string(Str), '1.7976931348623157e308');
end;

procedure TTest_JsonSerializer.FloatMin;
const FloatMin = 2.2250738585072014e-308;
var
  FloatObj: IFloat;
  Serializable: ISerializable;
  Serializer: ISerializer;
  StrObj: IString;
  Str: PAnsiChar;
begin
  CreateFloat(FloatObj, FloatMin);
  CreateJsonSerializer(Serializer);

  Serializable := GetSerializableInterface(FloatObj);
  Serializable.Serialize(Serializer);

  Serializer.GetOutput(StrObj);
  StrObj.GetCharPtr(@Str);
  Assert.AreEqual(string(Str), '2.2250738585072014e-308');
end;

procedure TTest_JsonSerializer.IntZero;
var
  IntObj: IInteger;
  Serializable: ISerializable;
  Serializer: ISerializer;
  StrObj: IString;
  Str: PAnsiChar;
begin
  CreateInteger(IntObj, 0);
  CreateJsonSerializer(Serializer);

  Serializable := GetSerializableInterface(IntObj);
  Serializable.Serialize(Serializer);

  Serializer.GetOutput(StrObj);
  StrObj.GetCharPtr(@Str);
  Assert.AreEqual(string(Str), '0');
end;

procedure TTest_JsonSerializer.IntMax;
const IntMax = 9223372036854775807;
var
  IntObj: IInteger;
  Serializable: ISerializable;
  Serializer: ISerializer;
  StrObj: IString;
  Str: PAnsiChar;
begin
  CreateInteger(IntObj, IntMax);
  CreateJsonSerializer(Serializer);

  Serializable := GetSerializableInterface(IntObj);
  Serializable.Serialize(Serializer);

  Serializer.GetOutput(StrObj);
  StrObj.GetCharPtr(@Str);
  Assert.AreEqual(string(Str), '9223372036854775807');
end;

procedure TTest_JsonSerializer.IntMin;
const IntMin = -9223372036854775808;
var
  IntObj: IInteger;
  Serializable: ISerializable;
  Serializer: ISerializer;
  StrObj: IString;
  Str: PAnsiChar;
begin
  CreateInteger(IntObj, IntMin);
  CreateJsonSerializer(Serializer);

  Serializable := GetSerializableInterface(IntObj);
  Serializable.Serialize(Serializer);

  Serializer.GetOutput(StrObj);
  StrObj.GetCharPtr(@Str);
  Assert.AreEqual(string(Str), '-9223372036854775808');
end;

procedure TTest_JsonSerializer.AsciiStr;
const Ascii = ' !"#$%&''()*+''-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijklmnopqrstuvwxyz{|}~';
const QuotedAndEscaped = '" !\"#$%&''()*+''-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~"';
var
  Serializable: ISerializable;
  Serializer: ISerializer;
  StringObj: IString;
  StrObj: IString;
  Str: PAnsiChar;
begin
  CreateString(StringObj, Ascii);
  CreateJsonSerializer(Serializer);

  Serializable := GetSerializableInterface(StringObj);
  Serializable.Serialize(Serializer);

  Serializer.GetOutput(StrObj);
  StrObj.GetCharPtr(@Str);
  Assert.AreEqual(string(Str), QuotedAndEscaped);
end;

procedure TTest_JsonSerializer.EmptyList;
var
  Serializable: ISerializable;
  Serializer: ISerializer;
  EmptyList: IListObject;
  StrObj: IString;
  Str: PAnsiChar;
begin
  CreateList(EmptyList);
  CreateJsonSerializer(Serializer);

  Serializable := GetSerializableInterface(EmptyList);
  Serializable.Serialize(Serializer);

  Serializer.GetOutput(StrObj);
  StrObj.GetCharPtr(@Str);
  Assert.AreEqual(string(Str), '[]');
end;

procedure TTest_JsonSerializer.StringListOne;
var
  Serializable: ISerializable;
  Serializer: ISerializer;
  ArrayOnItem: IListObject;
  StrObj: IString;
  Str: PAnsiChar;
  StringObj: IString;
begin
  CreateList(ArrayOnItem);
  CreateString(StringObj, 'Item1');
  ArrayOnItem.PushBack(IBaseObject(StringObj));
  CreateJsonSerializer(Serializer);

  Serializable := GetSerializableInterface(ArrayOnItem);
  Serializable.Serialize(Serializer);

  Serializer.GetOutput(StrObj);
  StrObj.GetCharPtr(@Str);
  Assert.AreEqual(string(Str), '["Item1"]');
end;

procedure TTest_JsonSerializer.StringListMultiple;
var
  Serializable: ISerializable;
  Serializer: ISerializer;
  ArrayOnItem: IListObject;
  StrObj: IString;
  Str: PAnsiChar;
  StringObj1, StringObj2: IString;
begin
  CreateList(ArrayOnItem);
  CreateString(StringObj1, 'Item1');
  CreateString(StringObj2, 'Item2');
  ArrayOnItem.PushBack(IBaseObject(StringObj1));
  ArrayOnItem.PushBack(IBaseObject(StringObj2));
  CreateJsonSerializer(Serializer);

  Serializable := GetSerializableInterface(ArrayOnItem);
  Serializable.Serialize(Serializer);

  Serializer.GetOutput(StrObj);
  StrObj.GetCharPtr(@Str);
  Assert.AreEqual(string(Str), '["Item1","Item2"]');
end;

procedure TTest_JsonSerializer.FloatListOne;
var
  Serializable: ISerializable;
  Serializer: ISerializer;
  ArrayOnItem: IListObject;
  StrObj: IString;
  Str: PAnsiChar;
  FloatObj1: IFloat;
begin
  CreateList(ArrayOnItem);
  CreateFloat(FloatObj1, 0.0);
  ArrayOnItem.PushBack(IBaseObject(FloatObj1));
  CreateJsonSerializer(Serializer);

  Serializable := GetSerializableInterface(ArrayOnItem);
  Serializable.Serialize(Serializer);

  Serializer.GetOutput(StrObj);
  StrObj.GetCharPtr(@Str);
  Assert.AreEqual(string(Str), '[0.0]');
end;

procedure TTest_JsonSerializer.FloatListMultiple;
const FloatMax = 1.7976931348623157e308;
const FloatMin = 2.2250738585072014e-308;
var
  Serializable: ISerializable;
  Serializer: ISerializer;
  ArrayOnItem: IListObject;
  StrObj: IString;
  Str: PAnsiChar;
  FloatObj1, FloatObj2, FloatObj3 :IFloat;
begin
  CreateList(ArrayOnItem);
  CreateFloat(FloatObj1, 0.0);
  CreateFloat(FloatObj2, FloatMin);
  CreateFloat(FloatObj3, FloatMax);
  ArrayOnItem.PushBack(IBaseObject(FloatObj1));
  ArrayOnItem.PushBack(IBaseObject(FloatObj2));
  ArrayOnItem.PushBack(IBaseObject(FloatObj3));
  CreateJsonSerializer(Serializer);

  Serializable := GetSerializableInterface(ArrayOnItem);
  Serializable.Serialize(Serializer);

  Serializer.GetOutput(StrObj);
  StrObj.GetCharPtr(@Str);
  Assert.AreEqual(string(Str), '[0.0,2.2250738585072014e-308,1.7976931348623157e308]');
end;

procedure TTest_JsonSerializer.IntListOne;
var
  Serializable: ISerializable;
  Serializer: ISerializer;
  ArrayOnItem: IListObject;
  StrObj: IString;
  Str: PAnsiChar;
  IntObj1: IInteger;
begin
  CreateList(ArrayOnItem);
  CreateInteger(IntObj1, 0);
  ArrayOnItem.PushBack(IBaseObject(IntObj1));
  CreateJsonSerializer(Serializer);

  Serializable := GetSerializableInterface(ArrayOnItem);
  Serializable.Serialize(Serializer);

  Serializer.GetOutput(StrObj);
  StrObj.GetCharPtr(@Str);
  Assert.AreEqual(string(Str), '[0]');
end;

procedure TTest_JsonSerializer.IntListMultiple;
const IntMax = 9223372036854775807;
const IntMin = -9223372036854775808;
var
  Serializable: ISerializable;
  Serializer: ISerializer;
  ArrayOnItem: IListObject;
  StrObj: IString;
  Str: PAnsiChar;
  IntObj1, IntObj2, IntObj3: IInteger;
begin
  CreateList(ArrayOnItem);
  CreateInteger(IntObj1, 0);
  CreateInteger(IntObj2, IntMin);
  CreateInteger(IntObj3, IntMax);
  ArrayOnItem.PushBack(IBaseObject(IntObj1));
  ArrayOnItem.PushBack(IBaseObject(IntObj2));
  ArrayOnItem.PushBack(IBaseObject(IntObj3));
  CreateJsonSerializer(Serializer);

  Serializable := GetSerializableInterface(ArrayOnItem);
  Serializable.Serialize(Serializer);

  Serializer.GetOutput(StrObj);
  StrObj.GetCharPtr(@Str);
  Assert.AreEqual(string(Str), '[0,-9223372036854775808,9223372036854775807]');
end;

procedure TTest_JsonSerializer.IntFloatList;
const FloatMax = 1.7976931348623157e308;
const FloatMin = 2.2250738585072014e-308;
const IntMax = 9223372036854775807;
const IntMin = -9223372036854775808;
var
  Serializable: ISerializable;
  Serializer: ISerializer;
  ArrayOnItem: IListObject;
  StrObj: IString;
  Str: PAnsiChar;
  FloatObj1, FloatObj2, FloatObj3, FloatObj4, FloatObj5: IFloat;
  IntObj1, IntObj2, IntObj3, IntObj4, IntObj5: IInteger;
begin
  CreateList(ArrayOnItem);
  CreateFloat(FloatObj1, 0.0);
  CreateInteger(IntObj1, 0);
  CreateFloat(FloatObj2, -2.5);
  CreateFloat(FloatObj3, 1.5);
  CreateInteger(IntObj2, 1);
  CreateInteger(IntObj3, -2);
  CreateFloat(FloatObj4, FloatMin);
  CreateFloat(FloatObj5, FloatMax);
  CreateInteger(IntObj4, IntMin);
  CreateInteger(IntObj5, IntMax);
  ArrayOnItem.PushBack(IBaseObject(FloatObj1));
  ArrayOnItem.PushBack(IBaseObject(IntObj1));
  ArrayOnItem.PushBack(IBaseObject(FloatObj2));
  ArrayOnItem.PushBack(IBaseObject(FloatObj3));
  ArrayOnItem.PushBack(IBaseObject(IntObj2));
  ArrayOnItem.PushBack(IBaseObject(IntObj3));
  ArrayOnItem.PushBack(IBaseObject(FloatObj4));
  ArrayOnItem.PushBack(IBaseObject(FloatObj5));
  ArrayOnItem.PushBack(IBaseObject(IntObj4));
  ArrayOnItem.PushBack(IBaseObject(IntObj5));
  CreateJsonSerializer(Serializer);

  Serializable := GetSerializableInterface(ArrayOnItem);
  Serializable.Serialize(Serializer);

  Serializer.GetOutput(StrObj);
  StrObj.GetCharPtr(@Str);
  Assert.AreEqual(string(Str), '[0.0,0,-2.5,1.5,1,-2,2.2250738585072014e-308,1.7976931348623157e308,-9223372036854775808,9223372036854775807]');
end;

procedure TTest_JsonSerializer.MixedList;
const FloatMax = 1.7976931348623157e308;
const FloatMin = 2.2250738585072014e-308;
const IntMax = 9223372036854775807;
const IntMin = -9223372036854775808;
var
  Serializable: ISerializable;
  Serializer: ISerializer;
  ArrayOnItem: IListObject;
  StrObj: IString;
  Str: PAnsiChar;
  FloatObj1, FloatObj2, FloatObj3, FloatObj4, FloatObj5: IFloat;
  IntObj1, IntObj2, IntObj3, IntObj4, IntObj5: IInteger;
  StringObj: IString;
begin
  CreateList(ArrayOnItem);
  CreateFloat(FloatObj1, 0.0);
  CreateInteger(IntObj1, 0);
  CreateFloat(FloatObj2, -2.5);
  CreateFloat(FloatObj3, 1.5);
  CreateInteger(IntObj2, 1);
  CreateInteger(IntObj3, -2);
  CreateFloat(FloatObj4, FloatMin);
  CreateFloat(FloatObj5, FloatMax);
  CreateInteger(IntObj4, IntMin);
  CreateInteger(IntObj5, IntMax);
  CreateString(StringObj, 'Test1');
  ArrayOnItem.PushBack(IBaseObject(FloatObj1));
  ArrayOnItem.PushBack(IBaseObject(IntObj1));
  ArrayOnItem.PushBack(IBaseObject(FloatObj2));
  ArrayOnItem.PushBack(IBaseObject(FloatObj3));
  ArrayOnItem.PushBack(IBaseObject(IntObj2));
  ArrayOnItem.PushBack(IBaseObject(IntObj3));
  ArrayOnItem.PushBack(IBaseObject(FloatObj4));
  ArrayOnItem.PushBack(IBaseObject(FloatObj5));
  ArrayOnItem.PushBack(IBaseObject(IntObj4));
  ArrayOnItem.PushBack(IBaseObject(IntObj5));
  ArrayOnItem.PushBack(IBaseObject(StringObj));
  CreateJsonSerializer(Serializer);

  Serializable := GetSerializableInterface(ArrayOnItem);
  Serializable.Serialize(Serializer);

  Serializer.GetOutput(StrObj);
  StrObj.GetCharPtr(@Str);
  Assert.AreEqual(string(Str), '[0.0,0,-2.5,1.5,1,-2,2.2250738585072014e-308,1.7976931348623157e308,-9223372036854775808,9223372036854775807,"Test1"]');
end;

procedure TTest_JsonSerializer.StartTaggedNull;
var
  Serializer: ISerializer;

begin
  CreateJsonSerializer(Serializer);

  Assert.AreEqual(Serializer.StartTaggedObject(nil), OPENDAQ_ERR_ARGUMENT_NULL);
end;

procedure TTest_JsonSerializer.IsCompleteFalse;
var
  Serializer: ISerializer;
  Complete: Boolean;

begin
  CreateJsonSerializer(Serializer);
  Serializer.StartObject;

  Serializer.IsComplete(Complete);
  Assert.IsFalse(Complete);
end;

procedure TTest_JsonSerializer.IsCompleteEmpty;
var
  Serializer: ISerializer;
  Complete: Boolean;

begin
  CreateJsonSerializer(Serializer);
  Serializer.IsComplete(Complete);
  Assert.IsFalse(Complete);
end;

procedure TTest_JsonSerializer.IsCompleteEmptyObject;
var
  Serializer: ISerializer;
  Complete: Boolean;

begin
  CreateJsonSerializer(Serializer);
  Serializer.StartObject;
  Serializer.EndObject;
  Serializer.IsComplete(Complete);
  Assert.IsTrue(Complete);
end;

procedure TTest_JsonSerializer.Reset;
var
  Serializer: ISerializer;
  Complete: Boolean;

begin
  CreateJsonSerializer(Serializer);
  Serializer.StartObject;
  Serializer.EndObject;

  Serializer.IsComplete(Complete);
  Assert.IsTrue(Complete);

  Serializer.Reset;
  Serializer.IsComplete(Complete);
  Assert.IsFalse(Complete);
end;

procedure TTest_JsonSerializer.SimpleObject;
var
  Serializer: ISerializer;
  Str: AnsiString;
  StringObj: IString;
  StrVal: PAnsiChar;

begin
  CreateJsonSerializer(Serializer);
  Serializer.StartObject;
  Serializer.Key('test');
  Str := 'success';
  Serializer.WriteString(PAnsiChar(Str), SizeT(Length(Str)));
  Serializer.EndObject;

  Serializer.GetOutput(StringObj);
  StringObj.GetCharPtr(@StrVal);
  Assert.AreEqual(string(StrVal), '{"test":"success"}');
end;

procedure TTest_JsonSerializer.SimpleObjectKeyLength;
var
  Serializer: ISerializer;
  Str1, Str2: AnsiString;
  StringObj: IString;
  StrVal: PAnsiChar;

begin
  CreateJsonSerializer(Serializer);
  Serializer.StartObject;
  Str1 := 'test';
  Serializer.KeyRaw(PAnsiChar(Str1), SizeT(Length(Str1)));

  Str2 := 'success';
  Serializer.WriteString(PAnsiChar(Str2), SizeT(Length(Str2)));
  Serializer.EndObject;

  Serializer.GetOutput(StringObj);
  StringObj.GetCharPtr(@StrVal);
  Assert.AreEqual(string(StrVal), '{"test":"success"}');
end;

procedure TTest_JsonSerializer.ObjectZeroLengthKey;
var
  Serializer: ISerializer;

begin
  CreateJsonSerializer(Serializer);
  Serializer.StartObject;
  Assert.AreEqual(Serializer.Key(''), OPENDAQ_ERR_INVALIDPARAMETER);
end;

procedure TTest_JsonSerializer.ObjectNullKeyPtrInterface;
var
  Serializer: ISerializer;

begin
  CreateJsonSerializer(Serializer);
  Serializer.StartObject;
  Assert.AreEqual(Serializer.Key(PAnsiChar(nil)), OPENDAQ_ERR_ARGUMENT_NULL);
end;

procedure TTest_JsonSerializer.ObjectKeyNullSizeInterface;
var
  Serializer: ISerializer;

begin
  CreateJsonSerializer(Serializer);
  Serializer.StartObject;
  Assert.AreEqual(Serializer.KeyRaw(nil, 0), OPENDAQ_ERR_ARGUMENT_NULL);
end;

initialization
  TDSUnitTestEngine.RegisterUnitTest(TTest_JsonSerializer);

end.
