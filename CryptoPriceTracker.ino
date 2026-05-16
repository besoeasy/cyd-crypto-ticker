#define ESP_DRD_USE_SPIFFS true

#include <WiFi.h>
#include <FS.h>
#include "SPIFFS.h"
#include <WiFiManager.h>
#include <ESP_DoubleResetDetector.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>

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
                coins[coinCount].id       = token;
                coins[coinCount].valid    = false;
                coins[coinCount].usdPrice = 0;
                coins[coinCount].inrPrice = 0;
                coinCount++;
            }
            start = i + 1;
        }
    }
}

void fetchPrices()
{
    if (coinCount == 0) return;

    String ids = "";
    for (int i = 0; i < coinCount; i++)
    {
        if (i > 0) ids += ",";
        ids += coins[i].id;
    }

    String url = "https://api.coingecko.com/api/v3/simple/price?ids=" + ids
                 + "&vs_currencies=usd,inr";

    HTTPClient http;
    http.begin(url);
    http.setTimeout(10000);

    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK)
    {
        String payload = http.getString();
        StaticJsonDocument<1024> json;
        DeserializationError error = deserializeJson(json, payload);
        if (!error)
        {
            for (int i = 0; i < coinCount; i++)
            {
                const char *id = coins[i].id.c_str();
                if (json.containsKey(id))
                {
                    coins[i].usdPrice = json[id]["usd"].as<double>();
                    coins[i].inrPrice = json[id]["inr"].as<double>();
                    coins[i].valid    = true;
                }
                else
                {
                    coins[i].valid = false;
                }
            }
        }
        else
        {
            Serial.print("JSON parse failed: ");
            Serial.println(error.c_str());
        }
    }
    else
    {
        Serial.print("Price fetch failed, HTTP: ");
        Serial.println(httpCode);
        for (int i = 0; i < coinCount; i++)
            coins[i].valid = false;
    }
    http.end();
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

