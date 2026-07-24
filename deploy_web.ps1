# Deploy Web Files to SD Card
# Usage: .\deploy_web.ps1 E:
# Where E: is your SD card drive letter

param(
    [Parameter(Mandatory=$false)]
    [string]$SDDrive = "E:"
)

$SourceDir = "data\www"
$DestDir = "$SDDrive\www"

Write-Host "========================================" -ForegroundColor Cyan
Write-Host " ESP32-S3 Web Files Deployment" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# Check if source directory exists
if (-not (Test-Path $SourceDir)) {
    Write-Host "ERROR: Source directory not found: $SourceDir" -ForegroundColor Red
    exit 1
}

# Check if SD card is accessible
if (-not (Test-Path $SDDrive)) {
    Write-Host "ERROR: SD card not found at $SDDrive" -ForegroundColor Red
    Write-Host "Available drives:" -ForegroundColor Yellow
    Get-PSDrive -PSProvider FileSystem | Where-Object { $_.Used -gt 0 } | Format-Table Name, Root, @{Label="Size (GB)"; Expression={[math]::Round($_.Used/1GB + $_.Free/1GB, 2)}}
    exit 1
}

# Create destination directory if it doesn't exist
if (-not (Test-Path $DestDir)) {
    Write-Host "Creating directory: $DestDir" -ForegroundColor Yellow
    New-Item -ItemType Directory -Path $DestDir -Force | Out-Null
}

# Copy files
Write-Host "Copying files from $SourceDir to $DestDir..." -ForegroundColor Green
try {
    Copy-Item -Path "$SourceDir\*" -Destination $DestDir -Recurse -Force
    Write-Host "✓ Files copied successfully!" -ForegroundColor Green
} catch {
    Write-Host "ERROR: Failed to copy files" -ForegroundColor Red
    Write-Host $_.Exception.Message -ForegroundColor Red
    exit 1
}

# List deployed files
Write-Host ""
Write-Host "Deployed files:" -ForegroundColor Cyan
Get-ChildItem -Path $DestDir -Recurse | ForEach-Object {
    $relativePath = $_.FullName.Replace($DestDir, "").TrimStart('\')
    Write-Host "  $relativePath" -ForegroundColor Gray
}

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Deployment complete!" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "SD Card Structure:" -ForegroundColor Yellow
Write-Host "  $SDDrive\" -ForegroundColor White
Write-Host "    └── www\" -ForegroundColor White
Write-Host "        └── index.html" -ForegroundColor White
Write-Host ""
Write-Host "Next steps:" -ForegroundColor Yellow
Write-Host "  1. Safely eject SD card" -ForegroundColor White
Write-Host "  2. Insert into ESP32-S3 device" -ForegroundColor White
Write-Host "  3. Power on device" -ForegroundColor White
Write-Host "  4. Connect to WiFi" -ForegroundColor White
Write-Host "  5. Open http://<ESP32-IP>/ in browser" -ForegroundColor White
Write-Host ""
