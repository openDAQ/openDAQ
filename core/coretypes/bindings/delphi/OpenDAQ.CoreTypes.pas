unit OpenDAQ.CoreTypes;

interface

uses
  OpenDAQ.CoreTypes.Errors
  {$IFDEF DW}
  , Dewesoft_TLB
  {$ENDIF}
  ;

type
  SizeT = Nativeint;
  PSizeT = ^SizeT;
  RtNativeHandle = NativeUInt;
  ErrCode = Cardinal;
  CoreType = Integer;
  PBoolean = ^Boolean;
  RtInt = Int64;
  RtUInt = UInt64;
  PRtInt = ^RtInt;
  RtFloat = Double;
  PRtFloat = ^RtFloat;
  TConstChar = PAnsiChar;
  TConstCharPtr = PAnsiChar;

  {$MINENUMSIZE 4}

  {$IFDEF DW}
    TCoreType = TRTCoreType;

    const
      ctBool = Dewesoft_TLB.ctBool;
      ctInt = Dewesoft_TLB.ctInt;
      ctFloat = Dewesoft_TLB.ctFloat;
      ctString = Dewesoft_TLB.ctString;
      ctList = Dewesoft_TLB.ctList;
      ctDict = Dewesoft_TLB.ctDict;
      ctRatio = Dewesoft_TLB.ctRatio;
      ctProc = Dewesoft_TLB.ctProc;
      ctObject = Dewesoft_TLB.ctObject;
      ctBinaryData = Dewesoft_TLB.ctBinaryData;
      ctFunc = Dewesoft_TLB.ctFunc;
      ctComplexNumber = ctFunc + 1;  ///< Complex number (real, imaginary)
      ctStruct = ctFunc + 2;         ///< Constant structure with dictionary of fields and types
      ctUndefined = Dewesoft_TLB.ctUndefined;

    type
  {$ELSE}
    TCoreType = (ctBool = 0, ctInt, ctFloat, ctString, ctList, ctDict, ctRatio, ctProc, ctObject, ctBinaryData, ctFunc, ctComplexNumber, ctStruct, ctUndefined = $FFFF);
  {$ENDIF}

  TConfigurationMode = (cmNone=$0, cmStatic=$1, cmDynamic=$2, cmBoth);

  ISerializable = interface;
  ISerializer = interface;
  ISerializedObject = interface;
  IBaseObject = interface;

  TDSRTDeserializerFactory = function(SerializedObject: ISerializedObject; Context: IBaseObject; out Obj: IBaseObject): Errcode; cdecl;

  IBaseObject = interface(IUnknown)
  ['{9C911F6D-1664-5AA2-97BD-90FE3143E881}']
    function BorrowInterface(const IID: TGUID; out Obj): HResult; stdcall;
    procedure Dispose; stdcall;

    function GetHashCodeEx(out HashCode: SizeT): ErrCode; stdcall;
    function EqualsObject(Other: IBaseObject; out Equal: Boolean): ErrCode; stdcall;
    function ToCharPtr(Str: PPAnsiChar): ErrCode; stdcall;
  end;

  IIterator = interface(IBaseObject)
  ['{F3B87158-F4CD-5890-9476-3C0E315C56D9}']
    function MoveNext: ErrCode stdcall;
    function GetCurrent(out BaseObject: IBaseObject): ErrCode stdcall;
  end;

  IIterable = interface(IBaseObject)
  ['{EC09F2E5-614D-5780-81CB-ECE8ECB2655B}']
    function CreateStartIterator(out Value: IIterator): ErrCode stdcall;
    function CreateEndIterator(out Value: IIterator): ErrCode stdcall;
  end;

  IBoolean = interface(IBaseObject)
  ['{9F20E31A-D0FB-5679-A188-4942B3FED6E2}']
    function GetValue(out Value: Boolean): ErrCode; stdcall;
    function EqualsValue(const Value: Boolean; out Equal: Boolean): ErrCode stdcall;
  end;

  IFloat = interface(IBaseObject)
  ['{D1E14646-B7B0-5D3D-9553-BE6F1CD4F0D8}']
    function GetValue(out Value: RtFloat): ErrCode; stdcall;
    function EqualsValue(const Value: RtFloat; out Equal: Boolean): ErrCode stdcall;
  end;

  IInteger = interface(IBaseObject)
  ['{B5C52F78-45F9-5C54-9BC1-CA65A46472CB}']
    function GetValue(out Value: RtInt): ErrCode; stdcall;
    function EqualsValue(const Value: RtInt; out Equal: Boolean): ErrCode stdcall;
  end;

  IString = interface(IBaseObject)
  ['{D2ED1120-F7FF-556F-A98D-3F3EDF1A3874}']
    function GetCharPtr(Value: PPAnsiChar): ErrCode stdcall;
    function GetLength(out Size: SizeT): ErrCode stdcall;
  end;

  IInspectable = interface(IBaseObject)
  ['{9869DF21-C7B3-5E0E-8E4B-66DB6A7265A8}']
    function getInterfaceIds(var Count: SizeT; IIDs: PGUID): ErrCode; stdcall;
    function getRuntimeClassName(out ImplementationName: IString): ErrCode; stdcall;
  end;

  IListObject = interface(IBaseObject)
  ['{E7866BCC-0563-5504-B61B-A8116B614D8F}']
    function GetItemAt(Index: SizeT; out BaseObject: IBaseObject): ErrCode stdcall;
    function GetCount(out Size: SizeT): ErrCode stdcall;
    function SetItemAt(Index: SizeT; BaseObject: IBaseObject): ErrCode stdcall;
    function PushBack(BaseObject: IBaseObject): ErrCode stdcall;
    function PushFront(BaseObject: IBaseObject): ErrCode stdcall;
    function MoveBack(BaseObject: IBaseObject): ErrCode stdcall;
    function MoveFront(BaseObject: IBaseObject): ErrCode stdcall;
    function PopBack(out BaseObject: IBaseObject): ErrCode stdcall;
    function PopFront(out BaseObject: IBaseObject): ErrCode stdcall;
    function InsertAt(Index: SizeT; BaseObject: IBaseObject): ErrCode stdcall;
    function RemoveAt(Index: SizeT; out BaseObject: IBaseObject): ErrCode stdcall;
    function DeleteAt(Index: SizeT): ErrCode stdcall;
    function Clear: ErrCode stdcall;

    function CreateStartIterator(out Iterator: IIterator): ErrCode stdcall;
    function CreateEndIterator(out Iterator: IIterator): ErrCode stdcall;
  end;

  IList = IListObject;

  IDictObject = interface(IBaseObject)
  ['{E3DE60DA-0366-5DA5-8334-F9DCADFF5AD0}']
    function GetItem(Key: IBaseObject; out Value: IBaseObject): ErrCode stdcall;
    function SetItem(Key: IBaseObject; Value: IBaseObject): ErrCode stdcall;
    function RemoveItem(Key: IBaseObject; out Value: IBaseObject): ErrCode stdcall;
    function DeleteItem(Key: IBaseObject): ErrCode stdcall;
    function Clear: ErrCode stdcall;
    function GetCount(out Size: SizeT): ErrCode stdcall;
    function HasKey(Key: IBaseObject; var HasKey: Boolean): ErrCode stdcall;
    function GetKeyList(out Keys: IListObject): ErrCode stdcall;
    function GetValueList(out Values: IListObject): ErrCode stdcall;
    function GetKeys(out Keys: IIterable): ErrCode stdcall;
    function GetValues(out Values: IIterable): ErrCode stdcall;
  end;

  IConvertible = interface(IBaseObject)
  ['{D984FD0F-7980-5E7B-8EC1-75C507F302FE}']
    function ToFloat(out Val: RtFloat): ErrCode stdcall;
    function ToInt(out Val: RtInt): ErrCode stdcall;
    function ToBool(out Val: Boolean): ErrCode stdcall;
  end;

  ICoreType = interface(IBaseObject)
  ['{0562D045-C94E-5E6D-8360-2CFC9DB76A04}']
    function GetCoreType(out CoreType: TCoreType): ErrCode stdcall;
  end;

  IFreezable = interface(IBaseObject)
  ['{06F0D04D-3CA7-5E0F-A6CB-2459608C6519}']
    function Freeze(): ErrCode stdcall;
    function IsFrozen(out IsFrozen: Boolean): ErrCode stdcall;
  end;

  ISerializable = interface(IBaseObject)
  ['{831915F2-C42F-5520-A420-56524D2AC552}']
    function Serialize(Serializer: ISerializer): ErrCode stdcall;
    function GetSerializeId(const Id: PPAnsiChar): ErrCode stdcall;
  end;

  ISerializer = interface(IBaseObject)
  ['{4230318E-85D5-5BB6-80C8-10CC47B7A3D2}']
    function StartTaggedObject(Obj: ISerializable): ErrCode stdcall;

    function StartObject(): ErrCode stdcall;
    function EndObject(): ErrCode stdcall;
    function StartList(): ErrCode stdcall;
    function EndList(): ErrCode stdcall;

    function GetOutput(out Serialized: IString): ErrCode stdcall;

    function Key(const Str: PAnsiChar): ErrCode  stdcall;
    function KeyStr(Name: IString): ErrCode stdcall;
    function KeyRaw(const Str: PAnsiChar; Len: SizeT): ErrCode stdcall;
    function WriteInt(Int: RtInt): ErrCode stdcall;
    function WriteBool(Bool: Boolean): ErrCode stdcall;
    function WriteFloat(Real: RtFloat): ErrCode stdcall;
    function WriteString(const Str: PAnsiChar; Len: SizeT): ErrCode stdcall;
    function WriteNull(): ErrCode stdcall;

    function Reset(): ErrCode stdcall;
    function IsComplete(out Complete: Boolean): ErrCode stdcall;
  end;

  ISerializedList = interface(IBaseObject)
  ['{A9E1FD59-8AD5-5F3C-B4F8-2A9CDE66E598}']
    function ReadSerializedObject(out PlainObj: ISerializedObject): ErrCode stdcall;
    function ReadSerializedList(out List: ISerializedList): ErrCode stdcall;
    function ReadList(Context: IBaseObject; out List: IListObject): ErrCode stdcall;
    function ReadObject(Context: IBaseObject; out Obj: IBaseObject): ErrCode stdcall;
    function ReadString(out Str: IString): ErrCode stdcall;
    function ReadBool(out Bool: Boolean): ErrCode stdcall;
    function ReadFloat(out Real: RtFloat): ErrCode stdcall;
    function ReadInt(out Int: RtInt): ErrCode stdcall;
    function GetCount(out Size: SizeT): ErrCode stdcall;
    function GetCurrentItemType(out AType: TCoreType): ErrCode stdcall;
  end;

  ISerializedObject = interface(IBaseObject)
  ['{EC052FCE-7ADC-5335-9929-66731EA35698}']
    function ReadSerializedObject(Key: IString; out PlainObj: ISerializedObject): ErrCode stdcall;
    function ReadSerializedList(Key: IString; out List: ISerializedList): ErrCode stdcall;
    function ReadList(Key: IString; Context: IBaseObject; out List: IListObject): ErrCode stdcall;
    function ReadObject(Key: IString; Context: IBaseObject; out Obj: IBaseObject): ErrCode stdcall;
    function ReadString(Key: IString; out Str: IString): ErrCode stdcall;
    function ReadBool(Key: IString; out Bool: Boolean): ErrCode stdcall;
    function ReadFloat(Key: IString; out Real: RtFloat): ErrCode stdcall;
    function ReadInt(Key: IString; out Int: RtInt): ErrCode stdcall;
    function HasKey(Key: IString; out HasKey: Boolean): ErrCode stdcall;
    function GetKeys(out List: IListObject): ErrCode stdcall;
  end;

  IUpdatable = interface(IBaseObject)
  ['{94BF8B0E-2868-51A2-8773-CBB98A4DD1BE}']
    function Update(Mode: TConfigurationMode; Update: ISerializedObject): ErrCode; stdcall;
  end;

  IDeserializer = interface(IBaseObject)
  ['{66DEEEF9-2B0D-5A49-A050-2820C4738AE7}']
    function Deserialize(Serialized: IString; Context: IBaseObject; out Obj: IBaseObject): ErrCode stdcall;
    function Update(Updatable: IUpdatable; Mode: TConfigurationMode; Serialized: IString): ErrCode stdcall;
  end;

  IFunction = interface(IBaseObject)
  ['{2EEACD91-0883-5FC8-8EB8-4F4C80CD8131}']
    function Call(Params : IBaseObject; out Result : IBaseObject) : ErrCode; stdcall;
  end;
  
  IProcedure = interface(IBaseObject)
  ['{36247E6D-6BDD-5964-857D-0FD296EEB5C3}']
    function Execute(Params : IBaseObject) : ErrCode; stdcall;
  end;

  IBinaryData = interface(IBaseObject)
  ['{778DCE96-1ECD-59C0-B066-FD47BAF07789}']
    function GetAddress(var Address: Pointer): ErrCode stdcall;
    function GetSize(var Size: SizeT): ErrCode stdcall;
  end;

  IRatio = interface(IBaseObject)
  ['{08D28C13-55A6-5FE5-A0F0-19A3F8707C15}']
    function GetNumerator(out Numerator : RtInt) : ErrCode; stdcall;
    function GetDenominator(out Denominator : RtInt) : ErrCode; stdcall;
  end;

  IErrorInfo = interface(IBaseObject)
  ['{483B3446-8F45-53CE-B4EE-EC2B03CF6A4C}']
    function SetMessage(Msg: IString): ErrCode; stdcall;
    function GetMessage(out Msg: IString): ErrCode; stdcall;
    function SetSource(Source: IString): ErrCode; stdcall;
    function GetSource(out Source: IString): ErrCode; stdcall;
  end;

  ISmartPtr = interface
  ['{A66C7CE0-A304-458A-99BB-5476E9C59132}']
    function GetObject() : IBaseObject;
    function IsAssigned() : Boolean;
  end;

  TSmartPtr = class abstract(TInterfacedObject, ISmartPtr)
  public
    constructor Create(); overload; virtual;
    constructor Create(Obj : IBaseObject); overload; virtual;

    function GetObject() : IBaseObject; virtual; abstract;
    function IsAssigned() : Boolean; virtual; abstract;
  end;

  IObjectPtr<T> = interface(ISmartPtr)
  ['{31F23BE7-1644-4F35-B914-D4681AD0089A}']
    function GetHashCodeEx(): SizeT;
    function EqualsObject(Other: IBaseObject): Boolean; overload;
    function EqualsObject(Other: ISmartPtr): Boolean; overload;

    function ToString(): string;

    function GetCoreType() : TCoreType;
    function GetInterface() : T;
  end;

  IFreezablePtr = interface(IObjectPtr<IFreezable>)
  ['{1F077BE4-8E27-4E70-8D71-FBA43769A556}']
    procedure Freeze();
    function IsFrozen(): Boolean;
  end;

  ISerializablePtr = interface(IObjectPtr<ISerializable>)
  ['{59907C80-3B5A-427F-A6BB-A9B6B8089A83}']
    procedure Serialize(Serializer: ISerializer);
    function GetSerializeId(): PAnsiChar;
  end;

  PointerTo<T> = record
  type
    PT = ^T;
  end;

  SmartPtrClass = class of TSmartPtr;

