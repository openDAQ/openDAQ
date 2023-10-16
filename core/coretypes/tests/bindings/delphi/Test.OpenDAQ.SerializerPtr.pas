unit Test.OpenDAQ.SerializerPtr;

interface

uses
  DunitX.TestFramework, DS.UT.DSUnitTestUnit, OpenDAQ.CoreTypes;

type
  [TestFixture]
  TTest_BB_SerializerPtr = class(TDSUnitTest)
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
    procedure NullString;
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
  WinApi.Windows,
  System.SysUtils,
  OpenDAQ.List,
  OpenDAQ.TString,
  OpenDAQ.Integer,
  OpenDAQ.Boolean,
  OpenDAQ.Float,
  DS.UT.DSUnitTestEngineUnit,
  OpenDAQ.CoreTypes.Errors,
  OpenDAQ.Exceptions,
  OpenDAQ.Serializer,
  OpenDAQ.Serializable;

{ TTest.BB.SerializerPtr }

procedure TTest_BB_SerializerPtr.Setup;
begin
end;

procedure TTest_BB_SerializerPtr.TearDown;
begin
end;

procedure TTest_BB_SerializerPtr.BoolTrue();
var
  BoolObj: IBooleanPtr;
  Serializable: ISerializablePtr;
  Serializer: ISerializerPtr;
begin
  BoolObj := TBooleanPtr.Create(True);
  Serializer := TSerializerPtr.Create();

  Serializable := BoolObj as ISerializablePtr;
  Serializable.Serialize(Serializer as ISerializer);

  Assert.AreEqual(Serializer.GetOutputString(), 'true');
end;

procedure TTest_BB_SerializerPtr.BoolFalse();
var
  BoolObj: IBooleanPtr;
  Serializable: ISerializablePtr;
  Serializer: ISerializerPtr;
begin
  BoolObj := TBooleanPtr.Create(False);
  Serializer := TSerializerPtr.Create();

  Serializable := BoolObj as ISerializablePtr;
  Serializable.Serialize(Serializer as ISerializer);

  Assert.AreEqual(Serializer.GetOutputString(), 'false');
end;

procedure TTest_BB_SerializerPtr.FloatZero();
var
  FloatObj: IFloatPtr;
  Serializable: ISerializablePtr;
  Serializer: ISerializerPtr;
begin
  FloatObj := TFloatPtr.Create(0);
  Serializer := TSerializerPtr.Create();

  Serializable := FloatObj as ISerializablePtr;
  Serializable.Serialize(Serializer as ISerializer);

  Assert.AreEqual(Serializer.GetOutputString(), '0.0');
end;

procedure TTest_BB_SerializerPtr.FloatMax();
const FloatMax = 1.7976931348623157e308;
var
  FloatObj: IFloatPtr;
  Serializable: ISerializablePtr;
  Serializer: ISerializerPtr;
begin
  FloatObj := TFloatPtr.Create(FloatMax);
  Serializer := TSerializerPtr.Create();

  Serializable := FloatObj as ISerializablePtr;
  Serializable.Serialize(Serializer as ISerializer);

  Assert.AreEqual(Serializer.GetOutputString(), '1.7976931348623157e308');
end;

procedure TTest_BB_SerializerPtr.FloatMin();
const FloatMin = 2.2250738585072014e-308;
var
  FloatObj: IFloatPtr;
  Serializable: ISerializablePtr;
  Serializer: ISerializerPtr;
begin
  FloatObj := TFloatPtr.Create(FloatMin);
  Serializer := TSerializerPtr.Create();

  Serializable := FloatObj as ISerializablePtr;
  Serializable.Serialize(Serializer as ISerializer);

  Assert.AreEqual(Serializer.GetOutputString(), '2.2250738585072014e-308');
end;

procedure TTest_BB_SerializerPtr.IntZero();
var
  IntObj: IIntegerPtr;
  Serializable: ISerializablePtr;
  Serializer: ISerializerPtr;
begin
  IntObj := TIntegerPtr.Create(0);
  Serializer := TSerializerPtr.Create();

  Serializable := IntObj as ISerializablePtr;
  Serializable.Serialize(Serializer as ISerializer);

  Assert.AreEqual(Serializer.GetOutputString(), '0');
end;

procedure TTest_BB_SerializerPtr.IntMax();
const IntMax = 9223372036854775807;
var
  IntObj: IIntegerPtr;
  Serializable: ISerializablePtr;
  Serializer: ISerializerPtr;
