[CmdletBinding()]
param(
  [switch] $SkipInstall,
  [switch] $RunTests
)

$ErrorActionPreference = 'Stop'

function New-ApiKeyBase64 {
  [Convert]::ToBase64String([System.Security.Cryptography.RandomNumberGenerator]::GetBytes(32))
}

Write-Host "==> CI-Local: Starting" -ForegroundColor Cyan

# 1) Ensure ESPHome installed (unless skipped)
if (-not $SkipInstall) {
  Write-Host "==> Installing/Updating ESPHome in current environment" -ForegroundColor Cyan
  pip install --disable-pip-version-check -U esphome | Out-Host
}

# 2) Ensure secrets.yaml exists (do not overwrite user's file)
$secretsPath = Join-Path $PSScriptRoot '..' | Join-Path -ChildPath 'secrets.yaml' | Resolve-Path -ErrorAction SilentlyContinue
if (-not $secretsPath) {
  $repoRoot = Resolve-Path (Join-Path $PSScriptRoot '..')
  $secretsPath = Join-Path $repoRoot 'secrets.yaml'
  Write-Host "==> secrets.yaml not found, creating a minimal temporary one" -ForegroundColor Yellow
  $api = New-ApiKeyBase64
  @(
    'wifi_ssid: "ValidationSSID"',
    'wifi_password: "ValidationPassword"',
    "api_encryption_key: \"$api\"",
    'ota_password: "testpassword"'
  ) | Set-Content -Encoding UTF8 -Path $secretsPath
}
else {
  Write-Host "==> Using existing secrets.yaml at $secretsPath" -ForegroundColor Green
}

# 3) Validate config
$yaml = Join-Path (Resolve-Path (Join-Path $PSScriptRoot '..')) 'body_sound_sensor.yaml'
Write-Host "==> Validating YAML: $yaml" -ForegroundColor Cyan
esphome config $yaml | Out-Host

# 4) Compile firmware
Write-Host "==> Compiling firmware" -ForegroundColor Cyan
esphome compile $yaml | Out-Host

# 5) Locate firmware artifact and report size
$buildRoot = Join-Path (Resolve-Path (Join-Path $PSScriptRoot '..')) '.esphome\build'
$firmware = Get-ChildItem -Path $buildRoot -Recurse -Filter 'firmware.bin' -ErrorAction SilentlyContinue | Select-Object -First 1
if (-not $firmware) {
  throw "Firmware binary not found under $buildRoot"
}
$size = ($firmware | Get-Item).Length
Write-Host ("✓ Firmware: {0}" -f $firmware.FullName) -ForegroundColor Green
Write-Host ("✓ Size: {0} bytes" -f $size) -ForegroundColor Green

if ($size -lt 100000) {
  Write-Warning "Firmware size looks suspiciously small (<100KB)"
}

# 6) Optional: run Python example tests
if ($RunTests) {
  Write-Host "==> Installing Python example deps and running quick tests" -ForegroundColor Cyan
  $examplesPy = Join-Path (Resolve-Path (Join-Path $PSScriptRoot '..')) 'examples\python'
  pushd $examplesPy | Out-Null
  try {
    if (Test-Path 'requirements.txt') {
      pip install --disable-pip-version-check -r requirements.txt | Out-Host
    }
    python - <<'PY'
import sys
from analyze_spectrum import parse_spectrum, extract_features, classify_device, compute_band_frequencies

test_json = '{"fs":1000.0,"n":512,"bin_hz":1.953,"rms":0.012345,"peak_hz":49.2,"bands":[12.5,8.3,15.7,22.1,18.9,11.2,9.4,7.8,6.5,5.2,4.1,3.3,2.8,2.1,1.5,1.2]}'

spectrum = parse_spectrum(test_json)
assert spectrum is not None
assert spectrum['fs'] == 1000.0
assert spectrum['n'] == 512
features = extract_features(spectrum)
assert 'rms' in features and 'peak_hz' in features
centers, lows, highs = compute_band_frequencies(1000.0, 16)
assert len(centers) == 16 and centers[-1] < 500
assert isinstance(classify_device(spectrum), str)
print('✅ Example tests passed')
PY
  }
  finally {
    popd | Out-Null
  }
}

Write-Host "==> CI-Local: Done" -ForegroundColor Cyan
