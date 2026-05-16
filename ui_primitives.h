#ifndef UI_PRIMITIVES_H
#define UI_PRIMITIVES_H

#include <lvgl.h>
#include "ui_palette.h"

// ── Low-level LVGL building blocks ───────────────────────────────────────────

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
                          int x, int y, int w, int h,
                          uint32_t bg, uint32_t border,
                          const char *text,
                          uint32_t color,
                          const lv_font_t *font)
{
    lv_obj_t *pill  = _mkCard(parent, x, y, w, h, bg, bg, border, h / 2);
    lv_obj_t *label = _mkLabel(pill, text, color, font);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    return pill;
}

// Invalidate and pump LVGL for a one-shot render
static void _lvFlush()
{
    lv_obj_invalidate(lv_scr_act());
    for (int i = 0; i < 16; i++) lv_timer_handler();
}

#endif // UI_PRIMITIVES_H
