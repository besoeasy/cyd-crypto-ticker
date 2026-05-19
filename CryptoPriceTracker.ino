#define ESP_DRD_USE_SPIFFS true

#include <WiFi.h>
#include <FS.h>
#include "SPIFFS.h"
#include <WiFiManager.h>
#include <ESP_DoubleResetDetector.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <math.h>
#include <esp_system.h>

#include "projectConfig.h"
#include "projectDisplay.h"
#include "wifiManagerHandler.h"
#include "cheapYellowLCD.h"

#define DRD_TIMEOUT 10
#define DRD_ADDRESS 0

#define MAX_COINS 8
#define SETTINGS_COIN_OPTION_COUNT 8

ProjectConfig projectConfig;
CheapYellowDisplay cyd;
ProjectDisplay *projectDisplay = &cyd;

static const SettingsCoinOption SETTINGS_COIN_OPTIONS[SETTINGS_COIN_OPTION_COUNT] = {
    {"bitcoin", "BTC"},
    {"ethereum", "ETH"},
    {"solana", "SOL"},
    {"binancecoin", "BNB"},
    {"ripple", "XRP"},
    {"dogecoin", "DOGE"},
    {"cardano", "ADA"},
    {"chainlink", "LINK"}
};

CryptoData coins[MAX_COINS];
int coinCount   = 0;
int currentCoin = 0;
String activeCoinIds;

String settingsDraftCoinIds[MAX_COINS];
int settingsDraftCoinCount = 0;
bool settingsCoinSelected[SETTINGS_COIN_OPTION_COUNT] = {false};
uint8_t settingsDraftRandomCoinCount = DEFAULT_RANDOM_COIN_COUNT;
uint16_t settingsDraftPriceRefreshSeconds = DEFAULT_PRICE_REFRESH_SECONDS;
uint16_t settingsDraftRotateSeconds = DEFAULT_ROTATE_SECONDS;
bool settingsDirty = false;
bool settingsScreenActive = false;

unsigned long lastPriceFetch = 0;
unsigned long lastAutoRotate = 0;
unsigned long priceIntervalMs  = DEFAULT_PRICE_REFRESH_SECONDS * 1000UL;
unsigned long rotateIntervalMs = DEFAULT_ROTATE_SECONDS * 1000UL;

static void resetCoinMarketData(CryptoData &coin)
{
    coin.usdPrice  = 0;
    coin.inrPrice  = 0;
    coin.hasInr    = false;
    coin.change24h = PriceChangeData();
    coin.change7d  = PriceChangeData();
    coin.change30d = PriceChangeData();
    coin.valid     = false;
    coin.lastUpdatedMs = 0;
    coin.feedStatus = COIN_FEED_NO_DATA;
}

static CoinFeedStatus httpCodeToFeedStatus(int httpCode)
{
    if (httpCode == 429) return COIN_FEED_RATE_LIMITED;
    return COIN_FEED_NETWORK;
}

static void markCoinValid(CryptoData &coin, unsigned long updatedAt)
{
    coin.valid = true;
    coin.feedStatus = COIN_FEED_OK;
    coin.lastUpdatedMs = updatedAt;
}

static void setInvalidCoinStatuses(CoinFeedStatus status)
{
    for (int i = 0; i < coinCount; i++)
    {
        if (!coins[i].valid) coins[i].feedStatus = status;
    }
}

static bool readOptionalDouble(JsonVariantConst value, double &result)
{
    if (value.isNull()) return false;
    result = value.as<double>();
    return true;
}

static bool setChangeFromPercent(PriceChangeData &change, double currentPrice, double percent)
{
    change = PriceChangeData();

    if (!isfinite(currentPrice) || !isfinite(percent)) return false;

    double denominator = 100.0 + percent;
    if (fabs(denominator) < 0.000001) return false;

    double previousPrice = currentPrice * 100.0 / denominator;
    change.percent = percent;
    change.usdAmount = currentPrice - previousPrice;
    change.available = true;
    return true;
}

static bool readPercentWithFallback(JsonObjectConst entry, const char *primaryKey, const char *fallbackKey, double &percent)
{
    return readOptionalDouble(entry[primaryKey], percent)
        || readOptionalDouble(entry[fallbackKey], percent);
}

static bool containsCoinId(const String *ids, int count, const String &id)
{
    for (int i = 0; i < count; i++)
    {
        if (ids[i] == id) return true;
    }
    return false;
}

