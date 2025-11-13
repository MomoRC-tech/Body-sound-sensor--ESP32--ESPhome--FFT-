# ESP32 Vibration FFT Analyzer - Quick Setup Script
# Run this script in PowerShell to set up the project

Write-Host "=== ESP32 Vibration FFT Analyzer Setup ===" -ForegroundColor Cyan
Write-Host ""

# Check if running in project directory
if (-not (Test-Path "body_sound_sensor.yaml")) {
    Write-Host "ERROR: Please run this script from the project root directory!" -ForegroundColor Red
    Write-Host "Expected file 'body_sound_sensor.yaml' not found." -ForegroundColor Red
    exit 1
}

# Step 1: Check Python installation
Write-Host "[1/7] Checking Python installation..." -ForegroundColor Yellow
try {
    $pythonVersion = python --version 2>&1
    Write-Host "  ✓ $pythonVersion" -ForegroundColor Green
} catch {
    Write-Host "  ✗ Python not found! Please install Python 3.8+ first." -ForegroundColor Red
    Write-Host "    Download from: https://www.python.org/downloads/" -ForegroundColor Yellow
    exit 1
}

# Step 2: Check/Install ESPHome
Write-Host "[2/7] Checking ESPHome installation..." -ForegroundColor Yellow
try {
    $esphomeVersion = esphome version 2>&1
    if ($LASTEXITCODE -eq 0) {
        Write-Host "  ✓ ESPHome already installed" -ForegroundColor Green
    } else {
        throw "Not installed"
    }
} catch {
    Write-Host "  Installing latest ESPHome..." -ForegroundColor Yellow
    pip install esphome
    if ($LASTEXITCODE -eq 0) {
        $ver = esphome version 2>&1
        Write-Host "  ✓ Installed $ver" -ForegroundColor Green
    } else {
        Write-Host "  ✗ Failed to install ESPHome" -ForegroundColor Red
        exit 1
    }
}

# Step 3: Create secrets.yaml if it doesn't exist
Write-Host "[3/7] Checking secrets.yaml..." -ForegroundColor Yellow
if (-not (Test-Path "secrets.yaml")) {
    Write-Host "  Creating secrets.yaml from template..." -ForegroundColor Yellow
    Copy-Item "secrets.yaml.example" "secrets.yaml"
    
    # Generate API key
    Write-Host "  Generating API encryption key..." -ForegroundColor Yellow
    $bytes = New-Object byte[] 32
    [System.Security.Cryptography.RandomNumberGenerator]::Create().GetBytes($bytes)
    $apiKey = [Convert]::ToBase64String($bytes)
    
    # Update secrets.yaml with generated key
    $secretsContent = Get-Content "secrets.yaml" -Raw
    $secretsContent = $secretsContent -replace 'YOUR_32_BYTE_BASE64_KEY_HERE', $apiKey
    Set-Content "secrets.yaml" $secretsContent
    
    Write-Host "  ✓ secrets.yaml created with generated API key" -ForegroundColor Green
    Write-Host "  ⚠ IMPORTANT: Edit secrets.yaml and add your WiFi password and OTA password!" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "  Your generated API key: $apiKey" -ForegroundColor Cyan
    Write-Host ""
    
    # Open secrets.yaml in default editor
    $openFile = Read-Host "  Open secrets.yaml now to edit passwords? (Y/n)"
    if ($openFile -ne 'n') {
        Start-Process "secrets.yaml"
        Write-Host "  Waiting for you to edit secrets.yaml... Press Enter when done." -ForegroundColor Yellow
        Read-Host
    }
} else {
    Write-Host "  ✓ secrets.yaml already exists" -ForegroundColor Green
}

