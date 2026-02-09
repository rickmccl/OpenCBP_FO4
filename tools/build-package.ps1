# build-package.ps1

$ErrorActionPreference = "Stop"

Write-Host "=== OpenCBP Packaging Ritual Starting ==="

# --- 1. Resolve paths -------------------------------------------------------

$scriptDir   = Split-Path -Parent $MyInvocation.MyCommand.Path # $solutionDir\Tools
$solutionDir = Split-Path -Parent $scriptDir          # one level up
$cbpDir      = Join-Path $solutionDir "CBPSSE"  # location of Version.h
$versionFile = Join-Path $cbpDir "version.h"   # contains version number macros
$packageSource = join-path $solutionDir "modpackage"  # mod resource files source Folder

$pkgRoot     = "C:\Games\FO4 mods\rickmccl\OCBP Packages"  # where to build packages

# --- 2. Extract version from version.h --------------------------------------

$versionText = Get-Content $versionFile -Raw

$major = [regex]::Match($versionText, 'OCBP_VERSION_MAJOR\s+(\d+)').Groups[1].Value
$minor = [regex]::Match($versionText, 'OCBP_VERSION_MINOR\s+(\d+)').Groups[1].Value
$patch = [regex]::Match($versionText, 'OCBP_VERSION_PATCH\s+(\d+)').Groups[1].Value

if (-not $major -or -not $minor -or -not $patch) {
    throw "Could not extract version macros from version.h"
}

$version = "$major.$minor.$patch"
$packageName = "OpenCBP_FO4-$version"

Write-Host "Version extracted: $version"
Write-Host "Package name: $packageName"

# --- 3. Prepare output directory --------------------------------------------

$packageDir = Join-Path $pkgRoot $packageName

if (Test-Path $packageDir) {
    Write-Host "Removing existing package directory..."
    Remove-Item $packageDir -Recurse -Force
}

Write-Host "Creating fresh package directory..."
New-Item -ItemType Directory -Path $packageDir | Out-Null

# --- 4. Copy modpackage contents --------------------------------------------

Write-Host "Copying modpackage staging folder..."
Copy-Item -Path (Join-Path $packageSource "*") -Destination $packageDir -Recurse -Force -exclude *.bak

# --- 5. Copy built DLL + PDB ------------------------------------------------

$buildDll = Join-Path $solutionDir "x64\Release\CBP.dll"
$buildPdb = Join-Path $solutionDir "x64\Release\CBP.pdb"

$targetPluginDir = Join-Path $packageDir "Data\F4SE\Plugins"

Write-Host "Adding fresh CBP.dll and CBP.pdb..."
Copy-Item $buildDll $targetPluginDir -Force
Copy-Item $buildPdb $targetPluginDir -Force

# --- 6. apply version number --------------------------------------

$filetargets="$packageDir\Data\F4SE\Plugins\OCBP.ini","$packageDir\FOMOD\info.xml"
foreach ($file in $filetargets) {
    Write-Host "Stamping version number in $file"
    (Get-Content $file) -replace "@VERSION@", $version | Set-Content $file
    }
    

# --- 7. create ZIP ------------------------------------------------

$zipPath = "$packageDir.zip"

if (Test-Path $zipPath) {
    Remove-Item $zipPath -Force
}

Write-Host "Creating ZIP archive..."
Compress-Archive -Path $packageDir -DestinationPath $zipPath

Write-Host "=== Packaging Complete ==="
Write-Host "Output:"
Write-Host "  Folder: $packageDir"
Write-Host "  ZIP:    $zipPath"