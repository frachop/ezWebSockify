#include "common.hpp"
#include <iostream>
#include <array>
#include <optional>
#include <psapi.h> // for EnumProcesses

#pragma comment(lib, "advapi32.lib")


// -///////////////////////////////////////////////////////////////////////////////

bool readSetupArguments(std::wstring const & path)
{
	/*
		service settings file structure :

		{
			"name": "serviceName",
			"displayName": "service display name",
			"description": "service description
		}
	
	*/
	using nlohmann::json;

	try {

		std::wifstream iFile(path);
		std::wstring content;
		if (!iFile)
			return false;

		std::wstring line;
		while (std::getline(iFile, line))
			content += line;

		json j = json::parse(content);
		_serviceSettings._name= utf8to16(j["name"].get<std::string>());
		_serviceSettings._displayName= utf8to16(j["displayName"].get<std::string>());
		_serviceSettings._description= utf8to16(j["description"].get<std::string>());
		return true;
	}
	catch (std::exception const & e)
	{
		wprintf(L"error parsing %s", path.data());
		printf("%s", e.what());
		_serviceSettings = ServiceSettings{};
		return false;
	}
}

// -///////////////////////////////////////////////////////////////////////////////

class CServiceHandle
{
public:
	CServiceHandle() : _handle(nullptr) {}
	explicit CServiceHandle(SC_HANDLE h) : _handle(h) {}
	~CServiceHandle() { close(); }

	SC_HANDLE handle() const
	{
		return _handle;
	}

	bool setHandle(SC_HANDLE h)
	{
		close();
		_handle = h;
		return _handle != nullptr;
	}

	bool close()
	{
		bool result(true);
		if (_handle)
			result = CloseServiceHandle(_handle) ? true : false;

		_handle = nullptr;
		return result;
	}

	bool isValid() const { return _handle != nullptr; }
	operator bool() const { return isValid(); }
	operator SC_HANDLE() const { return _handle; }

private:
	SC_HANDLE _handle;
};

// -///////////////////////////////////////////////////////////////////////////////

class CServiceManager
	: public CServiceHandle
{
public:
	explicit CServiceManager(DWORD dwDesiredAccess)
		: CServiceHandle(OpenSCManager(nullptr, nullptr, dwDesiredAccess)) {}

	~CServiceManager() {}
};

// -///////////////////////////////////////////////////////////////////////////////



//- /////////////////////////////////////////////////////////////////////////////////////////////////////////

