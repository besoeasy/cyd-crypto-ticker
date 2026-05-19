#ifndef CHEAPYELLOWLCD_H
#define CHEAPYELLOWLCD_H

#include "projectDisplay.h"
#include "ui_palette.h"
#include "ui_formatters.h"
#include "ui_theme.h"
#include "ui_primitives.h"
#include "ui_components.h"

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
#define TOUCH_Y_MIN  240
#define TOUCH_Y_MAX  3850

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
        _screenMode = SCREEN_WIFI;
        lv_obj_t *scr = lv_scr_act();
        lv_obj_clean(scr);
        _styleScreen(scr);
        _mkBar(scr, 0, 0, 320, 1, C_BLUE);

        lv_obj_t *hero = _mkCard(scr, 7, 8, 306, 48, 0x0A0A0A, 0x050505, C_BORDER, 8);
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
        lv_obj_t *hint = _mkLabel(cc, "Adjust coins later on the Settings tab", C_TEXT2, &lv_font_montserrat_12);
        lv_obj_set_pos(hint, 12, 50);

        lv_obj_t *ft = _mkLabel(scr, "Waiting for connection...", C_TEXT3, &lv_font_montserrat_12);
        lv_obj_align(ft, LV_ALIGN_BOTTOM_MID, 0, -8);

        _lvFlush();
    }

    void drawCoinScreen(const CryptoData *coins, int count, int index) override
    {
        if (count == 0) { showMessage("No coins configured"); return; }

        _screenMode = SCREEN_COIN;

        const CryptoData &coin = coins[index];
        CoinTheme th = _getCoinTheme(coin.id);
        uint32_t  C  = th.hex;

        // Determine slide direction based on previous index
        int fromX = 0;
        if (_lastCoinIndex >= 0 && count > 1)
        {
            bool forward = (index > _lastCoinIndex) ||
                           (index == 0 && _lastCoinIndex == count - 1);
            fromX = forward ? 320 : -320;
        }
        _lastCoinIndex = index;

        lv_obj_t *scr = lv_scr_act();
        lv_obj_clean(scr);
        _styleScreen(scr);

        // 1px accent bar stays on scr (does not slide)
        _mkBar(scr, 0, 0, 320, 1, C);

        // All content goes in a transparent slide container
        lv_obj_t *cont = lv_obj_create(scr);
        lv_obj_set_size(cont, 320, 240);
        lv_obj_set_pos(cont, 0, 0);
        lv_obj_set_style_bg_opa(cont,       LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(cont, 0,             0);
        lv_obj_set_style_pad_all(cont,      0,             0);
        lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);

        _drawHeader(cont, coin, th, count + 1, index, C);
        _divider(cont, 54);

        if (coin.valid)
        {
            _drawPriceBlock(cont, coin, C);
            _divider(cont, 150);
            _drawMetrics(cont, coin);
        }
        else
        {
            _drawEmptyState(cont, coin);
        }

        // Slide in from the correct side (or plain flush on first render)
        _slideIn(cont, fromX);
    }

    void drawSettingsScreen(const SettingsViewData &data) override
    {
        _screenMode = SCREEN_SETTINGS;
        _lastCoinIndex = -1;

        lv_obj_t *scr = lv_scr_act();
        lv_obj_clean(scr);
        _styleScreen(scr);
        _mkBar(scr, 0, 0, 320, 1, C_BLUE);

        lv_obj_t *hero = _mkCard(scr, 7, 8, 306, 48, 0x0A0A0A, 0x050505, C_BORDER, 8);
        _mkPill(hero, 10, 10, 78, 20, 0x0A1024, C_BORDER, "SETTINGS", C_BLUE, &lv_font_montserrat_12);
        lv_obj_t *title = _mkLabel(hero, "Coin mix", C_TEXT1, &lv_font_montserrat_16);
        lv_obj_set_pos(title, 10, 28);
        lv_obj_t *sub = _mkLabel(hero, "Pick base coins and extra randoms", C_TEXT3, &lv_font_montserrat_12);
        lv_obj_set_pos(sub, 108, 14);

        lv_obj_t *randomCard = _mkCard(scr, 7, 64, 306, 40, 0x14173A, 0x0B0E21, C_BORDER, 12);
        lv_obj_t *randomLabel = _mkLabel(randomCard, "Random coins", C_TEXT2, &lv_font_montserrat_12);
        lv_obj_set_pos(randomLabel, 14, 7);

        lv_obj_t *minus = _mkCard(randomCard, 12, 8, 36, 24, 0x10131A, 0x10131A, C_BORDER, 12);
        lv_obj_t *minusLabel = _mkLabel(minus, "-", C_TEXT1, &lv_font_montserrat_16);
        lv_obj_align(minusLabel, LV_ALIGN_CENTER, 0, -1);

        lv_obj_t *plus = _mkCard(randomCard, 258, 8, 36, 24, 0x10131A, 0x10131A, C_BORDER, 12);
        lv_obj_t *plusLabel = _mkLabel(plus, "+", C_TEXT1, &lv_font_montserrat_16);
        lv_obj_align(plusLabel, LV_ALIGN_CENTER, 0, -1);

        String randomValue = String(data.randomCoinCount) + " / " + String(data.maxRandomCoinCount);
        lv_obj_t *randomCount = _mkLabel(randomCard, randomValue.c_str(), C_TEXT1, &lv_font_montserrat_16);
        lv_obj_align(randomCount, LV_ALIGN_CENTER, 0, 0);

        String baseSummary = String(data.selectedCount) + " base / " + String(data.maxBaseCoinCount) + " slots";
        lv_obj_t *baseCount = _mkLabel(randomCard, baseSummary.c_str(), C_TEXT3, &lv_font_montserrat_12);
        lv_obj_set_pos(baseCount, 108, 22);

        lv_obj_t *gridCard = _mkCard(scr, 7, 112, 306, 86, 0x14173A, 0x0B0E21, C_BORDER, 12);
        for (int i = 0; i < data.optionCount; i++)
        {
            int col = i % 4;
            int row = i / 4;
            int x = 10 + col * 73;
            int y = 11 + row * 34;
            CoinTheme theme = _getCoinTheme(data.options[i].id);
            bool enabled = data.selected[i];
            uint32_t bg = enabled ? _dimBg(theme.hex) : 0x10131A;
            uint32_t border = enabled ? theme.hex : C_BORDER;
            uint32_t color = enabled ? C_TEXT1 : C_TEXT3;

            lv_obj_t *pill = _mkCard(gridCard, x, y, 66, 24, bg, bg, border, 12);
            lv_obj_t *label = _mkLabel(pill, data.options[i].label, color, &lv_font_montserrat_12);
            lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
        }

        lv_obj_t *footer = _mkCard(scr, 7, 206, 306, 26, 0x0A0A0A, 0x050505, C_BORDER, 12);
        _mkPill(footer, 8, 3, 70, 20, 0x10131A, C_BORDER, "PREV", C_TEXT3, &lv_font_montserrat_12);
        _mkPill(footer, 228, 3, 70, 20, 0x10131A, C_BORDER, "NEXT", C_TEXT3, &lv_font_montserrat_12);

        uint32_t applyBorder = data.dirty ? C_BLUE : C_BORDER;
        uint32_t applyBg = data.dirty ? 0x0A1024 : 0x10131A;
        lv_obj_t *apply = _mkCard(footer, 103, 3, 100, 20, applyBg, applyBg, applyBorder, 10);
        lv_obj_t *applyLabel = _mkLabel(apply, data.dirty ? "APPLY" : "SAVED", data.dirty ? C_BLUE : C_TEXT3, &lv_font_montserrat_12);
        lv_obj_align(applyLabel, LV_ALIGN_CENTER, 0, 0);

        if (data.hiddenSelectedCount > 0)
        {
            String note = "+" + String(data.hiddenSelectedCount) + " custom IDs kept from portal";
            lv_obj_t *noteLabel = _mkLabel(scr, note.c_str(), C_TEXT3, &lv_font_montserrat_12);
            lv_obj_set_pos(noteLabel, 13, 196);
        }

        _lvFlush();
    }

    void showMessage(const String &msg) override
    {
        _screenMode = SCREEN_MESSAGE;
        lv_obj_t *scr = lv_scr_act();
        lv_obj_clean(scr);
        _styleScreen(scr);
        _mkBar(scr, 0, 0, 320, 1, C_BLUE);

        lv_obj_t *card = _mkCard(scr, 38, 54, 244, 128, 0x0A0A0A, 0x050505, C_BORDER, 8);
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

        // Reset so the next coin screen animates from scratch
        _lastCoinIndex = -1;

        _lvFlush();
    }

    TouchAction getTouchAction() override
    {
        lv_timer_handler();

        bool nowTouched = ts.touched();
        if (!nowTouched) { _wasTouched = false; return TouchAction(); }
        if (_wasTouched)  return TouchAction();

        _wasTouched = true;
        TS_Point p  = ts.getPoint();
        int sx      = _mapTouchAxis(p.x, TOUCH_X_MIN, TOUCH_X_MAX, screenWidth);
        int sy      = _mapTouchAxis(p.y, TOUCH_Y_MIN, TOUCH_Y_MAX, screenHeight);

        if (_screenMode == SCREEN_SETTINGS)
            return _getSettingsTouchAction(sx, sy);

        if (_screenMode != SCREEN_COIN)
            return TouchAction();

        if (sx < screenWidth / 3)        return {TOUCH_PREV, -1};
        if (sx > (screenWidth * 2) / 3)  return {TOUCH_NEXT, -1};
        return {TOUCH_REFRESH, -1};
    }

