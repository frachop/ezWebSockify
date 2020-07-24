//
//  main.cpp
//  ezWebSockify
//
//  Created by Franck  on 22/07/2020.
//  Copyright © 2020 Frachop. All rights reserved.
//

#include <ezWebSockifyLib/ezWebSockifyLib.hpp>

int main(int argc, const char * argv[]) {
	ezWebSockify::run(4822, "192.168.1.49", 5900);
	return 1;
}
