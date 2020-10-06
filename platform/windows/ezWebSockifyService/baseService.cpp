#include "common.hpp"
#include "baseService.hpp"

// -///////////////////////////////////////////////////////////////////////////////

//
// Purpose: 
//   Called by SCM whenever a control code is sent to the service
//   using the ControlService function.

BaseService::BaseService(std::wstring const & name)
:	_name{ name }
,	_dwCheckPoint(1)
,	_hEventStop(CreateEvent(NULL,
		TRUE,    // manual reset event
		FALSE,   // not signaled
		NULL))   // no name
{
	// These SERVICE_STATUS members remain as set here
	memset(&_status, 0, sizeof(_status));
	_status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	_status.dwServiceSpecificExitCode = 0;
}

BaseService::~BaseService()
{
}

bool BaseService::main(DWORD dwArgc, LPTSTR* lpszArgv)
{

	_dwCheckPoint = 1;

	// Register the handler function for the service
	_statusHandle = RegisterServiceCtrlHandler(_name.data(), getCtrlHandler());
	if (!_statusHandle)
	{
		SvcReportEvent(TEXT("RegisterServiceCtrlHandler"), _name.data());
		return false;
	}

	// Report initial status to the SCM
	reportStatus(SERVICE_START_PENDING, NO_ERROR, getStarTimeoutMs());

	if (!pendingStart(dwArgc, lpszArgv))
	{
		reportStatus(SERVICE_STOPPED, EXIT_FAILURE, 0);
		return false;
	}

	reportStatus(SERVICE_RUNNING, NO_ERROR, 0);

	mainLoop();

	reportStatus(SERVICE_STOP_PENDING, NO_ERROR, 0);

	pendingStop();

	reportStatus(SERVICE_STOPPED, NO_ERROR, 0);

	return true;
}

bool BaseService::pendingStart(DWORD dwArgc, LPTSTR* lpszArgv)
{
	if (_hEventStop == nullptr)
	{
		SvcReportEvent(L"CreateEvent", _name.data());
		return false;
	}

	return true;
}

void BaseService::ctrlHandler(DWORD dwCtrl)
{
	switch (dwCtrl)
	{
	case SERVICE_CONTROL_STOP:
		reportStatus(SERVICE_STOP_PENDING, NO_ERROR, 0);
		SetEvent(_hEventStop);
		reportStatus(_status.dwCurrentState, NO_ERROR, 0);
		return;

	case SERVICE_CONTROL_INTERROGATE:
		break;

	default:
		break;
	}

}

void BaseService::reportStatus(DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint)
{
	// Fill in the SERVICE_STATUS structure.

	_status.dwCurrentState = dwCurrentState;
	_status.dwWin32ExitCode = dwWin32ExitCode;
	_status.dwWaitHint = dwWaitHint;

	if (dwCurrentState == SERVICE_START_PENDING)
		_status.dwControlsAccepted = 0;
	else _status.dwControlsAccepted = SERVICE_ACCEPT_STOP;

	if ((dwCurrentState == SERVICE_RUNNING) ||
		(dwCurrentState == SERVICE_STOPPED))
		_status.dwCheckPoint = 0;
	else _status.dwCheckPoint = _dwCheckPoint++;

	// Report the status of the service to the SCM.
	SetServiceStatus(_statusHandle, &_status);
}

// -///////////////////////////////////////////////////////////////////////////////

void BaseService::mainLoop()
{
	WaitForSingleObject(_hEventStop, INFINITE);
}

