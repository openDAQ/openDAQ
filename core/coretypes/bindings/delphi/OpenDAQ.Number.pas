unit OpenDAQ.Number;

interface
uses
  OpenDAQ.CoreTypes,
  OpenDAQ.ObjectPtr;

type
  INumber = interface(IBaseObject)
  ['{52711B8D-DF25-59B0-AF86-1015C7B54603}']
    function GetFloatValue(out Value: RtFloat): ErrCode stdcall;
    function GetIntValue(out Value: RtInt): ErrCode stdcall;
  end;

  INumberPtr = interface(IObjectPtr<INumber>)
  ['{9DB056EC-57AA-4193-B071-660F2C44F84D}']
    function GetFloatValue(): RtFloat;
    function GetIntValue(): RtInt;
  end;

  TNumberPtr = class(TObjectPtr<INumber>, INumber, INumberPtr)
  public
    constructor Create(Obj : IBaseObject); overload; override;
    constructor Create(Obj : INumber); overload;

    function GetFloatValue(): RtFloat;
    function GetIntValue(): RtInt;
  private
    function INumber.GetFloatValue = Interface_GetFloatValue;
    function INumber.GetIntValue = Interface_GetIntValue;

    function Interface_GetFloatValue(out Value: RtFloat): ErrCode; stdcall;
    function Interface_GetIntValue(out Value: RtInt): ErrCode; stdcall;
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

function TNumberPtr.GetFloatValue: RtFloat;
var
  Err: ErrCode;
  Value: RtFloat;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.GetFloatValue(Value);
  CheckRtErrorInfo(Err);

  Result := Value;
end;

function TNumberPtr.GetIntValue: RtInt;
var
  Err: ErrCode;
  Value: RtInt;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.GetIntValue(Value);
  CheckRtErrorInfo(Err);

  Result := Value;
end;

function TNumberPtr.Interface_GetFloatValue(out Value: RtFloat): ErrCode;
begin
  Result := FObject.GetFloatValue(Value);
end;

function TNumberPtr.Interface_GetIntValue(out Value: RtInt): ErrCode;
begin
  Result := FObject.GetIntValue(Value);
end;

initialization
  TSmartPtrRegistry.RegisterPtr(INumber, INumberPtr, TNumberPtr);

finalization
  TSmartPtrRegistry.UnregisterPtr(INumber);

end.