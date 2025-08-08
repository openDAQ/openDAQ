unit OpenDAQ.Exceptions;

interface

uses
  SysUtils,
  OpenDAQ.CoreTypes,
  System.Generics.Collections;

type
  EDaqException = class(Exception)
  private
    ErrorCode: ErrCode;
  protected
    constructor Create(ErrorCode: ErrCode; Msg: string); overload;
  public
    constructor Create(); overload;
    constructor Create(Msg: string); overload;
    constructor Create(ErrorCode: ErrCode); overload; virtual;
    constructor Create(Msg: string; ErrorCode: ErrCode); overload;
    property Code: ErrCode read ErrorCode;
  end;

  ERTInvalidParameterException = class(EDaqException)
  public
    constructor Create(); overload;
    constructor Create(ErrorCode: ErrCode); overload; override;
    constructor Create(Msg: string); overload;
  end;

  ERTArgumentNullException = class(EDaqException)
  public
    constructor Create(); overload;
    constructor Create(ErrorCode: ErrCode); overload; override;
    constructor Create(Msg: string); overload;
  end;

  ERTOutOfRangeException = class(EDaqException)
  public
    constructor Create(); overload;
    constructor Create(ErrorCode: ErrCode); overload; override;
    constructor Create(Msg: string); overload;
  end;

  ERTNotFoundException = class(EDaqException)
  public
    constructor Create(); overload;
    constructor Create(ErrorCode: ErrCode); overload; override;
    constructor Create(Msg: string); overload;
  end;

  ERTFrozenException = class(EDaqException)
  public
    constructor Create(); overload;
    constructor Create(ErrorCode: ErrCode); overload; override;
    constructor Create(Msg: string); overload;
  end;

  ERTNoInterfaceException = class(EDaqException)
  public
    constructor Create(); overload;
    constructor Create(ErrorCode: ErrCode); overload; override;
    constructor Create(Msg: string); overload;
  end;

  ERTNotImplementedException = class(EDaqException)
  public
    constructor Create(); overload;
    constructor Create(ErrorCode: ErrCode); overload; override;
    constructor Create(Msg: string); overload;
  end;

  ERTInvalidTypeException = class(EDaqException)
  public
    constructor Create(); overload;
    constructor Create(ErrorCode: ErrCode); overload; override;
    constructor Create(Msg: string); overload;
  end;

  ERTNotAssignedException = class(EDaqException)
  public
    constructor Create(); overload;
    constructor Create(ErrorCode: ErrCode); overload; override;
    constructor Create(Msg: string); overload;
  end;

  ERTNotEnabledException = class(EDaqException)
  public
    constructor Create(); overload;
    constructor Create(ErrorCode: ErrCode); overload; override;
    constructor Create(Msg: string); overload;
  end;

  ERTCoversionFailedException = class(EDaqException)
  public
    constructor Create(); overload;
    constructor Create(ErrorCode: ErrCode); overload; override;
    constructor Create(Msg: string); overload;
  end;

  ERTInvalidPropertyException = class(EDaqException)
  public
    constructor Create(); overload;
    constructor Create(ErrorCode: ErrCode); overload; override;
    constructor Create(Msg: string); overload;
  end;

  ERTInvalidValueException = class(EDaqException)
  public
    constructor Create(); overload;
    constructor Create(ErrorCode: ErrCode); overload; override;
    constructor Create(Msg: string); overload;
  end;

  ERTInvalidStateException = class(EDaqException)
  public
    constructor Create(); overload;
    constructor Create(ErrorCode: ErrCode); overload; override;
    constructor Create(Msg: string); overload;
  end;

  ERTAlreadyExistsException = class(EDaqException)
  public
    constructor Create(); overload;
    constructor Create(ErrorCode: ErrCode); overload; override;
    constructor Create(Msg: string); overload;
  end;

  EDaqNoMemoryException = class(EDaqException)
  public
    constructor Create(); overload;
    constructor Create(ErrorCode: ErrCode); overload; override;
    constructor Create(Msg: string); overload;
  end;

  EDaqSizeTooSmallException = class(EDaqException)
  public
    constructor Create(); overload;
    constructor Create(ErrorCode: ErrCode); overload; override;
    constructor Create(Msg: string); overload;
  end;

  EDaqCallFailedException = class(EDaqException)
  public
    constructor Create(); overload;
    constructor Create(ErrorCode: ErrCode); overload; override;
    constructor Create(Msg: string); overload;
  end;

  EDaqParseFailedException = class(EDaqException)
  public
    constructor Create(); overload;
    constructor Create(ErrorCode: ErrCode); overload; override;
    constructor Create(Msg: string); overload;
  end;

  EDaqResolveFailedException = class(EDaqException)
  public
    constructor Create(); overload;
    constructor Create(ErrorCode: ErrCode); overload; override;
    constructor Create(Msg: string); overload;
  end;

  EDaqAccessDeniedException = class(EDaqException)
  public
    constructor Create(); overload;
    constructor Create(ErrorCode: ErrCode); overload; override;
    constructor Create(Msg: string); overload;
  end;

  EDaqNotFrozenException = class(EDaqException)
  public
    constructor Create(); overload;
    constructor Create(ErrorCode: ErrCode); overload; override;
    constructor Create(Msg: string); overload;
  end;

  EDaqNotSerializableException = class(EDaqException)
  public
    constructor Create(); overload;
    constructor Create(ErrorCode: ErrCode); overload; override;
    constructor Create(Msg: string); overload;
  end;

  EDaqInvalidPropertyException = class(EDaqException)
  public
    constructor Create(); overload;
    constructor Create(ErrorCode: ErrCode); overload; override;
    constructor Create(Msg: string); overload;
  end;

  EDaqDuplicateItemException = class(EDaqException)
  public
    constructor Create(); overload;
    constructor Create(ErrorCode: ErrCode); overload; override;
    constructor Create(Msg: string); overload;
  end;

  EDaqInvalidOperationException = class(EDaqException)
  public
    constructor Create(); overload;
    constructor Create(ErrorCode: ErrCode); overload; override;
    constructor Create(Msg: string); overload;
  end;

  EDaqUninitializedException = class(EDaqException)
  public
    constructor Create(); overload;
    constructor Create(ErrorCode: ErrCode); overload; override;
    constructor Create(Msg: string); overload;
  end;

  EDaqValidateFailedException = class(EDaqException)
  public
    constructor Create(); overload;
    constructor Create(ErrorCode: ErrCode); overload; override;
    constructor Create(Msg: string); overload;
  end;

  EDaqNotUpdatableException = class(EDaqException)
  public
    constructor Create(); overload;
    constructor Create(ErrorCode: ErrCode); overload; override;
    constructor Create(Msg: string); overload;
  end;
  
  EDaqNoCompatibleVersionException = class(EDaqException)
  public
    constructor Create(); overload;
    constructor Create(ErrorCode: ErrCode); overload; override;
    constructor Create(Msg: string); overload;
  end;

  EDaqLockedException = class(EDaqException)
  public
    constructor Create(); overload;
    constructor Create(ErrorCode: ErrCode); overload; override;
    constructor Create(Msg: string); overload;
  end;

  EDaqSizeTooLargeException = class(EDaqException)
  public
    constructor Create(); overload;
    constructor Create(ErrorCode: ErrCode); overload; override;
    constructor Create(Msg: string); overload;
  end;

  EDaqBufferFullException = class(EDaqException)
  public
    constructor Create(); overload;
    constructor Create(ErrorCode: ErrCode); overload; override;
    constructor Create(Msg: string); overload;
  end;

  EDaqEmptyScalingTableException = class(EDaqException)
  public
    constructor Create(); overload;
    constructor Create(ErrorCode: ErrCode); overload; override;
    constructor Create(Msg: string); overload;
  end;

  EDaqEmptyRangeException = class(EDaqException)
  public
    constructor Create(); overload;
    constructor Create(ErrorCode: ErrCode); overload; override;
    constructor Create(Msg: string); overload;
  end;

  EDaqCreateFailedException = class(EDaqException)
  public
    constructor Create(); overload;
    constructor Create(ErrorCode: ErrCode); overload; override;
    constructor Create(Msg: string); overload;
  end;

  EDaqDiscoveryFailedException = class(EDaqException)
  public
    constructor Create(); overload;
    constructor Create(ErrorCode: ErrCode); overload; override;
    constructor Create(Msg: string); overload;
  end;

  EDaqCoerceFailedException = class(EDaqException)
  public
    constructor Create(); overload;
    constructor Create(ErrorCode: ErrCode); overload; override;
    constructor Create(Msg: string); overload;
  end;

  EDaqNotSupportedException = class(EDaqException)
  public
    constructor Create(); overload;
    constructor Create(ErrorCode: ErrCode); overload; override;
    constructor Create(Msg: string); overload;
  end;

  EDaqListNotHomogeneousException = class(EDaqException)
  public
    constructor Create(); overload;
    constructor Create(ErrorCode: ErrCode); overload; override;
    constructor Create(Msg: string); overload;
  end;

  // Deserialize Exceptions

  ERTDeserializeException = class abstract(EDaqException)
  public
    constructor Create(Err: ErrCode); overload; override;
    constructor Create(Msg: string; Err: ErrCode); overload;
  end;

  ERTDeserializeParseException = class(ERTDeserializeException)
  public
    constructor Create(); overload;
    constructor Create(ErrorCode: ErrCode); overload; override;
    constructor Create(Msg: string); overload;
  end;

  ERTDeserializeFactoryNotRegisteredException = class(ERTDeserializeException)
  public
    constructor Create(); overload;
    constructor Create(ErrorCode: ErrCode); overload; override;
    constructor Create(Msg: string); overload;
  end;

  ERTDeserializeInvalidTypeTagException = class(ERTDeserializeException)
  public
    constructor Create(); overload;
    constructor Create(ErrorCode: ErrCode); overload; override;
    constructor Create(Msg: string); overload;
  end;

  ERTDeserializeNoTypeTagException = class(ERTDeserializeException)
  public
    constructor Create(); overload;
    constructor Create(ErrorCode: ErrCode); overload; override;
    constructor Create(Msg: string); overload;
  end;

  DaqExceptionClass = class of EDaqException;

  TDaqExceptionRegistry = class
  private
    class var FKnownExceptions : TDictionary<ErrCode, DaqExceptionClass>;
  public
    class procedure RegisterException(Err: ErrCode; ClassType: DaqExceptionClass); static;
    class procedure UnregisterException(Err: ErrCode); static;
    class function GetExceptionClass(Err: ErrCode): DaqExceptionClass; static;
  end;

  procedure CheckDaqErrorInfo(Err: ErrCode);

