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

$packDirectory = Join-Path $outputRoot 'HAKUI-FEMALE-MANNEQUIN-LAB'
$zipPath = Join-Path $outputRoot 'HAKUI-FEMALE-MANNEQUIN-LAB-v0.1-windows-x64.zip'

if (Test-Path -LiteralPath $packDirectory) {
    Remove-Item -LiteralPath $packDirectory -Recurse -Force
}
if (Test-Path -LiteralPath $zipPath) {
    Remove-Item -LiteralPath $zipPath -Force
}
New-Item -ItemType Directory -Path $packDirectory | Out-Null

$labExecutable = Get-ChildItem -LiteralPath $resolvedBuild -Filter 'HAKUI-FEMALE-MANNEQUIN-LAB.exe' -Recurse |
    Select-Object -First 1
if (-not $labExecutable) {
    throw "HAKUI-FEMALE-MANNEQUIN-LAB.exe was not found under $resolvedBuild"
}

$sdlRuntime = Get-ChildItem -LiteralPath $resolvedBuild -Filter 'SDL3.dll' -Recurse |
    Select-Object -First 1
if (-not $sdlRuntime) {
    throw "SDL3.dll was not found under $resolvedBuild"
}

Copy-Item -LiteralPath $labExecutable.FullName -Destination $packDirectory
Copy-Item -LiteralPath $sdlRuntime.FullName -Destination $packDirectory

$readme = @'
HAKUI FEMALE MANNEQUIN LAB v0.1
================================

Separate female rig-science executable built from the HAKUI Mannequin Lab v0.13
silhouette-pass rig. It does not boot the HAKUI game world and it does not modify
the male mannequin lab.

FEMALE SHELL v0.1
- same canonical HAKUI mannequin skeleton and pose science
- narrower/lower shoulder line
- stronger ribcage-to-waist taper
- wider pelvis/hip shell
- fuller thigh silhouette with slimmer calves
- slimmer wrists and smaller hands/feet
- cleaner neck/head proportions
- female-specific Neutral, T, A, Crouch and Ollie-Pop reach targets

POSE PRESETS
1  Neutral
2  T-Pose
3  A-Pose
4  Crouch
5  Ollie Pop study pose

LIVE RIG TUNING
Q / E       Pelvis yaw
A / D       Torso yaw
W / S       Torso lean
[ / ]       Both knees
Left/Right  Rotate mannequin
J           Toggle joint markers
R           Reset camera
RMB drag    Orbit camera
Mouse wheel Zoom
Esc         Quit

Architecture law:
The male v0.13 mannequin remains the baseline shell. Female v0.1 is a sibling
body presentation bound to the same rig, so later heads, skins, hair, clothing,
and body archetypes can share animation/pose semantics without duplicating the
skeleton.
'@
Set-Content -LiteralPath (Join-Path $packDirectory 'README.txt') -Value $readme -Encoding UTF8

Compress-Archive -LiteralPath $packDirectory -DestinationPath $zipPath
Write-Output $zipPath
