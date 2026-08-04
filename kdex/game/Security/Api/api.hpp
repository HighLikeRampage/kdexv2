#pragma once

#include <optional>
#include <string>
#include <vector>

#include "../LazyImporter.hpp"
#include "../xorstr.hpp"
#include "json.hpp"

#if defined(_WIN32) || defined(_WIN64)
#include <algorithm>
#include <windows.h>
#include <intrin.h>
#include <sstream>
#include <iomanip>
#include <winioctl.h>
#define CURL_STATICLIB
#include "curl/curl.h"
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "Normaliz.lib")
#pragma comment(lib, "Crypt32.lib")
#pragma comment(lib, "Wldap32.lib")
#pragma comment(lib, "libcurl.lib")
#endif

namespace Security::Api {
namespace crypto {
inline std::string process(const std::string &data) {
  std::string key = xorstr("f7e8d9c8b7a69584736251403f2e1d0c");
  std::string output = data;
  uint32_t state[4] = {0xdeadbeef, 0xcafebabe, 0x1337c0de, 0x8badf00d};
  for (size_t i = 0; i < key.size(); ++i) {
    state[i % 4] ^= key[i];
  }
  for (size_t i = 0; i < data.size(); ++i) {
    state[0] += state[1];
    state[3] ^= state[0];
    state[3] = (state[3] << 16) | (state[3] >> 16);
    state[2] += state[3];
    state[1] ^= state[2];
    state[1] = (state[1] << 12) | (state[1] >> 20);
    state[0] += state[1];
    state[3] ^= state[0];
    state[3] = (state[3] << 8) | (state[3] >> 24);
    state[2] += state[3];
    state[1] ^= state[2];
    state[1] = (state[1] << 7) | (state[1] >> 25);
    output[i] = data[i] ^ (state[(i % 4)] & 0xFF);
  }
  return output;
}

inline std::string to_base64(const std::string &data) {
  static const std::string lut_storage =
      xorstr("mN9bV8cZ7xX6aA5sS4dD3fF2gG1hH0jJqQwWeErRtTyYuUiIoOpPkKlLzC+vB/nM");
  const char *lut = lut_storage.c_str();
  std::string out;
  int val = 0, valb = -6;
  for (uint8_t c : data) {
    val = (val << 8) + c;
    valb += 8;
    while (valb >= 0) {
      out.push_back(lut[(val >> valb) & 0x3F]);
      valb -= 6;
    }
  }
  if (valb > -6)
    out.push_back(lut[((val << 8) >> (valb + 8)) & 0x3F]);
  while (out.size() % 4)
    out.push_back('=');
  return out;
}

inline std::string from_base64(const std::string &data) {
  static const int lut[] = {
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63,
      39, 37, 33, 29, 25, 21, 17, 13, 9,  5,  -1, -1, -1, -1, -1, -1,
      -1, 19, 61, 59, 23, 43, 27, 31, 35, 47, 41, 53, 57, 1,  -1, 49,
      45, 44, 22, 42, 48, 20, 40, 16, 52, 56, -1, -1, -1, -1, -1, -1,
      -1, 18, 3,  10, 24, 46, 28, 30, 34, 51, 38, 54, 58, 63, 60, 50,
      4,  36, 26, 32, 55, 62, 45, 14, 15, 55, -1, -1, -1, -1, -1};
  std::string out;
  int val = 0, valb = -8;
  for (uint8_t c : data) {
    if (c == '=')
      break;
    int l = -1;
    if (c == 'm')
      l = 0;
    else if (c == 'N')
      l = 1;
    else if (c == '9')
      l = 2;
    else if (c == 'b')
      l = 3;
    else if (c == 'V')
      l = 4;
    else if (c == '8')
      l = 5;
    else if (c == 'c')
      l = 6;
    else if (c == 'Z')
      l = 7;
    else if (c == '7')
      l = 8;
    else if (c == 'x')
      l = 9;
    else if (c == 'X')
      l = 10;
    else if (c == '6')
      l = 11;
    else if (c == 'a')
      l = 12;
    else if (c == 'A')
      l = 13;
    else if (c == '5')
      l = 14;
    else if (c == 's')
      l = 15;
    else if (c == 'S')
      l = 16;
    else if (c == '4')
      l = 17;
    else if (c == 'd')
      l = 18;
    else if (c == 'D')
      l = 19;
    else if (c == '3')
      l = 20;
    else if (c == 'f')
      l = 21;
    else if (c == 'F')
      l = 22;
    else if (c == '2')
      l = 23;
    else if (c == 'g')
      l = 24;
    else if (c == 'G')
      l = 25;
    else if (c == '1')
      l = 26;
    else if (c == 'h')
      l = 27;
    else if (c == 'H')
      l = 28;
    else if (c == '0')
      l = 29;
    else if (c == 'j')
      l = 30;
    else if (c == 'J')
      l = 31;
    else if (c == 'q')
      l = 32;
    else if (c == 'Q')
      l = 33;
    else if (c == 'w')
      l = 34;
    else if (c == 'W')
      l = 35;
    else if (c == 'e')
      l = 36;
    else if (c == 'E')
      l = 37;
    else if (c == 'r')
      l = 38;
    else if (c == 'R')
      l = 39;
    else if (c == 't')
      l = 40;
    else if (c == 'T')
      l = 41;
    else if (c == 'y')
      l = 42;
    else if (c == 'Y')
      l = 43;
    else if (c == 'u')
      l = 44;
    else if (c == 'U')
      l = 45;
    else if (c == 'i')
      l = 46;
    else if (c == 'I')
      l = 47;
    else if (c == 'o')
      l = 48;
    else if (c == 'O')
      l = 49;
    else if (c == 'p')
      l = 50;
    else if (c == 'P')
      l = 51;
    else if (c == 'k')
      l = 52;
    else if (c == 'K')
      l = 53;
    else if (c == 'l')
      l = 54;
    else if (c == 'L')
      l = 55;
    else if (c == 'z')
      l = 56;
    else if (c == 'C')
      l = 57;
    else if (c == '+')
      l = 58;
    else if (c == 'v')
      l = 59;
    else if (c == 'B')
      l = 60;
    else if (c == '/')
      l = 61;
    else if (c == 'n')
      l = 62;
    else if (c == 'M')
      l = 63;
    if (l == -1)
      continue;
    val = (val << 6) + l;
    valb += 6;
    if (valb >= 0) {
      out.push_back(char((val >> valb) & 0xFF));
      valb -= 8;
    }
  }
  return out;
}
}

namespace Hwidgen {
inline std::string GetCPUSerial() {
  int cpuInfo[4] = {0};
  __cpuid(cpuInfo, 1);
  std::stringstream ss;
  ss << std::hex << std::setfill('0') << std::setw(8) << cpuInfo[0]
     << std::setw(8) << cpuInfo[3];
  return ss.str();
}

inline std::string GetPhysicalDiskSerial() {
  std::string serial = xorstr("UNKNOWN_DISK");

  HANDLE hDevice =
      LI_FN(CreateFileA)(xorstr("\\\\.\\PhysicalDrive0"), 0,
                         FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                         OPEN_EXISTING, 0, nullptr);

  if (hDevice == INVALID_HANDLE_VALUE)
    return serial;

  STORAGE_PROPERTY_QUERY storagePropertyQuery{};
  storagePropertyQuery.PropertyId = StorageDeviceProperty;
  storagePropertyQuery.QueryType = PropertyStandardQuery;

  STORAGE_DESCRIPTOR_HEADER storageDescriptorHeader{};
  DWORD dwBytesReturned = 0;

  if (LI_FN(DeviceIoControl)(hDevice, IOCTL_STORAGE_QUERY_PROPERTY,
                             &storagePropertyQuery,
                             sizeof(STORAGE_PROPERTY_QUERY),
                             &storageDescriptorHeader,
                             sizeof(STORAGE_DESCRIPTOR_HEADER),
                             &dwBytesReturned, nullptr)) {
    const DWORD dwOutBufferSize = storageDescriptorHeader.Size;
    std::vector<BYTE> pOutBuffer(dwOutBufferSize);

    if (LI_FN(DeviceIoControl)(hDevice, IOCTL_STORAGE_QUERY_PROPERTY,
                               &storagePropertyQuery,
                               sizeof(STORAGE_PROPERTY_QUERY), pOutBuffer.data(),
                               dwOutBufferSize, &dwBytesReturned, nullptr)) {
      STORAGE_DEVICE_DESCRIPTOR *pDeviceDescriptor =
          reinterpret_cast<STORAGE_DEVICE_DESCRIPTOR *>(pOutBuffer.data());

      if (pDeviceDescriptor->SerialNumberOffset) {
        char *rawSerial = reinterpret_cast<char *>(
            pOutBuffer.data() + pDeviceDescriptor->SerialNumberOffset);
        serial = "";
        for (int i = 0; rawSerial[i] != '\0'; ++i) {
          if (rawSerial[i] > 32 && rawSerial[i] < 127) {
            serial += rawSerial[i];
          }
        }
      }
    }
  }

  LI_FN(CloseHandle)(hDevice);
  return serial;
}

inline std::string CalculateSHA256(const std::string &input) {
  HCRYPTPROV hProv = 0;
  HCRYPTHASH hHash = 0;
  std::string hashStr = "";

  auto advapi = LI_FN(LoadLibraryA)(xorstr("advapi32.dll"));

  auto pCryptAcquireContextA = LI_FN(CryptAcquireContextA).in(advapi);
  auto pCryptCreateHash = LI_FN(CryptCreateHash).in(advapi);
  auto pCryptHashData = LI_FN(CryptHashData).in(advapi);
  auto pCryptGetHashParam = LI_FN(CryptGetHashParam).in(advapi);
  auto pCryptDestroyHash = LI_FN(CryptDestroyHash).in(advapi);
  auto pCryptReleaseContext = LI_FN(CryptReleaseContext).in(advapi);

  if (!pCryptAcquireContextA || !pCryptCreateHash)
    return hashStr;

  if (pCryptAcquireContextA(&hProv, nullptr, nullptr, 24, 0xF0000000)) {
    if (pCryptCreateHash(hProv, 0x0000800c, 0, 0, &hHash)) {
      if (pCryptHashData(hHash, reinterpret_cast<const BYTE *>(input.data()),
                         (DWORD)input.length(), 0)) {
        DWORD hashLen = 0;
        DWORD hashLenSize = sizeof(DWORD);

        pCryptGetHashParam(hHash, 0x0004, reinterpret_cast<BYTE *>(&hashLen),
                           &hashLenSize, 0);

        std::vector<BYTE> buffer(hashLen);

        if (pCryptGetHashParam(hHash, 0x0002, buffer.data(), &hashLen, 0)) {
          std::stringstream ss;
          for (DWORD i = 0; i < hashLen; ++i) {
            ss << std::hex << std::setfill('0') << std::setw(2)
               << (int)buffer[i];
          }
          hashStr = ss.str();
        }
      }
      pCryptDestroyHash(hHash);
    }
    pCryptReleaseContext(hProv, 0);
  }

  return hashStr;
}

inline std::string getHwid() {
  std::string cpu = GetCPUSerial();
  std::string disk = GetPhysicalDiskSerial();
  std::string salt = (const char *)xorstr("Negro!@#");

  std::string rawHWID = cpu + xorstr("-") + disk + xorstr("-") + salt;

  return CalculateSHA256(rawHWID);
}

inline std::string getTrustedDeviceStorePath() {
  wchar_t roaming[MAX_PATH] = {};
  DWORD n = GetEnvironmentVariableW(L"APPDATA", roaming, MAX_PATH);
  std::string base;
  if (n > 0 && n < MAX_PATH) {
    char narrow[MAX_PATH * 2] = {};
    WideCharToMultiByte(CP_UTF8, 0, roaming, -1, narrow, sizeof(narrow), nullptr, nullptr);
    base = narrow;
  } else {
    base = ".";
  }
  base += xorstr("\\discord");
  CreateDirectoryA(base.c_str(), nullptr);
  return base + xorstr("\\kdex_dev.bin");
}

inline std::string readTrustedDeviceToken() {
  std::string path = getTrustedDeviceStorePath();
  HANDLE h = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (h == INVALID_HANDLE_VALUE) return {};
  DWORD sz = GetFileSize(h, nullptr);
  if (sz == 0 || sz > 4096) { CloseHandle(h); return {}; }
  std::string buf; buf.resize(sz);
  DWORD read = 0;
  ReadFile(h, buf.data(), sz, &read, nullptr);
  CloseHandle(h);
  buf.resize(read);
  return buf;
}

inline void writeTrustedDeviceToken(const std::string& token) {
  std::string path = getTrustedDeviceStorePath();
  if (token.empty()) { DeleteFileA(path.c_str()); return; }
  HANDLE h = CreateFileA(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_HIDDEN, nullptr);
  if (h == INVALID_HANDLE_VALUE) return;
  DWORD w = 0;
  WriteFile(h, token.data(), (DWORD)token.size(), &w, nullptr);
  CloseHandle(h);
}
}

namespace detail {

inline std::string wide_to_utf8(const wchar_t *w) {
  std::string s;
  for (; *w; ++w) {
    if (*w < 128)
      s += static_cast<char>(*w);
    else
      s += '?';
  }
  return s;
}

inline nlohmann::json build_device_info() {
  nlohmann::json info;
  info[xorstr("platform")] = xorstr("windows");
#if defined(_WIN32) || defined(_WIN64)
  wchar_t hostname[256] = {};
  DWORD len = static_cast<DWORD>(std::size(hostname));
  if (GetComputerNameW(hostname, &len))
    info[xorstr("hostname")] = wide_to_utf8(hostname);
  SYSTEM_INFO si = {};
  GetNativeSystemInfo(&si);
  switch (si.wProcessorArchitecture) {
    case PROCESSOR_ARCHITECTURE_AMD64: info[xorstr("arch")] = xorstr("x64"); break;
    case PROCESSOR_ARCHITECTURE_ARM64: info[xorstr("arch")] = xorstr("arm64"); break;
    case PROCESSOR_ARCHITECTURE_INTEL: info[xorstr("arch")] = xorstr("x86"); break;
    default: info[xorstr("arch")] = xorstr("unknown"); break;
  }
  info[xorstr("osType")] = xorstr("Windows_NT");
#endif
  return info;
}

static size_t WriteCallback(void *contents, size_t size, size_t nmemb,
                            void *userp) {
  ((std::string *)userp)->append((char *)contents, size * nmemb);
  return size * nmemb;
}

inline bool curl_request(const std::string &method, const std::string &url,
                         const std::string &bearer_token,
                         const std::string &payload, long &out_status,
                         std::string &out_body) {
  CURL *curl = curl_easy_init();
  if (!curl)
    return false;

  struct curl_slist *headers = NULL;
  headers = curl_slist_append(headers,
                              xorstr("Content-Type: application/octet-stream"));
  headers = curl_slist_append(headers, xorstr("X-Client: loader"));

  if (!bearer_token.empty()) {
    std::string auth =
        std::string(xorstr("Authorization: Bearer ")) + bearer_token;
    headers = curl_slist_append(headers, auth.c_str());
  }

  std::string processed_payload = payload;
  std::string domain = xorstr("windowsupates.live");
  bool is_kdex = (url.find(domain) != std::string::npos);

  if (!payload.empty() && is_kdex) {
    processed_payload = crypto::to_base64(crypto::process(payload));
  }

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method.c_str());

