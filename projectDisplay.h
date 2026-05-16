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

enum TouchAction
{
  TOUCH_NONE,
  TOUCH_PREV,
  TOUCH_NEXT,
  TOUCH_REFRESH
};

class ProjectDisplay
{
public:
  virtual void displaySetup() = 0;
  virtual void drawWifiManagerMessage(WiFiManager *myWiFiManager) = 0;
  virtual void drawCoinScreen(const CryptoData *coins, int count, int index) = 0;
  virtual void showMessage(const String &msg) = 0;
  virtual TouchAction getTouchAction() { return TOUCH_NONE; }

  void setWidth(int w)  { screenWidth = w; screenCenterX = w / 2; }
  void setHeight(int h) { screenHeight = h; }

protected:
  int screenWidth   = 320;
  int screenHeight  = 240;
  int screenCenterX = 160;
};

#endif
