#include <iostream>
#include <string>

#include <readline/readline.h>
#include <readline/history.h>

#include "protocol.h"
#include "server.h"
#include "race.h"

int main(int argc, const char* argv[])
{
	boost::thread io_thread;

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

		io_thread = boost::thread( boost::bind( &boost::asio::io_service::run, &io_service ) );
		
		// readline
		rl_bind_key('\t', rl_complete);
		char *input;
		while (1) {
			input = readline("server> ");
			if (!input) break;

			if (!strcmp(input, "list")) {
				
				hwo_race_manager::Instance().mutex_.lock();
				for (auto &r : hwo_race_manager::Instance().get_racelist()) {
					std::cout << "\trace " << (r->get_race_param()).trackname << std::endl;
				}
				hwo_race_manager::Instance().mutex_.unlock();
				
			}

			add_history(input);
			free(input);
		}

		io_service.stop();

	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return 2;
	}

	std::cout << std::endl << std::endl;

	return 0;
}
