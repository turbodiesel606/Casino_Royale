#requires -Version 7.2
<# Focused launcher/preset regression checks in private fixtures. Requires local
Qt and Inno, but never compiles an application, installs, or launches Setup.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$QtRoot,
    [Parameter(Mandatory)][string]$WorkDirectory,
    [string]$IncompatibleQtRoot
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$PSNativeCommandUseErrorActionPreference = $false
if (-not $IsWindows) { throw 'These launcher fixtures require Windows.' }
$repository = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
. (Join-Path $repository 'Installer/Windows/PayloadValidation.ps1')
$work = [IO.Path]::GetFullPath($WorkDirectory)
if (-not $work.StartsWith((Join-Path $repository 'build') + '\', [StringComparison]::OrdinalIgnoreCase)) { throw 'Use a fresh private work directory under build/.' }
Assert-NoReparsePoint $work
if (Test-Path -LiteralPath $work) { throw 'Test output already exists.' }
$null = [IO.Directory]::CreateDirectory($work)
$cmake = (Get-Command cmake -CommandType Application | Select-Object -First 1).Source
$pwsh = (Get-Command pwsh -CommandType Application | Select-Object -First 1).Source
$results = [Collections.Generic.List[object]]::new()
$saved = @{}
foreach ($key in @('JOBTRACKER_CREATE_PRESET','JOBTRACKER_CREATE_QT','JOBTRACKER_CREATE_RESULT','QTDIR','CXX')) {
    $saved[$key] = [Environment]::GetEnvironmentVariable($key, 'Process')
}
function Record([string]$Name, [bool]$Condition, [string]$Evidence) {
    if (-not $Condition) { throw "${Name}: $Evidence" }
    $results.Add(@{name=$Name;result='PASS';evidence=$Evidence})
    $results | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $work 'results.json') -Encoding utf8
    Write-Host "PASS: $Name"
}
function New-Fixture([string]$Name) {
    $root = Join-Path $work "Проверка с пробелами/$Name"
    $null = [IO.Directory]::CreateDirectory((Join-Path $root 'Installer/Windows'))
    foreach ($relative in @('Installer/Resolve-CreatePreset.cmake','Installer/Windows/Create-Installer.ps1','Installer/Windows/PayloadValidation.ps1','CMakePresets.json')) {
        Copy-Item -LiteralPath (Join-Path $repository $relative) -Destination (Join-Path $root $relative)
    }
    @'
cmake_minimum_required(VERSION 3.22)
message(FATAL_ERROR "Intentional launcher configure failure")
project(JobTracker VERSION 0.1.0 LANGUAGES NONE)
set(JOBTRACKER_QT_VERSION 6.10.3)
'@ | Set-Content -LiteralPath (Join-Path $root 'CMakeLists.txt') -Encoding utf8
    $local = @{
        name='windows-release-local';inherits='windows-release'
        binaryDir='${sourceDir}/build/Сборка с пробелами'
        cacheVariables=@{CMAKE_PREFIX_PATH=@{type='PATH';value='$env{CREATE_FIXTURE_QT}'};JOBTRACKER_BUILD_TESTS=@{type='BOOL';value='OFF'}}
        environment=@{CREATE_FIXTURE_QT=$QtRoot}
    }
    $pair = @{name='windows-release-local';configurePreset='windows-release-local';configuration='Release'}
    return @{Root=$root;Local=$local;Pair=$pair;Build=(Join-Path $root 'build/Сборка с пробелами')}
}
function Save-Presets([hashtable]$Fixture) {
    $pairs = @()
    if ($Fixture.Pair) { $pairs = @($Fixture.Pair) }
    @{version=3;configurePresets=@($Fixture.Local);buildPresets=$pairs} |
        ConvertTo-Json -Depth 10 | Set-Content -LiteralPath (Join-Path $Fixture.Root 'CMakeUserPresets.json') -Encoding utf8
}
function Resolve-Fixture([hashtable]$Fixture, [string]$Name, [bool]$Success, [string]$Pattern) {
    Save-Presets $Fixture
    $env:JOBTRACKER_CREATE_RESULT = Join-Path $Fixture.Root 'resolved.txt'
    $output = @(& $cmake -P (Join-Path $Fixture.Root 'Installer/Resolve-CreatePreset.cmake') 2>&1)
    $code = $LASTEXITCODE
    $message = $output -join "`n"
    $output | Set-Content -LiteralPath (Join-Path $work "$Name.log") -Encoding utf8
    if ($Success -and $code -eq 0) { $message = Get-Content -LiteralPath $env:JOBTRACKER_CREATE_RESULT -Raw }
    $checkedMessage = $message
    if (-not $Success) { $checkedMessage = [regex]::Replace($message, '\s+', ' ') }
    Record $Name (($code -eq 0) -eq $Success -and $checkedMessage -match $Pattern) "Exit=$code; $message"
}
function Add-Cache([hashtable]$Fixture, [string]$Extra = '') {
    $null = [IO.Directory]::CreateDirectory($Fixture.Build)
    $root = $Fixture.Root.Replace('\','/')
    $qt = $QtRoot.Replace('\','/')
    @"
CMAKE_HOME_DIRECTORY:INTERNAL=$root
CMAKE_GENERATOR:INTERNAL=Visual Studio 17 2022
CMAKE_GENERATOR_PLATFORM:INTERNAL=x64
JOBTRACKER_BUILD_TESTS:BOOL=OFF
Qt6_DIR:PATH=$qt/lib/cmake/Qt6
Qt6OptionalTools_DIR:PATH=Qt6OptionalTools_DIR-NOTFOUND
$Extra
"@ | Set-Content -LiteralPath (Join-Path $Fixture.Build 'CMakeCache.txt') -Encoding utf8
}
function Run-Launcher([hashtable]$Fixture, [string]$Name, [string[]]$Options, [string]$Pattern) {
    Save-Presets $Fixture
    $output = @(& $pwsh -NoProfile -File (Join-Path $Fixture.Root 'Installer/Windows/Create-Installer.ps1') @Options 2>&1)
    $code = $LASTEXITCODE
    $output | Set-Content -LiteralPath (Join-Path $work "$Name.log") -Encoding utf8
    Record $Name ($code -ne 0 -and ($output -join "`n") -match $Pattern -and ($output -join "`n") -notmatch 'PASS: JobTracker') "Exit=$code; $($output -join "`n")"
}
try {
    $env:JOBTRACKER_CREATE_PRESET = $null
    $env:JOBTRACKER_CREATE_QT = $null
    $env:QTDIR = $QtRoot
    $env:CXX = $null
    $f = New-Fixture 'typed-preset'
    Resolve-Fixture $f 'typed-local-preset-and-environment' $true 'buildPreset=windows-release-local'
    Record 'resolved-exact-unicode-build-directory' ((Get-Content $env:JOBTRACKER_CREATE_RESULT -Raw).Contains('build=' + $f.Build.Replace('\','/'))) $f.Build
    Add-Cache $f
    Resolve-Fixture $f 'compatible-cache-with-optional-notfound' $true 'preset=windows-release-local'
    $f = New-Fixture 'configure-environment-fallback'
    $f.Pair = $null
    $f.Local.environment.CREATE_FIXTURE_UNSET = $null
    Resolve-Fixture $f 'fallback-build-retains-null-environment-contract' $true '(?m)^unset.CREATE_FIXTURE_UNSET=\r?$'
    $f.Local.environment.CREATE_FIXTURE_ROOT = '${sourceDir}'
    $f.Local.environment.CREATE_FIXTURE_ASSETS = '$env{CREATE_FIXTURE_ROOT}/assets'
    Resolve-Fixture $f 'nested-source-and-environment-macros' $true ('env.CREATE_FIXTURE_ASSETS=' + [regex]::Escape($f.Root.Replace('\','/') + '/assets'))
    $f = New-Fixture 'compiler-preset-path'
    $compilerDirectory = Join-Path $f.Root 'compiler'
    $null = [IO.Directory]::CreateDirectory($compilerDirectory)
    $fixtureCompiler = Join-Path $compilerDirectory 'fixture-cxx.exe'
    'Selection-only fixture; never executed.' | Set-Content -LiteralPath $fixtureCompiler
    Add-Cache $f ('CMAKE_CXX_COMPILER:FILEPATH=' + $fixtureCompiler.Replace('\','/'))
    $f.Local.environment.PATH = $compilerDirectory + ';$penv{PATH}'
    $f.Local.environment.CXX = 'fixture-cxx.exe'
    Resolve-Fixture $f 'compiler-resolves-with-effective-preset-path' $true 'preset=windows-release-local'
    $f = New-Fixture 'foreign-qt'
    Add-Cache $f 'Qt6Core_DIR:PATH=C:/foreign-sdk/lib/cmake/Qt6Core'
    $before = Get-Sha256 (Join-Path $f.Build 'CMakeCache.txt')
    Resolve-Fixture $f 'reject-foreign-qt-cache' $false 'Incompatible cache'
    Record 'incompatible-cache-preserved' ($before -ceq (Get-Sha256 (Join-Path $f.Build 'CMakeCache.txt'))) $before
    $f = New-Fixture 'test-cache'
    Add-Cache $f 'JOBTRACKER_BUILD_TESTS:BOOL=ON'
    Resolve-Fixture $f 'reject-test-cache' $false 'Incompatible cache'
    $f = New-Fixture 'compiler-cache'
    Add-Cache $f 'CMAKE_CXX_COMPILER:FILEPATH=C:/fixture/old-compiler.exe'
    $env:CXX = $cmake
    Resolve-Fixture $f 'reject-explicit-environment-compiler-conflict' $false 'Incompatible cache'
    $env:CXX = $null
    $f = New-Fixture 'wrong-build-pair'
    $f.Pair.configurePreset = 'windows-debug'
    Resolve-Fixture $f 'reject-build-preset-using-another-tree' $false 'must refer to the selected configure preset'
    $f = New-Fixture 'missing-kit'
    $env:JOBTRACKER_CREATE_QT = Join-Path $work 'missing-qt'
    Resolve-Fixture $f 'reject-explicit-missing-sdk' $false 'Missing Qt SDK'
    $env:JOBTRACKER_CREATE_QT = $null
    $f.Local.cacheVariables.CMAKE_PREFIX_PATH.value = Join-Path $work 'missing-local-kit'
    Resolve-Fixture $f 'automatic-fallback-to-shared' $true '(?m)^preset=windows-release\r?$'
    $env:JOBTRACKER_CREATE_PRESET = 'windows-release-local'
    Resolve-Fixture $f 'explicit-local-kit-never-falls-back' $false 'Missing Qt SDK'
    $env:JOBTRACKER_CREATE_PRESET = $null
    if ($IncompatibleQtRoot) {
        $f.Local.cacheVariables.CMAKE_PREFIX_PATH.value = $IncompatibleQtRoot
        Resolve-Fixture $f 'incompatible-installed-local-falls-back' $true '(?m)^preset=windows-release\r?$'
    }
    $f = New-Fixture 'failure-propagation'
    Run-Launcher $f 'native-configure-failure-stops-workflow' @() 'Intentional launcher configure failure'
    Record 'configure-failure-created-no-installer-stage' (-not (Test-Path (Join-Path $f.Root 'Installer/Windows/output')) -and -not (Test-Path (Join-Path $f.Root 'build/installer')) -and -not @(Get-ChildItem (Join-Path $f.Root 'build') -Filter 'installer-stage-*').Count) 'No packaging/stage after failed configure.'
    $f = New-Fixture 'configuration-precedence'
    $config = Join-Path $f.Root 'Локальная настройка.psd1'
    "@{ QtRoot = 'C:/PLACEHOLDER/missing-sdk' }" | Set-Content -LiteralPath $config -Encoding utf8
    Run-Launcher $f 'cli-overrides-local-unicode-config' @('-ConfigFile',$config,'-QtRoot',$QtRoot) 'Intentional launcher configure failure'
    Run-Launcher $f 'missing-local-config-is-an-error' @('-ConfigFile',(Join-Path $f.Root 'missing.psd1')) 'Local configuration does not exist'
    $f = New-Fixture 'overlap'
    $null = [IO.Directory]::CreateDirectory($f.Build)
    $guard = [IO.File]::Open((Join-Path $f.Build '.jobtracker-installer.lock'), 'OpenOrCreate', 'ReadWrite', 'None')
    try { Run-Launcher $f 'overlapping-launcher-rejected-before-configure' @() 'exclusive installer launcher lock' } finally { $guard.Dispose() }
    Write-Host "PASS: $($results.Count) focused creation checks. Evidence: $work"
} finally {
    foreach ($key in $saved.Keys) { [Environment]::SetEnvironmentVariable($key, $saved[$key], 'Process') }
}