function DaqGetTrackedObjectCount: NativeUInt; cdecl;
procedure DaqPrintTrackedObjects; cdecl;
procedure DaqFreeMemory(Ptr: Pointer); cdecl;
function daqAllocateMemory(Size: SizeT): Pointer; cdecl;

function DaqRegisterSerializerFactory(Id: string; Factory: TDSRTDeserializerFactory): Boolean; overload;
function DaqRegisterSerializerFactory(Id: PAnsiChar; Factory: TDSRTDeserializerFactory): ErrCode; cdecl; overload;

function DaqUnregisterSerializerFactory(Id: PAnsiChar): ErrCode; cdecl; overload;
function DaqUnregisterSerializerFactory(Id: string): Boolean; overload;

function DaqGetSerializerFactory(Id: PAnsiChar; out Factory: TDSRTDeserializerFactory): ErrCode; cdecl; overload;
function DaqGetSerializerFactory(Id: string): TDSRTDeserializerFactory; overload;

function TryGetRtSerializeFactory(Id : string; out Factory : TDSRTDeserializerFactory) : Boolean;
function IsRtSerializeFactoryRegistered(Id : string) : Boolean;

function CreateBaseObject(out Obj: IBaseObject): ErrCode; cdecl;
function CreateBoolean(out Obj: IBoolean; const Value: Boolean): ErrCode; cdecl;
function CreateFloat(out Obj: IFloat; const Value: RtFloat): ErrCode; cdecl;
function CreateInteger(out Obj: IInteger; const Value: RtInt): ErrCode; cdecl;
function CreateString(out Obj: IString; const Value: PAnsiChar): ErrCode; cdecl;
function CreateStringFromDelphiString(const Value: string): IString; overload;
function CreateStringFromDelphiString(out Obj: IString; const Value: string): ErrCode; overload;
function CreateBinaryData(out Obj: IBinaryData; const Size: SizeT): ErrCode; cdecl;
function CreateRatio(out Obj: IRatio; const Numerator: RtInt; const Denominator: RtInt): ErrCode; cdecl;

