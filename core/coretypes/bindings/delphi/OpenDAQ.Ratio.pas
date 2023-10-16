unit OpenDAQ.Ratio;

interface
uses
  OpenDAQ.CoreTypes,
  OpenDAQ.ObjectPtr;
  
type
  IRatioPtr = interface(IObjectPtr<IRatio>)
  ['{2CF308AC-0126-44FB-9767-A08B2D77428D}']
    function GetNumerator() : RtInt;
    function GetDenominator() : RtInt;
  end;

  TRatioPtr = class(TObjectPtr<IRatio>, IRatioPtr, IRatio)
  public
    constructor Create(Value : RtInt) overload;
    constructor Create(Numerator : RtInt; Denumerator : RtInt); overload;
    constructor Create(Obj : IBaseObject); overload; override;
    constructor Create(Obj : IRatio); overload;

    function GetNumerator() : RtInt;
    function GetDenominator() : RtInt;

  private
    function IRatio.GetNumerator = Interface_GetNumerator;
    function IRatio.GetDenominator = Interface_GetDenominator;

    function Interface_GetNumerator(out Numerator : RtInt) : ErrCode; stdcall;
    function Interface_GetDenominator(out Denominator : RtInt) : ErrCode; stdcall;
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

constructor TRatioPtr.Create(Value: RtInt);
var
  Err : ErrCode;
  RatioObj : IRatio;
begin
  Err := CreateRatio(RatioObj, Value, 1);
  CheckRtErrorInfo(Err);

  Create(RatioObj);
end;

constructor TRatioPtr.Create(Numerator : RtInt; Denumerator: RtInt);
var
  Err : ErrCode;
  RatioObj : IRatio;
begin
  Err := CreateRatio(RatioObj, Numerator, Denumerator);
  CheckRtErrorInfo(Err);

  Create(RatioObj);
end;

function TRatioPtr.GetNumerator(): RtInt;
var
  Err : ErrCode;
  Numerator : RtInt;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.GetNumerator(Numerator);
  CheckRtErrorInfo(Err);

  Result := Numerator;
end;

function TRatioPtr.GetDenominator(): RtInt;
var
  Err : ErrCode;
  Denominator : RtInt;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.GetDenominator(Denominator);
  CheckRtErrorInfo(Err);

  Result := Denominator;
end;

function TRatioPtr.Interface_GetNumerator(out Numerator : RtInt): ErrCode;
begin
  Result := FObject.GetNumerator(Numerator);
end;

function TRatioPtr.Interface_GetDenominator(out Denominator : RtInt): ErrCode;
begin
  Result := FObject.GetDenominator(Denominator);
end;

initialization
  TSmartPtrRegistry.RegisterPtr(IRatio, IRatioPtr, TRatioPtr);

finalization
  TSmartPtrRegistry.UnregisterPtr(IRatio);

end.