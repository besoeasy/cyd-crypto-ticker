#ifndef UI_THEME_H
#define UI_THEME_H

#include <Arduino.h>

// ── Coin branding ─────────────────────────────────────────────────────────────

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

    // Unknown coin: derive symbol from first 4 chars and capitalise name
    static char symBuf[8], nameBuf[32];
    String sym = id.substring(0, 4);
    sym.toUpperCase();
    sym.toCharArray(symBuf, sizeof(symBuf));
    id.toCharArray(nameBuf, sizeof(nameBuf));
    if (nameBuf[0]) nameBuf[0] = toupper((unsigned char)nameBuf[0]);
    return {0x6B7DB3, symBuf, nameBuf};
}

// Darken an RGB hex colour to ~30% for dim accent backgrounds
static uint32_t _dim(uint32_t hex)
{
    uint8_t r = ((hex >> 16) & 0xFF) * 30 / 100;
    uint8_t g = ((hex >>  8) & 0xFF) * 30 / 100;
    uint8_t b = ((hex      ) & 0xFF) * 30 / 100;
    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}

#endif // UI_THEME_H
