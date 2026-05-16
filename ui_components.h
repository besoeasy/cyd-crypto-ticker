#ifndef UI_COMPONENTS_H
#define UI_COMPONENTS_H

#include "ui_primitives.h"
#include "ui_theme.h"
#include "ui_formatters.h"
#include "projectDisplay.h"

// ── Decorative backdrop (placed directly on scr – does not slide) ─────────────

static void _drawBackdrop(lv_obj_t *scr, uint32_t accent)
{
    _mkOrb(scr, -26, -10, 88, accent,   LV_OPA_20);
    _mkOrb(scr, 250,  10, 64, 0x1A224A, LV_OPA_40);
    _mkOrb(scr, 230, 162, 96, accent,   LV_OPA_10);
}

// ── Pagination dots ───────────────────────────────────────────────────────────

static void _drawPageDots(lv_obj_t *parent, int count, int index,
                           int right, int y, uint32_t accent)
{
    int dotW = count * 10 - 4;
    int x0   = right - dotW;
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

// ── Header card (coin badge + name + page dots) ───────────────────────────────

static lv_obj_t *_drawHeaderCard(lv_obj_t *parent,
                                  const CoinTheme &theme,
                                  int count, int index,
                                  uint32_t accent, uint32_t accentDim)
{
    lv_obj_t *hdr = _mkCard(parent, 7, 8, 306, 52, 0x12142F, 0x0A0C21, C_BORDER, 14);
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

// ── Price hero card ───────────────────────────────────────────────────────────

static lv_obj_t *_drawHeroCard(lv_obj_t *parent,
                                const CryptoData &coin,
                                const CoinTheme  &theme,
                                uint32_t          accent)
{
    lv_obj_t *card = _mkCard(parent, 7, 70, 306, 76, 0x15183A, 0x0B0D22, accent, 18);
    _mkBar(card, 0, 0, 306, 4, accent);
    _mkOrb(card, 234, -12, 72, accent, LV_OPA_10);

    _mkPill(card, 14, 12, 54, 20, 0x0B1028, C_BORDER, "SPOT",        C_TEXT3, &lv_font_montserrat_12);
    _mkPill(card, 226, 12, 66, 20, 0x0B1028, C_BORDER, theme.symbol, accent,  &lv_font_montserrat_12);

    String usd = _fmtUSD(coin.usdPrice);
    lv_obj_t *price = _mkLabel(card, usd.c_str(), C_TEXT1, &lv_font_montserrat_24);
    lv_obj_set_pos(price, 14, 30);

    String inr = coin.hasInr ? _fmtINR(coin.inrPrice) : String("INR N/A");
    lv_obj_t *inrLabel = _mkLabel(card, inr.c_str(), C_TEXT2, &lv_font_montserrat_12);
    lv_obj_align(inrLabel, LV_ALIGN_BOTTOM_RIGHT, -14, -10);
    return card;
}

// ── Individual change metric tile ─────────────────────────────────────────────

static uint32_t _changeColor(const PriceChangeData &change)
{
    if (!change.available) return C_TEXT3;
    return change.percent < 0 ? 0xF07070 : 0x35C46A;
}

static void _drawChangeCard(lv_obj_t *parent,
                             int x, int y, int w, int h,
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

// ── 24H / 7D / 1M strip ──────────────────────────────────────────────────────

static void _drawMetricStrip(lv_obj_t *parent, const CryptoData &coin)
{
    _drawChangeCard(parent,   7, 156, 96, 64, "24H", coin.change24h);
    _drawChangeCard(parent, 112, 156, 96, 64, "7D",  coin.change7d);
    _drawChangeCard(parent, 217, 156, 96, 64, "1M",  coin.change30d);
}

// ── Bottom navigation dock ────────────────────────────────────────────────────

static void _drawNavDock(lv_obj_t *parent)
{
    lv_obj_t *dock = _mkCard(parent, 44, 221, 232, 16, 0x0E1026, 0x0A0C1C, C_BORDER, 10);
    lv_obj_t *left    = _mkLabel(dock, LV_SYMBOL_LEFT,    C_TEXT3, &lv_font_montserrat_12);
    lv_obj_set_pos(left, 10, 2);
    lv_obj_t *refresh = _mkLabel(dock, LV_SYMBOL_REFRESH, C_TEXT3, &lv_font_montserrat_12);
    lv_obj_align(refresh, LV_ALIGN_CENTER, 0, 0);
    lv_obj_t *right   = _mkLabel(dock, LV_SYMBOL_RIGHT,   C_TEXT3, &lv_font_montserrat_12);
    lv_obj_align(right, LV_ALIGN_RIGHT_MID, -10, 0);

    lv_obj_t *hint = _mkLabel(parent, "Swipe zones: prev / refresh / next",
                               C_TEXT3, &lv_font_montserrat_12);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -1);
}

// ── Offline / error state card ────────────────────────────────────────────────

static void _drawEmptyState(lv_obj_t *parent, const char *message)
{
    lv_obj_t *card = _mkCard(parent, 7, 70, 306, 150, 0x200A0A, 0x100408, 0x5A1818, 18);
    _mkPill(card, 112, 18, 82, 22, 0x341113, 0x5A1818, "OFFLINE", 0xF07070, &lv_font_montserrat_12);
    lv_obj_t *msg = _mkLabel(card, message, 0xFFD0D0, &lv_font_montserrat_16);
    lv_obj_align(msg, LV_ALIGN_CENTER, 0, -2);
}

// ── Slide-in animation helper ─────────────────────────────────────────────────
// Wraps lv_obj_set_x for use as an lv_anim_exec_xcb_t callback.

static void _animSetX(void *obj, int32_t val)
{
    lv_obj_set_x((lv_obj_t *)obj, (lv_coord_t)val);
}

// Animate 'cont' sliding in from 'fromX' to x=0, then pump LVGL until done.
static void _slideIn(lv_obj_t *cont, int fromX)
{
    if (fromX == 0) { _lvFlush(); return; }

    lv_obj_set_x(cont, (lv_coord_t)fromX);

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a,      cont);
    lv_anim_set_exec_cb(&a,  _animSetX);
    lv_anim_set_values(&a,   fromX, 0);
    lv_anim_set_time(&a,     220);
    lv_anim_set_path_cb(&a,  lv_anim_path_ease_out);
    lv_anim_start(&a);

    uint32_t t = millis();
    while (millis() - t < 260) lv_timer_handler();
}

#endif // UI_COMPONENTS_H
