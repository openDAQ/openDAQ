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
    procedure Execute(Args : DaqInt); overload;
    procedure Execute(Args : DaqFloat); overload;
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
    procedure Execute(Args : DaqInt); overload;
    procedure Execute(Args : DaqFloat); overload;
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
  CheckDaqErrorInfo(Err);
end;

procedure TProcedurePtr.Execute(Args: DaqFloat);
var
  Err : ErrCode;
  FloatObj: IFloat;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := CreateFloat(FloatObj, Args);
  CheckDaqErrorInfo(Err);

  Err := FObject.Execute(FloatObj);
  CheckDaqErrorInfo(Err);
end;

procedure TProcedurePtr.Execute(Args: Boolean);
var
  Err : ErrCode;
  BoolObj: IBoolean;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := CreateBoolean(BoolObj, Args);
  CheckDaqErrorInfo(Err);

  Err := FObject.Execute(BoolObj);
  CheckDaqErrorInfo(Err);
end;

procedure TProcedurePtr.Execute();
var
  Err : ErrCode;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.Execute(nil);
  CheckDaqErrorInfo(Err);
end;

procedure TProcedurePtr.Execute(Args: DaqInt);
var
  Err : ErrCode;
  IntObj: IInteger;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := CreateInteger(IntObj, Args);
  CheckDaqErrorInfo(Err);

  Err := FObject.Execute(IntObj);
  CheckDaqErrorInfo(Err);
end;

procedure TProcedurePtr.Execute(Args : IBaseObject);
var
  Err : ErrCode;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.Execute(Args);
  CheckDaqErrorInfo(Err);
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
  CheckDaqErrorInfo(Err);
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