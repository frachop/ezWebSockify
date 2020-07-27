//
//  common.hpp
//  ezWebSockify
//
//  Created by Franck  on 22/07/2020.
//  Copyright © 2020 Frachop. All rights reserved.
//

#pragma once
#include "../include/ezWebSockifyLib/ezWebSockifyLib.hpp"

namespace ezWebSockify
{
	spdlog::logger* getLogger();
	#define LOGT(...) SPDLOG_LOGGER_TRACE(getLogger(),  __VA_ARGS__)
	#define LOGD(...) SPDLOG_LOGGER_DEBUG(getLogger(),  __VA_ARGS__)
	#define LOGI(...) SPDLOG_LOGGER_INFO(getLogger(),  __VA_ARGS__)
	#define LOGW(...) SPDLOG_LOGGER_WARN(getLogger(),  __VA_ARGS__)
	#define LOGE(...) SPDLOG_LOGGER_ERROR(getLogger(),  __VA_ARGS__)
	#define LOGC(...) SPDLOG_LOGGER_CRITICAL(getLogger(),  __VA_ARGS__)
	
	#define LOGTFN LOGT("{}", BOOST_CURRENT_FUNCTION)

	//- /////////////////////////////////////////////////////////////////////////////////

	namespace net = boost::asio;            // from <boost/asio.hpp>
	using tcp = net::ip::tcp;               // from <boost/asio/ip/tcp.hpp>

	//- /////////////////////////////////////////////////////////////////////////////////

	constexpr std::size_t MAX_FRAME_SIZE{ 32 * 1024 };
	constexpr auto TIMER_TIMEOUT{ std::chrono::milliseconds(5) };

	//- /////////////////////////////////////////////////////////////////////////////////

	class Frame
	{
	private:
		static std::list<Frame*> _all;
	
	public:
		constexpr static size_t MAX_FRAME_SIZE { ezWebSockify::MAX_FRAME_SIZE };

		enum class Status {
			released= 0,
			readyToRead,
			readed,
			waitToWrite
		};

	public:
		Frame() : _size{0}, _status{ Status::released } { _all.push_back(this); }
		void clear() { _size = 0; }
		std::size_t size() const { return _size; }
		void resize(std::size_t sz) {
			if ( sz > MAX_FRAME_SIZE )
				throw std::range_error("frame size out of range");
			_size = sz;
		}
		uint8_t * data() { return _data.data(); }
		uint8_t const * cdata() const { return _data.data(); }

		Status status() const { return _status; }
		void setStatus(Status s) { _status= s; }

		static void dumpAll() {
			std::array<std::size_t, 4> histogram{0};
			std::for_each( std::begin(_all), std::end(_all),
				[&histogram](Frame const * f){
					histogram[ static_cast<int>(f->_status) ]++;
				}
			);
			LOGD("Frames {}", _all.size());
			LOGD("  released    [{}]", histogram[0]);
			LOGD("  readyToRead [{}]", histogram[1]);
			LOGD("  readed      [{}]", histogram[2]);
			LOGD("  waitToWrite [{}]", histogram[3]);
		}

	private:
		std::size_t _size;
		std::array<uint8_t, MAX_FRAME_SIZE> _data;
		Status _status;
	};
	
	using frame_queue_t= std::deque<Frame*> ;
}

#include "pool.hpp"

namespace ezWebSockify
{
	struct Context {
		std::atomic_bool _mustQuit{false};
	};
}

#include "tcpClient.hpp"

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>

namespace ezWebSockify{
	namespace beast = boost::beast;         // from <boost/beast.hpp>
	namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
}

#include "wsServer.hpp"

