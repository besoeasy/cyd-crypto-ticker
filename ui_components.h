#ifndef UI_COMPONENTS_H
#define UI_COMPONENTS_H

#include "ui_primitives.h"
#include "ui_theme.h"
#include "ui_formatters.h"
#include "projectDisplay.h"

// ── Change colour helper ──────────────────────────────────────────────────────
static uint32_t _changeColor(const PriceChangeData &change)
{
    if (!change.available) return C_TEXT3;
    return change.percent < 0 ? C_RED : C_GREEN;
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

// ── Thin horizontal divider ───────────────────────────────────────────────────
static void _divider(lv_obj_t *parent, int y)
{
    _mkBar(parent, 12, y, 296, 1, C_BORDER);
}

// ── Header row: dot + name + symbol pill | page dots + eyebrow ───────────────
static void _drawHeader(lv_obj_t *parent,
                        const CoinTheme &theme,
                        int count, int index,
                        uint32_t accent)
{
    // Filled accent dot
    lv_obj_t *dot = lv_obj_create(parent);
    lv_obj_set_size(dot, 8, 8);
    lv_obj_set_pos(dot, 14, 22);
    lv_obj_set_style_radius(dot,       LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(dot,     lv_color_hex(accent), 0);
    lv_obj_set_style_bg_opa(dot,       LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(dot, 0, 0);
    lv_obj_set_style_pad_all(dot,      0, 0);
    lv_obj_clear_flag(dot, LV_OBJ_FLAG_SCROLLABLE);

    // Coin name
    lv_obj_t *name = _mkLabel(parent, theme.name, C_TEXT1, &lv_font_montserrat_16);
    lv_obj_set_pos(name, 28, 16);

    // Eyebrow below name
    lv_obj_t *eyebrow = _mkLabel(parent, "LIVE", C_TEXT3, &lv_font_montserrat_12);
    lv_obj_set_pos(eyebrow, 28, 36);

    // Symbol pill — fixed right-side position
    uint32_t pillBg = _dimBg(accent);
    _mkPill(parent, 222, 18, 54, 18, pillBg, accent, theme.symbol, accent, &lv_font_montserrat_12);

    // Page dots
    _drawPageDots(parent, count, index, 310, 23, accent);
}

// ── Price block ───────────────────────────────────────────────────────────────
static void _drawPriceBlock(lv_obj_t *parent,
                             const CryptoData &coin,
                             uint32_t          accent)
{
    // "SPOT PRICE" eyebrow
    lv_obj_t *spot = _mkLabel(parent, "SPOT PRICE", C_TEXT3, &lv_font_montserrat_12);
    lv_obj_set_pos(spot, 12, 60);

    // INR label + value (right-aligned)
    lv_obj_t *inrLbl = _mkLabel(parent, "INR", C_TEXT3, &lv_font_montserrat_12);
    lv_obj_align(inrLbl, LV_ALIGN_TOP_RIGHT, -12, 60);

    String inr = coin.hasInr ? _fmtINR(coin.inrPrice) : String("N/A");
    lv_obj_t *inrVal = _mkLabel(parent, inr.c_str(), C_TEXT2, &lv_font_montserrat_12);
    lv_obj_align(inrVal, LV_ALIGN_TOP_RIGHT, -12, 76);

    // Big USD price
    String usd = _fmtUSD(coin.usdPrice);
    lv_obj_t *price = _mkLabel(parent, usd.c_str(), C_TEXT1, &lv_font_montserrat_24);
    lv_obj_set_pos(price, 12, 72);

    // 24h change badge pill + USD amount
    const PriceChangeData &c24 = coin.change24h;
    if (c24.available)
    {
        uint32_t col = c24.percent >= 0 ? C_GREEN : C_RED;
        String pct   = _fmtSignedPercent(c24.percent);
        _mkPill(parent, 12, 108, 62, 18, _dimBg(col), col, pct.c_str(), col, &lv_font_montserrat_12);

        String amt = _fmtSignedUSD(c24.usdAmount);
        lv_obj_t *amtLbl = _mkLabel(parent, amt.c_str(), C_TEXT3, &lv_font_montserrat_12);
        lv_obj_set_pos(amtLbl, 80, 111);
    }
}

// ── Metrics columns (24H / 7D / 1M) ──────────────────────────────────────────
static void _drawMetrics(lv_obj_t *parent, const CryptoData &coin)
{
    // Vertical hairline dividers between columns
    _mkBar(parent, 12 + 96,  138, 1, 72, C_BORDER);
    _mkBar(parent, 12 + 192, 138, 1, 72, C_BORDER);

    const PriceChangeData *changes[3] = {
        &coin.change24h, &coin.change7d, &coin.change30d
    };
    const char *labels[3] = { "24H", "7D", "1M" };

    for (int i = 0; i < 3; i++)
    {
        int mx = 12 + i * 96 + 6;
        const PriceChangeData &c = *changes[i];
        uint32_t col = _changeColor(c);

        lv_obj_t *lbl = _mkLabel(parent, labels[i], C_TEXT3, &lv_font_montserrat_12);
        lv_obj_set_pos(lbl, mx, 142);

        String pct = c.available ? _fmtSignedPercent(c.percent) : "--";
        lv_obj_t *pctLbl = _mkLabel(parent, pct.c_str(), col, &lv_font_montserrat_16);
        lv_obj_set_pos(pctLbl, mx, 158);

        String amt = c.available ? _fmtSignedUSD(c.usdAmount) : "N/A";
        lv_obj_t *amtLbl = _mkLabel(parent, amt.c_str(), C_TEXT3, &lv_font_montserrat_12);
        lv_obj_set_pos(amtLbl, mx, 178);
    }
}

// ── Offline / error state ─────────────────────────────────────────────────────
static void _drawEmptyState(lv_obj_t *parent, const char *message)
{
    _mkPill(parent, 118, 96, 84, 22, _dimBg(C_RED), C_RED, "OFFLINE", C_RED, &lv_font_montserrat_12);
    lv_obj_t *msg = _mkLabel(parent, message, C_TEXT3, &lv_font_montserrat_16);
    lv_obj_align(msg, LV_ALIGN_CENTER, 0, 20);
}

// ── Slide-in animation ────────────────────────────────────────────────────────
static void _animSetX(void *obj, int32_t val)
{
    lv_obj_set_x((lv_obj_t *)obj, (lv_coord_t)val);
}

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
