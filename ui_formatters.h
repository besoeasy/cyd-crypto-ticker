#ifndef UI_FORMATTERS_H
#define UI_FORMATTERS_H

#include <Arduino.h>

// ── Number / price formatting ─────────────────────────────────────────────────

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
    if (p >= 1e6)  { dtostrf(p / 1e6, 1, 2, buf); return String("$") + buf + "M"; }
    if (p >= 1000) return "$" + _fmtCommas((long)p);
    if (p >= 1)    { dtostrf(p, 1, 2, buf); return String("$") + buf; }
    dtostrf(p, 1, 6, buf); return String("$") + buf;
}

static String _fmtINR(double p)
{
    char buf[16];
    if (p >= 1e7)  { dtostrf(p / 1e7, 1, 2, buf); return String("Rs.") + buf + "Cr"; }
    if (p >= 1e5)  { dtostrf(p / 1e5, 1, 2, buf); return String("Rs.") + buf + "L"; }
    if (p >= 1000) return "Rs." + _fmtCommas((long)p);
    dtostrf(p, 1, 2, buf); return String("Rs.") + buf;
}

static String _fmtSignedUSD(double value)
{
    String amount = _fmtUSD(fabs(value));
    if (value >  0.000001) return "+" + amount;
    if (value < -0.000001) return "-" + amount;
    return amount;
}

static String _fmtSignedPercent(double value)
{
    char buf[16];
    dtostrf(fabs(value), 1, 2, buf);
    String text = buf;
    text.trim();
    if (value >  0.000001) return "+" + text + "%";
    if (value < -0.000001) return "-" + text + "%";
    return text + "%";
}

#endif // UI_FORMATTERS_H
