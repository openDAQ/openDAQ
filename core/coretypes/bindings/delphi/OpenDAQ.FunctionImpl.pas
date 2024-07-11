unit OpenDAQ.FunctionImpl;

interface

uses
  OpenDAQ.CoreTypes,
  OpenDAQ.BaseObjectImpl;

type

  TCallEvent = function (Params: IBaseObject; out Res: IBaseObject): ErrCode of object;

  TFunctionImpl = class(TBaseObjectImpl, IFunction, ICoreType)
  public
    constructor Create(CallEvent: TCallEvent);
    function Call(Params: IBaseObject; out Res: IBaseObject): ErrCode; stdcall;
    function GetCoreType(out CoreType: TCoreType): ErrCode; stdcall;
    function ToCharPtr(Str: PPAnsiChar): ErrCode; stdcall;
  protected
    FCallEvent : TCallEvent;
  end;

implementation

uses
  OpenDAQ.CoreTypes.Errors, OpenDAQ.Exceptions, System.SysUtils;

{ TProcedure }

constructor TFunctionImpl.Create(CallEvent: TCallEvent);
begin
  FCallEvent := CallEvent;
end;

function TFunctionImpl.Call(Params: IBaseObject; out Res: IBaseObject): ErrCode;
begin
  try
    Result := FCallEvent(Params, Res);
  except
    on E: ERTException do
      Exit(E.Code);
    on E: Exception do
      Exit(OPENDAQ_ERR_GENERALERROR);
  end;
end;

function TFunctionImpl.GetCoreType(out CoreType: TCoreType): ErrCode;
begin
  CoreType := ctFunc;
  Result := OPENDAQ_SUCCESS;
end;

function TFunctionImpl.ToCharPtr(Str: PPAnsiChar): ErrCode;
const
  DStr: UTF8String = 'Function'#0;
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
