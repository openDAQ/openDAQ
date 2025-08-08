unit OpenDAQ.Integer;

interface
uses
  OpenDAQ.CoreTypes,
  OpenDAQ.ObjectPtr;

type
  IIntegerPtr = interface(IObjectPtr<IInteger>)
  ['{44365893-D448-4A7F-B744-1BD687F26608}']
    function GetValue(): DaqInt;
    function EqualsValue(Value: DaqInt): Boolean;
  end;

  TIntegerPtr = class(TObjectPtr<IInteger>, IInteger, IIntegerPtr)
  public
    constructor Create(Obj : IBaseObject); overload; override;
    constructor Create(Obj : IInteger); overload;
    constructor Create(Value : DaqInt); overload;

    function GetValue(): DaqInt;
    function EqualsValue(Value: DaqInt): Boolean;
  private
    function IInteger.GetValue = Interface_GetValue;
    function IInteger.EqualsValue = Interface_EqualsValue;

    function Interface_GetValue(out Value: DaqInt): ErrCode; stdcall;
    function Interface_EqualsValue(const Value: DaqInt; out Equal: Boolean): ErrCode stdcall;
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

constructor TIntegerPtr.Create(Value: DaqInt);
var
  IntegerObj : IInteger;
  Err : ErrCode;
begin
  Err := CreateInteger(IntegerObj, Value);
  CheckDaqErrorInfo(Err);

  inherited Create(IntegerObj);
end;

function TIntegerPtr.GetValue: DaqInt;
var
  Err: ErrCode;
  Value: DaqInt;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.GetValue(Value);
  CheckDaqErrorInfo(Err);

  Result := Value;
end;

function TIntegerPtr.EqualsValue(Value: DaqInt): Boolean;
var
  Err: ErrCode;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.EqualsValue(Value, Result);
  CheckDaqErrorInfo(Err);
end;

function TIntegerPtr.Interface_GetValue(out Value: DaqInt): ErrCode;
begin
  Result := FObject.GetValue(Value);
end;

function TIntegerPtr.Interface_EqualsValue(const Value: DaqInt; out Equal: Boolean): ErrCode;
begin
  Result := FObject.EqualsValue(Value, Equal);
end;

initialization
  TSmartPtrRegistry.RegisterPtr(IInteger, IIntegerPtr, TIntegerPtr);

finalization
  TSmartPtrRegistry.UnregisterPtr(IInteger);

end.