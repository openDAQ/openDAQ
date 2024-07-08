unit OpenDAQ.SerializedList;

interface
uses
  OpenDAQ.CoreTypes,
  OpenDAQ.List,
  OpenDAQ.TString,
  OpenDAQ.ObjectPtr,
  OpenDAQ.Serializer;

type
  TSerializedListPtr = class(TObjectPtr<ISerializedList>, ISerializedListPtr, ISerializedList)
  public
    constructor Create(Obj: IBaseObject); overload; override;
    constructor Create(Obj: ISerializedList); overload;

    function ReadSerializedObject(): ISerializedObjectPtr;
    function ReadSerializedList(): ISerializedListPtr;

    function ReadList(Context: IBaseObject = nil; FactoryCallback: IFunction = nil): IListPtr<IBaseObject>; overload;
    function ReadObject(Context: IBaseObject = nil; FactoryCallback: IFunction  = nil): IObjectPtr;

    function ReadStringPtr(): IStringPtr; overload;
    function ReadString(): string; overload;

    function ReadBool(): Boolean;
    function ReadFloat(): RtFloat;
    function ReadInt(): RtInt;

    function GetCount(): SizeT;
    function GetCurrentItemType(): TCoreType;

  private
    function ISerializedList.ReadSerializedObject = Interface_ReadSerializedObject;
    function ISerializedList.ReadSerializedList = Interface_ReadSerializedList;
    function ISerializedList.ReadList = Interface_ReadList;
    function ISerializedList.ReadObject = Interface_ReadObject;
    function ISerializedList.ReadString = Interface_ReadString;
    function ISerializedList.ReadBool = Interface_ReadBool;
    function ISerializedList.ReadFloat = Interface_ReadFloat;
    function ISerializedList.ReadInt = Interface_ReadInt;
    function ISerializedList.GetCount = Interface_GetCount;
    function ISerializedList.GetCurrentItemType = Interface_GetCurrentItemType;

    function Interface_ReadSerializedObject(out PlainObj: ISerializedObject): ErrCode stdcall;
    function Interface_ReadSerializedList(out List: ISerializedList): ErrCode stdcall;
    function Interface_ReadList(Context: IBaseObject; FactoryCallback: IFunction; out List: IListObject): ErrCode; stdcall;
    function Interface_ReadObject(Context: IBaseObject; FactoryCallback: IFunction; out Obj: IBaseObject): ErrCode; stdcall;
    function Interface_ReadString(out Str: IString): ErrCode stdcall;
    function Interface_ReadBool(out Bool: Boolean): ErrCode stdcall;
    function Interface_ReadFloat(out Real: Double): ErrCode stdcall;
    function Interface_ReadInt(out Int: RtInt): ErrCode stdcall;
    function Interface_GetCount(out Size: SizeT): ErrCode stdcall;
    function Interface_GetCurrentItemType(out AType: TCoreType): ErrCode stdcall;
  end;

implementation
uses
  OpenDAQ.Exceptions,
  OpenDAQ.SmartPtrRegistry,
  OpenDAQ.SerializedObject;

{ TSerializedList }

constructor TSerializedListPtr.Create(Obj: IBaseObject);
begin
  inherited Create(Obj);
end;

constructor TSerializedListPtr.Create(Obj: ISerializedList);
begin
  inherited Create(Obj);
end;

function TSerializedListPtr.ReadBool(): Boolean;
var
  Err : ErrCode;
  Value : Boolean;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.ReadBool(Value);
  CheckRtErrorInfo(Err);

  Result := Value;
end;

function TSerializedListPtr.ReadFloat(): RtFloat;
var
  Err : ErrCode;
  Value : RtFloat;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.ReadFloat(Value);
  CheckRtErrorInfo(Err);

  Result := Value;
end;

function TSerializedListPtr.ReadInt(): RtInt;
var
  Err : ErrCode;
  Value : RtInt;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.ReadInt(Value);
  CheckRtErrorInfo(Err);

  Result := Value;
end;

