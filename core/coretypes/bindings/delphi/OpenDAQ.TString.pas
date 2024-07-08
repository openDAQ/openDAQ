unit OpenDAQ.TString;

interface
uses
  OpenDAQ.CoreTypes,
  OpenDAQ.ObjectPtr;

type
  IStringPtr = interface(IObjectPtr<IString>)
  ['{EC4621A5-90B2-439E-AAB4-7AF27EE8C05F}']
    function ToString() : string;
    function ToStringOrEmpty() : string;
    function GetLength() : SizeT;
  end;

  TStringPtr = class(TObjectPtr<IString>, IString, IStringPtr)
  public
    constructor Create(Ptr : Pointer); overload;
    constructor Create(Str : string); overload;
    constructor Create(Obj : IString); overload;
    constructor Create(Obj : IBaseObject); overload; override;

    function ToString() : string; override;
    function ToStringOrEmpty() : string;
    function GetLength() : SizeT; overload;
  private
    function GetCharPtr(Value: PPAnsiChar): ErrCode; overload; stdcall;
    function GetLength(out Size: SizeT): ErrCode; overload; stdcall;
  end;

implementation
uses
  OpenDAQ.Exceptions,
  OpenDAQ.SmartPtrRegistry;

{ TStringPtr<T> }

constructor TStringPtr.Create(Ptr: Pointer);
var
  Str : IString;
  Err : ErrCode;
begin
  if Assigned(Ptr) then
    raise ERTInvalidParameterException.Create('The parameter must be nil to initialize as a nil string.');

  Err := CreateString(Str, Ptr);
  CheckRtErrorInfo(Err);

  Create(Str);
end;

constructor TStringPtr.Create(Obj: IString);
begin
  inherited Create(Obj);
end;

constructor TStringPtr.Create(Obj: IBaseObject);
begin
  inherited Create(Obj);
end;

constructor TStringPtr.Create(Str: string);
begin
  inherited Create(CreateStringFromDelphiString(Str));
end;

function TStringPtr.GetLength(): SizeT;
var
  Err : ErrCode;
  Length : SizeT;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.GetLength(Length);
  CheckRtErrorInfo(Err);

  Result := Length;
end;

function TStringPtr.ToString(): string;
var
  Err : ErrCode;
  Ptr : PAnsiChar;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.GetCharPtr(@Ptr);
  CheckRtErrorInfo(Err);

  Result := string(UTF8String(Ptr));
end;

function TStringPtr.ToStringOrEmpty(): string;
begin
  if not Assigned(FObject) then
    Result := ''
  else
    Result := ToString;
end;

function TStringPtr.GetCharPtr(Value: PPAnsiChar): ErrCode;
begin
  Result := FObject.GetCharPtr(Value);
end;

function TStringPtr.GetLength(out Size: SizeT): ErrCode;
begin
  Result := FObject.GetLength(Size);
end;

initialization
  TSmartPtrRegistry.RegisterPtr(IString, IStringPtr, TStringPtr);

finalization
  TSmartPtrRegistry.UnregisterPtr(IString);

end.
