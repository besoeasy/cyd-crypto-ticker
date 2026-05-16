#ifndef WATCHONLYWALLET_H
#define WATCHONLYWALLET_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <mbedtls/bignum.h>
#include <mbedtls/ecp.h>
#include <mbedtls/md.h>

namespace watch_only_wallet
{
constexpr size_t GAP_LIMIT = 100;
constexpr size_t BATCH_SIZE = 12;

enum class AddressStyle
{
  Legacy,
  NestedSegwit,
  NativeSegwit
};

struct ExtendedPublicKey
{
  uint32_t version = 0;
  uint8_t depth = 0;
  uint32_t childNumber = 0;
  uint8_t chainCode[32] = {0};
  uint8_t publicKey[33] = {0};
  bool mainnet = true;
  AddressStyle style = AddressStyle::NativeSegwit;
};

struct BalanceScanResult
{
  int64_t totalBalance = -1;
  String nextUnusedAddress;
  String error;
  bool valid = false;
};

struct AddressSummary
{
  String address;
  int64_t balance = 0;
  int txCount = 0;
  bool found = false;
};

struct DerivedAddress
{
  String address;
  uint32_t index = 0;
};

struct MpiGuard
{
  mbedtls_mpi value;

  MpiGuard()
  {
    mbedtls_mpi_init(&value);
  }

  ~MpiGuard()
  {
    mbedtls_mpi_free(&value);
  }
};

struct EcpPointGuard
{
  mbedtls_ecp_point value;

  EcpPointGuard()
  {
    mbedtls_ecp_point_init(&value);
  }

