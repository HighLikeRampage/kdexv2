#include "Dumper.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <functional>
#include <string>
#include <thread>
#include <unordered_map>
#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>
#include <bcrypt.h>
#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "bcrypt.lib")

typedef struct _KDEX_MIB_TCPROW_OWNER_PID {
    DWORD dwState;
    DWORD dwLocalAddr;
    DWORD dwLocalPort;
    DWORD dwRemoteAddr;
    DWORD dwRemotePort;
    DWORD dwOwningPid;
} KDEX_MIB_TCPROW_OWNER_PID;

typedef struct _KDEX_MIB_TCPTABLE_OWNER_PID {
    DWORD dwNumEntries;
    KDEX_MIB_TCPROW_OWNER_PID table[1];
} KDEX_MIB_TCPTABLE_OWNER_PID;

#define KDEX_MIB_TCP_STATE_ESTAB 5
#define KDEX_TCP_TABLE_OWNER_PID_CONNECTIONS 4
#define KDEX_AF_INET 2

typedef DWORD(WINAPI *pfnGetExtendedTcpTable)(PVOID pTcpTable, PDWORD pdwSize,
                                              BOOL bOrder, ULONG ulAf,
                                              ULONG TableClass, ULONG Reserved);

#define CURL_STATICLIB
#include "../../../Security/Api/curl/curl.h"
#include "../../../Security/Api/json.hpp"
#include "../../../Security/xorstr.hpp"
#include "../../SDK/Memory.hpp"
#pragma comment(lib, "libcurl.lib")
#pragma comment(lib, "Crypt32.lib")
#pragma comment(lib, "Normaliz.lib")
#pragma comment(lib, "Wldap32.lib")

namespace Core::Features::Dumper {

ResourceDumper g_ResourceDumper;

namespace {

uint64_t NowMs() {
    return (uint64_t)std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

size_t WriteCurlBuf(void *contents, size_t size, size_t nmemb, void *userp) {
    auto *buf = reinterpret_cast<std::vector<uint8_t> *>(userp);
    size_t total = size * nmemb;
    auto *bytes = reinterpret_cast<uint8_t *>(contents);
    buf->insert(buf->end(), bytes, bytes + total);
    return total;
}

bool HttpFetch(const std::string &url, const std::string &postBody,
               std::vector<uint8_t> &out, long &httpStatus,
               int timeout_seconds = 30) {
    out.clear();
    httpStatus = 0;

    CURL *curl = curl_easy_init();
    if (!curl) return false;

    struct curl_slist *headers = nullptr;
    headers = curl_slist_append(headers, xorstr("User-Agent: CitizenFX/1"));
    if (!postBody.empty())
        headers = curl_slist_append(
            headers, xorstr("Content-Type: application/x-www-form-urlencoded"));

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCurlBuf);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &out);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, (long)timeout_seconds);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, xorstr(""));

    if (!postBody.empty()) {
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postBody.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)postBody.size());
    }

    CURLcode res = curl_easy_perform(curl);
    if (res == CURLE_OK)
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpStatus);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return res == CURLE_OK;
}