  if (method != xorstr("GET") && method != xorstr("DELETE")) {
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, processed_payload.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE,
                     (long)processed_payload.length());
  }

  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &out_body);
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

  CURLcode res = curl_easy_perform(curl);
  if (res == CURLE_OK) {
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &out_status);
    if (!out_body.empty() && is_kdex) {
      out_body = crypto::process(crypto::from_base64(out_body));
    }
  }

  curl_slist_free_all(headers);
  curl_easy_cleanup(curl);

  return (res == CURLE_OK);
}

inline bool http_post_json_with_auth(const std::string &url,
                                     const std::string &bearer_token,
                                     const nlohmann::json &body,
                                     long &out_status, std::string &out_body) {
  std::string payload = body.dump();
  return curl_request(xorstr("POST"), url, bearer_token, payload, out_status,
                      out_body);
}

inline bool http_post_json(const std::string &url, const nlohmann::json &body,
                           long &out_status, std::string &out_body) {
  return http_post_json_with_auth(url, std::string(), body, out_status,
                                  out_body);
}

inline bool http_patch_json_with_auth(const std::string &url,
                                      const std::string &bearer_token,
                                      const nlohmann::json &body,
                                      long &out_status, std::string &out_body) {
  std::string payload = body.dump();
  return curl_request(xorstr("PATCH"), url, bearer_token, payload, out_status,
                      out_body);
}

