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
    // ── Apple colour palette ──────────────────────────────────────────────
    const uint16_t CARD   = 0x2104; // #222222  elevated surface
    const uint16_t BORDER = 0x39C7; // #3A3A3C  separator
    const uint16_t GRAY2  = 0x8C72; // #8E8E93  secondary label
    const uint16_t GRAY3  = 0x630C; // #636366  tertiary label
    const uint16_t BLUE   = 0x0C3F; // #0A84FF  Apple system blue

    tft.fillScreen(TFT_BLACK);

    // 3-px blue accent bar
    tft.fillRect(0, 0, screenWidth, 3, BLUE);

    // Title
    tft.setTextColor(TFT_WHITE);
    tft.drawCentreString("WiFi Setup", screenCenterX, 14, 4);

    // ── Network card ──────────────────────────────────────────────────────
    tft.fillRoundRect(16, 52, 288, 72, 10, CARD);
    tft.drawRoundRect(16, 52, 288, 72, 10, BORDER);

    tft.setTextColor(GRAY3);
    tft.drawString("NETWORK", 28, 59, 1);
    tft.setTextColor(BLUE);
    tft.drawString(myWiFiManager->getConfigPortalSSID(), 28, 70, 2);

    tft.setTextColor(GRAY3);
    tft.drawString("PASSWORD", 28, 92, 1);
    tft.setTextColor(GRAY2);
    tft.drawString("crypto123", 28, 103, 2);

    // ── Instructions card ─────────────────────────────────────────────────
    tft.fillRoundRect(16, 136, 288, 68, 10, CARD);
    tft.drawRoundRect(16, 136, 288, 68, 10, BORDER);

    tft.setTextColor(GRAY3);
    tft.drawString("CONFIGURE AT", 28, 143, 1);
    tft.setTextColor(BLUE);
    tft.drawString("http://192.168.4.1", 28, 154, 2);
    tft.setTextColor(GRAY3);
    tft.drawString("Enter your coin list to track", 28, 176, 1);
    tft.drawString("e.g. bitcoin,ethereum,solana", 28, 187, 1);

    // Footer
    tft.setTextColor(GRAY3);
    tft.drawCentreString("Waiting for connection...", screenCenterX, 218, 1);
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
    CoinMeta meta          = getCoinMeta(coin.id);

    // ── Apple colour palette ──────────────────────────────────────────────
    const uint16_t CARD   = 0x2104; // #222222  elevated surface
    const uint16_t BORDER = 0x39C7; // #3A3A3C  separator
    const uint16_t GRAY2  = 0x8C72; // #8E8E93  secondary label
    const uint16_t GRAY3  = 0x630C; // #636366  tertiary label

    tft.fillScreen(TFT_BLACK);

    // ── 3-px coin-colour accent bar ───────────────────────────────────────
    tft.fillRect(0, 0, screenWidth, 3, meta.color);

    // ── Coin badge circle ─────────────────────────────────────────────────
    const int CX = screenCenterX;
    const int CY = 51;
    const int CR = 41;
    uint16_t dimColor = ((meta.color >> 1) & 0x7BEF); // 50% darker fill
    tft.fillCircle(CX, CY, CR, dimColor);
    tft.drawCircle(CX, CY, CR,     meta.color);        // double ring for weight
    tft.drawCircle(CX, CY, CR - 1, meta.color);
    tft.setTextColor(TFT_WHITE);
    tft.drawCentreString(meta.symbol, CX, CY - 13, 4); // symbol centred inside

    // ── Coin name ─────────────────────────────────────────────────────────
    tft.setTextColor(GRAY2);
    tft.drawCentreString(meta.name, screenCenterX, 100, 2);

    // ── Prices ────────────────────────────────────────────────────────────
    if (coin.valid)
    {
      // USD card – coin-coloured border for primary emphasis
      tft.setTextColor(GRAY3);
      tft.drawString("USD", 28, 120, 1);
      tft.fillRoundRect(16, 129, 288, 34, 8, CARD);
      tft.drawRoundRect(16, 129, 288, 34, 8, meta.color);
      tft.setTextColor(TFT_WHITE);
      tft.drawCentreString(formatUSD(coin.usdPrice), screenCenterX, 133, 4);

      // INR card – neutral border for secondary value
      tft.setTextColor(GRAY3);
      tft.drawString("INR", 28, 167, 1);
      tft.fillRoundRect(16, 176, 288, 26, 8, CARD);
      tft.drawRoundRect(16, 176, 288, 26, 8, BORDER);
      tft.setTextColor(GRAY2);
      tft.drawCentreString(formatINR(coin.inrPrice), screenCenterX, 180, 2);
    }
    else
    {
      tft.fillRoundRect(16, 120, 288, 82, 8, CARD);
      tft.drawRoundRect(16, 120, 288, 82, 8, BORDER);
      tft.setTextColor(0xF8C5); // soft warm-red
      tft.drawCentreString("Price unavailable", screenCenterX, 153, 2);
    }

    // ── Page dots ─────────────────────────────────────────────────────────
    const int DOT_R = 3;
    const int DOT_S = 10;
    int dotX0 = screenCenterX - ((count - 1) * DOT_S) / 2;
    for (int i = 0; i < count; i++)
    {
      int dx = dotX0 + i * DOT_S;
      if (i == index)
      {
        tft.fillCircle(dx, 214, DOT_R, meta.color);
      }
      else
      {
        tft.fillCircle(dx, 214, DOT_R, TFT_BLACK);  // clear
        tft.drawCircle(dx, 214, DOT_R, BORDER);     // outline only
      }
    }

    // ── Touch hints ───────────────────────────────────────────────────────
    tft.setTextColor(GRAY3);
    tft.drawString("< prev", 8, 228, 1);
    tft.drawCentreString("refresh", screenCenterX, 228, 1);
    tft.drawRightString("next >", 312, 228, 1);
  }

  void showMessage(const String &msg) override
  {
    const uint16_t CARD   = 0x2104; // #222222
    const uint16_t BORDER = 0x39C7; // #3A3A3C
    const uint16_t GRAY2  = 0x8C72; // #8E8E93

    tft.fillScreen(TFT_BLACK);
    const int cardH = 44;
    const int cardY = (screenHeight - cardH) / 2;
    tft.fillRoundRect(16, cardY, 288, cardH, 12, CARD);
    tft.drawRoundRect(16, cardY, 288, cardH, 12, BORDER);
    tft.setTextColor(GRAY2);
    tft.drawCentreString(msg, screenCenterX, cardY + (cardH - 16) / 2, 2);
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
