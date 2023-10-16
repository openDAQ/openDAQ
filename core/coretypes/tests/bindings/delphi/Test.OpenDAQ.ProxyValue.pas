unit Test.OpenDAQ.ProxyValue;

interface

uses
  DunitX.TestFramework, DS.UT.DSUnitTestUnit, OpenDAQ.CoreTypes;

type
  [TestFixture]
  TTest_ProxyValue = class(TDSUnitTest)
  private
  public
    [Test]
    procedure ConvertToInteger;
    [Test]
    procedure ConvertToIntegerInterface;
    [Test]
    procedure ConvertToIntegerInterfaceNil;
    [Test]
    procedure ConvertToIntegerPtrNil;
    [Test]
    procedure ConvertToIntegerPtr;
    [Test]
    procedure Test2;
    [Test]
    procedure Test3;
    [Test]
    procedure ConvertFromIntValue;
    [Test]
    procedure CreateFloatFromInt;
    [Test]
    procedure CreateFloatFromConvertible();
    [Test]
    procedure CreateIntFromConvertible();
    [Test]
    procedure ConvertFromString;
    [Test]
    procedure ConvertFromFloatValue;
    [Test]
    procedure ConvertFromBoolValueTrue;
    [Test]
    procedure ConvertFromBoolValueFalse;
    [Test]
    procedure ConvertToSmartPtr;
    [Test]
    procedure ConvertToSmartPtr2;
    [Test]
    procedure TestGetPtr;
    [Test]
    procedure TestGetInterface;
  end;

implementation
uses  
  WinApi.Windows,
  System.SysUtils,
  DS.UT.DSUnitTestEngineUnit,
  OpenDAQ.CoreTypes.Errors,
  OpenDAQ.Exceptions,
  OpenDAQ.Integer,
  OpenDAQ.Float,
  OpenDAQ.Boolean,
  OpenDAQ.ProxyValue,
  OpenDAQ.Convertible;

{ TTest_DictObject }

procedure TTest_ProxyValue.ConvertToInteger();
var
  IntObj : IInteger;
  Proxy : TProxyValue<IInteger>;
  IntValue : RtInt;
begin
  CreateInteger(IntObj, 5);

  Proxy := TProxyValue<IInteger>.Create(IntObj);
  IntValue := Proxy;

  Assert.AreEqual<RtInt>(IntValue, 5);
end;

procedure TTest_ProxyValue.ConvertToIntegerInterface;
var
  IntObj : IInteger;
  Proxy : TProxyValue<IInteger>;
  ConvertObj: IConvertible;
begin
  CreateInteger(IntObj, 5);

  Proxy := TProxyValue<IInteger>.Create(IntObj);
  ConvertObj := Proxy.AsInterfaceOrNil<IConvertible>();

  Assert.IsNotNull(ConvertObj);
end;

procedure TTest_ProxyValue.ConvertToIntegerInterfaceNil();
var
  IntObj : IInteger;
  Proxy : TProxyValue<IInteger>;
  FloatObj: IFloat;
begin
  CreateInteger(IntObj, 5);

  Proxy := TProxyValue<IInteger>.Create(IntObj);
  FloatObj := Proxy.AsInterfaceOrNil<IFloat>();

  Assert.IsNull(FloatObj);
end;

procedure TTest_ProxyValue.ConvertToIntegerPtr();
var
  IntObj : IInteger;
  Proxy : TProxyValue<IInteger>;
  ConvertPtr: IConvertiblePtr;
begin
  CreateInteger(IntObj, 5);

  Proxy := TProxyValue<IInteger>.Create(IntObj);
  ConvertPtr := Proxy.AsPtrOrNil<IConvertiblePtr>();

  Assert.IsNotNull(ConvertPtr);
end;

procedure TTest_ProxyValue.ConvertToIntegerPtrNil();
var
  IntObj : IInteger;
  Proxy : TProxyValue<IInteger>;
  FloatObj: IFloatPtr;
begin
  CreateInteger(IntObj, 5);

  Proxy := TProxyValue<IInteger>.Create(IntObj);
  FloatObj := Proxy.AsPtrOrNil<IFloatPtr>();

  Assert.IsNull(FloatObj);
end;

procedure TTest_ProxyValue.TestGetPtr();
var
  IntPtr : IIntegerPtr;
  IntVal : RtInt;
  Proxy : TProxyValue<IInteger>;
begin
  Proxy := 5;
  IntPtr := Proxy.AsPtr<IIntegerPtr>();

  if Assigned(IntPtr) then
  begin
    IntVal := IntPtr.GetValue();
    Assert.AreEqual<RtInt>(IntVal, 5);
  end;
end;

procedure TTest_ProxyValue.CreateFloatFromInt();
var
  FloatPtr: IFloatPtr;
  FloatVal : RtFloat;
  Proxy : TProxyValue<IFloat>;
