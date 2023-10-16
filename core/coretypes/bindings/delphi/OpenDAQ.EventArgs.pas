unit OpenDAQ.EventArgs;

interface
uses 
  OpenDAQ.CoreTypes,
  OpenDAQ.TString,
  OpenDAQ.ObjectPtr;

type
  {$MINENUMSIZE 4}

  IEventArgs = interface(IBaseObject)
  ['{b3a08f90-069f-5f84-8afd-0c2fa0bfa4a9}']
    function GetEventId(out Id : RtInt) : ErrCode; stdcall;
    function GetEventName(out Name : IString) : ErrCode; stdcall;
  end;

  IEventArgsPtr<T : IEventArgs> = interface(IObjectPtr<T>)
  ['{2e26250c-0947-5b97-9962-b034b22f6771}']
    function GetEventId() : RtInt;
    function GetEventName() : IStringPtr;
  end;

  IEventArgsPtr = IEventArgsPtr<IEventArgs>;

  TEventArgsPtr<T : IEventArgs> = class(TObjectPtr<T>, IEventArgsPtr<T>, IEventArgs)
  public
    constructor Create(Obj : IBaseObject); overload; override;
    constructor Create(Obj : T); overload;

    function GetEventId() : RtInt;
    function GetEventName() : IStringPtr;
  private
    function IEventArgs.GetEventId = Interface_GetEventId;
    function IEventArgs.GetEventName = Interface_GetEventName;

    function Interface_GetEventId(out Id : RtInt) : ErrCode; stdcall;
    function Interface_GetEventName(out Name : IString) : ErrCode; stdcall;
  end;

  TEventArgs = TEventArgsPtr<IEventArgs>;

implementation
uses
  OpenDAQ.Exceptions,
  OpenDAQ.CoreTypes.Errors;

constructor TEventArgsPtr<T>.Create(Obj: T);
begin
  inherited Create(Obj);
end;

constructor TEventArgsPtr<T>.Create(Obj: IBaseObject);
begin
  inherited Create(Obj);
end;

function TEventArgsPtr<T>.GetEventId() : RtInt;
var
  Err : ErrCode;
  Id: RtInt;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.GetEventId(Id);
  CheckRtErrorInfo(Err);

  Result := Id;
end;

function TEventArgsPtr<T>.GetEventName() : IStringPtr;
var
  Err : ErrCode;
  Name: IString;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.GetEventName(Name);
  CheckRtErrorInfo(Err);

  Result := TStringPtr.Create(Name);
end;

function TEventArgsPtr<T>.Interface_GetEventId(out Id : RtInt) : ErrCode; stdcall;
begin
  Result := FObject.GetEventId(Id);
end;

function TEventArgsPtr<T>.Interface_GetEventName(out Name : IString) : ErrCode; stdcall;
begin
  Result := FObject.GetEventName(Name);
end;

end.
