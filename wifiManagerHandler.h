#ifndef WIFIMANAGERHANDLER_H
#define WIFIMANAGERHANDLER_H

DoubleResetDetector *drd;
ProjectDisplay *wm_Display;
WiFiManager wm;

bool shouldSaveConfig = false;
bool wmConfigured = false;
bool wmShowPortalSplash = false;
bool wmRuntimePortalActive = false;

const char *CONFIG_PORTAL_SSID = "CryptoTracker";
const char *CONFIG_PORTAL_PASSWORD = "crypto123";

WiFiManagerParameter cryptoIdsParam(
  CONFIG_CRYPTO_IDS,
  "Advanced coin IDs override (optional CSV)",
  DEFAULT_CRYPTO_IDS,
  160);
WiFiManagerParameter randomCoinCountParam(
  CONFIG_RANDOM_COIN_COUNT,
  "Random coins (0-8)",
  "3",
  4);
WiFiManagerParameter priceRefreshParam(
  CONFIG_PRICE_REFRESH_SECONDS,
  "Refresh seconds (15-300)",
  "60",
  4);
WiFiManagerParameter rotateSecondsParam(
  CONFIG_ROTATE_SECONDS,
  "Rotate seconds (3-60)",
  "8",
  4);

void saveConfigCallback()
{
  shouldSaveConfig = true;
}

void configModeCallback(WiFiManager *myWiFiManager)
{
  if (wmShowPortalSplash && wm_Display != nullptr)
    wm_Display->drawWifiManagerMessage(myWiFiManager);
}

static int clampPortalInt(int value, int minValue, int maxValue)
{
  if (value < minValue) return minValue;
  if (value > maxValue) return maxValue;
  return value;
}

static void syncPortalParamsFromConfig(const ProjectConfig &config)
{
  char buffer[16];

  cryptoIdsParam.setValue(config.cryptoIds.c_str(), 160);

  snprintf(buffer, sizeof(buffer), "%u", config.randomCoinCount);
  randomCoinCountParam.setValue(buffer, 4);

  snprintf(buffer, sizeof(buffer), "%u", config.priceRefreshSeconds);
  priceRefreshParam.setValue(buffer, 4);

  snprintf(buffer, sizeof(buffer), "%u", config.rotateSeconds);
  rotateSecondsParam.setValue(buffer, 4);
}

static bool applyPortalValuesToConfig(ProjectConfig &config)
{
  shouldSaveConfig = false;

  String nextCryptoIds = String(cryptoIdsParam.getValue());
  int nextRandomCount = clampPortalInt(String(randomCoinCountParam.getValue()).toInt(), 0, 8);
  int nextPriceRefresh = clampPortalInt(String(priceRefreshParam.getValue()).toInt(), 15, 300);
  int nextRotate = clampPortalInt(String(rotateSecondsParam.getValue()).toInt(), 3, 60);

  bool changed = nextCryptoIds != config.cryptoIds
      || nextRandomCount != config.randomCoinCount
      || nextPriceRefresh != config.priceRefreshSeconds
      || nextRotate != config.rotateSeconds;

  config.cryptoIds = nextCryptoIds;
  config.randomCoinCount = (uint8_t)nextRandomCount;
  config.priceRefreshSeconds = (uint16_t)nextPriceRefresh;
  config.rotateSeconds = (uint16_t)nextRotate;
  return changed;
}

static void ensureWiFiManagerConfigured(ProjectConfig &config, ProjectDisplay *theDisplay)
{
  wm_Display = theDisplay;
  if (wmConfigured) return;

  wm.setSaveConfigCallback(saveConfigCallback);
  wm.setAPCallback(configModeCallback);
  wm.addParameter(&cryptoIdsParam);
  wm.addParameter(&randomCoinCountParam);
  wm.addParameter(&priceRefreshParam);
  wm.addParameter(&rotateSecondsParam);
  syncPortalParamsFromConfig(config);
  wmConfigured = true;
}

void setupWiFiManager(bool forceConfig, ProjectConfig &config, ProjectDisplay *theDisplay)
{
  ensureWiFiManagerConfigured(config, theDisplay);
  shouldSaveConfig = false;
  syncPortalParamsFromConfig(config);
  wmShowPortalSplash = forceConfig;
  wm.setConfigPortalBlocking(true);
  wm.setBreakAfterConfig(true);
  wm.setDisableConfigPortal(true);

  if (forceConfig)
  {
    drd->stop();
    if (!wm.startConfigPortal(CONFIG_PORTAL_SSID, CONFIG_PORTAL_PASSWORD))
      ESP.restart();
  }
  else
  {
    if (!wm.autoConnect(CONFIG_PORTAL_SSID, CONFIG_PORTAL_PASSWORD))
      ESP.restart();
  }

  wmShowPortalSplash = false;

  if (shouldSaveConfig)
  {
    applyPortalValuesToConfig(config);
    config.saveConfigFile();
    drd->stop();
    ESP.restart();
  }
}

bool startSettingsConfigPortal(ProjectConfig &config, ProjectDisplay *theDisplay)
{
  ensureWiFiManagerConfigured(config, theDisplay);
  shouldSaveConfig = false;
  syncPortalParamsFromConfig(config);
  wmShowPortalSplash = false;
  wm.setConfigPortalBlocking(false);
  wm.setBreakAfterConfig(false);
  wm.setDisableConfigPortal(false);

  if (wmRuntimePortalActive) return true;

  bool started = wm.startConfigPortal(CONFIG_PORTAL_SSID, CONFIG_PORTAL_PASSWORD);
  wmRuntimePortalActive = started || wm.getConfigPortalSSID().length() > 0;
  return wmRuntimePortalActive;
}

void stopSettingsConfigPortal()
{
  if (!wmRuntimePortalActive) return;
  wm.stopConfigPortal();
  wmRuntimePortalActive = false;
  shouldSaveConfig = false;
}

bool processSettingsConfigPortal(ProjectConfig &config)
{
  if (!wmRuntimePortalActive && !shouldSaveConfig) return false;

  if (wmRuntimePortalActive)
    wm.process();

  if (!shouldSaveConfig) return false;

  bool changed = applyPortalValuesToConfig(config);
  config.saveConfigFile();
  syncPortalParamsFromConfig(config);
  return changed;
}

bool isSettingsConfigPortalActive()
{
  return wmRuntimePortalActive;
}

String getSettingsConfigPortalSsid()
{
  String ssid = wm.getConfigPortalSSID();
  return ssid.length() > 0 ? ssid : String(CONFIG_PORTAL_SSID);
}

String getSettingsConfigPortalPassword()
{
  return String(CONFIG_PORTAL_PASSWORD);
}

String getSettingsConfigPortalIp()
{
  IPAddress ip = WiFi.softAPIP();
  if ((uint32_t)ip == 0) return String("192.168.4.1");
  return ip.toString();
}

#endif
