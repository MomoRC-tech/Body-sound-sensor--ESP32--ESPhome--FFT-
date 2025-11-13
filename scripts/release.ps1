# Release Helper Script
# This script helps prepare and push code for release

param(
    [Parameter(Mandatory=$false)]
    [ValidateSet('patch', 'minor', 'major')]
    [string]$VersionBump = 'patch',
    
    [Parameter(Mandatory=$false)]
    [string]$ReleaseNotes = ""
)

Write-Host "=== ESP32 Vibration FFT - Release Helper ===" -ForegroundColor Cyan
Write-Host ""

# Check if we're in the right directory
if (-not (Test-Path "body_sound_sensor.yaml")) {
    Write-Host "ERROR: Please run this script from the project root directory!" -ForegroundColor Red
    exit 1
}

# Check for uncommitted changes
Write-Host "[1/5] Checking for uncommitted changes..." -ForegroundColor Yellow
$gitStatus = git status --porcelain
if ($gitStatus) {
    Write-Host "⚠ You have uncommitted changes:" -ForegroundColor Yellow
    git status --short
    Write-Host ""
    $continue = Read-Host "Do you want to commit these changes? (y/N)"
    if ($continue -eq 'y') {
        $commitMessage = Read-Host "Enter commit message"
        git add .
        git commit -m "$commitMessage"
        Write-Host "✓ Changes committed" -ForegroundColor Green
    } else {
        Write-Host "Please commit or stash changes before releasing" -ForegroundColor Yellow
        exit 1
    }
} else {
    Write-Host "✓ No uncommitted changes" -ForegroundColor Green
}

# Run local tests
Write-Host ""
Write-Host "[2/5] Running local tests..." -ForegroundColor Yellow

# Check if secrets.yaml exists (for validation)
if (-not (Test-Path "secrets.yaml")) {
    Write-Host "⚠ secrets.yaml not found, creating test version..." -ForegroundColor Yellow
    Copy-Item "secrets.yaml.example" "secrets.yaml"
    $createdSecrets = $true
}

# Validate ESPHome config
Write-Host "  → Validating ESPHome configuration..." -ForegroundColor Cyan
try {
    $output = esphome config body_sound_sensor.yaml 2>&1
    if ($LASTEXITCODE -eq 0) {
        Write-Host "  ✓ ESPHome configuration valid" -ForegroundColor Green
    } else {
        Write-Host "  ✗ ESPHome configuration invalid" -ForegroundColor Red
        Write-Host $output
        exit 1
    }
} catch {
    Write-Host "  ✗ Failed to validate ESPHome config" -ForegroundColor Red
    Write-Host $_.Exception.Message
    exit 1
}

# Clean up test secrets if we created it
if ($createdSecrets) {
    Remove-Item "secrets.yaml"
}

# Check Python examples
Write-Host "  → Testing Python examples..." -ForegroundColor Cyan
try {
    Push-Location examples\python
    pip install -q -r requirements.txt
    
    python -c @"
from analyze_spectrum import parse_spectrum, extract_features
test_json = '{\"fs\":1000.0,\"n\":512,\"bin_hz\":1.953,\"rms\":0.012345,\"peak_hz\":49.2,\"bands\":[12.5,8.3,15.7,22.1,18.9,11.2,9.4,7.8,6.5,5.2,4.1,3.3,2.8,2.1,1.5,1.2]}'
spectrum = parse_spectrum(test_json)
assert spectrum is not None
features = extract_features(spectrum)
assert 'rms' in features
print('Python tests passed')
"@
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host "  ✓ Python tests passed" -ForegroundColor Green
    } else {
        Write-Host "  ✗ Python tests failed" -ForegroundColor Red
        Pop-Location
        exit 1
    }
    Pop-Location
} catch {
    Write-Host "  ✗ Python tests failed" -ForegroundColor Red
    Pop-Location
    exit 1
}

Write-Host "✓ All local tests passed" -ForegroundColor Green

# Get current version
Write-Host ""
Write-Host "[3/5] Determining version..." -ForegroundColor Yellow
$currentTag = git describe --tags --abbrev=0 2>$null
if (-not $currentTag) {
    $currentTag = "v0.0.0"
}
Write-Host "  Current version: $currentTag" -ForegroundColor Cyan

# Calculate new version
$version = $currentTag -replace '^v', ''
$parts = $version -split '\.'
$major = [int]$parts[0]
$minor = [int]$parts[1]
$patch = [int]$parts[2]

