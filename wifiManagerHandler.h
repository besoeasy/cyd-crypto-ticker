#ifndef WIFIMANAGERHANDLER_H
#define WIFIMANAGERHANDLER_H

DoubleResetDetector *drd;
ProjectDisplay *wm_Display;

bool shouldSaveConfig = false;

void saveConfigCallback()
{
  shouldSaveConfig = true;
}

void configModeCallback(WiFiManager *myWiFiManager)
{
  wm_Display->drawWifiManagerMessage(myWiFiManager);
}

void setupWiFiManager(bool forceConfig, ProjectConfig &config, ProjectDisplay *theDisplay)
{
  wm_Display = theDisplay;
  WiFiManager wm;

  wm.setSaveConfigCallback(saveConfigCallback);
  wm.setAPCallback(configModeCallback);

  WiFiManagerParameter cryptoIdsParam(
      CONFIG_CRYPTO_IDS,
      "Coin IDs (comma-separated CoinGecko IDs)",
      config.cryptoIds.c_str(),
      160);

  wm.addParameter(&cryptoIdsParam);

  if (forceConfig)
  {
    drd->stop();
    if (!wm.startConfigPortal("CryptoTracker", "crypto123"))
      ESP.restart();
  }
  else
  {
    if (!wm.autoConnect("CryptoTracker", "crypto123"))
      ESP.restart();
  }

  if (shouldSaveConfig)
  {
    config.cryptoIds = String(cryptoIdsParam.getValue());
    config.saveConfigFile();
    drd->stop();
    ESP.restart();
  }
}

#endif
