//
//  common.cpp
//  ezWebSockifyLib
//
//  Created by Franck  on 22/07/2020.
//  Copyright © 2020 Frachop. All rights reserved.
//

#include "common.hpp"
#include <spdlog/sinks/base_sink.h>
#include <spdlog/sinks/dist_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace ezWebSockify
{
	spdlog::logger* getLogger()
	{
		auto l = spdlog::get("ezWebSockify");
		if (!l)
		{
			auto dist_sink   = std::make_shared<spdlog::sinks::dist_sink_mt>();
			auto stdout_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
			dist_sink->add_sink(stdout_sink );
			l = std::make_shared<spdlog::logger>("ezWebSockify", dist_sink);

			l->set_level(spdlog::level::trace);
			l->set_pattern("[%T][%t:%L][%-15s] %v");
			
			register_logger(l);
		}
		
		return l.get();
	}

	std::list<Frame*> Frame::_all{};

	class Engine {
	
		net::io_context _iocTcp;
		net::io_context _iocWs;
		FramePool       _framePool;
		IOStream        _ws2tcp;
		IOStream        _tcp2ws;
		Context         _context;
		
		TCPClient       _tcpClient;
		WSServer        _wsServer;
	
		std::thread _thTcp;
		std::thread _thWs;
		std::thread _thMonitor;
	public:
		Engine();
		void run(uint16_t wsPort, std::string const & tcpHost, uint16_t tcpPort);
	};



	Engine::Engine()
	:	_iocTcp{}
	,	_iocWs{}
	,	_framePool{}
	,	_ws2tcp{_framePool}
	,	_tcp2ws{_framePool}
	,	_context{}
	,	_tcpClient{_context, _iocTcp, _ws2tcp, _tcp2ws}
	,	_wsServer{_context, _iocWs, _tcp2ws, _ws2tcp}
	,	_thTcp{}
	,	_thWs{}
	,	_thMonitor{}
	{
	}

	void Engine::run(uint16_t wsPort, std::string const & tcpHost, uint16_t tcpPort /*tcp::resolver::results_type const& endpoints*/ )
	{
		_tcpClient.start(tcp::resolver(_iocTcp).resolve(tcpHost, std::to_string(tcpPort)));
		_wsServer.start(tcp::endpoint{net::ip::make_address("127.0.0.1"), wsPort});

		_thTcp = std::thread([this](){ _iocTcp.run(); });
		_thWs  = std::thread([this](){ _iocWs.run(); });
		_thMonitor = std::thread( [this](){
			while (!_context._mustQuit) {
				std::this_thread::sleep_for(std::chrono::milliseconds(20));
			}
			_iocWs.stop();
			_iocTcp.stop();
		});

		_thMonitor.join();
		_thWs.join();
		_thTcp.join();
	}

	void run(uint16_t wsPort, std::string const & tcpHost, uint16_t tcpPort)
	{
		try
		{
			Engine engine;
			engine.run(wsPort, tcpHost, tcpPort);
		}
		catch (std::exception& e)
		{
			LOGE "Exception: {}", e.what());
		}
		
	}

}
