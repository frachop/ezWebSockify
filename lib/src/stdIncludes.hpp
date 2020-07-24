//
//  stdIncludes.hpp
//  ezWebSockify
//
//  Created by Franck  on 23/07/2020.
//  Copyright © 2020 Frachop. All rights reserved.
//

#ifndef stdIncludes_h
#define stdIncludes_h

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

#endif /* stdIncludes_h */