function CreateDict(out Obj: IDictObject): ErrCode; cdecl;
function CreateList(out Obj: IListObject): ErrCode; cdecl;
function CreateJsonSerializer(out Obj: ISerializer; Pretty: Boolean = False): ErrCode; cdecl;
function CreateJsonDeserializer(out Obj: IDeserializer): ErrCode; cdecl;

procedure DaqSetErrorInfo(ErrorInfo: IErrorInfo); cdecl;
procedure DaqGetErrorInfo(out ErrorInfo: IErrorInfo); cdecl;
procedure DaqClearErrorInfo(); cdecl;

function BaseObjectToFloat(Obj: IBaseObject): RtFloat; overload;
function BaseObjectToFloat(Obj: ISmartPtr): RtFloat; overload;

function BaseObjectToInt(Obj: IBaseObject): RtInt; overload;
function BaseObjectToInt(Obj: ISmartPtr): RtInt; overload;

function BaseObjectToBool(Obj: IBaseObject): Boolean; overload;
function BaseObjectToBool(Obj: ISmartPtr): Boolean; overload;

function BaseObjectToString(Obj: IBaseObject): string;

function GetCoreType(Obj: IBaseObject): TCoreType; overload;
function GetCoreType(Obj: ISmartPtr): TCoreType; overload;
function GetFreezableInterface(Obj: IBaseObject): IFreezable; overload;
function GetFreezableInterface(Obj: ISmartPtr): IFreezablePtr; overload;
function GetSerializableInterface(Obj: IBaseObject): ISerializable; overload;
function GetSerializableInterface(Obj: ISmartPtr): ISerializablePtr; overload;

