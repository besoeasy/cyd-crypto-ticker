# CYD Crypto Ticker

A crypto price tracker for the **Cheap Yellow Display (CYD)** — an ESP32 with a built-in 2.8" ILI9341 TFT touchscreen.

Fetches live prices from CoinGecko and cycles through your configured coins on-screen. Supports up to 8 total slots, with coin mix configurable on-device from a Settings panel.

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
2. Enter your WiFi credentials. The portal still accepts an optional advanced CoinGecko ID CSV override if you want it.
3. Save and the device will reboot, connect to WiFi, and start displaying prices.
4. Swipe left and right to cycle coins. Tap the middle of a coin screen to open the `SETTINGS` panel, which now opens on an icon-based home screen. Tap `Coins` or `Device` to drill into sub settings, then use `BACK` to return.

> Double-press the reset button at any time to re-open the configuration portal.

## Default Coins

```
bitcoin, ethereum, solana
```

By default the firmware uses:

- 3 base coins: bitcoin, ethereum, solana
- 3 extra random coins

The Settings panel now starts with icon-based categories. The `Coins` sub settings let you choose the base mix from the built-in popular coin list and set how many random coins to add. The `Device` sub settings let you change the price refresh interval, change the auto-rotate interval, and reopen the captive WiFi portal. Random picks are filled from CoinGecko trending and high-volume markets.

If you keep the default coin settings unchanged, startup adds:

- 1 random trending coin
- 2 random coins picked from CoinGecko's top 100 by trading volume

Up to **8 coins** are supported. Default timing is a 60 second refresh interval and an 8 second auto-rotate interval, both configurable from the Settings panel.

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
