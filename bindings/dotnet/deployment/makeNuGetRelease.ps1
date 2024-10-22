
param([Parameter(Mandatory=$false)][Switch] $all,
      [Parameter(Mandatory=$false)][Switch] $unpack,
      [Parameter(Mandatory=$false)][Switch] $build)

#[Parameter(Mandatory=$false)][Switch] $repack)


function PrintUsage()
{
  Write-Host '
usage:
  -all               do all of the options below
  -unpack            locally install the latest nupkg file in ./build/bindings/CSharp/nuget/unpacked/
  -build             locally build the assembly and NuGet package with the release version number
' -ForegroundColor Yellow
#  -repack            repack the package from -unpack (assembly should have a new file-version by then)
}

function GetVersion
{
  $files = Get-ChildItem "$nugetWorkDir/*-*" -Include '*.nupkg'
  if ($files.Length -eq 0)
  {
    Write-Host "*** an error occurred (nupkg file not found) ***" -ForegroundColor Magenta
    Write-Host "Please provide a nupkg file for re-packaging in '$nugetWorkDir'`n" -ForegroundColor Yellow
    [System.Media.SystemSounds]::Hand.Play()
    exit 1
  }

  #find the latest nupkg file
  $file = $files | sort LastWriteTime | select -last 1

  Write-Host "found $file.Name for re-packaging`n" -ForegroundColor Yellow

  return $file.Name.Replace('opendaq.net.','').Replace('.nupkg','')
}

function UnPackNuGet()
{
  if (Test-Path "$nugetWorkDir/unpacked")
  {
    Write-Host "> cleaning up unpacked dir`n" -ForegroundColor DarkGreen
    Remove-Item "$nugetWorkDir/unpacked" -Recurse
    if( -not $? ) { popd ; exit $LASTEXITCODE }
  }

  Write-Host "> unpacking $($file.Name)`n" -ForegroundColor DarkGreen
  nuget install 'openDAQ.Net' -OutputDirectory "$nugetWorkDir/unpacked" -source "$nugetWorkDir" -Prerelease -DirectDownload -NoHttpCache -PackageSaveMode nuspec -Version $version
  if( -not $? ) { popd ; exit $LASTEXITCODE }

  Write-Host ''
}

function BuildDotnet()
{
  $env:SHORT_SHA = ""

  [string] $unpackedDir = "$nugetWorkDir/unpacked/opendaq.net.$version"
  [string] $DAQ_VERSION = "-p:OPENDAQ_PACKAGE_VERSION=$OPENDAQ_PACKAGE_VERSION"
  [string] $BUILD_PATH = "-p:CMAKE_RUNTIME_OUTPUT_DIRECTORY=`"$nugetBuildDir`""
  [string] $NO_WARN = '-p:noWarn=\"CS1591,NU1801,CS1572,CS1573,CS1734\"'
  [string] $NUGET_REPACK_PATH = "-p:NUGET_REPACK_PATH=`"$unpackedDir`""

  Write-Host "> comparing package and local commits`n" -ForegroundColor DarkGreen
  [string] $nuspec = Get-Content "$unpackedDir/openDAQ.Net.nuspec"
  [string] $packageCommitHash = if ($nuspec -match '\<repository type=\"git\" commit=\"(?<hash>[a-z0-9]+)\"\s*/\>') { $Matches.hash } else {'not found'}
  [string] $localCommitHash = git rev-parse HEAD
  Write-Host "local commit hash   = $localCommitHash"
  Write-Host "package commit hash = $packageCommitHash"
  if ($localCommitHash -ne $packageCommitHash)
  {
    Write-Host "`n*** The package commit has to be checked out before re-building the DLL ***`n" -ForegroundColor Red
    [System.Media.SystemSounds]::Hand.Play()
    popd
    exit 1
  }

  if (! (Test-Path "$nugetBuildDir"))
  {
    Write-Host "`n> creating build dir`n" -ForegroundColor DarkGreen
    md "$nugetBuildDir"
    if( -not $? ) { popd ; exit $LASTEXITCODE }
    Write-Host ''
  }
  else
  {
    Write-Host "`n> cleaning up build dir`n" -ForegroundColor DarkGreen
    Remove-Item "$nugetBuildDir/*" -Recurse
    if( -not $? ) { popd ; exit $LASTEXITCODE }
  }

  pushd "$baseDir/bindings/dotnet/openDAQ.Net/openDAQ.Net"

  Write-Host "dotnet build openDAQ.Net.csproj --configuration Release -p:Platform=x64 --verbosity minimal --no-incremental $NO_WARN $BUILD_PATH $NUGET_REPACK_PATH $DAQ_VERSION`n" -ForegroundColor Yellow
  dotnet build openDAQ.Net.csproj --configuration Release -p:Platform=x64 --verbosity minimal --no-incremental $NO_WARN $BUILD_PATH $NUGET_REPACK_PATH $DAQ_VERSION
  # dotnet restore openDAQ.Net.csproj --verbosity minimal $NO_WARN $BUILD_PATH $NUGET_REPACK_PATH $DAQ_VERSION
  # dotnet msbuild openDAQ.Net.csproj -p:Configuration=Release -p:Platform=x64 -verbosity:minimal -noWarn:"CS1591,NU1801,CS1572,CS1573" $BUILD_PATH $NUGET_REPACK_PATH $DAQ_VERSION
  if( -not $? ) { popd ; exit $LASTEXITCODE }

  popd

  Write-Host "`n> copying DLL from build dir to unpacked lib dir`n" -ForegroundColor DarkGreen
  Copy-Item "$nugetWorkDir/build/*" -Filter "*.nupkg" -Destination "$nugetWorkDir"
  if( -not $? ) { popd ; exit $LASTEXITCODE }
}