static String normalizeCoinId(String id)
{
    id.trim();
    id.toLowerCase();
    id.replace(" ", "-");

    if (id == "btc") return "bitcoin";
    if (id == "eth") return "ethereum";
    if (id == "sol") return "solana";
    if (id == "ton" || id == "toncoin") return "the-open-network";
    if (id == "bnb") return "binancecoin";
    if (id == "xrp") return "ripple";
    if (id == "doge") return "dogecoin";
    if (id == "ada") return "cardano";
    if (id == "dot") return "polkadot";
    if (id == "link") return "chainlink";

    return id;
}

static void appendUniqueCoinId(String *ids, int &count, int maxCount, const String &id)
{
    String normalized = normalizeCoinId(id);
    if (normalized.length() == 0 || count >= maxCount || containsCoinId(ids, count, normalized)) return;
    ids[count++] = normalized;
}

static void appendCoinIdsFromCsv(const String &src, String *ids, int &count, int maxCount)
{
    int start = 0;
    for (int i = 0; i <= (int)src.length() && count < maxCount; i++)
    {
        if (i == (int)src.length() || src[i] == ',')
        {
            String token = src.substring(start, i);
            token.trim();
            appendUniqueCoinId(ids, count, maxCount, token);
            start = i + 1;
        }
    }
}

static String joinCoinIds(const String *ids, int count)
{
    String joined;
    for (int i = 0; i < count; i++)
    {
        if (i > 0) joined += ",";
        joined += ids[i];
    }
    return joined;
}

static bool hasAnyValidCoin(const CryptoData *data, int count)
{
    for (int i = 0; i < count; i++)
    {
        if (data[i].valid) return true;
    }
    return false;
}

static void normalizeConfiguredCoinIds()
{
    String normalizedIds[MAX_COINS];
    int normalizedCount = 0;
    appendCoinIdsFromCsv(projectConfig.cryptoIds, normalizedIds, normalizedCount, MAX_COINS);

    String normalized = joinCoinIds(normalizedIds, normalizedCount);
    if (normalized.length() == 0) return;

    if (normalized != projectConfig.cryptoIds)
    {
        Serial.print("Normalized coin IDs: ");
        Serial.println(normalized);
        projectConfig.cryptoIds = normalized;
    }
}

static bool appendRandomTrendingCoin(String *ids, int &count, int maxCount)
{
    HTTPClient http;
    http.begin("https://api.coingecko.com/api/v3/search/trending");
    http.useHTTP10(true);
    http.setTimeout(10000);

    int httpCode = http.GET();
    if (httpCode != HTTP_CODE_OK)
    {
        Serial.print("Trending fetch failed, HTTP: ");
        Serial.println(httpCode);
        http.end();
        return false;
    }

    StaticJsonDocument<128> filter;
    filter["coins"][0]["item"]["id"] = true;

    DynamicJsonDocument json(3072);
    DeserializationError error = deserializeJson(json, http.getStream(), DeserializationOption::Filter(filter));
    http.end();

    if (error)
    {
        Serial.print("Trending JSON parse failed: ");
        Serial.println(error.c_str());
        return false;
    }

    String chosenId;
    int eligibleCount = 0;
    JsonArrayConst trending = json["coins"].as<JsonArrayConst>();
    for (JsonObjectConst wrapper : trending)
    {
        String id = wrapper["item"]["id"] | "";
        if (id.length() == 0 || containsCoinId(ids, count, id)) continue;

        eligibleCount++;
        if (eligibleCount == 1 || random(eligibleCount) == 0)
            chosenId = id;
    }

    if (chosenId.length() == 0) return false;

    appendUniqueCoinId(ids, count, maxCount, chosenId);
    return true;
}