inline bool http_get_json(const std::string &url,
                          const std::string &bearer_token, long &out_status,
                          std::string &out_body) {
  return curl_request(xorstr("GET"), url, bearer_token, "", out_status,
                      out_body);
}

inline bool http_delete_json(const std::string &url,
                             const std::string &bearer_token, long &out_status,
                             std::string &out_body) {
  return curl_request(xorstr("DELETE"), url, bearer_token, "", out_status,
                      out_body);
}

inline std::string map_error_code(const std::string &code) {
  if (code == xorstr("invalid_credentials"))
    return xorstr("Invalid username or password");
  if (code == xorstr("hwid_mismatch"))
    return xorstr("This device isn't linked to your account. Log in from the "
                  "device you used to register.");
  if (code == xorstr("register_failed"))
    return xorstr("Check: username 3-32 chars, valid email, password 8+ chars, "
                  "key format KDEX-XXXXX (5 letters/numbers)");
  if (code == xorstr("invalid_input"))
    return xorstr("Invalid request. Check your input.");
  if (code == xorstr("already_exists"))
    return xorstr("User already exists");
  if (code == xorstr("invalid_key"))
    return xorstr("Invalid activation key");
  if (code == xorstr("banned"))
    return xorstr("Your account is banned");
  if (code == xorstr("locked"))
    return xorstr("Please try again later.");
  if (code == xorstr("invalid_token"))
    return xorstr("Session expired. Please log in.");
  if (code == xorstr("subscription_expired"))
    return xorstr("Your subscription has expired.");
  if (code == xorstr("no_trusted_device"))
    return xorstr("No saved session on this device.");
  return xorstr("Authentication failed");
}

}

