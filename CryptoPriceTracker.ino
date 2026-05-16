#define ESP_DRD_USE_SPIFFS true

#include <WiFi.h>
#include <FS.h>
#include "SPIFFS.h"
#include <WiFiManager.h>
#include <ESP_DoubleResetDetector.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <math.h>

#include "projectConfig.h"
#include "projectDisplay.h"
#include "wifiManagerHandler.h"
#include "cheapYellowLCD.h"

#define DRD_TIMEOUT 10
#define DRD_ADDRESS 0

#define MAX_COINS 8

ProjectConfig projectConfig;
CheapYellowDisplay cyd;
ProjectDisplay *projectDisplay = &cyd;

CryptoData coins[MAX_COINS];
int coinCount   = 0;
int currentCoin = 0;

unsigned long lastPriceFetch = 0;
unsigned long lastAutoRotate = 0;
const unsigned long PRICE_INTERVAL  = 60000;  // 1 min
const unsigned long ROTATE_INTERVAL = 8000;   // 8 sec

static void resetCoinMarketData(CryptoData &coin)
{
    coin.usdPrice  = 0;
    coin.inrPrice  = 0;
    coin.hasInr    = false;
    coin.change24h = PriceChangeData();
    coin.change7d  = PriceChangeData();
    coin.change30d = PriceChangeData();
    coin.valid     = false;
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

// Split a comma-separated string into the coins[] array
void initCoins()
{
    coinCount = 0;
    const String &src = projectConfig.cryptoIds;
    int start = 0;
    for (int i = 0; i <= (int)src.length() && coinCount < MAX_COINS; i++)
    {
        if (i == (int)src.length() || src[i] == ',')
        {
            String token = src.substring(start, i);
            token.trim();
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

void fetchPrices()
{
    if (coinCount == 0) return;

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

                    CryptoData &coin = coins[i];
                    coin.usdPrice = entry["current_price"] | 0.0;
                    coin.valid = !entry["current_price"].isNull();

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
        }
    }
    else
    {
        Serial.print("Market fetch failed, HTTP: ");
        Serial.println(httpCode);
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

                JsonObjectConst entry = simpleJson[id];
                if (!entry["usd"].isNull())
                {
                    coins[i].usdPrice = entry["usd"].as<double>();
                    coins[i].valid = true;
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
        }
    }
    else
    {
        Serial.print("Simple price fetch failed, HTTP: ");
        Serial.println(simpleCode);
    }
    simpleHttp.end();
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

    initCoins();

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

    projectDisplay->showMessage("Fetching prices...");
    fetchPrices();
    lastPriceFetch = millis();
    lastAutoRotate = millis();

    projectDisplay->drawCoinScreen(coins, coinCount, currentCoin);
}

void baseProjectLoop()
{
    drd->loop();

    unsigned long now = millis();

    // Touch interaction
    TouchAction action = projectDisplay->getTouchAction();
    if (action == TOUCH_PREV)
    {
        currentCoin = (currentCoin - 1 + coinCount) % coinCount;
        lastAutoRotate = now;
        projectDisplay->drawCoinScreen(coins, coinCount, currentCoin);
    }
    else if (action == TOUCH_NEXT)
    {
        currentCoin = (currentCoin + 1) % coinCount;
        lastAutoRotate = now;
        projectDisplay->drawCoinScreen(coins, coinCount, currentCoin);
    }
    else if (action == TOUCH_REFRESH)
    {
        projectDisplay->showMessage("Refreshing...");
        fetchPrices();
        lastPriceFetch = now;
        lastAutoRotate = now;
        projectDisplay->drawCoinScreen(coins, coinCount, currentCoin);
    }

    // Auto-rotate every ROTATE_INTERVAL
    if (now - lastAutoRotate >= ROTATE_INTERVAL)
    {
        currentCoin = (currentCoin + 1) % coinCount;
        lastAutoRotate = now;
        projectDisplay->drawCoinScreen(coins, coinCount, currentCoin);
    }

    // Refresh prices every PRICE_INTERVAL
    if (now - lastPriceFetch >= PRICE_INTERVAL)
    {
        fetchPrices();
        lastPriceFetch = now;
        projectDisplay->drawCoinScreen(coins, coinCount, currentCoin);
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
    baseProjectSetup();
}

void loop()
{
    baseProjectLoop();
}

