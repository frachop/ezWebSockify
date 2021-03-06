#pragma once
// -///////////////////////////////////////////////////////////////////////////////

class BaseService
{
public:
	std::wstring const _name;

public:
	BaseService() = delete;
	explicit BaseService(std::wstring const & name) ;
	~BaseService();


public:
	bool main(DWORD dwArgc, LPTSTR* lpszArgv);
	virtual void ctrlHandler(DWORD dwCtrl);

protected:
	virtual LPHANDLER_FUNCTION getCtrlHandler() const = 0;
	virtual DWORD getStarTimeoutMs() const { return 6000; }

	virtual bool pendingStart(DWORD dwArgc, LPTSTR* lpszArgv);
	virtual void mainLoop();
	virtual void pendingStop() {}

protected:
	void reportStatus(DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint);

private:
	DWORD                 _dwCheckPoint;
	SERVICE_STATUS        _status;
	SERVICE_STATUS_HANDLE _statusHandle;
	HANDLE                _hEventStop;
};
