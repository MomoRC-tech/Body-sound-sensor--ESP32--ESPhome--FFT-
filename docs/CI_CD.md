# CI/CD Documentation

## Overview

This project uses **GitHub Actions** for automated testing, building, and releasing. The CI/CD pipeline ensures code quality and automates the release process.

---

## Workflows

### 1. **Continuous Integration** (`esphome-ci.yml`)

Runs automatically on every push and pull request to `main` or `develop` branches.

**Jobs:**

#### Validate
- ✅ Validates ESPHome YAML configuration
- ✅ Checks for placeholder values
- ✅ Verifies required files exist

#### Compile
- ✅ Compiles full firmware binary (verification only)
- ✅ Verifies firmware size
- ✅ Uses build caching for speed

#### Lint
- ✅ Checks C++ code for syntax errors
- ✅ Detects trailing whitespace
- ✅ Verifies required methods exist
- ✅ Runs clang-format style check

#### Test Python
- ✅ Tests spectrum analysis functions
- ✅ Validates JSON parsing
- ✅ Tests feature extraction
- ✅ Verifies classification logic

**Trigger:**
- Automatic on push/PR to `main` or `develop`

**Duration:** ~5-10 minutes (first run), ~2-3 minutes (cached)

---

### 2. **Release Workflow** (`release.yml`)

Creates versioned releases with full testing and artifact publishing.

**What it does:**

1. **Test Everything**
   - ✅ Validates ESPHome configuration
   - ✅ Checks C++ syntax
   - ✅ Compiles firmware
   - ✅ Runs Python tests
   - ✅ Verifies firmware binary integrity

2. **Version Management**
   - ✅ Automatically calculates new version (major.minor.patch)
   - ✅ Creates git tag
   - ✅ Pushes tag to repository

3. **Create Release**
   - ✅ Generates changelog from git commits
   - ✅ Creates GitHub release
   - ✅ Includes configuration and documentation files (source-only)
   - ✅ Adds hardware specs and installation instructions

**Trigger:**
- Manual via GitHub Actions UI
 - Or via `scripts/release.ps1` script (recommended)

**Inputs:**
- `version_bump`: Choose `patch`, `minor`, or `major`
- `release_notes`: Optional custom notes

**Duration:** ~10-15 minutes

---

## Using the Release System

### Method 1: Automated Script (Recommended)

```powershell
# Release a patch version (e.g., v0.1.0 → v0.1.1)
.\scripts\release.ps1 -VersionBump patch

# Release a minor version (e.g., v0.1.5 → v0.2.0)
.\scripts\release.ps1 -VersionBump minor

# Release a major version (e.g., v0.2.3 → v1.0.0)
.\scripts\release.ps1 -VersionBump major

# Add custom release notes
.\scripts\release.ps1 -VersionBump patch -ReleaseNotes "Fixed CPU load calculation bug"
```

**What the script does:**
1. ✅ Checks for uncommitted changes
2. ✅ Runs local validation tests
3. ✅ Tests Python examples
4. ✅ Calculates new version number
5. ✅ **Asks for your confirmation of the version**
6. ✅ Pushes to GitHub
7. ✅ Triggers release workflow (if GitHub CLI installed)

**Requirements:**
- Git configured and authenticated
- Latest ESPHome installed (`pip install esphome`)
- GitHub CLI (optional): `winget install GitHub.cli`

---

### Method 2: Manual GitHub Actions

1. Go to **Actions** tab in GitHub repository
2. Select **Release** workflow
3. Click **Run workflow**
4. Choose branch: `main`
5. Select version bump type: `patch`, `minor`, or `major`
6. Optionally add release notes
7. Click **Run workflow**

---

### Method 3: Via Copilot

Simply ask:

> "Push and release a patch version"

Or:

> "Create a minor release with note 'Added WiFi reconnect logic'"

GitHub Copilot will:
1. ✅ Run local tests
2. ✅ Commit any changes
3. ✅ Push to GitHub
4. ✅ Trigger release workflow
5. ✅ Monitor progress

---

## Semantic Versioning

