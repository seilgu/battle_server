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

	jsoncons::json make_car_positions(std::vector<simulation::car> &cars);
	jsoncons::json make_turbo_available();
}

#endif
