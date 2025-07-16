unit OpenDAQ.TSourceLocation;

interface
uses
  OpenDAQ.CoreTypes;

type
  TSourceLocation = packed record
    FileName: PAnsiChar;
    Line: DaqInt;
    FuncName: PAnsiChar;
  end;

implementation

end.