implementation

uses
  OpenDAQ.CoreTypes.Errors,
  System.StrUtils;

procedure CheckDaqErrorInfo(Err: ErrCode);
var
  ExceptionClass: DaqExceptionClass;
  Msg: string;
  ErrorMsg: IString;
  ErrorInfo: IErrorInfo;
begin
  // TODO: Use ErrorInfo to get the exception message
  if not OPENDAQ_FAILED(Err) then
    Exit;

  DaqGetErrorInfo(ErrorInfo);
  if Assigned(ErrorInfo) then
  begin
    ErrorInfo.GetMessage(ErrorMsg);
    ErrorInfo := nil;

    if Assigned(ErrorMsg) then
      Msg := RtToString(ErrorMsg);

    DaqClearErrorInfo;
  end;

  ExceptionClass := TDaqExceptionRegistry.GetExceptionClass(Err);
  if not Assigned(ExceptionClass) then
    raise EDaqException.Create(IfThen(Length(Msg) = 0, 'openDAQ Exception occured', Msg), Err)
  else
    if Length(Msg) = 0 then
      raise ExceptionClass.Create(Err)
    else
      raise ExceptionClass.Create(Msg, Err);
end;

{ ERTException }

constructor EDaqException.Create();
begin
  inherited Create('openDAQ Genneral error occured.');