std::vector<uint8_t> B64Decode(const std::string &in) {
    static bool inited = false;
    static int tab[256];
    if (!inited) {
        for (int i = 0; i < 256; i++) tab[i] = -1;
        std::string alphabet = xorstr(
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/");
        for (int i = 0; i < 64; i++) tab[(uint8_t)alphabet[i]] = i;
        inited = true;
    }
    std::vector<uint8_t> out;
    int val = 0, valb = -8;
    for (uint8_t c : in) {
        if (c == '=' || c == '\r' || c == '\n' || c == ' ') continue;
        int v = tab[c];
        if (v < 0) continue;
        val = (val << 6) | v;
        valb += 6;
        if (valb >= 0) {
            out.push_back((uint8_t)((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    return out;
}

bool HmacSha256(const uint8_t *key, size_t keyLen, const uint8_t *data,
                size_t dataLen, uint8_t out[32]) {
    BCRYPT_ALG_HANDLE hAlg = nullptr;
    BCRYPT_HASH_HANDLE hHash = nullptr;
    NTSTATUS s = BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_SHA256_ALGORITHM,
                                             nullptr,
                                             BCRYPT_ALG_HANDLE_HMAC_FLAG);
    if (s < 0) return false;
    s = BCryptCreateHash(hAlg, &hHash, nullptr, 0, (PUCHAR)key, (ULONG)keyLen, 0);
    if (s < 0) { BCryptCloseAlgorithmProvider(hAlg, 0); return false; }
    s = BCryptHashData(hHash, (PUCHAR)data, (ULONG)dataLen, 0);
    if (s >= 0) s = BCryptFinishHash(hHash, out, 32, 0);
    BCryptDestroyHash(hHash);
    BCryptCloseAlgorithmProvider(hAlg, 0);
    return s >= 0;
}

static inline uint32_t Rotl32(uint32_t v, int n) {
    return (v << n) | (v >> (32 - n));
}

static void ChaCha20Block(const uint32_t key[8], uint64_t counter,
                          const uint32_t nonce[2], uint8_t out[64]) {
    uint32_t x[16];
    x[0] = 0x61707865u;
    x[1] = 0x3320646eu;
    x[2] = 0x79622d32u;
    x[3] = 0x6b206574u;
    for (int i = 0; i < 8; i++) x[4 + i] = key[i];
    x[12] = (uint32_t)(counter & 0xFFFFFFFFu);
    x[13] = (uint32_t)((counter >> 32) & 0xFFFFFFFFu);
    x[14] = nonce[0];
    x[15] = nonce[1];
    uint32_t s[16];
    memcpy(s, x, sizeof(x));
#define QR(a, b, c, d)                    \
    do {                                  \
        a += b; d = Rotl32(d ^ a, 16);    \
        c += d; b = Rotl32(b ^ c, 12);    \
        a += b; d = Rotl32(d ^ a, 8);     \
        c += d; b = Rotl32(b ^ c, 7);     \
    } while (0)
    for (int i = 0; i < 10; i++) {
        QR(x[0], x[4], x[8], x[12]);
        QR(x[1], x[5], x[9], x[13]);
        QR(x[2], x[6], x[10], x[14]);
        QR(x[3], x[7], x[11], x[15]);
        QR(x[0], x[5], x[10], x[15]);
        QR(x[1], x[6], x[11], x[12]);
        QR(x[2], x[7], x[8], x[13]);
        QR(x[3], x[4], x[9], x[14]);
    }
#undef QR
    for (int i = 0; i < 16; i++) x[i] += s[i];
    memcpy(out, x, 64);
}

static void ChaCha20Decrypt(const uint8_t key32[32], const uint8_t nonce8[8],
                            const uint8_t *in, size_t len, uint8_t *out) {
    uint32_t k[8];
    for (int i = 0; i < 8; i++) {
        k[i] = (uint32_t)key32[i * 4] | ((uint32_t)key32[i * 4 + 1] << 8) |
               ((uint32_t)key32[i * 4 + 2] << 16) |
               ((uint32_t)key32[i * 4 + 3] << 24);
    }
    uint32_t n[2];
    for (int i = 0; i < 2; i++) {
        n[i] = (uint32_t)nonce8[i * 4] | ((uint32_t)nonce8[i * 4 + 1] << 8) |
               ((uint32_t)nonce8[i * 4 + 2] << 16) |
               ((uint32_t)nonce8[i * 4 + 3] << 24);
    }
    uint64_t counter = 0;
    uint8_t block[64];
    for (size_t i = 0; i < len; i += 64) {
        ChaCha20Block(k, counter++, n, block);
        size_t chunk = std::min<size_t>(64, len - i);
        for (size_t j = 0; j < chunk; j++) out[i + j] = in[i + j] ^ block[j];
    }
}

struct ResourceUriKey {
    uint8_t key[32];
    uint8_t iv[8];
    bool ok = false;
};

ResourceUriKey ProcessResourceUri(const std::string &uriB64) {
    ResourceUriKey out;
    auto raw = B64Decode(uriB64);
    if (raw.size() < 19 + 8 + 2 + 32) return out;
    std::vector<uint8_t> remaining(raw.begin() + 19, raw.end());
    if (remaining.size() < 8 + 32 + 2) return out;

    for (size_t i = 0; i < 8; i++) out.iv[i] = remaining[remaining.size() - 8 + i];

    size_t keyLen = remaining.size() - 8;
    std::vector<uint8_t> xored(keyLen);
    for (size_t i = 0; i < keyLen; i++) xored[i] = remaining[i] ^ 0x69;
    if (xored.size() < 2 + 32) return out;
    xored.resize(xored.size() - 2);

    if (xored.size() < 32) return out;
    memcpy(out.key, xored.data(), 32);
    out.ok = true;
    return out;
}

bool CalculateChaChaKey(const uint8_t key32[32], const std::string &fileName,
                        uint8_t out32[32]) {
    return HmacSha256(key32, 32,
                      reinterpret_cast<const uint8_t *>(fileName.data()),
                      fileName.size(), out32);
}

#pragma pack(push, 1)
struct Rpf2Header {
    uint32_t magic;
    uint32_t tocSize;
    uint32_t numEntries;
    uint32_t unkFlag;
    uint32_t cryptoFlag;
};
struct Rpf2RawEntry {
    uint32_t nameOffset;
    uint32_t length;
    uint32_t dataOffsetPacked;
    uint32_t flags;
};
#pragma pack(pop)

struct Rpf2Entry {
    uint32_t nameOffset;
    uint32_t length;
    uint32_t dataOffset;
    bool isDirectory;
    uint32_t flags;
};

struct Rpf2Archive {
    std::vector<Rpf2Entry> entries;
    std::vector<uint8_t> nameTable;
    std::vector<uint8_t> data;

    std::string GetEntryName(const Rpf2Entry &e) const {
        size_t end = e.nameOffset;
        while (end < nameTable.size() && nameTable[end] != 0) end++;
        return std::string(reinterpret_cast<const char *>(nameTable.data()) + e.nameOffset,
                           end - e.nameOffset);
    }
};

bool ParseRpf2(const std::vector<uint8_t> &buf, Rpf2Archive &out) {
    if (buf.size() < sizeof(Rpf2Header) + 2048) return false;
    Rpf2Header hdr;
    memcpy(&hdr, buf.data(), sizeof(hdr));
    if (hdr.magic != 0x32465052u) return false;
    if (hdr.cryptoFlag != 0) return false;
    if (2048 + hdr.tocSize > buf.size()) return false;

    size_t entryTableSize = (size_t)hdr.numEntries * sizeof(Rpf2RawEntry);
    if (entryTableSize > hdr.tocSize) return false;

    const uint8_t *toc = buf.data() + 2048;
    out.entries.clear();
    out.entries.reserve(hdr.numEntries);
    for (uint32_t i = 0; i < hdr.numEntries; i++) {
        Rpf2RawEntry raw;
        memcpy(&raw, toc + i * sizeof(Rpf2RawEntry), sizeof(raw));
        Rpf2Entry e;
        e.nameOffset = raw.nameOffset;
        e.length = raw.length;
        e.dataOffset = raw.dataOffsetPacked & 0x7FFFFFFFu;
        e.isDirectory = (raw.dataOffsetPacked >> 31) & 1u;
        e.flags = raw.flags;
        out.entries.push_back(e);
    }
    out.nameTable.assign(toc + entryTableSize, toc + hdr.tocSize);
    out.data = buf;
    return true;
}

std::shared_ptr<VfsNode> BuildVfsFromRpf(const Rpf2Archive &rpf,
                                         const std::string &resourceName) {
    auto rootNode = std::make_shared<VfsNode>();
    rootNode->name = resourceName;
    rootNode->fullPath = xorstr("/") + resourceName;
    rootNode->isDirectory = true;
    if (rpf.entries.empty()) return rootNode;

    std::function<void(const Rpf2Entry &, std::shared_ptr<VfsNode>)> walk =
        [&](const Rpf2Entry &e, std::shared_ptr<VfsNode> parent) {
            if (!e.isDirectory) return;
            size_t start = e.dataOffset;
            size_t end = start + e.length;
            if (start >= rpf.entries.size() || end > rpf.entries.size()) return;

            for (size_t i = start; i < end; i++) {
                const Rpf2Entry &child = rpf.entries[i];
                std::string name = rpf.GetEntryName(child);
                if (name.empty()) continue;
                auto node = std::make_shared<VfsNode>();
                node->name = name;
                node->fullPath = parent->fullPath + xorstr("/") + name;
                node->isDirectory = child.isDirectory ? true : false;
                if (!child.isDirectory) {
                    size_t off = child.dataOffset;
                    size_t len = child.length;
                    if (off + len <= rpf.data.size()) {
                        node->content.assign(rpf.data.begin() + off,
                                             rpf.data.begin() + off + len);
                        node->size = len;
                    }
                } else {
                    walk(child, node);
                }
                parent->children.push_back(std::move(node));
            }
        };

    walk(rpf.entries[0], rootNode);
    std::sort(rootNode->children.begin(), rootNode->children.end(),
              [](const std::shared_ptr<VfsNode> &a,
                 const std::shared_ptr<VfsNode> &b) {
                  if (a->isDirectory != b->isDirectory) return a->isDirectory > b->isDirectory;
                  return a->name < b->name;
              });
    return rootNode;
}

void SortTreeRecursive(std::shared_ptr<VfsNode> node) {
    if (!node || !node->isDirectory) return;
    std::sort(node->children.begin(), node->children.end(),
              [](const std::shared_ptr<VfsNode> &a,
                 const std::shared_ptr<VfsNode> &b) {
                  if (a->isDirectory != b->isDirectory) return a->isDirectory > b->isDirectory;
                  return a->name < b->name;
              });
    for (auto &c : node->children) SortTreeRecursive(c);
}

std::shared_ptr<VfsNode> InsertStreamFile(std::shared_ptr<VfsNode> resourceRoot,
                                          const std::string &fileName,
                                          std::vector<uint8_t> content) {
    const std::string kStream = xorstr("stream");
    auto streamDir = std::shared_ptr<VfsNode>();
    for (auto &c : resourceRoot->children) {
        if (c->isDirectory && c->name == kStream) { streamDir = c; break; }
    }
    if (!streamDir) {
        streamDir = std::make_shared<VfsNode>();
        streamDir->name = kStream;
        streamDir->isDirectory = true;
        streamDir->fullPath = resourceRoot->fullPath + xorstr("/") + kStream;
        resourceRoot->children.push_back(streamDir);
    }
    auto file = std::make_shared<VfsNode>();
    file->name = fileName;
    file->fullPath = streamDir->fullPath + xorstr("/") + fileName;
    file->isDirectory = false;
    file->size = content.size();
    file->content = std::move(content);
    streamDir->children.push_back(file);
    return file;
}

std::string NormalizeServerUrl(std::string url) {
    while (!url.empty() && (url.back() == '/' || url.back() == ' '))
        url.pop_back();
    std::string http = xorstr("http://");
    std::string https = xorstr("https://");
    if (url.rfind(http, 0) != 0 && url.rfind(https, 0) != 0)
        url = http + url;
    return url;
}

}

namespace {

DWORD FindFivemPid() {
    if (Core::Mem.ProcId) return Core::Mem.ProcId;
    static const char *candidates[] = {
        "FiveM_b3751_GTAProcess.exe",
        "FiveM_GTAProcess.exe",
        "FiveM.exe",
    };
    for (auto n : candidates) {
        DWORD pid = Core::Mem.GetPidByName(n);
        if (pid) return pid;
    }
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return 0;
    PROCESSENTRY32W pe = { sizeof(pe) };
    std::wstring needle = xorstr(L"fivem");
    DWORD found = 0;
    if (Process32FirstW(snap, &pe)) {
        do {
            std::wstring name = pe.szExeFile;
            for (auto &c : name) c = towlower(c);
            if (name.find(needle) != std::wstring::npos) {
                found = pe.th32ProcessID;
                break;
            }
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    return found;
}

std::string ReadServerIpFromFivem(DWORD pid) {
    if (!pid) return {};

    uintptr_t modSize = 0;
    uintptr_t modBase = Core::Mem.GetModuleBaseAddr(pid, xorstr("net.dll"), &modSize);
    if (!modBase || !modSize) return {};

    auto sig = Core::Mem.Pattern2Vector(
        xorstr("48 8D 3D ? ? ? ? 4C 8B C7 E8 ? ? ? ? 0F 57 C0"));
    uintptr_t hit = Core::Mem.FindSignatureBypass(sig, modBase, modSize);
    if (!hit) return {};

    int32_t disp = Core::Mem.Read<int32_t>(hit + 3);
    uintptr_t bufAddr = hit + 7 + (int64_t)disp;

    char tmp[17] = {0};
    if (!Core::Mem.ReadRaw(bufAddr, tmp, 16)) return {};
    tmp[16] = 0;
    for (int i = 0; i < 16; i++) {
        char c = tmp[i];
        if (c == 0) break;
        if (c < 32 || c > 126) { tmp[i] = 0; break; }
    }
    return std::string(tmp);
}

DWORD FindPortForIp(const std::string &ip, DWORD targetPid) {
    HMODULE hIpHlp = LoadLibraryA(xorstr("iphlpapi.dll"));
    if (!hIpHlp) return 0;
    auto pGetExtendedTcpTable = (pfnGetExtendedTcpTable)GetProcAddress(
        hIpHlp, xorstr("GetExtendedTcpTable"));
    if (!pGetExtendedTcpTable) { FreeLibrary(hIpHlp); return 0; }

    ULONG size = 0;
    DWORD probe = pGetExtendedTcpTable(nullptr, &size, FALSE, KDEX_AF_INET,
                                       KDEX_TCP_TABLE_OWNER_PID_CONNECTIONS, 0);
    if (probe != ERROR_INSUFFICIENT_BUFFER) { FreeLibrary(hIpHlp); return 0; }
    std::vector<uint8_t> tblBuf(size);
    auto *table = reinterpret_cast<KDEX_MIB_TCPTABLE_OWNER_PID *>(tblBuf.data());
    DWORD rc = pGetExtendedTcpTable(table, &size, FALSE, KDEX_AF_INET,
                                    KDEX_TCP_TABLE_OWNER_PID_CONNECTIONS, 0);
    FreeLibrary(hIpHlp);
    if (rc != NO_ERROR) return 0;

    for (DWORD i = 0; i < table->dwNumEntries; i++) {
        const auto &row = table->table[i];
        if (targetPid && row.dwOwningPid != targetPid) continue;
        if (row.dwState != KDEX_MIB_TCP_STATE_ESTAB) continue;
        DWORD raw = row.dwRemoteAddr;
        unsigned a = raw & 0xFF;
        unsigned b = (raw >> 8) & 0xFF;
        unsigned c = (raw >> 16) & 0xFF;
        unsigned d = (raw >> 24) & 0xFF;
        char asStr[32];
        snprintf(asStr, sizeof(asStr), xorstr("%u.%u.%u.%u"), a, b, c, d);
        if (ip == asStr) {
            DWORD port = ((row.dwRemotePort & 0xFF) << 8) |
                         ((row.dwRemotePort >> 8) & 0xFF);
            return port;
        }
    }
    return 0;
}

}

std::string ResourceDumper::DetectServerUrl() {
    DWORD pid = FindFivemPid();
    std::string ip = ReadServerIpFromFivem(pid);
    if (ip.empty()) return {};

    DWORD port = FindPortForIp(ip, pid);
    if (port == 0) port = FindPortForIp(ip, 0);
    if (port == 0) port = 30120;

    char url[96];
    snprintf(url, sizeof(url), xorstr("http://%s:%lu"), ip.c_str(), port);
    return url;
}

bool ResourceDumper::LooksTextual(const std::vector<uint8_t> &content) {
    if (content.empty()) return true;
    size_t sample = std::min<size_t>(4096, content.size());
    size_t bad = 0;
    for (size_t i = 0; i < sample; i++) {
        uint8_t b = content[i];
        if (b == 0) return false;
        if (b < 9 || (b > 13 && b < 32) || b == 127) bad++;
    }
    return bad * 20 < sample;
}

std::string ResourceDumper::GuessLanguage(const std::string &fileName) {
    auto pos = fileName.find_last_of('.');
    if (pos == std::string::npos) return xorstr("text");
    std::string ext = fileName.substr(pos + 1);
    for (auto &c : ext) c = (char)tolower(c);
    if (ext == xorstr("lua")) return xorstr("lua");
    if (ext == xorstr("js") || ext == xorstr("mjs") || ext == xorstr("ts")) return xorstr("js");
    if (ext == xorstr("json")) return xorstr("json");
    if (ext == xorstr("xml") || ext == xorstr("meta") || ext == xorstr("ymt") || ext == xorstr("ymap")) return xorstr("xml");
    if (ext == xorstr("cfg") || ext == xorstr("conf") || ext == xorstr("ini")) return xorstr("ini");
    if (ext == xorstr("html") || ext == xorstr("htm")) return xorstr("html");
    if (ext == xorstr("css")) return xorstr("css");
    if (ext == xorstr("md")) return xorstr("markdown");
    if (ext == xorstr("cs")) return xorstr("csharp");
    if (ext == xorstr("py")) return xorstr("python");
    if (ext == xorstr("sh")) return xorstr("sh");
    return xorstr("text");
}

void ResourceDumper::Reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    stop_.store(true);
    root_ = std::make_shared<VfsNode>();
    root_->name = "";
    root_->fullPath = "";
    root_->isDirectory = true;
    status_ = DumperStatus{};
}

void ResourceDumper::Cancel() { stop_.store(true); }

DumperStatus ResourceDumper::GetStatus() {
    std::lock_guard<std::mutex> lock(mutex_);
    return status_;
}

std::shared_ptr<VfsNode> ResourceDumper::GetRoot() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!root_) {
        root_ = std::make_shared<VfsNode>();
        root_->isDirectory = true;
    }
    return root_;
}

namespace {

static uint32_t Crc32Table[256];
static bool Crc32Inited = false;
static void InitCrc32() {
    if (Crc32Inited) return;
    for (uint32_t i = 0; i < 256; i++) {
        uint32_t c = i;
        for (int k = 0; k < 8; k++) c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
        Crc32Table[i] = c;
    }
    Crc32Inited = true;
}
static uint32_t Crc32(const uint8_t *data, size_t len) {
    InitCrc32();
    uint32_t c = 0xFFFFFFFFu;
    for (size_t i = 0; i < len; i++) c = Crc32Table[(c ^ data[i]) & 0xFF] ^ (c >> 8);
    return c ^ 0xFFFFFFFFu;
}

static void W16(std::vector<uint8_t> &b, uint16_t v) {
    b.push_back((uint8_t)(v & 0xFF));
    b.push_back((uint8_t)((v >> 8) & 0xFF));
}
static void W32(std::vector<uint8_t> &b, uint32_t v) {
    b.push_back((uint8_t)(v & 0xFF));
    b.push_back((uint8_t)((v >> 8) & 0xFF));
    b.push_back((uint8_t)((v >> 16) & 0xFF));
    b.push_back((uint8_t)((v >> 24) & 0xFF));
}

struct ZipEntry {
    std::string name;
    uint32_t crc;
    uint32_t size;
    uint32_t offset;
};

static void CollectFiles(std::shared_ptr<VfsNode> node, const std::string &prefix,
                         std::vector<std::pair<std::string, std::vector<uint8_t> *>> &out) {
    if (!node) return;
    if (node->isDirectory) {
        std::string p = prefix.empty() ? node->name : (prefix + "/" + node->name);
        if (node->name.empty()) p = prefix;
        for (auto &c : node->children) CollectFiles(c, p, out);
    } else {
        std::string p = prefix.empty() ? node->name : (prefix + "/" + node->name);
        while (!p.empty() && p.front() == '/') p.erase(p.begin());
        out.push_back({p, &node->content});
    }
}

}

bool ResourceDumper::ExportZip(std::vector<uint8_t> &out) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!root_ || root_->children.empty()) return false;

    std::vector<std::pair<std::string, std::vector<uint8_t> *>> files;
    CollectFiles(root_, "", files);
    if (files.empty()) return false;

    out.clear();
    std::vector<ZipEntry> entries;
    entries.reserve(files.size());

    for (auto &f : files) {
        ZipEntry e;
        e.name = f.first;
        e.size = (uint32_t)f.second->size();
        e.crc = f.second->empty() ? 0
                                  : Crc32(f.second->data(), f.second->size());
        e.offset = (uint32_t)out.size();

        W32(out, 0x04034b50u);
        W16(out, 20);
        W16(out, 0);
        W16(out, 0);
        W16(out, 0);
        W16(out, 0);
        W32(out, e.crc);
        W32(out, e.size);
        W32(out, e.size);
        W16(out, (uint16_t)e.name.size());
        W16(out, 0);
        out.insert(out.end(), e.name.begin(), e.name.end());
        if (!f.second->empty())
            out.insert(out.end(), f.second->begin(), f.second->end());

        entries.push_back(e);
    }

    uint32_t cdOffset = (uint32_t)out.size();
    for (auto &e : entries) {
        W32(out, 0x02014b50u);
        W16(out, 20);
        W16(out, 20);
        W16(out, 0);
        W16(out, 0);
        W16(out, 0);
        W16(out, 0);
        W32(out, e.crc);
        W32(out, e.size);
        W32(out, e.size);
        W16(out, (uint16_t)e.name.size());
        W16(out, 0);
        W16(out, 0);
        W16(out, 0);
        W16(out, 0);
        W32(out, 0);
        W32(out, e.offset);
        out.insert(out.end(), e.name.begin(), e.name.end());
    }
    uint32_t cdSize = (uint32_t)(out.size() - cdOffset);

    W32(out, 0x06054b50u);
    W16(out, 0);
    W16(out, 0);
    W16(out, (uint16_t)entries.size());
    W16(out, (uint16_t)entries.size());
    W32(out, cdSize);
    W32(out, cdOffset);
    W16(out, 0);
    return true;
}

bool ResourceDumper::GetFileContent(const std::string &fullPath,
                                    std::vector<uint8_t> &out) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!root_) return false;
    std::function<std::shared_ptr<VfsNode>(std::shared_ptr<VfsNode>)> walk =
        [&](std::shared_ptr<VfsNode> n) -> std::shared_ptr<VfsNode> {
        if (!n) return nullptr;
        if (n->fullPath == fullPath && !n->isDirectory) return n;
        for (auto &c : n->children) {
            auto r = walk(c);
            if (r) return r;
        }
        return nullptr;
    };
    auto file = walk(root_);
    if (!file) return false;
    out = file->content;
    return true;
}

