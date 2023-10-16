unit OpenDAQ.Dict;

interface
uses 
  OpenDAQ.CoreTypes,
  OpenDAQ.List,
  OpenDAQ.ObjectPtr,
  OpenDAQ.ProxyValue,
  OpenDAQ.Iterable;

type
  {$MINENUMSIZE 4}

  IDictionaryPtr = interface(IObjectPtr<IDictObject>)
  ['{BC9D416C-3C28-48D9-9759-000F4A12480B}']
    procedure Clear();
    function GetCount() : SizeT;
  end;

  IDictionaryPtr<TKey : IBaseObject; TValue : IBaseObject> = interface(IDictionaryPtr)
  ['{CB33B901-2162-4BBD-87D4-54D84704AB2F}']
    function GetItem(Key : TProxyValue<TKey>) : TProxyValue<TValue>;
    procedure SetItem(Key : TProxyValue<TKey>; Value : TProxyValue<TValue>);
    function RemoveItem(Key : TProxyValue<TKey>) : TProxyValue<TValue>;
    procedure DeleteItem(Key : TProxyValue<TKey>);

    function HasKey(Key : TProxyValue<TKey>) : Boolean;
    function GetKeyList() : IListPtr<TKey>;
    function GetValueList() : IListPtr<TValue>;

    function GetKeys() : IIterablePtr<TKey>;
    function GetValues() : IIterablePtr<TValue>;

    property Items[Key : TProxyValue<TKey>] : TProxyValue<TValue> read GetItem write SetItem; default;
  end;

  TDictionaryPtr<TKey : IBaseObject; TValue : IBaseObject> = class(TObjectPtr<IDictObject>, IDictionaryPtr<TKey, TValue>, IDictObject)
  public
    constructor Create(); overload; override;
    constructor Create(Obj : IBaseObject); overload; override;
    constructor Create(Obj : IDictObject); overload;

    function GetItem(Key : TProxyValue<TKey>) : TProxyValue<TValue>;
    procedure SetItem(Key : TProxyValue<TKey>; Value : TProxyValue<TValue>);

    function RemoveItem(Key : TProxyValue<TKey>) : TProxyValue<TValue>;
    procedure DeleteItem(Key : TProxyValue<TKey>);

    procedure Clear();
    function GetCount() : SizeT;

    function HasKey(Key : TProxyValue<TKey>) : Boolean;

    function GetKeyList() : IListPtr<TKey>;
    function GetValueList() : IListPtr<TValue>;

    function GetKeys() : IIterablePtr<TKey>;
    function GetValues() : IIterablePtr<TValue>;

    property Items[Key : TProxyValue<TKey>] : TProxyValue<TValue> read GetItem write SetItem; default;

  private
    function IDictObject.GetItem = Interface_GetItem;
    function IDictObject.SetItem = Interface_SetItem;
    function IDictObject.RemoveItem = Interface_RemoveItem;
    function IDictObject.DeleteItem = Interface_DeleteItem;
    function IDictObject.Clear = Interface_Clear;
    function IDictObject.GetCount = Interface_GetCount;
    function IDictObject.HasKey = Interface_HasKey;
    function IDictObject.GetKeyList = Interface_GetKeyList;
    function IDictObject.GetValueList = Interface_GetValueList;
    function IDictObject.GetKeys = Interface_GetKeys;
    function IDictObject.GetValues = Interface_GetValues;

    function Interface_GetItem(Key : IBaseObject; out Value : IBaseObject) : ErrCode; stdcall;
    function Interface_SetItem(Key : IBaseObject; Value : IBaseObject) : ErrCode; stdcall;
    function Interface_RemoveItem(Key : IBaseObject; out Value : IBaseObject) : ErrCode; stdcall;
    function Interface_DeleteItem(Key : IBaseObject) : ErrCode; stdcall;
    function Interface_Clear() : ErrCode; stdcall;
    function Interface_GetCount(out Size : SizeT) : ErrCode; stdcall;
    function Interface_HasKey(Key : IBaseObject; var HasKey : Boolean) : ErrCode; stdcall;
    function Interface_GetKeyList(out Keys : IListObject) : ErrCode; stdcall;
    function Interface_GetValueList(out Values : IListObject) : ErrCode; stdcall;
    function Interface_GetKeys(out Keys : IIterable) : ErrCode; stdcall;
    function Interface_GetValues(out Values : IIterable) : ErrCode; stdcall;
  end;

implementation
uses  
  OpenDAQ.CoreTypes.Errors,
  OpenDAQ.Exceptions,
  OpenDAQ.SmartPtrRegistry;

constructor TDictionaryPtr<TKey, TValue>.Create();
var
  Dict : IDictObject;
  Err : ErrCode;
begin
  Err := CreateDict(Dict);
  CheckRtErrorInfo(Err);
  
  inherited Create(Dict);
end;

constructor TDictionaryPtr<TKey, TValue>.Create(Obj: IDictObject);
begin
  inherited Create(Obj);
end;

constructor TDictionaryPtr<TKey, TValue>.Create(Obj: IBaseObject);
begin
  inherited Create(Obj);
end;

function TDictionaryPtr<TKey, TValue>.GetItem(Key : TProxyValue<TKey>) : TProxyValue<TValue>;
var
  Err : ErrCode;
  Value: IBaseObject;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.GetItem(Key, Value);
  CheckRtErrorInfo(Err);

  Result := TProxyValue<TValue>.Create(TValue(Value));
