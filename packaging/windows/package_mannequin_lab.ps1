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

$packDirectory = Join-Path $outputRoot 'HAKUI-MANNEQUIN-LAB'
$zipPath = Join-Path $outputRoot 'HAKUI-MANNEQUIN-LAB-v0.13-SILHOUETTE-PASS-windows-x64.zip'

if (Test-Path -LiteralPath $packDirectory) {
    Remove-Item -LiteralPath $packDirectory -Recurse -Force
}
if (Test-Path -LiteralPath $zipPath) {
    Remove-Item -LiteralPath $zipPath -Force
}
New-Item -ItemType Directory -Path $packDirectory | Out-Null

$labExecutable = Get-ChildItem -LiteralPath $resolvedBuild -Filter 'HAKUI-MANNEQUIN-LAB.exe' -Recurse |
    Select-Object -First 1
if (-not $labExecutable) {
    throw "HAKUI-MANNEQUIN-LAB.exe was not found under $resolvedBuild"
}

$sdlRuntime = Get-ChildItem -LiteralPath $resolvedBuild -Filter 'SDL3.dll' -Recurse |
    Select-Object -First 1
if (-not $sdlRuntime) {
    throw "SDL3.dll was not found under $resolvedBuild"
}

Copy-Item -LiteralPath $labExecutable.FullName -Destination $packDirectory
Copy-Item -LiteralPath $sdlRuntime.FullName -Destination $packDirectory

$readme = @'
HAKUI MANNEQUIN LAB v0.13 — SILHOUETTE PASS
============================================

Separate rig-science executable. It does not boot the HAKUI game world.

SILHOUETTE PASS
- lower, narrower shoulders with visible clavicle slope
- ribcage-to-waist torso taper
- pelvis-to-waist bridge
- tapered upper/lower arms and smaller hands
- thigh-to-calf taper and smaller feet
- cleaner neck/head connection
- front-facing default preserved

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

The block avatar remains HAKUI's engineering truth rig. This executable studies
HAKUI's human mannequin presentation layer before skinning, clothing and face work.
'@
Set-Content -LiteralPath (Join-Path $packDirectory 'README.txt') -Value $readme -Encoding UTF8

Compress-Archive -LiteralPath $packDirectory -DestinationPath $zipPath
Write-Output $zipPath
