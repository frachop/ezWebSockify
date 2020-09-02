//
//  wsServer.hpp
//  ezWebSockifyLib
//
//  Created by Franck  on 22/07/2020.
//  Copyright © 2020 Frachop. All rights reserved.
//

#pragma once
namespace ezWebSockify {

	class WsSession;

	class WSServer
	{
	private:
		Context & _context;
		net::io_context& _ioc;
		IStream & _in;
		OStream & _out;
		tcp::acceptor _acceptor;
		std::unique_ptr<WsSession> _uniqueSession;

	public:
		WSServer(Context & context, net::io_context& io_context, IStream & _in, OStream & out);
		~WSServer();
		void start(tcp::endpoint const & endpoint);
		
	private:
		void onAccept(beast::error_code ec, tcp::socket socket);
	};

}

