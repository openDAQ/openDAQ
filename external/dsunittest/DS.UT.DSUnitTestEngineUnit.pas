unit DS.UT.DSUnitTestEngineUnit;
{$STRONGLINKTYPES ON}
interface
uses
  DUnitX.TestFramework, DS.UT.DSUnitTestUnit, System.Generics.Collections,
  DUnitX.Extensibility;
type
  TDSTestUnitOptions = class
  private
    FPrintAllTests: Boolean;
    FUseFilter: Boolean;
    FFilter: string;
    FOutputJUnitXml: Boolean;
    FOutputJUnitXmlFileName: string;
  protected
  public
    procedure CheckCommandLine;
    procedure CheckEnv;
  end;
  TDSUnitTestEngine = class
  private
    class var FDSOptions: TDSTestUnitOptions;
    class var FDSUnitTestList: TList<TDSUnitTestClass>;
    class procedure RegisterUT;
    class procedure ExportResultsToJUnitXmlFormat(FileName: string; RunResults: IRunResults);
    class procedure PrintAllTests(TestFixtureList: ITestFixtureList);
    class procedure SetUnitTestsFilter(TestFixtureList: ITestFixtureList; Filter: string);
  protected
  public
    class constructor Create;
    class destructor Destroy;
    class procedure RunTests;
    class procedure RegisterUnitTest(UnitTestClass: TDSUnitTestClass);
  end;
implementation
uses
  DUnitX.Loggers.Console, System.SysUtils, System.StrUtils,
  Winapi.Windows, XMLIntf, XMLDoc, Winapi.ActiveX, IOUtils;
{ TDSUnitTestEngine }
class constructor TDSUnitTestEngine.Create;
begin
  FDSUnitTestList := TList<TDSUnitTestClass>.Create;
  FDSOptions := TDSTestUnitOptions.Create;
end;
class destructor TDSUnitTestEngine.Destroy;
begin
  FDSOptions.Free;
  FDSUnitTestList.Free;
  inherited;
end;
class procedure TDSUnitTestEngine.ExportResultsToJUnitXmlFormat(FileName: string; RunResults: IRunResults);
var
  XMLSetup: IXMLDocument;
  TestSuitesNode: IXMLNode;
  FixtureResult: IFixtureResult;
  XmlStr: string;
  procedure ExportFixtureResult(FixtureResult: IFixtureResult);
  var
    TestSuiteNode, TestCaseNode, ResultNode: IXMLNode;
    Child: IFixtureResult;
    TestResult: ITestResult;
  begin
    if FixtureResult.ChildCount > 0 then
    begin
      for Child in FixtureResult.Children do
        ExportFixtureResult(Child);
      Exit;
    end;
    TestSuiteNode := TestSuitesNode.AddChild('testsuite');
    TestSuiteNode.Attributes['name'] := FixtureResult.Fixture.FullName;
    TestSuiteNode.Attributes['tests'] := IntToStr(FixtureResult.ResultCount);
    TestSuiteNode.Attributes['errors'] := '0';
    TestSuiteNode.Attributes['skipped'] := IntToStr(FixtureResult.IgnoredCount);
    TestSuiteNode.Attributes['failures'] := IntToStr(FixtureResult.FailureCount);
    TestSuiteNode.Attributes['time'] := FloatToStr(FixtureResult.Duration.TotalSeconds, TFormatSettings.Invariant);
    TestSuiteNode.Attributes['timestamp'] := DateTimeToStr(FixtureResult.StartTime);
    for TestResult in FixtureResult.TestResults do
    begin
      TestCaseNode := TestSuiteNode.AddChild('testcase');
      TestCaseNode.Attributes['classname'] := TestResult.Test.FullName;
      TestCaseNode.Attributes['name'] := TestResult.Test.Name;
      TestCaseNode.Attributes['time'] := FloatToStr(TestResult.Duration.TotalSeconds, TFormatSettings.Invariant);
      if TestResult.ResultType = TTestResultType.Ignored then
        TestCaseNode.AddChild('skipped')
      else if TestResult.ResultType <> TTestResultType.Pass then
      begin
        ResultNode := TestCaseNode.AddChild('failure');
        ResultNode.Attributes['message'] := 'test failure';
        ResultNode.Text := TestResult.Message;
      end;
    end;
  end;
begin
  CoInitialize(nil);
  XMLSetup := TXMLDocument.Create(nil);
  XMLSetup.Active := True;
  XMLSetup.DocumentElement := XMLSetup.CreateNode('testsuites', ntElement, '');
  TestSuitesNode := XMLSetup.DocumentElement;
  for FixtureResult in RunResults.FixtureResults do
    ExportFixtureResult(FixtureResult);
  XMLSetup.SaveToXML(XmlStr);
  XmlStr := FormatXMLData(XmlStr);
  TFile.WriteAllText(FileName, XmlStr);
