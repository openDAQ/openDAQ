unit Test.OpenDAQ.ConvertibleObject;

interface
uses
  DunitX.TestFramework, DS.UT.DSUnitTestUnit, OpenDAQ.CoreTypes;

type
  [TestFixture]
  TTest_ConvertibleObject = class(TDSUnitTest)
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
  DS.UT.DSUnitTestEngineUnit, WinApi.Windows, OpenDAQ.CoreTypes.Errors, SysUtils;

{ TTest_ConvertibleObject }

procedure TTest_ConvertibleObject.TearDown;
begin
end;

procedure TTest_ConvertibleObject.Setup;
begin
end;

procedure TTest_ConvertibleObject.IntToBool;
var
  IntObj: IInteger;
  BoolValue: Bool;
begin
  CreateInteger(IntObj, 1);
  BoolValue := BaseObjectToBool(IntObj);
  Assert.IsTrue(BoolValue = True);

  CreateInteger(IntObj, 0);
  BoolValue := BaseObjectToBool(IntObj);
  Assert.IsTrue(BoolValue = False);
end;

procedure TTest_ConvertibleObject.IntToFloat;
var
  IntObj: IInteger;
  FloatValue: Double;
begin
  CreateInteger(IntObj, 5);
  FloatValue := BaseObjectToFloat(IntObj);
  Assert.IsTrue(FloatValue = 5.0);
end;

procedure TTest_ConvertibleObject.IntToInt;
var
  IntObj: IInteger;
  IntValue: Int64;
begin
  CreateInteger(IntObj, 5);
  IntValue := BaseObjectToInt(IntObj);
  Assert.IsTrue(IntValue = 5);
end;

procedure TTest_ConvertibleObject.StringToBool;
var
  StringObj: IString;
  BoolValue: ByteBool;
begin
  CreateString(StringObj, '1');
  BoolValue := BaseObjectToBool(StringObj);
  Assert.IsTrue(BoolValue = True);

  CreateString(StringObj, '0');
  BoolValue := BaseObjectToBool(StringObj);
  Assert.IsTrue(BoolValue = False);

  CreateString(StringObj, '');
  BoolValue := BaseObjectToBool(StringObj);
  Assert.IsTrue(BoolValue = False);

  CreateString(StringObj, nil);
  BoolValue := BaseObjectToBool(StringObj);
  Assert.IsTrue(BoolValue = False);

  CreateString(StringObj, 'a');
{$IFDEF TEST_EXCEPTIONS}
  Assert.WillRaise(procedure
    begin
      BaseObjectToInt(StringObj);
    end,
    EConvertError
  );
{$ENDIF}
end;

procedure TTest_ConvertibleObject.StringToFloat;
var
  StringObj: IString;
  FloatValue: RtFloat;
begin
  CreateString(StringObj, '5.2');
  FloatValue := BaseObjectToFloat(StringObj);
  Assert.AreEqual<RtFloat>(FloatValue, 5.2);

  CreateString(StringObj, 'a');
{$IFDEF TEST_EXCEPTIONS}
  Assert.WillRaise(procedure
    begin
      BaseObjectToInt(StringObj);
    end,
    EConvertError
  );
{$ENDIF}

end;

procedure TTest_ConvertibleObject.StringToInt;
var
  StringObj: IString;
  IntValue: Int64;
begin
  CreateString(StringObj, '5');
  IntValue := BaseObjectToInt(StringObj);
  Assert.IsTrue(IntValue = 5);

  CreateString(StringObj, 'a');
{$IFDEF TEST_EXCEPTIONS}
  Assert.WillRaise(procedure
    begin
      BaseObjectToInt(StringObj);
    end,
    EConvertError
  );
{$ENDIF}
end;

procedure TTest_ConvertibleObject.BoolToBool;
var
  BoolObj: IBoolean;
  BoolValue: ByteBool;
begin
  CreateBoolean(BoolObj, True);
  BoolValue := BaseObjectToBool(BoolObj);
  Assert.IsTrue(BoolValue = True);

  CreateBoolean(BoolObj, False);
  BoolValue := BaseObjectToBool(BoolObj);
  Assert.IsTrue(BoolValue = False);
end;

procedure TTest_ConvertibleObject.BoolToFloat;
var
  BoolObj: IBoolean;
  FloatValue: Double;
begin
  CreateBoolean(BoolObj, True);
  FloatValue := BaseObjectToInt(BoolObj);
  Assert.IsTrue(FloatValue <> 0);

  CreateBoolean(BoolObj, False);
  FloatValue := BaseObjectToInt(BoolObj);
  Assert.IsTrue(FloatValue = 0);
end;

procedure TTest_ConvertibleObject.BoolToInt;
var
  BoolObj: IBoolean;
  IntValue: Int64;
begin
  CreateBoolean(BoolObj, True);
  IntValue := BaseObjectToInt(BoolObj);
  Assert.IsTrue(IntValue <> 0);

  CreateBoolean(BoolObj, False);
  IntValue := BaseObjectToInt(BoolObj);
  Assert.IsTrue(IntValue = 0);
end;

procedure TTest_ConvertibleObject.FloatToBool;
var
  FloatObj: IFloat;
  BoolValue: ByteBool;
begin
  CreateFloat(FloatObj, 1.0);
  BoolValue := BaseObjectToBool(FloatObj);
  Assert.IsTrue(BoolValue = True);

  CreateFloat(FloatObj, 0.0);
  BoolValue := BaseObjectToBool(FloatObj);
  Assert.IsTrue(BoolValue = False);
end;

procedure TTest_ConvertibleObject.FloatToFloat;
var
  FloatObj: IFloat;
  FloatValue: Double;
begin
  CreateFloat(FloatObj, 1.0);
  FloatValue := BaseObjectToFloat(FloatObj);
  Assert.IsTrue(FloatValue = 1.0);
end;

procedure TTest_ConvertibleObject.FloatToInt;
var
  FloatObj: IFloat;
  IntValue: Int64;
begin
  CreateFloat(FloatObj, 5.0);
  IntValue := BaseObjectToInt(FloatObj);
  Assert.IsTrue(IntValue = 5);
end;

initialization
  TDSUnitTestEngine.RegisterUnitTest(TTest_ConvertibleObject);

end.
