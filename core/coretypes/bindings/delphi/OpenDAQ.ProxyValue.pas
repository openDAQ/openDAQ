unit OpenDAQ.ProxyValue;

interface
uses 
  OpenDAQ.CoreTypes,
  OpenDAQ.ObjectPtr;

type
  {$MINENUMSIZE 4}

  TProxyValue<T : IBaseObject> = record
  public
    constructor Create(Obj : T); overload;

    class operator Implicit(Proxy: TProxyValue<T>): RtInt;
    class operator Implicit(Proxy: TProxyValue<T>): RtFloat;
    class operator Implicit(Proxy: TProxyValue<T>): Boolean;
    class operator Implicit(Proxy: TProxyValue<T>): string;

    class operator Implicit(Proxy: TProxyValue<T>): T;
    class operator Implicit(Proxy: TProxyValue<T>): IObjectPtr<T>;

    class operator Implicit(Value : RtInt): TProxyValue<T>;
    class operator Implicit(Value : RtFloat): TProxyValue<T>;
    class operator Implicit(Value : Boolean): TProxyValue<T>;
    class operator Implicit(Value : string): TProxyValue<T>;

    class operator Implicit(Ptr : Pointer) : TProxyValue<T>;
    class operator Implicit(Ptr : IObjectPtr<T>) : TProxyValue<T>;
    class operator Implicit(Value : T): TProxyValue<T>;

    function AsInterface<U : IBaseObject>() : U;
    function AsInterfaceOrNil<U : IBaseObject>() : U;

    function AsPtr<U : ISmartPtr>() : U;
    function AsPtrOrNil<U : ISmartPtr>() : U;

    function IsAssigned(): Boolean;
  private
    FObject : T;
  end;

  TProxyValue = TProxyValue<IBaseObject>;

implementation
uses  
  System.TypInfo,
  System.SysUtils,
  OpenDAQ.Exceptions,
  OpenDAQ.SmartPtrRegistry;

{ ProxyValue }

constructor TProxyValue<T>.Create(Obj : T);
begin
  FObject := Obj;
end;

function TProxyValue<T>.AsInterface<U>(): U;
var
  Intf : U;
begin
  if not Assigned(FObject) then
    Exit(nil);

  if Supports(FObject, GetTypeData(TypeInfo(U))^.GUID, Intf) then
    Result := Intf
  else
    raise ERTNoInterfaceException.Create('Internal object does not implement this interface.');
end;

function TProxyValue<T>.AsInterfaceOrNil<U>(): U;
var
  Intf : U;
begin
  if not Assigned(FObject) then
    Exit(nil);

  if Supports(FObject, GetTypeData(TypeInfo(U))^.GUID, Intf) then
    Result := Intf
  else
    Result := nil;
end;

function TProxyValue<T>.AsPtr<U>(): U;
var
  Ptr : TSmartPtr;
  PtrClass : SmartPtrClass;
  InterfaceGuid : TGUID;
  PtrInterface : U;
begin
  if not Assigned(FObject) then
    Exit(nil);

  InterfaceGuid := GetTypeData(TypeInfo(U))^.Guid;
  PtrClass := TSmartPtrRegistry.GetPtrClass(InterfaceGuid);

  if not Assigned(PtrClass) then
    raise ERTException.Create('SmartPtr class for this interface is not registered.');

  Ptr := PtrClass.Create(FObject);

  if Supports(Ptr, InterfaceGuid, PtrInterface) then
    Result := PtrInterface
  else
    raise ERTException.Create('The registered SmartPtr class does not implement the specified interface.');
end;

function TProxyValue<T>.AsPtrOrNil<U>(): U;
var
  Ptr : TSmartPtr;
  PtrClass : SmartPtrClass;
  PtrGuid : TGUID;
  InterfaceGuid: TGUID;
  PtrInterface : U;
  InterfaceObj: IBaseObject;
begin
  if not Assigned(FObject) then
    Exit(nil);

  PtrGuid := GetTypeData(TypeInfo(U))^.Guid;
  PtrClass := TSmartPtrRegistry.GetPtrClass(PtrGuid);

  if not Assigned(PtrClass) then
    raise ERTException.Create('SmartPtr class for this interface is not registered.');

  InterfaceGuid := TSmartPtrRegistry.GetInterfaceFromPtr(PtrGuid);
  if not Supports(FObject, InterfaceGuid, InterfaceObj) then
    Exit(nil);

  Ptr := PtrClass.Create(InterfaceObj);
  if Supports(Ptr, PtrGuid, PtrInterface) then
    Result := PtrInterface
  else
    Result := nil;
end;

function TProxyValue<T>.IsAssigned: Boolean;
begin
  Result := Assigned(FObject);
end;

class operator TProxyValue<T>.Implicit(Proxy: TProxyValue<T>): RtInt;
var
  Err : ErrCode;
  Value : RtInt;
  Convertible: IConvertible;