end;

procedure TDictionaryPtr<TKey, TValue>.SetItem(Key : TProxyValue<TKey>; Value : TProxyValue<TValue>);
var
  Err : ErrCode;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.SetItem(Key, Value);
  CheckRtErrorInfo(Err);
end;

function TDictionaryPtr<TKey, TValue>.RemoveItem(Key : TProxyValue<TKey>) : TProxyValue<TValue>;
var
  Err : ErrCode;
  Value: IBaseObject;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.RemoveItem(key, Value);
  CheckRtErrorInfo(Err);

  Result := TValue(Value);
end;

procedure TDictionaryPtr<TKey, TValue>.DeleteItem(Key : TProxyValue<TKey>);
var
  Err : ErrCode;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.DeleteItem(key);
  CheckRtErrorInfo(Err);
end;

procedure TDictionaryPtr<TKey, TValue>.Clear();
var
  Err : ErrCode;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.Clear();
  CheckRtErrorInfo(Err);
end;

function TDictionaryPtr<TKey, TValue>.GetCount() : SizeT;
var
  Err : ErrCode;
  Size: SizeT;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.GetCount(Size);
  CheckRtErrorInfo(Err);

  Result := Size;
end;

function TDictionaryPtr<TKey, TValue>.HasKey(Key : TProxyValue<TKey>) : Boolean;
var
  Err : ErrCode;
  HasKey: Boolean;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.HasKey(key, HasKey);
  CheckRtErrorInfo(Err);

  Result := HasKey;
end;

function TDictionaryPtr<TKey, TValue>.GetKeyList() : IListPtr<TKey>;
var
  Err : ErrCode;
  Keys: IListObject;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.GetKeyList(Keys);
  CheckRtErrorInfo(Err);

  Result := TListPtr<TKey>.Create(Keys);
end;

function TDictionaryPtr<TKey, TValue>.GetValueList() : IListPtr<TValue>;
var
  Err : ErrCode;
  Values: IListObject;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.GetValueList(Values);
  CheckRtErrorInfo(Err);

  Result := TListPtr<TValue>.Create(Values);
end;

function TDictionaryPtr<TKey, TValue>.GetKeys() : IIterablePtr<TKey>;
var
  Err : ErrCode;
  Keys: IIterable;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.GetKeys(Keys);
  CheckRtErrorInfo(Err);

  Result := TIterablePtr<TKey>.Create(Keys);
end;

function TDictionaryPtr<TKey, TValue>.GetValues() : IIterablePtr<TValue>;
var
  Err : ErrCode;
  Values: IIterable;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.GetValues(Values);
  CheckRtErrorInfo(Err);

  Result := TIterablePtr<TValue>.Create(Values);
end;


function TDictionaryPtr<TKey, TValue>.Interface_GetItem(Key : IBaseObject; out Value : IBaseObject) : ErrCode; stdcall;
begin
  Result := FObject.GetItem(key, value);
end;

function TDictionaryPtr<TKey, TValue>.Interface_SetItem(Key : IBaseObject; Value : IBaseObject) : ErrCode; stdcall;
begin
  Result := FObject.SetItem(key, value);
end;

function TDictionaryPtr<TKey, TValue>.Interface_RemoveItem(Key : IBaseObject; out Value : IBaseObject) : ErrCode; stdcall;
begin
  Result := FObject.RemoveItem(key, value);
end;

function TDictionaryPtr<TKey, TValue>.Interface_DeleteItem(Key : IBaseObject) : ErrCode; stdcall;
begin
  Result := FObject.DeleteItem(key);
end;

function TDictionaryPtr<TKey, TValue>.Interface_Clear() : ErrCode; stdcall;
begin
  Result := FObject.Clear();
end;

function TDictionaryPtr<TKey, TValue>.Interface_GetCount(out Size : SizeT) : ErrCode; stdcall;
begin
  Result := FObject.GetCount(size);
end;

function TDictionaryPtr<TKey, TValue>.Interface_HasKey(Key : IBaseObject; var HasKey : Boolean) : ErrCode; stdcall;
begin
  Result := FObject.HasKey(key, hasKey);
end;

function TDictionaryPtr<TKey, TValue>.Interface_GetKeyList(out Keys : IListObject) : ErrCode; stdcall;
begin
  Result := FObject.GetKeyList(keys);
end;

function TDictionaryPtr<TKey, TValue>.Interface_GetValueList(out Values : IListObject) : ErrCode; stdcall;
begin
  Result := FObject.GetValueList(values);
end;

function TDictionaryPtr<TKey, TValue>.Interface_GetKeys(out Keys : IIterable) : ErrCode; stdcall;
begin
  Result := FObject.GetKeys(Keys);
end;

function TDictionaryPtr<TKey, TValue>.Interface_GetValues(out Values : IIterable) : ErrCode; stdcall;
begin
  Result := FObject.GetValues(Values);
end;

initialization
  TSmartPtrRegistry.RegisterPtr(IDictObject, IDictionaryPtr<IBaseObject, IBaseObject>, TDictionaryPtr<IBaseObject, IBaseObject>);

finalization
  TSmartPtrRegistry.UnregisterPtr(IDictObject);

end.