void ResourceDumper::StartWatcher() {
    bool expected = false;
    if (!watcher_started_.compare_exchange_strong(expected, true)) return;
    watcher_stop_.store(false);
    std::thread([this]() { RunWatcher(); }).detach();
}

void ResourceDumper::StopWatcher() {
    watcher_stop_.store(true);
    watcher_started_.store(false);
}

void ResourceDumper::RunWatcher() {
    while (!watcher_stop_.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
        if (watcher_stop_.load()) break;
        if (running_.load()) continue;

        std::string url = DetectServerUrl();
        if (url.empty()) continue;

        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (url == lastDumpedServer_ &&
                (status_.state == DumpState::Done ||
                 status_.state == DumpState::Running ||
                 status_.state == DumpState::FetchingConfig))
                continue;
            lastDumpedServer_ = url;
        }

        StartDumpAsync(url);
    }
}

void ResourceDumper::StartDumpAsync(std::string serverUrl) {
    if (running_.load()) return;

    stop_.store(false);
    running_.store(true);

    {
        std::lock_guard<std::mutex> lock(mutex_);
        root_ = std::make_shared<VfsNode>();
        root_->name = "";
        root_->fullPath = "";
        root_->isDirectory = true;
        status_ = DumperStatus{};
        status_.state = DumpState::Detecting;
        status_.serverUrl = serverUrl;
        status_.startedAt = NowMs();
    }

    std::thread([this, serverUrl]() { RunAsync(serverUrl); }).detach();
}