function RePackNuGet()
{
  if (! (Test-Path "$nugetWorkDir/unpacked/opendaq.net.$version"))
  {
    Write-Host "`n*** an error occurred (unpacked directory not found) ***`n" -ForegroundColor Magenta
    [System.Media.SystemSounds]::Hand.Play()
    exit 1
  }

  pushd "$nugetWorkDir/unpacked/opendaq.net.$version"

  Write-Host "`n> copying DLL from build dir to unpacked lib dir`n" -ForegroundColor DarkGreen
  Copy-Item "$nugetWorkDir/build/opendaq.net.dll" -Destination "./lib/net6.0"
  if( -not $? ) { popd ; exit $LASTEXITCODE }

  [string] $fileVersion = (Get-Item "./lib/net6.0/opendaq.net.dll").VersionInfo.FileVersion

  Write-Host "> replacing version '$version' by '$fileVersion' in nuspec file`n" -ForegroundColor DarkGreen
  $(Get-Content 'openDAQ.Net.nuspec') -Replace "$version","$fileVersion" | Set-Content 'openDAQ.Net.nuspec'
  if( -not $? ) { popd ; exit $LASTEXITCODE }

  Write-Host "> re-packing nupkg file`n" -ForegroundColor DarkGreen
  nuget pack 'openDAQ.Net.nuspec' -BasePath "$nugetWorkDir/unpacked/opendaq.net.$version" -OutputDirectory "$nugetWorkDir"
  if( -not $? ) { popd ; exit $LASTEXITCODE }

  popd
}

#=====================================


pushd

trap
{
  popd
  Write-Host "`n*** trapped: an error occurred ***`n" -ForegroundColor Magenta
  [System.Media.SystemSounds]::Hand.Play()
  break
}

#find the base dir
while (! (Test-Path -Path ".\opendaq_version"))
{
  cd ..
}
[string] $baseDir = $(Get-Location).Path.Replace("\", "/")
[string] $buildDir = "$baseDir/build"
[string] $nugetWorkDir = "$buildDir/bindings/CSharp/nuget"
[string] $nugetBuildDir = "$nugetWorkDir/build"
[string] $OPENDAQ_PACKAGE_VERSION = $(Get-Content "$baseDir/opendaq_version" | Out-String).Trim()

Write-Host "`nbaseDir                 = $baseDir"
Write-Host "nugetWorkDir            = $nugetWorkDir"
Write-Host "nugetBuildDir           = $nugetBuildDir"
Write-Host "OPENDAQ_PACKAGE_VERSION = $OPENDAQ_PACKAGE_VERSION"

popd

[string] $startTime = "$(Get-Date -format "dd-MMM-yyyy HH:mm:ss") started"

if (! $unpack -and ! $build -and ! $repack -and ! $all)
{
 PrintUsage
}
else
{
  Write-Host "`n$startTime`n" -ForegroundColor DarkGreen

  if (! (Test-Path "$nugetWorkDir")) { md "$nugetWorkDir" ; Write-Host '' }

  [string] $version = GetVersion

  if ($unpack -or $all) { UnPackNuGet }
  if ($build -or $all) { BuildDotnet }
#  if ($repack -or $all) { RePackNuGet } #not repacking for now since BuildDotnet creates a NuGet package with the un-packed libraries

  Write-Host "`n$startTime`n$(Get-Date -format "dd-MMM-yyyy HH:mm:ss") finished" -ForegroundColor DarkGreen
}

popd

[System.Media.SystemSounds]::Beep.Play()
Write-Host ''
