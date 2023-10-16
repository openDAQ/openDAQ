unit OpenDAQ.List;

interface
uses
  OpenDAQ.CoreTypes,
  OpenDAQ.ObjectPtr,
  OpenDAQ.ProxyValue,
  OpenDAQ.Iterator;

type
  IListBasePtr = interface(IObjectPtr<IListObject>)
  ['{527C824F-C2A7-4200-BA75-07D08214F872}']
    function GetCount(): SizeT;
    procedure Clear();
  end;

  IListPtr<T : IBaseObject> = interface(IListBasePtr)
  ['{E2658D5B-A4E6-4108-8630-538AC96351C3}']
    function GetItemAt(Index : SizeT) : TProxyValue<T>;
    procedure SetItemAt(Index : SizeT; Value : TProxyValue<T>);

    procedure PushBack(Value : TProxyValue<T>);
    procedure PushFront(Value : TProxyValue<T>);

    procedure MoveBack(Value : TProxyValue<T>);
    procedure MoveFront(Value : TProxyValue<T>);

    function PopBack() : TProxyValue<T>;
    function PopFront() : TProxyValue<T>;

    procedure InsertAt(Index : SizeT; Value : TProxyValue<T>);
    function RemoveAt(Index : SizeT) : TProxyValue<T>;
    procedure DeleteAt(Index : SizeT);

    function CreateStartIterator() : IIteratorPtr<T>;
    function CreateEndIterator() : IIteratorPtr<T>;

    function GetEnumerator: IEnumerator<T>;

    property Items[Idx : SizeT] : TProxyValue<T> read GetItemAt write SetItemAt; default;
  end;

  IListPtr = IListPtr<IBaseObject>;

  TListPtr<T : IBaseObject> = class(TObjectPtr<IListObject>, IListPtr<T>, IListObject)
  public
    constructor Create(); overload; override;
    constructor Create(Obj : IListObject); overload;

    function GetItemAt(Index : SizeT) : TProxyValue<T>;
    procedure SetItemAt(Index : SizeT; Value : TProxyValue<T>);

    function GetCount(): SizeT;

    procedure PushBack(Value : TProxyValue<T>);
    procedure PushFront(Value : TProxyValue<T>);

    procedure MoveBack(Value : TProxyValue<T>);
    procedure MoveFront(Value : TProxyValue<T>);

    function PopBack() : TProxyValue<T>;
    function PopFront() : TProxyValue<T>;

    procedure InsertAt(Index : SizeT; Value : TProxyValue<T>);
    function RemoveAt(Index : SizeT) : TProxyValue<T>;
    procedure DeleteAt(Index : SizeT);

    procedure Clear();

    property Items[Idx : SizeT] : TProxyValue<T> read GetItemAt write SetItemAt;

    function CreateStartIterator() : IIteratorPtr<T>;
    function CreateEndIterator() : IIteratorPtr<T>;

    function GetEnumerator: IEnumerator<T>;

  private
    function IListObject.GetItemAt = Interface_GetItemAt;
    function IListObject.SetItemAt = Interface_SetItemAt;
    function IListObject.GetCount = Interface_GetCount;
    function IListObject.PushBack = Interface_PushBack;
    function IListObject.PushFront = Interface_PushFront;
    function IListObject.MoveBack = Interface_MoveBack;
    function IListObject.MoveFront = Interface_MoveFront;
    function IListObject.PopBack = Interface_PopBack;
    function IListObject.PopFront = Interface_PopFront;
    function IListObject.InsertAt = Interface_InsertAt;
    function IListObject.RemoveAt = Interface_RemoveAt;
    function IListObject.DeleteAt = Interface_DeleteAt;
    function IListObject.Clear = Interface_Clear;
    function IListObject.CreateStartIterator = Interface_CreateStartIterator;
    function IListObject.CreateEndIterator = Interface_CreateEndIterator;

    function Interface_GetItemAt(Index: SizeT; out BaseObject: IBaseObject): ErrCode; stdcall;
    function Interface_SetItemAt(Index: SizeT; BaseObject: IBaseObject): ErrCode; stdcall;

    function Interface_GetCount(out Size: SizeT): ErrCode; stdcall;

    function Interface_PushBack(BaseObject: IBaseObject): ErrCode; stdcall;
    function Interface_PushFront(BaseObject: IBaseObject): ErrCode; stdcall;

    function Interface_MoveBack(BaseObject: IBaseObject): ErrCode; stdcall;
    function Interface_MoveFront(BaseObject: IBaseObject): ErrCode; stdcall;

    function Interface_PopBack(out BaseObject: IBaseObject): ErrCode; stdcall;
    function Interface_PopFront(out BaseObject: IBaseObject): ErrCode; stdcall;
    function Interface_InsertAt(Index: SizeT; BaseObject: IBaseObject): ErrCode; stdcall;
    function Interface_RemoveAt(Index: SizeT; out BaseObject: IBaseObject): ErrCode; stdcall;
    function Interface_DeleteAt(Index: SizeT): ErrCode; stdcall;
    function Interface_Clear: ErrCode; stdcall;

    function Interface_CreateStartIterator(out Iterator: IIterator): ErrCode; stdcall;
    function Interface_CreateEndIterator(out Iterator: IIterator): ErrCode; stdcall;
  end;

implementation
uses
  OpenDAQ.Iterable,
  OpenDAQ.Exceptions,
  OpenDAQ.SmartPtrRegistry;

{ ListPtr<T> }

constructor TListPtr<T>.Create();
var
  List : IListObject;
  Err : ErrCode;
begin
  Err := CreateList(List);
  CheckRtErrorInfo(Err);

  inherited Create(List);
end;

constructor TListPtr<T>.Create(Obj: IListObject);
begin
  inherited Create(Obj);
