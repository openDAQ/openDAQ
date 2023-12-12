unit OpenDAQ.Event;

interface
uses 
  OpenDAQ.CoreTypes,
  OpenDAQ.ObjectPtr,
  OpenDAQ.EventHandler,
  OpenDAQ.EventArgs;

type
  {$MINENUMSIZE 4}

  IEvent = interface(IBaseObject)
  ['{82774C35-1638-5228-9A72-8DDDFDF10C10}']
    function AddHandler(EventHandler : IEventHandler) : ErrCode; stdcall;
    function RemoveHandler(EventHandler : IEventHandler) : ErrCode; stdcall;
    function Trigger(Sender : IBaseObject; Args : IEventArgs) : ErrCode; stdcall;
    function Clear() : ErrCode; stdcall;
    function GetSubscriberCount(out Count : SizeT) : ErrCode; stdcall;
  end;

  IEventPtr<T : IEvent> = interface(IObjectPtr<T>)
  ['{80408d6d-ad97-5f32-a232-cf4e6a51422b}']
    procedure AddHandler(EventHandler : IEventHandler);
    procedure RemoveHandler(EventHandler : IEventHandler);
    procedure Trigger(Sender : IBaseObject; Args : IEventArgs);
    procedure Clear();
    function GetSubscriberCount() : SizeT;
  end;

  IEventPtr = IEventPtr<IEvent>;

  TEventPtr<T : IEvent> = class(TObjectPtr<T>, IEventPtr<T>, IEvent)
  public
    constructor Create(Obj : IBaseObject); overload; override;
    constructor Create(Obj : T); overload;

    procedure AddHandler(EventHandler : IEventHandler);
    procedure RemoveHandler(EventHandler : IEventHandler);
    procedure Trigger(Sender : IBaseObject; Args : IEventArgs);
    procedure Clear();
    function GetSubscriberCount() : SizeT;
  private
    function IEvent.AddHandler = Interface_AddHandler;
    function IEvent.RemoveHandler = Interface_RemoveHandler;
    function IEvent.Trigger = Interface_Trigger;
    function IEvent.Clear = Interface_Clear;
    function IEvent.GetSubscriberCount = Interface_GetSubscriberCount;

    function Interface_AddHandler(EventHandler : IEventHandler) : ErrCode; stdcall;
    function Interface_RemoveHandler(EventHandler : IEventHandler) : ErrCode; stdcall;
    function Interface_Trigger(Sender : IBaseObject; Args : IEventArgs) : ErrCode; stdcall;
    function Interface_Clear() : ErrCode; stdcall;
    function Interface_GetSubscriberCount(out Count : SizeT) : ErrCode; stdcall;
  end;

  TEventPtr = TEventPtr<IEvent>;

implementation
uses
  OpenDAQ.Exceptions,
  OpenDAQ.CoreTypes.Errors,
  OpenDAQ.SmartPtrRegistry;

constructor TEventPtr<T>.Create(Obj: T);
begin
  inherited Create(Obj);
end;

constructor TEventPtr<T>.Create(Obj: IBaseObject);
begin
  inherited Create(Obj);
end;

procedure TEventPtr<T>.AddHandler(EventHandler : IEventHandler);
var
  Err : ErrCode;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.AddHandler(EventHandler);
  CheckRtErrorInfo(Err);
end;

procedure TEventPtr<T>.RemoveHandler(EventHandler : IEventHandler);
var
  Err : ErrCode;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.RemoveHandler(EventHandler);
  CheckRtErrorInfo(Err);
end;

procedure TEventPtr<T>.Trigger(Sender : IBaseObject; Args : IEventArgs);
var
  Err : ErrCode;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.Trigger(Sender, Args);
  CheckRtErrorInfo(Err);
end;

procedure TEventPtr<T>.Clear();
var
  Err : ErrCode;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.Clear();
  CheckRtErrorInfo(Err);
end;

function TEventPtr<T>.GetSubscriberCount() : SizeT;
var
  Err : ErrCode;
  Count: SizeT;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.GetSubscriberCount(Count);
  CheckRtErrorInfo(Err);

  Result := Count;
end;

function TEventPtr<T>.Interface_AddHandler(EventHandler : IEventHandler) : ErrCode; stdcall;
begin
  Result := FObject.AddHandler(EventHandler);
end;

function TEventPtr<T>.Interface_RemoveHandler(EventHandler : IEventHandler) : ErrCode; stdcall;
begin
  Result := FObject.RemoveHandler(EventHandler);
end;

function TEventPtr<T>.Interface_Trigger(Sender : IBaseObject; Args : IEventArgs) : ErrCode; stdcall;
begin
  Result := FObject.Trigger(Sender, Args);
end;

function TEventPtr<T>.Interface_Clear() : ErrCode; stdcall;
begin
  Result := FObject.Clear();
end;

function TEventPtr<T>.Interface_GetSubscriberCount(out Count : SizeT) : ErrCode; stdcall;
begin
  Result := FObject.GetSubscriberCount(Count);
end;

initialization
  TSmartPtrRegistry.RegisterPtr(IEvent, IEventPtr<IEvent>, TEventPtr<IEvent>);

finalization
  TSmartPtrRegistry.UnregisterPtr(IEvent);

end.
