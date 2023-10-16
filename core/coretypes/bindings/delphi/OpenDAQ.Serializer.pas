unit OpenDAQ.Serializer;

interface
uses
  OpenDAQ.CoreTypes,
  OpenDAQ.List,
  OpenDAQ.TString,
  OpenDAQ.ObjectPtr;
  
type
  ISerializerPtr = interface(IObjectPtr<ISerializer>)
  ['{4AD25D34-A404-4164-861F-70E6526612A4}']

    procedure StartTaggedObject(Obj: ISerializable);

    procedure StartObject();
    procedure EndObject();
    procedure StartList();
    procedure EndList();

    function GetOutput(): IStringPtr; overload;
    function GetOutputString(): string; overload;

    procedure Key(const Str: PAnsiChar); overload;
    procedure Key(Name: IString); overload;
    procedure Key(const Str: PAnsiChar; Len: SizeT); overload;

    procedure WriteInt(Int: RtInt);
    procedure WriteBool(Bool: Boolean);
    procedure WriteFloat(Real: Double);
    procedure WriteNull();

    procedure WriteString(const Str: PAnsiChar; Len: SizeT); overload;
    procedure WriteString(Str : string); overload;
    procedure WriteString(Str : IString); overload;

    procedure Reset();
    function IsComplete(): Boolean;
  end;

  ISerializedListPtr = interface;

  ISerializedObjectPtr = interface(IObjectPtr<ISerializedObject>)
  ['{D04E4F2D-1C31-47E8-BF85-FE468F1C4FCA}']
    function ReadSerializedObject(Key : IString): ISerializedObjectPtr; overload;
    function ReadSerializedObject(Key : string): ISerializedObjectPtr; overload;

    function ReadSerializedList(Key : IString): ISerializedListPtr; overload;
    function ReadSerializedList(Key : string): ISerializedListPtr; overload;

    function ReadList(Key : IString; Context: IBaseObject = nil): IListPtr<IBaseObject>; overload;
    function ReadList(Key : string; Context: IBaseObject = nil): IListPtr<IBaseObject>; overload;

    function ReadObject(Key : IString; Context: IBaseObject = nil): IObjectPtr; overload;
    function ReadObject(Key : string; Context: IBaseObject = nil): IObjectPtr; overload;

    function ReadStringPtr(Key : IString): IStringPtr; overload;
    function ReadString(Key : IString): string; overload;

    function ReadStringPtr(Key : string): IStringPtr; overload;
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
  end;

  ISerializedListPtr = interface(IObjectPtr<ISerializedList>)
  ['{F7896FC1-BD92-4988-9BDA-23253EE58CC7}']
    function ReadSerializedObject(): ISerializedObjectPtr;
    function ReadSerializedList(): ISerializedListPtr;
    function ReadList(Context: IBaseObject = nil): IListPtr<IBaseObject>;
    function ReadObject(Context: IBaseObject = nil): IObjectPtr;
    function ReadStringPtr(): IStringPtr; overload;
    function ReadString(): string; overload;
    function ReadBool(): Boolean;
    function ReadFloat(): RtFloat;
    function ReadInt(): RtInt;
    function GetCount(): SizeT;
    function GetCurrentItemType(): TCoreType;
  end;

  TSerializerPtr = class(TObjectPtr<ISerializer>, ISerializerPtr, ISerializer)
  public
    constructor Create(); overload; override;
    constructor Create(Obj : IBaseObject); overload; override;
    constructor Create(Obj : ISerializer); overload;

    procedure StartTaggedObject(Obj: ISerializable);

    procedure StartObject();
    procedure EndObject();
    procedure StartList();
    procedure EndList();

    function GetOutput(): IStringPtr; overload;
    function GetOutputString(): string; overload;

    procedure Key(const Str: PAnsiChar); overload;
    procedure Key(Name: IString); overload;
    procedure Key(const Str: PAnsiChar; Len: SizeT); overload;
    procedure WriteInt(Int: RtInt);
    procedure WriteBool(Bool: Boolean);
    procedure WriteFloat(Real: Double);
    procedure WriteString(const Str: PAnsiChar; Len: SizeT); overload;
    procedure WriteString(Str : string); overload;
    procedure WriteString(Str : IString); overload;
    procedure WriteNull();

    procedure Reset();
    function IsComplete(): Boolean;

  private
    function ISerializer.StartTaggedObject = Interface_StartTaggedObject;
    function ISerializer.StartObject = Interface_StartObject;
    function ISerializer.EndObject = Interface_EndObject;
    function ISerializer.StartList = Interface_StartList;
    function ISerializer.EndList = Interface_EndList;
    function ISerializer.GetOutput = Interface_GetOutput;
    function ISerializer.Key = Interface_Key;
    function ISerializer.KeyStr = Interface_KeyStr;
    function ISerializer.KeyRaw = Interface_KeyRaw;
    function ISerializer.WriteInt = Interface_WriteInt;
    function ISerializer.WriteBool = Interface_WriteBool;
    function ISerializer.WriteFloat = Interface_WriteFloat;
    function ISerializer.WriteString = Interface_WriteString;
    function ISerializer.WriteNull = Interface_WriteNull;
    function ISerializer.Reset = Interface_Reset;
    function ISerializer.IsComplete = Interface_IsComplete;

    function Interface_StartTaggedObject(Obj: ISerializable): ErrCode; stdcall;

    function Interface_StartObject(): ErrCode; stdcall;
    function Interface_EndObject(): ErrCode; stdcall;
    function Interface_StartList(): ErrCode; stdcall;
    function Interface_EndList(): ErrCode; stdcall;

    function Interface_GetOutput(out Serialized: IString): ErrCode; stdcall;

    function Interface_Key(const Str: PAnsiChar): ErrCode; stdcall;
    function Interface_KeyStr(Name: IString): ErrCode stdcall;
    function Interface_KeyRaw(const Str: PAnsiChar; Len: SizeT): ErrCode; stdcall;
    function Interface_WriteInt(Int: RtInt): ErrCode stdcall;
    function Interface_WriteBool(Bool: Boolean): ErrCode; stdcall;
    function Interface_WriteFloat(Real: RtFloat): ErrCode; stdcall;
    function Interface_WriteString(const Str: PAnsiChar; Len: SizeT): ErrCode; stdcall;
    function Interface_WriteNull(): ErrCode; stdcall;

    function Interface_Reset(): ErrCode stdcall;
    function Interface_IsComplete(out Complete: Boolean): ErrCode stdcall;
  end;