end;

constructor EDaqException.Create(Msg: string);
begin
  inherited Create(Msg);
  Self.ErrorCode := OPENDAQ_ERR_GENERALERROR;
end;

constructor EDaqException.Create(Msg: string; ErrorCode: ErrCode);
begin
  inherited Create('openDAQ Error 0x' + IntToHex(ErrorCode, 8) + ': ' + Msg);
  Self.ErrorCode := ErrorCode;
end;

constructor EDaqException.Create(ErrorCode: ErrCode);
begin
  inherited Create('Error ' + IntToHex(ErrorCode, 8));
  Self.ErrorCode := ErrorCode;
end;

constructor EDaqException.Create(ErrorCode: ErrCode; Msg: string);
begin
  inherited Create(Msg);
  Self.ErrorCode := ErrorCode;
end;

{ ERTInvalidParameterException }

constructor ERTInvalidParameterException.Create(Msg: string);
begin
  inherited Create(Msg, OPENDAQ_ERR_INVALIDPARAMETER);
end;

constructor ERTInvalidParameterException.Create();
begin
  Create(OPENDAQ_ERR_INVALIDPARAMETER);
end;

constructor ERTInvalidParameterException.Create(ErrorCode: ErrCode);
begin
  inherited Create('Invalid parameter', ErrorCode);
end;

{ TRTExceptionRegistry }

class procedure TDaqExceptionRegistry.RegisterException(Err: ErrCode; ClassType: DaqExceptionClass);
begin
  FKnownExceptions.Add(Err, ClassType);
end;

class procedure TDaqExceptionRegistry.UnregisterException(Err: ErrCode);
begin
  FKnownExceptions.Remove(Err);
end;

class function TDaqExceptionRegistry.GetExceptionClass(Err: ErrCode): DaqExceptionClass;
begin
  if not FKnownExceptions.ContainsKey(Err) then
    Exit(nil);

  Result := FKnownExceptions[Err];
end;

{ ERTOutOfRangeException }

constructor ERTOutOfRangeException.Create(Msg: string);
begin
  inherited Create(Msg, OPENDAQ_ERR_OUTOFRANGE);
end;

constructor ERTOutOfRangeException.Create();
begin
  Create(OPENDAQ_ERR_OUTOFRANGE);
end;

constructor ERTOutOfRangeException.Create(ErrorCode: ErrCode);
begin
  inherited Create('Out of range', ErrorCode);
end;

{ ERTNotFoundException }

constructor ERTNotFoundException.Create();
begin
  Create(OPENDAQ_ERR_NOTFOUND);
end;

constructor ERTNotFoundException.Create(Msg: string);
begin
  inherited Create(Msg, OPENDAQ_ERR_NOTFOUND);
end;

constructor ERTNotFoundException.Create(ErrorCode: ErrCode);
begin
  inherited Create('Not found', ErrorCode);
end;

{ ERTFrozenException }

constructor ERTFrozenException.Create(Msg: string);
begin
  inherited Create(Msg, OPENDAQ_ERR_FROZEN);
end;

constructor ERTFrozenException.Create();
begin
  Create(OPENDAQ_ERR_FROZEN);
end;

constructor ERTFrozenException.Create(ErrorCode: ErrCode);
begin
  inherited Create('Frozen', ErrorCode);
end;

{ ERTNoInterfaceException }

constructor ERTNoInterfaceException.Create(Msg: string);
begin
  inherited Create(Msg, OPENDAQ_ERR_NOINTERFACE);
end;

constructor ERTNoInterfaceException.Create();
begin
  Create(OPENDAQ_ERR_NOINTERFACE);
end;

