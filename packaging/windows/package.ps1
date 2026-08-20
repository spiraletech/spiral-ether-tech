param(
    [Parameter(Mandatory = $true)]
    [string]$BuildDirectory,

    [Parameter(Mandatory = $true)]
    [string]$OutputDirectory
)

$ErrorActionPreference = 'Stop'

$resolvedBuild = (Resolve-Path -LiteralPath $BuildDirectory).Path
$outputRoot = [IO.Path]::GetFullPath($OutputDirectory)
if (-not (Test-Path -LiteralPath $outputRoot)) {
    New-Item -ItemType Directory -Path $outputRoot | Out-Null
}

$packDirectory = Join-Path $outputRoot 'SPIRAL-OS-HAKUI-ENGINE'
$zipPath = Join-Path $outputRoot 'spiral-os-hakui-engine-windows-x64.zip'

$expectedPrefix = $outputRoot.TrimEnd([IO.Path]::DirectorySeparatorChar) +
    [IO.Path]::DirectorySeparatorChar
if (-not $packDirectory.StartsWith($expectedPrefix, [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Package directory escaped the requested output directory'
}

if (Test-Path -LiteralPath $packDirectory) {
    Remove-Item -LiteralPath $packDirectory -Recurse -Force
}
if (Test-Path -LiteralPath $zipPath) {
    Remove-Item -LiteralPath $zipPath -Force
}
New-Item -ItemType Directory -Path $packDirectory | Out-Null

$hakuiExecutable = Get-ChildItem -LiteralPath $resolvedBuild -Filter 'hakui.exe' -Recurse |
    Select-Object -First 1
if (-not $hakuiExecutable) {
    throw "hakui.exe was not found under $resolvedBuild"
}

$sdlRuntime = Get-ChildItem -LiteralPath $resolvedBuild -Filter 'SDL3.dll' -Recurse |
    Select-Object -First 1
if (-not $sdlRuntime) {
    throw "SDL3.dll was not found under $resolvedBuild"
}

Copy-Item -LiteralPath $hakuiExecutable.FullName `
    -Destination (Join-Path $packDirectory 'SPIRAL-OS-HAKUI-ENGINE.exe')
Copy-Item -LiteralPath $sdlRuntime.FullName -Destination $packDirectory
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'START_HERE.txt') `
    -Destination $packDirectory

Compress-Archive -LiteralPath $packDirectory -DestinationPath $zipPath
Write-Output $zipPath
