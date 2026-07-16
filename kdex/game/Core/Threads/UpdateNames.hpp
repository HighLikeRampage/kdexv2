#pragma once
#include <Security/Api/json.hpp>
#include <Includes/Includes.hpp>
#include <Includes/Utils.hpp>
#include <Core/Offsets.hpp>
#include <Core/SDK/SDK.hpp>

#include <fstream>
#include <string>
#include <cctype>
#include <deque>

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
			std::string ServerToken;
			std::string DirFiveM;
			std::string RedirectUrl;
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

			static std::string& LastWorkingToken() { static std::string s; return s; }

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
					if (!tok.empty()) return tok;
				}

				if (ServerIp.empty() && !g_Variables.ServerIp.empty())
					ServerIp = g_Variables.ServerIp;
				if (ServerIp.empty())
					return xorstr("");

				g_Variables.ServerIp = ServerIp;

				std::string Token = TryExtractTokenFromRedirect();
				if (!Token.empty()) return Token;
				if (!LastWorkingToken().empty()) return LastWorkingToken();
				return xorstr("");
			}
		public:

			nlohmann::json GetPlayerData() {
				ServerToken = GetServerToken();

				if (ServerToken.empty())
					return NULL;

				std::string ApiUrl = xorstr("https://servers-frontend.fivem.net/api/servers/single/") + ServerToken;

				std::string ResponseStr;
				CURL* hnd;
				CURLcode res;
				hnd = curl_easy_init();
				if (hnd) {
					curl_easy_setopt(hnd, CURLOPT_CUSTOMREQUEST, xorstr("GET"));
					curl_easy_setopt(hnd, CURLOPT_URL, ApiUrl.c_str());
					curl_easy_setopt(hnd, CURLOPT_TIMEOUT, 10L);
					struct curl_slist* headers = NULL;
					headers = curl_slist_append(headers, xorstr("User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36"));
					curl_easy_setopt(hnd, CURLOPT_HTTPHEADER, headers);
					curl_easy_setopt(hnd, CURLOPT_WRITEFUNCTION, WriteCallBack);
					curl_easy_setopt(hnd, CURLOPT_WRITEDATA, &ResponseStr);
					res = curl_easy_perform(hnd);
					curl_easy_cleanup(hnd);
				}

				if (ResponseStr.empty())
					return NULL;

				nlohmann::json ResponseJson;
				try { ResponseJson = json::parse(ResponseStr); } catch (...) { return NULL; }
				if (!ResponseJson.is_object() && !ResponseJson.is_array())
					return NULL;
				nlohmann::json ServerData = ResponseJson;
				if (ResponseJson.contains(xorstr("Data")))
					ServerData = ResponseJson[xorstr("Data")];
				else if (ResponseJson.contains((xorstr("data"))))
					ServerData = ResponseJson[(xorstr("data"))];
				if (!ServerData.contains(xorstr("players")) && !ServerData.contains((xorstr("players"))))
					return NULL;
				nlohmann::json PlayersObj = ServerData.contains(xorstr("players")) ? ServerData[xorstr("players")] : ServerData[(xorstr("players"))];
				nlohmann::json PlayersArray = PlayersObj.contains(xorstr("list")) ? PlayersObj[xorstr("list")] : (PlayersObj.contains((xorstr("list"))) ? PlayersObj[(xorstr("list"))] : PlayersObj);
				if (!PlayersArray.is_array())
					return NULL;

				LastWorkingToken() = ServerToken;
				return PlayersArray;
			}

			std::unordered_map<std::string, std::string> DiscordUsernameCache;
			std::mutex DiscordUsernameCacheMutex;

			std::deque<std::string> DiscordIdQueue;
			std::mutex DiscordIdQueueMutex;
			bool WorkerThreadStarted = false;

			std::string GetDiscordUsername(const std::string& discordId) {
				if (discordId.empty()) return "";

				{
					std::lock_guard<std::mutex> lock(DiscordUsernameCacheMutex);
					auto dit = DiscordUsernameCache.find(discordId);
					if (dit != DiscordUsernameCache.end()) return dit->second;

					DiscordUsernameCache[discordId] = discordId;
				}

				{
					std::lock_guard<std::mutex> lock(DiscordIdQueueMutex);
					DiscordIdQueue.push_back(discordId);
					if (!WorkerThreadStarted) {
						WorkerThreadStarted = true;
						std::thread([this]() {
							while (!g_Variables.g_Unload) {
								std::string currentId = "";
								{
									std::lock_guard<std::mutex> lock(DiscordIdQueueMutex);
									if (!DiscordIdQueue.empty()) {
										currentId = DiscordIdQueue.front();
										DiscordIdQueue.pop_front();
									}
								}

								if (currentId.empty()) {
									std::this_thread::sleep_for(std::chrono::milliseconds(500));
									continue;
								}

								std::string ApiUrl = xorstr("https://discord-lookup-api.vercel.app/v1/user/") + currentId;
								std::string ResponseStr;
								CURL* hnd = curl_easy_init();
								if (hnd) {
									curl_easy_setopt(hnd, CURLOPT_CUSTOMREQUEST, xorstr("GET"));
									curl_easy_setopt(hnd, CURLOPT_URL, ApiUrl.c_str());
									curl_easy_setopt(hnd, CURLOPT_TIMEOUT, 3L);
									struct curl_slist* headers = NULL;
									headers = curl_slist_append(headers, xorstr("User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64)"));
									curl_easy_setopt(hnd, CURLOPT_HTTPHEADER, headers);
									curl_easy_setopt(hnd, CURLOPT_WRITEFUNCTION, WriteCallBack);
									curl_easy_setopt(hnd, CURLOPT_WRITEDATA, &ResponseStr);
									curl_easy_perform(hnd);
									curl_easy_cleanup(hnd);
								}

								bool success = false;
								if (!ResponseStr.empty()) {
									try {
										nlohmann::json ResponseJson = json::parse(ResponseStr);
										std::string finalName = "";
										if (ResponseJson.contains(xorstr("global_name")) && !ResponseJson[xorstr("global_name")].is_null()) {
											finalName = ResponseJson[xorstr("global_name")].get<std::string>();
										} else if (ResponseJson.contains(xorstr("username")) && !ResponseJson[xorstr("username")].is_null()) {
											finalName = ResponseJson[xorstr("username")].get<std::string>();
										}

										if (!finalName.empty()) {
											std::lock_guard<std::mutex> lock(DiscordUsernameCacheMutex);
											DiscordUsernameCache[currentId] = finalName;
											success = true;
										}
									} catch (...) {}
								}

								if (!success) {
									std::string FallbackApiUrl = xorstr("") + currentId;
									ResponseStr.clear();
									hnd = curl_easy_init();
									if (hnd) {
										curl_easy_setopt(hnd, CURLOPT_CUSTOMREQUEST, xorstr("GET"));
										curl_easy_setopt(hnd, CURLOPT_URL, FallbackApiUrl.c_str());
										curl_easy_setopt(hnd, CURLOPT_TIMEOUT, 3L);
										struct curl_slist* headers = NULL;
										headers = curl_slist_append(headers, xorstr("User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64)"));
										curl_easy_setopt(hnd, CURLOPT_HTTPHEADER, headers);
										curl_easy_setopt(hnd, CURLOPT_WRITEFUNCTION, WriteCallBack);
										curl_easy_setopt(hnd, CURLOPT_WRITEDATA, &ResponseStr);
										curl_easy_perform(hnd);
										curl_easy_cleanup(hnd);
									}
									if (!ResponseStr.empty()) {
										try {
											nlohmann::json ResponseJson = json::parse(ResponseStr);
											std::string finalName = "";
											if (ResponseJson.contains(xorstr("display_name")) && !ResponseJson[xorstr("display_name")].is_null()) {
												finalName = ResponseJson[xorstr("display_name")].get<std::string>();
											} else if (ResponseJson.contains(xorstr("username")) && !ResponseJson[xorstr("username")].is_null()) {
												finalName = ResponseJson[xorstr("username")].get<std::string>();
											}

											if (!finalName.empty()) {
												std::lock_guard<std::mutex> lock(DiscordUsernameCacheMutex);
												DiscordUsernameCache[currentId] = finalName;
												success = true;
											}
										} catch (...) {}
									}
								}

								if (!success) {
								}

								std::this_thread::sleep_for(std::chrono::milliseconds(50));
							}
						}).detach();
					}
				}

				return discordId;
			}

			void GetPlayerNames()
			{
				nlohmann::json PlayersArr = GetPlayerData();

				if (PlayersArr == NULL) {
					std::lock_guard<std::mutex> lock(NamesMutex);
					NetworkMap.clear();
					return;
				}

				std::unordered_map<int, Core::SDK::Game::NetworkInfo> newMap;
				newMap.reserve(256);

				for (const auto& Player : PlayersArr)
				{
					if (!Player.is_object() || !Player.contains(xorstr("id")) || !Player.contains(xorstr("name")))
						continue;
					int PlayerId = 0;
					if (Player[xorstr("id")].is_number_integer())
						PlayerId = Player[xorstr("id")].get<int>();
					else if (Player[xorstr("id")].is_string())
						PlayerId = std::atoi(Player[xorstr("id")].get<std::string>().c_str());
					else
						continue;
					std::string PlayerName = Player[xorstr("name")].is_string() ? Player[xorstr("name")].get<std::string>() : std::string();

					std::string Discord, SteamId;
					if (Player.contains(xorstr("identifiers")) && Player[xorstr("identifiers")].is_array())
					{
						for (const auto& Identifier : Player[xorstr("identifiers")])
						{
							if (!Identifier.is_string())
								continue;

							std::string IdentifierVal = Identifier.get<std::string>();

							if (IdentifierVal.find(xorstr("discord:")) != std::string::npos)
								Discord = GetDiscordUsername(IdentifierVal.substr(8));
							else if (IdentifierVal.find(xorstr("steam:")) != std::string::npos)
								SteamId = IdentifierVal.substr(6);
						}
					}

					newMap[PlayerId] = { PlayerName, Discord, SteamId };
				}

				{
					std::lock_guard<std::mutex> lock(NamesMutex);
					NetworkMap.swap(newMap);
				}
			}

			void Update()
			{
				while (!g_Variables.g_Unload)
				{
					try {
						GetPlayerNames();
					}
					catch (const std::exception& e) {
						std::string errorMessage = xorstr("Crash Detected. Code: 2\nException: ");
						errorMessage += e.what();
						break;
					}
					catch (...) {
						break;
					}

					std::this_thread::sleep_for(std::chrono::milliseconds(1000));
				}
			}
		};

		inline cUpdateNames g_UpdateNames;
	}

}