  ~EcpPointGuard()
  {
    mbedtls_ecp_point_free(&value);
  }
};

static const char BASE58_ALPHABET[] = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";
static const char BECH32_ALPHABET[] = "qpzry9x8gf2tvdw0s3jn54khce6mua7l";

inline const mbedtls_ecp_group &secp256k1Group()
{
  static mbedtls_ecp_group group;
  static bool initialized = false;

  if (!initialized)
  {
    mbedtls_ecp_group_init(&group);

    mbedtls_mpi_read_string(&group.P, 16, "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEFFFFFC2F");
    mbedtls_mpi_read_string(&group.A, 16, "0");
    mbedtls_mpi_read_string(&group.B, 16, "7");
    mbedtls_mpi_read_string(&group.G.X, 16, "79BE667EF9DCBBAC55A06295CE870B07029BFCDB2DCE28D959F2815B16F81798");
    mbedtls_mpi_read_string(&group.G.Y, 16, "483ADA7726A3C4655DA4FBFC0E1108A8FD17B448A68554199C47D08FFB10D4B8");
    mbedtls_mpi_lset(&group.G.Z, 1);
    mbedtls_mpi_read_string(&group.N, 16, "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEBAAEDCE6AF48A03BBFD25E8CD0364141");

    group.id = MBEDTLS_ECP_DP_NONE;
    group.pbits = 256;
    group.nbits = 256;
    group.h = 1;
    initialized = true;
  }

  return group;
}

inline int base58CharValue(char c)
{
  const char *position = strchr(BASE58_ALPHABET, c);
  if (position == nullptr)
  {
    return -1;
  }
  return static_cast<int>(position - BASE58_ALPHABET);
}

inline bool sha256(const uint8_t *input, size_t inputLength, uint8_t output[32])
{
  const mbedtls_md_info_t *info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
  return info != nullptr && mbedtls_md(info, input, inputLength, output) == 0;
}

inline bool ripemd160(const uint8_t *input, size_t inputLength, uint8_t output[20])
{
  const mbedtls_md_info_t *info = mbedtls_md_info_from_type(MBEDTLS_MD_RIPEMD160);
  return info != nullptr && mbedtls_md(info, input, inputLength, output) == 0;
}

inline bool hash160(const uint8_t *input, size_t inputLength, uint8_t output[20])
{
  uint8_t sha[32];
  return sha256(input, inputLength, sha) && ripemd160(sha, sizeof(sha), output);
}

inline bool sha256d(const uint8_t *input, size_t inputLength, uint8_t output[32])
{
  uint8_t first[32];
  return sha256(input, inputLength, first) && sha256(first, sizeof(first), output);
}

inline String base58Encode(const uint8_t *input, size_t inputLength)
{
  MpiGuard value;
  if (mbedtls_mpi_read_binary(&value.value, input, inputLength) != 0)
  {
    return String();
  }

  char encoded[128];
  size_t encodedPos = sizeof(encoded) - 1;
  encoded[sizeof(encoded) - 1] = '\0';

  while (mbedtls_mpi_cmp_int(&value.value, 0) > 0)
  {
    MpiGuard quotient;
    MpiGuard remainder;

    if (mbedtls_mpi_div_int(&quotient.value, &remainder.value, &value.value, 58) != 0)
    {
      return String();
    }

    uint8_t remainderByte[1] = {0};
    if (mbedtls_mpi_write_binary(&remainder.value, remainderByte, sizeof(remainderByte)) != 0)
    {
      return String();
    }

    encoded[--encodedPos] = BASE58_ALPHABET[remainderByte[0]];
    if (mbedtls_mpi_copy(&value.value, &quotient.value) != 0)
    {
      return String();
    }
  }

  for (size_t i = 0; i < inputLength && input[i] == 0; ++i)
  {
    encoded[--encodedPos] = '1';
  }

  return String(&encoded[encodedPos]);
}

inline String base58CheckEncode(const uint8_t *payload, size_t payloadLength)
{
  uint8_t buffer[128];
  if (payloadLength + 4 > sizeof(buffer))
  {
    return String();
  }

  memcpy(buffer, payload, payloadLength);

  uint8_t checksum[32];
  if (!sha256d(buffer, payloadLength, checksum))
  {
    return String();
  }

  memcpy(buffer + payloadLength, checksum, 4);
  return base58Encode(buffer, payloadLength + 4);
}

inline String encodeP2PKH(const uint8_t hash[20], bool mainnet)
{
  uint8_t payload[21];
  payload[0] = mainnet ? 0x00 : 0x6f;
  memcpy(payload + 1, hash, 20);
  return base58CheckEncode(payload, sizeof(payload));
}

inline String encodeP2SH(const uint8_t scriptHash[20], bool mainnet)
{
  uint8_t payload[21];
  payload[0] = mainnet ? 0x05 : 0xc4;
  memcpy(payload + 1, scriptHash, 20);
  return base58CheckEncode(payload, sizeof(payload));
}

inline bool convertBits(const uint8_t *input, size_t inputLength, int fromBits, int toBits, bool pad, uint8_t *output, size_t outputCapacity, size_t &outputLength)
{
  uint32_t acc = 0;
  int bits = 0;
  const uint32_t maxValue = (1u << toBits) - 1u;
  const uint32_t maxAcc = (1u << (fromBits + toBits - 1)) - 1u;
  outputLength = 0;

  for (size_t i = 0; i < inputLength; ++i)
  {
    uint8_t value = input[i];
    if ((value >> fromBits) != 0)
    {
      return false;
    }

    acc = ((acc << fromBits) | value) & maxAcc;
    bits += fromBits;

    while (bits >= toBits)
    {
      bits -= toBits;
      if (outputLength >= outputCapacity)
      {
        return false;
      }
      output[outputLength++] = static_cast<uint8_t>((acc >> bits) & maxValue);
    }
  }

  if (pad)
  {
    if (bits > 0)
    {
      if (outputLength >= outputCapacity)
      {
        return false;
      }
      output[outputLength++] = static_cast<uint8_t>((acc << (toBits - bits)) & maxValue);
    }
  }
  else if (bits >= fromBits || ((acc << (toBits - bits)) & maxValue) != 0)
  {
    return false;
  }

  return true;
}

inline uint32_t bech32Polymod(const uint8_t *values, size_t length)
{
  static const uint32_t generator[5] = {
      0x3b6a57b2,
      0x26508e6d,
      0x1ea119fa,
      0x3d4233dd,
      0x2a1462b3};

  uint32_t chk = 1;
  for (size_t i = 0; i < length; ++i)
  {
    uint32_t top = chk >> 25;
    chk = ((chk & 0x1ffffff) << 5) ^ values[i];
    for (size_t j = 0; j < 5; ++j)
    {
      if ((top >> j) & 1u)
      {
        chk ^= generator[j];
      }
    }
  }
  return chk;
}

inline void bech32HrpExpand(const char *hrp, uint8_t *output, size_t &outputLength)
{
  outputLength = 0;
  for (const char *p = hrp; *p != '\0'; ++p)
  {
    output[outputLength++] = static_cast<uint8_t>(*p >> 5);
  }
  output[outputLength++] = 0;
  for (const char *p = hrp; *p != '\0'; ++p)
  {
    output[outputLength++] = static_cast<uint8_t>(*p & 31);
  }
}

inline String bech32Encode(const char *hrp, uint8_t witnessVersion, const uint8_t *program, size_t programLength)
{
  uint8_t program5bit[64];
  size_t program5bitLength = 0;
  if (!convertBits(program, programLength, 8, 5, true, program5bit, sizeof(program5bit), program5bitLength))
  {
    return String();
  }

  uint8_t values[128];
  size_t valuesLength = 0;
  bech32HrpExpand(hrp, values, valuesLength);

  values[valuesLength++] = witnessVersion;
  for (size_t i = 0; i < program5bitLength; ++i)
  {
    values[valuesLength++] = program5bit[i];
  }
  for (size_t i = 0; i < 6; ++i)
  {
    values[valuesLength++] = 0;
  }

  uint32_t checksum = bech32Polymod(values, valuesLength) ^ 1u;

  String result = String(hrp) + "1";
  result += BECH32_ALPHABET[witnessVersion];
  for (size_t i = 0; i < program5bitLength; ++i)
  {
    result += BECH32_ALPHABET[program5bit[i]];
  }
  for (int i = 5; i >= 0; --i)
  {
    result += BECH32_ALPHABET[(checksum >> (5 * i)) & 31u];
  }

  return result;
}

inline String encodeWitnessProgram(const uint8_t hash[20], bool mainnet)
{
  return bech32Encode(mainnet ? "bc" : "tb", 0, hash, 20);
}

inline bool decodeBase58Check(const String &input, uint8_t *output, size_t outputCapacity, size_t &outputLength)
{
  MpiGuard value;
  size_t leadingZeros = 0;

  while (leadingZeros < input.length() && input[leadingZeros] == '1')
  {
    ++leadingZeros;
  }

  for (size_t i = 0; i < input.length(); ++i)
  {
    int digit = base58CharValue(input[i]);
    if (digit < 0)
    {
      return false;
    }

    if (mbedtls_mpi_mul_int(&value.value, &value.value, 58) != 0)
    {
      return false;
    }
    if (mbedtls_mpi_add_int(&value.value, &value.value, digit) != 0)
    {
      return false;
    }
  }

  size_t significantLength = mbedtls_mpi_size(&value.value);
  if (leadingZeros + significantLength > outputCapacity)
  {
    return false;
  }

  memset(output, 0, leadingZeros + significantLength);
  if (significantLength > 0 && mbedtls_mpi_write_binary(&value.value, output + leadingZeros, significantLength) != 0)
  {
    return false;
  }

  outputLength = leadingZeros + significantLength;
  if (outputLength < 4)
  {
    return false;
  }

  uint8_t checksum[32];
  if (!sha256d(output, outputLength - 4, checksum))
  {
    return false;
  }

  if (memcmp(checksum, output + outputLength - 4, 4) != 0)
  {
    return false;
  }

  outputLength -= 4;
  return true;
}

inline bool parseExtendedPublicKey(const String &encoded, ExtendedPublicKey &key, String &error)
{
  uint8_t raw[128];
  size_t rawLength = 0;
  if (!decodeBase58Check(encoded, raw, sizeof(raw), rawLength) || rawLength != 78)
  {
    error = "[E2] Invalid xpub";
    return false;
  }

  key.version = (static_cast<uint32_t>(raw[0]) << 24) |
                (static_cast<uint32_t>(raw[1]) << 16) |
                (static_cast<uint32_t>(raw[2]) << 8) |
                static_cast<uint32_t>(raw[3]);
  key.depth = raw[4];
  key.childNumber = (static_cast<uint32_t>(raw[9]) << 24) |
                    (static_cast<uint32_t>(raw[10]) << 16) |
                    (static_cast<uint32_t>(raw[11]) << 8) |
                    static_cast<uint32_t>(raw[12]);
  memcpy(key.chainCode, raw + 13, 32);
  memcpy(key.publicKey, raw + 45, 33);

  switch (key.version)
  {
  case 0x0488b21e:
    key.style = AddressStyle::Legacy;
    key.mainnet = true;
    break;
  case 0x049d7cb2:
    key.style = AddressStyle::NestedSegwit;
    key.mainnet = true;
    break;
  case 0x04b24746:
    key.style = AddressStyle::NativeSegwit;
    key.mainnet = true;
    break;
  case 0x043587cf:
    key.style = AddressStyle::Legacy;
    key.mainnet = false;
    break;
  case 0x044a5262:
    key.style = AddressStyle::NestedSegwit;
    key.mainnet = false;
    break;
  case 0x045f1cf6:
    key.style = AddressStyle::NativeSegwit;
    key.mainnet = false;
    break;
  default:
    error = "[E3] Bad key version";
    return false;
  }

  return true;
}

inline bool hmacSha512(const uint8_t *key, size_t keyLength, const uint8_t *input, size_t inputLength, uint8_t output[64])
{
  const mbedtls_md_info_t *info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA512);
  return info != nullptr && mbedtls_md_hmac(info, key, keyLength, input, inputLength, output) == 0;
}