begin
  IntObj := TIntegerPtr.Create(IntMax);
  Serializer := TSerializerPtr.Create();

  Serializable := IntObj as ISerializablePtr;
  Serializable.Serialize(Serializer as ISerializer);

  Assert.AreEqual(Serializer.GetOutputString(), '9223372036854775807');
end;

procedure TTest_BB_SerializerPtr.IntMin();
const IntMin = -9223372036854775808;
var
  IntObj: IIntegerPtr;
  Serializable: ISerializablePtr;
  Serializer: ISerializerPtr;
begin
  IntObj := TIntegerPtr.Create(IntMin);
  Serializer := TSerializerPtr.Create();

  Serializable := IntObj as ISerializablePtr;
  Serializable.Serialize(Serializer as ISerializer);

  Assert.AreEqual(Serializer.GetOutputString(), '-9223372036854775808');
end;

procedure TTest_BB_SerializerPtr.AsciiStr();
const Ascii = ' !"#$%&''()*+''-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijklmnopqrstuvwxyz{|}~';
const QuotedAndEscaped = '" !\"#$%&''()*+''-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~"';
var
  Serializable: ISerializablePtr;
  Serializer: ISerializerPtr;
  StringObj: IStringPtr;
begin
  StringObj := TStringPtr.Create(Ascii);
  Serializer := TSerializerPtr.Create();

  Serializable := StringObj as ISerializablePtr;
  Serializable.Serialize(Serializer as ISerializer);

  Assert.AreEqual(Serializer.GetOutputString(), QuotedAndEscaped);
end;

procedure TTest_BB_SerializerPtr.NullString();
var
  Serializable: ISerializablePtr;
  Serializer: ISerializerPtr;
  StringObj: IStringPtr;
begin
  StringObj := TStringPtr.Create(nil);
  Serializer := TSerializerPtr.Create();

  Serializable := StringObj as ISerializablePtr;
  Serializable.Serialize(Serializer as ISerializer);

  Assert.AreEqual(Serializer.GetOutputString(), '""');
end;

procedure TTest_BB_SerializerPtr.EmptyList();
var
  Serializable: ISerializablePtr;
  Serializer: ISerializerPtr;
  EmptyList: IListPtr<IBaseObject>;
begin
  EmptyList := TListPtr<IBaseObject>.Create();
  Serializer := TSerializerPtr.Create();

  Serializable := EmptyList as ISerializablePtr;
  Serializable.Serialize(Serializer as ISerializer);

  Assert.AreEqual(Serializer.GetOutputString(), '[]');
end;

procedure TTest_BB_SerializerPtr.StringListOne();
var
  Serializable: ISerializablePtr;
  Serializer: ISerializerPtr;
  ArrayOneItem: IListPtr<IString>;
begin
  Serializer := TSerializerPtr.Create();
  ArrayOneItem := TListPtr<IString>.Create();
  ArrayOneItem.PushBack('Item1');

  Serializable := ArrayOneItem as ISerializablePtr;
  Serializable.Serialize(Serializer as ISerializer);

  Assert.AreEqual(Serializer.GetOutputString(), '["Item1"]');
end;

procedure TTest_BB_SerializerPtr.StringListMultiple();
var
  Serializable: ISerializablePtr;
  Serializer: ISerializerPtr;
  ArrayMultipleItems: IListPtr<IString>;
begin
  Serializer := TSerializerPtr.Create();
  ArrayMultipleItems := TListPtr<IString>.Create();

  ArrayMultipleItems.PushBack('Item1');
  ArrayMultipleItems.PushBack('Item2');

  Serializable := ArrayMultipleItems as ISerializablePtr;
  Serializable.Serialize(Serializer as ISerializer);

  Assert.AreEqual(Serializer.GetOutputString(), '["Item1","Item2"]');
end;

procedure TTest_BB_SerializerPtr.FloatListOne();
var
  Serializable: ISerializablePtr;
  Serializer: ISerializerPtr;
  ArrayOneItem: IListPtr<IFloat>;
begin
  Serializer := TSerializerPtr.Create();
  ArrayOneItem := TListPtr<IFloat>.Create();
  ArrayOneItem.PushBack(0.0);

  Serializable := ArrayOneItem as ISerializablePtr;
  Serializable.Serialize(Serializer as ISerializer);

  Assert.AreEqual(Serializer.GetOutputString(), '[0.0]');