function RtToString(Value : IString) : string;

function DaqBoxValue(Value: Boolean): IBoolean; overload;
function DaqBoxValue(Value: RtInt): IInteger; overload;
function DaqBoxValue(Value: RtFloat): IFloat; overload;

implementation

uses
  OpenDAQ.CoreTypes.Config,
  OpenDAQ.Exceptions,
  OpenDAQ.Freezable,
  OpenDAQ.Serializable,
  TypInfo;

function DaqGetTrackedObjectCount: NativeUInt; external DSCoreTypesDLL name 'daqGetTrackedObjectCount';
procedure DaqPrintTrackedObjects; external DSCoreTypesDLL name 'daqPrintTrackedObjects';
procedure DaqFreeMemory; external DSCoreTypesDLL name 'daqFreeMemory';
function daqAllocateMemory(Size: SizeT): Pointer; external DSCoreTypesDLL name 'daqAllocateMemory';
function DaqRegisterSerializerFactory(Id: PAnsiChar; Factory: TDSRTDeserializerFactory): ErrCode; external DSCoreTypesDLL name 'daqRegisterSerializerFactory';
function DaqUnregisterSerializerFactory(Id: PAnsiChar): ErrCode; external DSCoreTypesDLL name 'daqUnregisterSerializerFactory';
function DaqGetSerializerFactory(Id: PAnsiChar; out Factory: TDSRTDeserializerFactory): ErrCode; external DSCoreTypesDLL name 'daqGetSerializerFactory';

