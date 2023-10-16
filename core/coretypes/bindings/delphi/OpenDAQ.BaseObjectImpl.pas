unit OpenDAQ.BaseObjectImpl;

interface

uses
  OpenDAQ.CoreTypes;

type

  TBaseObjectImpl = class(TInterfacedObject, IBaseObject)
  protected
    function BorrowInterface(const IID: TGUID; out Obj): HResult; stdcall;
    procedure Dispose; stdcall;

    function IBaseObject.GetHashCodeEx = GetHashCode_BaseObject;
    function GetHashCode_BaseObject(out HashCode: SizeT): ErrCode; stdcall;
	
    function IBaseObject.EqualsObject = Equals_BaseObject;
    function Equals_BaseObject(Other: IBaseObject; out Equals: Boolean): ErrCode; stdcall;
	
    function ToCharPtr(Str: PPAnsiChar): ErrCode; stdcall;
  end;

function CreateDBaseObject(out BaseObject: IBaseObject): ErrCode; cdecl;

implementation

uses
  OpenDAQ.CoreTypes.Errors, ActiveX;

{ TBaseObjectImpl }

function TBaseObjectImpl.BorrowInterface(const IID: TGUID; out Obj): HResult;
begin
  Result := QueryInterface(IID, Obj);
  if Succeeded(Result) then
    IInterface(Obj)._Release;
end;

procedure TBaseObjectImpl.Dispose;
begin
end;

function TBaseObjectImpl.Equals_BaseObject(Other: IBaseObject; out Equals: Boolean): ErrCode;
var
  ThisObj: IBaseObject;
begin
  Assert(QueryInterface(IBaseObject, ThisObj) = S_OK);
  if Other = ThisObj then
    Equals := True
  else
    Equals := False;

  Result := OPENDAQ_SUCCESS;
end;

function TBaseObjectImpl.GetHashCode_BaseObject(out HashCode: SizeT): ErrCode;
begin
  HashCode := SizeT(Self);
  Result := OPENDAQ_SUCCESS;
end;

function TBaseObjectImpl.ToCharPtr(Str: PPAnsiChar): ErrCode;
const
  DStr: UTF8String = 'BaseObject-Delphi'#0;
var
  RStr: PAnsiChar;
begin
  if not Assigned(Str) then
     Exit(OPENDAQ_ERR_INVALIDPARAMETER);

  RStr := daqAllocateMemory(Length(DStr) + 1);
  Move(DStr[1], RStr^, Length(DStr));
  Str^ := RStr;

  Result := OPENDAQ_SUCCESS;
end;

function CreateDBaseObject(out BaseObject: IBaseObject): ErrCode; cdecl;
begin
  BaseObject := TBaseObjectImpl.Create;
  Result := OPENDAQ_SUCCESS;
end;

end.
