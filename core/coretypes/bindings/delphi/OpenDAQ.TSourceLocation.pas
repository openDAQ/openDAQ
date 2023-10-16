unit OpenDAQ.TSourceLocation;

interface
uses
  OpenDAQ.CoreTypes;

type
  TSourceLocation = packed record
    FileName: PAnsiChar;
    Line: RtInt;
    FuncName: PAnsiChar;
  end;

implementation

end.