struct DeviceStatusResult {
  int status = 0;
  std::string message;
  int ban_user_id = 0;
};

inline std::string base_url() { return xorstr("https://windowsupates.live"); }

inline DeviceStatusResult device_status() {
  DeviceStatusResult result;
  long status = 0;
  std::string body;
  nlohmann::json payload{{xorstr("hwid"), Hwidgen::getHwid()},
                         {xorstr("deviceInfo"), detail::build_device_info()}};
  if (!detail::http_post_json(base_url() + xorstr("/auth/device-status"),
                              payload, status, body)) {
    result.status = 0;
    return result;
  }
  try {
    nlohmann::json json = nlohmann::json::parse(body);
    std::string s = json.value(xorstr("status"), std::string(xorstr("ok")));
    if (s == xorstr("banned")) {
      result.status = 1;
      result.message = json.value(
          xorstr("message"), std::string(xorstr("Your device is banned.")));
      result.ban_user_id = json.value(xorstr("userId"), 0);
    } else if (s == xorstr("locked")) {
      result.status = 2;
      result.message = json.value(
          xorstr("message"), std::string(xorstr("Please try again later.")));
    }
  } catch (...) {
  }
  return result;
}

struct MonitorConfigResult {
  bool secondMonitor = false;
  std::string username;
};