constructor ERTNoInterfaceException.Create(ErrorCode: ErrCode);
begin
  inherited Create('No interface', ErrorCode);
end;

{ ERTNotImplementedException }

constructor ERTNotImplementedException.Create(Msg: string);
begin
  inherited Create(Msg, OPENDAQ_ERR_NOTIMPLEMENTED);
end;

constructor ERTNotImplementedException.Create();
begin
  Create(OPENDAQ_ERR_NOTIMPLEMENTED);
end;

constructor ERTNotImplementedException.Create(ErrorCode: ErrCode);
begin
  inherited Create('Not implemented', ErrorCode);
end;

{ ERTDeserializeParseException }

constructor ERTDeserializeParseException.Create();
begin
  Create(OPENDAQ_ERR_DESERIALIZE_PARSE_ERROR);
end;

constructor ERTDeserializeParseException.Create(Msg: string);
begin
  inherited Create(Msg, OPENDAQ_ERR_DESERIALIZE_PARSE_ERROR);
end;

constructor ERTDeserializeParseException.Create(ErrorCode: ErrCode);
begin
  inherited Create('Error when parsing serialized data', ErrorCode);
end;

{ ERTDeserializeFactoryNotRegisteredException }

constructor ERTDeserializeFactoryNotRegisteredException.Create;
begin
  Create(OPENDAQ_ERR_FACTORY_NOT_REGISTERED);
end;

constructor ERTDeserializeFactoryNotRegisteredException.Create(Msg: string);
begin
  inherited Create(Msg, OPENDAQ_ERR_FACTORY_NOT_REGISTERED);
end;

constructor ERTDeserializeFactoryNotRegisteredException.Create(ErrorCode: ErrCode);
begin
  inherited Create('Deserialize factory for the serialized type is not registered', ErrorCode);
end;

{ ERTDeserializeException }

constructor ERTDeserializeException.Create(Err: ErrCode);
begin
  inherited Create(Err);
end;

constructor ERTDeserializeException.Create(Msg: string; Err: ErrCode);
begin
  inherited Create(Msg, Err);
end;

{ ERTDeserializeInvalidTypeTagException }

constructor ERTDeserializeInvalidTypeTagException.Create();
begin
  Create(OPENDAQ_ERR_DESERIALIZE_UNKNOWN_TYPE);
end;

constructor ERTDeserializeInvalidTypeTagException.Create(Msg: string);
begin
  inherited Create(Msg, OPENDAQ_ERR_DESERIALIZE_UNKNOWN_TYPE);
end;

constructor ERTDeserializeInvalidTypeTagException.Create(ErrorCode: ErrCode);
begin
  inherited Create('Encountered an invalid object type tag when deserializing', ErrorCode);
end;

{ ERTDeserializeNoTypeTagException }

constructor ERTDeserializeNoTypeTagException.Create();
begin
  Create(OPENDAQ_ERR_DESERIALIZE_NO_TYPE);
end;

constructor ERTDeserializeNoTypeTagException.Create(Msg: string);
begin
  inherited Create(Msg, OPENDAQ_ERR_DESERIALIZE_NO_TYPE);
end;

constructor ERTDeserializeNoTypeTagException.Create(ErrorCode: ErrCode);
begin
  inherited Create('Can not deserialize the object as no type tag was found', ErrorCode);
end;

{ ERTInvalidTypeException }

constructor ERTInvalidTypeException.Create();
begin
  Create(OPENDAQ_ERR_INVALIDTYPE);
end;

constructor ERTInvalidTypeException.Create(Msg: string);
begin
  inherited Create(Msg, OPENDAQ_ERR_INVALIDTYPE);
end;

constructor ERTInvalidTypeException.Create(ErrorCode: ErrCode);
begin
  inherited Create('Invalid type', ErrorCode);
end;

{ ERTArgumentNullException }

constructor ERTArgumentNullException.Create;
begin
  Create(OPENDAQ_ERR_ARGUMENT_NULL);
end;

constructor ERTArgumentNullException.Create(Msg: string);
begin
  inherited Create(Msg, OPENDAQ_ERR_ARGUMENT_NULL);
end;

constructor ERTArgumentNullException.Create(ErrorCode: ErrCode);
begin
  inherited Create('Argument must not be null', ErrorCode);
end;

{ ERTNotAssignedException }

constructor ERTNotAssignedException.Create;
begin
  Create(OPENDAQ_ERR_NOTASSIGNED);
end;

constructor ERTNotAssignedException.Create(Msg: string);
begin
  inherited Create(Msg, OPENDAQ_ERR_NOTASSIGNED);
end;

constructor ERTNotAssignedException.Create(ErrorCode: ErrCode);
begin
  inherited Create('Not assigned', ErrorCode);
end;

{ ERTNotEnabledException }

constructor ERTNotEnabledException.Create;
begin
  Create(OPENDAQ_ERR_NOTENABLED);
end;

constructor ERTNotEnabledException.Create(Msg: string);
begin
  inherited Create(Msg, OPENDAQ_ERR_NOTASSIGNED);
end;

constructor ERTNotEnabledException.Create(ErrorCode: ErrCode);
begin
  inherited Create('Not enabled', ErrorCode);
end;

{ ERTCoversionFailedException }

constructor ERTCoversionFailedException.Create;
begin
  Create(OPENDAQ_ERR_CONVERSIONFAILED);