begin
  if not Assigned(Proxy.FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  if (TypeInfo(T) = TypeInfo(IInteger)) then
    Err := IInteger(Proxy.FObject).GetValue(Value)
  else if (Supports(Proxy.FObject, IConvertible, Convertible)) then
    Err := Convertible.ToInt(Value)
  else
    raise ERTInvalidParameterException.Create('Could not convert ' + GetTypeName(TypeInfo(T)) +' to RtInt.');

  CheckRtErrorInfo(Err);
  Result := Value;
end;

class operator TProxyValue<T>.Implicit(Proxy: TProxyValue<T>): RtFloat;
var
  Err : ErrCode;
  Value : RtFloat;
  Convertible: IConvertible;
begin
  if not Assigned(Proxy.FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  if (TypeInfo(T) = TypeInfo(IFloat)) then
    Err := IFloat(Proxy.FObject).GetValue(Value)
  else if (Supports(Proxy.FObject, IConvertible, Convertible)) then
    Err := Convertible.ToFloat(Value)
  else
    raise ERTInvalidParameterException.Create('Could not convert ' + GetTypeName(TypeInfo(T)) +' to RtInt.');

  CheckRtErrorInfo(Err);
  Result := Value;
end;

class operator TProxyValue<T>.Implicit(Proxy: TProxyValue<T>): Boolean;
var
  Err : ErrCode;
  Value : Boolean;
  Convertible: IConvertible;
begin
  if not Assigned(Proxy.FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  if (TypeInfo(T) = TypeInfo(IBoolean)) then
    Err := IBoolean(Proxy.FObject).GetValue(Value)
  else if (Supports(Proxy.FObject, IConvertible, Convertible)) then
    Err := Convertible.ToBool(Value)
  else
    raise ERTInvalidParameterException.Create('Could not convert ' + GetTypeName(TypeInfo(T)) +' to RtInt.');

  CheckRtErrorInfo(Err);
  Result := Value;
end;

class operator TProxyValue<T>.Implicit(Proxy: TProxyValue<T>): string;
var
  Err : ErrCode;
  RtStr: IString;
begin
  if not Assigned(Proxy.FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  if (TypeInfo(T) = TypeInfo(IString)) then
    Result := RtToString(IString(Proxy.FObject))
  else
  begin
    if (Supports(Proxy.FObject, IString, RtStr)) then
      Result := RtToString(RtStr)
    else
      Result := BaseObjectToString(Proxy.FObject);
  end;
end;

class operator TProxyValue<T>.Implicit(Proxy: TProxyValue<T>): T;
begin
  Result := T(Proxy.FObject);
end;

class operator TProxyValue<T>.Implicit(Proxy: TProxyValue<T>): IObjectPtr<T>;
var
  PtrClass : SmartPtrClass;
  Ptr : ISmartPtr;
  Intf: T;
  IntfGuid: TGUID;
begin
  IntfGuid := GetTypeData(TypeInfo(T))^.Guid;
  PtrClass := TSmartPtrRegistry.GetPtrClass(IntfGuid);

  if not Supports(Proxy.FObject, IntfGuid, Intf) then
    raise ERTNoInterfaceException.Create('Could not implicitly convert to IObjectPtr<T>.');

  Result := PtrClass.Create(Intf) as IObjectPtr<T>;
end;

class operator TProxyValue<T>.Implicit(Ptr: IObjectPtr<T>): TProxyValue<T>;
begin
  Result := TProxyValue<T>.Create(Ptr.GetInterface());
end;

class operator TProxyValue<T>.Implicit(Value: T): TProxyValue<T>;
begin
  Result := TProxyValue<T>.Create(Value);
end;

class operator TProxyValue<T>.Implicit(Ptr: Pointer): TProxyValue<T>;
begin
  if Assigned(Ptr) then
    raise ERTInvalidParameterException.Create('Parameter must be nil');

  Result := TProxyValue<T>.Create(nil);
end;

class operator TProxyValue<T>.Implicit(Value: RtFloat): TProxyValue<T>;
var
  FloatObj : IFloat;
  Err : ErrCode;
begin
  if (TypeInfo(T) <> TypeInfo(IFloat)) and (TypeInfo(T) <> TypeInfo(IBaseObject)) then
    raise ERTInvalidParameterException.Create('Interface is not IFloat or IBaseObject.');

  Err := CreateFloat(FloatObj, Value);
  CheckRtErrorInfo(Err);

  Result := TProxyValue<T>(TProxyValue<IFloat>.Create(FloatObj));
end;

class operator TProxyValue<T>.Implicit(Value: Boolean): TProxyValue<T>;
var
  BoolObj : IBoolean;
  Err : ErrCode;
begin
  // TODO: Support IConvertible
  if (TypeInfo(T) <> TypeInfo(IBoolean)) and (TypeInfo(T) <> TypeInfo(IBaseObject)) then
    raise ERTInvalidParameterException.Create('Interface is not IBoolean or IBaseObject.');

  Err := CreateBoolean(BoolObj, Value);
  CheckRtErrorInfo(Err);

  Result := TProxyValue<T>(TProxyValue<IBoolean>.Create(BoolObj));
end;

class operator TProxyValue<T>.Implicit(Value: string): TProxyValue<T>;
var
  Str : IString;
  Err : ErrCode;
begin
  if (TypeInfo(T) <> TypeInfo(IString)) and (TypeInfo(T) <> TypeInfo(IBaseObject)) then
    raise ERTInvalidParameterException.Create('Interface is not IString or IBaseObject.');

  Str := CreateStringFromDelphiString(Value);
  Result := TProxyValue<T>(TProxyValue<IString>.Create(Str));
end;

class operator TProxyValue<T>.Implicit(Value: RtInt): TProxyValue<T>;
var
  IntObj : IInteger;
  FloatObj : IFloat;
  Err : ErrCode;
begin
  if ((TypeInfo(T) = TypeInfo(IInteger)) or (TypeInfo(T) = TypeInfo(IBaseObject))) then
  begin
    Err := CreateInteger(IntObj, Value);
    CheckRtErrorInfo(Err);

    Exit(TProxyValue<T>(TProxyValue<IInteger>.Create(IntObj)))
  end
  else if (TypeInfo(T) = TypeInfo(IFloat)) then
  begin
    Err := CreateFloat(FloatObj, Value);
    CheckRtErrorInfo(Err);

    Exit(TProxyValue<T>(TProxyValue<IFloat>.Create(FloatObj)))
  end
end;

end.