static int appendRandomHighVolumeCoins(String *ids, int &count, int maxCount, int desiredCount)
{
    HTTPClient http;
    http.begin("https://api.coingecko.com/api/v3/coins/markets?vs_currency=usd&order=volume_desc&per_page=100&page=1");
    http.useHTTP10(true);
    http.setTimeout(15000);

    int httpCode = http.GET();
    if (httpCode != HTTP_CODE_OK)
    {
        Serial.print("High-volume fetch failed, HTTP: ");
        Serial.println(httpCode);
        http.end();
        return 0;
    }

    StaticJsonDocument<64> filter;
    filter[0]["id"] = true;

    DynamicJsonDocument json(8192);
    DeserializationError error = deserializeJson(json, http.getStream(), DeserializationOption::Filter(filter));
    http.end();

    if (error)
    {
        Serial.print("High-volume JSON parse failed: ");
        Serial.println(error.c_str());
        return 0;
    }

    String reservoir[2];
    int reservoirSize = 0;
    int eligibleCount = 0;

    JsonArrayConst markets = json.as<JsonArrayConst>();
    for (JsonObjectConst entry : markets)
    {
        String id = entry["id"] | "";
        if (id.length() == 0 || containsCoinId(ids, count, id)) continue;

        bool alreadyPicked = false;
        for (int i = 0; i < reservoirSize; i++)
        {
            if (reservoir[i] == id)
            {
                alreadyPicked = true;
                break;
            }
        }
        if (alreadyPicked) continue;

        eligibleCount++;
        if (reservoirSize < desiredCount)
        {
            reservoir[reservoirSize++] = id;
            continue;
        }

        int slot = random(eligibleCount);
        if (slot < desiredCount)
            reservoir[slot] = id;
    }

    int added = 0;
    for (int i = 0; i < reservoirSize; i++)
    {
        int before = count;
        appendUniqueCoinId(ids, count, maxCount, reservoir[i]);
        if (count > before) added++;
    }
    return added;
}

static int maxBaseCoinCountForRandomCount(int randomCoinCount)
{
    int maxBase = MAX_COINS - randomCoinCount;
    return maxBase > 0 ? maxBase : 0;
}

static int maxRandomCoinCountForBaseCount(int baseCoinCount)
{
    int maxRandom = MAX_COINS - baseCoinCount;
    return maxRandom > 0 ? maxRandom : 0;
}

static int findSettingsOptionIndex(const String &id)
{
    for (int i = 0; i < SETTINGS_COIN_OPTION_COUNT; i++)
    {
        if (id == SETTINGS_COIN_OPTIONS[i].id) return i;
    }
    return -1;
}

static void syncSettingsSelectionFlags()
{
    for (int i = 0; i < SETTINGS_COIN_OPTION_COUNT; i++)
        settingsCoinSelected[i] = false;

    for (int i = 0; i < settingsDraftCoinCount; i++)
    {
        int optionIndex = findSettingsOptionIndex(settingsDraftCoinIds[i]);
        if (optionIndex >= 0) settingsCoinSelected[optionIndex] = true;
    }
}

static int countHiddenSettingsCoins()
{
    int hiddenCount = 0;
    for (int i = 0; i < settingsDraftCoinCount; i++)
    {
        if (findSettingsOptionIndex(settingsDraftCoinIds[i]) < 0) hiddenCount++;
    }
    return hiddenCount;
}

static void loadSettingsDraftFromConfig()
{
    settingsDraftCoinCount = 0;
    appendCoinIdsFromCsv(projectConfig.cryptoIds, settingsDraftCoinIds, settingsDraftCoinCount, MAX_COINS);
    settingsDraftRandomCoinCount = projectConfig.randomCoinCount;
    settingsDraftPriceRefreshSeconds = projectConfig.priceRefreshSeconds;
    settingsDraftRotateSeconds = projectConfig.rotateSeconds;

    int maxRandom = maxRandomCoinCountForBaseCount(settingsDraftCoinCount);
    if (settingsDraftRandomCoinCount > maxRandom)
        settingsDraftRandomCoinCount = maxRandom;

    syncSettingsSelectionFlags();
    settingsDirty = false;
}

static bool adjustSettingsPriceRefreshSeconds(int delta)
{
    int next = (int)settingsDraftPriceRefreshSeconds + delta;
    if (next < 15 || next > 300) return false;

    settingsDraftPriceRefreshSeconds = (uint16_t)next;
    settingsDirty = true;
    return true;
}

static bool adjustSettingsRotateSeconds(int delta)
{
    int next = (int)settingsDraftRotateSeconds + delta;
    if (next < 3 || next > 60) return false;

    settingsDraftRotateSeconds = (uint16_t)next;
    settingsDirty = true;
    return true;
}

static void syncRuntimeIntervals()
{
    priceIntervalMs = projectConfig.priceRefreshSeconds * 1000UL;
    rotateIntervalMs = projectConfig.rotateSeconds * 1000UL;
}