function CreateBaseObject; external DSCoreTypesDLL name 'createBaseObject';
function CreateBoolean; external DSCoreTypesDLL name 'createBoolean';
function CreateFloat; external DSCoreTypesDLL name 'createFloat';
function CreateInteger; external DSCoreTypesDLL name 'createInteger';
function CreateString; external DSCoreTypesDLL name 'createString';
function CreateBinaryData; external DSCoreTypesDLL name 'createBinaryData';
function CreateRatio; external DSCoreTypesDLL name 'createRatio';
function CreateDict; external DSCoreTypesDLL name 'createDict';
function CreateList; external DSCoreTypesDLL name 'createList';
function CreateJsonSerializer; external DSCoreTypesDLL name 'createJsonSerializer';
function CreateJsonDeserializer; external DSCoreTypesDLL name 'createJsonDeserializer';

procedure DaqSetErrorInfo; external DSCoreTypesDLL name 'daqSetErrorInfo';
procedure DaqGetErrorInfo; external DSCoreTypesDLL name 'daqGetErrorInfo';
procedure DaqClearErrorInfo; external DSCoreTypesDLL name 'daqClearErrorInfo';

function DaqRegisterSerializerFactory(Id: string; Factory: TDSRTDeserializerFactory) : Boolean;
var
  Err : ErrCode;