end;

procedure TListPtr<T>.PushBack(Value: TProxyValue<T>);
var
  Err : ErrCode;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.PushBack(Value);
  CheckRtErrorInfo(Err);
end;

procedure TListPtr<T>.PushFront(Value: TProxyValue<T>);
var
  Err : ErrCode;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.PushFront(Value);
  CheckRtErrorInfo(Err);
end;

function TListPtr<T>.PopBack(): TProxyValue<T>;
var
  Err : ErrCode;
  Removed : IBaseObject;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.PopBack(Removed);
  CheckRtErrorInfo(Err);

  Result := T(Removed);
end;

function TListPtr<T>.PopFront(): TProxyValue<T>;
var
  Err : ErrCode;
  Removed : IBaseObject;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.PopFront(Removed);
  CheckRtErrorInfo(Err);

  Result := T(Removed);
end;

procedure TListPtr<T>.MoveBack(Value: TProxyValue<T>);
var
  Err : ErrCode;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.MoveBack(Value);
  CheckRtErrorInfo(Err);
end;

procedure TListPtr<T>.MoveFront(Value: TProxyValue<T>);
var
  Err : ErrCode;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.MoveFront(Value);
  CheckRtErrorInfo(Err);
end;

procedure TListPtr<T>.InsertAt(Index: SizeT; Value: TProxyValue<T>);
var
  Err : ErrCode;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.InsertAt(Index, Value);
  CheckRtErrorInfo(Err);
end;

function TListPtr<T>.RemoveAt(Index: SizeT) : TProxyValue<T>;
var
  Err : ErrCode;
  Removed : IBaseObject;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.RemoveAt(Index, Removed);
  CheckRtErrorInfo(Err);

  Result := T(Removed);
end;

procedure TListPtr<T>.DeleteAt(Index: SizeT);
var
  Err : ErrCode;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.DeleteAt(Index);
  CheckRtErrorInfo(Err);
end;

procedure TListPtr<T>.SetItemAt(Index: SizeT; Value: TProxyValue<T>);
var
  Err : ErrCode;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.SetItemAt(Index, Value);
  CheckRtErrorInfo(Err);
end;

procedure TListPtr<T>.Clear();
var
  Err : ErrCode;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.Clear();
  CheckRtErrorInfo(Err);
end;

function TListPtr<T>.GetCount(): SizeT;
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

function TListPtr<T>.GetItemAt(Index: SizeT): TProxyValue<T>;
var
  Err : ErrCode;
  Item : IBaseObject;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.GetItemAt(Index, Item);
  CheckRtErrorInfo(Err);

  Result := T(Item);
end;

function TListPtr<T>.CreateStartIterator(): IIteratorPtr<T>;
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

function TListPtr<T>.CreateEndIterator(): IIteratorPtr<T>;
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

function TListPtr<T>.GetEnumerator: IEnumerator<T>;
begin
  Result := TRtCoreEnumerator<T>.Create(CreateStartIterator);
end;

// Interface decorator

function TListPtr<T>.Interface_PushBack(BaseObject: IBaseObject): ErrCode;
begin
  Result := FObject.PushBack(BaseObject);
end;

function TListPtr<T>.Interface_PushFront(BaseObject: IBaseObject): ErrCode;
begin
  Result := FObject.PushFront(BaseObject);
end;

function TListPtr<T>.Interface_MoveBack(BaseObject: IBaseObject): ErrCode;
begin
  Result := FObject.MoveBack(BaseObject);
end;

function TListPtr<T>.Interface_MoveFront(BaseObject: IBaseObject): ErrCode;
begin
  Result := FObject.MoveFront(BaseObject);
end;

function TListPtr<T>.Interface_InsertAt(Index: SizeT; BaseObject: IBaseObject): ErrCode;
begin
  Result := FObject.InsertAt(Index, BaseObject);
end;

function TListPtr<T>.Interface_PopBack(out BaseObject: IBaseObject): ErrCode;
begin
  Result := FObject.PopBack(BaseObject);
end;

function TListPtr<T>.Interface_PopFront(out BaseObject: IBaseObject): ErrCode;
begin
  Result := FObject.PopFront(BaseObject);
end;

function TListPtr<T>.Interface_Clear(): ErrCode;
begin
  Result := FObject.Clear();
end;

function TListPtr<T>.Interface_DeleteAt(Index: SizeT): ErrCode;
begin
  Result := FObject.DeleteAt(Index);
end;

function TListPtr<T>.Interface_GetCount(out Size: SizeT): ErrCode;
begin
  Result := FObject.GetCount(Size);
end;

function TListPtr<T>.Interface_GetItemAt(Index: SizeT; out BaseObject: IBaseObject): ErrCode;
begin
  Result := FObject.GetItemAt(Index, BaseObject);
end;

function TListPtr<T>.Interface_RemoveAt(Index: SizeT; out BaseObject: IBaseObject): ErrCode;
begin
  Result := FObject.RemoveAt(Index, BaseObject);
end;

function TListPtr<T>.Interface_SetItemAt(Index: SizeT; BaseObject: IBaseObject): ErrCode;
begin
  Result := FObject.SetItemAt(Index, BaseObject);
end;

function TListPtr<T>.Interface_CreateEndIterator(out Iterator: IIterator): ErrCode;
begin
  Result := FObject.CreateEndIterator(Iterator);
end;

function TListPtr<T>.Interface_CreateStartIterator(out Iterator: IIterator): ErrCode;
begin
  Result := FObject.CreateStartIterator(Iterator);
end;

initialization
  TSmartPtrRegistry.RegisterPtr(IListObject, IListPtr<IBaseObject>, TListPtr<IBaseObject>);

finalization
  TSmartPtrRegistry.UnregisterPtr(IListObject);

end.
