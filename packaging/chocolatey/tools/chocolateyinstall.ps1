$ErrorActionPreference = 'Stop'

$packageName = 'sshpass-win64'
$toolsDir = "$(Split-Path -Parent $MyInvocation.MyCommand.Definition)"

$url64 = "https://github.com/sharpninja/sshpass-win64/releases/download/v$($env:ChocolateyPackageVersion)/sshpass-win64-$($env:ChocolateyPackageVersion).zip"

$packageArgs = @{
    packageName    = $packageName
    unzipLocation  = $toolsDir
    url64bit       = $url64
    checksum64     = '$CHECKSUM$'
    checksumType64 = 'sha256'
}

Install-ChocolateyZipPackage @packageArgs

# Create shim for sshpass.exe
$exePath = Join-Path $toolsDir 'sshpass.exe'
if (Test-Path $exePath) {
    Write-Host "sshpass.exe installed to $exePath"
}
