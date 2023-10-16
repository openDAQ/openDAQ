unit Test.OpenDAQ.ProcedureImpl;

interface
uses
  DunitX.TestFramework, DS.UT.DSUnitTestUnit, OpenDAQ.CoreTypes, OpenDAQ.ProcedureImpl;

type
  [TestFixture]
  TTest_ProcedureImpl = class(TDSUnitTest)
  private
    FObjectCount: Nativeuint;
    FMemoryLeakExpected: Boolean;
    FNumOfCalls: Integer;
    function Callback(Params: IBaseObject): ErrCode;
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

function TTest_ProcedureImpl.Callback(Params: IBaseObject): ErrCode;
begin
  FNumOfCalls := FNumOfCalls + 1;
  Result := OPENDAQ_SUCCESS;
end;

procedure TTest_ProcedureImpl.Create;
var
  Proc: IProcedure;
begin
  Assert.WillNotRaiseAny(procedure()
    begin
      Proc := TProcedureImpl.Create(Callback);
    end
  );
end;

procedure TTest_ProcedureImpl.Execute;
var
  Proc: IProcedure;
begin
  Proc := TProcedureImpl.Create(Callback);
  Assert.AreEqual(FNumOfCalls, 0);
  Proc.Execute(nil);
  Proc.Execute(nil);
  Assert.AreEqual(FNumOfCalls, 2);
end;

procedure TTest_ProcedureImpl.GetCoreType;
var
  Proc: IProcedure;
  CoreType: TCoreType;
begin
  Proc := TProcedureImpl.Create(Callback);
  (Proc as ICoreType).GetCoreType(CoreType);
  Assert.AreEqual(CoreType, TCoreType.ctProc);
end;

procedure TTest_ProcedureImpl.ToCharPtr;
var
  Proc: IProcedure;
  Str: PAnsiChar;
begin
  Proc := TProcedureImpl.Create(Callback);
  Proc.ToCharPtr(@Str);
  Assert.AreEqual(string(Str), 'Procedure');
  DaqFreeMemory(Str);
end;

procedure TTest_ProcedureImpl.ToCharPtrNilPtr;
var
  Proc: IProcedure;
begin
  Proc := TProcedureImpl.Create(Callback);
  Assert.AreEqual(Proc.ToCharPtr(nil), OPENDAQ_ERR_INVALIDPARAMETER);
end;

procedure TTest_ProcedureImpl.Setup;
begin
  FObjectCount := DaqGetTrackedObjectCount;
  FNumOfCalls := 0;
end;

procedure TTest_ProcedureImpl.TearDown;
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
  TDSUnitTestEngine.RegisterUnitTest(TTest_ProcedureImpl);

end.
