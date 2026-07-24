# Copy Web Files from data\www to D:\www and SD Card
# This script syncs web files from project to D:\ and SD card

param(
    [Parameter(Mandatory=$false)]
    [string]$SDDrive = "E:"
)

$SourceDir = "data\www"
$DestDirD = "D:\www"
$DestDirSD = "$SDDrive\www"

Write-Host "========================================" -ForegroundColor Cyan
Write-Host " Deploy Web Files" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# Check if source exists
if (-not (Test-Path $SourceDir)) {
    Write-Host "ERROR: Source directory not found: $SourceDir" -ForegroundColor Red
    exit 1
}

# Copy to D:\www first
Write-Host "Step 1: Copying to D:\www..." -ForegroundColor Yellow
if (-not (Test-Path $DestDirD)) {
    New-Item -ItemType Directory -Path $DestDirD -Force | Out-Null
}
Copy-Item -Path "$SourceDir\*" -Destination $DestDirD -Recurse -Force
Write-Host "[OK] Copied to D:\www" -ForegroundColor Green

# Copy to SD card
Write-Host ""
Write-Host "Step 2: Copying to SD card ($SDDrive)..." -ForegroundColor Yellow
if (-not (Test-Path $SDDrive)) {
    Write-Host "WARNING: SD card not found at $SDDrive - skipping" -ForegroundColor Yellow
    Write-Host "Available drives:" -ForegroundColor Gray
    Get-PSDrive -PSProvider FileSystem | Where-Object { $_.Used -gt 0 } | Format-Table Name, Root
} else {
    if (-not (Test-Path $DestDirSD)) {
        New-Item -ItemType Directory -Path $DestDirSD -Force | Out-Null
    }
    try {
        Copy-Item -Path "$SourceDir\*" -Destination $DestDirSD -Recurse -Force
        Write-Host "[OK] Copied to SD card" -ForegroundColor Green
    } catch {
        Write-Host "ERROR: Failed to copy to SD card" -ForegroundColor Red
        Write-Host $_.Exception.Message -ForegroundColor Red
    }
}

# Show summary
Write-Host ""
Write-Host "Deployed files:" -ForegroundColor Cyan
if (Test-Path $DestDirD) {
    Write-Host "  D:\www\" -ForegroundColor White
    Get-ChildItem -Path $DestDirD -Recurse -File | ForEach-Object {
        Write-Host "    $($_.Name)" -ForegroundColor Gray
    }
}
if (Test-Path $DestDirSD) {
    Write-Host "  $DestDirSD\" -ForegroundColor White
    Get-ChildItem -Path $DestDirSD -Recurse -File | ForEach-Object {
        Write-Host "    $($_.Name)" -ForegroundColor Gray
    }
}

Write-Host ""
Write-Host "[OK] Deployment complete!" -ForegroundColor Green
Write-Host ""