static void removeSettingsDraftCoinId(const String &id)
{
    for (int i = 0; i < settingsDraftCoinCount; i++)
    {
        if (settingsDraftCoinIds[i] != id) continue;

        for (int j = i; j < settingsDraftCoinCount - 1; j++)
            settingsDraftCoinIds[j] = settingsDraftCoinIds[j + 1];

        settingsDraftCoinIds[settingsDraftCoinCount - 1] = "";
        settingsDraftCoinCount--;
        return;
    }
}

static bool toggleSettingsCoinSelection(int optionIndex)
{
    if (optionIndex < 0 || optionIndex >= SETTINGS_COIN_OPTION_COUNT) return false;

    String id = SETTINGS_COIN_OPTIONS[optionIndex].id;
    if (containsCoinId(settingsDraftCoinIds, settingsDraftCoinCount, id))
    {
        removeSettingsDraftCoinId(id);
        syncSettingsSelectionFlags();
        settingsDirty = true;
        return true;
    }

    if (settingsDraftCoinCount >= maxBaseCoinCountForRandomCount(settingsDraftRandomCoinCount))
        return false;

    appendUniqueCoinId(settingsDraftCoinIds, settingsDraftCoinCount, MAX_COINS, id);
    syncSettingsSelectionFlags();
    settingsDirty = true;
    return true;
}

static bool adjustSettingsRandomCoinCount(int delta)
{
    int nextCount = (int)settingsDraftRandomCoinCount + delta;
    int maxRandom = maxRandomCoinCountForBaseCount(settingsDraftCoinCount);
    if (nextCount < 0 || nextCount > maxRandom) return false;

    settingsDraftRandomCoinCount = (uint8_t)nextCount;
    settingsDirty = true;
    return true;
}

static String buildResolvedCoinIds()
{
    String resolvedIds[MAX_COINS];
    int resolvedCount = 0;
    appendCoinIdsFromCsv(projectConfig.cryptoIds, resolvedIds, resolvedCount, MAX_COINS);

    int desiredRandom = projectConfig.randomCoinCount;
    int remainingSlots = MAX_COINS - resolvedCount;
    if (desiredRandom > remainingSlots) desiredRandom = remainingSlots;

    int randomAdded = 0;
    if (desiredRandom > 0)
    {
        if (appendRandomTrendingCoin(resolvedIds, resolvedCount, MAX_COINS))
            randomAdded++;

        if (desiredRandom > randomAdded)
            appendRandomHighVolumeCoins(resolvedIds, resolvedCount, MAX_COINS, desiredRandom - randomAdded);
    }

    if (resolvedCount == 0 && desiredRandom == 0)
        appendCoinIdsFromCsv(String(DEFAULT_CORE_CRYPTO_IDS), resolvedIds, resolvedCount, MAX_COINS);

    String dynamicIds = joinCoinIds(resolvedIds, resolvedCount);
    if (dynamicIds.length() > 0)
    {
        Serial.print("Resolved coin list: ");
        Serial.println(dynamicIds);
    }
    return dynamicIds;
}

// Split a comma-separated string into the coins[] array
void initCoins(const String &src)
{
    coinCount = 0;
    int start = 0;
    for (int i = 0; i <= (int)src.length() && coinCount < MAX_COINS; i++)
    {
        if (i == (int)src.length() || src[i] == ',')
        {
            String token = normalizeCoinId(src.substring(start, i));
            if (token.length() > 0)
            {
                coins[coinCount] = CryptoData();
                coins[coinCount].id = token;
                coinCount++;
            }
            start = i + 1;
        }
    }
}

static void refreshResolvedCoins()
{
    normalizeConfiguredCoinIds();
    activeCoinIds = buildResolvedCoinIds();
    initCoins(activeCoinIds);

    if (coinCount == 0)
        currentCoin = 0;
    else if (currentCoin >= coinCount)
        currentCoin = coinCount - 1;
}