begin
  Result := False;
  Err := DaqRegisterSerializerFactory(PAnsiChar(UTF8String(Id)), Factory);

  if Err = OPENDAQ_SUCCESS then
    Exit(True);

  if Err = OPENDAQ_ERR_FACTORY_NOT_REGISTERED then
    Exit(False);

  CheckRtErrorInfo(Err);
end;

function DaqGetSerializerFactory(Id: string): TDSRTDeserializerFactory;
var
  Err : ErrCode;
  Factory : TDSRTDeserializerFactory;
begin
  Err := DaqGetSerializerFactory(PAnsiChar(UTF8String(Id)), Factory);
  CheckRtErrorInfo(Err);

  Result := Factory;
end;

function IsRtSerializeFactoryRegistered(Id : string) : Boolean;
var
  Err : ErrCode;
  Factory : TDSRTDeserializerFactory;
begin
  Err := DaqGetSerializerFactory(PAnsiChar(UTF8String(Id)), Factory);
  Result := Err = OPENDAQ_SUCCESS;
end;

function TryGetRtSerializeFactory(Id : string; out Factory : TDSRTDeserializerFactory) : Boolean;
var
  Err : ErrCode;
begin
  Err := DaqGetSerializerFactory(PAnsiChar(UTF8String(Id)), Factory);
  Result := Err = OPENDAQ_SUCCESS;
end;

function DaqUnregisterSerializerFactory(Id: string) : Boolean;
var
  Err : ErrCode;
begin
  Result := False;
  Err := DaqUnregisterSerializerFactory(PAnsiChar(UTF8String(Id)));

  if Err = OPENDAQ_SUCCESS then
    Exit(True);

  if Err = OPENDAQ_ERR_FACTORY_NOT_REGISTERED then
    Exit(False);

  CheckRtErrorInfo(Err);
end;

function DaqBoxValue(Value: Boolean): IBoolean; overload;
var
  Err: ErrCode;
begin
  Err := CreateBoolean(Result, Value);
  CheckRtErrorInfo(Err);
end;

function DaqBoxValue(Value: RtInt): IInteger; overload;
var
  Err: ErrCode;
begin
  Err := CreateInteger(Result, Value);
  CheckRtErrorInfo(Err);
