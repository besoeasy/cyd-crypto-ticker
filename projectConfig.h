#ifndef PROJECTCONFIG_H
#define PROJECTCONFIG_H

#define PROJECT_CONFIG_JSON "/project_config.json"

#define CONFIG_CRYPTO_IDS "cryptoIds"
#define DEFAULT_CRYPTO_IDS "bitcoin,ethereum,solana,the-open-network"

class ProjectConfig
{
public:
  String cryptoIds = DEFAULT_CRYPTO_IDS;

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

    return true;
  }

  bool saveConfigFile()
  {
    StaticJsonDocument<512> json;
    json[CONFIG_CRYPTO_IDS] = cryptoIds;

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
};

#endif
