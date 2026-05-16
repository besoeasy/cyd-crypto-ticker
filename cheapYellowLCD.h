#ifndef CHEAPYELLOWLCD_H
#define CHEAPYELLOWLCD_H

#include "projectDisplay.h"

#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <SPI.h>

#define TOUCH_CS_PIN  33
#define TOUCH_CLK_PIN 25
#define TOUCH_DIN_PIN 32
#define TOUCH_DO_PIN  39

// Raw ADC calibration bounds for XPT2046 (adjust if touch zones feel off)
#define TOUCH_X_MIN  200
#define TOUCH_X_MAX  3900

TFT_eSPI tft = TFT_eSPI();
SPIClass touchSPI(VSPI);
XPT2046_Touchscreen ts(TOUCH_CS_PIN);

// ── Coin metadata lookup ──────────────────────────────────────────────────────
struct CoinMeta
{
  const char *symbol;
  const char *name;
  uint16_t   color;   // RGB565
};

static CoinMeta getCoinMeta(const String &id)
{
  if (id == "bitcoin")           return {"BTC",  "Bitcoin",  0xFC83}; // #F7931A
  if (id == "ethereum")          return {"ETH",  "Ethereum", 0x63FD}; // #627EEA
  if (id == "solana")            return {"SOL",  "Solana",   0x9A3F}; // #9945FF
  if (id == "the-open-network")  return {"TON",  "Toncoin",  0x04DD}; // #0098EA
  if (id == "binancecoin")       return {"BNB",  "BNB",      0xF5E0}; // #F3B93A
  if (id == "ripple")            return {"XRP",  "XRP",      0x34BD}; // #0055AA
  if (id == "dogecoin")          return {"DOGE", "Dogecoin", 0xC4A0}; // #C2A633
  if (id == "cardano")           return {"ADA",  "Cardano",  0x0277}; // #0033AD
  if (id == "polkadot")          return {"DOT",  "Polkadot", 0xE81F}; // #E6007A
  if (id == "chainlink")         return {"LINK", "Chainlink",0x095F}; // #2A5ADA
  // fallback: first 4 chars of id, uppercased
  String sym = id.substring(0, 4);
  sym.toUpperCase();
  CoinMeta fallback;
  fallback.symbol = nullptr; // handled below
  fallback.name   = nullptr;
  fallback.color  = 0x7BEF;  // TFT_DARKGREY
  // We can't return a pointer to a local String, so use a static buffer
  static char symBuf[8];
  static char nameBuf[32];
  sym.toCharArray(symBuf, sizeof(symBuf));
  id.toCharArray(nameBuf, sizeof(nameBuf));
  nameBuf[0] = toupper(nameBuf[0]); // capitalise first letter
  return {symBuf, nameBuf, 0x7BEF};
}

// ── Price formatting ──────────────────────────────────────────────────────────
static String formatWithCommas(long n)
{
  String s = String(n);
  int len = s.length();
  String result = "";
  for (int i = 0; i < len; i++)
  {
    if (i > 0 && (len - i) % 3 == 0)
      result += ",";
    result += s[i];
  }
  return result;
}

static String formatUSD(double price)
{
  char buf[16];
  if (price >= 1000000)
  {
    dtostrf(price / 1000000.0, 1, 2, buf);
    return String("$") + buf + "M";
  }
  if (price >= 1000)
    return "$" + formatWithCommas((long)price);
  if (price >= 1)
  {
    dtostrf(price, 1, 2, buf);
    return String("$") + buf;
  }
  dtostrf(price, 1, 6, buf);
  return String("$") + buf;
}

static String formatINR(double price)
{
  char buf[16];
  if (price >= 10000000) // >= 1 crore
  {
    dtostrf(price / 10000000.0, 1, 2, buf);
    return String("Rs.") + buf + "Cr";
  }
  if (price >= 100000) // >= 1 lakh
  {
    dtostrf(price / 100000.0, 1, 2, buf);
    return String("Rs.") + buf + "L";
  }
  if (price >= 1000)
    return "Rs." + formatWithCommas((long)price);
  dtostrf(price, 1, 2, buf);
  return String("Rs.") + buf;
}

// ── Display class ─────────────────────────────────────────────────────────────
class CheapYellowDisplay : public ProjectDisplay
{
public:
  void displaySetup() override
  {
    setWidth(320);
    setHeight(240);

    tft.init();
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK);