static void renderCurrentScreen()
{
    if (settingsScreenActive)
    {
        SettingsViewData data;
        data.options = SETTINGS_COIN_OPTIONS;
        data.selected = settingsCoinSelected;
        data.optionCount = SETTINGS_COIN_OPTION_COUNT;
        data.selectedCount = settingsDraftCoinCount;
        data.hiddenSelectedCount = countHiddenSettingsCoins();
        data.maxBaseCoinCount = maxBaseCoinCountForRandomCount(settingsDraftRandomCoinCount);
        data.randomCoinCount = settingsDraftRandomCoinCount;
        data.maxRandomCoinCount = maxRandomCoinCountForBaseCount(settingsDraftCoinCount);
        data.priceRefreshSeconds = settingsDraftPriceRefreshSeconds;
        data.rotateSeconds = settingsDraftRotateSeconds;
        data.dirty = settingsDirty;
        projectDisplay->drawSettingsScreen(data);
        return;
    }

    if (coinCount == 0)
    {
        projectDisplay->showMessage("No coins configured");
        return;
    }

    projectDisplay->drawCoinScreen(coins, coinCount, currentCoin);
}

static bool applySettingsDraft()
{
    if (settingsDraftCoinCount == 0 && settingsDraftRandomCoinCount == 0)
        return false;

    projectConfig.cryptoIds = joinCoinIds(settingsDraftCoinIds, settingsDraftCoinCount);
    projectConfig.randomCoinCount = settingsDraftRandomCoinCount;
    projectConfig.priceRefreshSeconds = settingsDraftPriceRefreshSeconds;
    projectConfig.rotateSeconds = settingsDraftRotateSeconds;

    if (!projectConfig.saveConfigFile())
    {
        Serial.println("Failed to save settings config");
        return false;
    }

    syncRuntimeIntervals();
    refreshResolvedCoins();
    loadSettingsDraftFromConfig();
    return true;
}

static void restartIntoWifiPortal()
{
    projectConfig.openWifiPortal = true;
    if (!projectConfig.saveConfigFile())
    {
        Serial.println("Failed to save WiFi portal reboot flag");
        return;
    }

    projectDisplay->showMessage("Opening WiFi...");
    delay(600);
    ESP.restart();
}