private:
    enum ScreenMode
    {
        SCREEN_MESSAGE,
        SCREEN_WIFI,
        SCREEN_COIN,
        SCREEN_SETTINGS
    };

    bool _wasTouched    = false;
    int  _lastCoinIndex = -1;   // -1 = no previous coin rendered yet
    ScreenMode _screenMode = SCREEN_MESSAGE;

    static bool _pointInRect(int x, int y, int left, int top, int width, int height)
    {
        return x >= left && x < left + width && y >= top && y < top + height;
    }

    static int _mapTouchAxis(int value, int minValue, int maxValue, int size)
    {
        value = constrain(value, minValue, maxValue);
        return map(value, minValue, maxValue, 0, size - 1);
    }

    TouchAction _getSettingsTouchAction(int sx, int sy) const
    {
        if (_pointInRect(sx, sy, 19, 72, 36, 24))
            return {TOUCH_RANDOM_COUNT_DEC, -1};

        if (_pointInRect(sx, sy, 265, 72, 36, 24))
            return {TOUCH_RANDOM_COUNT_INC, -1};

        for (int i = 0; i < 8; i++)
        {
            int col = i % 4;
            int row = i / 4;
            int x = 17 + col * 73;
            int y = 123 + row * 34;
            if (_pointInRect(sx, sy, x, y, 66, 24))
                return {TOUCH_TOGGLE_SETTINGS_COIN, i};
        }

        if (_pointInRect(sx, sy, 110, 209, 100, 20))
            return {TOUCH_APPLY_SETTINGS, -1};

        if (_pointInRect(sx, sy, 15, 209, 70, 20))
            return {TOUCH_PREV, -1};

        if (_pointInRect(sx, sy, 235, 209, 70, 20))
            return {TOUCH_NEXT, -1};

        return TouchAction();
    }
};

#endif // CHEAPYELLOWLCD_H
