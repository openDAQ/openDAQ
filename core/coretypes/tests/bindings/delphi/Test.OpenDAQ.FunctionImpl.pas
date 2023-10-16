unit Test.OpenDAQ.FunctionImpl;

interface
uses
  DunitX.TestFramework,
  DS.UT.DSUnitTestUnit,
  OpenDAQ.FunctionImpl,
  OpenDAQ.CoreTypes;

type
  [TestFixture]
  TTest_FunctionImpl = class(TDSUnitTest)
  private
    FObjectCount: Nativeuint;
    FMemoryLeakExpected: Boolean;
    FNumOfCalls: Integer;
    function Callback(Params: IBaseObject; out Res: IBaseObject): ErrCode;
  public
    [Setup]
    procedure Setup; override;
    [TearDown]
    procedure TearDown; override;
    [Test]
    procedure Create;
    [Test]
    procedure Execute;
    [Test]
    procedure GetCoreType;
    [Test]
    procedure ToCharPtr;
    [Test]
    procedure ToCharPtrNilPtr;
  end;

implementation

uses
  DS.UT.DSUnitTestEngineUnit, WinApi.Windows, OpenDAQ.CoreTypes.Errors, SysUtils;

{ TTest_ProcedureImpl }

function TTest_FunctionImpl.Callback(Params: IBaseObject; out Res: IBaseObject): ErrCode;
begin
  FNumOfCalls := FNumOfCalls + 1;
  Result := OPENDAQ_SUCCESS;
end;

procedure TTest_FunctionImpl.Create;
var
  Func: IFunction;
begin
  Assert.WillNotRaiseAny(procedure()
    begin
      Func := TFunctionImpl.Create(Callback);
    end
  );
end;

procedure TTest_FunctionImpl.Execute;
var
  Func: IFunction;
  Res: IBaseObject;
begin
  Func := TFunctionImpl.Create(Callback);
  Assert.AreEqual(FNumOfCalls, 0);
  Func.Call(nil, Res);
  Func.Call(nil, Res);
  Assert.AreEqual(FNumOfCalls, 2);
end;

procedure TTest_FunctionImpl.GetCoreType;
var
  Func: IFunction;
  CoreType: TCoreType;
begin
  Func := TFunctionImpl.Create(Callback);
  (Func as ICoreType).GetCoreType(CoreType);
  Assert.AreEqual(CoreType, TCoreType.ctFunc);
end;

procedure TTest_FunctionImpl.ToCharPtr;
var
  Func: IFunction;
  Str: PAnsiChar;
begin
  Func := TFunctionImpl.Create(Callback);
  Func.ToCharPtr(@Str);
  Assert.AreEqual(string(Str), 'Function');
  DaqFreeMemory(Str);
end;

procedure TTest_FunctionImpl.ToCharPtrNilPtr;
var
  Func: IFunction;
begin
  Func := TFunctionImpl.Create(Callback);
  Assert.AreEqual(Func.ToCharPtr(nil), OPENDAQ_ERR_INVALIDPARAMETER);
end;

procedure TTest_FunctionImpl.Setup;
begin
  FObjectCount := DaqGetTrackedObjectCount;
  FNumOfCalls := 0;
end;

procedure TTest_FunctionImpl.TearDown;
var
  NewObjectCount, Diff: Nativeuint;
  Str: string;
begin
  if not FMemoryLeakExpected then
  begin
    NewObjectCount := DaqGetTrackedObjectCount;
    Diff := NewObjectCount - FObjectCount;
    Str := Format('%d object(s) still alive', [Diff]);
    if (NewObjectCount <> FObjectCount) then
    begin
      DaqPrintTrackedObjects;
      Assert.Fail(Str);
    end;
  end;
end;

initialization
  TDSUnitTestEngine.RegisterUnitTest(TTest_FunctionImpl);

end.