switch ($VersionBump) {
    'major' {
        $major++
        $minor = 0
        $patch = 0
    }
    'minor' {
        $minor++
        $patch = 0
    }
    'patch' {
        $patch++
    }
}

$newVersion = "v$major.$minor.$patch"
Write-Host "  New version: $newVersion ($VersionBump bump)" -ForegroundColor Green

# Confirm version with user
Write-Host ""
Write-Host "[4/6] Confirm version..." -ForegroundColor Yellow
Write-Host "  Current version: $currentTag" -ForegroundColor Cyan
Write-Host "  New version will be: $newVersion" -ForegroundColor Green
Write-Host ""
$confirmVersion = Read-Host "Is this version correct? (yes/N)"

if ($confirmVersion -ne 'yes') {
    Write-Host "Release cancelled - you can specify a different version bump type" -ForegroundColor Yellow
    Write-Host "Usage: .\scripts\release.ps1 -VersionBump [patch|minor|major]" -ForegroundColor Cyan
    exit 0
}

# Confirm release
Write-Host ""
Write-Host "[5/6] Ready to release..." -ForegroundColor Yellow
Write-Host "  Version: $currentTag → $newVersion" -ForegroundColor Cyan
Write-Host "  Type: $VersionBump" -ForegroundColor Cyan
if ($ReleaseNotes) {
    Write-Host "  Notes: $ReleaseNotes" -ForegroundColor Cyan
}
Write-Host ""
$confirm = Read-Host "Push to GitHub and trigger release workflow? (yes/N)"

if ($confirm -ne 'yes') {
    Write-Host "Release cancelled" -ForegroundColor Yellow
    exit 0
}

# Push to GitHub
Write-Host ""
Write-Host "[6/6] Pushing to GitHub..." -ForegroundColor Yellow
git push origin main

if ($LASTEXITCODE -ne 0) {
    Write-Host "✗ Failed to push to GitHub" -ForegroundColor Red
    exit 1
}

Write-Host "✓ Code pushed to GitHub" -ForegroundColor Green

# Trigger release workflow via GitHub CLI (if available)
Write-Host ""
Write-Host "Triggering release workflow..." -ForegroundColor Yellow

$ghAvailable = Get-Command gh -ErrorAction SilentlyContinue

if ($ghAvailable) {
    Write-Host "Using GitHub CLI to trigger workflow..." -ForegroundColor Cyan
    
    $workflowInputs = "version_bump=$VersionBump"
    if ($ReleaseNotes) {
        $workflowInputs += ",release_notes=$ReleaseNotes"
    }
    
    gh workflow run release.yml -f $workflowInputs
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host "✓ Release workflow triggered!" -ForegroundColor Green
        Write-Host ""
        Write-Host "Monitor progress:" -ForegroundColor Cyan
        Write-Host "  https://github.com/MomoRC-tech/Body-sound-sensor--ESP32--ESPhome--FFT-/actions" -ForegroundColor White
        Write-Host ""
        Write-Host "Or run: gh run watch" -ForegroundColor White
    } else {
        Write-Host "✗ Failed to trigger workflow" -ForegroundColor Red
        Write-Host "Please trigger manually via GitHub Actions web interface" -ForegroundColor Yellow
    }
} else {
    Write-Host "GitHub CLI (gh) not found" -ForegroundColor Yellow
    Write-Host "Please trigger the release workflow manually:" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "1. Go to: https://github.com/MomoRC-tech/Body-sound-sensor--ESP32--ESPhome--FFT-/actions/workflows/release.yml" -ForegroundColor White
    Write-Host "2. Click 'Run workflow'" -ForegroundColor White
    Write-Host "3. Select version bump: $VersionBump" -ForegroundColor White
    if ($ReleaseNotes) {
        Write-Host "4. Add release notes: $ReleaseNotes" -ForegroundColor White
    }
    Write-Host ""
    Write-Host "Or install GitHub CLI: winget install --id GitHub.cli" -ForegroundColor Yellow
}

Write-Host ""
Write-Host "=== Release process initiated ===" -ForegroundColor Green
Write-Host ""
Write-Host "The GitHub Actions workflow will:" -ForegroundColor Cyan
Write-Host "  ✓ Run all tests" -ForegroundColor White
Write-Host "  ✓ Validate configuration" -ForegroundColor White
Write-Host "  ✓ Compile firmware" -ForegroundColor White
Write-Host "  ✓ Create git tag: $newVersion" -ForegroundColor White
Write-Host "  ✓ Publish GitHub release (source-only)" -ForegroundColor White
