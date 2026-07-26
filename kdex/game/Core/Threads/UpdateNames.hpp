#pragma once
#include <Security/Api/json.hpp>
#include <Includes/Includes.hpp>
#include <Includes/Utils.hpp>
#include <Core/Offsets.hpp>
#include <Core/SDK/SDK.hpp>

#include <fstream>
#include <string>
#include <cctype>

#define CURL_STATICLIB
#include <Security/Api/curl/curl.h>
#pragma comment( lib, "ws2_32.lib" )
#pragma comment( lib, "Normaliz.lib" )
#pragma comment( lib, "Crypt32.lib" )
#pragma comment( lib, "Wldap32.lib" )
#pragma comment( lib, "libcurl.lib" )

using json = nlohmann::json;

namespace Core
{
	namespace Threads
	{
		class cUpdateNames
		{
		private:
			std::string ServerIp;
			std::string DirFiveM;
			std::string RedirectUrl;
			std::string CachedServerToken;
			std::mutex NamesMutex;
		public:
			std::unordered_map<int, Core::SDK::Game::NetworkInfo> NetworkMap;
			bool TryGetNetworkInfo(int playerId, Core::SDK::Game::NetworkInfo& out) {
				std::lock_guard<std::mutex> lock(NamesMutex);
				auto it = NetworkMap.find(playerId);
				if (it == NetworkMap.end())
					return false;
				out = it->second;
				return true;
			}
		private:
			static size_t WriteCallBack(void* contents, size_t size, size_t nmemb, void* userp)
			{
				((std::string*)userp)->append((char*)contents, size * nmemb);
				return size * nmemb;
			}

			std::string ExtractIp(const std::string& line)
			{
				std::string ip;

				size_t fist_serv = line.rfind(xorstr("last_server_url"));
				if (fist_serv != std::string::npos)
				{
					size_t start = line.find(xorstr("last_server"), fist_serv + 16 - 1);
					if (start != std::string::npos)
					{
						size_t last_serv = line.find(xorstr(":"), start);
						if (last_serv != std::string::npos)
						{
							size_t ip_start = start + 12 - 1;
							size_t ip_end = line.find(xorstr(":"), ip_start) + 6;
							if (ip_end != std::string::npos && ip_end > ip_start)
							{
								ip = line.substr(ip_start, ip_end - ip_start);
							}
						}
					}
				}

				return ip;
			}

			std::string TryExtractTokenFromRedirect() {
				if (ServerIp.empty()) return "";
				std::string ReqUrl = std::string(xorstr("http://")) + ServerIp;
				std::string ResponseStr;
				CURL* hnd = curl_easy_init();
				if (!hnd) return "";
				curl_easy_setopt(hnd, CURLOPT_URL, ReqUrl.c_str());
				curl_easy_setopt(hnd, CURLOPT_WRITEFUNCTION, WriteCallBack);
				curl_easy_setopt(hnd, CURLOPT_WRITEDATA, &ResponseStr);
				curl_easy_setopt(hnd, CURLOPT_FOLLOWLOCATION, 1L);
				curl_easy_setopt(hnd, CURLOPT_TIMEOUT, 5L);
				CURLcode res = curl_easy_perform(hnd);
				if (res == CURLE_OK) {
					char* FinalUrl = nullptr;
					curl_easy_getinfo(hnd, CURLINFO_EFFECTIVE_URL, &FinalUrl);
					if (FinalUrl) RedirectUrl = std::string(FinalUrl);
				}
				curl_easy_cleanup(hnd);
				if (RedirectUrl.find(xorstr("cfx.re/join/")) != std::string::npos || RedirectUrl.find(xorstr("join/")) != std::string::npos) {
					size_t joinPos = RedirectUrl.find(xorstr("join/"));
					if (joinPos != std::string::npos) {
						std::string Token = RedirectUrl.substr(joinPos + 5);
						size_t q = Token.find('?');
						if (q != std::string::npos) Token = Token.substr(0, q);
						size_t end = Token.find('/');
						if (end != std::string::npos) Token = Token.substr(0, end);
						if (!Token.empty()) return Token;
					}
				}
				size_t pos = RedirectUrl.find_last_of('/');
				if (pos != std::string::npos && pos + 1 < RedirectUrl.size()) {
					std::string Token = RedirectUrl.substr(pos + 1);
					size_t q = Token.find('?');
					if (q != std::string::npos) Token = Token.substr(0, q);
					if (!Token.empty()) return Token;
				}
				return "";
			}