    touchSPI.begin(TOUCH_CLK_PIN, TOUCH_DO_PIN, TOUCH_DIN_PIN, TOUCH_CS_PIN);
    ts.begin(touchSPI);
    ts.setRotation(1);
  }

  void drawWifiManagerMessage(WiFiManager *myWiFiManager) override
  {
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_ORANGE, TFT_BLACK);
    tft.drawCentreString("Crypto Tracker - Config", screenCenterX, 5, 2);

    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString("Connect to WiFi AP:", 5, 35, 2);
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.drawString(myWiFiManager->getConfigPortalSSID(), 15, 55, 2);

    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString("Password:", 5, 80, 2);
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.drawString("crypto123", 15, 100, 2);

    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString("Then open http://192.168.4.1", 5, 130, 2);
    tft.drawString("to set your coin list", 5, 150, 2);
    tft.drawString("e.g. bitcoin,ethereum,solana", 5, 170, 1);
  }

  // Draw a single-coin detail screen and navigation dots
  void drawCoinScreen(const CryptoData *coins, int count, int index) override
  {
    if (count == 0)
    {
      showMessage("No coins configured");
      return;
    }

    const CryptoData &coin = coins[index];
    CoinMeta meta = getCoinMeta(coin.id);

    tft.fillScreen(TFT_BLACK);

    // ── Coloured header bar ──
    tft.fillRect(0, 0, screenWidth, 26, meta.color);
    tft.setTextColor(TFT_WHITE, meta.color);
    tft.drawCentreString("CRYPTO TRACKER", screenCenterX, 5, 2);

    // Navigation dots (top-right, inside header)
    const int DOT_R      = 4;
    const int DOT_SPACE  = 12;
    int dotsW  = (count - 1) * DOT_SPACE + DOT_R * 2;
    int dotX0  = screenWidth - dotsW - 6;
    for (int i = 0; i < count; i++)
    {
      uint16_t dc = (i == index) ? TFT_WHITE : 0x8410;
      tft.fillCircle(dotX0 + i * DOT_SPACE + DOT_R, 13, DOT_R, dc);
    }

    // ── Large logo circle ──
    const int LOGO_X = screenCenterX;
    const int LOGO_Y = 88;
    const int LOGO_R = 50;
    tft.fillCircle(LOGO_X, LOGO_Y, LOGO_R, meta.color);
    tft.drawCircle(LOGO_X, LOGO_Y, LOGO_R, TFT_WHITE);
    // Symbol centred inside (font 4 = ~26 px tall)
    tft.setTextColor(TFT_WHITE, meta.color);
    tft.drawCentreString(meta.symbol, LOGO_X, LOGO_Y - 13, 4);

    // ── Coin name ──
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawCentreString(meta.name, screenCenterX, 147, 2);

    // ── Prices ──
    if (coin.valid)
    {
      tft.setTextColor(TFT_YELLOW, TFT_BLACK);
      tft.drawCentreString(formatUSD(coin.usdPrice), screenCenterX, 168, 4);

      tft.setTextColor(TFT_CYAN, TFT_BLACK);
      tft.drawCentreString(formatINR(coin.inrPrice), screenCenterX, 198, 4);
    }
    else
    {
      tft.setTextColor(TFT_RED, TFT_BLACK);
      tft.drawCentreString("Price unavailable", screenCenterX, 183, 2);
    }

    // ── Bottom touch-hint bar ──
    tft.drawFastHLine(0, 218, screenWidth, TFT_DARKGREY);
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.drawString("< prev", 5, 225, 1);
    tft.drawCentreString("tap: refresh", screenCenterX, 225, 1);
    tft.drawRightString("next >", 315, 225, 1);
  }

  void showMessage(const String &msg) override
  {
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawCentreString(msg, screenCenterX, screenHeight / 2 - 8, 2);
  }

  // Returns PREV/NEXT/REFRESH based on which screen-third was tapped,
  // debounced so each press fires only once.
  TouchAction getTouchAction() override
  {
    bool nowTouched = ts.touched();
    if (!nowTouched)
    {
      _wasTouched = false;
      return TOUCH_NONE;
    }
    if (_wasTouched)
      return TOUCH_NONE; // already consumed this press

    _wasTouched = true;
    TS_Point p = ts.getPoint();
    // Map raw x (TOUCH_X_MIN..TOUCH_X_MAX) → screen x (0..screenWidth)
    int sx = map(p.x, TOUCH_X_MIN, TOUCH_X_MAX, 0, screenWidth);
    if (sx < screenWidth / 3)
      return TOUCH_PREV;
    if (sx > (screenWidth * 2 / 3))
      return TOUCH_NEXT;
    return TOUCH_REFRESH;
  }

private:
  bool _wasTouched = false;
};

#endif
