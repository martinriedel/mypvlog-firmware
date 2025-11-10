# MyPVLog Web Flasher

Browser-based firmware flasher using ESP Web Tools.

## Hosting

This web flasher can be hosted anywhere:

### Option 1: GitHub Pages (Free)

1. Go to repository Settings → Pages
2. Source: Deploy from branch `feature/web-flasher`
3. Folder: `/web-flasher`
4. Save

Access at: `https://martinriedel.github.io/mypvlog-firmware/`

### Option 2: Netlify/Vercel (Free)

Deploy the `web-flasher` directory to Netlify or Vercel.

### Option 3: Custom Domain (flash.mypvlog.net)

Host on your own server or use Cloudflare Pages:

```bash
# Upload web-flasher directory to server
scp -r web-flasher/* user@server:/var/www/flash.mypvlog.net/
```

## Local Testing

```bash
cd web-flasher
python3 -m http.server 8000
```

Open: http://localhost:8000

## Requirements

- Modern browser with WebSerial API support:
  - Chrome 89+
  - Edge 89+
  - Opera 75+
- ESP32/ESP8266 connected via USB

## Manifest Files

Manifest files point to GitHub releases. Create a release with firmware binaries:

```bash
# Tag a release
git tag v1.0.0
git push origin v1.0.0
```

GitHub Actions will build and attach firmware binaries automatically.

## Security

All firmware is loaded from GitHub releases via HTTPS. Manifests specify exact file paths and offsets.

## Credits

Built with [ESP Web Tools](https://esphome.github.io/esp-web-tools/) by ESPHome.
