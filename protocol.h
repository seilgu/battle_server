#ifndef HWO_PROTOCOL_H
#define HWO_PROTOCOL_H

#include <string>
#include <iostream>
#include <jsoncons/json.hpp>

#include "simulation.h"

namespace hwo_protocol
{
	jsoncons::json make_request(const std::string& msg_type, const jsoncons::json& data);
	//jsoncons::json make_join(const std::string& name, const std::string& key);
	jsoncons::json make_ping();

	jsoncons::json make_game_start();
	jsoncons::json make_car_positions(std::vector<simulation::car> &cars);
	jsoncons::json make_turbo_available();
	jsoncons::json make_crash( simulation::car& );
	jsoncons::json make_spawn(  simulation::car& );
	jsoncons::json make_turbo_start(  simulation::car& );
	jsoncons::json make_turbo_end(  simulation::car& );
	jsoncons::json make_finish( simulation::car& );
	jsoncons::json make_lap_finished(  simulation::car& );
	jsoncons::json make_tournament_end();
}

#endif
