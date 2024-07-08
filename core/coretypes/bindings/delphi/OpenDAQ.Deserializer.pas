unit OpenDAQ.Deserializer;

interface
uses
  OpenDAQ.CoreTypes,
  OpenDAQ.ObjectPtr,
  OpenDAQ.ProxyValue,
  OpenDAQ.TString,
  OpenDAQ.TFunction,
  OpenDAQ.Updatable,
  OpenDAQ.TProcedure;

type
  {$MINENUMSIZE 4}

  IDeserializerPtr = interface(IObjectPtr<IDeserializer>)
  ['{dd38d3e0-f7f4-5474-a08c-2a3258b660e1}']
    function Deserialize(Serialized: IString; Context: TProxyValue; FactoryCallback: IFunction): TProxyValue; overload;
    function Deserialize(Serialized: IStringPtr; Context: ISmartPtr; FactoryCallback: IFunctionPtr): TProxyValue; overload;
    function Deserialize(Serialized: string; Context: ISmartPtr; FactoryCallback: IFunctionPtr): TProxyValue; overload;
    function Deserialize(Serialized: string; Context: TProxyValue; FactoryCallback: IFunctionPtr): TProxyValue; overload;

    procedure Update(Updatable: IUpdatable; Serialized: IString); overload;
    procedure Update(Updatable: IUpdatablePtr; Serialized: IStringPtr); overload;
    procedure Update(Updatable: IUpdatablePtr; Serialized: string); overload;

    procedure CallCustomProc(CustomDeserialize: IProcedure; Serialized: IString); overload;
    procedure CallCustomProc(CustomDeserialize: IProcedurePtr; Serialized: IStringPtr); overload;
    procedure CallCustomProc(CustomDeserialize: IProcedurePtr; Serialized: string); overload;
  end;

  TDeserializerPtr = class(TObjectPtr<IDeserializer>, IDeserializerPtr, IDeserializer)
  public
    constructor Create(Obj: IBaseObject); overload; override;
    constructor Create(Obj: IDeserializer); overload;

    function Deserialize(Serialized: IString; Context: TProxyValue; FactoryCallback: IFunction): TProxyValue; overload;
    function Deserialize(Serialized: IStringPtr; Context: ISmartPtr; FactoryCallback: IFunctionPtr): TProxyValue; overload;
    function Deserialize(Serialized: string; Context: ISmartPtr; FactoryCallback: IFunctionPtr): TProxyValue; overload;
    function Deserialize(Serialized: string; Context: TProxyValue; FactoryCallback: IFunctionPtr): TProxyValue; overload;

    procedure Update(Updatable: IUpdatable; Serialized: IString); overload;
    procedure Update(Updatable: IUpdatablePtr; Serialized: IStringPtr); overload;
    procedure Update(Updatable: IUpdatablePtr; Serialized: string); overload;

    procedure CallCustomProc(CustomDeserialize: IProcedure; Serialized: IString); overload;
    procedure CallCustomProc(CustomDeserialize: IProcedurePtr; Serialized: IStringPtr); overload;
    procedure CallCustomProc(CustomDeserialize: IProcedurePtr; Serialized: string); overload;
  private
    function IDeserializer.Deserialize = Interface_Deserialize;
    function IDeserializer.Update = Interface_Update;
    function IDeserializer.CallCustomProc = Interface_CallCustomProc;

    function Interface_Deserialize(Serialized: IString; Context: IBaseObject; FactoryCallback: IFunction; out Obj: IBaseObject): ErrCode; stdcall;
    function Interface_Update(Updatable: IUpdatable; Serialized: IString): ErrCode; stdcall;
    function Interface_CallCustomProc(CustomDeserialize: IProcedure; Serialized: IString): ErrCode; stdcall;
  end;


implementation
uses
  OpenDAQ.CoreTypes.Errors,
  OpenDAQ.Exceptions,
  OpenDAQ.SmartPtrRegistry;


constructor TDeserializerPtr.Create(Obj: IDeserializer);
begin
  inherited Create(Obj);
end;

constructor TDeserializerPtr.Create(Obj: IBaseObject);
begin
  inherited Create(Obj);
end;

function TDeserializerPtr.Deserialize(Serialized: IString; Context: TProxyValue; FactoryCallback: IFunction): TProxyValue;
var
  Err: ErrCode;
  Obj: IBaseObject;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.Deserialize(Serialized, Context, FactoryCallback, Obj);
  CheckRtErrorInfo(Err);

  Result := TProxyValue.Create(Obj);
end;

function TDeserializerPtr.Deserialize(Serialized: IStringPtr; Context: ISmartPtr; FactoryCallback: IFunctionPtr): TProxyValue;
var
  Err: ErrCode;
  Obj: IBaseObject;
  SerializedIntf: IString;
  ContextIntf: IBaseObject;
  FactoryCallbackIntf: IFunction;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  if Assigned(Serialized) then
    SerializedIntf := Serialized.GetInterface()
  else
    SerializedIntf := nil;

  if Assigned(Context) then
    ContextIntf := Context.GetObject()
  else
    ContextIntf := nil;

  if Assigned(FactoryCallback) then
    FactoryCallbackIntf := FactoryCallback.GetInterface()
  else
    FactoryCallbackIntf := nil;

  Err := FObject.Deserialize(SerializedIntf, ContextIntf, FactoryCallbackIntf, Obj);
  CheckRtErrorInfo(Err);

  Result := TProxyValue.Create(Obj);