end;

constructor ERTCoversionFailedException.Create(Msg: string);
begin
  inherited Create(Msg, OPENDAQ_ERR_CONVERSIONFAILED);
end;

constructor ERTCoversionFailedException.Create(ErrorCode: ErrCode);
begin
  inherited Create('Conversion failed', ErrorCode);
end;

{ ERTInvalidPropertyException }

constructor ERTInvalidPropertyException.Create;
begin
  Create(OPENDAQ_ERR_INVALIDPROPERTY);
end;

constructor ERTInvalidPropertyException.Create(Msg: string);
begin
  inherited Create(Msg, OPENDAQ_ERR_INVALIDPROPERTY);
end;

constructor ERTInvalidPropertyException.Create(ErrorCode: ErrCode);
begin
  inherited Create('Invalid property', ErrorCode);
end;

{ ERTInvalidValueException }

constructor ERTInvalidValueException.Create;
begin
  Create(OPENDAQ_ERR_INVALIDVALUE);
end;

constructor ERTInvalidValueException.Create(Msg: string);
begin
  inherited Create(Msg, OPENDAQ_ERR_INVALIDVALUE);
end;

constructor ERTInvalidValueException.Create(ErrorCode: ErrCode);
begin
  inherited Create('Invalid value', ErrorCode);
end;

{ ERTInvalidStateException }

constructor ERTInvalidStateException.Create;
begin
  Create(OPENDAQ_ERR_INVALIDSTATE);
end;

constructor ERTInvalidStateException.Create(Msg: string);
begin
  inherited Create(Msg, OPENDAQ_ERR_INVALIDSTATE);
end;

constructor ERTInvalidStateException.Create(ErrorCode: ErrCode);
begin
  inherited Create('Invalid state', ErrorCode);
end;

{ ERTAlreadyExistsException }

constructor ERTAlreadyExistsException.Create;
begin
  Create(OPENDAQ_ERR_ALREADYEXISTS);
end;

constructor ERTAlreadyExistsException.Create(Msg: string);
begin
  inherited Create(Msg, OPENDAQ_ERR_ALREADYEXISTS);
end;

constructor ERTAlreadyExistsException.Create(ErrorCode: ErrCode);
begin
  inherited Create('Already exists', ErrorCode);
end;

{ EDaqNoMemoryException }

constructor EDaqNoMemoryException.Create;
begin
  Create(OPENDAQ_ERR_NOMEMORY);
end;

constructor EDaqNoMemoryException.Create(ErrorCode: ErrCode);
begin
  inherited Create('No memory', ErrorCode);
end;

constructor EDaqNoMemoryException.Create(Msg: string);
begin
  inherited Create(Msg, OPENDAQ_ERR_NOMEMORY);
end;

{ EDaqListNotHomogeneous }

constructor EDaqListNotHomogeneousException.Create;
begin
  Create(OPENDAQ_ERR_LIST_NOT_HOMOGENEOUS);
end;

constructor EDaqListNotHomogeneousException.Create(ErrorCode: ErrCode);
begin
  inherited Create('List is not homogeneous', ErrorCode);
end;

constructor EDaqListNotHomogeneousException.Create(Msg: string);
begin
  inherited Create(Msg, OPENDAQ_ERR_LIST_NOT_HOMOGENEOUS);
end;

{ EDaqNotSupported }

constructor EDaqNotSupportedException.Create;
begin
  Create(OPENDAQ_ERR_NOT_SUPPORTED);
end;

constructor EDaqNotSupportedException.Create(ErrorCode: ErrCode);
begin
  inherited Create('The operation or type is not supported', ErrorCode);
end;

constructor EDaqNotSupportedException.Create(Msg: string);
begin
  inherited Create(Msg, OPENDAQ_ERR_NOT_SUPPORTED);
end;

{ EDaqDiscoveryFailed }

constructor EDaqDiscoveryFailedException.Create;
begin
  Create(OPENDAQ_ERR_DISCOVERY_FAILED);
end;

constructor EDaqDiscoveryFailedException.Create(ErrorCode: ErrCode);
begin
  inherited Create('Device discovery failed', ErrorCode);
end;

constructor EDaqDiscoveryFailedException.Create(Msg: string);
begin
  inherited Create(Msg, OPENDAQ_ERR_DISCOVERY_FAILED);
end;

{ EDaqCreateFailed }

constructor EDaqCreateFailedException.Create;
begin
  Create(OPENDAQ_ERR_CREATE_FAILED);
end;

constructor EDaqCreateFailedException.Create(ErrorCode: ErrCode);
begin
  inherited Create('Failed to create object', ErrorCode);
end;

constructor EDaqCreateFailedException.Create(Msg: string);
begin
  inherited Create(Msg, OPENDAQ_ERR_CREATE_FAILED);
end;

{ EDaqEmptyRange }

constructor EDaqEmptyRangeException.Create;
begin
  Create(OPENDAQ_ERR_EMPTY_RANGE);
end;

constructor EDaqEmptyRangeException.Create(ErrorCode: ErrCode);
begin
  inherited Create('Scaling range must not be empty', ErrorCode);
end;

constructor EDaqEmptyRangeException.Create(Msg: string);
begin
  inherited Create(Msg, OPENDAQ_ERR_EMPTY_RANGE);