# Step 4: Validate secrets.yaml
Write-Host "[4/7] Validating secrets.yaml..." -ForegroundColor Yellow
$secretsContent = Get-Content "secrets.yaml" -Raw
if ($secretsContent -match "YOUR_WIFI_PASSWORD_HERE" -or $secretsContent -match "YOUR_OTA_PASSWORD_HERE") {
    Write-Host "  ⚠ WARNING: secrets.yaml still contains placeholder values!" -ForegroundColor Yellow
    Write-Host "    Please edit secrets.yaml with your actual passwords." -ForegroundColor Yellow
    $continue = Read-Host "  Continue anyway? (y/N)"
    if ($continue -ne 'y') {
        exit 1
    }
} else {
    Write-Host "  ✓ secrets.yaml appears configured" -ForegroundColor Green
}

# Step 5: Validate ESPHome configuration
Write-Host "[5/7] Validating ESPHome configuration..." -ForegroundColor Yellow
esphome config body_sound_sensor.yaml
if ($LASTEXITCODE -eq 0) {
    Write-Host "  ✓ Configuration is valid!" -ForegroundColor Green
} else {
    Write-Host "  ✗ Configuration validation failed" -ForegroundColor Red
    Write-Host "    Check the error messages above and fix issues in body_sound_sensor.yaml" -ForegroundColor Yellow
    exit 1
}

# Step 6: Check USB connection
Write-Host "[6/7] Checking for ESP32 USB connection..." -ForegroundColor Yellow
$comPorts = Get-PnpDevice -Class Ports | Where-Object {$_.FriendlyName -like "*USB*" -or $_.FriendlyName -like "*UART*" -or $_.FriendlyName -like "*Serial*"}
if ($comPorts) {
    Write-Host "  ✓ Found USB/Serial devices:" -ForegroundColor Green
    $comPorts | ForEach-Object { Write-Host "    - $($_.FriendlyName)" -ForegroundColor Cyan }
} else {
    Write-Host "  ⚠ No USB/Serial devices found" -ForegroundColor Yellow
    Write-Host "    Make sure ESP32 is connected via USB" -ForegroundColor Yellow
    Write-Host "    You may need to install CP210x drivers: https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers" -ForegroundColor Yellow
}

Write-Host ""
Write-Host "[7/7] Setup complete!" -ForegroundColor Green
Write-Host ""

# Ask if user wants to compile/upload now
Write-Host "Next steps:" -ForegroundColor Cyan
Write-Host "  1. Connect ESP32 to computer via USB" -ForegroundColor White
Write-Host "  2. Run: esphome upload body_sound_sensor.yaml" -ForegroundColor White
Write-Host "  3. Monitor logs: esphome logs body_sound_sensor.yaml" -ForegroundColor White
Write-Host ""

$upload = Read-Host "Do you want to compile and upload now? (y/N)"
if ($upload -eq 'y') {
    Write-Host ""
    Write-Host "Compiling firmware (this may take 5-10 minutes on first run)..." -ForegroundColor Yellow
    esphome compile body_sound_sensor.yaml
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host ""
        Write-Host "Compilation successful! Now uploading..." -ForegroundColor Green
        esphome upload body_sound_sensor.yaml
        
        if ($LASTEXITCODE -eq 0) {
            Write-Host ""
            Write-Host "Upload complete! Monitoring logs..." -ForegroundColor Green
            Write-Host "Press Ctrl+C to stop monitoring" -ForegroundColor Yellow
            Write-Host ""
            Start-Sleep -Seconds 2
            esphome logs body_sound_sensor.yaml
        } else {
            Write-Host ""
            Write-Host "Upload failed. Check the error messages above." -ForegroundColor Red
            Write-Host "Try running manually: esphome upload body_sound_sensor.yaml" -ForegroundColor Yellow
        }
    } else {
        Write-Host ""
        Write-Host "Compilation failed. Check the error messages above." -ForegroundColor Red
    }
} else {
    Write-Host ""
    Write-Host "Setup complete! Run 'esphome upload body_sound_sensor.yaml' when ready." -ForegroundColor Green
}

Write-Host ""
Write-Host "For detailed instructions, see: docs\SETUP.md" -ForegroundColor Cyan
Write-Host "For hardware wiring, see: docs\HARDWARE.md" -ForegroundColor Cyan