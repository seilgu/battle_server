
#include <vector>

#include "protocol.h"

namespace hwo_protocol
{
	int gameTick;
	
	jsoncons::json make_request(const std::string& msg_type, const jsoncons::json& data)
	{
		jsoncons::json r;
		r["msgType"] = msg_type;
		r["data"] = data;
		r["gameTick"] = gameTick;
		return r;
	}

	jsoncons::json make_ping()
	{
		return make_request("ping", jsoncons::null_type());
	}

	jsoncons::json make_game_start() {
		return make_request("gameStart", jsoncons::null_type());
	}

	jsoncons::json make_car_positions(std::map<std::string, simulation::car> &cars) {
		jsoncons::json data(jsoncons::json::an_array);

		for (auto &pc : cars) {
			simulation::car &cc = pc.second;
			jsoncons::json carjson;
			carjson["id"] = jsoncons::json();
			carjson["id"]["name"] = cc.name;
			carjson["id"]["color"] = cc.color;
			carjson["piecePosition"] = jsoncons::json();
			carjson["piecePosition"]["pieceIndex"] = cc.p;
			carjson["piecePosition"]["inPieceDistance"] = cc.x;
			carjson["angle"] = cc.angle;
			carjson["piecePosition"]["lane"] = jsoncons::json();
			carjson["piecePosition"]["lane"]["startLaneIndex"] = cc.startLane;
			carjson["piecePosition"]["lane"]["endLaneIndex"] = cc.endLane;
			carjson["piecePosition"]["lap"] = cc.laps;

			data.add(carjson);
		}
		
		return make_request("carPositions", data);
	}

	jsoncons::json make_turbo_available(int turboDurationTicks, int turboFactor) {
		jsoncons::json data;
		data["turboDurationTicks"] = turboDurationTicks;
		data["turboFactor"] = turboFactor;
		return make_request("turboAvailable", data);
	}

	jsoncons::json make_crash( simulation::car &cc ) {
		jsoncons::json data;
		data["name"] = cc.name;
		data["color"] = cc.color;
		return make_request("crash", data);
	}
	jsoncons::json make_spawn( simulation::car &cc ) {
		jsoncons::json data;
		data["name"] = cc.name;
		data["color"] = cc.color;
		return make_request("spawn", data);
	}
	jsoncons::json make_turbo_start( simulation::car &cc ) {
		jsoncons::json data;
		data["name"] = cc.name;
		data["color"] = cc.color;
		return make_request("turboStart", data);
	}
	jsoncons::json make_turbo_end( simulation::car &cc ) {
		jsoncons::json data;
		data["name"] = cc.name;
		data["color"] = cc.color;
		return make_request("turboEnd", data);
	}
	jsoncons::json make_lap_finished( simulation::car &cc ) {
		jsoncons::json data;
		data["name"] = cc.name;
		data["color"] = cc.color;
		return make_request("lapFinished", data);
	}
	jsoncons::json make_finish( simulation::car &cc ) {
		jsoncons::json data;
		data["name"] = cc.name;
		data["color"] = cc.color;
		return make_request("finish", data);
	}
	jsoncons::json make_tournament_end() {
		return make_request("tournamentEnd", jsoncons::null_type());
	}

}  // namespace hwo_protocol