function TSerializedListPtr.ReadList(Context: IBaseObject = nil; FactoryCallback: IFunction = nil): IListPtr<IBaseObject>;
var
  Err : ErrCode;
  List : IListObject;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.ReadList(Context, FactoryCallback, List);
  CheckRtErrorInfo(Err);

  Result := TListPtr<IBaseObject>.Create(List);
end;

function TSerializedListPtr.ReadSerializedList(): ISerializedListPtr;
var
  Err : ErrCode;
  SerializedList : ISerializedList;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.ReadSerializedList(SerializedList);
  CheckRtErrorInfo(Err);

  Result := TSerializedListPtr.Create(SerializedList);
end;

function TSerializedListPtr.ReadObject(Context: IBaseObject = nil; FactoryCallback: IFunction = nil): IObjectPtr;
var
  Err : ErrCode;
  Obj : IBaseObject;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.ReadObject(Context, FactoryCallback, Obj);
  CheckRtErrorInfo(Err);

  Result := TObjectPtr<IBaseObject>.Create(Obj);
end;

function TSerializedListPtr.ReadSerializedObject(): ISerializedObjectPtr;
var
  Err : ErrCode;
  SerializedObj : ISerializedObject;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.ReadSerializedObject(SerializedObj);
  CheckRtErrorInfo(Err);

  Result := TSerializedObjectPtr.Create(SerializedObj);
end;

function TSerializedListPtr.ReadString(): string;
var
  Err : ErrCode;
  Str : IString;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.ReadString(Str);
  CheckRtErrorInfo(Err);

  Result := RtToString(Str);
end;

function TSerializedListPtr.ReadStringPtr(): IStringPtr;
var
  Err : ErrCode;
  Str : IString;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.ReadString(Str);
  CheckRtErrorInfo(Err);

  Result := TStringPtr.Create(Str);
end;

function TSerializedListPtr.GetCount(): SizeT;
var
  Err : ErrCode;
  Count : SizeT;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.GetCount(Count);
  CheckRtErrorInfo(Err);

  Result := Count;
end;

function TSerializedListPtr.GetCurrentItemType: TCoreType;
var
  Err : ErrCode;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.GetCurrentItemType(Result);
  CheckRtErrorInfo(Err);
end;

// Decorated methods

function TSerializedListPtr.Interface_ReadInt(out Int: RtInt): ErrCode;
begin
  Result := FObject.ReadInt(Int);
end;

function TSerializedListPtr.Interface_ReadBool(out Bool: Boolean): ErrCode;
begin
  Result := FObject.ReadBool(Bool);
end;

function TSerializedListPtr.Interface_ReadFloat(out Real: Double): ErrCode;
begin
  Result := FObject.ReadFloat(Real);
end;

function TSerializedListPtr.Interface_ReadString(out Str: IString): ErrCode;
begin
  Result := FObject.ReadString(Str);
end;

function TSerializedListPtr.Interface_ReadList(Context: IBaseObject; FactoryCallback: IFunction; out List: IListObject): ErrCode;
begin
  Result := FObject.ReadList(Context, FactoryCallback, List);
end;

function TSerializedListPtr.Interface_ReadSerializedList(out List: ISerializedList): ErrCode;
begin
  Result := FObject.ReadSerializedList(List);
end;

function TSerializedListPtr.Interface_ReadSerializedObject(out PlainObj: ISerializedObject): ErrCode;
begin
  Result := FObject.ReadSerializedObject(PlainObj);
end;

function TSerializedListPtr.Interface_ReadObject(Context: IBaseObject; FactoryCallback: IFunction; out Obj: IBaseObject): ErrCode;
begin
  Result := FObject.ReadObject(Context, FactoryCallback, Obj);
end;

function TSerializedListPtr.Interface_GetCount(out Size: SizeT): ErrCode;
begin
  Result := FObject.GetCount(Size);
end;

function TSerializedListPtr.Interface_GetCurrentItemType(out AType: TCoreType): ErrCode;
begin
  Result := FObject.GetCurrentItemType(AType);
end;

initialization
  TSmartPtrRegistry.RegisterPtr(ISerializedList, ISerializedListPtr, TSerializedListPtr);

finalization
  TSmartPtrRegistry.UnregisterPtr(ISerializedList);

end.