end;
class procedure TDSUnitTestEngine.PrintAllTests(TestFixtureList: ITestFixtureList);
var
  TestFixture: ITestFixture;
  procedure PrintFixtureResult(TestFixture: ITestFixture);
  var
    Child: ITestFixture;
    Test: ITest;
  begin
    if TestFixture.Children.Count > 0 then
    begin
      for Child in TestFixture.Children do
        PrintFixtureResult(Child);
      Exit;
    end;
    for Test in TestFixture.Tests do
      System.Writeln(#9 + Test.FullName);
  end;
begin
  System.Writeln('Tests:');
  for TestFixture in TestFixtureList do
    PrintFixtureResult(TestFixture);
end;
class procedure TDSUnitTestEngine.RegisterUnitTest(
  UnitTestClass: TDSUnitTestClass);
begin
  FDSUnitTestList.Add(UnitTestClass);
end;
class procedure TDSUnitTestEngine.RegisterUT;
var
  TC: TDSUnitTestClass;
begin
  for TC in FDSUnitTestList do
    TDUnitX.RegisterTestFixture(TC);
end;
class procedure TDSUnitTestEngine.RunTests;
var
  Runner: ITestRunner;
  Results: IRunResults;
  Logger: ITestLogger;
  TestFixtureList: ITestFixtureList;
begin
  try
    //Check parameters
    FDSOptions.CheckEnv;
    FDSOptions.CheckCommandLine;
    //register unit tests
    RegisterUT;
    //Create the test runner
    Runner := TDUnitX.CreateRunner;
    if FDSOptions.FPrintAllTests then
    begin
      TestFixtureList := Runner.BuildFixtures as ITestFixtureList;
      PrintAllTests(TestFixtureList);
    end
    else
    begin
      if FDSOptions.FUseFilter then
      begin
        TestFixtureList := Runner.BuildFixtures as ITestFixtureList;
        SetUnitTestsFilter(TestFixtureList, FDSOptions.FFilter);
      end;
      //tell the runner how we will log things
      //Log to the console window
      Logger := TDUnitXConsoleLogger.Create(False);
      Runner.AddLogger(Logger);
      Runner.FailsOnNoAsserts := False; //When true, Assertions must be made during tests;
      //Run tests
      Results := Runner.Execute;
      if not Results.AllPassed then
        System.ExitCode := EXIT_ERRORS;
      if FDSOptions.FOutputJUnitXml then
        ExportResultsToJUnitXmlFormat(FDSOptions.FOutputJUnitXmlFileName, Results);
    end;
    {$IFNDEF CI}
    //We don't want this happening when running under CI.
    if TDUnitX.Options.ExitBehavior = TDUnitXExitBehavior.Pause then
    begin
      System.WriteLn('Done.. press <Enter> key to quit.');
//      ConsoleWritten := True;
    end;
    {$ENDIF}
  except
    on E: Exception do
      System.Writeln(E.ClassName, ': ', E.Message);
  end;
end;
class procedure TDSUnitTestEngine.SetUnitTestsFilter(TestFixtureList: ITestFixtureList; Filter: string);
var
  TestFixture: ITestFixture;

  function IsTestActive(Name: string): Boolean;

//  var

//    RegExp : TRegExpr;

  begin

    Result := True;

(*    RegExp := TRegExpr.Create;
    RegExp.Expression := Filter;
    Result := RegExp.Exec(Name);
    RegExp.Free;*)
  end;
  procedure SetActiveTestFixture(TestFixture: ITestFixture);
  var
    Child: ITestFixture;
    Test: ITest;
  begin
    if TestFixture.Children.Count > 0 then
    begin
      for Child in TestFixture.Children do
        SetActiveTestFixture(Child);
      Exit;
    end;
    for Test in TestFixture.Tests do
      Test.Enabled := Test.Enabled and IsTestActive(Test.FullName);
  end;
begin
  for TestFixture in TestFixtureList do
    SetActiveTestFixture(TestFixture);
end;
{ TDSTestUnitOptions }
procedure TDSTestUnitOptions.CheckEnv;
var
  GTestOutput, GTestOutputPath, ExeName: string;
begin
  GTestOutput := System.SysUtils.GetEnvironmentVariable('GTEST_OUTPUT');
  if SameText(LeftStr(GTestOutput, 4), 'xml:') then
  begin
    GTestOutputPath := RightStr(GTestOutput, Length(GTestOutput) - 4);
    FOutputJUnitXml := True;
    if RightStr(GTestOutputPath, 1) = '\' then
    begin
      ExeName := ExtractFileName(ParamStr(0));
      ExeName := LeftStr(ExeName, Length(ExeName) - 4);
      FOutputJUnitXmlFileName := GTestOutputPath + ExeName + '.xml';
    end
    else
      FOutputJUnitXmlFileName := GTestOutputPath;
    OutputDebugString(PChar(FOutputJUnitXmlFileName));
  end;
end;
procedure TDSTestUnitOptions.CheckCommandLine;
var
  Param: string;
  ParamIndex: Integer;
begin
  ParamIndex := 1;
  while ParamCount >= ParamIndex do
  begin
    Param := ParamStr(ParamIndex);
    if CompareText(Param, '-list_tests') = 0 then
    begin
      FPrintAllTests := True;
    end
    else if (CompareText(Param, '-filter') = 0) and (ParamCount >= ParamIndex + 1) then
    begin
      FUseFilter := True;
      FFilter := ParamStr(ParamIndex + 1);
      Inc(ParamIndex);
    end
    else if (CompareText(Param, '-output_junit_xml') = 0) and (ParamCount >= ParamIndex + 1) then
    begin
      FOutputJUnitXml := True;
      FOutputJUnitXmlFileName := ParamStr(ParamIndex + 1);
      Inc(ParamIndex);
    end;
    Inc(ParamIndex);
  end;
end;
end.
