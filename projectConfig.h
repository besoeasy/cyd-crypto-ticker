#ifndef PROJECTCONFIG_H
#define PROJECTCONFIG_H

#define PROJECT_CONFIG_JSON "/project_config.json"

#define CONFIG_CRYPTO_IDS "cryptoIds"
#define CONFIG_RANDOM_COIN_COUNT "randomCoinCount"
#define CONFIG_PRICE_REFRESH_SECONDS "priceRefreshSeconds"
#define CONFIG_ROTATE_SECONDS "rotateSeconds"
#define CONFIG_OPEN_WIFI_PORTAL "openWifiPortal"
#define DEFAULT_CORE_CRYPTO_IDS "bitcoin,ethereum,solana"
#define DEFAULT_CRYPTO_IDS DEFAULT_CORE_CRYPTO_IDS
#define LEGACY_DEFAULT_CRYPTO_IDS "bitcoin,ethereum"
#define DEFAULT_RANDOM_COIN_COUNT 3
#define DEFAULT_PRICE_REFRESH_SECONDS 60
#define DEFAULT_ROTATE_SECONDS 8

class ProjectConfig
{
public:
  String cryptoIds = DEFAULT_CRYPTO_IDS;
  uint8_t randomCoinCount = DEFAULT_RANDOM_COIN_COUNT;
  uint16_t priceRefreshSeconds = DEFAULT_PRICE_REFRESH_SECONDS;
  uint16_t rotateSeconds = DEFAULT_ROTATE_SECONDS;
  bool openWifiPortal = false;

  bool usesDefaultCryptoIds() const
  {
    String configured = normalizeCryptoIdsCsv(cryptoIds);
    return configured == normalizeCryptoIdsCsv(String(DEFAULT_CRYPTO_IDS))
        || configured == normalizeCryptoIdsCsv(String(LEGACY_DEFAULT_CRYPTO_IDS));
  }

  bool usesDefaultCoinSettings() const
  {
    return usesDefaultCryptoIds()
        && randomCoinCount == DEFAULT_RANDOM_COIN_COUNT
        && priceRefreshSeconds == DEFAULT_PRICE_REFRESH_SECONDS
        && rotateSeconds == DEFAULT_ROTATE_SECONDS;
  }

  bool consumeOpenWifiPortal()
  {
    if (!openWifiPortal) return false;
    openWifiPortal = false;
    return true;
  }

  bool fetchConfigFile()
  {
    if (!SPIFFS.exists(PROJECT_CONFIG_JSON))
    {
      Serial.println("Config file does not exist");
      return false;
    }

    File configFile = SPIFFS.open(PROJECT_CONFIG_JSON, "r");
    if (!configFile)
    {
      Serial.println("Failed to open config file");
      return false;
    }

    StaticJsonDocument<512> json;
    DeserializationError error = deserializeJson(json, configFile);
    configFile.close();

    if (error)
    {
      Serial.println("Failed to parse config JSON");
      return false;
    }

    if (json.containsKey(CONFIG_CRYPTO_IDS))
      cryptoIds = json[CONFIG_CRYPTO_IDS].as<String>();

    if (json.containsKey(CONFIG_RANDOM_COIN_COUNT))
      randomCoinCount = clampRandomCoinCount(json[CONFIG_RANDOM_COIN_COUNT].as<int>());
    else
      randomCoinCount = usesDefaultCryptoIds() ? DEFAULT_RANDOM_COIN_COUNT : 0;

    if (json.containsKey(CONFIG_PRICE_REFRESH_SECONDS))
      priceRefreshSeconds = clampPriceRefreshSeconds(json[CONFIG_PRICE_REFRESH_SECONDS].as<int>());

    if (json.containsKey(CONFIG_ROTATE_SECONDS))
      rotateSeconds = clampRotateSeconds(json[CONFIG_ROTATE_SECONDS].as<int>());

    if (json.containsKey(CONFIG_OPEN_WIFI_PORTAL))
      openWifiPortal = json[CONFIG_OPEN_WIFI_PORTAL].as<bool>();

    return true;
  }

  bool saveConfigFile()
  {
    StaticJsonDocument<512> json;
    json[CONFIG_CRYPTO_IDS] = cryptoIds;
    json[CONFIG_RANDOM_COIN_COUNT] = randomCoinCount;
    json[CONFIG_PRICE_REFRESH_SECONDS] = priceRefreshSeconds;
    json[CONFIG_ROTATE_SECONDS] = rotateSeconds;
    json[CONFIG_OPEN_WIFI_PORTAL] = openWifiPortal;

    File configFile = SPIFFS.open(PROJECT_CONFIG_JSON, "w");
    if (!configFile)
    {
      Serial.println("Failed to open config file for writing");
      return false;
    }

    if (serializeJson(json, configFile) == 0)
    {
      configFile.close();
      return false;
    }
    configFile.close();
    return true;
  }

private:
  static uint8_t clampRandomCoinCount(int value)
  {
    if (value < 0) return 0;
    if (value > 8) return 8;
    return (uint8_t)value;
  }

  static uint16_t clampPriceRefreshSeconds(int value)
  {
    if (value < 15) return 15;
    if (value > 300) return 300;
    return (uint16_t)value;
  }

  static uint16_t clampRotateSeconds(int value)
  {
    if (value < 3) return 3;
    if (value > 60) return 60;
    return (uint16_t)value;
  }

  static String normalizeCryptoIdsCsv(const String &value)
  {
    String normalized;
    int start = 0;
    for (int i = 0; i <= (int)value.length(); i++)
    {
      if (i == (int)value.length() || value[i] == ',')
      {
        String token = value.substring(start, i);
        token.trim();
        if (token.length() > 0)
        {
          if (normalized.length() > 0) normalized += ",";
          normalized += token;
        }
        start = i + 1;
      }
    }
    return normalized;
  }
};

#endif
