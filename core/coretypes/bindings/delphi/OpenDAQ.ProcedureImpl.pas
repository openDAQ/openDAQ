unit OpenDAQ.ProcedureImpl;

interface

uses
  OpenDAQ.CoreTypes, OpenDAQ.BaseObjectImpl;

type

  TExecuteEvent = function (Params : IBaseObject): ErrCode of object;

  TProcedureImpl = class(TBaseObjectImpl, IProcedure, ICoreType)
  public
    constructor Create(CallEvent: TExecuteEvent);
    function Execute(Params : IBaseObject) : ErrCode; stdcall;
    function GetCoreType(out CoreType: TCoreType): ErrCode; stdcall;
    function ToCharPtr(Str: PPAnsiChar): ErrCode; stdcall;
  protected
    FCallEvent : TExecuteEvent;
  end;

implementation

uses
  OpenDAQ.CoreTypes.Errors, OpenDAQ.Exceptions, System.SysUtils;

{ TProcedure }

constructor TProcedureImpl.Create(CallEvent: TExecuteEvent);
begin
  FCallEvent := CallEvent;
end;

function TProcedureImpl.Execute(Params : IBaseObject): ErrCode;
begin
  try
    Result := FCallEvent(Params);
  except
    on E: ERTException do
      Exit(E.Code);
    on E: Exception do
      Exit(OPENDAQ_ERR_GENERALERROR);
  end;
end;

function TProcedureImpl.GetCoreType(out CoreType: TCoreType): ErrCode;
begin
  CoreType := ctProc;
  Result := OPENDAQ_SUCCESS;
end;

function TProcedureImpl.ToCharPtr(Str: PPAnsiChar): ErrCode;
const
  DStr: UTF8String = 'Procedure'#0;
var
  RStr: PAnsiChar;
begin
  if not Assigned(Str) then
     Exit(OPENDAQ_ERR_INVALIDPARAMETER);

  RStr := daqAllocateMemory(Length(DStr) + 1);
  Move(DStr[1], RStr^, Length(DStr));
  Str^ := RStr;

  Result := OPENDAQ_SUCCESS;
end;

end.