end;

{ EDaqEmptyScalingTable }

constructor EDaqEmptyScalingTableException.Create;
begin
  Create(OPENDAQ_ERR_EMPTY_SCALING_TABLE);
end;

constructor EDaqEmptyScalingTableException.Create(ErrorCode: ErrCode);
begin
  inherited Create('Scaling table must not be empty', ErrorCode);
end;

constructor EDaqEmptyScalingTableException.Create(Msg: string);
begin
  inherited Create(Msg, OPENDAQ_ERR_EMPTY_SCALING_TABLE);
end;

{ EDaqBufferFull }

constructor EDaqBufferFullException.Create;
begin
  Create(OPENDAQ_ERR_BUFFERFULL);
end;

constructor EDaqBufferFullException.Create(ErrorCode: ErrCode);
begin
  inherited Create('Buffer full', ErrorCode);
end;

constructor EDaqBufferFullException.Create(Msg: string);
begin
  inherited Create(Msg, OPENDAQ_ERR_BUFFERFULL);
end;

{ EDaqSizeTooLarge }

constructor EDaqSizeTooLargeException.Create;
begin
  Create(OPENDAQ_ERR_SIZETOOLARGE);
end;

constructor EDaqSizeTooLargeException.Create(ErrorCode: ErrCode);
begin
  inherited Create('Size too large', ErrorCode);
end;

constructor EDaqSizeTooLargeException.Create(Msg: string);
begin
  inherited Create(Msg, OPENDAQ_ERR_SIZETOOLARGE);
end;

{ EDaqLocked }

constructor EDaqLockedException.Create;
begin
  Create(OPENDAQ_ERR_LOCKED);
end;

constructor EDaqLockedException.Create(ErrorCode: ErrCode);
begin
  inherited Create('Locked', ErrorCode);
end;

constructor EDaqLockedException.Create(Msg: string);
begin
  inherited Create(Msg, OPENDAQ_ERR_LOCKED);
end;

{ EDaqNoCompatibleVersion }

constructor EDaqNoCompatibleVersionException.Create;
begin
  Create(OPENDAQ_ERR_NO_COMPATIBLE_VERSION);
end;

constructor EDaqNoCompatibleVersionException.Create(ErrorCode: ErrCode);
begin
  inherited Create('No compatible version', ErrorCode);
end;

constructor EDaqNoCompatibleVersionException.Create(Msg: string);
begin
  inherited Create(Msg, OPENDAQ_ERR_NO_COMPATIBLE_VERSION);
end;

{ EDaqNotUpdatable }

constructor EDaqNotUpdatableException.Create;
begin
  Create(OPENDAQ_ERR_NOT_UPDATABLE);
end;

constructor EDaqNotUpdatableException.Create(ErrorCode: ErrCode);
begin
  inherited Create('Not updatable', ErrorCode);
end;

constructor EDaqNotUpdatableException.Create(Msg: string);
begin
  inherited Create(Msg, OPENDAQ_ERR_NOT_UPDATABLE);
end;

{ EDaqValidateFailed }

constructor EDaqValidateFailedException.Create;
begin
  Create(OPENDAQ_ERR_VALIDATE_FAILED);
end;

constructor EDaqValidateFailedException.Create(ErrorCode: ErrCode);
begin
  inherited Create('Validate failed', ErrorCode);
end;

constructor EDaqValidateFailedException.Create(Msg: string);
begin
  inherited Create(Msg, OPENDAQ_ERR_VALIDATE_FAILED);
end;

{ EDaqUninitialized }

constructor EDaqUninitializedException.Create;
begin
  Create(OPENDAQ_ERR_UNINITIALIZED);
end;

constructor EDaqUninitializedException.Create(ErrorCode: ErrCode);
begin
  inherited Create('The operation requires initialization', ErrorCode);
end;

constructor EDaqUninitializedException.Create(Msg: string);
begin
  inherited Create(Msg, OPENDAQ_ERR_UNINITIALIZED);
end;

{ EDaqInvalidOperation }

constructor EDaqInvalidOperationException.Create;
begin
  Create(OPENDAQ_ERR_INVALID_OPERATION);
end;

constructor EDaqInvalidOperationException.Create(ErrorCode: ErrCode);
begin
  inherited Create('Operation in not valid for the current type or state.', ErrorCode);
end;

constructor EDaqInvalidOperationException.Create(Msg: string);
begin
  inherited Create(Msg, OPENDAQ_ERR_INVALID_OPERATION);
end;

{ EDaqDuplicateItem }

constructor EDaqDuplicateItemException.Create;
begin
  Create(OPENDAQ_ERR_DUPLICATEITEM);
end;

constructor EDaqDuplicateItemException.Create(ErrorCode: ErrCode);
begin
  inherited Create('Duplicate item', ErrorCode);
end;

constructor EDaqDuplicateItemException.Create(Msg: string);
begin
  inherited Create(Msg, OPENDAQ_ERR_DUPLICATEITEM);
end;

{ EDaqInvalidProperty }

constructor EDaqInvalidPropertyException.Create;
begin
  Create(OPENDAQ_ERR_INVALIDPROPERTY);
end;

