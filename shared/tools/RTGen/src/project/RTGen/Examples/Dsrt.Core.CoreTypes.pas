unit Dsrt.Core.CoreTypes;

interface

uses
  Dsrt.Core.CoreTypes.Errors;

type
  SizeT = Nativeint;
  ErrCode = Cardinal;
  CoreType = Integer;
  PBoolean = ^Boolean;
  RtInt = Int64;

  TCoreType = (ctBool = 0, ctInt, ctFloat, ctString, ctList, ctDict, ctFunc, ctObject, ctUndefined);

  ISerializable = interface;
  ISerializer = interface;
  ISerializedObject = interface;
  IBaseObject = interface;

  TDSRTDeserializerFactory = function(SerializedObject: ISerializedObject; var Obj: IBaseObject): Errcode; cdecl;

  IBaseObject = interface(IUnknown)
    ['{E8F364F8-E940-572D-BB89-8A7D2AE1DDE7}']
    function BorrowInterface(const IID: TGUID; out Obj): HResult; stdcall;
    procedure Dispose; stdcall;

    function GetHashCodeEx(var HashCode: SizeT): ErrCode; stdcall;
    function EqualsObject(Other: IBaseObject): ErrCode; stdcall;
    function ToCharPtr(Str: PPAnsiChar): ErrCode; stdcall;
  end;

  IIterator = interface(IBaseObject)
  ['{1B66CB09-960D-52DC-8282-A1E24319D68E}']
    function MoveNext: ErrCode stdcall;
    function GetCurrent(var BaseObject: IBaseObject): ErrCode stdcall;
  end;

  IBoolean = interface(IBaseObject)
    ['{F41A7890-D353-5F5D-87D9-B8DCB8CD53C1}']
    function GetValue(Value: PBoolean): ErrCode; stdcall;
    function EqualsValue(const Value: Boolean): ErrCode stdcall;
  end;

  IFloat = interface(IBaseObject)
  ['{454D3070-99EE-5167-A603-228B7EC6701C}']
    function GetValue(Value: PDouble): ErrCode; stdcall;
    function EqualsValue(const Value: Double): ErrCode stdcall;
  end;

  IInteger = interface(IBaseObject)
  ['{4DF6D212-BA6C-506B-AFDD-71CA884B399D}']
    function GetValue(Value: PInt64): ErrCode; stdcall;
    function EqualsValue(const Value: Int64): ErrCode stdcall;
  end;

  IString = interface(IBaseObject)
  ['{3D7F9D7D-8A70-5339-9038-B53F5FBF2442}']
    function GetCharPtr(Value: PPAnsiChar): ErrCode stdcall;
    function GetLength(Size: PNativeInt): ErrCode stdcall;
  end;

  IListObject = interface(IBaseObject)
  ['{D43F916D-9902-562D-AB87-E69A61326368}']
    function GetItemAt(Index: SizeT; var BaseObject: IBaseObject): ErrCode stdcall;
    function GetCount(var Size: SizeT): ErrCode stdcall;
    function SetItemAt(Index: SizeT; BaseObject: IBaseObject): ErrCode stdcall;
    function PushBack(BaseObject: IBaseObject): ErrCode stdcall;
    function PushFront(BaseObject: IBaseObject): ErrCode stdcall;
    function MoveBack(BaseObject: IBaseObject): ErrCode stdcall;
    function MoveFront(BaseObject: IBaseObject): ErrCode stdcall;
    function PopBack(var BaseObject: IBaseObject): ErrCode stdcall;
    function PopFront(var BaseObject: IBaseObject): ErrCode stdcall;
    function InsertAt(Index: SizeT; BaseObject: IBaseObject): ErrCode stdcall;
    function RemoveAt(Index: SizeT; var BaseObject: IBaseObject): ErrCode stdcall;
    function DeleteAt(Index: SizeT): ErrCode stdcall;
    function Clear: ErrCode stdcall;

    function CreateStartIterator(var Iterator: IIterator): ErrCode stdcall;
    function CreateEndIterator(var Iterator: IIterator): ErrCode stdcall;
  end;

  IDictObject = interface(IBaseObject)
  ['{90EAAC02-F875-510A-A730-8B792FCD4963}']
    function GetItem(Key: IBaseObject; var Value: IBaseObject): ErrCode stdcall;
    function SetItem(Key: IBaseObject; Value: IBaseObject): ErrCode stdcall;
    function RemoveItem(Key: IBaseObject; var Value: IBaseObject): ErrCode stdcall;
    function DeleteItem(Key: IBaseObject): ErrCode stdcall;
    function Clear: ErrCode stdcall;
    function GetCount(var Size: SizeT): ErrCode stdcall;
    function EnumKeys(var Keys: IListObject): ErrCode stdcall;
    function EnumValues(var Values: IListObject): ErrCode stdcall;
  end;

  IConvertible = interface(IBaseObject)
  ['{A52D39FE-9118-509F-8923-DB4A163F7BA1}']
    function ToFloat(var Val: Double): ErrCode stdcall;
    function ToInt(var Val: Int64): ErrCode stdcall;
    function ToBool(var Val: Boolean): ErrCode stdcall;
  end;

  ICoreType = interface(IBaseObject)
  ['{72E0D318-84DD-589F-AA58-D570B81CD77D}']
    function GetCoreType(var CoreType: NativeInt): ErrCode stdcall;
  end;

  IFreezable = interface(IBaseObject)
  ['{991DD442-EE18-5815-89E0-B0AF020A16E0}']
    function Freeze(): ErrCode stdcall;
    function IsFrozen(var IsFrozen: Boolean): ErrCode stdcall;
  end;

  ISerializable = interface(IBaseObject)
  ['{F2A26E1A-0735-5758-88E7-F41BCB9E2EDC}']
    function Serialize(Serializer: ISerializer): ErrCode stdcall;
    function GetSerializeId(const Id: PPAnsiChar): ErrCode stdcall;
  end;

  ISerializer = interface(IBaseObject)
  ['{2A74B851-D3DC-5C19-8D36-716535B81BC7}']
    function StartTaggedObject(Obj: ISerializable): ErrCode stdcall;

    function StartObject(): ErrCode stdcall;
    function EndObject(): ErrCode stdcall;
    function StartList(): ErrCode stdcall;
    function EndList(): ErrCode stdcall;

    function ToString(var Serialized: IString): ErrCode stdcall;

    function Key(const Str: PAnsiChar): ErrCode  stdcall;
    function KeyStr(Name: IString): ErrCode stdcall;
    function KeyRaw(const Str: PAnsiChar; Len: SizeT): ErrCode stdcall;
    function WriteInt(Int: Int64): ErrCode stdcall;
    function WriteBool(Bool: Boolean): ErrCode stdcall;
    function WriteFloat(Real: Double): ErrCode stdcall;
    function WriteString(const Str: PAnsiChar; Len: SizeT): ErrCode stdcall;
    function WriteNull(): ErrCode stdcall;

    function Reset(): ErrCode stdcall;
    function IsComplete(var Complete: Boolean): ErrCode stdcall;
  end;

  ISerializedList = interface(IBaseObject)
  ['{0FAA7F66-1FE8-55E7-AAED-F348AA1AE8D3}']
    function ReadSerializedObject(var PlainObj: ISerializedObject): ErrCode stdcall;
    function ReadSerializedList(var List: ISerializedList): ErrCode stdcall;
    function ReadList(var List: IListObject): ErrCode stdcall;
    function ReadObject(var Obj: IBaseObject): ErrCode stdcall;
    function ReadString(var Str: IString): ErrCode stdcall;
    function ReadBool(var Bool: Boolean): ErrCode stdcall;
    function ReadFloat(var Real: Double): ErrCode stdcall;
    function ReadInt(var Int: Int64): ErrCode stdcall;
    function GetCount(var Size: SizeT): ErrCode stdcall;
  end;

  ISerializedObject = interface(IBaseObject)
  ['{500448A1-F784-5542-9170-202E2F002A19}']
    function ReadSerializedObject(Key: IString; var PlainObj: ISerializedObject): ErrCode stdcall;
    function ReadSerializedList(Key: IString; var List: ISerializedList): ErrCode stdcall;
    function ReadList(Key: IString; var List: IListObject): ErrCode stdcall;
    function ReadObject(Key: IString; var Obj: IBaseObject): ErrCode stdcall;
    function ReadString(Key: IString; var Str: IString): ErrCode stdcall;
    function ReadBool(Key: IString; var Bool: Boolean): ErrCode stdcall;
    function ReadFloat(Key: IString; var Real: Double): ErrCode stdcall;
    function ReadInt(Key: IString; var Int: Int64): ErrCode stdcall;
    function HasKey(Key: IString; var HasKey: Boolean): ErrCode stdcall;
    function GetKeys(var List: IListObject): ErrCode stdcall;
  end;

  IDeserializer = interface(IBaseObject)
  ['{F2E68289-2134-53E5-9840-28D25D5FBDAA}']
    function Deserialize(Serialized: IString; var Obj: IBaseObject): ErrCode stdcall;
  end;

