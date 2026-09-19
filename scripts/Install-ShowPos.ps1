[CmdletBinding()]
param([Parameter(Mandatory=$true)][string]$Csgo)
$ErrorActionPreference = 'Stop'
$destinationRoot = [IO.Path]::GetFullPath($Csgo).TrimEnd('\','/')
$packageRoot = Split-Path -Parent $PSScriptRoot
if (!(Test-Path -LiteralPath (Join-Path $packageRoot 'addons/showpos/bin/win64/showpos.dll'))) {
    $packageRoot = $PSScriptRoot
}
if (!(Test-Path -LiteralPath (Join-Path $destinationRoot 'gameinfo.gi'))) { throw 'Expected the server game/csgo directory.' }
$relativeFiles = @('addons/showpos/bin/win64/showpos.dll','addons/showpos/gamedata/showpos.games.txt','cfg/showpos.cfg','addons/metamod/zzz_showpos.vdf')
foreach ($relative in $relativeFiles) {
    if (!(Test-Path -LiteralPath (Join-Path $packageRoot $relative))) { throw "Incomplete package: $relative" }
}
$coreDll = Join-Path $destinationRoot 'addons/cs2kz/bin/win64/cs2kz.dll'
if (!(Test-Path -LiteralPath $coreDll)) { throw 'CS2KZ core DLL not found.' }
$before = (Get-FileHash -LiteralPath $coreDll -Algorithm SHA256).Hash
$stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
foreach ($relative in $relativeFiles) {
    $source = Join-Path $packageRoot $relative
    $target = [IO.Path]::GetFullPath((Join-Path $destinationRoot $relative))
    if (!$target.StartsWith($destinationRoot + '\',[StringComparison]::OrdinalIgnoreCase)) { throw 'Target escaped csgo directory.' }
    # Refuse write-through junctions/symlinks anywhere under the selected csgo root.
    $ancestor = $target
    while ($ancestor.Length -gt $destinationRoot.Length) {
        if (Test-Path -LiteralPath $ancestor) {
            if ((Get-Item -LiteralPath $ancestor -Force).Attributes -band [IO.FileAttributes]::ReparsePoint) { throw "Refusing linked target: $ancestor" }
        }
        $ancestor = Split-Path -Parent $ancestor
    }
    if (Test-Path -LiteralPath $target) {
        if ($relative -eq 'cfg/showpos.cfg') { Write-Output 'Preserved existing cfg/showpos.cfg'; continue }
        if ((Get-FileHash -LiteralPath $source).Hash -eq (Get-FileHash -LiteralPath $target).Hash) { continue }
        $backup = Join-Path $destinationRoot "addons/showpos/backups/$stamp/$relative"
        New-Item -ItemType Directory -Path (Split-Path -Parent $backup) -Force | Out-Null
        Copy-Item -LiteralPath $target -Destination $backup
    }
    New-Item -ItemType Directory -Path (Split-Path -Parent $target) -Force | Out-Null
    Copy-Item -LiteralPath $source -Destination $target -Force
    if ((Get-FileHash -LiteralPath $source).Hash -ne (Get-FileHash -LiteralPath $target).Hash) { throw "Copy verification failed: $relative" }
    Write-Output "Installed $relative"
}
if ((Get-FileHash -LiteralPath $coreDll -Algorithm SHA256).Hash -ne $before) { throw 'CS2KZ core hash changed unexpectedly.' }
Write-Output "CS2KZ core unchanged: $before"
Write-Output 'Ready. Server console: meta load addons/showpos/bin/win64/showpos.dll'
Write-Output 'Then run showpos_status. Players: !showpos'
