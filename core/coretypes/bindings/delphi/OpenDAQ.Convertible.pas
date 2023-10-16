unit OpenDAQ.Convertible;

interface
uses
  OpenDAQ.CoreTypes,
  OpenDAQ.ObjectPtr;
  
type
  IConvertiblePtr = interface(IObjectPtr<IConvertible>)
  ['{5B0F0EDC-24E0-4E6B-B252-68DC9BA77A26}']
    function ToFloat(): RtFloat;
    function ToInt(): RtInt;
    function ToBool(): Boolean;
  end;

  TConvertiblePtr = class(TObjectPtr<IConvertible>, IConvertiblePtr, IConvertible)
  public
    constructor Create(Obj : IBaseObject); overload; override;
    constructor Create(Obj : IConvertible); overload;

    function ToFloat(): RtFloat;
    function ToInt(): RtInt;
    function ToBool(): Boolean;

  private
    function IConvertible.ToFloat = Interface_ToFloat;
    function IConvertible.ToInt = Interface_ToInt;
    function IConvertible.ToBool = Interface_ToBool;

    function Interface_ToFloat(out Val: RtFloat): ErrCode stdcall;
    function Interface_ToInt(out Val: RtInt): ErrCode stdcall;
    function Interface_ToBool(out Val: Boolean): ErrCode stdcall;
  end;

implementation
uses
  OpenDAQ.Exceptions,
  OpenDAQ.SmartPtrRegistry;

{ TConvertiblePtr }

constructor TConvertiblePtr.Create(Obj: IBaseObject);
begin
  inherited Create(Obj);
end;

constructor TConvertiblePtr.Create(Obj: IConvertible);
begin
  inherited Create(Obj);
end;

function TConvertiblePtr.ToFloat() : RtFloat;
var
  Err : ErrCode;
  FloatVal : RtFloat;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.ToFloat(FloatVal);
  CheckRtErrorInfo(Err);
  
  Result := FloatVal;
end;

function TConvertiblePtr.ToInt(): RtInt;
var
  Err : ErrCode;
  IntVal : RtInt;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.ToInt(IntVal);
  CheckRtErrorInfo(Err);

  Result := IntVal;
end;

function TConvertiblePtr.ToBool(): Boolean;
var
  Err : ErrCode;
  BoolVal : Boolean;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.ToBool(BoolVal);
  CheckRtErrorInfo(Err);

  Result := BoolVal;
end;

function TConvertiblePtr.Interface_ToFloat(out Val: RtFloat): ErrCode;
begin
  Result := FObject.ToFloat(Val);
end;

function TConvertiblePtr.Interface_ToInt(out Val: RtInt): ErrCode;
begin
  Result := FObject.ToInt(Val);
end;

function TConvertiblePtr.Interface_ToBool(out Val: Boolean): ErrCode;
begin
  Result := FObject.ToBool(Val);
end;

initialization
  TSmartPtrRegistry.RegisterPtr(IConvertible, IConvertiblePtr, TConvertiblePtr);

finalization
  TSmartPtrRegistry.UnregisterPtr(IConvertible);

end.