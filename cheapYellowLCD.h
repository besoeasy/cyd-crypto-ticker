#ifndef CHEAPYELLOWLCD_H
#define CHEAPYELLOWLCD_H

#include "projectDisplay.h"

#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <SPI.h>
#include <lvgl.h>

// ── Touch hardware pins ───────────────────────────────────────────────────────
#define TOUCH_CS_PIN   33
#define TOUCH_CLK_PIN  25
#define TOUCH_DIN_PIN  32
#define TOUCH_DO_PIN   39

// Raw ADC calibration for XPT2046
#define TOUCH_X_MIN  200
#define TOUCH_X_MAX  3900

// ── Hardware instances ────────────────────────────────────────────────────────
TFT_eSPI   tft = TFT_eSPI();
SPIClass   touchSPI(VSPI);
XPT2046_Touchscreen ts(TOUCH_CS_PIN);

// ── LVGL draw buffer (320 × 20 lines = 12.5 KB) ──────────────────────────────
static lv_disp_draw_buf_t _draw_buf;
static lv_color_t         _lv_buf[320 * 20];

// ── LVGL flush callback ───────────────────────────────────────────────────────
static void _lv_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p)
{
    uint32_t w = area->x2 - area->x1 + 1;
    uint32_t h = area->y2 - area->y1 + 1;
    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    // LV_COLOR_16_SWAP 1 -> bytes already in ILI9341 order; no TFT swap needed
    tft.pushColors((uint16_t *)color_p, w * h, false);
    tft.endWrite();
    lv_disp_flush_ready(drv);
}

// ── Coin theme ────────────────────────────────────────────────────────────────
struct CoinTheme
{
    uint32_t    hex;
    const char *symbol;
    const char *name;
};

static CoinTheme _getCoinTheme(const String &id)
{
    if (id == "bitcoin")           return {0xF7931A, "BTC",  "Bitcoin"};
    if (id == "ethereum")          return {0x627EEA, "ETH",  "Ethereum"};
    if (id == "solana")            return {0x9945FF, "SOL",  "Solana"};
    if (id == "the-open-network")  return {0x0098EA, "TON",  "Toncoin"};
    if (id == "binancecoin")       return {0xF3B93A, "BNB",  "BNB"};
    if (id == "ripple")            return {0x346CDB, "XRP",  "XRP"};
    if (id == "dogecoin")          return {0xC2A633, "DOGE", "Dogecoin"};
    if (id == "cardano")           return {0x0E4DAA, "ADA",  "Cardano"};
    if (id == "polkadot")          return {0xE6007A, "DOT",  "Polkadot"};
    if (id == "chainlink")         return {0x2A5ADA, "LINK", "Chainlink"};

    static char symBuf[8], nameBuf[32];
    String sym = id.substring(0, 4);
    sym.toUpperCase();
    sym.toCharArray(symBuf, sizeof(symBuf));
    id.toCharArray(nameBuf, sizeof(nameBuf));
    if (nameBuf[0]) nameBuf[0] = toupper((unsigned char)nameBuf[0]);
    return {0x6B7DB3, symBuf, nameBuf};
}

// ── Price formatting ──────────────────────────────────────────────────────────
static String _fmtCommas(long n)
{
    String s = String(n), r = "";
    int len = s.length();
    for (int i = 0; i < len; i++)
    {
        if (i > 0 && (len - i) % 3 == 0) r += ",";
        r += s[i];
    }
    return r;
}

static String _fmtUSD(double p)
{
    char buf[16];
    if (p >= 1e6)   { dtostrf(p / 1e6,  1, 2, buf); return String("$") + buf + "M"; }
    if (p >= 1000)  return "$" + _fmtCommas((long)p);
    if (p >= 1)     { dtostrf(p,         1, 2, buf); return String("$") + buf; }
    dtostrf(p, 1, 6, buf); return String("$") + buf;
}

