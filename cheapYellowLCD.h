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
    lv_obj_t *card = _mkCard(parent, x, y, w, h, 0x11112B, C_BG, C_BORDER, 8);

    lv_obj_t *title = _mkLabel(card, label, C_TEXT3, &lv_font_montserrat_12);
    lv_obj_set_pos(title, 8, 6);

    String percent = change.available ? _fmtSignedPercent(change.percent) : "--";
    lv_obj_t *percentLabel = _mkLabel(card, percent.c_str(), color, &lv_font_montserrat_16);
    lv_obj_align(percentLabel, LV_ALIGN_CENTER, 0, -6);

    String amount = change.available ? _fmtSignedUSD(change.usdAmount) : "N/A";
    lv_obj_t *amountLabel = _mkLabel(card, amount.c_str(), C_TEXT2, &lv_font_montserrat_12);
    lv_obj_align(amountLabel, LV_ALIGN_BOTTOM_MID, 0, -6);
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

        _mkBar(scr, 0, 0, 320, 4, C_BLUE);

        lv_obj_t *title = _mkLabel(scr, "WiFi Setup", C_TEXT1, &lv_font_montserrat_16);
        lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

        // Network card
        lv_obj_t *nc = _mkCard(scr, 7, 40, 306, 68, 0x14143A, C_BG, C_BORDER);
        lv_obj_t *nHead = _mkLabel(nc, "NETWORK", C_TEXT3, &lv_font_montserrat_12);
        lv_obj_set_pos(nHead, 12, 8);

        String ssidStr = wm->getConfigPortalSSID();
        lv_obj_t *ssidL = _mkLabel(nc, ssidStr.c_str(), C_BLUE, &lv_font_montserrat_16);
        lv_obj_set_pos(ssidL, 12, 22);

        lv_obj_t *pHead = _mkLabel(nc, "PASSWORD", C_TEXT3, &lv_font_montserrat_12);
        lv_obj_set_pos(pHead, 12, 42);
        lv_obj_t *pwL = _mkLabel(nc, "crypto123", C_TEXT2, &lv_font_montserrat_12);
        lv_obj_set_pos(pwL, 12, 54);

        // Configure card
        lv_obj_t *cc = _mkCard(scr, 7, 116, 306, 68, 0x14143A, C_BG, C_BORDER);
        lv_obj_t *cHead = _mkLabel(cc, "CONFIGURE AT", C_TEXT3, &lv_font_montserrat_12);
        lv_obj_set_pos(cHead, 12, 8);
        lv_obj_t *ipL = _mkLabel(cc, "http://192.168.4.1", C_BLUE, &lv_font_montserrat_12);
        lv_obj_set_pos(ipL, 12, 22);
        lv_obj_t *hint = _mkLabel(cc, "Enter coin IDs: bitcoin,ethereum,solana", C_TEXT3, &lv_font_montserrat_12);
        lv_obj_set_pos(hint, 12, 44);

        lv_obj_t *ft = _mkLabel(scr, "Waiting for connection...", C_TEXT3, &lv_font_montserrat_12);
        lv_obj_align(ft, LV_ALIGN_BOTTOM_MID, 0, -6);

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

        // Accent bar
        _mkBar(scr, 0, 0, 320, 4, C);

        // Header card
        lv_obj_t *hdr = _mkCard(scr, 7, 6, 306, 46, 0x13132E, C_BG, C_BORDER);

        // Badge circle
        lv_obj_t *badge = lv_obj_create(hdr);
        lv_obj_set_size(badge, 36, 36);
        lv_obj_set_pos(badge, 5, 5);
        lv_obj_set_style_radius(badge,       LV_RADIUS_CIRCLE,  0);
        lv_obj_set_style_bg_color(badge,     lv_color_hex(Cd),  0);
        lv_obj_set_style_bg_opa(badge,       LV_OPA_COVER,      0);
        lv_obj_set_style_border_color(badge, lv_color_hex(C),   0);
        lv_obj_set_style_border_width(badge, 2,                 0);
        lv_obj_set_style_pad_all(badge,      0,                 0);
        lv_obj_clear_flag(badge, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *symL = _mkLabel(badge, th.symbol, C, &lv_font_montserrat_12);
        lv_obj_align(symL, LV_ALIGN_CENTER, 0, 0);

        lv_obj_t *nameL = _mkLabel(hdr, th.name,   C_TEXT1, &lv_font_montserrat_16);
        lv_obj_set_pos(nameL, 48, 6);
        lv_obj_t *tickL = _mkLabel(hdr, th.symbol, C,       &lv_font_montserrat_12);
        lv_obj_set_pos(tickL, 48, 26);

        // Page dots
        int dotW  = count * 10 - 4;
        int dotX0 = 306 - 10 - dotW;
        int dotY  = (46 - 6) / 2;
        for (int i = 0; i < count; i++)
        {
            lv_obj_t *dot = lv_obj_create(hdr);
            lv_obj_set_size(dot, 6, 6);
            lv_obj_set_pos(dot, dotX0 + i * 10, dotY);
            lv_obj_set_style_radius(dot,       LV_RADIUS_CIRCLE, 0);
            lv_obj_set_style_border_width(dot, 0,                0);
            lv_obj_set_style_pad_all(dot,      0,                0);
            lv_obj_clear_flag(dot, LV_OBJ_FLAG_SCROLLABLE);
            uint32_t dc = (i == index) ? C : C_BORDER;
            lv_obj_set_style_bg_color(dot, lv_color_hex(dc), 0);
            lv_obj_set_style_bg_opa(dot,   LV_OPA_COVER,     0);
        }

        if (coin.valid)
        {
            lv_obj_t *uCard = _mkCard(scr, 7, 58, 306, 62, 0x16163A, C_BG, C);
            lv_obj_t *uTag  = _mkLabel(uCard, "USD", C_TEXT3, &lv_font_montserrat_12);
            lv_obj_set_pos(uTag, 14, 8);
            String uVal = _fmtUSD(coin.usdPrice);
            lv_obj_t *uPri = _mkLabel(uCard, uVal.c_str(), C_TEXT1, &lv_font_montserrat_24);
            lv_obj_set_pos(uPri, 14, 24);

            lv_obj_t *iTag  = _mkLabel(uCard, "INR", C_TEXT3, &lv_font_montserrat_12);
            lv_obj_set_pos(iTag, 208, 8);
            String iVal = coin.hasInr ? _fmtINR(coin.inrPrice) : String("N/A");
            lv_obj_t *iPri = _mkLabel(uCard, iVal.c_str(), C_TEXT2, &lv_font_montserrat_16);
            lv_obj_set_pos(iPri, 168, 26);

            _drawChangeCard(scr, 7,   128, 96, 64, "24H", coin.change24h);
            _drawChangeCard(scr, 112, 128, 96, 64, "7D",  coin.change7d);
            _drawChangeCard(scr, 217, 128, 96, 64, "1M",  coin.change30d);
        }
        else
        {
            lv_obj_t *eCard = _mkCard(scr, 7, 58, 306, 134, 0x200A0A, C_BG, 0x5A1818);
            lv_obj_t *eMsg  = _mkLabel(eCard, "Price unavailable", 0xF07070, &lv_font_montserrat_16);
            lv_obj_align(eMsg, LV_ALIGN_CENTER, 0, 0);
        }

        // Nav icons
        lv_obj_t *navL = _mkLabel(scr, LV_SYMBOL_LEFT,    C_TEXT3, &lv_font_montserrat_12);
        lv_obj_set_pos(navL, 12, 206);
        lv_obj_t *navC = _mkLabel(scr, LV_SYMBOL_REFRESH, C_TEXT3, &lv_font_montserrat_12);
        lv_obj_align(navC, LV_ALIGN_BOTTOM_MID, 0, -10);
        lv_obj_t *navR = _mkLabel(scr, LV_SYMBOL_RIGHT,   C_TEXT3, &lv_font_montserrat_12);
        lv_obj_align(navR, LV_ALIGN_BOTTOM_RIGHT, -12, -10);

        _lvFlush();
    }

    void showMessage(const String &msg) override
    {
        lv_obj_t *scr = lv_scr_act();
        lv_obj_clean(scr);
        _styleScreen(scr);

        lv_obj_t *spin = lv_spinner_create(scr, 1200, 80);
        lv_obj_set_size(spin, 52, 52);
        lv_obj_align(spin, LV_ALIGN_CENTER, 0, -24);
        lv_obj_set_style_arc_color(spin, lv_color_hex(C_BLUE),   LV_PART_INDICATOR);
        lv_obj_set_style_arc_width(spin, 4,                       LV_PART_INDICATOR);
        lv_obj_set_style_arc_color(spin, lv_color_hex(C_BORDER),  LV_PART_MAIN);
        lv_obj_set_style_arc_width(spin, 4,                       LV_PART_MAIN);
        lv_obj_set_style_bg_opa(spin,    LV_OPA_TRANSP,           LV_PART_MAIN);

        lv_obj_t *lbl = _mkLabel(scr, msg.c_str(), C_TEXT2, &lv_font_montserrat_12);
        lv_obj_align(lbl, LV_ALIGN_CENTER, 0, 24);

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