end;

procedure TTest_BB_SerializerPtr.FloatListMultiple();
const FloatMax = 1.7976931348623157e308;
const FloatMin = 2.2250738585072014e-308;
var
  Serializable: ISerializablePtr;
  Serializer: ISerializerPtr;
  ArrayMultipleItems: IListPtr<IFloat>;
begin
  ArrayMultipleItems := TListPtr<IFLoat>.Create();

  ArrayMultipleItems.PushBack(0.0);
  ArrayMultipleItems.PushBack(FloatMin);
  ArrayMultipleItems.PushBack(FloatMax);

  Serializer := TSerializerPtr.Create();
  Serializable := ArrayMultipleItems as ISerializablePtr;
  Serializable.Serialize(Serializer as ISerializer);

  Assert.AreEqual(Serializer.GetOutputString(), '[0.0,2.2250738585072014e-308,1.7976931348623157e308]');
end;

procedure TTest_BB_SerializerPtr.IntListOne();
var
  Serializable: ISerializablePtr;
  Serializer: ISerializerPtr;
  ArrayOneItem: IListPtr<IInteger>;
begin
  ArrayOneItem := TListPtr<IInteger>.Create();
  ArrayOneItem.PushBack(0);

  Serializer := TSerializerPtr.Create();
  Serializable := ArrayOneItem as ISerializablePtr;
  Serializable.Serialize(Serializer as ISerializer);

  Assert.AreEqual(Serializer.GetOutputString(), '[0]');
end;

procedure TTest_BB_SerializerPtr.IntListMultiple();
const IntMax = 9223372036854775807;
const IntMin = -9223372036854775808;
var
  Serializable: ISerializablePtr;
  Serializer: ISerializerPtr;
  ArrayMultipleItems: IListPtr<IInteger>;
begin
  ArrayMultipleItems := TListPtr<IInteger>.Create();
  ArrayMultipleItems.PushBack(0);
  ArrayMultipleItems.PushBack(IntMin);
  ArrayMultipleItems.PushBack(IntMax);

  Serializer := TSerializerPtr.Create();
  Serializable := ArrayMultipleItems as ISerializablePtr;
  Serializable.Serialize(Serializer as ISerializer);

  Assert.AreEqual(Serializer.GetOutputString(), '[0,-9223372036854775808,9223372036854775807]');
end;

procedure TTest_BB_SerializerPtr.IntFloatList();
const FloatMax = 1.7976931348623157e308;
const FloatMin = 2.2250738585072014e-308;
const IntMax = 9223372036854775807;
const IntMin = -9223372036854775808;
var
  Serializable: ISerializablePtr;
  Serializer: ISerializerPtr;
  MixedArray: IListPtr<IBaseObject>;
begin
  MixedArray := TListPtr<IBaseObject>.Create();
  MixedArray.PushBack(0.0);
  MixedArray.PushBack(0);
  MixedArray.PushBack(-2.5);
  MixedArray.PushBack(1.5);
  MixedArray.PushBack(1);
  MixedArray.PushBack(-2);
  MixedArray.PushBack(FloatMin);
  MixedArray.PushBack(FloatMax);
  MixedArray.PushBack(IntMin);
  MixedArray.PushBack(IntMax);

  Serializer := TSerializerPtr.Create();
  Serializable := MixedArray as ISerializablePtr;
  Serializable.Serialize(Serializer as ISerializer);

  Assert.AreEqual(Serializer.GetOutputString(), '[0.0,0,-2.5,1.5,1,-2,2.2250738585072014e-308,1.7976931348623157e308,-9223372036854775808,9223372036854775807]');
end;

procedure TTest_BB_SerializerPtr.MixedList();
const FloatMax = 1.7976931348623157e308;
const FloatMin = 2.2250738585072014e-308;
const IntMax = 9223372036854775807;
const IntMin = -9223372036854775808;
var
  Serializable: ISerializablePtr;
  Serializer: ISerializerPtr;
  MixedArray: IListPtr<IBaseObject>;
