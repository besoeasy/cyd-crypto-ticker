/**
 * @file lv_conf.h
 * LVGL 8.3 configuration for CYD Crypto Ticker (ESP32 + ILI9341, 320x240)
 */

#if 1 /* Set to "1" to enable */

#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

/*====================
   COLOR SETTINGS
 *====================*/
#define LV_COLOR_DEPTH     16
#define LV_COLOR_16_SWAP   1   /* Swap bytes → ILI9341 MSB-first */
#define LV_COLOR_SCREEN_TRANSP 0
#define LV_COLOR_MIX_ROUND_OFS 0
#define LV_COLOR_CHROMA_KEY lv_color_hex(0x00ff00)

/*=========================
   MEMORY SETTINGS
 *=========================*/
#define LV_MEM_CUSTOM      0
#define LV_MEM_SIZE        (48U * 1024U)
#define LV_MEM_ADR         0
#define LV_MEM_POOL_INCLUDE <stdlib.h>
#define LV_MEM_POOL_ALLOC   malloc
#define LV_MEM_POOL_FREE    free

/*====================
   HAL SETTINGS
 *====================*/
#define LV_DISP_DEF_REFR_PERIOD  16   /* ms (≈60 fps) */
#define LV_INDEV_DEF_READ_PERIOD 30   /* ms */

/* Custom tick source: Arduino millis() */
#define LV_TICK_CUSTOM     1
#define LV_TICK_CUSTOM_INCLUDE  "Arduino.h"
#define LV_TICK_CUSTOM_SYS_TIME_EXPR  (millis())

#define LV_TIMER_CUSTOM    0
#define LV_TIMER_HANDLER_INCLUDE  "lvgl.h"

#define LV_DPI_DEF         130

/*====================
   FEATURE CONFIGURATION
 *====================*/
#define LV_DRAW_COMPLEX    1   /* Enables gradients, rounded corners, AA */
#define LV_SHADOW_CACHE_SIZE  0
#define LV_CIRCLE_CACHE_SIZE  4
#define LV_IMG_CACHE_DEF_SIZE 0
#define LV_GRADIENT_MAX_STOPS 2
#define LV_GRAD_CACHE_DEF_SIZE 0
#define LV_DITHER_GRADIENT    0
#define LV_DISP_ROT_MAX_BUF  (10*1024)

/*====================
   LOGGING
 *====================*/
#define LV_USE_LOG         0
#define LV_LOG_LEVEL       LV_LOG_LEVEL_WARN
#define LV_LOG_PRINTF      0
#define LV_LOG_USE_TIMESTAMP 1
#define LV_LOG_USE_FILE_LINE 1

/*====================
   ASSERTIONS
 *====================*/
#define LV_USE_ASSERT_NULL          1
#define LV_USE_ASSERT_MALLOC        1
#define LV_USE_ASSERT_STYLE         0
#define LV_USE_ASSERT_MEM_INTEGRITY 0
#define LV_USE_ASSERT_OBJ           0
#define LV_ASSERT_HANDLER_INCLUDE   <stdint.h>
#define LV_ASSERT_HANDLER   while(1);

/*====================
   DEBUG / PERF
 *====================*/
#define LV_USE_PERF_MONITOR         0
#define LV_USE_PERF_MONITOR_POS     LV_ALIGN_BOTTOM_RIGHT
#define LV_USE_MEM_MONITOR          0
#define LV_USE_MEM_MONITOR_POS      LV_ALIGN_TOP_LEFT
#define LV_USE_REFR_DEBUG           0

/*====================
   SPRINTF
 *====================*/
#define LV_SPRINTF_CUSTOM           0
#define LV_SPRINTF_USE_FLOAT        0

/*====================
   OS / HAL
 *====================*/
#define LV_USE_USER_DATA            1
#define LV_ENABLE_GC                0
#define LV_BIN_DECIMALS             3

/*====================
   COMPILER SETTINGS
 *====================*/
#define LV_ATTRIBUTE_MEM_ALIGN_SIZE  4
#define LV_ATTRIBUTE_MEM_ALIGN
#define LV_ATTRIBUTE_LARGE_CONST
#define LV_ATTRIBUTE_LARGE_RAM_ARRAY
#define LV_ATTRIBUTE_FAST_MEM
#define LV_ATTRIBUTE_DMA
#define LV_EXPORT_CONST_INT(int_value) struct _silence_gcc_warning
#define LV_USE_LARGE_COORD           0

/*====================
   BUILT-IN IMAGE SUPPORT
 *====================*/
#define LV_USE_PNG    0
#define LV_USE_BMP    0
#define LV_USE_JPG    0
#define LV_USE_GIF    0
#define LV_USE_QRCODE 0
#define LV_USE_RLOTTIE 0
#define LV_USE_FFMPEG 0

/*====================
   FONT SETTINGS
 *====================*/