inline bool childKeyDerivation(const ExtendedPublicKey &parent, uint32_t childIndex, ExtendedPublicKey &child)
{
  mbedtls_ecp_group &group = const_cast<mbedtls_ecp_group &>(secp256k1Group());
  EcpPointGuard parentPoint;
  EcpPointGuard derivedPoint;
  MpiGuard childScalar;

  if (mbedtls_ecp_point_read_binary(&group, &parentPoint.value, parent.publicKey, sizeof(parent.publicKey)) != 0)
  {
    return false;
  }

  uint8_t data[37];
  memcpy(data, parent.publicKey, sizeof(parent.publicKey));
  data[33] = static_cast<uint8_t>((childIndex >> 24) & 0xff);
  data[34] = static_cast<uint8_t>((childIndex >> 16) & 0xff);
  data[35] = static_cast<uint8_t>((childIndex >> 8) & 0xff);
  data[36] = static_cast<uint8_t>(childIndex & 0xff);

  uint8_t hmac[64];
  if (!hmacSha512(parent.chainCode, sizeof(parent.chainCode), data, sizeof(data), hmac))
  {
    return false;
  }

  if (mbedtls_mpi_read_binary(&childScalar.value, hmac, 32) != 0)
  {
    return false;
  }

  if (mbedtls_mpi_cmp_int(&childScalar.value, 0) == 0 || mbedtls_mpi_cmp_mpi(&childScalar.value, &group.N) >= 0)
  {
    return false;
  }

  if (mbedtls_ecp_mul(&group, &derivedPoint.value, &childScalar.value, &group.G, nullptr, nullptr) != 0)
  {
    return false;
  }

  {
    mbedtls_mpi one;
    mbedtls_mpi_init(&one);
    mbedtls_mpi_lset(&one, 1);
    int ret = mbedtls_ecp_muladd(&group, &derivedPoint.value, &one, &derivedPoint.value, &one, &parentPoint.value);
    mbedtls_mpi_free(&one);
    if (ret != 0)
    {
      return false;
    }
  }

  size_t outputLength = 0;
  uint8_t compressed[33];
  if (mbedtls_ecp_point_write_binary(&group, &derivedPoint.value, MBEDTLS_ECP_PF_COMPRESSED, &outputLength, compressed, sizeof(compressed)) != 0 || outputLength != sizeof(compressed))
  {
    return false;
  }

  child = parent;
  memcpy(child.chainCode, hmac + 32, 32);
  memcpy(child.publicKey, compressed, sizeof(compressed));
  child.childNumber = childIndex;
  child.depth = static_cast<uint8_t>(parent.depth + 1);
  return true;
}