begin
  MixedArray := TListPtr<IBaseObject>.Create();
  MixedArray.PushBack(0.0);
  MixedArray.PushBack(0);
  MixedArray.PushBack(-2.5);
  MixedArray.PushBack(1.5);
  MixedArray.PushBack(1);
  MixedArray.PushBack(-2);
  MixedArray.PushBack(FloatMin);
  MixedArray.PushBack(FloatMax);
  MixedArray.PushBack(IntMin);
  MixedArray.PushBack(IntMax);
  MixedArray.PushBack('Test1');

  Serializer := TSerializerPtr.Create();
  Serializable := MixedArray as ISerializablePtr;
  Serializable.Serialize(Serializer as ISerializer);

  Assert.AreEqual(Serializer.GetOutputString(), '[0.0,0,-2.5,1.5,1,-2,2.2250738585072014e-308,1.7976931348623157e308,-9223372036854775808,9223372036854775807,"Test1"]');
end;

procedure TTest_BB_SerializerPtr.StartTaggedNull();
var
  Serializer: ISerializerPtr;
begin
  Serializer := TSerializerPtr.Create();

  Assert.WillRaise(procedure()
    begin
      Serializer.StartTaggedObject(nil);
    end,
    ERTArgumentNullException
  );
end;

procedure TTest_BB_SerializerPtr.IsCompleteFalse();
var
  Serializer: ISerializerPtr;
begin
  Serializer := TSerializerPtr.Create();
  Serializer.StartObject();

  Assert.IsFalse(Serializer.IsComplete());
end;

procedure TTest_BB_SerializerPtr.IsCompleteEmpty;
var
  Serializer: ISerializerPtr;
begin
  Serializer := TSerializerPtr.Create();
  Assert.IsFalse(Serializer.IsComplete());
end;

procedure TTest_BB_SerializerPtr.IsCompleteEmptyObject();
var
  Serializer: ISerializerPtr;
begin
  Serializer := TSerializerPtr.Create();
  Serializer.StartObject();
  Serializer.EndObject();

  Assert.IsTrue(Serializer.IsComplete());
end;

procedure TTest_BB_SerializerPtr.Reset();
var
  Serializer: ISerializerPtr;
begin
  Serializer := TSerializerPtr.Create();
  Serializer.StartObject();
  Serializer.EndObject();

  Assert.IsTrue(Serializer.IsComplete());

  Serializer.Reset();
  Assert.IsFalse(Serializer.IsComplete());
end;

procedure TTest_BB_SerializerPtr.SimpleObject();
var
  Serializer: ISerializerPtr;
begin
  Serializer := TSerializerPtr.Create();
  Serializer.StartObject();
    Serializer.Key('test');
    Serializer.WriteString('success');
  Serializer.EndObject();

  Assert.AreEqual(Serializer.GetOutputString(), '{"test":"success"}');
end;
//
procedure TTest_BB_SerializerPtr.SimpleObjectKeyLength;
var
  Serializer: ISerializerPtr;
  Str1, Str2: AnsiString;
begin
  Serializer := TSerializerPtr.Create();
  Serializer.StartObject();
    Str1 := 'test';
    Serializer.Key(PAnsiChar(Str1), SizeT(Length(Str1)));

    Str2 := 'success';
    Serializer.WriteString(PAnsiChar(Str2), SizeT(Length(Str2)));
  Serializer.EndObject();

  Assert.AreEqual(Serializer.GetOutputString(), '{"test":"success"}');
end;
//
procedure TTest_BB_SerializerPtr.ObjectZeroLengthKey;
var
  Serializer: ISerializerPtr;

begin
  Serializer := TSerializerPtr.Create();
  Serializer.StartObject();

  Assert.WillRaise(procedure()
    begin
      Serializer.Key('');
    end,
    ERTInvalidParameterException
  );
end;

procedure TTest_BB_SerializerPtr.ObjectNullKeyPtrInterface();
var
  Serializer: ISerializerPtr;
begin
  Serializer := TSerializerPtr.Create();
  Serializer.StartObject();

  Assert.WillRaise(procedure()
    begin
      Serializer.Key(PAnsiChar(nil))
    end,
    ERTArgumentNullException
  );
end;

procedure TTest_BB_SerializerPtr.ObjectKeyNullSizeInterface;
var
  Serializer: ISerializerPtr;
begin
  Serializer := TSerializerPtr.Create();
  Serializer.StartObject();

  Assert.WillRaise(procedure()
    begin
      Serializer.Key(nil, 0)
    end,
    ERTArgumentNullException
  );
end;

initialization
  TDSUnitTestEngine.RegisterUnitTest(TTest_BB_SerializerPtr);

end.