constructor EDaqInvalidPropertyException.Create(ErrorCode: ErrCode);
begin
  inherited Create('Invalid property', ErrorCode);
end;

constructor EDaqInvalidPropertyException.Create(Msg: string);
begin
  inherited Create(Msg, OPENDAQ_ERR_INVALIDPROPERTY);
end;

{ EDaqNotSerializable }

constructor EDaqNotSerializableException.Create;
begin
  Create(OPENDAQ_ERR_NOT_SERIALIZABLE);
end;

constructor EDaqNotSerializableException.Create(ErrorCode: ErrCode);
begin
  inherited Create('Not serializable', ErrorCode);
end;

constructor EDaqNotSerializableException.Create(Msg: string);
begin
  inherited Create(Msg, OPENDAQ_ERR_NOT_SERIALIZABLE);
end;

{ EDaqNotFrozen }

constructor EDaqNotFrozenException.Create;
begin
  Create(OPENDAQ_ERR_NOT_FROZEN);
end;

constructor EDaqNotFrozenException.Create(ErrorCode: ErrCode);
begin
  inherited Create('Object is not frozen', ErrorCode);
end;

constructor EDaqNotFrozenException.Create(Msg: string);
begin
  inherited Create(Msg, OPENDAQ_ERR_NOT_FROZEN);
end;

{ EDaqAccessDenied }

constructor EDaqAccessDeniedException.Create;
begin
  Create(OPENDAQ_ERR_ACCESSDENIED);
end;

constructor EDaqAccessDeniedException.Create(ErrorCode: ErrCode);
begin
  inherited Create('Access denied', ErrorCode);
end;

constructor EDaqAccessDeniedException.Create(Msg: string);
begin
  inherited Create(Msg, OPENDAQ_ERR_ACCESSDENIED);
end;

{ EDaqResolveFailed }

constructor EDaqResolveFailedException.Create;
begin
  Create(OPENDAQ_ERR_RESOLVEFAILED);
end;

constructor EDaqResolveFailedException.Create(ErrorCode: ErrCode);
begin
  inherited Create('Resolve failed', ErrorCode);
end;

constructor EDaqResolveFailedException.Create(Msg: string);
begin
  inherited Create(Msg, OPENDAQ_ERR_RESOLVEFAILED);
end;

{ EDaqParseFailed }

constructor EDaqParseFailedException.Create;
begin
  Create(OPENDAQ_ERR_PARSEFAILED);
end;

constructor EDaqParseFailedException.Create(ErrorCode: ErrCode);
begin
  inherited Create('Parse failed', ErrorCode);
end;

constructor EDaqParseFailedException.Create(Msg: string);
begin
  inherited Create(Msg, OPENDAQ_ERR_PARSEFAILED);
end;

{ EDaqCallFailed }

constructor EDaqCallFailedException.Create;
begin
  Create(OPENDAQ_ERR_CALLFAILED);
end;

constructor EDaqCallFailedException.Create(ErrorCode: ErrCode);
begin
  inherited Create('Call failed', ErrorCode);
end;

constructor EDaqCallFailedException.Create(Msg: string);
begin
  inherited Create(Msg, OPENDAQ_ERR_CALLFAILED);
end;

{ EDaqSizeTooSmall }

constructor EDaqSizeTooSmallException.Create;
begin
  Create(OPENDAQ_ERR_SIZETOOSMALL);
end;

constructor EDaqSizeTooSmallException.Create(ErrorCode: ErrCode);
begin
  inherited Create('Size too small', ErrorCode);
end;

constructor EDaqSizeTooSmallException.Create(Msg: string);
begin
  inherited Create(Msg, OPENDAQ_ERR_SIZETOOSMALL);
end;

{ EDaqCoerceFailed }

constructor EDaqCoerceFailedException.Create;
begin
  Create(OPENDAQ_ERR_COERCE_FAILED);
end;

constructor EDaqCoerceFailedException.Create(ErrorCode: ErrCode);
begin
  inherited Create('Coercing failed', ErrorCode);
end;

constructor EDaqCoerceFailedException.Create(Msg: string);
begin
  inherited Create(Msg, OPENDAQ_ERR_COERCE_FAILED);
end;