			std::string TryReadTokenFromFile(const std::string& path) {
				std::ifstream f(path, std::ios::binary);
				if (!f) return "";
				std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
				f.close();
				size_t join = content.find(xorstr("join/"));
				if (join != std::string::npos && join + 5 < content.size()) {
					size_t end = join + 5;
					while (end < content.size() && (std::isalnum((unsigned char)content[end]) || content[end] == '-' || content[end] == '_')) end++;
					std::string tok = content.substr(join + 5, end - (join + 5));
					if (tok.size() >= 4) return tok;
				}
				return "";
			}

			std::string GetServerToken() {
				if (!CachedServerToken.empty())
					return CachedServerToken;

				if (DirFiveM.empty())
				{
					char value[255];
					DWORD BufferSize = 8192;
					auto GetDirFiveM = RegGetValueA(HKEY_CURRENT_USER, xorstr("Software\\CitizenFX\\FiveM"), xorstr("Last Run Location"), RRF_RT_REG_SZ, NULL, (PVOID)&value, &BufferSize);
					if (GetDirFiveM != ERROR_SUCCESS)
						return xorstr("");
					DirFiveM = (std::string)value;
					if (!DirFiveM.empty() && DirFiveM.back() != '\\') DirFiveM += '\\';
				}

				std::string paths[] = {
					DirFiveM + xorstr("data\\cache\\crashometry"),
					DirFiveM + xorstr("FiveM.app\\data\\cache\\crashometry"),
					std::string()
				};
				char appData[512] = { 0 };
				if (GetEnvironmentVariableA(xorstr("LOCALAPPDATA"), appData, sizeof(appData)) > 0) {
					paths[2] = std::string(appData) + xorstr("\\FiveM\\FiveM.app\\data\\cache\\crashometry");
				}

				for (const std::string& CrashoMetryDir : paths) {
					if (CrashoMetryDir.empty()) continue;
					std::ifstream File(CrashoMetryDir, std::ios::binary);
					if (!File) continue;

					std::string line;
					while (std::getline(File, line)) {
						size_t LastServer = line.find(xorstr("last_server"));
						if (LastServer != std::string::npos) {
							ServerIp = this->ExtractIp(line);
							if (!ServerIp.empty()) break;
						}
					}
					File.close();
					if (!ServerIp.empty()) break;

					File.open(CrashoMetryDir, std::ios::binary);
					if (File) {
						std::string content((std::istreambuf_iterator<char>(File)), std::istreambuf_iterator<char>());
						File.close();
						ServerIp = this->ExtractIp(content);
					}
					if (!ServerIp.empty()) break;
				}

				for (const std::string& p : paths) {
					if (p.empty()) continue;
					std::string tok = TryReadTokenFromFile(p);
					if (!tok.empty()) {
						CachedServerToken = tok;
						return CachedServerToken;
					}
				}

				if (ServerIp.empty() && !g_Variables.ServerIp.empty())
					ServerIp = g_Variables.ServerIp;
				if (ServerIp.empty())
					return xorstr("");

				g_Variables.ServerIp = ServerIp;

				std::string Token = TryExtractTokenFromRedirect();
				if (!Token.empty()) {
					CachedServerToken = Token;
					return CachedServerToken;
				}
				return xorstr("");
			}

