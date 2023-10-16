unit OpenDAQ.TProcedure;

interface
uses
  OpenDAQ.CoreTypes,
  OpenDAQ.ObjectPtr;
  
type
  IProcedurePtr = interface(IObjectPtr<IProcedure>)
  ['{D248D58F-3E2A-4C38-AFB5-5414A9AFC6E4}']
    procedure Execute(); overload;
    procedure Execute(Args : IBaseObject); overload;
    procedure Execute(Args : ISmartPtr); overload;
    procedure Execute(Args : string); overload;
    procedure Execute(Args : RtInt); overload;
    procedure Execute(Args : RtFloat); overload;
    procedure Execute(Args : Boolean); overload;
  end;

  TProcedurePtr = class(TObjectPtr<IProcedure>, IProcedurePtr, IProcedure)
  public
    constructor Create(Obj : IBaseObject); overload; override;
    constructor Create(Obj : IProcedure); overload;

    procedure Execute(); overload;
    procedure Execute(Args : IBaseObject); overload;
    procedure Execute(Args : ISmartPtr); overload;
    procedure Execute(Args : string); overload;
    procedure Execute(Args : RtInt); overload;
    procedure Execute(Args : RtFloat); overload;
    procedure Execute(Args : Boolean); overload;

  private
    function IProcedure.Execute = Interface_Execute;

    function Interface_Execute(Args : IBaseObject): ErrCode; stdcall;
  end;

implementation
uses
  OpenDAQ.Exceptions,
  OpenDAQ.SmartPtrRegistry;

{ TProcedurePtr }

constructor TProcedurePtr.Create(Obj: IBaseObject);
begin
  inherited Create(Obj);
end;

constructor TProcedurePtr.Create(Obj: IProcedure);
begin
  inherited Create(Obj);
end;

procedure TProcedurePtr.Execute(Args: string);
var
  Err : ErrCode;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.Execute(CreateStringFromDelphiString(Args));
  CheckRtErrorInfo(Err);
end;

procedure TProcedurePtr.Execute(Args: RtFloat);
var
  Err : ErrCode;
  FloatObj: IFloat;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := CreateFloat(FloatObj, Args);
  CheckRtErrorInfo(Err);

  Err := FObject.Execute(FloatObj);
  CheckRtErrorInfo(Err);
end;

procedure TProcedurePtr.Execute(Args: Boolean);
var
  Err : ErrCode;
  BoolObj: IBoolean;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := CreateBoolean(BoolObj, Args);
  CheckRtErrorInfo(Err);

  Err := FObject.Execute(BoolObj);
  CheckRtErrorInfo(Err);
end;

procedure TProcedurePtr.Execute();
var
  Err : ErrCode;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.Execute(nil);
  CheckRtErrorInfo(Err);
end;

procedure TProcedurePtr.Execute(Args: RtInt);
var
  Err : ErrCode;
  IntObj: IInteger;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := CreateInteger(IntObj, Args);
  CheckRtErrorInfo(Err);

  Err := FObject.Execute(IntObj);
  CheckRtErrorInfo(Err);
end;

procedure TProcedurePtr.Execute(Args : IBaseObject);
var
  Err : ErrCode;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.Execute(Args);
  CheckRtErrorInfo(Err);
end;

procedure TProcedurePtr.Execute(Args: ISmartPtr);
var
  Err : ErrCode;
  Param: IBaseObject;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  if Assigned(Args) and Args.IsAssigned() then
    Param := Args.GetObject()
  else
    Param := nil;

  Err := FObject.Execute(Param);
  CheckRtErrorInfo(Err);
end;

function TProcedurePtr.Interface_Execute(Args : IBaseObject): ErrCode;
begin
  Result := FObject.Execute(Args);
end;

initialization
  TSmartPtrRegistry.RegisterPtr(IProcedure, IProcedurePtr, TProcedurePtr);

finalization
  TSmartPtrRegistry.UnregisterPtr(IProcedure);

end.