end;

function DaqBoxValue(Value: RtFloat): IFloat; overload;
var
  Err: ErrCode;
begin
  Err := CreateFloat(Result, Value);
  CheckRtErrorInfo(Err);
end;

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

function BaseObjectToFloat(Obj: IBaseObject): RtFloat;
var
  Conv: IConvertible;
  Res: ErrCode;
begin
  Res := Obj.QueryInterface(IConvertible, Pointer(Conv));
  CheckError(Res);

  Res := Conv.ToFloat(Result);
  CheckError(Res);
end;

function BaseObjectToFloat(Obj: ISmartPtr): RtFloat;
begin
  Result := BaseObjectToFloat(Obj.GetObject());
end;

function BaseObjectToInt(Obj: IBaseObject): RtInt;
var
  Conv: IConvertible;
  Res: ErrCode;
begin
  Res := Obj.QueryInterface(IConvertible, Pointer(Conv));
  CheckError(Res);

  Res := Conv.ToInt(Result);
  CheckError(Res);
end;

function BaseObjectToInt(Obj: ISmartPtr): RtInt;
begin
  Result := BaseObjectToInt(Obj.GetObject());
end;

function BaseObjectToBool(Obj: IBaseObject): Boolean;
var
  Conv: IConvertible;
  Res: ErrCode;
begin
  Res := Obj.QueryInterface(IConvertible, Pointer(Conv));
  CheckError(Res);

  Res := Conv.ToBool(Result);
  CheckError(Res);
end;

function BaseObjectToBool(Obj: ISmartPtr): Boolean;
begin
  Result := BaseObjectToBool(Obj.GetObject());
end;

function BaseObjectToString(Obj: IBaseObject): string;
var
  Ptr : PAnsiChar;
  Error : ErrCode;
begin
  try
    Error := Obj.ToCharPtr(@Ptr);
    CheckError(Error);

    Result := string(UTF8String(Ptr));
  finally
    DaqFreeMemory(Ptr);
  end;
end;

function GetCoreType(Obj: IBaseObject): TCoreType;
var
  Res: ErrCode;
  CT: ICoreType;
  CoreTypeRaw: TCoreType;
begin
  Res := Obj.QueryInterface(ICoreType, Pointer(CT));
  if Res = OPENDAQ_ERR_NOINTERFACE then
    Exit(ctObject);

  CheckError(Res);

  Res := CT.GetCoreType(CoreTypeRaw);
  Result := CoreTypeRaw;
  CheckError(Res);
end;

function GetCoreType(Obj: ISmartPtr): TCoreType;
begin
  Result := GetCoreType(Obj.GetObject());
end;

function GetFreezableInterface(Obj: IBaseObject): IFreezable;
var
  Res: ErrCode;
begin
  Res := Obj.QueryInterface(IFreezable, Pointer(Result));
  CheckError(Res);
end;

function GetFreezableInterface(Obj: ISmartPtr): IFreezablePtr;
begin
  Result := TFreezablePtr.Create(GetFreezableInterface(Obj.GetObject()));
end;

function GetSerializableInterface(Obj: IBaseObject): ISerializable;
var
  Res: ErrCode;
begin
  Res := Obj.QueryInterface(ISerializable, Pointer(Result));
  CheckError(Res);
end;

function GetSerializableInterface(Obj: ISmartPtr): ISerializablePtr;
begin
  Result := TSerializablePtr.Create(GetSerializableInterface(Obj.GetObject()));
end;

function RtToString(Value : IString) : string;
var
  Ptr : PAnsiChar;
  Error : ErrCode;
begin
  if not Assigned(Value) then
    CheckError(OPENDAQ_ERR_INVALIDPARAMETER);

  Error := value.GetCharPtr(@Ptr);
  CheckError(Error);

  Result := string(UTF8String(Ptr));
end;

{ TSmartPtr }

constructor TSmartPtr.Create(Obj: IBaseObject);
begin
  inherited Create();
end;

constructor TSmartPtr.Create();
begin
  inherited Create();
end;

end.