inline MonitorConfigResult check_monitor_config(const std::string &username) {
  MonitorConfigResult result;
  if (username.empty()) return result;

  long status = 0;
  std::string body;
  std::string url =
      base_url() + xorstr("/configs/device-status?username=") + username;

  if (!detail::http_get_json(url, std::string(), status, body)) return result;
  if (status != 200) return result;

  try {
    nlohmann::json json = nlohmann::json::parse(body);
    result.username = json.value(xorstr("username"), std::string());
    result.secondMonitor = json.value(xorstr("secondMonitor"), false);
  } catch (...) {
  }
  return result;
}

struct AuthResult {
  bool success{false};
  std::string error_message;
  std::string access_token;
  std::string refresh_token;
  std::string trusted_device_token;
  std::string username;
  int days_left{0};
  std::string subscription_expires_at;
};

inline AuthResult login(const std::string &username,
                        const std::string &password,
                        bool remember = false) {
  AuthResult result{};

  long status = 0;
  std::string body;
  nlohmann::json payload{{xorstr("username"), username},
                         {xorstr("password"), password},
                         {xorstr("remember"), remember},
                         {xorstr("hwid"), Hwidgen::getHwid()},
                         {xorstr("deviceInfo"), detail::build_device_info()}};

  if (!detail::http_post_json(base_url() + xorstr("/auth/login"), payload,
                              status, body)) {
    result.error_message = xorstr("Failed to contact authentication server");
    return result;
  }

  nlohmann::json json;
  try {
    json = nlohmann::json::parse(body);
  } catch (...) {
    result.error_message =
        xorstr("Invalid response from authentication server");
    return result;
  }

  if (status != 200) {
    std::string code = json.value(xorstr("error"), std::string{});
    if (json.contains(xorstr("message")) && json[xorstr("message")].is_string())
      result.error_message = json[xorstr("message")].get<std::string>();
    else
      result.error_message = detail::map_error_code(code);
    return result;
  }

  result.success = true;
  result.access_token = json.value(xorstr("accessToken"), std::string{});
  result.refresh_token = json.value(xorstr("refreshToken"), std::string{});
  if (json.contains(xorstr("trustedDeviceToken")) &&
      json[xorstr("trustedDeviceToken")].is_string())
    result.trusted_device_token =
        json[xorstr("trustedDeviceToken")].get<std::string>();
  if (json.contains(xorstr("user")) && json[xorstr("user")].is_object())
    result.username =
        json[xorstr("user")].value(xorstr("username"), std::string{});

  if (json.contains(xorstr("subscription")) &&
      json[xorstr("subscription")].is_object()) {
    result.days_left =
        json[xorstr("subscription")].value(xorstr("daysLeft"), 0);
    if (json[xorstr("subscription")].contains(xorstr("expiresAt")) &&
        !json[xorstr("subscription")][xorstr("expiresAt")].is_null())
      result.subscription_expires_at =
          json[xorstr("subscription")][xorstr("expiresAt")].get<std::string>();
  }

  if (!result.trusted_device_token.empty())
    Hwidgen::writeTrustedDeviceToken(result.trusted_device_token);

  return result;
}

