#pragma once
#include <ezWebSockifyLib/ezWebSockifyLib.hpp>
#include <fstream>
#include <tchar.h>
#include <strsafe.h>
#include <map>
#include <set>
#include <vector>
#include <cassert>
#include <aclapi.h>
#include <nlohmann/json.hpp>

// -///////////////////////////////////////////////////////////////////////////////

std::wstring utf8to16(const std::string& src);

// -///////////////////////////////////////////////////////////////////////////////

extern std::wstring_view websockifyDefaultServiceName;
extern std::wstring_view websockifyDefaultServiceDisplayName;
extern std::wstring_view websockifyDefaultServiceDescription;

// -///////////////////////////////////////////////////////////////////////////////

struct ServiceSettings {
	std::wstring _name       { websockifyDefaultServiceName.data() };
	std::wstring _displayName{ websockifyDefaultServiceDisplayName.data() };
	std::wstring _description{ websockifyDefaultServiceDescription.data() };
};

extern ServiceSettings _serviceSettings;
bool readSetupArguments(std::wstring const& path);

// -///////////////////////////////////////////////////////////////////////////////

struct ServiceInstanceSettings
{
	uint16_t    _wsPort{0};
	std::string _tcpHost{};
	uint16_t    _tcpPort{0};
	spdlog::filename_t _logPath{};

	bool read(std::wstring const& path);
};

// -///////////////////////////////////////////////////////////////////////////////

void WINAPI websockifyServiceMain(DWORD dwArgc, LPTSTR* lpszArgv);
VOID SvcInstall(ServiceSettings const & ss);
VOID SvcUninstall(const std::wstring& serviceName);

// -///////////////////////////////////////////////////////////////////////////////

VOID SvcReportEvent(LPCTSTR szFunction, LPCTSTR szServiceName);