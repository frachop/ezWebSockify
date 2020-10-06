#include "common.hpp"
#include "baseService.hpp"
#include <boost/lexical_cast.hpp>

// -///////////////////////////////////////////////////////////////////////////////

std::wstring_view websockifyDefaultServiceName{ L"ezWebsockify" };
std::wstring_view websockifyDefaultServiceDisplayName{ L"ezWebsockify" };
std::wstring_view websockifyDefaultServiceDescription{ L"ez Websockify Service" };
ServiceSettings _serviceSettings;

// -///////////////////////////////////////////////////////////////////////////////

bool ServiceInstanceSettings::read(std::wstring const& path)
{
	using nlohmann::json;
	try {

		std::wifstream iFile(path);
		std::wstring content;
		if (!iFile)
			return false;

		{
			std::wstring line;
			while (std::getline(iFile, line))
				content += line;
		}

		json j = json::parse(content);
		j["wsPort"].get_to(_wsPort);
		j["tcpHost"].get_to(_tcpHost);
		j["tcpPort"].get_to(_tcpPort);

		if (j.contains("loggerPath"))
			_logPath = utf8to16(j["loggerPath"].get<std::string>());

		return true;
	}
	catch (std::exception const& e)
	{
		//std::ofstream of{ L"c:\\opt\\tmp\\service.txt", std::ios::out | std::ios::app };
		//of << e.what() << std::endl;

		*this = {};
		return false;
	}
}

// -///////////////////////////////////////////////////////////////////////////////

class WebsockifyService
	: public BaseService
{
private:
	static WebsockifyService* _instance;

public:
	static WebsockifyService* get() { return _instance; }

public:
	WebsockifyService()= delete;
	explicit WebsockifyService(std::wstring const& name);
	~WebsockifyService();
	/*
	static std::wstring_view const _name() {
		return websockifyServiceName;
	}

	virtual std::wstring_view name() const override {
		return _name();
	}
	*/

public:
	virtual bool pendingStart(DWORD dwArgc, LPTSTR* lpszArgv) override;
	virtual void pendingStop() override;

private:
	virtual LPHANDLER_FUNCTION getCtrlHandler() const override;
	virtual DWORD getStarTimeoutMs() const override { return 20000; }

private:
	SERVICE_STATUS        _status;
	SERVICE_STATUS_HANDLE _statusHandle;
	HANDLE                _hEventStop;
};


// -///////////////////////////////////////////////////////////////////////////////

WebsockifyService* WebsockifyService::_instance{ nullptr };

// -///////////////////////////////////////////////////////////////////////////////

WebsockifyService::WebsockifyService(std::wstring const& name)
:	BaseService{name}
,	_status{ 0 }
,	_statusHandle{0}
,	_hEventStop{0}
{
	assert(_instance == nullptr);
	_instance = this;
}

WebsockifyService::~WebsockifyService()
{
	assert(_instance == this);
	_instance = nullptr;
}

VOID WINAPI vpnServiceCtrlHandler(DWORD dwCtrl)
{
	assert(WebsockifyService::get());
	WebsockifyService::get()->ctrlHandler(dwCtrl);
}

LPHANDLER_FUNCTION WebsockifyService::getCtrlHandler() const
{
	return vpnServiceCtrlHandler;
}

bool WebsockifyService::pendingStart(DWORD dwArgc, LPTSTR* lpszArgv)
{
	/*
	std::wofstream of{ L"c:\\opt\\tmp\\service.txt" };
	of << dwArgc << L" " << _serviceSettings._name << std::endl;
	for (DWORD i = 0; i < dwArgc; ++i)
		of << std::wstring(lpszArgv[i]) << std::endl;
	*/

	if (!BaseService::pendingStart(dwArgc, lpszArgv))
		return false;
	
	if (dwArgc < 2)
		return false;

	ServiceInstanceSettings is;
	if (dwArgc == 2)
	{
		if (!is.read(lpszArgv[1]))
			return false;
	}
	else if (dwArgc < 4)
		return false;

	else {
		try {
			if (dwArgc > 4)
				is._logPath = lpszArgv[4];

			is._wsPort = boost::lexical_cast<uint16_t>(lpszArgv[1]);

			std::wstring_view wtcpHost(lpszArgv[2]);
			is._tcpHost= std::string( wtcpHost.begin(), wtcpHost.end() );
			
			is._tcpPort = boost::lexical_cast<uint16_t>(lpszArgv[3]);
	
		}
		catch (...) {
			return false;
		}
	}

	try
	{
		ezWebSockify::setLoggerPath(is._logPath);
		ezWebSockify::start(is._wsPort, is._tcpHost.data(), is._tcpPort);
	}
	catch (std::exception const&)
	{
		return false;
	}
	return true;
}

void WebsockifyService::pendingStop()
{
	ezWebSockify::stop();
	ezWebSockify::cleanup();
	BaseService::pendingStop();
}

// -///////////////////////////////////////////////////////////////////////////////

void WINAPI websockifyServiceMain(DWORD dwArgc, LPTSTR* lpszArgv)
{
	WebsockifyService service{ _serviceSettings._name };
	service.main(dwArgc, lpszArgv);
}