initialization
  TDaqExceptionRegistry.FKnownExceptions := TDictionary<ErrCode, DaqExceptionClass>.Create();
  TDaqExceptionRegistry.RegisterException(OPENDAQ_ERR_NOMEMORY, EDaqNoMemoryException);
  TDaqExceptionRegistry.RegisterException(OPENDAQ_ERR_INVALIDPARAMETER, ERTInvalidParameterException);
  TDaqExceptionRegistry.RegisterException(OPENDAQ_ERR_NOINTERFACE, ERTNoInterfaceException);
  TDaqExceptionRegistry.RegisterException(OPENDAQ_ERR_SIZETOOSMALL, EDaqSizeTooSmallException);
  TDaqExceptionRegistry.RegisterException(OPENDAQ_ERR_CONVERSIONFAILED, ERTCoversionFailedException);
  TDaqExceptionRegistry.RegisterException(OPENDAQ_ERR_OUTOFRANGE, ERTOutOfRangeException);
  TDaqExceptionRegistry.RegisterException(OPENDAQ_ERR_NOTFOUND, ERTNotFoundException);
  TDaqExceptionRegistry.RegisterException(OPENDAQ_ERR_ALREADYEXISTS, ERTAlreadyExistsException);
  TDaqExceptionRegistry.RegisterException(OPENDAQ_ERR_NOTASSIGNED, ERTNotAssignedException);
  TDaqExceptionRegistry.RegisterException(OPENDAQ_ERR_CALLFAILED, EDaqCallFailedException);
  TDaqExceptionRegistry.RegisterException(OPENDAQ_ERR_PARSEFAILED, EDaqParseFailedException);
  TDaqExceptionRegistry.RegisterException(OPENDAQ_ERR_INVALIDVALUE, ERTInvalidValueException);
  TDaqExceptionRegistry.RegisterException(OPENDAQ_ERR_RESOLVEFAILED, EDaqResolveFailedException);
  TDaqExceptionRegistry.RegisterException(OPENDAQ_ERR_INVALIDTYPE, ERTInvalidTypeException);
  TDaqExceptionRegistry.RegisterException(OPENDAQ_ERR_ACCESSDENIED, EDaqAccessDeniedException);
  TDaqExceptionRegistry.RegisterException(OPENDAQ_ERR_NOTENABLED, ERTNotEnabledException);
  TDaqExceptionRegistry.RegisterException(OPENDAQ_ERR_NOTIMPLEMENTED, ERTNotImplementedException);
  TDaqExceptionRegistry.RegisterException(OPENDAQ_ERR_FROZEN, ERTFrozenException);
  TDaqExceptionRegistry.RegisterException(OPENDAQ_ERR_NOT_FROZEN, EDaqNotFrozenException);
  TDaqExceptionRegistry.RegisterException(OPENDAQ_ERR_NOT_SERIALIZABLE, EDaqNotSerializableException);
  TDaqExceptionRegistry.RegisterException(OPENDAQ_ERR_FACTORY_NOT_REGISTERED, ERTDeserializeFactoryNotRegisteredException);
  TDaqExceptionRegistry.RegisterException(OPENDAQ_ERR_DESERIALIZE_PARSE_ERROR, ERTDeserializeParseException);
  TDaqExceptionRegistry.RegisterException(OPENDAQ_ERR_DESERIALIZE_UNKNOWN_TYPE, ERTDeserializeInvalidTypeTagException);
  TDaqExceptionRegistry.RegisterException(OPENDAQ_ERR_DESERIALIZE_NO_TYPE, ERTDeserializeNoTypeTagException);
  TDaqExceptionRegistry.RegisterException(OPENDAQ_ERR_DUPLICATEITEM, EDaqDuplicateItemException);
  TDaqExceptionRegistry.RegisterException(OPENDAQ_ERR_ARGUMENT_NULL, ERTArgumentNullException);
  TDaqExceptionRegistry.RegisterException(OPENDAQ_ERR_INVALID_OPERATION, EDaqInvalidOperationException);
  TDaqExceptionRegistry.RegisterException(OPENDAQ_ERR_UNINITIALIZED, EDaqUninitializedException);
  TDaqExceptionRegistry.RegisterException(OPENDAQ_ERR_INVALIDSTATE, ERTInvalidStateException);
  TDaqExceptionRegistry.RegisterException(OPENDAQ_ERR_VALIDATE_FAILED, EDaqValidateFailedException);
  TDaqExceptionRegistry.RegisterException(OPENDAQ_ERR_NOT_UPDATABLE, EDaqNotUpdatableException);
  TDaqExceptionRegistry.RegisterException(OPENDAQ_ERR_NO_COMPATIBLE_VERSION, EDaqNoCompatibleVersionException);
  TDaqExceptionRegistry.RegisterException(OPENDAQ_ERR_LOCKED, EDaqLockedException);
  TDaqExceptionRegistry.RegisterException(OPENDAQ_ERR_SIZETOOLARGE, EDaqSizeTooLargeException);
  TDaqExceptionRegistry.RegisterException(OPENDAQ_ERR_BUFFERFULL, EDaqBufferFullException);
  TDaqExceptionRegistry.RegisterException(OPENDAQ_ERR_EMPTY_SCALING_TABLE, EDaqEmptyScalingTableException);
  TDaqExceptionRegistry.RegisterException(OPENDAQ_ERR_EMPTY_RANGE, EDaqEmptyRangeException);
  TDaqExceptionRegistry.RegisterException(OPENDAQ_ERR_CREATE_FAILED, EDaqCreateFailedException);
  TDaqExceptionRegistry.RegisterException(OPENDAQ_ERR_DISCOVERY_FAILED, EDaqDiscoveryFailedException);
  TDaqExceptionRegistry.RegisterException(OPENDAQ_ERR_COERCE_FAILED, EDaqCoerceFailedException);
  TDaqExceptionRegistry.RegisterException(OPENDAQ_ERR_NOT_SUPPORTED, EDaqNotSupportedException);
  TDaqExceptionRegistry.RegisterException(OPENDAQ_ERR_LIST_NOT_HOMOGENEOUS, EDaqListNotHomogeneousException);
  TDaqExceptionRegistry.RegisterException(OPENDAQ_ERR_INVALIDPROPERTY, ERTInvalidPropertyException);

finalization
  TDaqExceptionRegistry.FKnownExceptions.Free();

end.

