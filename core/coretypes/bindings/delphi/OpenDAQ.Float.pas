unit OpenDAQ.Float;

interface
uses
  OpenDAQ.CoreTypes,
  OpenDAQ.ObjectPtr;

type
  IFloatPtr = interface(IObjectPtr<IFloat>)
  ['{35DD3B60-7C68-44B8-B8E9-0017BF587623}']
    function GetValue(): DaqFloat;
    function EqualsValue(Value: DaqFloat): Boolean;
  end;

  TFloatPtr = class(TObjectPtr<IFloat>, IFloat, IFloatPtr)
  public
    constructor Create(Obj : IBaseObject); overload; override;
    constructor Create(Obj : IFloat); overload;
    constructor Create(Value: DaqFloat); overload;

    function GetValue(): DaqFloat;
    function EqualsValue(Value: DaqFloat): Boolean;
  private
    function IFloat.GetValue = Interface_GetValue;
    function IFloat.EqualsValue = Interface_EqualsValue;

    function Interface_GetValue(out Value: DaqFloat): ErrCode; stdcall;
    function Interface_EqualsValue(const Value: DaqFloat; out Equal: Boolean): ErrCode stdcall;
  end;

implementation
uses
  OpenDAQ.Exceptions,
  OpenDAQ.SmartPtrRegistry;

{ TFloatPtr }

constructor TFloatPtr.Create(Obj: IBaseObject);
begin
  inherited Create(Obj);
end;

constructor TFloatPtr.Create(Obj: IFloat);
begin
  inherited Create(Obj);
end;

constructor TFloatPtr.Create(Value: DaqFloat);
var
  FloatObj : IFloat;
  Err : ErrCode;
begin
  Err := CreateFloat(FloatObj, Value);
  CheckDaqErrorInfo(Err);

  inherited Create(FloatObj);
end;

function TFloatPtr.GetValue: DaqFloat;
var
  Err: ErrCode;
  Value: DaqFloat;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.GetValue(Value);
  CheckDaqErrorInfo(Err);

  Result := Value;
end;

function TFloatPtr.EqualsValue(Value: DaqFloat): Boolean;
var
  Err: ErrCode;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.EqualsValue(Value, Result);
  CheckDaqErrorInfo(Err);
end;

function TFloatPtr.Interface_GetValue(out Value: DaqFloat): ErrCode;
begin
  Result := FObject.GetValue(Value);
end;

function TFloatPtr.Interface_EqualsValue(const Value: DaqFloat; out Equal: Boolean): ErrCode;
begin
  Result := FObject.EqualsValue(Value, Equal);
end;

initialization
try
  TSmartPtrRegistry.RegisterPtr(IFloat, IFloatPtr, TFloatPtr);
except

end;

finalization
  TSmartPtrRegistry.UnregisterPtr(IFloat);

end.