inline String addressFromPublicKey(const ExtendedPublicKey &key)
{
  uint8_t pubKeyHash[20];
  if (!hash160(key.publicKey, sizeof(key.publicKey), pubKeyHash))
  {
    return String();
  }

  switch (key.style)
  {
  case AddressStyle::Legacy:
    return encodeP2PKH(pubKeyHash, key.mainnet);
  case AddressStyle::NestedSegwit:
  {
    uint8_t redeemScript[22];
    redeemScript[0] = 0x00;
    redeemScript[1] = 0x14;
    memcpy(redeemScript + 2, pubKeyHash, sizeof(pubKeyHash));
    uint8_t scriptHash[20];
    if (!hash160(redeemScript, sizeof(redeemScript), scriptHash))
    {
      return String();
    }
    return encodeP2SH(scriptHash, key.mainnet);
  }
  case AddressStyle::NativeSegwit:
    return encodeWitnessProgram(pubKeyHash, key.mainnet);
  }

  return String();
}

inline bool deriveAddressAtIndex(const ExtendedPublicKey &accountKey, uint32_t chainIndex, uint32_t addressIndex, DerivedAddress &derived, String &error)
{
  ExtendedPublicKey chainKey;
  if (!childKeyDerivation(accountKey, chainIndex, chainKey))
  {
    error = "[E4] Chain derive";
    return false;
  }

  ExtendedPublicKey addressKey;
  if (!childKeyDerivation(chainKey, addressIndex, addressKey))
  {
    error = "[E5] Addr derive";
    return false;
  }

  derived.address = addressFromPublicKey(addressKey);
  if (derived.address.length() == 0)
  {
    error = "[E6] Encode addr";
    return false;
  }

  derived.index = addressIndex;
  return true;
}

