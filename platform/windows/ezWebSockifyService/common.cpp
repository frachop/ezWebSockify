#include "common.hpp"
#include "serviceMessage.hpp"

std::wstring utf8to16(const std::string& src)
{
	std::vector<wchar_t> buffer;
	buffer.resize(MultiByteToWideChar(CP_UTF8, 0, src.data(), -1, 0, 0));
	MultiByteToWideChar(CP_UTF8, 0, src.data(), -1, &buffer[0], buffer.size());
	return &buffer[0];
}


//
// Purpose: 
//   Logs messages to the event log
//
// Parameters:
//   szFunction - name of function that failed
// 
// Return value:
//   None
//
// Remarks:
//   The service must have an entry in the Application event log.
//
VOID SvcReportEvent(LPCTSTR szFunction, LPCTSTR szServiceName)
{
	HANDLE hEventSource;
	LPCTSTR lpszStrings[2];
	TCHAR Buffer[80];

	szServiceName = (szServiceName == nullptr) ? L"EclairExpress" : szServiceName;
	hEventSource = RegisterEventSource(NULL, szServiceName);

	if (NULL != hEventSource)
	{
		StringCchPrintf(Buffer, 80, TEXT("%s failed with %d"), szFunction, GetLastError());

		lpszStrings[0] = szServiceName;
		lpszStrings[1] = Buffer;

		ReportEvent(hEventSource,        // event log handle
			EVENTLOG_ERROR_TYPE, // event type
			0,                   // event category
			SVC_ERROR,           // event identifier
			NULL,                // no security identifier
			2,                   // size of lpszStrings array
			0,                   // no binary data
			lpszStrings,         // array of strings
			NULL);               // no binary data

		DeregisterEventSource(hEventSource);
	}
}
