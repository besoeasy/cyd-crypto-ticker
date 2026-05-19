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
        _resetSettingsTouch();

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

        _drawHeader(cont, coin, th, count, index, C);
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
        bool reopening = _screenMode != SCREEN_SETTINGS;
        _settingsData = data;
        _hasSettingsData = true;
        if (reopening)
        {
            _settingsScrollY = 0;
            _settingsPage = SETTINGS_HOME;
        }

        _screenMode = SCREEN_SETTINGS;
        _lastCoinIndex = -1;
        _clampSettingsScroll();
        _renderSettingsScreen();
    }

    void showMessage(const String &msg) override
    {
        _screenMode = SCREEN_MESSAGE;
        _resetSettingsTouch();
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

        if (_screenMode == SCREEN_SETTINGS)
            return _getSettingsTouchAction();

        bool nowTouched = ts.touched();
        if (!nowTouched) { _wasTouched = false; return TouchAction(); }
        if (_wasTouched)  return TouchAction();

        _wasTouched = true;
        TS_Point p  = ts.getPoint();
        int sx      = _mapTouchAxis(p.x, TOUCH_X_MIN, TOUCH_X_MAX, screenWidth);

        if (_screenMode != SCREEN_COIN)
            return TouchAction();

        if (sx < screenWidth / 3)        return {TOUCH_PREV, -1};
        if (sx > (screenWidth * 2) / 3)  return {TOUCH_NEXT, -1};
        return {TOUCH_OPEN_SETTINGS, -1};
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
    enum SettingsPage
    {
        SETTINGS_HOME,
        SETTINGS_COINS,
        SETTINGS_DEVICE
    };

    SettingsPage _settingsPage = SETTINGS_HOME;
    SettingsViewData _settingsData;
    bool _hasSettingsData = false;
    int _settingsScrollY = 0;
    bool _settingsTouchActive = false;
    bool _settingsTouchDragging = false;
    bool _settingsTouchCanScroll = false;
    int _settingsTouchStartX = 0;
    int _settingsTouchStartY = 0;
    int _settingsTouchLastX = 0;
    int _settingsTouchLastY = 0;

    static constexpr int SETTINGS_HEADER_X = 7;
    static constexpr int SETTINGS_HEADER_Y = 8;
    static constexpr int SETTINGS_HEADER_W = 306;
    static constexpr int SETTINGS_HEADER_H = 46;
    static constexpr int SETTINGS_VIEWPORT_X = 7;
    static constexpr int SETTINGS_VIEWPORT_Y = 60;
    static constexpr int SETTINGS_VIEWPORT_W = 306;
    static constexpr int SETTINGS_VIEWPORT_H = 132;
    static constexpr int SETTINGS_FOOTER_X = 7;
    static constexpr int SETTINGS_FOOTER_Y = 198;
    static constexpr int SETTINGS_FOOTER_W = 306;
    static constexpr int SETTINGS_FOOTER_H = 34;
    static constexpr int SETTINGS_CONTENT_X = 8;
    static constexpr int SETTINGS_CONTENT_W = 290;
    static constexpr int SETTINGS_SUMMARY_Y = 8;
    static constexpr int SETTINGS_SUMMARY_H = 56;
    static constexpr int SETTINGS_RANDOM_Y = 74;
    static constexpr int SETTINGS_RANDOM_H = 72;
    static constexpr int SETTINGS_SECTION_Y = 162;
    static constexpr int SETTINGS_OPTION_Y = 184;
    static constexpr int SETTINGS_OPTION_H = 44;
    static constexpr int SETTINGS_OPTION_GAP = 8;
    static constexpr int SETTINGS_NOTE_H = 38;
    static constexpr int SETTINGS_STEPPER_H = 52;
    static constexpr int SETTINGS_WIFI_H = 54;
    static constexpr int SETTINGS_SCROLL_THRESHOLD = 10;

    static constexpr int SETTINGS_HOME_SUMMARY_Y = 10;
    static constexpr int SETTINGS_HOME_SUMMARY_H = 46;
    static constexpr int SETTINGS_HOME_CARD_Y = 64;
    static constexpr int SETTINGS_HOME_CARD_W = 140;
    static constexpr int SETTINGS_HOME_CARD_H = 58;

    static bool _pointInRect(int x, int y, int left, int top, int width, int height)
    {
        return x >= left && x < left + width && y >= top && y < top + height;
    }

    static int _mapTouchAxis(int value, int minValue, int maxValue, int size)
    {
        value = constrain(value, minValue, maxValue);
        return map(value, minValue, maxValue, 0, size - 1);
    }

    void _resetSettingsTouch()
    {
        _settingsTouchActive = false;
        _settingsTouchDragging = false;
        _settingsTouchCanScroll = false;
    }

    int _settingsOptionTop(int index) const
    {
        return SETTINGS_OPTION_Y + index * (SETTINGS_OPTION_H + SETTINGS_OPTION_GAP);
    }

    int _settingsCoinsBottomY() const
    {
        if (_settingsData.optionCount <= 0) return SETTINGS_SECTION_Y;
        return _settingsOptionTop(_settingsData.optionCount - 1) + SETTINGS_OPTION_H;
    }

    int _settingsHiddenNoteY() const
    {
        return _settingsCoinsBottomY() + 8;
    }

    int _settingsDeviceSectionY() const
    {
        int y = _settingsCoinsBottomY() + 18;
        if (_settingsData.hiddenSelectedCount > 0)
            y = _settingsHiddenNoteY() + SETTINGS_NOTE_H + 18;
        return y;
    }

    int _settingsRefreshCardY() const
    {
        return _settingsDeviceSectionY() + 18;
    }

    int _settingsRotateCardY() const
    {
        return _settingsRefreshCardY() + SETTINGS_STEPPER_H + 8;
    }

    int _settingsWifiCardY() const
    {
        return _settingsRotateCardY() + SETTINGS_STEPPER_H + 8;
    }

    int _settingsContentHeight() const
    {
        if (!_hasSettingsData) return SETTINGS_VIEWPORT_H;

        if (_settingsPage == SETTINGS_HOME) return SETTINGS_VIEWPORT_H;

        if (_settingsPage == SETTINGS_COINS)
        {
            int bottom = _settingsCoinsBottomY() + 12;
            if (_settingsData.hiddenSelectedCount > 0)
                bottom = _settingsHiddenNoteY() + SETTINGS_NOTE_H + 12;
            return bottom > SETTINGS_VIEWPORT_H ? bottom : SETTINGS_VIEWPORT_H;
        }

        int wifiY = SETTINGS_RANDOM_Y + (SETTINGS_STEPPER_H + 8) * 2;
        int bottom = wifiY + SETTINGS_WIFI_H + 12;

        return bottom > SETTINGS_VIEWPORT_H ? bottom : SETTINGS_VIEWPORT_H;
    }

    int _settingsMaxScroll() const
    {
        int maxScroll = _settingsContentHeight() - SETTINGS_VIEWPORT_H;
        return maxScroll > 0 ? maxScroll : 0;
    }

    void _clampSettingsScroll()
    {
        int maxScroll = _settingsMaxScroll();
        if (_settingsScrollY < 0) _settingsScrollY = 0;
        if (_settingsScrollY > maxScroll) _settingsScrollY = maxScroll;
    }

    void _drawSettingsOptionRow(lv_obj_t *parent, int index) const
    {
        const SettingsCoinOption &option = _settingsData.options[index];
        bool enabled = _settingsData.selected[index];
        CoinTheme theme = _getCoinTheme(option.id);
        uint32_t border = enabled ? theme.hex : C_BORDER;
        uint32_t rowBg = enabled ? _dimBg(theme.hex) : 0x10131A;
        uint32_t rowBot = enabled ? _dim(theme.hex) : 0x10131A;
        uint32_t textColor = enabled ? C_TEXT1 : C_TEXT2;
        int y = _settingsOptionTop(index);

        lv_obj_t *row = _mkCard(parent, SETTINGS_CONTENT_X, y, SETTINGS_CONTENT_W, SETTINGS_OPTION_H, rowBg, rowBot, border, 14);

        lv_obj_t *badge = _mkCard(row, 10, 8, 54, 28, 0x0A1024, border, border, 14);
        lv_obj_t *badgeLabel = _mkLabel(badge, option.label, enabled ? C_TEXT1 : C_TEXT3, &lv_font_montserrat_12);
        lv_obj_align(badgeLabel, LV_ALIGN_CENTER, 0, 0);

        lv_obj_t *name = _mkLabel(row, theme.name, textColor, &lv_font_montserrat_16);
        lv_obj_set_pos(name, 76, 6);

        lv_obj_t *hint = _mkLabel(row, enabled ? "Included in base list" : "Tap to add", C_TEXT3, &lv_font_montserrat_12);
        lv_obj_set_pos(hint, 76, 24);

        lv_obj_t *state = _mkCard(row, 228, 9, 52, 26, enabled ? 0x0A1024 : 0x111111, border, border, 13);
        lv_obj_t *stateLabel = _mkLabel(state, enabled ? "ON" : "OFF", enabled ? C_BLUE : C_TEXT3, &lv_font_montserrat_12);
        lv_obj_align(stateLabel, LV_ALIGN_CENTER, 0, 0);
    }

    void _drawSettingsStepperCard(lv_obj_t *parent,
                                  int y,
                                  const char *title,
                                  const String &meta,
                                  const String &valueText) const
    {
        lv_obj_t *card = _mkCard(parent, SETTINGS_CONTENT_X, y, SETTINGS_CONTENT_W, SETTINGS_STEPPER_H, 0x14173A, 0x0B0E21, C_BORDER, 14);
        lv_obj_t *titleLabel = _mkLabel(card, title, C_TEXT1, &lv_font_montserrat_16);
        lv_obj_set_pos(titleLabel, 12, 8);
        lv_obj_t *metaLabel = _mkLabel(card, meta.c_str(), C_TEXT3, &lv_font_montserrat_12);
        lv_obj_set_pos(metaLabel, 12, 27);

        lv_obj_t *minus = _mkCard(card, 176, 9, 42, 34, 0x10131A, 0x10131A, C_BORDER, 12);
        lv_obj_t *minusLabel = _mkLabel(minus, "-", C_TEXT1, &lv_font_montserrat_16);
        lv_obj_align(minusLabel, LV_ALIGN_CENTER, 0, -1);

        lv_obj_t *plus = _mkCard(card, 244, 9, 42, 34, 0x10131A, 0x10131A, C_BORDER, 12);
        lv_obj_t *plusLabel = _mkLabel(plus, "+", C_TEXT1, &lv_font_montserrat_16);
        lv_obj_align(plusLabel, LV_ALIGN_CENTER, 0, -1);

        lv_obj_t *value = _mkLabel(card, valueText.c_str(), C_TEXT1, &lv_font_montserrat_16);
        lv_obj_align(value, LV_ALIGN_RIGHT_MID, -82, 0);
    }

    void _drawHomeIconCard(lv_obj_t *parent,
                           int x,
                           int y,
                           int w,
                           int h,
                           uint32_t accent,
                           const char *title,
                           const char *hint,
                           bool deviceCard) const
    {
        lv_obj_t *card = _mkCard(parent, x, y, w, h, 0x14173A, 0x0B0E21, C_BORDER, 16);

        lv_obj_t *iconBg = _mkCard(card, 12, 11, 40, 36, _dimBg(accent), _dim(accent), accent, 14);
        lv_obj_set_style_border_width(iconBg, 0, 0);

        if (deviceCard)
        {
            _mkOrb(iconBg, 8, 8, 20, accent, LV_OPA_60);
            _mkOrb(iconBg, 18, 4, 8, C_BG, LV_OPA_COVER);
            _mkBar(iconBg, 24, 12, 8, 2, accent);
            _mkBar(iconBg, 10, 22, 20, 2, accent);
        }
        else
        {
            _mkOrb(iconBg, 8, 10, 8, 0xF7931A, LV_OPA_COVER);
            _mkOrb(iconBg, 18, 6, 8, 0x627EEA, LV_OPA_COVER);
            _mkOrb(iconBg, 24, 18, 8, 0x00C18C, LV_OPA_COVER);
        }

        lv_obj_t *titleLabel = _mkLabel(card, title, C_TEXT1, &lv_font_montserrat_16);
        lv_obj_set_pos(titleLabel, 62, 12);
        lv_obj_t *hintLabel = _mkLabel(card, hint, C_TEXT3, &lv_font_montserrat_12);
        lv_obj_set_pos(hintLabel, 62, 30);
    }

    void _renderSettingsHome(lv_obj_t *scr) const
    {
        lv_obj_t *viewport = _mkCard(scr, SETTINGS_VIEWPORT_X, SETTINGS_VIEWPORT_Y, SETTINGS_VIEWPORT_W, SETTINGS_VIEWPORT_H, 0x0B0E21, 0x0B0E21, C_BORDER, 14);

        lv_obj_t *summary = _mkCard(viewport, SETTINGS_CONTENT_X, SETTINGS_HOME_SUMMARY_Y, SETTINGS_CONTENT_W, SETTINGS_HOME_SUMMARY_H, 0x14173A, 0x0B0E21, C_BORDER, 14);
        lv_obj_t *summaryTitle = _mkLabel(summary, "Crypto gadget controls", C_TEXT1, &lv_font_montserrat_16);
        lv_obj_set_pos(summaryTitle, 12, 9);
        String summaryMeta = String(_settingsData.selectedCount + _settingsData.randomCoinCount) + " active slots, refresh " + String(_settingsData.priceRefreshSeconds) + "s";
        lv_obj_t *summaryLabel = _mkLabel(summary, summaryMeta.c_str(), C_TEXT2, &lv_font_montserrat_12);
        lv_obj_set_pos(summaryLabel, 12, 28);

        _drawHomeIconCard(viewport, 10, SETTINGS_HOME_CARD_Y, SETTINGS_HOME_CARD_W, SETTINGS_HOME_CARD_H, 0xF7931A, "Coins", "Base list and random mix", false);
        _drawHomeIconCard(viewport, 156, SETTINGS_HOME_CARD_Y, SETTINGS_HOME_CARD_W, SETTINGS_HOME_CARD_H, C_BLUE, "Device", "Timing and WiFi portal", true);

        lv_obj_t *footer = _mkCard(scr, SETTINGS_FOOTER_X, SETTINGS_FOOTER_Y, SETTINGS_FOOTER_W, SETTINGS_FOOTER_H, 0x0A0A0A, 0x050505, C_BORDER, 12);
        _mkPill(footer, 8, 3, 88, 28, 0x10131A, C_BORDER, "BACK", C_TEXT3, &lv_font_montserrat_12);
        lv_obj_t *footerHint = _mkLabel(footer, "TAP CARD", C_TEXT3, &lv_font_montserrat_12);
        lv_obj_align(footerHint, LV_ALIGN_CENTER, 52, 0);
    }

    void _renderCoinsSettings(lv_obj_t *scr) const
    {
        lv_obj_t *viewport = _mkCard(scr, SETTINGS_VIEWPORT_X, SETTINGS_VIEWPORT_Y, SETTINGS_VIEWPORT_W, SETTINGS_VIEWPORT_H, 0x0B0E21, 0x0B0E21, C_BORDER, 14);
        lv_obj_set_style_clip_corner(viewport, true, 0);

        lv_obj_t *content = lv_obj_create(viewport);
        lv_obj_set_size(content, SETTINGS_VIEWPORT_W, _settingsContentHeight());
        lv_obj_set_pos(content, 0, -_settingsScrollY);
        lv_obj_set_style_bg_opa(content, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(content, 0, 0);
        lv_obj_set_style_pad_all(content, 0, 0);
        lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *summary = _mkCard(content, SETTINGS_CONTENT_X, SETTINGS_SUMMARY_Y, SETTINGS_CONTENT_W, SETTINGS_SUMMARY_H, 0x14173A, 0x0B0E21, C_BORDER, 14);
        String summaryTitle = String(_settingsData.selectedCount) + " base coins";
        lv_obj_t *summaryLabel = _mkLabel(summary, summaryTitle.c_str(), C_TEXT1, &lv_font_montserrat_16);
        lv_obj_set_pos(summaryLabel, 12, 10);
        String summaryBody = String(_settingsData.randomCoinCount) + " random, " + String(_settingsData.selectedCount + _settingsData.randomCoinCount) + " / 8 total";
        lv_obj_t *summaryValue = _mkLabel(summary, summaryBody.c_str(), C_TEXT2, &lv_font_montserrat_12);
        lv_obj_set_pos(summaryValue, 12, 31);
        lv_obj_t *summaryHint = _mkLabel(summary, "Tap rows to toggle tracked coins", C_TEXT3, &lv_font_montserrat_12);
        lv_obj_set_pos(summaryHint, 12, 45);

        lv_obj_t *randomCard = _mkCard(content, SETTINGS_CONTENT_X, SETTINGS_RANDOM_Y, SETTINGS_CONTENT_W, SETTINGS_RANDOM_H, 0x14173A, 0x0B0E21, C_BORDER, 14);
        lv_obj_t *randomTitle = _mkLabel(randomCard, "Random coins", C_TEXT1, &lv_font_montserrat_16);
        lv_obj_set_pos(randomTitle, 12, 10);
        String randomMeta = String(_settingsData.maxRandomCoinCount) + " available with current base count";
        lv_obj_t *randomMetaLabel = _mkLabel(randomCard, randomMeta.c_str(), C_TEXT3, &lv_font_montserrat_12);
        lv_obj_set_pos(randomMetaLabel, 12, 31);

        lv_obj_t *minus = _mkCard(randomCard, 12, 22, 48, 40, 0x10131A, 0x10131A, C_BORDER, 14);
        lv_obj_t *minusLabel = _mkLabel(minus, "-", C_TEXT1, &lv_font_montserrat_16);
        lv_obj_align(minusLabel, LV_ALIGN_CENTER, 0, -1);

        lv_obj_t *plus = _mkCard(randomCard, 230, 22, 48, 40, 0x10131A, 0x10131A, C_BORDER, 14);
        lv_obj_t *plusLabel = _mkLabel(plus, "+", C_TEXT1, &lv_font_montserrat_16);
        lv_obj_align(plusLabel, LV_ALIGN_CENTER, 0, -1);

        String randomValue = String(_settingsData.randomCoinCount);
        lv_obj_t *randomCount = _mkLabel(randomCard, randomValue.c_str(), C_TEXT1, &lv_font_montserrat_32);
        lv_obj_align(randomCount, LV_ALIGN_CENTER, 0, 7);

        lv_obj_t *section = _mkLabel(content, "Base coins", C_TEXT2, &lv_font_montserrat_12);
        lv_obj_set_pos(section, SETTINGS_CONTENT_X, SETTINGS_SECTION_Y);

        for (int i = 0; i < _settingsData.optionCount; i++)
            _drawSettingsOptionRow(content, i);

        if (_settingsData.hiddenSelectedCount > 0)
        {
            int noteY = _settingsHiddenNoteY();
            lv_obj_t *note = _mkCard(content, SETTINGS_CONTENT_X, noteY, SETTINGS_CONTENT_W, SETTINGS_NOTE_H, 0x14173A, 0x0B0E21, C_BORDER, 14);
            String hidden = String("+") + String(_settingsData.hiddenSelectedCount) + " custom portal IDs stay saved";
            lv_obj_t *hiddenLabel = _mkLabel(note, hidden.c_str(), C_TEXT2, &lv_font_montserrat_12);
            lv_obj_set_pos(hiddenLabel, 12, 10);
            lv_obj_t *hiddenHint = _mkLabel(note, "Use the portal only for advanced CoinGecko IDs", C_TEXT3, &lv_font_montserrat_12);
            lv_obj_set_pos(hiddenHint, 12, 22);
        }

        lv_obj_t *footer = _mkCard(scr, SETTINGS_FOOTER_X, SETTINGS_FOOTER_Y, SETTINGS_FOOTER_W, SETTINGS_FOOTER_H, 0x0A0A0A, 0x050505, C_BORDER, 12);
        _mkPill(footer, 8, 3, 88, 28, 0x10131A, C_BORDER, "BACK", C_TEXT3, &lv_font_montserrat_12);

        uint32_t applyBorder = _settingsData.dirty ? C_BLUE : C_BORDER;
        uint32_t applyBg = _settingsData.dirty ? 0x0A1024 : 0x10131A;
        lv_obj_t *apply = _mkCard(footer, 178, 3, 120, 28, applyBg, applyBg, applyBorder, 14);
        lv_obj_t *applyLabel = _mkLabel(apply, _settingsData.dirty ? "APPLY" : "SAVED", _settingsData.dirty ? C_BLUE : C_TEXT3, &lv_font_montserrat_12);
        lv_obj_align(applyLabel, LV_ALIGN_CENTER, 0, 0);
    }

    void _renderDeviceSettings(lv_obj_t *scr) const
    {
        lv_obj_t *viewport = _mkCard(scr, SETTINGS_VIEWPORT_X, SETTINGS_VIEWPORT_Y, SETTINGS_VIEWPORT_W, SETTINGS_VIEWPORT_H, 0x0B0E21, 0x0B0E21, C_BORDER, 14);
        lv_obj_set_style_clip_corner(viewport, true, 0);

        lv_obj_t *content = lv_obj_create(viewport);
        lv_obj_set_size(content, SETTINGS_VIEWPORT_W, _settingsContentHeight());
        lv_obj_set_pos(content, 0, -_settingsScrollY);
        lv_obj_set_style_bg_opa(content, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(content, 0, 0);
        lv_obj_set_style_pad_all(content, 0, 0);
        lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *summary = _mkCard(content, SETTINGS_CONTENT_X, SETTINGS_SUMMARY_Y, SETTINGS_CONTENT_W, SETTINGS_SUMMARY_H, 0x14173A, 0x0B0E21, C_BORDER, 14);
        lv_obj_t *summaryTitle = _mkLabel(summary, "Device behavior", C_TEXT1, &lv_font_montserrat_16);
        lv_obj_set_pos(summaryTitle, 12, 10);
        String summaryBody = String("Refresh ") + String(_settingsData.priceRefreshSeconds) + "s, rotate " + String(_settingsData.rotateSeconds) + "s";
        lv_obj_t *summaryValue = _mkLabel(summary, summaryBody.c_str(), C_TEXT2, &lv_font_montserrat_12);
        lv_obj_set_pos(summaryValue, 12, 31);
        lv_obj_t *summaryHint = _mkLabel(summary, "Keep timing low-friction for glanceable monitoring", C_TEXT3, &lv_font_montserrat_12);
        lv_obj_set_pos(summaryHint, 12, 45);

        String refreshMeta = "15s to 300s in 15s steps";
        String refreshValue = String(_settingsData.priceRefreshSeconds) + "s";
        _drawSettingsStepperCard(content, SETTINGS_RANDOM_Y, "Price refresh", refreshMeta, refreshValue);

        String rotateMeta = "3s to 60s in 1s steps";
        String rotateValue = String(_settingsData.rotateSeconds) + "s";
        _drawSettingsStepperCard(content, SETTINGS_RANDOM_Y + SETTINGS_STEPPER_H + 8, "Auto-rotate", rotateMeta, rotateValue);

        int wifiY = SETTINGS_RANDOM_Y + (SETTINGS_STEPPER_H + 8) * 2;
        lv_obj_t *wifiCard = _mkCard(content, SETTINGS_CONTENT_X, wifiY, SETTINGS_CONTENT_W, SETTINGS_WIFI_H, 0x14173A, 0x0B0E21, C_BORDER, 14);
        lv_obj_t *wifiTitle = _mkLabel(wifiCard, "WiFi portal", C_TEXT1, &lv_font_montserrat_16);
        lv_obj_set_pos(wifiTitle, 12, 8);
        lv_obj_t *wifiHint = _mkLabel(wifiCard, "Tap to reboot into captive setup", C_TEXT3, &lv_font_montserrat_12);
        lv_obj_set_pos(wifiHint, 12, 29);
        lv_obj_t *wifiAction = _mkCard(wifiCard, 194, 10, 86, 32, 0x0A1024, 0x0A1024, C_BLUE, 12);
        lv_obj_t *wifiActionLabel = _mkLabel(wifiAction, "OPEN", C_BLUE, &lv_font_montserrat_12);
        lv_obj_align(wifiActionLabel, LV_ALIGN_CENTER, 0, 0);

        lv_obj_t *footer = _mkCard(scr, SETTINGS_FOOTER_X, SETTINGS_FOOTER_Y, SETTINGS_FOOTER_W, SETTINGS_FOOTER_H, 0x0A0A0A, 0x050505, C_BORDER, 12);
        _mkPill(footer, 8, 3, 88, 28, 0x10131A, C_BORDER, "BACK", C_TEXT3, &lv_font_montserrat_12);

        uint32_t applyBorder = _settingsData.dirty ? C_BLUE : C_BORDER;
        uint32_t applyBg = _settingsData.dirty ? 0x0A1024 : 0x10131A;
        lv_obj_t *apply = _mkCard(footer, 178, 3, 120, 28, applyBg, applyBg, applyBorder, 14);
        lv_obj_t *applyLabel = _mkLabel(apply, _settingsData.dirty ? "APPLY" : "SAVED", _settingsData.dirty ? C_BLUE : C_TEXT3, &lv_font_montserrat_12);
        lv_obj_align(applyLabel, LV_ALIGN_CENTER, 0, 0);
    }

    void _renderSettingsScreen()
    {
        if (!_hasSettingsData) return;

        lv_obj_t *scr = lv_scr_act();
        lv_obj_clean(scr);
        _styleScreen(scr);
        _mkBar(scr, 0, 0, 320, 1, C_BLUE);

        lv_obj_t *hero = _mkCard(scr, SETTINGS_HEADER_X, SETTINGS_HEADER_Y, SETTINGS_HEADER_W, SETTINGS_HEADER_H, 0x0A0A0A, 0x050505, C_BORDER, 8);
        _mkPill(hero, 10, 10, 78, 20, 0x0A1024, C_BORDER, "SETTINGS", C_BLUE, &lv_font_montserrat_12);
        const char *titleText = "Settings home";
        const char *subText = "Pick a category, then drill into sub settings";
        if (_settingsPage == SETTINGS_COINS)
        {
            titleText = "Coin settings";
            subText = "Icon menu -> Coins -> mix and random picks";
        }
        else if (_settingsPage == SETTINGS_DEVICE)
        {
            titleText = "Device settings";
            subText = "Icon menu -> Device -> timing and WiFi tools";
        }

        lv_obj_t *title = _mkLabel(hero, titleText, C_TEXT1, &lv_font_montserrat_16);
        lv_obj_set_pos(title, 10, 27);
        lv_obj_t *sub = _mkLabel(hero, subText, C_TEXT3, &lv_font_montserrat_12);
        lv_obj_set_pos(sub, 104, 14);

        if (_settingsPage == SETTINGS_HOME)
            _renderSettingsHome(scr);
        else if (_settingsPage == SETTINGS_COINS)
            _renderCoinsSettings(scr);
        else
            _renderDeviceSettings(scr);

        int maxScroll = (_settingsPage == SETTINGS_HOME) ? 0 : _settingsMaxScroll();
        if (maxScroll > 0)
        {
            lv_obj_t *track = _mkCard(scr, 302, SETTINGS_VIEWPORT_Y + 8, 4, SETTINGS_VIEWPORT_H - 16, 0x1B1B1B, 0x1B1B1B, 0x1B1B1B, 2);
            int thumbHeight = ((SETTINGS_VIEWPORT_H - 16) * SETTINGS_VIEWPORT_H) / _settingsContentHeight();
            if (thumbHeight < 18) thumbHeight = 18;
            int thumbTravel = (SETTINGS_VIEWPORT_H - 16) - thumbHeight;
            int thumbY = SETTINGS_VIEWPORT_Y + 8;
            if (thumbTravel > 0)
                thumbY += (_settingsScrollY * thumbTravel) / maxScroll;
            _mkCard(scr, 302, thumbY, 4, thumbHeight, C_BLUE, C_BLUE, C_BLUE, 2);
        }

        _lvFlush();
    }

    TouchAction _hitTestSettingsAction(int sx, int sy)
    {
        if (_settingsPage == SETTINGS_HOME)
        {
            if (_pointInRect(sx, sy, 15, 201, 88, 28))
                return TouchAction(TOUCH_PREV, -1);

            if (_pointInRect(sx, sy, 17, 124, SETTINGS_HOME_CARD_W, SETTINGS_HOME_CARD_H))
            {
                _settingsPage = SETTINGS_COINS;
                _settingsScrollY = 0;
                _renderSettingsScreen();
                return TouchAction();
            }

            if (_pointInRect(sx, sy, 163, 124, SETTINGS_HOME_CARD_W, SETTINGS_HOME_CARD_H))
            {
                _settingsPage = SETTINGS_DEVICE;
                _settingsScrollY = 0;
                _renderSettingsScreen();
                return TouchAction();
            }

            return TouchAction();
        }

        if (_pointInRect(sx, sy, 15, 201, 88, 28))
        {
            _settingsPage = SETTINGS_HOME;
            _settingsScrollY = 0;
            _renderSettingsScreen();
            return TouchAction();
        }

        if (_pointInRect(sx, sy, 185, 201, 120, 28))
            return TouchAction(TOUCH_APPLY_SETTINGS, -1);

        if (!_pointInRect(sx, sy, SETTINGS_VIEWPORT_X, SETTINGS_VIEWPORT_Y, SETTINGS_VIEWPORT_W, SETTINGS_VIEWPORT_H))
            return TouchAction();

        int contentX = sx - SETTINGS_VIEWPORT_X;
        int contentY = sy - SETTINGS_VIEWPORT_Y + _settingsScrollY;

        if (_pointInRect(contentX, contentY, 20, SETTINGS_RANDOM_Y + 22, 48, 40))
            return TouchAction(TOUCH_RANDOM_COUNT_DEC, -1);

        if (_pointInRect(contentX, contentY, 238, SETTINGS_RANDOM_Y + 22, 48, 40))
            return TouchAction(TOUCH_RANDOM_COUNT_INC, -1);

        if (_settingsPage == SETTINGS_COINS)
        {
            for (int i = 0; i < _settingsData.optionCount; i++)
            {
                if (_pointInRect(contentX, contentY, SETTINGS_CONTENT_X, _settingsOptionTop(i), SETTINGS_CONTENT_W, SETTINGS_OPTION_H))
                    return TouchAction(TOUCH_TOGGLE_SETTINGS_COIN, i);
            }
            return TouchAction();
        }

        if (_pointInRect(contentX, contentY, 184, SETTINGS_RANDOM_Y + 9, 42, 34))
            return TouchAction(TOUCH_PRICE_REFRESH_DEC, -1);

        if (_pointInRect(contentX, contentY, 252, SETTINGS_RANDOM_Y + 9, 42, 34))
            return TouchAction(TOUCH_PRICE_REFRESH_INC, -1);

        if (_pointInRect(contentX, contentY, 184, SETTINGS_RANDOM_Y + SETTINGS_STEPPER_H + 17, 42, 34))
            return TouchAction(TOUCH_ROTATE_DEC, -1);

        if (_pointInRect(contentX, contentY, 252, SETTINGS_RANDOM_Y + SETTINGS_STEPPER_H + 17, 42, 34))
            return TouchAction(TOUCH_ROTATE_INC, -1);

        if (_pointInRect(contentX, contentY, SETTINGS_CONTENT_X, SETTINGS_RANDOM_Y + (SETTINGS_STEPPER_H + 8) * 2, SETTINGS_CONTENT_W, SETTINGS_WIFI_H))
            return TouchAction(TOUCH_OPEN_WIFI_PORTAL, -1);

        return TouchAction();
    }

    TouchAction _getSettingsTouchAction()
    {
        bool nowTouched = ts.touched();
        if (!nowTouched)
        {
            if (!_settingsTouchActive) return TouchAction();

            bool wasDragging = _settingsTouchDragging;
            int releaseX = _settingsTouchLastX;
            int releaseY = _settingsTouchLastY;
            _resetSettingsTouch();

            if (wasDragging) return TouchAction();
            return _hitTestSettingsAction(releaseX, releaseY);
        }

        TS_Point p = ts.getPoint();
        int sx = _mapTouchAxis(p.x, TOUCH_X_MIN, TOUCH_X_MAX, screenWidth);
        int sy = _mapTouchAxis(p.y, TOUCH_Y_MIN, TOUCH_Y_MAX, screenHeight);

        if (!_settingsTouchActive)
        {
            _settingsTouchActive = true;
            _settingsTouchDragging = false;
            _settingsTouchCanScroll = _settingsPage != SETTINGS_HOME
                && _pointInRect(sx, sy, SETTINGS_VIEWPORT_X, SETTINGS_VIEWPORT_Y, SETTINGS_VIEWPORT_W, SETTINGS_VIEWPORT_H);
            _settingsTouchStartX = sx;
            _settingsTouchStartY = sy;
            _settingsTouchLastX = sx;
            _settingsTouchLastY = sy;
            return TouchAction();
        }

        int totalDx = sx - _settingsTouchStartX;
        int totalDy = sy - _settingsTouchStartY;

        if (_settingsTouchCanScroll
            && (_settingsTouchDragging || (abs(totalDy) >= SETTINGS_SCROLL_THRESHOLD && abs(totalDy) > abs(totalDx))))
        {
            _settingsTouchDragging = true;

            int scrollStep = _settingsTouchLastY - sy;
            _settingsTouchLastX = sx;
            _settingsTouchLastY = sy;

            if (scrollStep != 0)
            {
                _settingsScrollY += scrollStep;
                _clampSettingsScroll();
                _renderSettingsScreen();
            }
            return TouchAction();
        }

        _settingsTouchLastX = sx;
        _settingsTouchLastY = sy;
        return TouchAction();
    }
};

#endif // CHEAPYELLOWLCD_H