static String _fmtINR(double p)
{
    char buf[16];
    if (p >= 1e7)  { dtostrf(p / 1e7,    1, 2, buf); return String("Rs.") + buf + "Cr"; }
    if (p >= 1e5)  { dtostrf(p / 1e5,    1, 2, buf); return String("Rs.") + buf + "L"; }
    if (p >= 1000) return "Rs." + _fmtCommas((long)p);
    dtostrf(p, 1, 2, buf); return String("Rs.") + buf;
}

static String _fmtSignedUSD(double value)
{
    String amount = _fmtUSD(fabs(value));
    if (value > 0.000001) return "+" + amount;
    if (value < -0.000001) return "-" + amount;
    return amount;
}

static String _fmtSignedPercent(double value)
{
    char buf[16];
    dtostrf(fabs(value), 1, 2, buf);

    String text = buf;
    text.trim();
    if (value > 0.000001) return "+" + text + "%";
    if (value < -0.000001) return "-" + text + "%";
    return text + "%";
}

// ── Palette ───────────────────────────────────────────────────────────────────
#define C_BG      0x07071A
#define C_SURFACE 0x11112B
#define C_BORDER  0x27274A
#define C_TEXT1   0xFFFFFF
#define C_TEXT2   0xA0AAC8
#define C_TEXT3   0x525A78
#define C_TEXT4   0xD7DDF3
#define C_BLUE    0x0A84FF

// ── UI helpers ────────────────────────────────────────────────────────────────
static void _styleScreen(lv_obj_t *scr)
{
    lv_obj_set_style_bg_color(scr,     lv_color_hex(C_BG), 0);
    lv_obj_set_style_bg_opa(scr,       LV_OPA_COVER,       0);
    lv_obj_set_style_pad_all(scr,      0,                  0);
    lv_obj_set_style_border_width(scr, 0,                  0);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
}

static lv_obj_t *_mkBar(lv_obj_t *parent, int x, int y, int w, int h, uint32_t color)
{
    lv_obj_t *o = lv_obj_create(parent);
    lv_obj_set_size(o, w, h);
    lv_obj_set_pos(o, x, y);
    lv_obj_set_style_radius(o,       0,                   0);
    lv_obj_set_style_bg_color(o,     lv_color_hex(color), 0);
    lv_obj_set_style_bg_opa(o,       LV_OPA_COVER,        0);
    lv_obj_set_style_border_width(o, 0,                   0);
    lv_obj_set_style_pad_all(o,      0,                   0);
    lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE);
    return o;
}

static lv_obj_t *_mkCard(lv_obj_t *parent,
                          int x, int y, int w, int h,
                          uint32_t bgTop, uint32_t bgBot,
                          uint32_t border, int radius = 10)
{
    lv_obj_t *o = lv_obj_create(parent);
    lv_obj_set_size(o, w, h);
    lv_obj_set_pos(o, x, y);
    lv_obj_set_style_radius(o,         radius,               0);
    lv_obj_set_style_bg_color(o,       lv_color_hex(bgTop),  0);
    lv_obj_set_style_bg_grad_color(o,  lv_color_hex(bgBot),  0);
    lv_obj_set_style_bg_grad_dir(o,    LV_GRAD_DIR_VER,      0);
    lv_obj_set_style_bg_opa(o,         LV_OPA_COVER,         0);
    lv_obj_set_style_border_color(o,   lv_color_hex(border), 0);
    lv_obj_set_style_border_width(o,   1,                    0);
    lv_obj_set_style_pad_all(o,        0,                    0);
    lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE);
    return o;
}

static lv_obj_t *_mkLabel(lv_obj_t *parent,
                           const char      *text,
                           uint32_t         color,
                           const lv_font_t *font)
{
    lv_obj_t *l = lv_label_create(parent);
    lv_label_set_text(l, text);
    lv_obj_set_style_text_color(l, lv_color_hex(color), 0);
    lv_obj_set_style_text_font(l,  font,                 0);
    lv_obj_set_style_bg_opa(l,     LV_OPA_TRANSP,        0);
    return l;
}

