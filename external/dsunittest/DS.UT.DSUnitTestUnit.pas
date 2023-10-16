unit DS.UT.DSUnitTestUnit;

interface

uses
  DUnitX.Attributes;

type
  TDSUnitTest = class;
  TDSUnitTestClass = class of TDSUnitTest;

  TDSUnitTest = class(TObject)
  private
  protected
  public
    [Setup]
    procedure Setup; virtual;
    [TearDown]
    procedure TearDown; virtual;
  end;

implementation

{ TDSUnitTest }

procedure TDSUnitTest.Setup;
begin
end;

procedure TDSUnitTest.TearDown;
begin
end;

end.
