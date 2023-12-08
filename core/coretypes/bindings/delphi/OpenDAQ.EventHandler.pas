unit OpenDAQ.EventHandler;

interface
uses 
  OpenDAQ.CoreTypes,
  OpenDAQ.ObjectPtr,
  OpenDAQ.EventArgs;

type
  {$MINENUMSIZE 4}
  

  IEventHandler = interface(IBaseObject)
  ['{8173CD51-2DE8-5DF3-A729-8A2728637DAD}']
    function HandleEvent(Sender : IBaseObject; EventArgs : IEventArgs) : ErrCode; stdcall;
  end;

  IEventHandlerPtr<T : IEventHandler> = interface(IObjectPtr<T>)
  ['{193d2384-f19b-5d45-9af9-51ddbc9e1f8b}']
    procedure HandleEvent(Sender : IBaseObject; EventArgs : IEventArgs);
  end;

  IEventHandlerPtr = IEventHandlerPtr<IEventHandler>;

  TEventHandlerPtr<T : IEventHandler> = class(TObjectPtr<T>, IEventHandlerPtr<T>, IEventHandler)
  public
    constructor Create(Obj : IBaseObject); overload; override;
    constructor Create(Obj : T); overload;

    procedure HandleEvent(Sender : IBaseObject; EventArgs : IEventArgs);
  private
    function IEventHandler.HandleEvent = Interface_HandleEvent;

    function Interface_HandleEvent(Sender : IBaseObject; EventArgs : IEventArgs) : ErrCode; stdcall;
  end;

  TEventHandlerPtr = TEventHandlerPtr<IEventHandler>;

implementation
uses
  OpenDAQ.Exceptions,
  OpenDAQ.CoreTypes.Errors;

constructor TEventHandlerPtr<T>.Create(Obj: T);
begin
  inherited Create(Obj);
end;

constructor TEventHandlerPtr<T>.Create(Obj: IBaseObject);
begin
  inherited Create(Obj);
end;

procedure TEventHandlerPtr<T>.HandleEvent(Sender : IBaseObject; EventArgs : IEventArgs);
var
  Err : ErrCode;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.HandleEvent(Sender, EventArgs);
  CheckRtErrorInfo(Err);
end;

function TEventHandlerPtr<T>.Interface_HandleEvent(Sender : IBaseObject; EventArgs : IEventArgs) : ErrCode; stdcall;
begin
  Result := FObject.HandleEvent(Sender, EventArgs);
end;

end.
