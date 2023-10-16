unit OpenDAQ.Integer;

interface
uses
  OpenDAQ.CoreTypes,
  OpenDAQ.ObjectPtr;

type
  IIntegerPtr = interface(IObjectPtr<IInteger>)
  ['{44365893-D448-4A7F-B744-1BD687F26608}']
    function GetValue(): RtInt;
    function EqualsValue(Value: RtInt): Boolean;
  end;

  TIntegerPtr = class(TObjectPtr<IInteger>, IInteger, IIntegerPtr)
  public
    constructor Create(Obj : IBaseObject); overload; override;
    constructor Create(Obj : IInteger); overload;
    constructor Create(Value : RtInt); overload;

    function GetValue(): RtInt;
    function EqualsValue(Value: RtInt): Boolean;
  private
    function IInteger.GetValue = Interface_GetValue;
    function IInteger.EqualsValue = Interface_EqualsValue;

    function Interface_GetValue(out Value: RtInt): ErrCode; stdcall;
    function Interface_EqualsValue(const Value: RtInt; out Equal: Boolean): ErrCode stdcall;
  end;

implementation
uses
  OpenDAQ.Exceptions,
  OpenDAQ.SmartPtrRegistry;

{ TIntegerPtr }

constructor TIntegerPtr.Create(Obj: IBaseObject);
begin
  inherited Create(Obj);
end;

constructor TIntegerPtr.Create(Obj: IInteger);
begin
  inherited Create(Obj);
end;

constructor TIntegerPtr.Create(Value: RtInt);
var
  IntegerObj : IInteger;
  Err : ErrCode;
begin
  Err := CreateInteger(IntegerObj, Value);
  CheckRtErrorInfo(Err);

  inherited Create(IntegerObj);
end;

function TIntegerPtr.GetValue: RtInt;
var
  Err: ErrCode;
  Value: RtInt;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.GetValue(Value);
  CheckRtErrorInfo(Err);

  Result := Value;
end;

function TIntegerPtr.EqualsValue(Value: RtInt): Boolean;
var
  Err: ErrCode;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.EqualsValue(Value, Result);
  CheckRtErrorInfo(Err);
end;

function TIntegerPtr.Interface_GetValue(out Value: RtInt): ErrCode;
begin
  Result := FObject.GetValue(Value);
end;

function TIntegerPtr.Interface_EqualsValue(const Value: RtInt; out Equal: Boolean): ErrCode;
begin
  Result := FObject.EqualsValue(Value, Equal);
end;

initialization
  TSmartPtrRegistry.RegisterPtr(IInteger, IIntegerPtr, TIntegerPtr);

finalization
  TSmartPtrRegistry.UnregisterPtr(IInteger);

end.