/* Montserrat fonts */
#define LV_FONT_MONTSERRAT_8  0
#define LV_FONT_MONTSERRAT_10 0
#define LV_FONT_MONTSERRAT_12 1
#define LV_FONT_MONTSERRAT_14 0
#define LV_FONT_MONTSERRAT_16 1
#define LV_FONT_MONTSERRAT_18 0
#define LV_FONT_MONTSERRAT_20 0
#define LV_FONT_MONTSERRAT_22 0
#define LV_FONT_MONTSERRAT_24 1
#define LV_FONT_MONTSERRAT_26 0
#define LV_FONT_MONTSERRAT_28 0
#define LV_FONT_MONTSERRAT_30 0
#define LV_FONT_MONTSERRAT_32 1
#define LV_FONT_MONTSERRAT_34 0
#define LV_FONT_MONTSERRAT_36 0
#define LV_FONT_MONTSERRAT_38 0
#define LV_FONT_MONTSERRAT_40 0
#define LV_FONT_MONTSERRAT_42 0
#define LV_FONT_MONTSERRAT_44 0
#define LV_FONT_MONTSERRAT_46 0
#define LV_FONT_MONTSERRAT_48 0

/* Special glyphs */
#define LV_FONT_MONTSERRAT_28_COMPRESSED 0
#define LV_FONT_DEJAVU_16_PERSIAN_HEBREW 0
#define LV_FONT_SIMSUN_16_CJK            0
#define LV_FONT_UNSCII_8                 0
#define LV_FONT_UNSCII_16                0
#define LV_FONT_CUSTOM_DECLARE

#define LV_FONT_DEFAULT &lv_font_montserrat_12

#define LV_FONT_FMT_TXT_LARGE   0
#define LV_USE_FONT_COMPRESSED  0
#define LV_USE_FONT_SUBPX       0
#define LV_USE_FONT_SUBPX_BGR   0

/*====================
   TEXT SETTINGS
 *====================*/
#define LV_TXT_ENC LV_TXT_ENC_UTF8
#define LV_TXT_BREAK_CHARS " ,.;:-_"
#define LV_TXT_LINE_BREAK_LONG_LEN          0
#define LV_TXT_LINE_BREAK_LONG_PRE_MIN_LEN  3
#define LV_TXT_LINE_BREAK_LONG_POST_MIN_LEN 3
#define LV_TXT_COLOR_CMD "#"
#define LV_USE_BIDI                 0
#define LV_BIDI_BASE_DIR_DEF        LV_BASE_DIR_LTR
#define LV_USE_ARABIC_PERSIAN_CHARS 0

/*====================
   THEMES
 *====================*/
#define LV_USE_THEME_DEFAULT    0
#define LV_USE_THEME_BASIC      0
#define LV_USE_THEME_MONO       0

/*====================
   LAYOUTS
 *====================*/
#define LV_USE_FLEX  0
#define LV_USE_GRID  0

/*====================
   WIDGET ENABLES
 *====================*/
#define LV_USE_ARC          1

#define LV_USE_ANIMIMG      0

#define LV_USE_BAR          0

#define LV_USE_BTN          0

#define LV_USE_BTNMATRIX    0

#define LV_USE_CANVAS       0

#define LV_USE_CHECKBOX     0

#define LV_USE_DROPDOWN     0

#define LV_USE_IMG          0

#define LV_USE_LABEL        1
#  define LV_LABEL_TEXT_SELECTION    0
#  define LV_LABEL_LONG_TXT_HINT     0

#define LV_USE_LINE         0

#define LV_USE_ROLLER       0

#define LV_USE_SLIDER       0

#define LV_USE_SWITCH       0

#define LV_USE_TEXTAREA     0

#define LV_USE_TABLE        0

/*====================
   EXTRA WIDGETS
 *====================*/
#define LV_USE_CALENDAR     0

#define LV_USE_CHART        0

#define LV_USE_COLORWHEEL   0

#define LV_USE_IMGBTN       0

#define LV_USE_KEYBOARD     0

#define LV_USE_LED          0

#define LV_USE_LIST         0

#define LV_USE_MENU         0

#define LV_USE_METER        0

#define LV_USE_MSGBOX       0

#define LV_USE_SPINBOX      0

#define LV_USE_SPINNER      1   /* loading animation */

#define LV_USE_TABVIEW      0

#define LV_USE_TILEVIEW     0

#define LV_USE_WIN          0

#define LV_USE_SPAN         0

/*====================
   USE OTHERS
 *====================*/
#define LV_USE_SNAPSHOT     0
#define LV_USE_MONKEY       0
#define LV_USE_GRIDNAV      0
#define LV_USE_FRAGMENT     0
#define LV_USE_IMGFONT      0
#define LV_USE_MSG          0
#define LV_USE_IME_PINYIN   0

#endif /* LV_CONF_H */
#endif /* End of file guard */
