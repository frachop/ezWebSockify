//
//  stdIncludes.hpp
//  ezWebSockify
//
//  Created by Franck  on 23/07/2020.
//  Copyright © 2020 Frachop. All rights reserved.
//

#ifndef stdIncludes_h
#define stdIncludes_h

#include "platform.hpp"

#if defined(DEBUG) || defined(_DEBUG)
#	if !defined(DEBUG)
#		define DEBUG 1
#	endif
#	if !defined(_DEBUG)
#		define _DEBUG 1
#	endif
#else
#	if !defined(NDEBUG)
#		define NDEBUG 1
#	endif
#endif

#if EZWSKFY_TARGET_PLATFORM(WINDOWS)
#	if !defined(NOMINMAX)
#		define NOMINMAX
#	endif
#	define _WINSOCKAPI_ // stops windows.h including winsock.h
#	include <windows.h>
#	include <winsock2.h>
typedef long long ssize_t;

// disable warnings about exception specifications,
// which are not implemented in Visual C++
#	pragma warning(disable:4290)
#	define PLATFORM_PID DWORD
#else

#	define PLATFORM_PID pid_t
#endif




#if DEBUG
#	define SPDLOG_ACTIVE_LEVEL 0
#else
#	define SPDLOG_ACTIVE_LEVEL 3
#endif

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/fmt/ostr.h>

#include <boost/asio/dispatch.hpp>
#include <boost/asio/strand.hpp>
#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/algorithm/string.hpp>
#include <iostream>
#include <thread>
#include <list>
#include <deque>
#include <vector>
#include <cinttypes>
#include <algorithm>
#include <cstdlib>
#include <functional>
#include <memory>
#include <string>
#include <set>
#include <vector>

#endif /* stdIncludes_h */