implementation
uses
  OpenDAQ.Exceptions,
  OpenDAQ.SmartPtrRegistry;

{ TSerializerPtr }

constructor TSerializerPtr.Create();
var
  Serializer : ISerializer;
  Err : ErrCode;
begin
  Err := CreateJsonSerializer(Serializer);
  CheckRtErrorInfo(Err);

  Create(Serializer);
end;

constructor TSerializerPtr.Create(Obj: IBaseObject);
begin
  inherited Create(Obj);
end;

constructor TSerializerPtr.Create(Obj: ISerializer);
begin
  inherited Create(Obj);
end;

procedure TSerializerPtr.StartList();
var
  Err : ErrCode;
begin
  Err := FObject.StartList();
  CheckRtErrorInfo(Err);
end;

procedure TSerializerPtr.StartObject();
var
  Err : ErrCode;
begin
  Err := FObject.StartObject();
  CheckRtErrorInfo(Err);
end;

procedure TSerializerPtr.StartTaggedObject(Obj: ISerializable);
var
  Err : ErrCode;
begin
  Err := FObject.StartTaggedObject(Obj);
  CheckRtErrorInfo(Err);
end;

procedure TSerializerPtr.EndList();
var
  Err : ErrCode;
begin
  Err := FObject.EndList();
  CheckRtErrorInfo(Err);
end;

procedure TSerializerPtr.EndObject();
var
  Err : ErrCode;
begin
  Err := FObject.EndObject();
  CheckRtErrorInfo(Err);
end;

function TSerializerPtr.GetOutput(): IStringPtr;
var
  Output : IString;
  Err : ErrCode;
begin
  Err := FObject.GetOutput(Output);
  CheckRtErrorInfo(Err);

  Result := TStringPtr.Create(Output);
end;

function TSerializerPtr.GetOutputString(): string;
begin
  Result := GetOutput().ToString();
end;

function TSerializerPtr.IsComplete(): Boolean;
var
  Err : ErrCode;
  Complete : Boolean;
begin
  Err := FObject.IsComplete(Complete);
  CheckRtErrorInfo(Err);

  Result := Complete;
end;

procedure TSerializerPtr.Key(const Str: PAnsiChar);
var
  Err : ErrCode;
begin
  Err := FObject.Key(Str);
  CheckRtErrorInfo(Err);
end;

procedure TSerializerPtr.Key(const Str: PAnsiChar; Len: SizeT);
var
  Err : ErrCode;
begin
  Err := FObject.KeyRaw(Str, Len);
  CheckRtErrorInfo(Err);