inline AuthResult register_user(const std::string &username,
                                const std::string &email,
                                const std::string &password,
                                const std::string &key,
                                bool remember = false) {
  AuthResult result{};

  long status = 0;
  std::string body;
  nlohmann::json payload{{xorstr("username"), username},
                         {xorstr("email"), email},
                         {xorstr("password"), password},
                         {xorstr("key"), key},
                         {xorstr("remember"), remember},
                         {xorstr("hwid"), Hwidgen::getHwid()},
                         {xorstr("deviceInfo"), detail::build_device_info()}};

  if (!detail::http_post_json(base_url() + xorstr("/auth/register"), payload,
                              status, body)) {
    result.error_message = xorstr("Failed to contact authentication server");
    return result;
  }

  nlohmann::json json;
  try {
    json = nlohmann::json::parse(body);
  } catch (...) {
    result.error_message =
        xorstr("Invalid response from authentication server");
    return result;
  }

  if (status != 200) {
    std::string code = json.value(xorstr("error"), std::string{});
    if (json.contains(xorstr("message")) && json[xorstr("message")].is_string())
      result.error_message = json[xorstr("message")].get<std::string>();
    else
      result.error_message = detail::map_error_code(code);
    return result;
  }

  result.success = true;
  result.access_token = json.value(xorstr("accessToken"), std::string{});
  result.refresh_token = json.value(xorstr("refreshToken"), std::string{});
  if (json.contains(xorstr("trustedDeviceToken")) &&
      json[xorstr("trustedDeviceToken")].is_string())
    result.trusted_device_token =
        json[xorstr("trustedDeviceToken")].get<std::string>();
  if (json.contains(xorstr("user")) && json[xorstr("user")].is_object())
    result.username =
        json[xorstr("user")].value(xorstr("username"), std::string{});

  if (json.contains(xorstr("subscription")) &&
      json[xorstr("subscription")].is_object()) {
    result.days_left =
        json[xorstr("subscription")].value(xorstr("daysLeft"), 0);
    if (json[xorstr("subscription")].contains(xorstr("expiresAt")) &&
        !json[xorstr("subscription")][xorstr("expiresAt")].is_null())
      result.subscription_expires_at =
          json[xorstr("subscription")][xorstr("expiresAt")].get<std::string>();
  }

  if (!result.trusted_device_token.empty())
    Hwidgen::writeTrustedDeviceToken(result.trusted_device_token);

  return result;
}

inline AuthResult hwid_login() {
  AuthResult result{};
  std::string stored = Hwidgen::readTrustedDeviceToken();
  if (stored.empty()) {
    result.error_message = xorstr("no_trusted_device");
    return result;
  }

  long status = 0;
  std::string body;
  nlohmann::json payload{
      {xorstr("trustedDeviceToken"), stored},
      {xorstr("hwid"), Hwidgen::getHwid()},
      {xorstr("deviceInfo"), detail::build_device_info()}};

  if (!detail::http_post_json(base_url() + xorstr("/auth/hwid-login"), payload,
                              status, body)) {
    result.error_message = xorstr("Failed to contact authentication server");
    return result;
  }

  nlohmann::json json;
  try {
    json = nlohmann::json::parse(body);
  } catch (...) {
    result.error_message = xorstr("Invalid response");
    return result;
  }

  if (status != 200) {
    std::string code = json.value(xorstr("error"), std::string{});
    result.error_message = detail::map_error_code(code);
    if (code == xorstr("invalid_token") || code == xorstr("hwid_mismatch") ||
        code == xorstr("subscription_expired") || code == xorstr("banned"))
      Hwidgen::writeTrustedDeviceToken(std::string());
    return result;
  }

  result.success = true;
  result.access_token = json.value(xorstr("accessToken"), std::string{});
  result.refresh_token = json.value(xorstr("refreshToken"), std::string{});
  result.trusted_device_token =
      json.value(xorstr("trustedDeviceToken"), stored);
  if (json.contains(xorstr("user")) && json[xorstr("user")].is_object())
    result.username =
        json[xorstr("user")].value(xorstr("username"), std::string{});
  if (json.contains(xorstr("subscription")) &&
      json[xorstr("subscription")].is_object()) {
    result.days_left =
        json[xorstr("subscription")].value(xorstr("daysLeft"), 0);
    if (json[xorstr("subscription")].contains(xorstr("expiresAt")) &&
        !json[xorstr("subscription")][xorstr("expiresAt")].is_null())
      result.subscription_expires_at =
          json[xorstr("subscription")][xorstr("expiresAt")].get<std::string>();
  }
  return result;
}

inline bool check_subscription(const std::string &access_token,
                               bool &active_out, int &days_left_out,
                               std::string &error_out) {
  active_out = false;
  days_left_out = 0;
  error_out.clear();

  if (access_token.empty()) {
    error_out = xorstr("Missing access token");
    return false;
  }

  long status = 0;
  std::string body;
  if (!detail::http_get_json(base_url() + xorstr("/subscription/check"),
                             access_token, status, body)) {
    error_out = xorstr("Failed to contact authentication server");
    return false;
  }

  nlohmann::json json;
  try {
    json = nlohmann::json::parse(body);
  } catch (...) {
    error_out = xorstr("Invalid response from authentication server");
    return false;
  }

  if (status != 200) {
    error_out = xorstr("Subscription check failed");
    return false;
  }

  active_out = json.value(xorstr("active"), false);
  days_left_out = json.value(xorstr("daysLeft"), 0);
  return true;
}

