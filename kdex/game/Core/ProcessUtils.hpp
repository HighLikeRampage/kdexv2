#pragma once
#include <Core/Offsets.hpp>
#include <Includes/Includes.hpp>
#include <Security/xorstr.hpp>
#include <Auth/lazyimporter.hpp>
#include <Windows.h>
#include <fstream>
#include <regex>
#include <string>
#include <utility>

namespace Core {

inline std::pair<DWORD, std::wstring>
GetProcessPidByName(const std::wstring &ProcessNamePattern) {
  HANDLE hSnapshot = SafeCall(CreateToolhelp32Snapshot)(TH32CS_SNAPPROCESS, 0);
  if (!hSnapshot || hSnapshot == INVALID_HANDLE_VALUE ||
      hSnapshot == ((HANDLE)(LONG_PTR)ERROR_BAD_LENGTH)) {
    return {0, L""};
  }

  DWORD Pid = 0;
  std::wstring processName;
  PROCESSENTRY32 ProcessEntry;
  ProcessEntry.dwSize = sizeof(ProcessEntry);

  std::wregex pattern(ProcessNamePattern);
  if (SafeCall(Process32First)(hSnapshot, &ProcessEntry)) {
    do {
      std::wstring exeFile(ProcessEntry.szExeFile);
      if (std::regex_match(exeFile, pattern)) {
        Pid = ProcessEntry.th32ProcessID;
        processName = exeFile;
        break;
      }
    } while (SafeCall(Process32Next)(hSnapshot, &ProcessEntry));
  }

  SafeCall(CloseHandle)(hSnapshot);
  return {Pid, processName};
}

inline std::string trim(const std::string &str) {
  size_t first = str.find_first_not_of(' ');
  if (first == std::string::npos)
    return "";
  size_t last = str.find_last_not_of(' ');
  return str.substr(first, last - first + 1);
}

inline int GetBuild() {
  char appDataA[MAX_PATH] = {0};
  std::wstring appDataPath;
  if (GetEnvironmentVariableA((xorstr("LOCALAPPDATA")), appDataA, MAX_PATH) >
      0) {
    wchar_t appDataW[MAX_PATH] = {0};
    if (MultiByteToWideChar(CP_ACP, 0, appDataA, -1, appDataW, MAX_PATH) > 0)
      appDataPath = appDataW;
  }
  std::string pathSuffix = xorstr("\\FiveM\\FiveM.app\\CitizenFX.ini");
  std::wstring iniFilePath =
      appDataPath + std::wstring(pathSuffix.begin(), pathSuffix.end());

  std::ifstream iniFile(iniFilePath);
  if (!iniFile.is_open()) {
    return -1;
  }

  std::string line;
  while (std::getline(iniFile, line)) {
    line = trim(line);

    if (line == xorstr("ReplaceExecutable=0")) {
      return 3258;
    }
  }

  iniFile.clear();
  iniFile.seekg(0);
  while (std::getline(iniFile, line)) {
    line = trim(line);

    if (line.find(xorstr("SavedBuildNumber=")) != std::string::npos) {
      std::string eq = xorstr("=");
      std::string buildNumberStr = line.substr(line.find(eq) + 1);
      buildNumberStr = trim(buildNumberStr);
      if (buildNumberStr.empty()) continue;
      try { return std::stoi(buildNumberStr); } catch (...) { return -1; }
    }
  }

  return -1;
}

}