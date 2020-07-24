//
//  tcpClient.hpp
//  ezWebSockifyLib
//
//  Created by Franck  on 22/07/2020.
//  Copyright © 2020 Frachop. All rights reserved.
//

#pragma once
namespace ezWebSockify {


	class TCPClient
	:	private Bridge
	{
	private:
		Context & _context;
		net::io_context& _ioc;
		tcp::socket _socket;
		frame_queue_t _writeQueue;
		Frame* _frameBuffer;
		net::steady_timer _timer;
		std::atomic_bool _closed;

	public:
		TCPClient(Context & context, net::io_context& io_context, IStream & in, OStream & out);
		virtual ~TCPClient();
		void start(tcp::resolver::results_type const& endpoints);
		void close();

	private:
		void connected(boost::system::error_code ec, tcp::endpoint);
		void doRead();
		void readed(boost::system::error_code ec, std::size_t length);
		void doWrite();
		void writed(boost::system::error_code ec, std::size_t length);
		void onTimer();
	};

}