inline bool fetchBalanceBatch(const String &apiBaseUrl, const String *addresses, size_t addressCount, AddressSummary *summaries, String &error)
{
  if (addressCount == 0)
  {
    return true;
  }

  String url = apiBaseUrl + "addrs/";
  for (size_t i = 0; i < addressCount; ++i)
  {
    if (i > 0)
    {
      url += ';';
    }
    url += addresses[i];
  }
  url += "/balance";

  HTTPClient http;
  http.begin(url);
  http.setTimeout(15000);

  int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK)
  {
    error = String("[E7] HTTP ") + httpCode;
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();

  DynamicJsonDocument doc(12288);
  DeserializationError parseError = deserializeJson(doc, payload);
  if (parseError)
  {
    error = String("[E8] JSON: ") + parseError.c_str();
    return false;
  }

  for (size_t i = 0; i < addressCount; ++i)
  {
    summaries[i].address = addresses[i];
    summaries[i].balance = 0;
    summaries[i].txCount = 0;
    summaries[i].found = false;
  }

  auto applyEntry = [&](JsonObjectConst entry) {
    const char *address = entry["address"] | "";
    int64_t balance = entry["final_balance"] | entry["balance"] | 0;
    int txCount = entry["final_n_tx"] | entry["n_tx"] | 0;
    int unconfirmedCount = entry["unconfirmed_n_tx"] | 0;

    for (size_t i = 0; i < addressCount; ++i)
    {
      if (summaries[i].address == address)
      {
        summaries[i].balance = balance;
        summaries[i].txCount = txCount + unconfirmedCount;
        summaries[i].found = true;
        break;
      }
    }
  };

  if (doc.is<JsonArray>())
  {
    for (JsonObjectConst entry : doc.as<JsonArray>())
    {
      applyEntry(entry);
    }
  }
  else if (doc.is<JsonObject>())
  {
    applyEntry(doc.as<JsonObject>());
  }
  else
  {
    error = "[E9] Bad resp format";
    return false;
  }

  for (size_t i = 0; i < addressCount; ++i)
  {
    if (!summaries[i].found)
    {
      error = String("[E10] No resp: ") + summaries[i].address.substring(0, 10);
      return false;
    }
  }

  return true;
}

