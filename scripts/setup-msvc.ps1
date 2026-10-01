$ErrorActionPreference = 'Stop'
$previousEnvironment = @{}
Get-ChildItem Env: | ForEach-Object { $previousEnvironment[$_.Name] = $_.Value }
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$installation = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if ($LASTEXITCODE -ne 0 -or -not $installation) { throw 'Visual Studio C++ tools were not found' }
$launcher = Join-Path $installation 'Common7\Tools\Launch-VsDevShell.ps1'
& $launcher -Arch amd64 -HostArch amd64 -SkipAutomaticLocation
if (-not (Get-Command cl.exe -ErrorAction SilentlyContinue)) { throw 'MSVC compiler is unavailable' }
if (-not (Get-Command rc.exe -ErrorAction SilentlyContinue)) { throw 'Windows Resource Compiler is unavailable' }
Get-ChildItem Env: | ForEach-Object {
    if ($previousEnvironment[$_.Name] -ne $_.Value) {
        "$($_.Name)=$($_.Value)" | Out-File -FilePath $env:GITHUB_ENV -Encoding utf8 -Append
    }
}