			nlohmann::json FetchCfxRayData(const std::string& token) {
				std::string ApiUrl = std::string(xorstr("https://frontend.cfx-services.net/api/servers/single/")) + token;

				std::string ResponseStr;
				CURL* hnd = curl_easy_init();
				if (!hnd) return nullptr;

				struct curl_slist* headers = NULL;
				headers = curl_slist_append(headers, xorstr("User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/150.0.0.0 Safari/537.36"));
				headers = curl_slist_append(headers, xorstr("Accept: */*"));

				curl_easy_setopt(hnd, CURLOPT_CUSTOMREQUEST, xorstr("GET"));
				curl_easy_setopt(hnd, CURLOPT_URL, ApiUrl.c_str());
				curl_easy_setopt(hnd, CURLOPT_TIMEOUT, 5L);
				curl_easy_setopt(hnd, CURLOPT_CONNECTTIMEOUT, 3L);
				curl_easy_setopt(hnd, CURLOPT_HTTPHEADER, headers);
				curl_easy_setopt(hnd, CURLOPT_WRITEFUNCTION, WriteCallBack);
				curl_easy_setopt(hnd, CURLOPT_WRITEDATA, &ResponseStr);

				CURLcode res = curl_easy_perform(hnd);
				curl_easy_cleanup(hnd);
				curl_slist_free_all(headers);

				if (res != CURLE_OK || ResponseStr.empty())
					return nullptr;

				try {
					return json::parse(ResponseStr);
				}
				catch (...) {
					return nullptr;
				}
			}

			void UpdatePlayerNames(const nlohmann::json& cfxData) {
				if (cfxData.is_null() || !cfxData.is_object())
					return;

				nlohmann::json playersList;

				if (cfxData.contains(xorstr("Data")) && cfxData[xorstr("Data")].is_object()) {
					auto& data = cfxData[xorstr("Data")];
					if (data.contains(xorstr("players")) && data[xorstr("players")].is_array())
						playersList = data[xorstr("players")];
				}

				if (!playersList.is_array())
					return;

				if (playersList.empty()) {
					std::lock_guard<std::mutex> lock(NamesMutex);
					NetworkMap.clear();
					return;
				}

				std::unordered_map<int, Core::SDK::Game::NetworkInfo> newMap;
				newMap.reserve(playersList.size());

				for (const auto& Player : playersList) {
					if (!Player.is_object())
						continue;
					if (!Player.contains(xorstr("id")) || !Player.contains(xorstr("name")))
						continue;

					int PlayerId = 0;
					if (Player[xorstr("id")].is_number_integer())
						PlayerId = Player[xorstr("id")].get<int>();
					else if (Player[xorstr("id")].is_string())
						PlayerId = std::atoi(Player[xorstr("id")].get<std::string>().c_str());
					else
						continue;

					std::string PlayerName = Player[xorstr("name")].is_string() ? Player[xorstr("name")].get<std::string>() : std::string();

					int PlayerPing = 0;
					if (Player.contains(xorstr("ping")) && Player[xorstr("ping")].is_number_integer())
						PlayerPing = Player[xorstr("ping")].get<int>();

					newMap[PlayerId] = { PlayerName, "", "", PlayerPing };
				}

				{
					std::lock_guard<std::mutex> lock(NamesMutex);
					NetworkMap.swap(newMap);
				}
			}

		public:
			void Update()
			{
				std::string token;

				while (!g_Variables.g_Unload)
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(500));
					token = GetServerToken();
					if (!token.empty())
						break;
				}

				if (g_Variables.g_Unload)
					return;

				std::fprintf(stderr, xorstr("[Names] token=%s\n"), token.c_str());

				while (!g_Variables.g_Unload)
				{
					try {
						nlohmann::json cfxData = FetchCfxRayData(token);
						if (!cfxData.is_null() && cfxData.contains(xorstr("Data"))) {
							auto& data = cfxData[xorstr("Data")];
							int clients = 0;
							if (data.contains(xorstr("clients")) && data[xorstr("clients")].is_number())
								clients = data[xorstr("clients")].get<int>();
							std::fprintf(stderr, xorstr("[Names] clients=%d map=%zu\n"), clients, NetworkMap.size());
						}
						UpdatePlayerNames(cfxData);
					}
					catch (...) {}

					std::this_thread::sleep_for(std::chrono::milliseconds(3000));
				}
			}
		};

		inline cUpdateNames g_UpdateNames;
	}

}
