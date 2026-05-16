#ifndef PROJECTDISPLAY_H
#define PROJECTDISPLAY_H

#include <Arduino.h>

struct CryptoData
{
  String id;
  double usdPrice = 0;
  double inrPrice = 0;
  bool valid = false;
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
