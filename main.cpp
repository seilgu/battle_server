#include <iostream>
#include <string>
#include <jsoncons/json.hpp>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <cstdlib>
#include <time.h>

#include "protocol.h"
#include "connection.h"

using namespace hwo_protocol;

int main(int argc, const char* argv[])
{
	try
	{
		if (argc < 2)
		{
			std::cerr << "Usage: ./run host port botname botkey" << std::endl;
			return 1;
		}

		const std::string port(argv[1]);
		std::cout << "Port: " << port << std::endl;

		boost::asio::io_service io_service;
		hwo_server server(io_service, 8091);

		io_service.run();
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return 2;
	}

	return 0;
}
