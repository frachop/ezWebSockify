//
//  tcpClient.cpp
//  ezWebSockifyLib
//
//  Created by Franck  on 22/07/2020.
//  Copyright © 2020 Frachop. All rights reserved.
//

#include "stdIncludes.hpp"
#include "common.hpp"

namespace ezWebSockify {

	TCPClient::TCPClient(Context & context, net::io_context& io_context, IStream & in, OStream & out)
	:	Bridge{in, out}
	,	_context{context}
	,	_ioc{io_context}
	,	_socket{io_context}
	,	_writeQueue{}
	,	_frameBuffer{ nullptr }
	,	_timer{ io_context, std::chrono::milliseconds(1000) }
	,	_closed{false}
	{
		LOGTFN;
	}

	void TCPClient::start(tcp::resolver::results_type const& endpoints)
	{
		LOGTFN;
		
		//tcp::endpoint ep{boost::asio::ip::address::from_string("127.0.0.1"), 7171};
		net::async_connect(_socket, endpoints,
			std::bind( &TCPClient::connected, this, std::placeholders::_1, std::placeholders::_2) );

		_timer.async_wait(std::bind(&TCPClient::onTimer, this));
	}

	TCPClient::~TCPClient()
	{
		LOGTFN;

	}

	void TCPClient::onTimer()
	{ 
		if (!_closed)
		{
			auto frame = in().get();
			if (frame)
			{
				const bool write_in_progress = !_writeQueue.empty();
				_writeQueue.push_back(frame);
				if (!write_in_progress)
				{
					doWrite();
				}
			}
		
			_timer.expires_at(_timer.expiry() + TIMER_TIMEOUT);
			_timer.async_wait(std::bind(&TCPClient::onTimer, this));
		}
	}

	void TCPClient::close()
	{
		LOGTFN;
		
		_context.stop();
		//_context._mustQuit = true;
		_closed = true;
		net::post(_ioc, [this]() { _socket.close(); });
	}

	void TCPClient::connected(boost::system::error_code ec, tcp::endpoint)
	{
		LOGTFN;

		if (!ec)
		{
			doRead();
		}
		else
		{
			LOGE("tcp connect : {}", ec.message());
			close();
		}
	}

	void TCPClient::doRead()
	{
		LOGTFN;

		if (_frameBuffer == nullptr )
			_frameBuffer = out().reserve();
			
		_socket.async_read_some(net::buffer( _frameBuffer->data(), Frame::MAX_FRAME_SIZE),
			std::bind(&TCPClient::readed, this, std::placeholders::_1, std::placeholders::_2));
	}

	void TCPClient::readed(boost::system::error_code ec, std::size_t length)
	{
		LOGTFN;

		if (!ec)
		{
			_frameBuffer->resize(length);
			out().add(_frameBuffer);
			_frameBuffer = nullptr;
			doRead();
		}
		else
		{
			LOGE("tcp readed : {}", ec.message());
			close();
		}
	}

	void TCPClient::doWrite()
	{
		LOGTFN;

		assert(!_writeQueue.empty());
		net::async_write(
			_socket,
			net::buffer(_writeQueue.front()->data(), _writeQueue.front()->size()),
			std::bind( &TCPClient::writed, this, std::placeholders::_1, std::placeholders::_2)
		);
	}

	void TCPClient::writed(boost::system::error_code ec, std::size_t length)
	{
		LOGTFN;
		
		assert(!_writeQueue.empty());
		in().release( _writeQueue.front() );
		_writeQueue.pop_front();

		if (!ec)
		{
		
			if (!_writeQueue.empty())
			{
				doWrite();
			}
		}
		else
		{
			LOGE("tcp writed : {}", ec.message());
			close();
		}
	}


}
