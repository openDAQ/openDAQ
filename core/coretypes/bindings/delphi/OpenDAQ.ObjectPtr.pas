unit OpenDAQ.ObjectPtr;

interface
uses
  OpenDAQ.CoreTypes;

type
  TObjectPtr<T : IBaseObject> = class(TSmartPtr, IObjectPtr<T>, IBaseObject)
  public
    constructor Create(); overload; override;
    constructor Create(Obj : IBaseObject); overload; override;
    constructor Create(Obj : T); overload;

    function GetHashCodeEx(): SizeT;
    function EqualsObject(Other: IBaseObject): Boolean; overload;
    function EqualsObject(Other: ISmartPtr): Boolean; overload;

    function GetCoreType() : TCoreType;
    function GetInterface() : T;
    function GetObject() : IBaseObject; override;
    function IsAssigned() : Boolean; override;

    function ToString(): string; override;

    function QueryInterface(const IID: TGUID; out Obj): HResult; stdcall;

  protected
    FObject : T;
  private
    function IBaseObject.EqualsObject = Interface_EqualsObject;
    function IBaseObject.GetHashCodeEx = Interface_GetHashCodeEx;
    function IBaseObject.ToCharPtr = Interface_ToCharPtr;

    function BorrowInterface(const IID: TGUID; out Obj): HResult; stdcall;
    procedure Dispose(); stdcall;

    function Interface_GetHashCodeEx(out HashCode: SizeT): ErrCode; overload; stdcall;
    function Interface_EqualsObject(Other: IBaseObject; out Equal: Boolean): ErrCode; overload; stdcall;
    function Interface_ToCharPtr(Str: PPAnsiChar): ErrCode; overload; stdcall;

    function ConverToSmartPtr(const PtrId : TGUID; out Obj) : HResult;
  end;

  IObjectPtr = IObjectPtr<IBaseObject>;

implementation
uses
  System.TypInfo,
  System.SysUtils,
  OpenDAQ.CoreTypes.Errors,
  OpenDAQ.Exceptions,
  OpenDAQ.SmartPtrRegistry;

{ TObjectPtr<T> }

constructor TObjectPtr<T>.Create(Obj: IBaseObject);
var
  Err : HResult;
begin
  if Assigned(Obj) then
  begin
    Err := Obj.QueryInterface(GetTypeData(TypeInfo(T))^.Guid, FObject);
    if Err <> S_OK then
      raise ERTNoInterfaceException.Create('Object does not implement the requested interface');
  end;
end;

constructor TObjectPtr<T>.Create(Obj: T);
begin
  FObject := Obj;
end;

constructor TObjectPtr<T>.Create();
var
  Obj : IBaseObject;
  Err : ErrCode;
begin
  Err := CreateBaseObject(Obj);
  CheckRtErrorInfo(Err);

  FObject := T(Obj);
end;

function TObjectPtr<T>.EqualsObject(Other: IBaseObject): Boolean;
var
  Err : ErrCode;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is null.');

  Err := FObject.EqualsObject(Other, Result);
  CheckRtErrorInfo(Err);
end;

function TObjectPtr<T>.EqualsObject(Other: ISmartPtr): Boolean;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is null.');

  Result := EqualsObject(Other.GetObject());
end;

function TObjectPtr<T>.GetHashCodeEx(): SizeT;
var
  Err : ErrCode;
  HashCode : SizeT;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is null.');

  Err := FObject.GetHashCodeEx(HashCode);
  CheckRtErrorInfo(Err);

  Result := HashCode;
end;

function TObjectPtr<T>.GetCoreType(): TCoreType;
begin
  Result := OpenDAQ.CoreTypes.GetCoreType(FObject);
end;

function TObjectPtr<T>.GetInterface(): T;
begin
  Result := FObject;
end;

function TObjectPtr<T>.GetObject(): IBaseObject;
begin
  Result := FObject;
end;

function TObjectPtr<T>.IsAssigned(): Boolean;
begin
  Result := Assigned(FObject);
end;

function TObjectPtr<T>.ToString(): string;
var
  Ptr : PAnsiChar;
  Error : ErrCode;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is null.');

  Error := FObject.ToCharPtr(@Ptr);
  CheckRtErrorInfo(Error);

  Result := string(UTF8String(Ptr));
end;

// Interface forwarding

function TObjectPtr<T>.BorrowInterface(const IID: TGUID; out Obj): HResult;
begin
  Result := FObject.BorrowInterface(IID, Obj);
end;

procedure TObjectPtr<T>.Dispose();
begin
  FObject.Dispose();
end;

function TObjectPtr<T>.Interface_EqualsObject(Other: IBaseObject; out Equal: Boolean): ErrCode;
var
  OtherPtr : ISmartPtr;
begin
  if Supports(Other, ISmartPtr, OtherPtr) then
    Result := FObject.EqualsObject(OtherPtr.GetObject(), Equal)
  else
    Result := FObject.EqualsObject(Other, Equal);
end;

function TObjectPtr<T>.Interface_GetHashCodeEx(out HashCode: SizeT): ErrCode;
begin
  Result := FObject.GetHashCodeEx(HashCode);
end;

function TObjectPtr<T>.Interface_ToCharPtr(Str: PPAnsiChar): ErrCode;
begin
  Result := FObject.ToCharPtr(Str);
end;

function TObjectPtr<T>.ConverToSmartPtr(const PtrId : TGUID; out Obj) : HResult;
var
  IntfId : TGUID;
  PtrInstance : TSmartPtr;
  PtrClass : SmartPtrClass;
  NewInterface : IBaseObject;
begin
  IntfId := TSmartPtrRegistry.GetInterfaceFromPtr(PtrId);

  if not Supports(FObject, IntfId, NewInterface) then
    Exit(E_NOINTERFACE);

  PtrClass := TSmartPtrRegistry.GetPtrClass(PtrId);
  PtrInstance := PtrClass.Create(NewInterface);

  if PtrInstance.GetInterface(PtrId, Obj) then
    Result := S_OK
  else
    Result := E_NOINTERFACE;
end;

function TObjectPtr<T>.QueryInterface(const IID: TGUID; out Obj): HResult;
begin
  if Assigned(FObject) then
  begin
    Result := FObject.QueryInterface(IID, Obj);

    if (Result = E_NOINTERFACE) then
      Result := inherited QueryInterface(IID, Obj);

    if (Result = E_NOINTERFACE) and (TSmartPtrRegistry.IsPtrRegistered(IID)) then
      Result := ConverToSmartPtr(IID, Obj);
  end
  else
    Result := inherited QueryInterface(IID, Obj);
end;

initialization
  TSmartPtrRegistry.RegisterPtr(IBaseObject, IObjectPtr, TObjectPtr<IBaseObject>);

finalization
  TSmartPtrRegistry.UnregisterPtr(IBaseObject);

end.
