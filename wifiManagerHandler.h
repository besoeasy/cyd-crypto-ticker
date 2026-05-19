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
  shouldSaveConfig = false;
  WiFiManager wm;

  wm.setSaveConfigCallback(saveConfigCallback);
  wm.setAPCallback(configModeCallback);
  wm.setBreakAfterConfig(true);

  WiFiManagerParameter cryptoIdsParam(
      CONFIG_CRYPTO_IDS,
      "Advanced coin IDs override (optional CSV)",
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