function DaqGetTrackedObjectCount: Nativeuint; cdecl;
procedure DaqPrintTrackedObjects; cdecl;
procedure DaqFreeMemory(Ptr: Pointer); cdecl;
function daqAllocateMemory(Size: SizeT): Pointer; cdecl;
function DaqRegisterSerializerFactory(Id: PAnsiChar; Factory: TDSRTDeserializerFactory): ErrCode; cdecl;
function DaqUnregisterSerializerFactory(Id: PAnsiChar): ErrCode; cdecl;
function DaqGetSerializerFactory(Id: PAnsiChar; var Factory: TDSRTDeserializerFactory): ErrCode; cdecl;

function CreateBaseObject(out Obj: IBaseObject): ErrCode; cdecl;
function CreateBoolean(out Obj: IBoolean; const Value: Boolean): ErrCode; cdecl;
function CreateFloat(out Obj: IFloat; const Value: Double): ErrCode; cdecl;
function CreateInteger(out Obj: IInteger; const Value: Int64): ErrCode; cdecl;
function CreateString(out Obj: IString; const Value: PAnsiChar): ErrCode; cdecl;
function CreateStringFromDelphiString(const Value: string): IString; overload;
function CreateStringFromDelphiString(out Obj: IString; const Value: string): ErrCode; overload;