begin
  Proxy := 5;
  FloatPtr := Proxy.AsPtr<IFloatPtr>();

  if Assigned(FloatPtr) then
  begin
    FloatVal := FloatPtr.GetValue();
    Assert.AreEqual<RtFloat>(FloatVal, 5);
  end;
end;

procedure TTest_ProxyValue.CreateFloatFromConvertible();
var
  Ratio: IRatio;
  FloatVal : RtFloat;
  Proxy : TProxyValue;
begin
  CreateRatio(Ratio, 1, 5);
  Proxy := Ratio;
  FloatVal := Proxy;

  Assert.AreEqual<RtFloat>(FloatVal, 0.2);
end;

procedure TTest_ProxyValue.CreateIntFromConvertible();
var
  Ratio: IRatio;
  IntVal : RtInt;
  Proxy : TProxyValue;
begin
  CreateRatio(Ratio, 6, 2);
  Proxy := Ratio;
  IntVal := Proxy;

  Assert.AreEqual<RtInt>(IntVal, 3);
end;

procedure TTest_ProxyValue.TestGetInterface();
var
  CoreTypeObj : ICoreType;
  CoreType : TCoreType;
  Proxy : TProxyValue<IInteger>;
begin
  Proxy := 5;
  CoreTypeObj := Proxy.AsInterface<ICoreType>();

  if Assigned(CoreTypeObj) then
  begin
    CoreTypeObj.GetCoreType(CoreType);
    Assert.AreEqual(CoreType, ctInt);
  end;
end;

procedure TTest_ProxyValue.ConvertToSmartPtr();
var
  Proxy : TProxyValue<IInteger>;
  IntVal: RtInt;
  IntIntf : IInteger;
  IntPtr2 : IObjectPtr<IInteger>;
begin
  Proxy := 5;
  IntIntf := Proxy;

  IntVal := Proxy;
  Assert.AreEqual<RtInt>(IntVal, 5);
  IntPtr2 := Proxy;
end;

procedure TTest_ProxyValue.ConvertToSmartPtr2();
var
  Proxy : TProxyValue<IInteger>;
  IntVal : RtInt;
  IntIntf : IInteger;
begin
  Proxy := nil;
  IntIntf := Proxy;

  Assert.WillRaise(procedure()
    begin
      IntVal := Proxy;
    end,
    ERTInvalidParameterException
  );
end;

procedure TTest_ProxyValue.Test2();
var
  IntObj : IInteger;
  Proxy : TProxyValue<IInteger>;
  ProxyIntObj : IInteger;
begin
  CreateInteger(IntObj, 5);

  Proxy := TProxyValue<IInteger>.Create(IntObj);
  ProxyIntObj := Proxy;

  Assert.AreEqual(IntObj, ProxyIntObj);
end;

procedure TTest_ProxyValue.Test3();
var
  IntObj : IInteger;
  Proxy : TProxyValue<IInteger>;
  ProxyIntObj : IInteger;
begin
  CreateInteger(IntObj, 5);

  Proxy := TProxyValue<IInteger>.Create(IntObj);
  ProxyIntObj := Proxy;

  Assert.AreEqual(IntObj, ProxyIntObj);
end;

procedure TTest_ProxyValue.ConvertFromIntValue();
var
  Proxy : TProxyValue<IInteger>;
  IntValue : RtInt;
begin
  Proxy := 5;
  IntValue := Proxy;
  Assert.AreEqual<RtInt>(IntValue, 5);
end;

procedure TTest_ProxyValue.ConvertFromString();
var
  Proxy : TProxyValue<IString>;
  StringValue : string;
begin
  Proxy := 'Test';
  StringValue := Proxy;
  Assert.AreEqual<string>(StringValue, 'Test');
end;

procedure TTest_ProxyValue.ConvertFromFloatValue();
var
  Proxy : TProxyValue<IFloat>;
  FloatValue : RtFloat;
begin
  Proxy := 5.5;
  FloatValue := Proxy;
  Assert.AreEqual<RtFloat>(FloatValue, 5.5);
end;

procedure TTest_ProxyValue.ConvertFromBoolValueTrue();
var
  Proxy : TProxyValue<IBoolean>;
  BoolValue : Boolean;
begin
  Proxy := True;
  BoolValue := Proxy;
  Assert.AreEqual<Boolean>(BoolValue, True);
end;

procedure TTest_ProxyValue.ConvertFromBoolValueFalse();
var
  Proxy : TProxyValue<IBoolean>;
  BoolValue : Boolean;
begin
  Proxy := False;
  BoolValue := Proxy;
  Assert.AreEqual<Boolean>(BoolValue, False);
end;

initialization
  TDSUnitTestEngine.RegisterUnitTest(TTest_ProxyValue);

end.