void ResourceDumper::RunAsync(std::string serverUrl) {
    struct Guard {
        std::atomic<bool> &r;
        ~Guard() { r.store(false); }
    } guard{running_};

    auto setState = [&](DumpState s) {
        std::lock_guard<std::mutex> lock(mutex_);
        status_.state = s;
    };
    auto setError = [&](const std::string &msg) {
        std::lock_guard<std::mutex> lock(mutex_);
        status_.state = DumpState::Failed;
        status_.lastError = msg;
        status_.finishedAt = NowMs();
    };

    serverUrl = NormalizeServerUrl(serverUrl);
    if (serverUrl.empty()) { setError(xorstr("empty server url")); return; }

    setState(DumpState::FetchingConfig);

    std::vector<uint8_t> respBuf;
    long httpStatus = 0;
    if (!HttpFetch(serverUrl + xorstr("/client"),
                   xorstr("method=getConfiguration"), respBuf, httpStatus) ||
        httpStatus != 200) {
        setError(xorstr("fetching /client failed (status ") +
                 std::to_string(httpStatus) + xorstr(")"));
        return;
    }

    nlohmann::json cfg;
    try {
        cfg = nlohmann::json::parse(respBuf.begin(), respBuf.end());
    } catch (...) {
        setError(xorstr("invalid /client json"));
        return;
    }

    if (!cfg.contains(xorstr("resources")) ||
        !cfg[xorstr("resources")].is_array()) {
        setError(xorstr("no resources in /client"));
        return;
    }
    auto &resources = cfg[xorstr("resources")];

    {
        std::lock_guard<std::mutex> lock(mutex_);
        status_.totalResources = (int)resources.size();
        status_.state = DumpState::Running;
    }

    const std::string kResourceRpf = xorstr("resource.rpf");
    const std::string kUriTag = xorstr("v3#");

    for (const auto &resource : resources) {
        if (stop_.load()) break;
        std::string name = resource.value(xorstr("name"), std::string());
        std::string uri = resource.value(xorstr("uri"), std::string());
        if (name.empty() || uri.empty()) continue;

        {
            std::lock_guard<std::mutex> lock(mutex_);
            status_.currentResource = name;
        }

        std::string uriB64;
        auto tagPos = uri.find(kUriTag);
        if (tagPos != std::string::npos) uriB64 = uri.substr(tagPos + kUriTag.size());
        else uriB64 = uri;

        auto uriKey = ProcessResourceUri(uriB64);
        if (!uriKey.ok) {
            std::lock_guard<std::mutex> lock(mutex_);
            status_.failedResources++;
            status_.doneResources++;
            continue;
        }

        std::string fileServer;
        if (resource.contains(xorstr("fileServer")) &&
            resource[xorstr("fileServer")].is_string())
            fileServer = resource[xorstr("fileServer")].get<std::string>();

        std::string hashRpf;
        if (resource.contains(xorstr("files")) &&
            resource[xorstr("files")].contains(kResourceRpf))
            hashRpf = resource[xorstr("files")][kResourceRpf].get<std::string>();

        std::string rpfUrl;
        if (!fileServer.empty())
            rpfUrl = fileServer + xorstr("/") + name + xorstr("/resource.rpf?hash=") + hashRpf;
        else
            rpfUrl = serverUrl + xorstr("/files/") + name + xorstr("/resource.rpf?hash=") + hashRpf;

        std::vector<uint8_t> encRpf;
        if (!HttpFetch(rpfUrl, {}, encRpf, httpStatus, 120) || httpStatus != 200 ||
            encRpf.empty()) {
            std::lock_guard<std::mutex> lock(mutex_);
            status_.failedResources++;
            status_.doneResources++;
            continue;
        }

        uint8_t chachaKey[32];
        if (!CalculateChaChaKey(uriKey.key, kResourceRpf, chachaKey)) {
            std::lock_guard<std::mutex> lock(mutex_);
            status_.failedResources++;
            status_.doneResources++;
            continue;
        }

        std::vector<uint8_t> plainRpf(encRpf.size());
        ChaCha20Decrypt(chachaKey, uriKey.iv, encRpf.data(), encRpf.size(),
                        plainRpf.data());

        Rpf2Archive archive;
        if (!ParseRpf2(plainRpf, archive)) {
            std::lock_guard<std::mutex> lock(mutex_);
            status_.failedResources++;
            status_.doneResources++;
            continue;
        }

        auto resourceRoot = BuildVfsFromRpf(archive, name);

        if (resource.contains(xorstr("streamFiles")) &&
            resource[xorstr("streamFiles")].is_object()) {
            for (auto it = resource[xorstr("streamFiles")].begin();
                 it != resource[xorstr("streamFiles")].end(); ++it) {
                if (stop_.load()) break;
                const std::string &fileName = it.key();
                std::string streamHash;
                if (it.value().contains(xorstr("hash")) &&
                    it.value()[xorstr("hash")].is_string())
                    streamHash = it.value()[xorstr("hash")].get<std::string>();
                std::string streamUrl;
                if (!fileServer.empty())
                    streamUrl = fileServer + xorstr("/") + name + xorstr("/") + fileName +
                                xorstr("?hash=") + streamHash;
                else
                    streamUrl = serverUrl + xorstr("/files/") + name + xorstr("/") + fileName +
                                xorstr("?hash=") + streamHash;

                std::vector<uint8_t> encStream;
                if (!HttpFetch(streamUrl, {}, encStream, httpStatus, 120) ||
                    httpStatus != 200 || encStream.empty())
                    continue;

                uint8_t streamKey[32];
                if (!CalculateChaChaKey(uriKey.key, fileName, streamKey)) continue;

                std::vector<uint8_t> plainStream(encStream.size());
                ChaCha20Decrypt(streamKey, uriKey.iv, encStream.data(),
                                encStream.size(), plainStream.data());
                InsertStreamFile(resourceRoot, fileName, std::move(plainStream));
            }
        }

        SortTreeRecursive(resourceRoot);

        {
            std::lock_guard<std::mutex> lock(mutex_);
            root_->children.push_back(resourceRoot);
            std::sort(root_->children.begin(), root_->children.end(),
                      [](const std::shared_ptr<VfsNode> &a,
                         const std::shared_ptr<VfsNode> &b) {
                          return a->name < b->name;
                      });
            status_.doneResources++;
        }
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        status_.state = stop_.load() ? DumpState::Idle : DumpState::Done;
        status_.finishedAt = NowMs();
        status_.currentResource.clear();
    }
}

}
