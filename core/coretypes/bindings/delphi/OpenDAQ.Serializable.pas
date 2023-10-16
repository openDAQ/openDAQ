unit OpenDAQ.Serializable;

interface
uses
  OpenDAQ.CoreTypes,
  OpenDAQ.ObjectPtr;
  
type
  TSerializablePtr = class(TObjectPtr<ISerializable>, ISerializablePtr, ISerializable)
  public
    constructor Create(Obj : IBaseObject); overload; override;
    constructor Create(Obj : ISerializable); overload;

    procedure Serialize(Serializer: ISerializer);
    function GetSerializeId(): PAnsiChar;

  private
    function ISerializable.Serialize = Interface_Serialize;
    function ISerializable.GetSerializeId = Interface_GetSerializeId;

    function Interface_Serialize(Serializer: ISerializer): ErrCode; stdcall;
    function Interface_GetSerializeId(const Id: PPAnsiChar): ErrCode; stdcall;
  end;

implementation
uses
  OpenDAQ.Exceptions,
  OpenDAQ.SmartPtrRegistry;

{ TSerializablePtr }

constructor TSerializablePtr.Create(Obj: IBaseObject);
begin
  inherited Create(Obj);
end;

constructor TSerializablePtr.Create(Obj: ISerializable);
begin
  inherited Create(Obj);
end;

procedure TSerializablePtr.Serialize(Serializer: ISerializer);
var
  Err : ErrCode;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.Serialize(Serializer);
  CheckRtErrorInfo(Err);
end;

function TSerializablePtr.GetSerializeId(): PAnsiChar;
var
  Err : ErrCode;
  Id : PAnsiChar;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.GetSerializeId(@Id);
  CheckRtErrorInfo(Err);

  Result := Id;
end;

function TSerializablePtr.Interface_GetSerializeId(const Id: PPAnsiChar): ErrCode;
begin
  Result := FObject.GetSerializeId(Id);
end;

function TSerializablePtr.Interface_Serialize(Serializer: ISerializer): ErrCode;
begin
  Result := FObject.Serialize(Serializer);
end;

initialization
  TSmartPtrRegistry.RegisterPtr(ISerializable, ISerializablePtr, TSerializablePtr);

finalization
  TSmartPtrRegistry.UnregisterPtr(ISerializable);

end.