function CreateDict(out Obj: IDictObject): ErrCode; cdecl;
function CreateList(out Obj: IListObject): ErrCode; cdecl;
function CreateJsonSerializer(out Obj: ISerializer; Pretty: Boolean = False): ErrCode; cdecl;
function CreateJsonDeserializer(out Obj: IDeserializer): ErrCode; cdecl;

function BaseObjectToFloat(Obj: IBaseObject): Double;
function BaseObjectToInt(Obj: IBaseObject): Int64;
function BaseObjectToBool(Obj: IBaseObject): Boolean;

function GetCoreType(Obj: IBaseObject): TCoreType;
function GetFreezableInterface(Obj: IBaseObject): IFreezable;
function GetSerializableInterface(Obj: IBaseObject): ISerializable;

implementation

uses
  Dsrt.Core.CoreTypesConfig, TypInfo;

function DaqGetTrackedObjectCount: Nativeuint; external DSCoreTypesDLL name 'daqGetTrackedObjectCount';
procedure DaqPrintTrackedObjects; external DSCoreTypesDLL name 'daqPrintTrackedObjects';
procedure DaqFreeMemory; external DSCoreTypesDLL name 'daqFreeMemory';
function daqAllocateMemory(Size: SizeT): Pointer; external DSCoreTypesDLL name 'daqAllocateMemory';
function DaqRegisterSerializerFactory(Id: PAnsiChar; Factory: TDSRTDeserializerFactory): ErrCode; external DSCoreTypesDLL name 'daqRegisterSerializerFactory';
function DaqUnregisterSerializerFactory(Id: PAnsiChar): ErrCode; external DSCoreTypesDLL name 'daqUnregisterSerializerFactory';
function DaqGetSerializerFactory(Id: PAnsiChar; var Factory: TDSRTDeserializerFactory): ErrCode; external DSCoreTypesDLL name 'daqGetSerializerFactory';

