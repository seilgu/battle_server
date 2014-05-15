#include "game_logic.h"
#include "protocol.h"
#include <ctime>
#include <ratio>
#include <chrono>

using namespace hwo_protocol;

game_logic::game_logic() : action_map {
	{ "gameInit", &game_logic::on_game_init }, 
}{}

game_logic::msg_vector game_logic::on_game_init(const jsoncons::json& data) {
	return { jsoncons::null_type() };
}

game_logic::msg_vector game_logic::react(const jsoncons::json& msg) {
	std::cout << "REACT" << msg << std::endl;
	if (!msg.has_member("msgType") || !msg.has_member("data")) 		return { make_ping() };

	const auto& msg_type = msg["msgType"].as<std::string>();
	const auto& data = msg["data"];
	auto action_it = action_map.find(msg_type);

	if (action_it != action_map.end()) {
		return (action_it->second)(this, data);
	} else {
		std::cout << "Unknown message type: " << msg_type << std::endl;
		return { make_ping() };
	}

}