//
//  common.cpp
//  ezWebSockifyLib
//
//  Created by Franck  on 22/07/2020.
//  Copyright © 2020 Frachop. All rights reserved.
//
#include "stdIncludes.hpp"
#include "common.hpp"
#include <spdlog/sinks/base_sink.h>
#include <spdlog/sinks/dist_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <fstream>

namespace ezWebSockify
{
	static spdlog::filename_t _loggerPath{};

	void setLoggerPath(spdlog::filename_t const& path) {
		_loggerPath = path;
	}

	spdlog::logger* getLogger()
	{
		static std::mutex _m;
		static std::unique_ptr<spdlog::logger> _l{nullptr};
		_m.lock();
		if (_l.get() == nullptr)
		{
			// std::ofstream of("c:\\opt\\tmp\\logger.txt", std::ios::out | std::ios::app);
			// of << "LOGGER " << std::string( _loggerPath.begin(), _loggerPath.end() ) << std::endl;

			auto dist_sink   = std::make_shared<spdlog::sinks::dist_sink_mt>();
			auto stdout_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
			dist_sink->add_sink(stdout_sink );

			if (!_loggerPath.empty()) {
				// Create a file rotating logger with 1MB size max and 3 rotated files
				std::size_t max_size = 1024*1024 * 1;
				std::size_t max_files = 3;
				auto rf_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(_loggerPath, max_size, max_files, false);
				dist_sink->add_sink(rf_sink);
			}

			_l.reset( new spdlog::logger("ezWebSockify", dist_sink) );
			_l->set_level(spdlog::level::trace);
			_l->set_pattern("[%T][%t:%L] %v");
			//register_logger(l);
		}
		_m.unlock();
		return _l.get();
	}

	std::list<Frame*> Frame::_all{};

	class EngineImpl
	{
		net::io_context _iocTcp;
		net::io_context _iocWs;
		FramePool       _framePool;
		IOStream        _ws2tcp;
		IOStream        _tcp2ws;
		
		Context         _context;
		
		TCPClient       _tcpClient;
		WSServer        _wsServer;
	
		std::thread     _thMonitor;

	public:
		EngineImpl();
		~EngineImpl();
		void start(uint16_t wsPort, std::string const & tcpHost, uint16_t tcpPort);
		void stop() { _context.stop(); }
		void wait() { _context.wait(); }
	};

	EngineImpl::EngineImpl()
	:	_iocTcp{}
	,	_iocWs{}
	,	_framePool{}
	,	_ws2tcp{_framePool}
	,	_tcp2ws{_framePool}
	,	_context{}
	,	_tcpClient{_context, _iocTcp, _ws2tcp, _tcp2ws}
	,	_wsServer{_context, _iocWs, _tcp2ws, _ws2tcp}
	,	_thMonitor{}
	{
	}

	EngineImpl::~EngineImpl()
	{
		LOGT("{}", BOOST_CURRENT_FUNCTION);
		if (_thMonitor.joinable())
		{
			stop();
			_thMonitor.join();
		}
	}

	void EngineImpl::start(uint16_t wsPort, std::string const & tcpHost, uint16_t tcpPort /*tcp::resolver::results_type const& endpoints*/ )
	{
		_tcpClient.start(tcp::resolver(_iocTcp).resolve(tcpHost, std::to_string(tcpPort)));
		_wsServer.start(tcp::endpoint{net::ip::make_address("127.0.0.1"), wsPort});

		_thMonitor = std::thread( [this](){
			std::thread thTcp = std::thread([this](){ _iocTcp.run(); });
			std::thread thWs  = std::thread([this](){ _iocWs.run(); });
			
			_context.wait();
			_iocWs.stop();
			_iocTcp.stop();

			thWs.join();
			thTcp.join();
		});
	}


	// - ///////////////////////////////////////////////////////////////////////////////////////////////////////

	static std::unique_ptr<EngineImpl> _instance{nullptr};

	bool start(uint16_t wsPort, std::string const& tcpHost, uint16_t tcpPort)
	{
		if (_instance.get())
			return false;

		LOGI("Starting {} <-> {}:{}", wsPort, tcpHost, tcpPort);

		_instance.reset(new EngineImpl());
		_instance->start(wsPort, tcpHost, tcpPort);
		return true;
	}

	bool running()
	{
		return _instance.get() != nullptr;
	}

	void cleanup()
	{
		if (_instance.get())
			_instance.reset(nullptr);
	}

	void wait()
	{
		if (_instance.get())
		{
			_instance->wait();
			cleanup();
		}
	}

	void stop()
	{
		if (!_instance.get())
			return;

		_instance->stop();
	}

	// - ///////////////////////////////////////////////////////////////////////////////////////////////////////

	void run(uint16_t wsPort, std::string const & tcpHost, uint16_t tcpPort)
	{
		try
		{
			EngineImpl engine;
			engine.start(wsPort, tcpHost, tcpPort);
		}
		catch (std::exception& e)
		{
			LOGE("Exception: {}", e.what());
		}
		
	}

}
