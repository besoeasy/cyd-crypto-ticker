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
bitcoin, ethereum, solana, the-open-network
```

Up to **8 coins** are supported. Prices refresh every 60 seconds and rotate every 8 seconds.

## Monitor Serial Output

```bash
pio device monitor
```
