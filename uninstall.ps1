# Removes an ADM installation made by install.ps1:
#
#   & "$env:LOCALAPPDATA\adm\uninstall.ps1"            or
#   irm https://raw.githubusercontent.com/admlang/adm/main/uninstall.ps1 | iex
#
# Removes only what install.ps1 recorded in <install dir>\install-manifest:
# the bin\, lib\ and tools\ folders, this script, and the user PATH entry it
# added. cache\, pkg\ and catalog\ hold the user's build cache and downloaded
# libraries; they stay unless -Purge is given (or $env:ADM_PURGE = "1", for a
# piped run, which takes no arguments). Anything else in the install folder is
# left alone, and the folder itself is removed only when empty.
#
#   $env:ADM_INSTALL_DIR = "..."  the installation to remove (default %LOCALAPPDATA%\adm)

param([switch]$Purge)

function Uninstall-Adm([bool]$purge) {
    $ErrorActionPreference = "Stop"

    function Status($text) { Write-Host ">>> $text" }
    function Fail($text) { Write-Host "ERROR: $text" -ForegroundColor Red; throw "adm uninstall: $text" }

    if ($env:ADM_PURGE -eq "1") { $purge = $true }
    $installDir = if ($env:ADM_INSTALL_DIR) { $env:ADM_INSTALL_DIR } else { Join-Path $env:LOCALAPPDATA "adm" }
    $manifest = Join-Path $installDir "install-manifest"

    # A development setup may link these at a checkout; never touch that.
    foreach ($part in "bin", "lib", "tools") {
        $item = Get-Item -LiteralPath (Join-Path $installDir $part) -Force -ErrorAction SilentlyContinue
        if ($item -and ($item.Attributes -band [IO.FileAttributes]::ReparsePoint)) {
            Fail "$($item.FullName) is a link (a development setup?); nothing removed"
        }
    }

    $dirs = @()
    $files = @()
    $pathAdded = $false
    $createdRoot = $false
    if (Test-Path -LiteralPath $manifest) {
        $lines = Get-Content -LiteralPath $manifest
        # Only names install.ps1 writes are accepted, whatever the file says.
        $dirs = $lines | Where-Object { $_ -match '^dir (bin|lib|tools)$' } | ForEach-Object { $_.Substring(4) }
        $files = $lines | Where-Object { $_ -eq "file uninstall.ps1" } | ForEach-Object { $_.Substring(5) }
        $pathAdded = $lines -contains "path-added 1"
        $createdRoot = $lines -contains "created-root 1"
    } elseif ($purge -and ((Test-Path (Join-Path $installDir "cache")) -or (Test-Path (Join-Path $installDir "pkg")) -or (Test-Path (Join-Path $installDir "catalog")))) {
        # Already uninstalled; -Purge still clears what the first run kept.
        $createdRoot = $true
    } else {
        Fail "no ADM installation found at $installDir"
    }

    if ($dirs.Count -gt 0) { Status "Removing ADM from $installDir..." }
    foreach ($part in $dirs) { Remove-Item -LiteralPath (Join-Path $installDir $part) -Recurse -Force -ErrorAction SilentlyContinue }
    if (Test-Path -LiteralPath $manifest) { Remove-Item -LiteralPath $manifest -Force }

    if ($pathAdded) {
        $bin = Join-Path $installDir "bin"
        $userPath = [Environment]::GetEnvironmentVariable("Path", "User")
        if ($userPath) {
            $kept = $userPath.Split(";") | Where-Object { $_ -ne "" -and $_.TrimEnd("\") -ine $bin.TrimEnd("\") }
            [Environment]::SetEnvironmentVariable("Path", ($kept -join ";"), "User")
            Status "Removed $bin from the user PATH"
        }
    }

    if ($purge) {
        foreach ($part in "cache", "pkg", "catalog") {
            $target = Join-Path $installDir $part
            if (Test-Path -LiteralPath $target) {
                Remove-Item -LiteralPath $target -Recurse -Force
                Status "Removed $target"
            }
        }
    } else {
        foreach ($part in "cache", "pkg", "catalog") {
            if (Test-Path -LiteralPath (Join-Path $installDir $part)) {
                Status "Kept $installDir\$part (-Purge removes it)"
            }
        }
    }

    # The script removes itself last: PowerShell has read it already.
    foreach ($f in $files) { Remove-Item -LiteralPath (Join-Path $installDir $f) -Force -ErrorAction SilentlyContinue }
    if ($createdRoot -and (Test-Path -LiteralPath $installDir) -and -not (Get-ChildItem -LiteralPath $installDir -Force)) {
        Remove-Item -LiteralPath $installDir -Force
    }
    Status "ADM removed"
}

Uninstall-Adm $Purge.IsPresent