This project follows [Semantic Versioning 2.0.0](https://semver.org/):

**Format:** `MAJOR.MINOR.PATCH` (e.g., `v1.2.3`)

### When to bump:

| Type | Description | Example |
|------|-------------|---------|
| **PATCH** | Bug fixes, minor tweaks | `v1.0.0` → `v1.0.1` |
| **MINOR** | New features (backwards compatible) | `v1.0.1` → `v1.1.0` |
| **MAJOR** | Breaking changes | `v1.5.2` → `v2.0.0` |

### Examples:

- **Patch**: Fixed RMS calculation bug
- **Patch**: Updated documentation
- **Minor**: Added new frequency band configuration
- **Minor**: Added anomaly detection feature
- **Major**: Changed JSON format (breaks compatibility)
- **Major**: Switched from MPU6050 to different sensor

---

## What Gets Tested

### ESPHome Configuration
- ✅ YAML syntax validation
- ✅ Secrets file structure (with test values)
- ✅ Custom component includes
- ✅ Sensor definitions
- ✅ WiFi/API/OTA configuration

### C++ Code
- ✅ Syntax validation
- ✅ Class and method presence
- ✅ Code style (clang-format)
- ✅ No trailing whitespace
- ✅ Compilation success

### Firmware Binary
- ✅ Successful compilation
- ✅ Binary file created
- ✅ Reasonable file size (100KB - 2MB)
- ✅ ESP32 compatibility

### Python Examples
- ✅ JSON parsing
- ✅ Feature extraction
- ✅ Band frequency calculation
- ✅ Device classification
- ✅ No import errors

---

## Build Artifacts

### What's Built
- **Firmware binary** (`.bin` file) during CI to verify successful compilation
- Compiled for ESP32-WROOM-32
- Not uploaded as an artifact; used only to validate build integrity
- Actual deployment via ESPHome Dashboard on Home Assistant

### Where to Find
- **Releases**: Source and documentation only (no firmware binaries attached)

### How to Deploy
- **Use ESPHome Dashboard** on Home Assistant
- Upload project files to ESPHome directory
- Compile and flash directly from dashboard
- CI-compiled firmware is for verification, not deployment

### Not Committed to Git
The following are **not** stored in the repository:
- ❌ `.esphome/` build directory
- ❌ `*.bin` firmware files
- ❌ `*.elf` debug files
- ❌ `.pio/` platformio cache
- ❌ `secrets.yaml` (credentials)

These are generated during CI/CD but excluded via `.gitignore`.

---

## Caching Strategy

To speed up builds, workflows cache:

1. **pip packages** (~/.cache/pip)
   - Key: OS + requirements.txt hash
   - Saves ~30 seconds

2. **ESPHome build** (.esphome/build)
   - Key: OS + YAML + C++ header hash
   - Saves ~5-8 minutes on unchanged builds

**Cache behavior:**
- Fresh on first run
- Reused on subsequent runs
- Invalidated when code changes

---

## Monitoring Workflows

### View Status Badges

Add to your README or documentation:

```markdown
[![ESPHome CI/CD](https://github.com/MomoRC-tech/Body-sound-sensor--ESP32--ESPhome--FFT-/actions/workflows/esphome-ci.yml/badge.svg)](https://github.com/MomoRC-tech/Body-sound-sensor--ESP32--ESPhome--FFT-/actions/workflows/esphome-ci.yml)
```

### GitHub Actions Page

View all workflow runs:
```
https://github.com/MomoRC-tech/Body-sound-sensor--ESP32--ESPhome--FFT-/actions
```

### GitHub CLI

```powershell
# Watch current workflow run
gh run watch

# List recent runs
gh run list

# View specific run
gh run view <run-id>
```

---

## Troubleshooting

### Workflow Fails: ESPHome Config Invalid

**Cause:** Invalid YAML or missing secrets

**Fix:**
1. Run locally: `esphome config body_sound_sensor.yaml`
2. Fix errors
3. Push changes

### Workflow Fails: Compilation Error

**Cause:** C++ syntax error in `mpu_fft_json.h`

**Fix:**
1. Check workflow logs for error details
2. Fix C++ code
3. Test locally (if possible)
4. Push changes

### Workflow Fails: Python Tests

**Cause:** Broken analysis script

**Fix:**
1. Run locally:
   ```powershell
   cd examples\python
   pip install -r requirements.txt
   python analyze_spectrum.py
   ```
2. Fix errors
3. Push changes

### Release Workflow Not Triggering

**Cause:** Missing GitHub CLI or permissions

**Fix:**
- Install GitHub CLI: `winget install GitHub.cli`
- Authenticate: `gh auth login`
- Or trigger manually via GitHub web UI

---

## Permissions

Workflows require these permissions:

- ✅ **Read** repository contents
- ✅ **Write** to create tags and releases
- ✅ **Upload** artifacts

These are configured in `.github/workflows/*.yml`:

```yaml
permissions:
  contents: write  # For creating releases and tags
```

---

## Security

### Secrets
- `secrets.yaml` is **never** committed
- Test credentials used in CI (not real)
- GitHub API token auto-provided by Actions

### What's Public
- ✅ Source code
- ✅ Workflow logs
- ✅ Release notes and documentation

### What's Private
- ❌ `secrets.yaml` (your WiFi passwords, API keys)
- ❌ GitHub Actions secrets (if configured)

---

## Best Practices

### Before Releasing

1. ✅ Test locally first
2. ✅ Commit all changes
3. ✅ Write meaningful commit messages
4. ✅ Choose appropriate version bump
5. ✅ Add release notes for significant changes

### Commit Messages

Use clear, descriptive messages:

```
✅ Good:
- "Fix CPU load calculation overflow"
- "Add support for MPU9250 sensor"
- "Update documentation for I2C configuration"

❌ Bad:
- "fix"
- "update"
- "changes"
```

### When to Release

- ✅ After fixing bugs (patch)
- ✅ After adding features (minor)
- ✅ After breaking changes (major)
- ✅ Before deploying to production

### What to Include in Release Notes

- Summary of changes
- Bug fixes
- New features
- Breaking changes
- Upgrade instructions (if needed)

---

## Future Enhancements

Potential CI/CD improvements:

1. **Hardware-in-the-Loop Testing**
   - Automated tests on real ESP32
   - Requires self-hosted runner

2. **Automated Changelogs**
   - Parse commit messages
   - Generate detailed release notes

3. **Deployment to ESPHome Dashboard**
   - Auto-update devices
   - Requires ESPHome API access

4. **Code Coverage**
   - Track test coverage
   - Generate reports

5. **Performance Benchmarks**
   - Track firmware size over time
   - Monitor CPU load regression

---

## Summary

### For Regular Development

**Every Push:**
- Automatic testing & validation
- No action needed
- Check status badge

### For Releases

**Use Script:**
```powershell
.\scripts\release.ps1 -VersionBump patch
```

**Or Ask Copilot:**
> "Push and release a patch version"

**Result:**
- ✅ All tests pass
- ✅ Version tag created
- ✅ GitHub release published (source-only)

---

**Questions?** Check workflow logs in GitHub Actions or ask Copilot!