//
// Purpose: 
//   Updates the service DACL to grant start, stop, delete, and read
//   control access to the Guest account.
//
// Parameters:
//   None
// 
// Return value:
//   None
//
VOID __stdcall DoUpdateSvcDacl(SC_HANDLE schService, const TCHAR* userName)
{
	EXPLICIT_ACCESS      ea;
	SECURITY_DESCRIPTOR  sd;
	PSECURITY_DESCRIPTOR psd = NULL;
	PACL                 pacl = NULL;
	PACL                 pNewAcl = NULL;
	BOOL                 bDaclPresent = FALSE;
	BOOL                 bDaclDefaulted = FALSE;
	DWORD                dwError = 0;
	DWORD                dwSize = 0;
	DWORD                dwBytesNeeded = 0;

	// Get the current security descriptor.

	if (!QueryServiceObjectSecurity(schService,
		DACL_SECURITY_INFORMATION,
		&psd,           // using NULL does not work on all versions
		0,
		&dwBytesNeeded))
	{
		if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
		{
			dwSize = dwBytesNeeded;
			psd = (PSECURITY_DESCRIPTOR)HeapAlloc(GetProcessHeap(),
				HEAP_ZERO_MEMORY, dwSize);
			if (psd == NULL)
			{
				// Note: HeapAlloc does not support GetLastError.
				printf("HeapAlloc failed\n");
				goto dacl_cleanup;
			}

			if (!QueryServiceObjectSecurity(schService,
				DACL_SECURITY_INFORMATION, psd, dwSize, &dwBytesNeeded))
			{
				printf("QueryServiceObjectSecurity failed (%d)\n", GetLastError());
				goto dacl_cleanup;
			}
		}
		else
		{
			printf("QueryServiceObjectSecurity failed (%d)\n", GetLastError());
			goto dacl_cleanup;
		}
	}

	// Get the DACL.

	if (!GetSecurityDescriptorDacl(psd, &bDaclPresent, &pacl,
		&bDaclDefaulted))
	{
		printf("GetSecurityDescriptorDacl failed(%d)\n", GetLastError());
		goto dacl_cleanup;
	}

	// Build the ACE.

	BuildExplicitAccessWithName(&ea, (TCHAR*)userName,
		SERVICE_START | SERVICE_STOP | READ_CONTROL | DELETE,
		SET_ACCESS, NO_INHERITANCE);

	dwError = SetEntriesInAcl(1, &ea, pacl, &pNewAcl);
	if (dwError != ERROR_SUCCESS)
	{
		printf("SetEntriesInAcl failed(%d)\n", dwError);
		goto dacl_cleanup;
	}

	// Initialize a new security descriptor.

	if (!InitializeSecurityDescriptor(&sd,
		SECURITY_DESCRIPTOR_REVISION))
	{
		printf("InitializeSecurityDescriptor failed(%d)\n", GetLastError());
		goto dacl_cleanup;
	}

	// Set the new DACL in the security descriptor.

	if (!SetSecurityDescriptorDacl(&sd, TRUE, pNewAcl, FALSE))
	{
		printf("SetSecurityDescriptorDacl failed(%d)\n", GetLastError());
		goto dacl_cleanup;
	}

	// Set the new DACL for the service object.

	if (!SetServiceObjectSecurity(schService,
		DACL_SECURITY_INFORMATION, &sd))
	{
		printf("SetServiceObjectSecurity failed(%d)\n", GetLastError());
		goto dacl_cleanup;
	}
	else printf("Service DACL updated successfully for user %S\n", userName);

dacl_cleanup:
	if (NULL != pNewAcl)
		LocalFree((HLOCAL)pNewAcl);
	if (NULL != psd)
		HeapFree(GetProcessHeap(), 0, (LPVOID)psd);
}


static std::wstring userFromProcess(DWORD procId)
{
	std::wstring result;
	auto hProcess = OpenProcess(READ_CONTROL | PROCESS_QUERY_INFORMATION, FALSE, procId);
	HANDLE hToken(NULL);
	if (OpenProcessToken(hProcess, TOKEN_READ, &hToken))
	{
		PTOKEN_USER ptu(nullptr);
		DWORD dwLength(0);
		if (!GetTokenInformation(hToken, TokenUser, ptu, 0, &dwLength))
		{
			if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
			{
				ptu = (PTOKEN_USER)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwLength);
				if (GetTokenInformation(hToken, TokenUser, ptu, dwLength, &dwLength))
				{
					SID_NAME_USE SidType;
					TCHAR lpName[256];
					TCHAR lpDomain[256];
					DWORD dwSize(256);

					if (LookupAccountSid(NULL, ptu->User.Sid, lpName, &dwSize, lpDomain, &dwSize, &SidType))
					{
						result = lpName;
					}
				}
				HeapFree(GetProcessHeap(), 0, (LPVOID)ptu);
			}
		}
		CloseHandle(hToken);
	}
	CloseHandle(hProcess);
	return result;
}

static std::set<std::wstring> getUsers()
{
	std::array<DWORD,4096> pProcessIds;
	DWORD cbNeeded(0);
	EnumProcesses(pProcessIds.data(), static_cast<DWORD>(sizeof(pProcessIds[0]) * pProcessIds.size()), &cbNeeded);
	const std::size_t cnt = cbNeeded / sizeof(pProcessIds[0]);
	std::set<std::wstring> names;
	for (std::size_t i = 0; i < cnt; ++i)
	{
		auto name = userFromProcess(pProcessIds[i]);
		if (!name.empty())
			if (name != L"SYSTEM")
				names.insert(name);
	}
	return names;
}

