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

namespace ezWebSockify {

	void run(uint16_t wsPort, std::string const & tcpHost, uint16_t tcpPort);

}
