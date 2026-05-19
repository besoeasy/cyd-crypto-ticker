#ifndef PROJECTDISPLAY_H
#define PROJECTDISPLAY_H

#include <Arduino.h>

struct PriceChangeData
{
  double percent = 0;
  double usdAmount = 0;
  bool available = false;
};

enum CoinFeedStatus
{
  COIN_FEED_OK,
  COIN_FEED_NETWORK,
  COIN_FEED_RATE_LIMITED,
  COIN_FEED_INVALID_ID,
  COIN_FEED_PARSE_ERROR,
  COIN_FEED_NO_DATA
};

struct CryptoData
{
  String id;
  double usdPrice = 0;
  double inrPrice = 0;
  bool hasInr = false;
  PriceChangeData change24h;
  PriceChangeData change7d;
  PriceChangeData change30d;
  bool valid = false;
  unsigned long lastUpdatedMs = 0;
  CoinFeedStatus feedStatus = COIN_FEED_NO_DATA;
};

struct SettingsCoinOption
{
  const char *id;
  const char *label;
};

struct SettingsViewData
{
  const SettingsCoinOption *options = nullptr;
  const bool *selected = nullptr;
  int optionCount = 0;
  int selectedCount = 0;
  int hiddenSelectedCount = 0;
  int maxBaseCoinCount = 0;
  int randomCoinCount = 0;
  int maxRandomCoinCount = 0;
  int priceRefreshSeconds = 0;
  int rotateSeconds = 0;
  bool dirty = false;
};

enum TouchActionType
{
  TOUCH_NONE,
  TOUCH_PREV,
  TOUCH_NEXT,
  TOUCH_REFRESH,
  TOUCH_OPEN_SETTINGS,
  TOUCH_APPLY_SETTINGS,
  TOUCH_RANDOM_COUNT_DEC,
  TOUCH_RANDOM_COUNT_INC,
  TOUCH_TOGGLE_SETTINGS_COIN,
  TOUCH_PRICE_REFRESH_DEC,
  TOUCH_PRICE_REFRESH_INC,
  TOUCH_ROTATE_DEC,
  TOUCH_ROTATE_INC,
  TOUCH_OPEN_WIFI_PORTAL
};

struct TouchAction
{
  TouchAction() = default;
  TouchAction(TouchActionType actionType, int actionValue)
      : type(actionType), value(actionValue) {}

  TouchActionType type = TOUCH_NONE;
  int value = -1;
};

class ProjectDisplay
{
public:
  virtual void displaySetup() = 0;
  virtual void drawWifiManagerMessage(WiFiManager *myWiFiManager) = 0;
  virtual void drawCoinScreen(const CryptoData *coins, int count, int index) = 0;
  virtual void drawSettingsScreen(const SettingsViewData &data) = 0;
  virtual void showMessage(const String &msg) = 0;
  virtual TouchAction getTouchAction() { return TouchAction(); }

  void setWidth(int w)  { screenWidth = w; screenCenterX = w / 2; }
  void setHeight(int h) { screenHeight = h; }

protected:
  int screenWidth   = 320;
  int screenHeight  = 240;
  int screenCenterX = 160;
};

#endif
