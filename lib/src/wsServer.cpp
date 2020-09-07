//
//  wsServer.cpp
//  ezWebSockifyLib
//
//  Created by Franck  on 22/07/2020.
//  Copyright © 2020 Frachop. All rights reserved.
//
#include "stdIncludes.hpp"
#include "common.hpp"

namespace ezWebSockify {
	namespace http = beast::http;

	class WsSession
	:	private Bridge
	{
	private:
		Context & _context;
		net::io_context& _ioc;
		websocket::stream<beast::tcp_stream> _ws;
		frame_queue_t _writeQueue;
		beast::flat_buffer _buffer;
		net::steady_timer _timer;
		std::atomic_bool _closed;

	public:
		WsSession(Context& context, net::io_context& io_context, tcp::socket&& socket, IStream& in, OStream& out);
		~WsSession();

		void run(http::request<http::string_body> const& req);
	
	private:
		void on_accept(beast::error_code ec);
		void do_read();
		void on_read(beast::error_code ec, std::size_t bytes_transferred);
		void onTimer();
		void onWrite(beast::error_code ec, std::size_t bytes_transferred);
	};

	// - ///////////////////////////////////////////////////////////////////////////////////////////////////////

	WsSession::WsSession(Context& context, net::io_context& io_context, tcp::socket&& socket, IStream& in, OStream& out)
		: Bridge{ in, out }
		, _context{ context }
		, _ioc{ io_context }
		, _ws{ std::move(socket) }
		, _writeQueue{}
		, _buffer{ }
		, _timer{ _ws.get_executor(), std::chrono::milliseconds(1000) }
		, _closed{ false }
	{
		LOGTFN;
		_timer.async_wait(std::bind(&WsSession::onTimer, this));
	}

	WsSession::~WsSession() {
		LOGTFN;
		_context.stop(); // _mustQuit = true;
	}

	static std::set<std::string> protocolRequestedByClient(http::request<http::string_body> const& req)
	{
		std::string protocolList;
		LOGD("Sec-Websocket-Protocol from client : {}", req["Sec-Websocket-Protocol"]);
		for (auto const& f : req) {

			if (f.name() == http::field::sec_websocket_protocol)
			{
				if (!protocolList.empty())
					protocolList += ',';

				protocolList += std::string(f.value());
			}
		}
		std::vector<std::string> pVec;
		boost::split(pVec, protocolList, boost::algorithm::is_any_of(","), boost::token_compress_on);
		std::set<std::string> pSet; // uniqueness
		for (auto const& protocol : pVec)
		{
			auto const pLower = boost::trim_copy(boost::to_lower_copy(protocol));
			if (!pLower.empty())
				pSet.insert(pLower);
		}
		return pSet;
	}


	void WsSession::run(http::request<http::string_body> const& req)
	{
		LOGTFN;

		// Set decorators to change the Server of the handshake
		_ws.set_option(
			websocket::stream_base::timeout::suggested(
				beast::role_type::server));

		auto const pSet{ protocolRequestedByClient(req) };
		if (!pSet.empty())
		{
			if (pSet.find("binary") == pSet.end())
			{
				LOGW("Sec-Websocket-Protocol from client : {}", req["Sec-Websocket-Protocol"]);
				LOGW("'binary' protocol is not accepted by client. Set 'binary' to force use");
			}

			_ws.set_option(
				websocket::stream_base::decorator(
					[](http::response_header<>& hdr) {
						hdr.set(
							http::field::sec_websocket_protocol,
							"binary");
					}));
		}

		/*
		ws_.set_option(websocket::stream_base::decorator(
			[](websocket::response_type& res)
			{
				res.set(http::field::server,
					std::string(BOOST_BEAST_VERSION_STRING) +
					" websocket-server-async");
			}));
		*/

		_ws.async_accept(req, beast::bind_front_handler(
			&WsSession::on_accept,
			this));
	}

	void WsSession::on_accept(beast::error_code ec)
	{
		LOGTFN;
		if (ec)
		{
			LOGE("ws onAccept : {}", ec.message());
			_closed = true;
			_context.stop(); // _mustQuit = true;
			return; // fail(ec, "accept");
		}

		_ws.binary(true);

		// Read a message
		do_read();
	}

	void WsSession::do_read()
	{
		// Read a message into our buffer
		LOGTFN;
		_ws.async_read(
			_buffer,
			beast::bind_front_handler(
				&WsSession::on_read,
				this
			)
		);
	}

