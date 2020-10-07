#include "common.hpp"
// change account
// sc.exe config cinemanextTMSWebsockify obj= "NT AUTHORITY\LocalService" password= ""

// -///////////////////////////////////////////////////////////////////////////////

SERVICE_TABLE_ENTRY DispatchTable[] =
{
	{ (LPWSTR)_serviceSettings._name.data(), (LPSERVICE_MAIN_FUNCTION)websockifyServiceMain },
	{ NULL, NULL }
};

// -///////////////////////////////////////////////////////////////////////////////

int __cdecl _tmain(int argc, TCHAR* argv[])
{
	// If command-line parameter is "version", display lib version. 
	if ((argc > 1) && (lstrcmpi(argv[1], TEXT("version")) == 0))
	{
		printf("ezWebSockifyService version %s\n", ezWebSockify::getVersionString().data()) ;
	}

	// If command-line parameter is "install", install the service. 
	else if ((argc > 1) && (lstrcmpi(argv[1], TEXT("install")) == 0))
	{
		if (argc > 2)
			readSetupArguments(argv[2]);

		SvcInstall(_serviceSettings);
	}

	// If command-line parameter is "uninstall", uninstall the service. 
	else if ((argc > 1) && (lstrcmpi(argv[1], TEXT("uninstall")) == 0))
	{
		if (argc > 2)
			_serviceSettings._name= argv[2];

		SvcUninstall(_serviceSettings._name);
	}

	else
	{
		if (argc == 2)
			_serviceSettings._name = argv[1];

		DispatchTable[0].lpServiceName = _serviceSettings._name.data();

		// This call returns when the service has stopped. 
		// The process should simply terminate when the call returns.

		if (!StartServiceCtrlDispatcher(DispatchTable))
			SvcReportEvent(TEXT("StartServiceCtrlDispatcher"), nullptr);
	}

	return 0;

}
