unit OpenDAQ.TFunction;

interface
uses
  OpenDAQ.CoreTypes,
  OpenDAQ.ObjectPtr;

type
  {$MINENUMSIZE 4}

  IFunctionPtr = interface(IObjectPtr<IFunction>)
  ['{E5C120B1-4383-4923-8FB0-0BD341665CA8}']
    function Call(): IObjectPtr; overload;
    function Call(Args: IBaseObject): IObjectPtr; overload;
    function Call(Args: ISmartPtr): IObjectPtr; overload;
    function Call(Args: string): IObjectPtr; overload;
    function Call(Args: RtInt): IObjectPtr; overload;
    function Call(Args: RtFloat): IObjectPtr; overload;
  end;

  TFunctionPtr = class(TObjectPtr<IFunction>, IFunctionPtr, IFunction)
  public
    constructor Create(Obj : IBaseObject); overload; override;
    constructor Create(Obj : IFunction); overload;

    function Call(): IObjectPtr; overload;
    function Call(Args: IBaseObject): IObjectPtr; overload;
    function Call(Args: ISmartPtr): IObjectPtr; overload;
    function Call(Args: string): IObjectPtr; overload;
    function Call(Args: RtInt): IObjectPtr; overload;
    function Call(Args: RtFloat): IObjectPtr; overload;
  private
    function IFunction.Call = Interface_Call;

    function Interface_Call(Args: IBaseObject; out AResult: IBaseObject) : ErrCode; stdcall;
  end;

implementation
uses
  OpenDAQ.Exceptions,
  OpenDAQ.CoreTypes.Errors,
  OpenDAQ.SmartPtrRegistry;

constructor TFunctionPtr.Create(Obj: IFunction);
begin
  inherited Create(Obj);
end;

constructor TFunctionPtr.Create(Obj: IBaseObject);
begin
  inherited Create(Obj);
end;

function TFunctionPtr.Call(): IObjectPtr;
var
  Err : ErrCode;
  AResult: IBaseObject;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.Call(nil, AResult);
  CheckRtErrorInfo(Err);

  Result := TObjectPtr<IBaseObject>.Create(AResult);
end;

function TFunctionPtr.Call(Args: IBaseObject) : IObjectPtr;
var
  Err : ErrCode;
  AResult: IBaseObject;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.Call(Args, AResult);
  CheckRtErrorInfo(Err);

  Result := TObjectPtr<IBaseObject>.Create(AResult);
end;

function TFunctionPtr.Call(Args: ISmartPtr): IObjectPtr;
var
  Err : ErrCode;
  AResult: IBaseObject;
  Param: IBaseObject;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  if Assigned(Args) and Args.IsAssigned then
    Param := Args.GetObject()
  else
    Param := nil;

  Err := FObject.Call(Param, AResult);
  CheckRtErrorInfo(Err);

  Result := TObjectPtr<IBaseObject>.Create(AResult);
end;

function TFunctionPtr.Call(Args: string): IObjectPtr;
var
  Err : ErrCode;
  StrObj: IString;
  AResult: IBaseObject;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := CreateStringFromDelphiString(StrObj, Args);
  CheckRtErrorInfo(Err);

  Err := FObject.Call(StrObj, AResult);
  CheckRtErrorInfo(Err);

  Result := TObjectPtr<IBaseObject>.Create(AResult);
end;

function TFunctionPtr.Call(Args: RtFloat): IObjectPtr;
var
  Err : ErrCode;
  FloatObj: IFloat;
  AResult: IBaseObject;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := CreateFloat(FloatObj, Args);
  CheckRtErrorInfo(Err);

  Err := FObject.Call(FloatObj, AResult);
  CheckRtErrorInfo(Err);

  Result := TObjectPtr<IBaseObject>.Create(AResult);
end;

function TFunctionPtr.Call(Args: RtInt): IObjectPtr;
var
  Err : ErrCode;
  IntObj: IInteger;
  AResult: IBaseObject;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := CreateInteger(IntObj, Args);
  CheckRtErrorInfo(Err);

  Err := FObject.Call(IntObj, AResult);
  CheckRtErrorInfo(Err);

  Result := TObjectPtr<IBaseObject>.Create(AResult);
end;

function TFunctionPtr.Interface_Call(Args: IBaseObject; out AResult: IBaseObject) : ErrCode;
begin
  Result := FObject.Call(Args, AResult);
end;

initialization
  TSmartPtrRegistry.RegisterPtr(IFunction, IFunctionPtr, TFunctionPtr);

finalization
  TSmartPtrRegistry.UnregisterPtr(IFunction);

end.
