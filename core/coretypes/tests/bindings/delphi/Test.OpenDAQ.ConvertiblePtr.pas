unit Test.OpenDAQ.ConvertiblePtr;

interface
uses
  DunitX.TestFramework, DS.UT.DSUnitTestUnit, OpenDAQ.CoreTypes;

type
  [TestFixture]
  TTest_ConvertiblePtr = class(TDSUnitTest)
  public
    [Setup]
    procedure Setup; override;
    [TearDown]
    procedure TearDown; override;
    [Test]
    procedure FloatToInt;
    [Test]
    procedure StringToInt;
    [Test]
    procedure IntToInt;
    [Test]
    procedure BoolToInt;
    [Test]
    procedure FloatToBool;
    [Test]
    procedure StringToBool;
    [Test]
    procedure IntToBool;
    [Test]
    procedure BoolToBool;
    [Test]
    procedure FloatToFloat;
    [Test]
    procedure StringToFloat;
    [Test]
    procedure IntToFloat;
    [Test]
    procedure BoolToFloat;
  end;

implementation
uses
  WinApi.Windows,
  System.SysUtils,
  OpenDAQ.TString,
  DS.UT.DSUnitTestEngineUnit,
  OpenDAQ.CoreTypes.Errors,
  OpenDAQ.Integer,
  OpenDAQ.Float,
  OpenDAQ.Boolean,
  OpenDAQ.Exceptions,
  OpenDAQ.Convertible;

{ TTest_ConvertibleObject }

procedure TTest_ConvertiblePtr.Setup();
begin
end;

procedure TTest_ConvertiblePtr.TearDown();
begin
end;

procedure TTest_ConvertiblePtr.IntToBool();
var
  IntObj: IIntegerPtr;
  Conv : IConvertible;
  ConvPtr : IConvertiblePtr;
  BoolValue: Bool;
begin
  IntObj := TIntegerPtr.Create(1);
  Conv := IntObj as IConvertible;
  Assert.IsNotNull(Conv);

  ConvPtr := IntObj as IConvertiblePtr;
  Assert.IsNotNull(ConvPtr);

  BoolValue := ConvPtr.ToBool();
  Assert.IsTrue(BoolValue);

  IntObj := TIntegerPtr.Create(0);
  BoolValue := (IntObj as IConvertiblePtr).ToBool();
  Assert.IsFalse(BoolValue);
end;

procedure TTest_ConvertiblePtr.IntToFloat();
var
  IntObj: IIntegerPtr;
  FloatValue: RtFloat;
begin
  IntObj := TIntegerPtr.Create(5);
  FloatValue := (IntObj as IConvertiblePtr).ToFloat();
  Assert.AreEqual<RtFloat>(FloatValue, 5.0);
end;

procedure TTest_ConvertiblePtr.IntToInt();
var
  IntObj: IIntegerPtr;
  IntValue: RtInt;
begin
  IntObj := TIntegerPtr.Create(5);
  IntValue := (IntObj as IConvertiblePtr).ToInt();
  Assert.AreEqual<RtInt>(IntValue, 5);
end;

procedure TTest_ConvertiblePtr.StringToBool();
var
  StringObj: IStringPtr;
  BoolValue: Boolean;
begin
  StringObj := TStringPtr.Create('1');
  BoolValue := (StringObj as IConvertiblePtr).ToBool();
  Assert.IsTrue(BoolValue);

  StringObj := TStringPtr.Create('0');
  BoolValue := (StringObj as IConvertiblePtr).ToBool();
  Assert.IsFalse(BoolValue);

  StringObj := TStringPtr.Create('');
  BoolValue := (StringObj as IConvertiblePtr).ToBool();
  Assert.IsFalse(BoolValue);

  StringObj := TStringPtr.Create();

  Assert.WillRaise(procedure()
    begin
      BoolValue := (StringObj as IConvertiblePtr).ToBool();
    end,
    EIntfCastError
  );

  StringObj := TStringPtr.Create('a');
{$IFDEF TEST_EXCEPTIONS}
  Assert.WillRaise(procedure
    begin
      BaseObjectToInt(StringObj);
    end,
    EConvertError
  );
{$ENDIF}
end;

