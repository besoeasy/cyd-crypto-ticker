# CYD Crypto Ticker

A crypto price tracker for the **Cheap Yellow Display (CYD)** — an ESP32 with a built-in 2.8" ILI9341 TFT touchscreen.

Fetches live prices from CoinGecko and cycles through your configured coins on-screen. Supports up to 8 coins, configurable via a captive-portal WiFi setup page.

## Hardware

- ESP32 with CYD (Cheap Yellow Display) — ILI9341 240×320 TFT
- USB cable for flashing

## Flash

Install [PlatformIO](https://platformio.org/), then run:

```bash
pio run -e cyd --target upload
```

or

```bash
~/.platformio/penv/bin/pio run -e cyd --target upload

```

For CYD boards with USB on the opposite side (CYD2USB), use:

```bash
pio run -e cyd2usb --target upload
```

## First Boot

1. The device creates a WiFi access point — connect to it.
2. Enter your WiFi credentials and the CoinGecko coin IDs you want to track (comma-separated, e.g. `bitcoin,ethereum,solana`).
3. Save and the device will reboot, connect to WiFi, and start displaying prices.

> Double-press the reset button at any time to re-open the configuration portal.

## Default Coins

```
bitcoin, ethereum, solana
```

If you keep the default coin IDs unchanged, startup adds:

- 1 random trending coin
- 2 random coins picked from CoinGecko's top 100 by trading volume

Up to **8 coins** are supported. Prices refresh every 60 seconds and rotate every 8 seconds.

## Monitor Serial Output

```bash
pio device monitor
```

## Web Flasher

The repo now includes a browser-based installer built with ESP Web Tools. It publishes two firmware targets:

- `cyd` for the standard Cheap Yellow Display profile
- `cyd2usb` for boards that need the alternate TFT inversion setting

Generate the local site bundle with:

```bash
chmod +x scripts/build_web_flasher_site.sh
./scripts/build_web_flasher_site.sh
```

That writes a deployable site to `.web-flasher/site/` with merged ESP32 binaries and manifests for both board variants.

To preview locally, serve that folder from `localhost`, for example:

```bash
cd .web-flasher/site
python3 -m http.server 8000
```

Then open `http://localhost:8000` in Chrome or Edge.

## GitHub Pages Deployment

The GitHub Actions workflow in `.github/workflows/web-flasher.yml` builds the firmware and publishes the installer to GitHub Pages.

1. Enable GitHub Pages for the repository and choose `GitHub Actions` as the source.
2. Push to the default branch, or run the workflow manually.
3. Open the published Pages URL to flash the board from the browser.

ESP Web Tools requires an HTTPS page or `localhost`, plus a Chromium browser with Web Serial support.
