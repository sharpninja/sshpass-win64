$ErrorActionPreference = 'Stop'

$packageName = 'sshpass-win64'
$toolsDir = "$(Split-Path -Parent $MyInvocation.MyCommand.Definition)"

# Remove the extracted files
$exePath = Join-Path $toolsDir 'sshpass.exe'
if (Test-Path $exePath) {
    Remove-Item $exePath -Force
    Write-Host "Removed $exePath"
}
