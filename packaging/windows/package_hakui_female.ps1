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

$packDirectory = Join-Path $outputRoot 'SPIRAL-OS-HAKUI-FEMALE'
$zipPath = Join-Path $outputRoot 'SPIRAL-OS-HAKUI-FEMALE-v1.011-windows-x64.zip'

if (Test-Path -LiteralPath $packDirectory) {
    Remove-Item -LiteralPath $packDirectory -Recurse -Force
}
if (Test-Path -LiteralPath $zipPath) {
    Remove-Item -LiteralPath $zipPath -Force
}
New-Item -ItemType Directory -Path $packDirectory | Out-Null

$exe = Get-ChildItem -LiteralPath $resolvedBuild -Filter 'SPIRAL-OS-HAKUI-FEMALE.exe' -Recurse |
    Select-Object -First 1
if (-not $exe) {
    throw "SPIRAL-OS-HAKUI-FEMALE.exe was not found under $resolvedBuild"
}

$sdlRuntime = Get-ChildItem -LiteralPath $resolvedBuild -Filter 'SDL3.dll' -Recurse |
    Select-Object -First 1
if (-not $sdlRuntime) {
    throw "SDL3.dll was not found under $resolvedBuild"
}

Copy-Item -LiteralPath $exe.FullName -Destination $packDirectory
Copy-Item -LiteralPath $sdlRuntime.FullName -Destination $packDirectory

$readme = @'
SPIRAL OS: HAKUI FEMALE v1.011
===============================

This is the patched sibling copy of polished HAKUI v1.01 with the validated
HAKUI Female Mannequin Lab v0.1 body shell promoted into normal gameplay.

v1.011 COUCH RESTORE
- fixes duplicated/repeated couch cushion geometry
- restores exactly two visible seat cushions
- preserves both couch seat anchors and occupancy semantics
- does not modify the female body shell or ghost gameplay rig

UNCHANGED GHOST / GAMEPLAY AUTHORITY
- locomotion
- jump / sprint
- skateboard and BMX systems
- v1.01 skate embodiment mechanics
- seating / couch interaction
- combat
- chat / social presentation
- camera
- world / casino / interaction systems

FEMALE PRESENTATION SHELL
- narrower/lower shoulders
- female clavicle line
- stronger ribcage-to-waist taper
- wider pelvis shell
- fuller thighs with slimmer calf taper
- slimmer arms/wrists
- smaller hands and feet
- female neck/head proportions

The original polished HAKUI executable remains unchanged.

Extract the ZIP and run SPIRAL-OS-HAKUI-FEMALE.exe.
'@
Set-Content -LiteralPath (Join-Path $packDirectory 'README.txt') -Value $readme -Encoding UTF8

Compress-Archive -LiteralPath $packDirectory -DestinationPath $zipPath
Write-Output $zipPath
