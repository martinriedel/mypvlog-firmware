# Pushing to GitHub

## Quick Start

```bash
cd /home/user/mypvlog-firmware

# Add your GitHub remote (already done)
git remote add origin https://github.com/martinriedel/mypvlog-firmware.git

# Push to GitHub
git push -u origin main
```

## If you need to authenticate:

### Option 1: Using GitHub Personal Access Token (Recommended)

1. Go to https://github.com/settings/tokens
2. Click "Generate new token (classic)"
3. Give it a name: "MyPVLog Firmware"
4. Check scopes: `repo` (Full control of private repositories)
5. Click "Generate token"
6. Copy the token (you won't see it again!)

Then push:
```bash
git push -u origin main
# Username: martinriedel
# Password: <paste your token here>
```

### Option 2: Using SSH

```bash
# Add SSH remote instead
git remote set-url origin git@github.com:martinriedel/mypvlog-firmware.git

# Push
git push -u origin main
```

## After First Push

GitHub Actions will automatically:
- Build all 4 firmware variants
- Run tests
- Show build status on repository page

## Creating a Release

```bash
# Tag a version
git tag -a v0.1.0 -m "Initial alpha release"
git push origin v0.1.0
```

This will trigger the release workflow and create downloadable firmware binaries!