	void WsSession::on_read(beast::error_code ec, std::size_t bytes_transferred) {
		LOGTFN;
		boost::ignore_unused(bytes_transferred);

		// This indicates that the session was closed
		if (ec == websocket::error::closed)
		{
			std::cout << "the WS session was closed" << std::endl;
			_closed = true;
			_context.stop(); // ._mustQuit = true;
			return;
		}
		if (ec)
		{
			LOGE("ws Read : {}", ec.message());
			_closed = true;
			_context.stop(); //  _mustQuit = true;
			return;
		}

		else {
			Frame* f = out().reserve();
			f->resize(net::buffer_size(_buffer.data()));
			memcpy(f->data(), net::buffer_cast<uint8_t const*>(beast::buffers_front(_buffer.data())), f->size());
			_buffer.consume(_buffer.size());
			out().add(f);
			do_read();
		}
	}

	void WsSession::onTimer()
	{
		//std::cout << std::this_thread::get_id() << " " << BOOST_CURRENT_FUNCTION << std::endl;
		if (!_closed)
		{
			auto frame = in().get();
			if (frame)
			{
				const bool write_in_progress = !_writeQueue.empty();
				_writeQueue.push_back(frame);
				if (!write_in_progress)
				{
					_ws.async_write(
						net::buffer(_writeQueue.front()->data(), _writeQueue.front()->size()),
						beast::bind_front_handler(
							&WsSession::onWrite,
							this
						)
					);
				}
			}

			_timer.expires_at(_timer.expiry() + TIMER_TIMEOUT);
			_timer.async_wait(std::bind(&WsSession::onTimer, this));
		}
	}

	void WsSession::onWrite(beast::error_code ec, std::size_t bytes_transferred)
	{
		LOGTFN;
		assert(!_writeQueue.empty());
		in().release(_writeQueue.front());
		_writeQueue.pop_front();

		if (ec)
		{
			LOGE("ws onWrite : {}", ec.message());
			_closed = true;
			_context.stop(); // _mustQuit = true;
			return;
		}

		if (!_writeQueue.empty())
		{
			_ws.async_write(
				net::buffer(_writeQueue.front()->data(), _writeQueue.front()->size()),
				beast::bind_front_handler(
					&WsSession::onWrite,
					this
				)
			);
		}
	}

	// - ///////////////////////////////////////////////////////////////////////////////////////////////////////

	WSServer::WSServer(Context & context, net::io_context& io_context, IStream & in, OStream & out)
	:	_context{context}
	,	_ioc{io_context}
	,	_in{in}
	,	_out{out}
	,	_acceptor{io_context}
	,	_uniqueSession{nullptr}
	{
		LOGTFN;
	}
	
	WSServer::~WSServer()
	{}

	void WSServer::start(tcp::endpoint const & endpoint)
	{
		LOGTFN;
		beast::error_code ec;

        // Open the acceptor
        _acceptor.open(endpoint.protocol(), ec);
        
        if (!ec)
			// Allow address reuse
			_acceptor.set_option(net::socket_base::reuse_address(true), ec);

        if (!ec)
			// Bind to the server address
			_acceptor.bind(endpoint, ec);
			
		LOGT("WSServer listening");
		std::cout << endpoint << std::endl;

        if (!ec)
			// Start listening for connections
			_acceptor.listen(1, ec); // accept only one connection

		_acceptor.async_accept(
			net::make_strand(_ioc),
			beast::bind_front_handler(
                &WSServer::onAccept,
                this));
	
		//_socket, std::bind( &WSServer::onAccept, this, std::placeholders::_1));
	}
	

	void WSServer::onAccept(beast::error_code ec, tcp::socket socket)
    {
		LOGTFN;
        if (!ec)
        {
			// This buffer is required for reading HTTP messages
			boost::beast::flat_buffer buffer;

			// Read the HTTP request ourselves
			boost::beast::http::request<boost::beast::http::string_body> req;
			boost::beast::http::read(socket, buffer, req);

			// See if its a WebSocket upgrade request
			if (websocket::is_upgrade(req))
			{
				// Create the session and run it
				assert(_uniqueSession.get() == nullptr);
				_uniqueSession.reset(new WsSession(_context, _ioc, std::move(socket), _in, _out));
				_uniqueSession->run(req);
			}
			else
			{
				LOGW("Request is not a websocket upgrade, so reject It.");
			}
		}

		//LOGW("Accept only one connection.");
        // Do not Accept another connection
        // do_accept();
    }

}