inline int validate_session(const std::string &access_token) {
  if (access_token.empty())
    return 0;
  long status = 0;
  std::string body;
  if (!detail::http_get_json(base_url() + xorstr("/auth/validate-token"),
                             access_token, status, body))
    return -1;
  if (status == 200)
    return 1;
  if (status == 401)
    return 0;
  return -1;
}

inline bool fivem_set_logged(const std::string &access_token, bool logged) {
  if (access_token.empty())
    return false;
  long status = 0;
  std::string body;
  nlohmann::json payload;
  payload[xorstr("logged")] = logged;
  if (!detail::http_post_json_with_auth(base_url() +
                                            xorstr("/auth/fivem/set-logged"),
                                        access_token, payload, status, body))
    return false;
  return (status == 200);
}

struct FivemCommandItem {
  int id = 0;
  std::string type;
  nlohmann::json payload;
};

struct LiveConfigResult {
  bool ok = false;
  bool changed = false;
  int version = 0;
  std::string data;
};

inline LiveConfigResult live_config_get(const std::string &access_token) {
  LiveConfigResult out;
  if (access_token.empty())
    return out;
  long status = 0;
  std::string body;
  if (!detail::http_get_json(base_url() + xorstr("/auth/live-config"),
                             access_token, status, body) ||
      status != 200)
    return out;
  try {
    nlohmann::json j = nlohmann::json::parse(body);
    out.ok = true;
    out.changed = true;
    if (j.contains(xorstr("data")) && j[xorstr("data")].is_string())
      out.data = j[xorstr("data")].get<std::string>();
    out.version = j.value(xorstr("version"), 0);
  } catch (...) {
  }
  return out;
}

inline LiveConfigResult live_config_wait(const std::string &access_token,
                                         int since_version,
                                         int timeout_ms = 25000) {
  LiveConfigResult out;
  if (access_token.empty())
    return out;
  long status = 0;
  std::string body;
  std::string url = base_url() + xorstr("/auth/live-config/wait?since=") +
                    std::to_string(since_version) + xorstr("&timeout=") +
                    std::to_string(timeout_ms);
  if (!detail::http_get_json(url, access_token, status, body) || status != 200)
    return out;
  try {
    nlohmann::json j = nlohmann::json::parse(body);
    out.ok = true;
    out.changed = j.value(xorstr("changed"), false);
    out.version = j.value(xorstr("version"), since_version);
    if (out.changed && j.contains(xorstr("data")) &&
        j[xorstr("data")].is_string())
      out.data = j[xorstr("data")].get<std::string>();
  } catch (...) {
  }
  return out;
}

inline bool live_config_update(const std::string &access_token,
                               const std::string &data_json,
                               int *inout_version = nullptr) {
  if (access_token.empty() || data_json.empty())
    return false;
  long status = 0;
  std::string body;
  nlohmann::json payload;
  payload[xorstr("data")] = data_json;
  if (inout_version && *inout_version >= 0)
    payload[xorstr("expectedVersion")] = *inout_version;
  if (!detail::http_patch_json_with_auth(base_url() +
                                             xorstr("/auth/live-config"),
                                         access_token, payload, status, body))
    return false;
  if (status == 409 && inout_version) {
    try {
      nlohmann::json j = nlohmann::json::parse(body);
      *inout_version = j.value(xorstr("version"), *inout_version);
    } catch (...) {
    }
    return false;
  }
  if (status != 200)
    return false;
  if (inout_version) {
    try {
      nlohmann::json j = nlohmann::json::parse(body);
      *inout_version = j.value(xorstr("version"), *inout_version);
    } catch (...) {
    }
  }
  return true;
}

inline void report_security_violation(const std::string &access_token,
                                      const std::string &violation_type,
                                      const std::string &details) {
  long status = 0;
  std::string body;
  nlohmann::json payload{{xorstr("hwid"), Hwidgen::getHwid()},
                         {xorstr("deviceInfo"), detail::build_device_info()},
                         {xorstr("reason"), violation_type},
                         {xorstr("details"), details}};
  if (!access_token.empty())
    payload[xorstr("accessToken")] = access_token;
  detail::http_post_json_with_auth(base_url() + xorstr("/auth/security-report"),
                                   access_token, payload, status, body);
}

struct ConfigEntry {
  int id = 0;
  std::string name;
  std::string code;
};

