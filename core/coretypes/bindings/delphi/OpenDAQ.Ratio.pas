unit OpenDAQ.Ratio;

interface
uses
  OpenDAQ.CoreTypes,
  OpenDAQ.ObjectPtr;
  
type
  IRatioPtr = interface(IObjectPtr<IRatio>)
  ['{2CF308AC-0126-44FB-9767-A08B2D77428D}']
    function GetNumerator() : DaqInt;
    function GetDenominator() : DaqInt;
  end;

  TRatioPtr = class(TObjectPtr<IRatio>, IRatioPtr, IRatio)
  public
    constructor Create(Value : DaqInt) overload;
    constructor Create(Numerator : DaqInt; Denumerator : DaqInt); overload;
    constructor Create(Obj : IBaseObject); overload; override;
    constructor Create(Obj : IRatio); overload;

    function GetNumerator() : DaqInt;
    function GetDenominator() : DaqInt;

  private
    function IRatio.GetNumerator = Interface_GetNumerator;
    function IRatio.GetDenominator = Interface_GetDenominator;

    function Interface_GetNumerator(out Numerator : DaqInt) : ErrCode; stdcall;
    function Interface_GetDenominator(out Denominator : DaqInt) : ErrCode; stdcall;
  end;

implementation
uses
  OpenDAQ.Exceptions,
  OpenDAQ.SmartPtrRegistry;

{ TFreezablePtr }

constructor TRatioPtr.Create(Obj: IBaseObject);
begin
  inherited Create(Obj);
end;

constructor TRatioPtr.Create(Obj: IRatio);
begin
  inherited Create(Obj);
end;

constructor TRatioPtr.Create(Value: DaqInt);
var
  Err : ErrCode;
  RatioObj : IRatio;
begin
  Err := CreateRatio(RatioObj, Value, 1);
  CheckDaqErrorInfo(Err);

  Create(RatioObj);
end;

constructor TRatioPtr.Create(Numerator : DaqInt; Denumerator: DaqInt);
var
  Err : ErrCode;
  RatioObj : IRatio;
begin
  Err := CreateRatio(RatioObj, Numerator, Denumerator);
  CheckDaqErrorInfo(Err);

  Create(RatioObj);
end;

function TRatioPtr.GetNumerator(): DaqInt;
var
  Err : ErrCode;
  Numerator : DaqInt;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.GetNumerator(Numerator);
  CheckDaqErrorInfo(Err);

  Result := Numerator;
end;

function TRatioPtr.GetDenominator(): DaqInt;
var
  Err : ErrCode;
  Denominator : DaqInt;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.GetDenominator(Denominator);
  CheckDaqErrorInfo(Err);

  Result := Denominator;
end;

function TRatioPtr.Interface_GetNumerator(out Numerator : DaqInt): ErrCode;
begin
  Result := FObject.GetNumerator(Numerator);
end;

function TRatioPtr.Interface_GetDenominator(out Denominator : DaqInt): ErrCode;
begin
  Result := FObject.GetDenominator(Denominator);
end;

initialization
  TSmartPtrRegistry.RegisterPtr(IRatio, IRatioPtr, TRatioPtr);

finalization
  TSmartPtrRegistry.UnregisterPtr(IRatio);

end.