end;

function TDeserializerPtr.Deserialize(Serialized: string; Context: ISmartPtr; FactoryCallback: IFunctionPtr): TProxyValue;
var
  Err: ErrCode;
  Obj: IBaseObject;
  ContextIntf: IBaseObject;
  FactoryCallbackIntf: IFunction;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  if Assigned(Context) then
    ContextIntf := Context.GetObject()
  else
    ContextIntf := nil;

  if Assigned(FactoryCallback) then
    FactoryCallbackIntf := FactoryCallback.GetInterface()
  else
    FactoryCallbackIntf := nil;

  Err := FObject.Deserialize(CreateStringFromDelphiString(Serialized), ContextIntf, FactoryCallbackIntf, Obj);
  CheckRtErrorInfo(Err);

  Result := TProxyValue.Create(Obj);
end;

function TDeserializerPtr.Deserialize(Serialized: string; Context: TProxyValue; FactoryCallback: IFunctionPtr): TProxyValue;
var
  Err: ErrCode;
  Obj: IBaseObject;
  FactoryCallbackIntf: IFunction;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  if Assigned(FactoryCallback) then
    FactoryCallbackIntf := FactoryCallback.GetInterface()
  else
    FactoryCallbackIntf := nil;

  Err := FObject.Deserialize(CreateStringFromDelphiString(Serialized), Context, FactoryCallbackIntf, Obj);
  CheckRtErrorInfo(Err);

  Result := TProxyValue.Create(Obj);
end;

procedure TDeserializerPtr.Update(Updatable: IUpdatable; Serialized: IString);
var
  Err: ErrCode;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.Update(Updatable, Serialized);
  CheckRtErrorInfo(Err);
end;

procedure TDeserializerPtr.Update(Updatable: IUpdatablePtr; Serialized: IStringPtr);
var
  Err: ErrCode;
  UpdatableIntf: IUpdatable;
  SerializedIntf: IString;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  if Assigned(Updatable) then
    UpdatableIntf := Updatable.GetInterface()
  else
    UpdatableIntf := nil;

  if Assigned(Serialized) then
    SerializedIntf := Serialized.GetInterface()
  else
    SerializedIntf := nil;

  Err := FObject.Update(UpdatableIntf, SerializedIntf);
  CheckRtErrorInfo(Err);
end;

procedure TDeserializerPtr.Update(Updatable: IUpdatablePtr; Serialized: string);
var
  Err: ErrCode;
  UpdatableIntf: IUpdatable;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  if Assigned(Updatable) then
    UpdatableIntf := Updatable.GetInterface()
  else
    UpdatableIntf := nil;

  Err := FObject.Update(UpdatableIntf, CreateStringFromDelphiString(Serialized));
  CheckRtErrorInfo(Err);
end;

procedure TDeserializerPtr.CallCustomProc(CustomDeserialize: IProcedure; Serialized: IString);
var
  Err: ErrCode;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.CallCustomProc(CustomDeserialize, Serialized);
  CheckRtErrorInfo(Err);
end;

procedure TDeserializerPtr.CallCustomProc(CustomDeserialize: IProcedurePtr; Serialized: IStringPtr);
var
  Err: ErrCode;
  CustomDeserializeIntf: IProcedure;
  SerializedIntf: IString;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  if Assigned(CustomDeserialize) then
    CustomDeserializeIntf := CustomDeserialize.GetInterface()
  else
    CustomDeserializeIntf := nil;

  if Assigned(Serialized) then
    SerializedIntf := Serialized.GetInterface()
  else
    SerializedIntf := nil;

  Err := FObject.CallCustomProc(CustomDeserializeIntf, SerializedIntf);
  CheckRtErrorInfo(Err);
end;

procedure TDeserializerPtr.CallCustomProc(CustomDeserialize: IProcedurePtr; Serialized: string);
var
  Err: ErrCode;
  CustomDeserializeIntf: IProcedure;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  if Assigned(CustomDeserialize) then
    CustomDeserializeIntf := CustomDeserialize.GetInterface()
  else
    CustomDeserializeIntf := nil;

  Err := FObject.CallCustomProc(CustomDeserializeIntf, CreateStringFromDelphiString(Serialized));
  CheckRtErrorInfo(Err);
end;

function TDeserializerPtr.Interface_Deserialize(Serialized: IString; Context: IBaseObject; FactoryCallback: IFunction; out Obj: IBaseObject): ErrCode; stdcall;
begin
  Result := FObject.Deserialize(Serialized, Context, FactoryCallback, Obj);
end;

function TDeserializerPtr.Interface_Update(Updatable: IUpdatable; Serialized: IString): ErrCode; stdcall;
begin
  Result := FObject.Update(Updatable, Serialized);
end;

function TDeserializerPtr.Interface_CallCustomProc(CustomDeserialize: IProcedure; Serialized: IString): ErrCode; stdcall;
begin
  Result := FObject.CallCustomProc(CustomDeserialize, Serialized);
end;

initialization
  TSmartPtrRegistry.RegisterPtr(IDeserializer, IDeserializerPtr, TDeserializerPtr);

finalization
  TSmartPtrRegistry.UnregisterPtr(IDeserializer);

end.
