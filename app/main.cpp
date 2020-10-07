//
//  main.cpp
//  ezWebSockify
//
//  Created by Franck  on 22/07/2020.
//  Copyright © 2020 Frachop. All rights reserved.
//

#include <ezWebSockifyLib/ezWebSockifyLib.hpp>
#include <iostream>
#include <limits>
#include <csignal>
#include <thread>


namespace
{
	volatile std::sig_atomic_t gSignalStatus;
}

void signal_handler(int signal)
{
	std::cout << "handling ctrl-c signal" << std::endl;
	gSignalStatus = signal;
	ezWebSockify::stop();
}


int main(int argc, const char * argv[])
{
	if (argc != 4)
	{
		std::cerr << "ezWebSockify version " << ezWebSockify::getVersionString() << std::endl;
		std::cerr << "Usage: ezWebSockify <wsPort> <tcpHost> <tcpPort>" << std::endl;
		return 1;
	}

	auto const wsPort{ std::atoi(argv[1]) };
	auto const tcpHost{ argv[2] };
	auto const tcpPort{ std::atoi(argv[3]) };

	if ((wsPort <= 0) || (tcpPort <= 0) || (wsPort > std::numeric_limits<uint16_t>::max()) || (tcpPort > std::numeric_limits<uint16_t>::max()))
	{
		std::cerr << "port out of range" << std::endl;
		return 1;
	}

	// Install a signal handler
	std::signal(SIGINT, signal_handler);

	ezWebSockify::start(wsPort, tcpHost, tcpPort);
	ezWebSockify::wait();
	ezWebSockify::cleanup();
	return 0;
}
