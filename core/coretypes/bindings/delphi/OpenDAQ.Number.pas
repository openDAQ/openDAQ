unit OpenDAQ.Number;

interface
uses
  OpenDAQ.CoreTypes,
  OpenDAQ.ObjectPtr;

type
  INumber = interface(IBaseObject)
  ['{52711B8D-DF25-59B0-AF86-1015C7B54603}']
    function GetFloatValue(out Value: DaqFloat): ErrCode stdcall;
    function GetIntValue(out Value: DaqInt): ErrCode stdcall;
  end;

  INumberPtr = interface(IObjectPtr<INumber>)
  ['{9DB056EC-57AA-4193-B071-660F2C44F84D}']
    function GetFloatValue(): DaqFloat;
    function GetIntValue(): DaqInt;
  end;

  TNumberPtr = class(TObjectPtr<INumber>, INumber, INumberPtr)
  public
    constructor Create(Obj : IBaseObject); overload; override;
    constructor Create(Obj : INumber); overload;

    function GetFloatValue(): DaqFloat;
    function GetIntValue(): DaqInt;
  private
    function INumber.GetFloatValue = Interface_GetFloatValue;
    function INumber.GetIntValue = Interface_GetIntValue;

    function Interface_GetFloatValue(out Value: DaqFloat): ErrCode; stdcall;
    function Interface_GetIntValue(out Value: DaqInt): ErrCode; stdcall;
  end;

implementation
uses
  OpenDAQ.Exceptions,
  OpenDAQ.SmartPtrRegistry;

{ TNumberPtr }

constructor TNumberPtr.Create(Obj: IBaseObject);
begin
  inherited Create(Obj);
end;

constructor TNumberPtr.Create(Obj: INumber);
begin
  inherited Create(Obj);
end;

function TNumberPtr.GetFloatValue: DaqFloat;
var
  Err: ErrCode;
  Value: DaqFloat;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.GetFloatValue(Value);
  CheckDaqErrorInfo(Err);

  Result := Value;
end;

function TNumberPtr.GetIntValue: DaqInt;
var
  Err: ErrCode;
  Value: DaqInt;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.GetIntValue(Value);
  CheckDaqErrorInfo(Err);

  Result := Value;
end;

function TNumberPtr.Interface_GetFloatValue(out Value: DaqFloat): ErrCode;
begin
  Result := FObject.GetFloatValue(Value);
end;

function TNumberPtr.Interface_GetIntValue(out Value: DaqInt): ErrCode;
begin
  Result := FObject.GetIntValue(Value);
end;

initialization
  TSmartPtrRegistry.RegisterPtr(INumber, INumberPtr, TNumberPtr);

finalization
  TSmartPtrRegistry.UnregisterPtr(INumber);

end.