#include <iostream>
#include <string>
#include <jsoncons/json.hpp>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <cstdlib>
#include <time.h>

#include "protocol.h"
#include "game_logic.h"
#include "connection.h"

using namespace hwo_protocol;

void run(hwo_connection& connection, const std::string& name, const std::string& key)
{
	game_logic game;

	connection.send_requests({ make_join(name, key) });

	for (;;)
	{
		boost::system::error_code error;
		auto response = connection.receive_response(error);

		if (error == boost::asio::error::eof)
		{
			std::cout << "Connection closed" << std::endl;
			break;
		}
		else if (error)
		{
			throw boost::system::system_error(error);
		}

		game_logic::msg_vector res = game.react(response);
		connection.send_requests(res);
		std::cout << std::endl;
	}
}

int main(int argc, const char* argv[])
{
	try
	{
		if (argc < 5)
		{
			std::cerr << "Usage: ./run host port botname botkey" << std::endl;
			return 1;
		}

		const std::string host(argv[1]);
		const std::string port(argv[2]);
		const std::string name(argv[3]);
		const std::string key(argv[4]);
		std::cout << "Host: " << host << ", port: " << port << ", name: " << name << ", key:" << key << std::endl;

		hwo_connection connection(host, port);
		run(connection, name, key);

	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return 2;
	}

	return 0;
}