void fetchPrices(bool allowDefaultFallback = true)
{
    if (coinCount == 0) return;

    CryptoData previousCoins[MAX_COINS];
    bool marketMatched[MAX_COINS] = {false};
    bool simpleMatched[MAX_COINS] = {false};
    unsigned long fetchedAt = millis();
    for (int i = 0; i < coinCount; i++)
        previousCoins[i] = coins[i];

    bool hadValidBefore = hasAnyValidCoin(previousCoins, coinCount);

    for (int i = 0; i < coinCount; i++)
        resetCoinMarketData(coins[i]);

    String ids = "";
    for (int i = 0; i < coinCount; i++)
    {
        if (i > 0) ids += ",";
        ids += coins[i].id;
    }

    String marketUrl = "https://api.coingecko.com/api/v3/coins/markets?vs_currency=usd&ids=" + ids
                     + "&price_change_percentage=24h,7d,30d&precision=full";

    HTTPClient http;
    http.begin(marketUrl);
    http.useHTTP10(true);
    http.setTimeout(15000);

    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK)
    {
        String payload = http.getString();
        DynamicJsonDocument json(24576);
        DeserializationError error = deserializeJson(json, payload);
        if (!error && json.is<JsonArray>())
        {
            JsonArrayConst markets = json.as<JsonArrayConst>();
            for (JsonObjectConst entry : markets)
            {
                String entryId = entry["id"] | "";
                for (int i = 0; i < coinCount; i++)
                {
                    if (coins[i].id != entryId) continue;
                    marketMatched[i] = true;

                    CryptoData &coin = coins[i];
                    if (!entry["current_price"].isNull())
                    {
                        coin.usdPrice = entry["current_price"].as<double>();
                        markCoinValid(coin, fetchedAt);
                    }

                    double percent = 0;
                    if (readPercentWithFallback(entry, "price_change_percentage_24h_in_currency", "price_change_percentage_24h", percent))
                    {
                        coin.change24h.percent = percent;
                        coin.change24h.available = true;
                        if (!readOptionalDouble(entry["price_change_24h"], coin.change24h.usdAmount))
                            setChangeFromPercent(coin.change24h, coin.usdPrice, percent);
                    }

                    if (readPercentWithFallback(entry, "price_change_percentage_7d_in_currency", "price_change_percentage_7d", percent))
                        setChangeFromPercent(coin.change7d, coin.usdPrice, percent);

                    if (readPercentWithFallback(entry, "price_change_percentage_30d_in_currency", "price_change_percentage_30d", percent))
                        setChangeFromPercent(coin.change30d, coin.usdPrice, percent);

                    break;
                }
            }
        }
        else
        {
            Serial.print("Market JSON parse failed: ");
            Serial.println(error.c_str());
            Serial.print("Market payload bytes: ");
            Serial.println(payload.length());
            setInvalidCoinStatuses(COIN_FEED_PARSE_ERROR);
        }
    }
    else
    {
        Serial.print("Market fetch failed, HTTP: ");
        Serial.println(httpCode);
        setInvalidCoinStatuses(httpCodeToFeedStatus(httpCode));
    }
    http.end();

    String simpleUrl = "https://api.coingecko.com/api/v3/simple/price?ids=" + ids
                     + "&vs_currencies=usd,inr";

    HTTPClient simpleHttp;
    simpleHttp.begin(simpleUrl);
    simpleHttp.useHTTP10(true);
    simpleHttp.setTimeout(10000);

    int simpleCode = simpleHttp.GET();
    if (simpleCode == HTTP_CODE_OK)
    {
        String payload = simpleHttp.getString();
        DynamicJsonDocument simpleJson(4096);
        DeserializationError simpleError = deserializeJson(simpleJson, payload);
        if (!simpleError)
        {
            for (int i = 0; i < coinCount; i++)
            {
                const char *id = coins[i].id.c_str();
                if (!simpleJson.containsKey(id)) continue;
                simpleMatched[i] = true;

                JsonObjectConst entry = simpleJson[id];
                if (!entry["usd"].isNull())
                {
                    coins[i].usdPrice = entry["usd"].as<double>();
                    markCoinValid(coins[i], fetchedAt);
                }

                if (!entry["inr"].isNull())
                {
                    coins[i].inrPrice = entry["inr"].as<double>();
                    coins[i].hasInr = true;
                }
            }
        }
        else
        {
            Serial.print("Simple price JSON parse failed: ");
            Serial.println(simpleError.c_str());
            Serial.print("Simple payload bytes: ");
            Serial.println(payload.length());
            setInvalidCoinStatuses(COIN_FEED_PARSE_ERROR);
        }
    }
    else
    {
        Serial.print("Simple price fetch failed, HTTP: ");
        Serial.println(simpleCode);
        setInvalidCoinStatuses(httpCodeToFeedStatus(simpleCode));
    }
    simpleHttp.end();

    for (int i = 0; i < coinCount; i++)
    {
        if (coins[i].valid || coins[i].feedStatus != COIN_FEED_NO_DATA) continue;
        coins[i].feedStatus = (!marketMatched[i] && !simpleMatched[i])
            ? COIN_FEED_INVALID_ID
            : COIN_FEED_NO_DATA;
    }

    if (!hasAnyValidCoin(coins, coinCount))
    {
        Serial.print("No valid prices returned for IDs: ");
        Serial.println(ids);

        if (hadValidBefore)
        {
            Serial.println("Keeping previous prices after fetch failure");
            for (int i = 0; i < coinCount; i++)
                coins[i] = previousCoins[i];
            return;
        }

        if (allowDefaultFallback && !projectConfig.usesDefaultCoinSettings())
        {
            Serial.println("Falling back to default coin settings");
            projectConfig.cryptoIds = String(DEFAULT_CORE_CRYPTO_IDS);
            projectConfig.randomCoinCount = DEFAULT_RANDOM_COIN_COUNT;
            refreshResolvedCoins();
            loadSettingsDraftFromConfig();
            fetchPrices(false);
        }
    }
}

void baseProjectSetup()
{
    projectDisplay->displaySetup();
    projectDisplay->showMessage("Starting...");

    bool forceConfig = false;

    drd = new DoubleResetDetector(DRD_TIMEOUT, DRD_ADDRESS);
    if (drd->detectDoubleReset())
    {
        Serial.println("Double reset - forcing config");
        forceConfig = true;
    }

    bool spiffsOk = SPIFFS.begin(false) || SPIFFS.begin(true);
    if (!spiffsOk)
    {
        projectDisplay->showMessage("SPIFFS init failed!");
        while (1) yield();
    }

    if (!projectConfig.fetchConfigFile())
        forceConfig = true;

    if (projectConfig.consumeOpenWifiPortal())
    {
        Serial.println("Settings requested WiFi portal");
        forceConfig = true;
        projectConfig.saveConfigFile();
    }

    setupWiFiManager(forceConfig, projectConfig, projectDisplay);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 40)
    {
        delay(500);
        attempts++;
    }
    if (WiFi.status() != WL_CONNECTED)
    {
        projectDisplay->showMessage("WiFi failed!");
        while (1) yield();
    }

    loadSettingsDraftFromConfig();
    syncRuntimeIntervals();
    refreshResolvedCoins();

    projectDisplay->showMessage("Fetching prices...");
    fetchPrices();
    lastPriceFetch = millis();
    lastAutoRotate = millis();

    renderCurrentScreen();
}

