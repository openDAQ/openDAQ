unit OpenDAQ.SerializedObject;

interface
uses
  OpenDAQ.CoreTypes,
  OpenDAQ.List,
  OpenDAQ.TString,
  OpenDAQ.ObjectPtr,
  OpenDAQ.Serializer;

type

  TSerializedObjectPtr = class(TObjectPtr<ISerializedObject>, ISerializedObjectPtr, ISerializedObject)
  public
    constructor Create(Obj : IBaseObject); overload; override;
    constructor Create(Obj : ISerializedObject); overload;

    function ReadSerializedObject(Key : IString): ISerializedObjectPtr; overload;
    function ReadSerializedObject(Key : string): ISerializedObjectPtr; overload;

    function ReadSerializedList(Key : IString): ISerializedListPtr; overload;
    function ReadSerializedList(Key : string): ISerializedListPtr; overload;

    function ReadList(Key : IString; Context: IBaseObject = nil): IListPtr<IBaseObject>; overload;
    function ReadList(Key : string; Context: IBaseObject = nil): IListPtr<IBaseObject>; overload;

    function ReadObject(Key : IString; Context: IBaseObject = nil): IObjectPtr; overload;
    function ReadObject(Key : string; Context: IBaseObject = nil): IObjectPtr; overload;

    function ReadStringPtr(Key : IString): IStringPtr; overload;
    function ReadStringPtr(Key : string): IStringPtr; overload;

    function ReadString(Key : IString): string; overload;
    function ReadString(Key : string): string; overload;

    function ReadBool(Key : IString): Boolean; overload;
    function ReadBool(Key : string): Boolean; overload;

    function ReadFloat(Key : IString): RtFloat; overload;
    function ReadFloat(Key : string): RtFloat; overload;

    function ReadInt(Key : IString): RtInt; overload;
    function ReadInt(Key : string): RtInt; overload;

    function HasKey(Key : IString): Boolean; overload;
    function HasKey(Key : string): Boolean; overload;

    function GetKeys() : IListPtr<IString>;

  private
    function ISerializedObject.ReadSerializedObject = Interface_ReadSerializedObject;
    function ISerializedObject.ReadSerializedList = Interface_ReadSerializedList;
    function ISerializedObject.ReadList = Interface_ReadList;
    function ISerializedObject.ReadObject = Interface_ReadObject;
    function ISerializedObject.ReadString = Interface_ReadString;
    function ISerializedObject.ReadBool = Interface_ReadBool;
    function ISerializedObject.ReadFloat = Interface_ReadFloat;
    function ISerializedObject.ReadInt = Interface_ReadInt;
    function ISerializedObject.HasKey = Interface_HasKey;
    function ISerializedObject.GetKeys = Interface_GetKeys;

    function Interface_ReadSerializedObject(Key: IString; out PlainObj: ISerializedObject): ErrCode stdcall;
    function Interface_ReadSerializedList(Key: IString; out List: ISerializedList): ErrCode stdcall;
    function Interface_ReadList(Key: IString; Context: IBaseObject; out List: IListObject): ErrCode stdcall;
    function Interface_ReadObject(Key: IString; Context: IBaseObject; out Obj: IBaseObject): ErrCode stdcall;
    function Interface_ReadString(Key: IString; out Str: IString): ErrCode stdcall;
    function Interface_ReadBool(Key: IString; out Bool: Boolean): ErrCode stdcall;
    function Interface_ReadFloat(Key: IString; out Real: Double): ErrCode stdcall;
    function Interface_ReadInt(Key: IString; out Int: RtInt): ErrCode stdcall;
    function Interface_HasKey(Key: IString; out HasKey: Boolean): ErrCode stdcall;
    function Interface_GetKeys(out List: IListObject): ErrCode stdcall;
  end;

implementation
uses
  OpenDAQ.Exceptions,
  OpenDAQ.SerializedList,
  OpenDAQ.SmartPtrRegistry;

{ TSerializedObjectPtr }

constructor TSerializedObjectPtr.Create(Obj: IBaseObject);
begin
  inherited Create(Obj);
end;

constructor TSerializedObjectPtr.Create(Obj: ISerializedObject);
begin
  inherited Create(Obj);
end;

function TSerializedObjectPtr.HasKey(Key: IString): Boolean;
var
  Err : ErrCode;
  Value : Boolean;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.HasKey(Key, Value);
  CheckRtErrorInfo(Err);

  Result := Value;
end;

function TSerializedObjectPtr.HasKey(Key: string): Boolean;
var
  Err : ErrCode;
  Value : Boolean;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.HasKey(CreateStringFromDelphiString(Key), Value);
  CheckRtErrorInfo(Err);

  Result := Value;
end;

function TSerializedObjectPtr.GetKeys(): IListPtr<IString>;
var
  Err : ErrCode;
  List : IListObject;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.GetKeys(List);
  CheckRtErrorInfo(Err);

  Result := TListPtr<IString>.Create(List);
end;

function TSerializedObjectPtr.ReadBool(Key : IString): Boolean;
var
  Err : ErrCode;
  Value : Boolean;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.ReadBool(Key, Value);
  CheckRtErrorInfo(Err);

  Result := Value;
end;

function TSerializedObjectPtr.ReadBool(Key : string): Boolean;
begin
  Result := ReadBool(CreateStringFromDelphiString(Key));
end;

function TSerializedObjectPtr.ReadFloat(Key : IString): RtFloat;
var
  Err : ErrCode;
  Value : RtFloat;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.ReadFloat(Key, Value);
  CheckRtErrorInfo(Err);

  Result := Value;
end;

