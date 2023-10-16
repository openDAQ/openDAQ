unit OpenDAQ.Deserializer;

interface
uses
  OpenDAQ.CoreTypes,
  OpenDAQ.ObjectPtr,
  OpenDAQ.ProxyValue;
  
type
  IDeserializerPtr<T : IBaseObject> = interface(IObjectPtr<IDeserializer>)
  ['{131FA3D2-2454-4909-A037-AAD747CC80DE}']
    function DeserializePtr(Serialized: IString; Context: IBaseObject = nil): IObjectPtr<T>; overload;
    function DeserializePtr(Serialized: string; Context: IBaseObject = nil): IObjectPtr<T>; overload;

    function DeserializeRaw(Serialized: IString; Context: IBaseObject = nil): T; overload;
    function DeserializeRaw(Serialized: string; Context: IBaseObject = nil): T; overload;

    function Deserialize(Serialized: IString; Context: IBaseObject = nil): TProxyValue<T>; overload;
    function Deserialize(Serialized: string; Context: IBaseObject = nil): TProxyValue<T>; overload;

    procedure Update(Updatable: IUpdatable; Mode: TConfigurationMode; Serialized: IString); overload;
    procedure Update(Updatable: IUpdatable; Mode: TConfigurationMode; Serialized: string); overload;
  end;

  TDeserializerPtr<T : IBaseObject> = class(TObjectPtr<IDeserializer>, IDeserializerPtr<T>, IDeserializer)
  public
    constructor Create(); overload; override;
    constructor Create(Obj: IBaseObject); overload; override;
    constructor Create(Obj: IDeserializer); overload;

    function DeserializePtr(Serialized: IString; Context: IBaseObject = nil): IObjectPtr<T>; overload;
    function DeserializePtr(Serialized: string; Context: IBaseObject): IObjectPtr<T>; overload;

    function DeserializeRaw(Serialized: IString; Context: IBaseObject): T; overload;
    function DeserializeRaw(Serialized: string; Context: IBaseObject): T; overload;

    function Deserialize(Serialized: IString; Context: IBaseObject): TProxyValue<T>; overload;
    function Deserialize(Serialized: string; Context: IBaseObject): TProxyValue<T>; overload;

    procedure Update(Updatable: IUpdatable; Mode: TConfigurationMode; Serialized: IString); overload;
    procedure Update(Updatable: IUpdatable; Mode: TConfigurationMode; Serialized: string); overload;
  private
    function IDeserializer.Deserialize = Interface_Deserialize;
    function IDeserializer.Update = Interface_Update;

    function Interface_Deserialize(Serialized: IString; Context: IBaseObject; out Obj: IBaseObject): ErrCode stdcall;
    function Interface_Update(Updatable: IUpdatable; Mode: TConfigurationMode; Serialized: IString): ErrCode stdcall;
  end;

  IDeserializerPtr = IDeserializerPtr<IBaseObject>;
  TDeserializerPtr = TDeserializerPtr<IBaseObject>;

implementation
uses
  System.SysUtils,
  System.TypInfo,
  OpenDAQ.Exceptions,
  OpenDAQ.SmartPtrRegistry;

{ TDeserializerPtr }

constructor TDeserializerPtr<T>.Create();
var
  Deserializer : IDeserializer;
  Err : ErrCode;
begin
  Err := CreateJsonDeserializer(Deserializer);
  CheckRtErrorInfo(Err);

  Create(Deserializer);
end;

constructor TDeserializerPtr<T>.Create(Obj: IBaseObject);
begin
  inherited Create(Obj);
end;

constructor TDeserializerPtr<T>.Create(Obj: IDeserializer);
begin
  inherited Create(Obj);
end;

function TDeserializerPtr<T>.DeserializeRaw(Serialized: IString; Context: IBaseObject): T;
var
  Err : ErrCode;
  Deserialized : IBaseObject;
  Typed: T;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.Deserialize(Serialized, Context, Deserialized);
  CheckRtErrorInfo(Err);

  if TypeInfo(T) = TypeInfo(IBaseObject) then
    Exit(T(Deserialized));

  if not Supports(Deserialized, GetTypeData(TypeInfo(T))^.Guid, Typed) then
    raise ERTNoInterfaceException.Create('The deserialized value does not implement the requested interface')
  else
    Result := Typed;
end;

function TDeserializerPtr<T>.DeserializeRaw(Serialized: string; Context: IBaseObject): T;
var
  Err : ErrCode;
  Deserialized : IBaseObject;
  Typed: T;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Result := DeserializeRaw(CreateStringFromDelphiString(Serialized), Context);
end;

function TDeserializerPtr<T>.DeserializePtr(Serialized: IString; Context: IBaseObject): IObjectPtr<T>;
var
  Deserialized: T;
  PtrClass: SmartPtrClass;
begin
  Deserialized := DeserializeRaw(Serialized, Context);
  PtrClass := TSmartPtrRegistry.GetPtrClass(GetTypeData(TypeInfo(T))^.Guid);

  Result := PtrClass.Create(Deserialized) as IObjectPtr<T>;
end;

function TDeserializerPtr<T>.Deserialize(Serialized: IString; Context: IBaseObject): TProxyValue<T>;
begin
  Result := TProxyValue<T>.Create(DeserializeRaw(Serialized, Context));
end;

function TDeserializerPtr<T>.Deserialize(Serialized: string; Context: IBaseObject): TProxyValue<T>;
begin
  Result := TProxyValue<T>.Create(DeserializeRaw(CreateStringFromDelphiString(Serialized), Context));
end;

function TDeserializerPtr<T>.DeserializePtr(Serialized: string; Context: IBaseObject): IObjectPtr<T>;
begin
  Result := DeserializePtr(CreateStringFromDelphiString(Serialized), Context);
end;

function TDeserializerPtr<T>.Interface_Deserialize(Serialized: IString; Context: IBaseObject; out Obj: IBaseObject): ErrCode;
begin
  Result := FObject.Deserialize(Serialized, Context, Obj);
end;

function TDeserializerPtr<T>.Interface_Update(Updatable: IUpdatable;
  Mode: TConfigurationMode; Serialized: IString): ErrCode;
begin

end;

procedure TDeserializerPtr<T>.Update(Updatable: IUpdatable; Mode: TConfigurationMode; Serialized: IString);
begin

end;

procedure TDeserializerPtr<T>.Update(Updatable: IUpdatable; Mode: TConfigurationMode; Serialized: string);
begin

end;

initialization
  TSmartPtrRegistry.RegisterPtr(IDeserializer, IDeserializerPtr, TDeserializerPtr);

finalization
  TSmartPtrRegistry.UnregisterPtr(IDeserializer);

end.