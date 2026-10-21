# Installs ADM on Windows for the current user:
#
#   irm https://raw.githubusercontent.com/admlang/adm/main/install.ps1 | iex
#
# Downloads the latest language package from the GitHub releases, unpacks it
# into %LOCALAPPDATA%\adm (bin\, lib\, tools\) and puts its bin\ on the user's
# PATH. No administrator rights. Windows 10 version 1903 or newer, x64 (or
# Windows 11 on Arm, which runs it emulated).
#
#   $env:ADM_VERSION = "0.1"      install that release instead of the latest
#   $env:ADM_INSTALL_DIR = "..."  install somewhere other than %LOCALAPPDATA%\adm
#   $env:ADM_REPO = "owner/name"  download from another repository's releases
#   $env:ADM_PACKAGE = "..."      install this package (a local .zip or a URL)
#                                 instead of looking up a release
#
# Everything installed is listed in <install dir>\install-manifest, which
# uninstall.ps1 reads to remove exactly that and nothing else. Written for
# Windows PowerShell 5.1, which every supported Windows has.
# Everything sits in one function so a truncated download cannot run half a
# script.

function Install-Adm {
    $ErrorActionPreference = "Stop"
    $ProgressPreference = "SilentlyContinue"

    function Status($text) { Write-Host ">>> $text" }
    function Fail($text) { Write-Host "ERROR: $text" -ForegroundColor Red; throw "adm install: $text" }

    $repo = if ($env:ADM_REPO) { $env:ADM_REPO } else { "admlang/adm" }
    $installDir = if ($env:ADM_INSTALL_DIR) { $env:ADM_INSTALL_DIR } else { Join-Path $env:LOCALAPPDATA "adm" }

    # adm.exe asks Windows 10 1903's API (build 18362).
    $build = [Environment]::OSVersion.Version.Build
    if ([Environment]::OSVersion.Platform -ne "Win32NT" -or $build -lt 18362) {
        Fail "ADM needs Windows 10 version 1903 (build 18362) or newer; this is build $build"
    }
    $arch = $env:PROCESSOR_ARCHITECTURE
    if ($env:PROCESSOR_ARCHITEW6432) { $arch = $env:PROCESSOR_ARCHITEW6432 }
    if ($arch -ne "AMD64" -and $arch -ne "ARM64") {
        Fail "packages are published for x64 Windows (this machine is $arch)"
    }
    if ($arch -eq "ARM64") {
        Status "Windows on Arm runs the x64 package emulated"
    }

    # Windows PowerShell 5.1 offers TLS 1.0 by default, which GitHub refuses.
    [Net.ServicePointManager]::SecurityProtocol = [Net.ServicePointManager]::SecurityProtocol -bor [Net.SecurityProtocolType]::Tls12

    $temp = Join-Path ([IO.Path]::GetTempPath()) ("adm-install-" + [Guid]::NewGuid().ToString("N"))
    New-Item -ItemType Directory -Path $temp | Out-Null
    try {
        # The release carries adm-<version>-windows-amd64.zip; ask the API for
        # it so the version number never has to be known in advance.
        if ($env:ADM_PACKAGE) {
            $assetUrl = $env:ADM_PACKAGE
        } else {
            $releaseUrl = if ($env:ADM_VERSION) { "https://api.github.com/repos/$repo/releases/tags/v$($env:ADM_VERSION)" } else { "https://api.github.com/repos/$repo/releases/latest" }
            Status "Looking up the release..."
            $release = Invoke-RestMethod -Uri $releaseUrl -Headers @{ "User-Agent" = "adm-install" }
            $asset = $release.assets | Where-Object { $_.name -match '^adm-.*-windows-amd64\.zip$' } | Select-Object -First 1
            if (-not $asset) {
                $which = if ($env:ADM_VERSION) { "release v$($env:ADM_VERSION) of $repo" } else { $repo }
                Fail "no windows-amd64 package in $which"
            }
            $assetUrl = $asset.browser_download_url
        }

        $archive = Join-Path $temp ([IO.Path]::GetFileName($assetUrl))
        if (Test-Path -LiteralPath $assetUrl -PathType Leaf) {
            Copy-Item -LiteralPath $assetUrl -Destination $archive
        } else {
            Status "Downloading $([IO.Path]::GetFileName($assetUrl))..."
            Invoke-WebRequest -Uri $assetUrl -OutFile $archive -UseBasicParsing
        }

        Status "Unpacking..."
        $unpacked = Join-Path $temp "unpacked"
        # Expand-Archive took five minutes over the package's files on Windows 11;
        # the .NET call it wraps takes seconds.
        Add-Type -AssemblyName System.IO.Compression.FileSystem
        [IO.Compression.ZipFile]::ExtractToDirectory($archive, $unpacked)
        $package = Get-ChildItem -LiteralPath $unpacked -Directory | Where-Object { $_.Name -like "adm-*" } | Select-Object -First 1
        if (-not $package -or -not (Test-Path -LiteralPath (Join-Path $package.FullName "bin\adm.exe"))) {
            Fail "the package has no bin\adm.exe"
        }
        # A file downloaded by a browser carries the mark of the web, which
        # SmartScreen asks about on every start of adm.exe and its tools.
        Get-ChildItem -LiteralPath $package.FullName -Recurse -File | Unblock-File

        # A development setup may link these at a checkout; never write over that.
        foreach ($part in "bin", "lib", "tools") {
            $item = Get-Item -LiteralPath (Join-Path $installDir $part) -Force -ErrorAction SilentlyContinue
            if ($item -and ($item.Attributes -band [IO.FileAttributes]::ReparsePoint)) {
                Fail "$($item.FullName) is a link (a development setup?); remove it or set ADM_INSTALL_DIR"
            }
        }

        Status "Installing to $installDir..."
        $manifest = Join-Path $installDir "install-manifest"
        # A reinstall keeps what the first install recorded, so uninstall
        # still knows the folder and the PATH entry were ours.
        $createdRoot = -not (Test-Path -LiteralPath $installDir)
        $pathAdded = $false
        if (Test-Path -LiteralPath $manifest) {
            $old = Get-Content -LiteralPath $manifest
            if ($old -contains "created-root 1") { $createdRoot = $true }
            if ($old -contains "path-added 1") { $pathAdded = $true }
        }
        New-Item -ItemType Directory -Path $installDir -Force | Out-Null
        # Only the package's own folders are replaced; cache\, pkg\ and
        # catalog\ belong to the user and stay.
        foreach ($part in "bin", "lib", "tools") {
            $target = Join-Path $installDir $part
            if (Test-Path -LiteralPath $target) { Remove-Item -LiteralPath $target -Recurse -Force }
            $source = Join-Path $package.FullName $part
            if (Test-Path -LiteralPath $source) { Copy-Item -LiteralPath $source -Destination $target -Recurse }
        }
        $uninstall = Join-Path $package.FullName "uninstall.ps1"
        if (Test-Path -LiteralPath $uninstall) {
            Copy-Item -LiteralPath $uninstall -Destination (Join-Path $installDir "uninstall.ps1") -Force
        }

        $bin = Join-Path $installDir "bin"
        $userPath = [Environment]::GetEnvironmentVariable("Path", "User")
        $entries = @()
        # @(): one entry would come back as a string, and + would join it to
        # $bin with no separator.
        if ($userPath) { $entries = @($userPath.Split(";") | Where-Object { $_ -ne "" }) }
        if (-not ($entries | Where-Object { $_.TrimEnd("\") -ieq $bin.TrimEnd("\") })) {
            [Environment]::SetEnvironmentVariable("Path", (($entries + $bin) -join ";"), "User")
            $pathAdded = $true
            Status "Added $bin to the user PATH; open a new terminal to use it"
        }
        if (-not (($env:Path.Split(";")) | Where-Object { $_.TrimEnd("\") -ieq $bin.TrimEnd("\") })) {
            $env:Path = "$bin;$env:Path"
        }

        $version = (Get-Content -LiteralPath (Join-Path $package.FullName "VERSION") -ErrorAction SilentlyContinue)
        if (-not $version) { $version = $package.Name -replace '^adm-', '' -replace '-windows-amd64$', '' }
        $lines = @(
            "# Written by install.ps1; uninstall.ps1 removes exactly what is listed here.",
            "version $version",
            "created-root $(if ($createdRoot) { 1 } else { 0 })",
            "path-added $(if ($pathAdded) { 1 } else { 0 })"
        )
        foreach ($part in "bin", "lib", "tools") {
            if (Test-Path -LiteralPath (Join-Path $installDir $part)) { $lines += "dir $part" }
        }
        if (Test-Path -LiteralPath (Join-Path $installDir "uninstall.ps1")) { $lines += "file uninstall.ps1" }
        Set-Content -LiteralPath $manifest -Value $lines -Encoding ASCII

        $installed = & (Join-Path $bin "adm.exe") version 2>$null | Select-Object -First 1
        if (-not $installed) { $installed = "adm" }
        Status "Installed $installed"
        Status "Run 'adm --help' to get started; 'adm doctor' checks the toolchain."
        # Windows runs no script file by its path under the default execution
        # policy; the irm line the website shows needs none.
        Status "To remove ADM later: powershell -ExecutionPolicy Bypass -File '$(Join-Path $installDir "uninstall.ps1")'"
    } finally {
        Remove-Item -LiteralPath $temp -Recurse -Force -ErrorAction SilentlyContinue
    }
}

Install-Adm