inline bool scanChain(const ExtendedPublicKey &accountKey, uint32_t chainIndex, bool captureNextUnused, int64_t &chainBalance, String &nextUnusedAddress, const String &apiBaseUrl, String &error)
{
  chainBalance = 0;
  nextUnusedAddress = "";

  uint32_t consecutiveUnused = 0;
  uint32_t addressIndex = 0;
  int32_t lastUsedIndex = -1;

  while (consecutiveUnused < GAP_LIMIT)
  {
    String requestAddresses[BATCH_SIZE];
    DerivedAddress derivedAddresses[BATCH_SIZE];
    size_t requestCount = 0;

    while (requestCount < BATCH_SIZE && consecutiveUnused < GAP_LIMIT)
    {
      DerivedAddress derived;
      if (!deriveAddressAtIndex(accountKey, chainIndex, addressIndex, derived, error))
      {
        return false;
      }

      requestAddresses[requestCount] = derived.address;
      derivedAddresses[requestCount] = derived;
      ++requestCount;
      ++addressIndex;
    }

    AddressSummary summaries[BATCH_SIZE];
    if (!fetchBalanceBatch(apiBaseUrl, requestAddresses, requestCount, summaries, error))
    {
      return false;
    }

    for (size_t i = 0; i < requestCount; ++i)
    {
      chainBalance += summaries[i].balance;
      bool used = summaries[i].txCount > 0;

      if (used)
      {
        consecutiveUnused = 0;
        lastUsedIndex = static_cast<int32_t>(derivedAddresses[i].index);
      }
      else
      {
        ++consecutiveUnused;
      }
    }
  }

  if (captureNextUnused)
  {
    uint32_t nextIndex = lastUsedIndex < 0 ? 0u : static_cast<uint32_t>(lastUsedIndex + 1);
    DerivedAddress nextAddress;
    if (!deriveAddressAtIndex(accountKey, chainIndex, nextIndex, nextAddress, error))
    {
      return false;
    }
    nextUnusedAddress = nextAddress.address;
  }

  return true;
}

inline BalanceScanResult scanWatchOnlyWallet(const String &extendedPublicKey)
{
  BalanceScanResult result;
  if (extendedPublicKey.length() == 0 || extendedPublicKey == "xpub..." || extendedPublicKey == "zpub...")
  {
    result.error = "[E1] Key not set";
    return result;
  }

  ExtendedPublicKey accountKey;
  if (!parseExtendedPublicKey(extendedPublicKey, accountKey, result.error))
  {
    return result;
  }

  const String apiBaseUrl = accountKey.mainnet ? "https://api.blockcypher.com/v1/btc/main/" : "https://api.blockcypher.com/v1/btc/test3/";

  int64_t externalBalance = 0;
  int64_t changeBalance = 0;
  String nextUnusedAddress;
  String chainError;
  String ignoredAddress;

  if (!scanChain(accountKey, 0, true, externalBalance, nextUnusedAddress, apiBaseUrl, chainError))
  {
    result.error = chainError;
    return result;
  }

  if (!scanChain(accountKey, 1, false, changeBalance, ignoredAddress, apiBaseUrl, chainError))
  {
    result.error = chainError;
    return result;
  }

  result.totalBalance = externalBalance + changeBalance;
  result.nextUnusedAddress = nextUnusedAddress;
  result.valid = true;
  return result;
}
} // namespace watch_only_wallet

using watch_only_wallet::BalanceScanResult;
using watch_only_wallet::scanWatchOnlyWallet;

#endif