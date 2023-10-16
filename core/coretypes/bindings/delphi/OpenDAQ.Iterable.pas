unit OpenDAQ.Iterable;

interface
uses
  OpenDAQ.CoreTypes,
  OpenDAQ.ObjectPtr,
  OpenDAQ.Iterator;

type

  IIterablePtr<T : IBaseObject> = interface(IObjectPtr<IIterable>)
  ['{00DF411A-E130-4985-9FF5-A2684D0B64B7}']
    function CreateStartIterator() : IIteratorPtr<T>;
    function CreateEndIterator() : IIteratorPtr<T>;

    function GetEnumerator: IEnumerator<T>;
  end;

  IIterablePtr = IIterablePtr<IBaseObject>;

  TRtCoreEnumerator = class(TInterfacedObject, IEnumerator)
  public
    constructor Create(Iterator: IIteratorPtr);

    function MoveNext: Boolean;
    procedure Reset;

    function GetCurrent: TObject;
  protected
    Iterator: IIteratorPtr;
  end;

  TRtCoreEnumerator<T: IBaseObject> = class(TRtCoreEnumerator, IEnumerator<T>)
  public
    constructor Create(Iterator: IIteratorPtr);

    function GetCurrent: T;
  end;

  TIterablePtr<T : IBaseObject> = class(TObjectPtr<IIterable>, IIterablePtr<T>, IIterable)
  public
    constructor Create(Obj : IBaseObject); overload; override;
    constructor Create(Obj : IIterable); overload;

    function CreateStartIterator() : IIteratorPtr<T>; overload;
    function CreateEndIterator() : IIteratorPtr<T>; overload;

    function GetEnumerator: IEnumerator<T>;
  private
    function IIterable.CreateStartIterator = Interface_CreateStartIterator;
    function IIterable.CreateEndIterator = Interface_CreateEndIterator;

    function Interface_CreateStartIterator(out Value: IIterator) : ErrCode; stdcall;
    function Interface_CreateEndIterator(out Value: IIterator) : ErrCode; stdcall;
  end;
  
implementation
uses  
  SysUtils,
  OpenDAQ.CoreTypes.Errors,
  OpenDAQ.Exceptions,
  OpenDAQ.ProxyValue,
  OpenDAQ.SmartPtrRegistry;

{ RtCoreEnumerator }

constructor TRtCoreEnumerator.Create(Iterator: IIteratorPtr);
begin
  Self.Iterator := Iterator;
end;

function TRtCoreEnumerator.MoveNext: Boolean;
begin
  Result := Iterator.MoveNext;
end;

procedure TRtCoreEnumerator.Reset;
begin
  raise Exception.Create('Not implemented');
end;

function TRtCoreEnumerator.GetCurrent: TObject;
begin
  Result := TObject(Iterator.GetCurrentPtr.GetObject);
end;

{ RtCoreEnumerator<T> }

constructor TRtCoreEnumerator<T>.Create(Iterator: IIteratorPtr);
begin
  inherited Create(Iterator);
end;

function TRtCoreEnumerator<T>.GetCurrent: T;
var
  CurObj: IBaseObject;
begin
  CurObj := Iterator.GetCurrentPtr.GetObject;
  Result := TProxyValue<IBaseObject>.Create(CurObj).AsInterface<T>;
end;

{ TIterablePtr<T> }

constructor TIterablePtr<T>.Create(Obj: IIterable);
begin
  inherited Create(Obj);
end;

constructor TIterablePtr<T>.Create(Obj: IBaseObject);
begin
  inherited Create(Obj);
end;

function TIterablePtr<T>.CreateStartIterator(): IIteratorPtr<T>;
var
  Err : ErrCode;
  Iter : IIterator;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.CreateStartIterator(Iter);
  CheckRtErrorInfo(Err);

  Result := TIteratorPtr<T>.Create(Iter);
end;

function TIterablePtr<T>.CreateEndIterator(): IIteratorPtr<T>;
var
  Err : ErrCode;
  Iter : IIterator;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.CreateEndIterator(Iter);
  CheckRtErrorInfo(Err);

  Result := TIteratorPtr<T>.Create(Iter);
end;

function TIterablePtr<T>.Interface_CreateStartIterator(out Value: IIterator) : ErrCode;
begin
  Result := FObject.CreateStartIterator(Value);
end;

function TIterablePtr<T>.Interface_CreateEndIterator(out Value: IIterator) : ErrCode;
begin
  Result := FObject.CreateEndIterator(Value);
end;

function TIterablePtr<T>.GetEnumerator: IEnumerator<T>;
begin
  Result := TRtCoreEnumerator<T>.Create(CreateStartIterator);
end;

initialization
  TSmartPtrRegistry.RegisterPtr(IIterable, IIterablePtr, TIterablePtr<IBaseObject>);

finalization
  TSmartPtrRegistry.UnregisterPtr(IIterable);

end.