static void _lvFlush()
{
    lv_obj_invalidate(lv_scr_act());
    for (int i = 0; i < 16; i++) lv_timer_handler();
}

static lv_obj_t *_mkOrb(lv_obj_t *parent, int x, int y, int size, uint32_t color, lv_opa_t opa)
{
    lv_obj_t *orb = lv_obj_create(parent);
    lv_obj_set_size(orb, size, size);
    lv_obj_set_pos(orb, x, y);
    lv_obj_set_style_radius(orb,       LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(orb,     lv_color_hex(color), 0);
    lv_obj_set_style_bg_opa(orb,       opa, 0);
    lv_obj_set_style_border_width(orb, 0, 0);
    lv_obj_set_style_pad_all(orb,      0, 0);
    lv_obj_clear_flag(orb, LV_OBJ_FLAG_SCROLLABLE);
    return orb;
}

static lv_obj_t *_mkPill(lv_obj_t *parent,
                         int x,
                         int y,
                         int w,
                         int h,
                         uint32_t bg,
                         uint32_t border,
                         const char *text,
                         uint32_t color,
                         const lv_font_t *font)
{
    lv_obj_t *pill = _mkCard(parent, x, y, w, h, bg, bg, border, h / 2);
    lv_obj_t *label = _mkLabel(pill, text, color, font);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    return pill;
}

static void _drawBackdrop(lv_obj_t *scr, uint32_t accent)
{
    _mkOrb(scr, -26, -10, 88, accent, LV_OPA_20);
    _mkOrb(scr, 250, 10, 64, 0x1A224A, LV_OPA_40);
    _mkOrb(scr, 230, 162, 96, accent, LV_OPA_10);
}

static void _drawPageDots(lv_obj_t *parent, int count, int index, int right, int y, uint32_t accent)
{
    int dotW = count * 10 - 4;
    int x0 = right - dotW;
    for (int i = 0; i < count; i++)
    {
        lv_obj_t *dot = lv_obj_create(parent);
        lv_obj_set_size(dot, 6, 6);
        lv_obj_set_pos(dot, x0 + i * 10, y);
        lv_obj_set_style_radius(dot,       LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_border_width(dot, 0, 0);
        lv_obj_set_style_pad_all(dot,      0, 0);
        lv_obj_set_style_bg_color(dot, lv_color_hex(i == index ? accent : C_BORDER), 0);
        lv_obj_set_style_bg_opa(dot,   LV_OPA_COVER, 0);
        lv_obj_clear_flag(dot, LV_OBJ_FLAG_SCROLLABLE);
    }
}

static lv_obj_t *_drawHeaderCard(lv_obj_t *scr,
                                 const CoinTheme &theme,
                                 int count,
                                 int index,
                                 uint32_t accent,
                                 uint32_t accentDim)
{
    lv_obj_t *hdr = _mkCard(scr, 7, 8, 306, 52, 0x12142F, 0x0A0C21, C_BORDER, 14);
    _mkOrb(hdr, -14, -12, 50, accent, LV_OPA_20);

    lv_obj_t *badge = lv_obj_create(hdr);
    lv_obj_set_size(badge, 40, 40);
    lv_obj_set_pos(badge, 8, 6);
    lv_obj_set_style_radius(badge,       LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(badge,     lv_color_hex(accentDim), 0);
    lv_obj_set_style_bg_opa(badge,       LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(badge, lv_color_hex(accent), 0);
    lv_obj_set_style_border_width(badge, 2, 0);
    lv_obj_set_style_pad_all(badge,      0, 0);
    lv_obj_clear_flag(badge, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *sym = _mkLabel(badge, theme.symbol, accent, &lv_font_montserrat_12);
    lv_obj_align(sym, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *eyebrow = _mkLabel(hdr, "LIVE MARKET", C_TEXT3, &lv_font_montserrat_12);
    lv_obj_set_pos(eyebrow, 58, 8);
    lv_obj_t *name = _mkLabel(hdr, theme.name, C_TEXT1, &lv_font_montserrat_16);
    lv_obj_set_pos(name, 58, 22);

    _drawPageDots(hdr, count, index, 292, 23, accent);
    return hdr;
}

static lv_obj_t *_drawHeroCard(lv_obj_t *scr,
                               const CryptoData &coin,
                               const CoinTheme &theme,
                               uint32_t accent)
{
    lv_obj_t *card = _mkCard(scr, 7, 70, 306, 76, 0x15183A, 0x0B0D22, accent, 18);
    _mkBar(card, 0, 0, 306, 4, accent);
    _mkOrb(card, 234, -12, 72, accent, LV_OPA_10);

    _mkPill(card, 14, 12, 54, 20, 0x0B1028, C_BORDER, "SPOT", C_TEXT3, &lv_font_montserrat_12);
    _mkPill(card, 226, 12, 66, 20, 0x0B1028, C_BORDER, theme.symbol, accent, &lv_font_montserrat_12);

    String usd = _fmtUSD(coin.usdPrice);
    lv_obj_t *price = _mkLabel(card, usd.c_str(), C_TEXT1, &lv_font_montserrat_24);
    lv_obj_set_pos(price, 14, 30);

    String inr = coin.hasInr ? _fmtINR(coin.inrPrice) : String("INR N/A");
    lv_obj_t *inrLabel = _mkLabel(card, inr.c_str(), C_TEXT2, &lv_font_montserrat_12);
    lv_obj_align(inrLabel, LV_ALIGN_BOTTOM_RIGHT, -14, -10);
    return card;
}

static uint32_t _changeColor(const PriceChangeData &change)
{
    if (!change.available) return C_TEXT3;
    return change.percent < 0 ? 0xF07070 : 0x35C46A;
}

static void _drawChangeCard(lv_obj_t *parent,
                            int x,
                            int y,
                            int w,
                            int h,
                            const char *label,
                            const PriceChangeData &change)
{
    uint32_t color = _changeColor(change);
    lv_obj_t *card = _mkCard(parent, x, y, w, h, 0x12152F, 0x0B0D21, C_BORDER, 14);
    _mkOrb(card, w - 26, -10, 34, color, LV_OPA_20);

    _mkPill(card, 8, 8, 38, 18, 0x0A1024, C_BORDER, label, C_TEXT3, &lv_font_montserrat_12);

    String percent = change.available ? _fmtSignedPercent(change.percent) : "--";
    lv_obj_t *percentLabel = _mkLabel(card, percent.c_str(), color, &lv_font_montserrat_16);
    lv_obj_set_pos(percentLabel, 8, 28);

    String amount = change.available ? _fmtSignedUSD(change.usdAmount) : "N/A";
    lv_obj_t *amountLabel = _mkLabel(card, amount.c_str(), C_TEXT2, &lv_font_montserrat_12);
    lv_obj_set_pos(amountLabel, 8, 50);
}

static void _drawMetricStrip(lv_obj_t *scr, const CryptoData &coin)
{
    _drawChangeCard(scr, 7,   156, 96, 64, "24H", coin.change24h);
    _drawChangeCard(scr, 112, 156, 96, 64, "7D",  coin.change7d);
    _drawChangeCard(scr, 217, 156, 96, 64, "1M",  coin.change30d);
}

static void _drawNavDock(lv_obj_t *scr)
{
    lv_obj_t *dock = _mkCard(scr, 44, 221, 232, 16, 0x0E1026, 0x0A0C1C, C_BORDER, 10);
    lv_obj_t *left = _mkLabel(dock, LV_SYMBOL_LEFT, C_TEXT3, &lv_font_montserrat_12);
    lv_obj_set_pos(left, 10, 2);
    lv_obj_t *refresh = _mkLabel(dock, LV_SYMBOL_REFRESH, C_TEXT3, &lv_font_montserrat_12);
    lv_obj_align(refresh, LV_ALIGN_CENTER, 0, 0);
    lv_obj_t *right = _mkLabel(dock, LV_SYMBOL_RIGHT, C_TEXT3, &lv_font_montserrat_12);
    lv_obj_align(right, LV_ALIGN_RIGHT_MID, -10, 0);

    lv_obj_t *hint = _mkLabel(scr, "Swipe zones: prev / refresh / next", C_TEXT3, &lv_font_montserrat_12);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -1);
}

static void _drawEmptyState(lv_obj_t *scr, const char *message)
{
    lv_obj_t *card = _mkCard(scr, 7, 70, 306, 150, 0x200A0A, 0x100408, 0x5A1818, 18);
    _mkPill(card, 112, 18, 82, 22, 0x341113, 0x5A1818, "OFFLINE", 0xF07070, &lv_font_montserrat_12);
    lv_obj_t *msg = _mkLabel(card, message, 0xFFD0D0, &lv_font_montserrat_16);
    lv_obj_align(msg, LV_ALIGN_CENTER, 0, -2);
}

// ── Main display class ────────────────────────────────────────────────────────
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

        lv_init();
        lv_disp_draw_buf_init(&_draw_buf, _lv_buf, NULL, 320 * 20);

        static lv_disp_drv_t disp_drv;
        lv_disp_drv_init(&disp_drv);
        disp_drv.hor_res  = 320;
        disp_drv.ver_res  = 240;
        disp_drv.flush_cb = _lv_flush_cb;
        disp_drv.draw_buf = &_draw_buf;
        lv_disp_drv_register(&disp_drv);
    }

    void drawWifiManagerMessage(WiFiManager *wm) override
    {
        lv_obj_t *scr = lv_scr_act();
        lv_obj_clean(scr);
        _styleScreen(scr);
        _drawBackdrop(scr, C_BLUE);

        _mkBar(scr, 0, 0, 320, 4, C_BLUE);

        lv_obj_t *hero = _mkCard(scr, 7, 8, 306, 48, 0x12152F, 0x090B1E, C_BORDER, 14);
        _mkPill(hero, 10, 10, 58, 20, 0x0A1024, C_BORDER, "PORTAL", C_BLUE, &lv_font_montserrat_12);
        lv_obj_t *title = _mkLabel(hero, "WiFi setup", C_TEXT1, &lv_font_montserrat_16);
        lv_obj_set_pos(title, 10, 28);
        lv_obj_t *sub = _mkLabel(hero, "Connect, configure, reboot", C_TEXT3, &lv_font_montserrat_12);
        lv_obj_set_pos(sub, 124, 14);

        lv_obj_t *nc = _mkCard(scr, 7, 66, 306, 68, 0x14173A, 0x0B0E21, C_BORDER, 14);
        _mkPill(nc, 12, 10, 74, 18, 0x0A1024, C_BORDER, "NETWORK", C_TEXT3, &lv_font_montserrat_12);

        String ssidStr = wm->getConfigPortalSSID();
        lv_obj_t *ssidL = _mkLabel(nc, ssidStr.c_str(), C_BLUE, &lv_font_montserrat_16);
        lv_obj_set_pos(ssidL, 12, 30);

        lv_obj_t *pwL = _mkLabel(nc, "Password: crypto123", C_TEXT2, &lv_font_montserrat_12);
        lv_obj_set_pos(pwL, 12, 52);

        lv_obj_t *cc = _mkCard(scr, 7, 142, 306, 70, 0x14173A, 0x0B0E21, C_BORDER, 14);
        _mkPill(cc, 12, 10, 92, 18, 0x0A1024, C_BORDER, "CONFIGURE", C_TEXT3, &lv_font_montserrat_12);
        lv_obj_t *ipL = _mkLabel(cc, "http://192.168.4.1", C_BLUE, &lv_font_montserrat_16);
        lv_obj_set_pos(ipL, 12, 28);
        lv_obj_t *hint = _mkLabel(cc, "Default IDs unlock dynamic picks", C_TEXT2, &lv_font_montserrat_12);
        lv_obj_set_pos(hint, 12, 50);

        lv_obj_t *ft = _mkLabel(scr, "Waiting for connection...", C_TEXT3, &lv_font_montserrat_12);
        lv_obj_align(ft, LV_ALIGN_BOTTOM_MID, 0, -8);

        _lvFlush();
    }

    void drawCoinScreen(const CryptoData *coins, int count, int index) override
    {
        if (count == 0) { showMessage("No coins configured"); return; }

        const CryptoData &coin = coins[index];
        CoinTheme th = _getCoinTheme(coin.id);
        uint32_t  C  = th.hex;
        uint32_t  Cd = _dim(C);

        lv_obj_t *scr = lv_scr_act();
        lv_obj_clean(scr);
        _styleScreen(scr);
        _drawBackdrop(scr, C);

        _mkBar(scr, 0, 0, 320, 4, C);
        _drawHeaderCard(scr, th, count, index, C, Cd);

        if (coin.valid)
        {
            _drawHeroCard(scr, coin, th, C);
            _drawMetricStrip(scr, coin);
        }
        else
        {
            _drawEmptyState(scr, "Price unavailable");
        }

        _drawNavDock(scr);

        _lvFlush();
    }

    void showMessage(const String &msg) override
    {
        lv_obj_t *scr = lv_scr_act();
        lv_obj_clean(scr);
        _styleScreen(scr);
        _drawBackdrop(scr, C_BLUE);

        lv_obj_t *card = _mkCard(scr, 38, 54, 244, 128, 0x11142F, 0x090B1E, C_BORDER, 18);
        _mkPill(card, 82, 16, 80, 22, 0x0A1024, C_BORDER, "WORKING", C_BLUE, &lv_font_montserrat_12);

        lv_obj_t *spin = lv_spinner_create(card, 1200, 80);
        lv_obj_set_size(spin, 52, 52);
        lv_obj_align(spin, LV_ALIGN_CENTER, 0, -8);
        lv_obj_set_style_arc_color(spin, lv_color_hex(C_BLUE),   LV_PART_INDICATOR);
        lv_obj_set_style_arc_width(spin, 4,                       LV_PART_INDICATOR);
        lv_obj_set_style_arc_color(spin, lv_color_hex(C_BORDER),  LV_PART_MAIN);
        lv_obj_set_style_arc_width(spin, 4,                       LV_PART_MAIN);
        lv_obj_set_style_bg_opa(spin,    LV_OPA_TRANSP,           LV_PART_MAIN);

        lv_obj_t *lbl = _mkLabel(card, msg.c_str(), C_TEXT4, &lv_font_montserrat_16);
        lv_obj_align(lbl, LV_ALIGN_BOTTOM_MID, 0, -18);

        _lvFlush();
    }

    TouchAction getTouchAction() override
    {
        lv_timer_handler();

        bool nowTouched = ts.touched();
        if (!nowTouched) { _wasTouched = false; return TOUCH_NONE; }
        if (_wasTouched)  return TOUCH_NONE;

        _wasTouched = true;
        TS_Point p  = ts.getPoint();
        int sx      = map(p.x, TOUCH_X_MIN, TOUCH_X_MAX, 0, screenWidth);

        if (sx < screenWidth / 3)        return TOUCH_PREV;
        if (sx > (screenWidth * 2) / 3)  return TOUCH_NEXT;
        return TOUCH_REFRESH;
    }

private:
    bool _wasTouched = false;

    static uint32_t _dim(uint32_t hex)
    {
        uint8_t r = ((hex >> 16) & 0xFF) * 30 / 100;
        uint8_t g = ((hex >>  8) & 0xFF) * 30 / 100;
        uint8_t b = ((hex      ) & 0xFF) * 30 / 100;
        return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
    }
};

#endif // CHEAPYELLOWLCD_H
