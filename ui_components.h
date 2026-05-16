#ifndef UI_COMPONENTS_H
#define UI_COMPONENTS_H

#include "ui_primitives.h"
#include "ui_theme.h"
#include "ui_formatters.h"
#include "projectDisplay.h"

#include <string.h>

// ── Change colour helper ──────────────────────────────────────────────────────
static uint32_t _changeColor(const PriceChangeData &change)
{
    if (!change.available) return C_TEXT3;
    return change.percent < 0 ? C_RED : C_GREEN;
}

// ── Pagination dots ───────────────────────────────────────────────────────────
static void _drawPageDots(lv_obj_t *parent, int count, int index,
                           int left, int right, int y, uint32_t accent)
{
    if (count <= 0 || right <= left) return;

    int dotSize = 6;
    int gap = 4;
    int available = right - left;
    int dotW = count * dotSize + (count - 1) * gap;

    while (dotW > available && gap > 2)
    {
        gap--;
        dotW = count * dotSize + (count - 1) * gap;
    }

    while (dotW > available && dotSize > 4)
    {
        dotSize--;
        dotW = count * dotSize + (count - 1) * gap;
    }

    int x0   = right - dotW;
    for (int i = 0; i < count; i++)
    {
        lv_obj_t *dot = lv_obj_create(parent);
        lv_obj_set_size(dot, dotSize, dotSize);
        lv_obj_set_pos(dot, x0 + i * (dotSize + gap), y + (6 - dotSize) / 2);
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

static String _headerStatusText(const CryptoData &coin)
{
    if (coin.valid && coin.lastUpdatedMs > 0)
    {
        unsigned long ageSeconds = (millis() - coin.lastUpdatedMs) / 1000UL;
        if (ageSeconds < 5) return "UPDATED JUST NOW";
        if (ageSeconds < 60) return "UPDATED " + String(ageSeconds) + "S AGO";

        unsigned long ageMinutes = ageSeconds / 60UL;
        if (ageMinutes < 60) return "UPDATED " + String(ageMinutes) + "M AGO";

        unsigned long ageHours = ageMinutes / 60UL;
        return "UPDATED " + String(ageHours) + "H AGO";
    }

    if (coin.feedStatus == COIN_FEED_RATE_LIMITED) return "RATE LIMITED";
    if (coin.feedStatus == COIN_FEED_NETWORK) return "NETWORK ISSUE";
    if (coin.feedStatus == COIN_FEED_INVALID_ID) return "INVALID COIN ID";
    if (coin.feedStatus == COIN_FEED_PARSE_ERROR) return "API RESPONSE ERROR";
    return "PRICE UNAVAILABLE";
}

static const char *_emptyStateTag(CoinFeedStatus status)
{
    if (status == COIN_FEED_RATE_LIMITED) return "RATE LIMIT";
    if (status == COIN_FEED_NETWORK) return "NETWORK";
    if (status == COIN_FEED_INVALID_ID) return "INVALID ID";
    if (status == COIN_FEED_PARSE_ERROR) return "API ERROR";
    return "NO DATA";
}

static const char *_emptyStateMessage(CoinFeedStatus status)
{
    if (status == COIN_FEED_RATE_LIMITED) return "Try again in a minute";
    if (status == COIN_FEED_NETWORK) return "Check WiFi connection";
    if (status == COIN_FEED_INVALID_ID) return "CoinGecko ID not found";
    if (status == COIN_FEED_PARSE_ERROR) return "Unexpected API response";
    return "Price feed unavailable";
}

// ── Header row: dot + name + symbol pill | page dots + eyebrow ───────────────
static void _drawHeader(lv_obj_t *parent,
                        const CryptoData &coin,
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

    // Freshness or status below name.
    String eyebrowText = _headerStatusText(coin);
    uint32_t eyebrowColor = coin.valid ? C_TEXT2 : C_RED;
    lv_obj_t *eyebrow = _mkLabel(parent, eyebrowText.c_str(), eyebrowColor, &lv_font_montserrat_12);
    lv_obj_set_pos(eyebrow, 28, 36);

    // Symbol pill uses content width and leaves the remaining right side for dots.
    uint32_t pillBg = _dimBg(accent);
    int pillW = max(32, (int)strlen(theme.symbol) * 7 + 12);
    int pillX = 124;
    _mkPill(parent, pillX, 18, pillW, 18, pillBg, accent, theme.symbol, accent, &lv_font_montserrat_12);

    // Page dots
    _drawPageDots(parent, count, index, pillX + pillW + 12, 308, 23, accent);
}

// ── Price block ───────────────────────────────────────────────────────────────
static const lv_font_t *_priceFontForText(const String &price)
{
    int len = price.length();
    if (len <= 9)  return &lv_font_montserrat_32;
    if (len <= 13) return &lv_font_montserrat_24;
    return &lv_font_montserrat_16;
}

static int _priceYForFont(const lv_font_t *font)
{
    if (font == &lv_font_montserrat_32) return 76;
    if (font == &lv_font_montserrat_24) return 84;
    return 94;
}

static void _drawPriceBlock(lv_obj_t *parent,
                             const CryptoData &coin,
                             uint32_t          accent)
{
    (void)accent;

    // Big USD price
    String usd = _fmtDisplayUSD(coin.usdPrice);
    const lv_font_t *font = _priceFontForText(usd);
    lv_obj_t *price = _mkLabel(parent, usd.c_str(), C_TEXT1, font);
    lv_obj_set_pos(price, 12, _priceYForFont(font));
}

// ── Metrics columns (24H / 7D / 1M) ──────────────────────────────────────────
static void _drawMetrics(lv_obj_t *parent, const CryptoData &coin)
{
    // Vertical hairline dividers between columns.
    _mkBar(parent, 108, 158, 1, 66, C_BORDER);
    _mkBar(parent, 204, 158, 1, 66, C_BORDER);

    const PriceChangeData *changes[3] = {
        &coin.change24h, &coin.change7d, &coin.change30d
    };
    const char *labels[3] = { "24H", "7D", "1M" };

    for (int i = 0; i < 3; i++)
    {
        int mx = 18 + i * 96;
        const PriceChangeData &c = *changes[i];
        uint32_t col = _changeColor(c);

        lv_obj_t *lbl = _mkLabel(parent, labels[i], C_TEXT3, &lv_font_montserrat_12);
        lv_obj_set_pos(lbl, mx, 162);

        String pct = c.available ? _fmtSignedPercent(c.percent) : "--";
        lv_obj_t *pctLbl = _mkLabel(parent, pct.c_str(), col, &lv_font_montserrat_16);
        lv_obj_set_pos(pctLbl, mx, 184);

        String amt = c.available ? _fmtSignedUSD(c.usdAmount) : "N/A";
        lv_obj_t *amtLbl = _mkLabel(parent, amt.c_str(), C_TEXT3, &lv_font_montserrat_12);
        lv_obj_set_pos(amtLbl, mx, 208);
    }
}

// ── Offline / error state ─────────────────────────────────────────────────────
static void _drawEmptyState(lv_obj_t *parent, const CryptoData &coin)
{
    const char *tag = _emptyStateTag(coin.feedStatus);
    const char *message = _emptyStateMessage(coin.feedStatus);
    _mkPill(parent, 102, 106, 116, 22, _dimBg(C_RED), C_RED, tag, C_RED, &lv_font_montserrat_12);
    lv_obj_t *msg = _mkLabel(parent, message, C_TEXT2, &lv_font_montserrat_16);
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