end;

procedure TSerializerPtr.Key(Name: IString);
var
  Err : ErrCode;
begin
  Err := FObject.KeyStr(Name);
  CheckRtErrorInfo(Err);
end;

procedure TSerializerPtr.Reset();
var
  Err : ErrCode;
begin
  Err := FObject.Reset();
  CheckRtErrorInfo(Err);
end;

procedure TSerializerPtr.WriteBool(Bool: Boolean);
var
  Err : ErrCode;
begin
  Err := FObject.WriteBool(Bool);
  CheckRtErrorInfo(Err);
end;

procedure TSerializerPtr.WriteFloat(Real: Double);
var
  Err : ErrCode;
begin
  Err := FObject.WriteFloat(Real);
  CheckRtErrorInfo(Err);
end;

procedure TSerializerPtr.WriteInt(Int: RtInt);
var
  Err : ErrCode;
begin
  Err := FObject.WriteInt(Int);
  CheckRtErrorInfo(Err);
end;

procedure TSerializerPtr.WriteNull();
var
  Err : ErrCode;
begin
  Err := FObject.WriteNull();
  CheckRtErrorInfo(Err);
end;

procedure TSerializerPtr.WriteString(const Str: PAnsiChar; Len: SizeT);
var
  Err : ErrCode;
begin
  Err := FObject.WriteString(Str, Len);
  CheckRtErrorInfo(Err);
end;

procedure TSerializerPtr.WriteString(Str: IString);
var
  Ptr : PAnsiChar;
  Length : SizeT;
  Err : ErrCode;
begin
  Err := Str.GetCharPtr(@Ptr);
  CheckRtErrorInfo(Err);

  Err := Str.GetLength(Length);
  CheckRtErrorInfo(Err);

  Err := FObject.WriteString(Ptr, Length);
  CheckRtErrorInfo(Err);
end;

procedure TSerializerPtr.WriteString(Str: string);
begin
  WriteString(CreateStringFromDelphiString(Str));
end;

// Decorated functions

function TSerializerPtr.Interface_EndList(): ErrCode;
begin
  Result := FObject.EndList();
end;

function TSerializerPtr.Interface_EndObject(): ErrCode;
begin
  Result := FObject.EndObject();
end;

function TSerializerPtr.Interface_GetOutput(out Serialized: IString): ErrCode;
begin
  Result := FObject.GetOutput(Serialized);
end;

function TSerializerPtr.Interface_IsComplete(out Complete: Boolean): ErrCode;
begin
  Result := FObject.IsComplete(Complete);
end;

function TSerializerPtr.Interface_Key(const Str: PAnsiChar): ErrCode;
begin
  Result := FObject.Key(Str);
end;

function TSerializerPtr.Interface_KeyRaw(const Str: PAnsiChar; Len: SizeT): ErrCode;
begin
  Result := FObject.KeyRaw(Str, Len);
end;

function TSerializerPtr.Interface_KeyStr(Name: IString): ErrCode;
begin
  Result := FObject.KeyStr(Name);
end;

function TSerializerPtr.Interface_Reset(): ErrCode;
begin
  Result := FObject.Reset();
end;

function TSerializerPtr.Interface_StartList(): ErrCode;
begin
  Result := FObject.StartList();
end;

function TSerializerPtr.Interface_StartObject(): ErrCode;
begin
  Result := FObject.StartObject();
end;

function TSerializerPtr.Interface_StartTaggedObject(Obj: ISerializable): ErrCode;
begin
  Result := FObject.StartTaggedObject(Obj);
end;

function TSerializerPtr.Interface_WriteBool(Bool: Boolean): ErrCode;
begin
  Result := FObject.WriteBool(Bool);
end;

function TSerializerPtr.Interface_WriteFloat(Real: RtFloat): ErrCode;
begin
  Result := FObject.WriteFloat(Real);
end;

function TSerializerPtr.Interface_WriteInt(Int: RtInt): ErrCode;
begin
  Result := FObject.WriteInt(Int);
end;

function TSerializerPtr.Interface_WriteNull(): ErrCode;
begin
  Result := FObject.WriteNull();
end;

function TSerializerPtr.Interface_WriteString(const Str: PAnsiChar; Len: SizeT): ErrCode;
begin
  Result := FObject.WriteString(Str, Len);
end;

initialization
  TSmartPtrRegistry.RegisterPtr(ISerializer, ISerializerPtr, TSerializerPtr);

finalization
  TSmartPtrRegistry.UnregisterPtr(ISerializer);

end.
