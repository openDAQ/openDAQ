unit OpenDAQ.CoreTypes.Errors;

interface

const
  OPENDAQ_SUCCESS = Cardinal($00000000);
  OPENDAQ_LOWER = Cardinal($00000002);
  OPENDAQ_EQUAL = Cardinal($00000003);
  OPENDAQ_GREATER = Cardinal($00000004);
  OPENDAQ_NO_MORE_ITEMS = Cardinal($00000005);
  OPENDAQ_IGNORED = Cardinal($00000006);

  OPENDAQ_ERR_NOMEMORY = Cardinal($80000000);
  OPENDAQ_ERR_INVALIDPARAMETER = Cardinal($80000001);
  OPENDAQ_ERR_NOINTERFACE = Cardinal($80004002);
  OPENDAQ_ERR_SIZETOOSMALL	= Cardinal($80000003);
  OPENDAQ_ERR_CONVERSIONFAILED = Cardinal($80000004);
  OPENDAQ_ERR_OUTOFRANGE = Cardinal($80000005);
  OPENDAQ_ERR_NOTFOUND	= Cardinal($80000006);
  OPENDAQ_ERR_ALREADYEXISTS = Cardinal($8000000A);
  OPENDAQ_ERR_NOTASSIGNED = Cardinal($8000000B);
  OPENDAQ_ERR_CALLFAILED = Cardinal($8000000C);
  OPENDAQ_ERR_PARSEFAILED = Cardinal($8000000D);
  OPENDAQ_ERR_INVALIDVALUE = Cardinal($8000000E);
  OPENDAQ_ERR_RESOLVEFAILED = Cardinal($80000010);
  OPENDAQ_ERR_INVALIDTYPE = Cardinal($80000011);
  OPENDAQ_ERR_ACCESSDENIED = Cardinal($80000012);
  OPENDAQ_ERR_NOTENABLED = Cardinal($80000013);
  OPENDAQ_ERR_GENERALERROR = Cardinal($80000014);
  OPENDAQ_ERR_CALCFAILED = Cardinal($80000015);
  OPENDAQ_ERR_NOTIMPLEMENTED = Cardinal($80000016);
  OPENDAQ_ERR_FROZEN = Cardinal($80000017);
  OPENDAQ_ERR_NOT_SERIALIZABLE = Cardinal($80000018);
  OPENDAQ_ERR_FACTORY_NOT_REGISTERED = Cardinal($80000020);
  OPENDAQ_ERR_DESERIALIZE_PARSE_ERROR = Cardinal($80000021);
  OPENDAQ_ERR_DESERIALIZE_UNKNOWN_TYPE = Cardinal($80000022);
  OPENDAQ_ERR_DESERIALIZE_NO_TYPE = Cardinal($80000023);
  OPENDAQ_ERR_INVALIDPROPERTY = Cardinal($80000024);
  OPENDAQ_ERR_DUPLICATEITEM = Cardinal($80000025);
  OPENDAQ_ERR_ARGUMENT_NULL = Cardinal($80000026);
  OPENDAQ_ERR_INVALID_OPERATION = Cardinal($80000027);
  OPENDAQ_ERR_UNINITIALIZED = Cardinal($80000028);
  OPENDAQ_ERR_INVALIDSTATE = Cardinal($80000029);
  OPENDAQ_ERR_VALIDATE_FAILED = Cardinal($80000030);
  OPENDAQ_ERR_NOT_UPDATABLE = Cardinal($80000031);
  OPENDAQ_ERR_NO_COMPATIBLE_VERSION = Cardinal($80000032);

  OPENDAQ_ERR_LOCKED = Cardinal($80000033);
  OPENDAQ_ERR_SIZETOOLARGE = Cardinal($80000034);
  OPENDAQ_ERR_BUFFERFULL = Cardinal($80000035);
  OPENDAQ_ERR_CREATE_FAILED = Cardinal($80000036);
  OPENDAQ_ERR_EMPTY_SCALING_TABLE = Cardinal($80000037);
  OPENDAQ_ERR_EMPTY_RANGE = Cardinal($80000038);
  OPENDAQ_ERR_DISCOVERY_FAILED = Cardinal($80000039);
  OPENDAQ_ERR_COERCE_FAILED = Cardinal($80000040);
  OPENDAQ_ERR_NOT_SUPPORTED = Cardinal($80000041);
  OPENDAQ_ERR_LIST_NOT_HOMOGENEOUS = Cardinal($80000042);
  OPENDAQ_ERR_NOT_FROZEN = Cardinal($80000043);


  function OPENDAQ_FAILED(X: Cardinal): LongBool;
  function OPENDAQ_SUCCEEDED(X: Cardinal): LongBool;

  procedure CheckError(ErrCode: Cardinal);

implementation
uses
  SysUtils;

function OPENDAQ_FAILED(X: Cardinal): LongBool;
begin
  Result := LongBool(X and $80000000);
end;

function OPENDAQ_SUCCEEDED(X: Cardinal): LongBool;
begin
  Result := not(OPENDAQ_FAILED(X));
end;

procedure CheckError(ErrCode: Cardinal);
begin
  if OPENDAQ_FAILED(ErrCode) then
  begin
    case ErrCode of
      OPENDAQ_ERR_NOMEMORY: raise EOutOfMemory.Create('Out of memory');
      OPENDAQ_ERR_INVALIDPARAMETER: raise Exception.create('Invalid argument');
      OPENDAQ_ERR_NOINTERFACE: raise Exception.create('No interface');
      OPENDAQ_ERR_SIZETOOSMALL: raise Exception.create('Size to small');
      OPENDAQ_ERR_CONVERSIONFAILED: raise EConvertError.Create('Conversion failed');
      OPENDAQ_ERR_OUTOFRANGE: raise ERangeError.create('Out of range');
      OPENDAQ_ERR_NOTFOUND: raise Exception.create('Not found');
      OPENDAQ_ERR_NOTASSIGNED: raise Exception.Create('Not assigned');
      OPENDAQ_ERR_NOTENABLED: raise Exception.Create('Not enabled');
      OPENDAQ_ERR_GENERALERROR: raise Exception.Create('General error"');
      OPENDAQ_ERR_INVALIDTYPE: raise Exception.Create('Invalid type');
      OPENDAQ_ERR_CALCFAILED: raise Exception.Create('"Calculation failed"');
      OPENDAQ_ERR_ALREADYEXISTS: raise Exception.Create('Already exists');
      OPENDAQ_ERR_FROZEN: raise Exception.Create('Object frozen');
      OPENDAQ_ERR_PARSEFAILED: raise Exception.Create('Parse failed');
      OPENDAQ_ERR_NOTIMPLEMENTED: raise Exception.Create('Not Implemented');
      OPENDAQ_ERR_NOT_SERIALIZABLE: raise Exception.Create('Not serializable');
      OPENDAQ_ERR_DESERIALIZE_PARSE_ERROR: raise Exception.Create('Error when parsing or deserializing');
      OPENDAQ_ERR_INVALIDPROPERTY: raise Exception.Create('Invalid property');
    else
      raise Exception.create('Unexpected error: ' + IntTohex(ErrCode, 8));
    end;
  end;

end;

end.