function CreateBaseObject; external DSCoreTypesDLL name 'createBaseObject';
function CreateBoolean; external DSCoreTypesDLL name 'createBoolean';
function CreateFloat; external DSCoreTypesDLL name 'createFloat';
function CreateInteger; external DSCoreTypesDLL name 'createInteger';
function CreateString; external DSCoreTypesDLL name 'createString';
function CreateDict; external DSCoreTypesDLL name 'createDict';
function CreateList; external DSCoreTypesDLL name 'createList';
function CreateJsonSerializer; external DSCoreTypesDLL name 'createJsonSerializer';
function CreateJsonDeserializer; external DSCoreTypesDLL name 'createJsonDeserializer';

function CreateStringFromDelphiString(const Value: string): IString;
var
  Str: IString;
  Res: ErrCode;
begin
  Res := CreateString(Str, PAnsiChar(UTF8String(Value)));
  CheckError(Res);
  Result := Str;
end;

function CreateStringFromDelphiString(out Obj: IString; const Value: string): ErrCode; overload;
begin
  Result := CreateString(Obj, PAnsiChar(UTF8String(Value)));
end;

function BaseObjectToFloat(Obj: IBaseObject): Double;
var
  Conv: IConvertible;
  Res: ErrCode;
begin
  Res := Obj.QueryInterface(GetTypeData(TypeInfo(IConvertible))^.Guid, Pointer(Conv));
  CheckError(Res);

  Res := Conv.ToFloat(Result);
  CheckError(Res);
end;

function BaseObjectToInt(Obj: IBaseObject): Int64;
var
  Conv: IConvertible;
  Res: ErrCode;
begin
  Res := Obj.QueryInterface(GetTypeData(TypeInfo(IConvertible))^.Guid, Pointer(Conv));
  CheckError(Res);

  Res := Conv.ToInt(Result);
  CheckError(Res);
end;

function BaseObjectToBool(Obj: IBaseObject): Boolean;
var
  Conv: IConvertible;
  Res: ErrCode;
begin
  Res := Obj.QueryInterface(GetTypeData(TypeInfo(IConvertible))^.Guid, Pointer(Conv));
  CheckError(Res);

  Res := Conv.ToBool(Result);
  CheckError(Res);
end;

function GetCoreType(Obj: IBaseObject): TCoreType;
var
  Res: ErrCode;
  CT: ICoreType;
  CoreTypeRaw: NativeInt;

begin
  Res := Obj.QueryInterface(GetTypeData(TypeInfo(ICoreType))^.Guid, Pointer(CT));
  CheckError(Res);

  Res := CT.GetCoreType(CoreTypeRaw);
  Result := TCoreType(CoreTypeRaw);
  CheckError(Res);
end;

function GetFreezableInterface(Obj: IBaseObject): IFreezable;
var
  Res: ErrCode;
begin
  Res := Obj.QueryInterface(GetTypeData(TypeInfo(IFreezable))^.Guid, Pointer(Result));
  CheckError(Res);
end;

function GetSerializableInterface(Obj: IBaseObject): ISerializable;
var
  Res: ErrCode;
begin
  Res := Obj.QueryInterface(GetTypeData(TypeInfo(ISerializable))^.Guid, Pointer(Result));
  CheckError(Res);
end;

end.