inline std::vector<ConfigEntry> config_list(const std::string &access_token) {
  std::vector<ConfigEntry> out;
  if (access_token.empty())
    return out;
  long status = 0;
  std::string body;
  if (!detail::http_get_json(base_url() + xorstr("/configs"), access_token,
                             status, body) ||
      status != 200)
    return out;
  try {
    nlohmann::json j = nlohmann::json::parse(body);
    if (!j.contains(xorstr("configs")) || !j[xorstr("configs")].is_array())
      return out;
    for (const auto &c : j[xorstr("configs")]) {
      ConfigEntry e;
      e.id = c.value(xorstr("id"), 0);
      e.name = c.value(xorstr("name"), std::string(xorstr("")));
      e.code = c.value(xorstr("code"), std::string(xorstr("")));
      if (e.id)
        out.push_back(e);
    }
  } catch (...) {
  }
  return out;
}

inline bool config_get(const std::string &access_token, int id,
                       std::string &name_out, std::string &code_out,
                       std::string &data_out) {
  name_out.clear();
  code_out.clear();
  data_out.clear();
  if (access_token.empty() || id <= 0)
    return false;
  long status = 0;
  std::string body;
  if (!detail::http_get_json(base_url() + xorstr("/configs/") +
                                 std::to_string(id),
                             access_token, status, body) ||
      status != 200)
    return false;
  try {
    nlohmann::json j = nlohmann::json::parse(body);
    name_out = j.value(xorstr("name"), std::string(xorstr("")));
    code_out = j.value(xorstr("code"), std::string(xorstr("")));
    data_out = j.value(xorstr("data"), std::string(xorstr("")));
    return !data_out.empty();
  } catch (...) {
  }
  return false;
}

inline bool config_get_by_code(const std::string &code_8, std::string &name_out,
                               std::string &data_out) {
  name_out.clear();
  data_out.clear();
  if (code_8.size() != 8)
    return false;
  long status = 0;
  std::string body;
  if (!detail::http_get_json(base_url() + xorstr("/configs/by-code/") + code_8,
                             std::string(), status, body) ||
      status != 200)
    return false;
  try {
    nlohmann::json j = nlohmann::json::parse(body);
    name_out = j.value(xorstr("name"), std::string(xorstr("")));
    data_out = j.value(xorstr("data"), std::string(xorstr("")));
    return !data_out.empty();
  } catch (...) {
  }
  return false;
}

inline int config_create(const std::string &access_token,
                         const std::string &name, const std::string &data) {
  if (access_token.empty() || name.empty() || data.empty())
    return 0;
  long status = 0;
  std::string body;
  nlohmann::json payload{{xorstr("name"), name}, {xorstr("data"), data}};
  if (!detail::http_post_json_with_auth(base_url() + xorstr("/configs"),
                                        access_token, payload, status, body))
    return 0;
  if (status != 201)
    return 0;
  try {
    nlohmann::json j = nlohmann::json::parse(body);
    return j.value(xorstr("id"), 0);
  } catch (...) {
  }
  return 0;
}

inline bool config_delete(const std::string &access_token, int id) {
  if (access_token.empty() || id <= 0)
    return false;
  long status = 0;
  std::string body;
  if (!detail::http_delete_json(base_url() + xorstr("/configs/") +
                                    std::to_string(id),
                                access_token, status, body))
    return false;
  return (status == 200);
}

inline int config_import(const std::string &access_token,
                         const std::string &name, const std::string &code) {
  if (access_token.empty() || name.empty() || code.empty())
    return 0;
  long status = 0;
  std::string body;
  nlohmann::json payload{{xorstr("name"), name}, {xorstr("code"), code}};
  if (!detail::http_post_json_with_auth(base_url() + xorstr("/configs/import"),
                                        access_token, payload, status, body))
    return 0;
  if (status != 200 && status != 201)
    return 0;
  try {
    nlohmann::json j = nlohmann::json::parse(body);
    return j.value(xorstr("id"), 0);
  } catch (...) {
  }
  return 0;
}

inline void logout(const std::string &access_token,
                   const std::string &refresh_token) {
  std::string stored = Hwidgen::readTrustedDeviceToken();
  Hwidgen::writeTrustedDeviceToken(std::string());
  if (access_token.empty() && refresh_token.empty() && stored.empty()) return;
  long status = 0;
  std::string body;
  nlohmann::json payload;
  if (!refresh_token.empty()) payload[xorstr("refreshToken")] = refresh_token;
  if (!stored.empty()) payload[xorstr("trustedDeviceToken")] = stored;
  detail::http_post_json_with_auth(base_url() + xorstr("/auth/logout"),
                                   access_token, payload, status, body);
}

inline bool game_state_update(const std::string &access_token,
                              const nlohmann::json &players_array) {
  if (access_token.empty())
    return false;
  long status = 0;
  std::string body;
  nlohmann::json payload;
  payload[xorstr("players")] =
      players_array.is_array() ? players_array : nlohmann::json::array();
  if (!detail::http_post_json_with_auth(base_url() +
                                            xorstr("/auth/fivem/game-state"),
                                        access_token, payload, status, body))
    return false;
  return (status == 200);
}

}