void baseProjectLoop()
{
    drd->loop();

    unsigned long now = millis();

    // Touch interaction
    TouchAction action = projectDisplay->getTouchAction();
    if (action.type == TOUCH_PREV)
    {
        if (settingsScreenActive)
        {
            settingsScreenActive = false;
        }
        else if (coinCount > 0)
        {
            currentCoin = (currentCoin - 1 + coinCount) % coinCount;
        }
        lastAutoRotate = now;
        renderCurrentScreen();
    }
    else if (action.type == TOUCH_NEXT)
    {
        if (settingsScreenActive)
        {
            settingsScreenActive = false;
        }
        else if (coinCount > 0)
        {
            currentCoin = (currentCoin + 1) % coinCount;
        }
        lastAutoRotate = now;
        renderCurrentScreen();
    }
    else if (action.type == TOUCH_OPEN_SETTINGS)
    {
        loadSettingsDraftFromConfig();
        settingsScreenActive = true;
        lastAutoRotate = now;
        renderCurrentScreen();
    }
    else if (action.type == TOUCH_RANDOM_COUNT_DEC)
    {
        if (adjustSettingsRandomCoinCount(-1)) renderCurrentScreen();
    }
    else if (action.type == TOUCH_RANDOM_COUNT_INC)
    {
        if (adjustSettingsRandomCoinCount(1)) renderCurrentScreen();
    }
    else if (action.type == TOUCH_TOGGLE_SETTINGS_COIN)
    {
        if (toggleSettingsCoinSelection(action.value)) renderCurrentScreen();
    }
    else if (action.type == TOUCH_PRICE_REFRESH_DEC)
    {
        if (adjustSettingsPriceRefreshSeconds(-15)) renderCurrentScreen();
    }
    else if (action.type == TOUCH_PRICE_REFRESH_INC)
    {
        if (adjustSettingsPriceRefreshSeconds(15)) renderCurrentScreen();
    }
    else if (action.type == TOUCH_ROTATE_DEC)
    {
        if (adjustSettingsRotateSeconds(-1)) renderCurrentScreen();
    }
    else if (action.type == TOUCH_ROTATE_INC)
    {
        if (adjustSettingsRotateSeconds(1)) renderCurrentScreen();
    }
    else if (action.type == TOUCH_OPEN_WIFI_PORTAL)
    {
        restartIntoWifiPortal();
    }
    else if (action.type == TOUCH_APPLY_SETTINGS && settingsDirty)
    {
        projectDisplay->showMessage("Applying...");
        if (applySettingsDraft())
        {
            projectDisplay->showMessage("Refreshing...");
            fetchPrices();
            lastPriceFetch = now;
            lastAutoRotate = now;
        }
        renderCurrentScreen();
    }

    // Auto-rotate every ROTATE_INTERVAL
    if (!settingsScreenActive && coinCount > 1 && now - lastAutoRotate >= rotateIntervalMs)
    {
        currentCoin = (currentCoin + 1) % coinCount;
        lastAutoRotate = now;
        renderCurrentScreen();
    }

    // Refresh prices every PRICE_INTERVAL
    if (now - lastPriceFetch >= priceIntervalMs)
    {
        fetchPrices();
        lastPriceFetch = now;
        renderCurrentScreen();
    }

    if (WiFi.status() != WL_CONNECTED)
    {
        projectDisplay->showMessage("WiFi disconnected!");
        delay(3000);
        ESP.restart();
    }
}

void setup()
{
    Serial.begin(115200);
    randomSeed((unsigned long)esp_random());
    baseProjectSetup();
}

void loop()
{
    baseProjectLoop();
}

