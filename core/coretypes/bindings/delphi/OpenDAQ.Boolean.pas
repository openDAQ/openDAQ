unit OpenDAQ.Boolean;

interface
uses
  OpenDAQ.CoreTypes,
  OpenDAQ.ObjectPtr;

type
  IBooleanPtr = interface(IObjectPtr<IBoolean>)
  ['{C52134DF-9A7F-40B6-A07A-E3771A3CC75D}']
    function GetValue(): Boolean;
    function EqualsValue(Value: Boolean): Boolean;
  end;

  TBooleanPtr = class(TObjectPtr<IBoolean>, IBoolean, IBooleanPtr)
  public
    constructor Create(Obj : IBaseObject); overload; override;
    constructor Create(Obj : IBoolean); overload;
    constructor Create(Value: Boolean); overload;

    function GetValue(): Boolean;
    function EqualsValue(Value: Boolean): Boolean;
  private
    function IBoolean.GetValue = Interface_GetValue;
    function IBoolean.EqualsValue = Interface_EqualsValue;

    function Interface_GetValue(out Value: Boolean): ErrCode; stdcall;
    function Interface_EqualsValue(const Value: Boolean; out Equal: Boolean): ErrCode stdcall;
  end;

implementation
uses
  OpenDAQ.Exceptions,
  OpenDAQ.SmartPtrRegistry;

{ TBooleanPtr }

constructor TBooleanPtr.Create(Obj: IBaseObject);
begin
  inherited Create(Obj);
end;

constructor TBooleanPtr.Create(Obj: IBoolean);
begin
  inherited Create(Obj);
end;

constructor TBooleanPtr.Create(Value: Boolean);
var
  BoolObj : IBoolean;
  Err : ErrCode;
begin
  Err := CreateBoolean(BoolObj, Value);
  CheckRtErrorInfo(Err);

  inherited Create(BoolObj);
end;

function TBooleanPtr.GetValue: Boolean;
var
  Err: ErrCode;
  Value: Boolean;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.GetValue(Value);
  CheckRtErrorInfo(Err);

  Result := Value;
end;

function TBooleanPtr.EqualsValue(Value: Boolean): Boolean;
var
  Err: ErrCode;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.EqualsValue(Value, Result);
  CheckRtErrorInfo(Err);
end;

function TBooleanPtr.Interface_GetValue(out Value: Boolean): ErrCode;
begin
  Result := FObject.GetValue(Value);
end;

function TBooleanPtr.Interface_EqualsValue(const Value: Boolean; out Equal: Boolean): ErrCode;
begin
  Result := FObject.EqualsValue(Value, Equal);
end;

initialization
  TSmartPtrRegistry.RegisterPtr(IBoolean, IBooleanPtr, TBooleanPtr);

finalization
  TSmartPtrRegistry.UnregisterPtr(IBoolean);

end.