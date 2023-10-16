unit OpenDAQ.Iterator;

interface
uses
  OpenDAQ.CoreTypes,
  OpenDAQ.ObjectPtr;

type
  IIteratorPtr = interface(IObjectPtr<IITerator>)
  ['{16A3CD66-7DD6-473E-B692-2E66EE1FD825}']
    function MoveNext() : Boolean;
    function GetCurrentPtr() : IObjectPtr;
  end;

  IIteratorPtr<T : IBaseObject> = interface(IIteratorPtr)
  ['{8CDAD3A0-5D36-452F-A895-84C29E805638}']
    function GetCurrent() : T;
  end;

  TIteratorPtr<T : IBaseObject> = class(TObjectPtr<IIterator>, IIteratorPtr<T>, IIterator)
  public
    constructor Create(Obj : IBaseObject); overload; override;
    constructor Create(Obj : IIterator); overload;

    function MoveNext() : Boolean;
    function GetCurrent() : T; overload;
    function GetCurrentPtr() : IObjectPtr; overload;

  private
    function IIterator.MoveNext = Interface_MoveNext;
    function IIterator.GetCurrent = Interface_GetCurrent;

    function Interface_MoveNext() : ErrCode; stdcall;
    function Interface_GetCurrent(out Obj : IBaseObject) : ErrCode; stdcall;
  end;
  
implementation
uses  
  OpenDAQ.CoreTypes.Errors,
  OpenDAQ.Exceptions,
  OpenDAQ.SmartPtrRegistry;

{ TIteratorPtr<T> }

constructor TIteratorPtr<T>.Create(Obj: IIterator);
begin
  inherited Create(Obj);
end;

constructor TIteratorPtr<T>.Create(Obj: IBaseObject);
begin
  inherited Create(Obj);
end;

function TIteratorPtr<T>.GetCurrent(): T;
var
  Err : ErrCode;
  Current : IBaseObject;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.GetCurrent(Current);
  CheckRtErrorInfo(Err);

  Result := T(Current);
end;

function TIteratorPtr<T>.GetCurrentPtr(): IObjectPtr;
var
  Err : ErrCode;
  Current : IBaseObject;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.GetCurrent(Current);
  CheckRtErrorInfo(Err);

  Result := TObjectPtr<IBaseObject>.Create(Current);
end;

function TIteratorPtr<T>.MoveNext(): Boolean;
var
  Err : ErrCode;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.MoveNext();

  if (Err = OPENDAQ_NO_MORE_ITEMS) then
    Exit(False);

  CheckRtErrorInfo(Err);

  Result := True;
end;

function TIteratorPtr<T>.Interface_GetCurrent(out Obj: IBaseObject): ErrCode;
begin
  Result := FObject.GetCurrent(Obj);
end;

function TIteratorPtr<T>.Interface_MoveNext(): ErrCode;
begin
  Result := FObject.MoveNext();
end;

initialization
  TSmartPtrRegistry.RegisterPtr(IIterator, IIteratorPtr, TIteratorPtr<IBaseObject>);

finalization
  TSmartPtrRegistry.UnregisterPtr(IIterator);

end.