//
// Purpose: 
//   Installs a service in the SCM database
//
// Parameters:
//   None
// 
// Return value:
//   None
//
VOID SvcInstall(ServiceSettings const & ss)
{
	TCHAR szPath[MAX_PATH];

	if (!GetModuleFileName(NULL, szPath, MAX_PATH))
	{
		wprintf(L"Cannot install service '%s' (%d)\n", ss._displayName.data(), GetLastError());
		return;
	}

	// Get a handle to the SCM database. 

	CServiceManager schSCManager(SC_MANAGER_ALL_ACCESS);  // full access rights 
	if (!schSCManager)
	{
		wprintf(L"OpenSCManager failed '%s' (%d)\n", ss._displayName.data(), GetLastError());
		return;
	}

	// Create the service
	std::wstring cmdLine{ szPath };
	cmdLine += L" " + ss._name;

	CServiceHandle schService;
	schService.setHandle(CreateService(
		schSCManager,              // SCM database 
		ss._name.data(),           // name of service 
		ss._displayName.data(),    // service name to display 
		SERVICE_ALL_ACCESS,        // desired access 
		SERVICE_WIN32_SHARE_PROCESS, // service type // SERVICE_WIN32_OWN_PROCESS
		SERVICE_DEMAND_START,      // start type 
		SERVICE_ERROR_NORMAL,      // error control type 
		cmdLine.data(),            // path to service's binary 
		NULL,                      // no load ordering group 
		NULL,                      // no tag identifier 
		NULL,                      // no dependencies 
		NULL,                      // LocalSystem account 
		NULL)                      // no password
	);

	if (!schService)
	{
		wprintf(L"CreateService failed '%s' (%d)\n", ss._displayName.data(), GetLastError());
		return;
	}
	else 
		wprintf(L"Service '%s' installed successfully\n", ss._displayName.data());

	// grant user access
	auto const users = getUsers();
	for (auto const& user : users)
		DoUpdateSvcDacl(schService, user.c_str());

	SERVICE_DESCRIPTION sd;
	sd.lpDescription = (LPWSTR)ss._description.data();
	/*BOOL bRes =*/ ChangeServiceConfig2(schService, SERVICE_CONFIG_DESCRIPTION, (LPVOID)&sd);
}

VOID SvcUninstall(const std::wstring& serviceName)
{
	SERVICE_STATUS ssSvcStatus = {};

	// Open the local default service control manager database 
	CServiceManager schSCManager(SC_MANAGER_CONNECT);
	if (!schSCManager)
	{
		wprintf(L"OpenSCManager failed w/err 0x%08lx\n", GetLastError());
		return;
	}

	// Open the service with delete, stop, and query status permissions 
	CServiceHandle schService;
	schService.setHandle(OpenService(schSCManager, serviceName.data(), SERVICE_STOP | SERVICE_QUERY_STATUS | DELETE));
	if (!schService)
	{
		wprintf(L"OpenService failed w/err 0x%08lx\n", GetLastError());
		return;
	}

	// Try to stop the service 
	if (ControlService(schService, SERVICE_CONTROL_STOP, &ssSvcStatus))
	{
		wprintf(L"Stopping %s.", serviceName.data());
		Sleep(1000);

		while (QueryServiceStatus(schService, &ssSvcStatus))
		{
			if (ssSvcStatus.dwCurrentState == SERVICE_STOP_PENDING)
			{
				wprintf(L".");
				Sleep(1000);
			}
			else break;
		}

		if (ssSvcStatus.dwCurrentState == SERVICE_STOPPED)
		{
			wprintf(L"\n%s is stopped.\n", serviceName.data());
		}
		else
		{
			wprintf(L"\n%s failed to stop.\n", serviceName.data());
		}
	}

	// Now remove the service by calling DeleteService. 
	if (!DeleteService(schService))
	{
		wprintf(L"DeleteService failed w/err 0x%08lx\n", GetLastError());
		return;
	}

	wprintf(L"%s is removed.\n", serviceName.data());
}