procedure TTest_ConvertiblePtr.StringToFloat();
var
  StringObj: IStringPtr;
  FloatValue: RtFloat;
begin
  StringObj := TStringPtr.Create('5.2');
  FloatValue := (StringObj as IConvertiblePtr).ToFloat();
  Assert.AreEqual<RtFloat>(FloatValue, 5.2);

  StringObj := TStringPtr.Create('a');
{$IFDEF TEST_EXCEPTIONS}
  Assert.WillRaise(procedure
    begin
      BaseObjectToInt(StringObj);
    end,
    EConvertError
  );
{$ENDIF}

end;

procedure TTest_ConvertiblePtr.StringToInt();
var
  StringObj: IStringPtr;
  IntValue: RtInt;
begin
  StringObj := TStringPtr.Create('5');
  IntValue := BaseObjectToInt(StringObj);
  Assert.AreEqual<RtInt>(IntValue, 5);

  StringObj := TStringPtr.Create('a');
{$IFDEF TEST_EXCEPTIONS}
  Assert.WillRaise(procedure
    begin
      BaseObjectToInt(StringObj);
    end,
    EConvertError
  );
{$ENDIF}
end;

procedure TTest_ConvertiblePtr.BoolToBool();
var
  BoolObj: IBooleanPtr;
  BoolValue: Boolean;
begin
  BoolObj := TBooleanPtr.Create(True);
  BoolValue := (BoolObj as IConvertiblePtr).ToBool();
  Assert.IsTrue(BoolValue);

  BoolObj := TBooleanPtr.Create(False);
  BoolValue := (BoolObj as IConvertiblePtr).ToBool();
  Assert.IsFalse(BoolValue);
end;

procedure TTest_ConvertiblePtr.BoolToFloat();
var
  BoolObj: IBooleanPtr;
  FloatValue: RtFloat;
begin
  BoolObj := TBooleanPtr.Create(True);
  FloatValue := (BoolObj as IConvertiblePtr).ToFloat();
  Assert.AreEqual<RtFloat>(FloatValue, 1);

  BoolObj := TBooleanPtr.Create(False);
  FloatValue := (BoolObj as IConvertiblePtr).ToFloat();
  Assert.AreEqual<RtFloat>(FloatValue, 0);
end;

procedure TTest_ConvertiblePtr.BoolToInt();
var
  BoolObj: IBooleanPtr;
  IntValue: RtInt;
begin
  BoolObj := TBooleanPtr.Create(True);
  IntValue := (BoolObj as IConvertiblePtr).ToInt();
  Assert.AreEqual<RtInt>(IntValue, 1);

  BoolObj := TBooleanPtr.Create(False);
  IntValue := (BoolObj as IConvertiblePtr).ToInt();
  Assert.AreEqual<RtInt>(IntValue, 0);
end;

procedure TTest_ConvertiblePtr.FloatToBool();
var
  FloatObj: IFloatPtr;
  BoolValue: Boolean;
begin
  FloatObj := TFloatPtr.Create(1.0);
  BoolValue := (FloatObj as IConvertiblePtr).ToBool();
  Assert.IsTrue(BoolValue = True);

  FloatObj := TFloatPtr.Create(0.0);
  BoolValue := (FloatObj as IConvertiblePtr).ToBool();
  Assert.IsTrue(BoolValue = False);
end;

procedure TTest_ConvertiblePtr.FloatToFloat();
var
  FloatObj: IFloatPtr;
  FloatValue: RtFloat;
begin
  FloatObj := TFloatPtr.Create(1.0);
  FloatValue := (FloatObj as IConvertiblePtr).ToFloat();
  Assert.IsTrue(FloatValue = 1.0);
end;

procedure TTest_ConvertiblePtr.FloatToInt();
var
  FloatObj: IFloatPtr;
  IntValue: RtInt;
begin
  FloatObj := TFloatPtr.Create(5.0);
  IntValue := (FloatObj as IConvertiblePtr).ToInt();
  Assert.IsTrue(IntValue = 5);
end;

initialization
  TDSUnitTestEngine.RegisterUnitTest(TTest_ConvertiblePtr);

end.
