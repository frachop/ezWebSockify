//
//  ezWebSockifyLib.hpp
//  ezWebSockify
//
//  Created by Franck  on 22/07/2020.
//  Copyright © 2020 Frachop. All rights reserved.
//

#pragma once
#include <string>
#include <cinttypes>
#include <spdlog/spdlog.h>

namespace ezWebSockify {

	// - ///////////////////////////////////////////////////////////////////////////////////////////////////////

	constexpr int VERSION_MAJOR{ 1 };
	constexpr int VERSION_MINOR{ 0 };
	constexpr int VERSION_PATCH{ 0 };
	constexpr int VERSION{ VERSION_MAJOR * 10000 + VERSION_MINOR * 100 + VERSION_PATCH };
	inline std::string_view getVersionString() { return "1.0.0"; }

	// - ///////////////////////////////////////////////////////////////////////////////////////////////////////

	void setLoggerPath(spdlog::filename_t const& path);
	spdlog::logger* getLogger();

#	define LOGT(...) SPDLOG_LOGGER_TRACE(getLogger(),  __VA_ARGS__)
#	define LOGD(...) SPDLOG_LOGGER_DEBUG(getLogger(),  __VA_ARGS__)
#	define LOGI(...) SPDLOG_LOGGER_INFO(getLogger(),  __VA_ARGS__)
#	define LOGW(...) SPDLOG_LOGGER_WARN(getLogger(),  __VA_ARGS__)
#	define LOGE(...) SPDLOG_LOGGER_ERROR(getLogger(),  __VA_ARGS__)
#	define LOGC(...) SPDLOG_LOGGER_CRITICAL(getLogger(),  __VA_ARGS__)
#	define LOGTFN LOGT("{}", BOOST_CURRENT_FUNCTION)

	bool start(uint16_t wsPort, std::string const& tcpHost, uint16_t tcpPort);
	void wait();
	bool running();
	void stop();
	void cleanup();
}