function TSerializedObjectPtr.ReadFloat(Key : string): RtFloat;
begin
  Result := ReadFloat(CreateStringFromDelphiString(Key));
end;

function TSerializedObjectPtr.ReadInt(Key : IString): RtInt;
var
  Err : ErrCode;
  Value : RtInt;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.ReadInt(Key, Value);
  CheckRtErrorInfo(Err);

  Result := Value;
end;

function TSerializedObjectPtr.ReadInt(Key : string): RtInt;
begin
  Result := ReadInt(CreateStringFromDelphiString(Key));
end;

function TSerializedObjectPtr.ReadList(Key : IString; Context: IBaseObject = nil): IListPtr<IBaseObject>;
var
  Err : ErrCode;
  List : IListObject;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.ReadList(Key, Context, List);
  CheckRtErrorInfo(Err);

  Result := TListPtr<IBaseObject>.Create(List);
end;

function TSerializedObjectPtr.ReadList(Key : string; Context: IBaseObject = nil): IListPtr<IBaseObject>;
begin
  Result := ReadList(CreateStringFromDelphiString(Key));
end;

function TSerializedObjectPtr.ReadObject(Key : IString; Context: IBaseObject = nil): IObjectPtr;
var
  Err : ErrCode;
  Obj : IBaseObject;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.ReadObject(Key, Context, Obj);
  CheckRtErrorInfo(Err);

  Result := TObjectPtr<IBaseObject>.Create(Obj);
end;

function TSerializedObjectPtr.ReadObject(Key : string; Context: IBaseObject = nil): IObjectPtr;
begin
  Result := ReadObject(CreateStringFromDelphiString(Key));
end;

function TSerializedObjectPtr.ReadSerializedList(Key : IString): ISerializedListPtr;
var
  Err : ErrCode;
  Obj : ISerializedList;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.ReadSerializedList(Key, Obj);
  CheckRtErrorInfo(Err);

  Result := TSerializedListPtr.Create(Obj);
end;

function TSerializedObjectPtr.ReadSerializedList(Key : string): ISerializedListPtr;
begin
  Result := ReadSerializedList(CreateStringFromDelphiString(Key));
end;

function TSerializedObjectPtr.ReadSerializedObject(Key : IString): ISerializedObjectPtr;
var
  Err : ErrCode;
  Obj : ISerializedObject;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.ReadSerializedObject(Key, Obj);
  CheckRtErrorInfo(Err);

  Result := TSerializedObjectPtr.Create(Obj);
end;

function TSerializedObjectPtr.ReadSerializedObject(Key : string): ISerializedObjectPtr;
begin
  Result := ReadSerializedObject(CreateStringFromDelphiString(Key));
end;

function TSerializedObjectPtr.ReadStringPtr(Key : IString): IStringPtr;
var
  Err : ErrCode;
  Str : IString;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.ReadString(Key, Str);
  CheckRtErrorInfo(Err);

  Result := TStringPtr.Create(Str);
end;

function TSerializedObjectPtr.ReadStringPtr(Key : string): IStringPtr;
begin
  Result := ReadStringPtr(CreateStringFromDelphiString(Key));
end;

function TSerializedObjectPtr.ReadString(Key : IString): string;
var
  Err : ErrCode;
  Obj : IString;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.ReadString(Key, Obj);
  CheckRtErrorInfo(Err);

  Result := RtToString(Obj);
end;

function TSerializedObjectPtr.ReadString(Key : string): string;
begin
  Result := ReadString(CreateStringFromDelphiString(Key));
end;

// Decorated Methods

function TSerializedObjectPtr.Interface_ReadSerializedObject(Key: IString; out PlainObj: ISerializedObject): ErrCode;
begin
  Result := FObject.ReadSerializedObject(Key, PlainObj);
end;

function TSerializedObjectPtr.Interface_ReadObject(Key: IString; Context: IBaseObject; out Obj: IBaseObject): ErrCode;
begin
  Result := FObject.ReadObject(Key, Context, Obj);
end;

function TSerializedObjectPtr.Interface_ReadSerializedList(Key: IString; out List: ISerializedList): ErrCode;
begin
  Result := FObject.ReadSerializedList(Key, List);
end;

function TSerializedObjectPtr.Interface_ReadList(Key: IString; Context: IBaseObject; out List: IListObject): ErrCode;
begin
  Result := FObject.ReadList(Key, Context, List);
end;

function TSerializedObjectPtr.Interface_ReadString(Key: IString; out Str: IString): ErrCode;
begin
  Result := FObject.ReadString(Key, Str);
end;

function TSerializedObjectPtr.Interface_ReadInt(Key: IString; out Int: RtInt): ErrCode;
begin
  Result := FObject.ReadInt(Key, Int);
end;

function TSerializedObjectPtr.Interface_ReadBool(Key: IString; out Bool: Boolean): ErrCode;
begin
  Result := FObject.ReadBool(Key, Bool);
end;

function TSerializedObjectPtr.Interface_ReadFloat(Key: IString; out Real: Double): ErrCode;
begin
  Result := FObject.ReadFloat(Key, Real);
end;

function TSerializedObjectPtr.Interface_HasKey(Key: IString; out HasKey: Boolean): ErrCode;
begin
  Result := FObject.HasKey(Key, HasKey);
end;

function TSerializedObjectPtr.Interface_GetKeys(out List: IListObject): ErrCode;
begin
  Result := FObject.GetKeys(List);
end;

initialization
  TSmartPtrRegistry.RegisterPtr(ISerializedObject, ISerializedObjectPtr, TSerializedObjectPtr);

finalization
  TSmartPtrRegistry.UnregisterPtr(ISerializedObject);

end.
