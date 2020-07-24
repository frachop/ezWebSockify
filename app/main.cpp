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

int main(int argc, const char * argv[])
{
	if (argc != 4)
	{
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

	ezWebSockify::run(wsPort, tcpHost, tcpPort);
	return 0;
}
