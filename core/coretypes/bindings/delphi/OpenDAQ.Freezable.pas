unit OpenDAQ.Freezable;

interface
uses
  OpenDAQ.CoreTypes,
  OpenDAQ.ObjectPtr;
  
type

  TFreezablePtr = class(TObjectPtr<IFreezable>, IFreezablePtr, IFreezable)
  public
    constructor Create(Obj : IBaseObject); overload; override;
    constructor Create(Obj : IFreezable); overload;

    procedure Freeze();
    function IsFrozen(): Boolean;

  private
    function IFreezable.Freeze = Interface_Freeze;
    function IFreezable.IsFrozen = Interface_IsFrozen;

    function Interface_Freeze(): ErrCode stdcall;
    function Interface_IsFrozen(out Frozen: Boolean): ErrCode stdcall;
  end;

implementation
uses
  OpenDAQ.Exceptions,
  OpenDAQ.SmartPtrRegistry;

{ TFreezablePtr }

constructor TFreezablePtr.Create(Obj: IBaseObject);
begin
  inherited Create(Obj);
end;

constructor TFreezablePtr.Create(Obj: IFreezable);
begin
  inherited Create(Obj);
end;

procedure TFreezablePtr.Freeze();
var
  Err : ErrCode;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.Freeze();
  CheckRtErrorInfo(Err);
end;

function TFreezablePtr.IsFrozen(): Boolean;
var
  Err : ErrCode;
  Frozen : Boolean;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.IsFrozen(Frozen);
  CheckRtErrorInfo(Err);

  Result := Frozen;
end;

function TFreezablePtr.Interface_Freeze(): ErrCode;
begin
  Result := FObject.Freeze();
end;

function TFreezablePtr.Interface_IsFrozen(out Frozen: Boolean): ErrCode;
begin
  Result := FObject.IsFrozen(Frozen);
end;

initialization
  TSmartPtrRegistry.RegisterPtr(IFreezable, IFreezablePtr, TFreezablePtr);

finalization
  TSmartPtrRegistry.UnregisterPtr(IFreezable);

end.