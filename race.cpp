#include "race.h"
#include "session.h"

#include "util.h"

#include <jsoncons/json.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

using namespace hwo_protocol;

/** hwo_race **/

hwo_race::hwo_race( hwo_session_ptr &s ) 
	: action_map {
		{ "ping", &hwo_race::on_ping }, 
		{ "throttle", &hwo_race::on_throttle }, 
		{ "switchLane", &hwo_race::on_switch_lane }, 
		{ "turbo", &hwo_race::on_turbo }
	}
{
	BOOST_ASSERT(s->carCount_ > 0);

	param_.trackname = s->trackname_;
	param_.password = s->password_;
	param_.carCount = s->carCount_;
	param_.raceId = hwo_race::getUniqueId(param_);
	
	param_.laps = 5;	

	sessions_.resize(0);
	race_finished_ = false;
	thread_running_ = false;
}

hwo_race::~hwo_race() {
	thread_running_ = false;
	race_thread_.join();

	// simply clearing it doesn't make use_count to zero
	for (auto s : sessions_)
		s->terminate("race ended");
	sessions_.clear();
}
std::string hwo_race::getUniqueId(race_param &p) {
	boost::uuids::uuid uuid = boost::uuids::random_generator()();
	return boost::uuids::to_string(uuid);
}

void hwo_race::on_ping(const jsoncons::json& data, const hwo_session_ptr s) {}
void hwo_race::on_throttle(const jsoncons::json& data, const hwo_session_ptr s) {
	try {
		double tt = data.as<double>();
		carmap[s]->throttle = tt;
	} catch (const jsoncons::json_exception& e) {
		std::cout << e.what() << std::endl;
	}
}
void hwo_race::on_switch_lane(const jsoncons::json& data, const hwo_session_ptr s) {
	try {
		if (data.as<std::string>() == "Right")		carmap[s]->direction = 1;
		else if (data.as<std::string>() == "Left")	carmap[s]->direction = -1;
	} catch (const jsoncons::json_exception& e) {
		std::cout << e.what() << std::endl;
	}
}
void hwo_race::on_turbo(const jsoncons::json& data, const hwo_session_ptr s) {
	carmap[s]->turboBeginTick = tick + 1;
}

void hwo_race::start() {

	// setup track, constants, cars
	std::string trackfileName = param_.trackname + ".track";
	std::ifstream trackfile(trackfileName);
	if (!trackfile.is_open()) {
		std::cout << "can't open track file \"" << trackfileName << "\"";
		race_finished_ = true;
		return;
	}
	jsoncons::json trackjson = jsoncons::json::parse(trackfile);
	trackfile.close();

	sim_.set_track(trackjson);

	sim_.k1 = 0.2;
	sim_.k2 = 0.02;
	sim_.A = 0.5303;

	sim_.turboFactor = 3.0;
	param_.turboDurationTicks = 30;
	param_.crashDurationTicks = 5;

	param_.turboAvailableTicks.clear();
	param_.turboAvailableTicks.push_back(600);
	param_.turboAvailableTicks.push_back(1200);
	param_.turboAvailableTicks.push_back(1800);

	sim_.cars.clear();
	std::vector<std::string> colors = { "red", "blue", "yellow", "orange", "purple", "green", 
		"brown", "pink" };
	for (auto &s : sessions_) {
		simulation::car cc;
		simulation::set_empty_car(cc);
		cc.name = s->name_;
		cc.color = colors.back();
		colors.pop_back();
		sim_.cars.push_back(cc);

		carmap[s] = &sim_.cars.back();
	}

	race_finished_ = false;

	tick = 0;

	boost::system::error_code error;

	// setup yourcar / gameInit
	for (auto s : sessions_) {	
		simulation::car &cc = *carmap[s];
		jsoncons::json msg_yourcar;
		msg_yourcar["msgType"] = "yourCar";
		msg_yourcar["data"] = jsoncons::json();
		msg_yourcar["data"]["name"] = cc.name;
		msg_yourcar["data"]["color"] = cc.color;
		s->send_response( { msg_yourcar }, error );
	}

	jsoncons::json msg_track;
	msg_track["msgType"] = "gameInit";
	msg_track["data"] = jsoncons::json();
	msg_track["data"]["race"] = jsoncons::json();
	msg_track["data"]["race"]["track"] = trackjson;

	jsoncons::json carjson(jsoncons::json::an_array);
	for (auto &cc : sim_.cars) {
		jsoncons::json cj;
		
		jsoncons::json idjson;
		idjson["name"] = cc.name;
		idjson["color"] = cc.color;
		cj["id"] = idjson;

		jsoncons::json dimjson;
		dimjson["length"] = 40.0;
		dimjson["width"] = 20.0;
		cj["dimensions"] = dimjson;

		carjson.add(cj);
	}
	msg_track["data"]["race"]["cars"] = carjson;

	jsoncons::json rs;
	rs["laps"] = param_.laps;
	msg_track["data"]["race"]["raceSession"] = rs;

	for (auto &s : sessions_) {
		s->send_response( { msg_track }, error );
	}

	thread_running_ = true;
	race_thread_ = boost::thread(&hwo_race::run, this);
}

//extern int hwo_protocol::gameTick;
void hwo_race::run() {
	
	std::map<hwo_session_ptr, boost::system::error_code> errors;
	for (auto &s : sessions_) 	errors[s] = boost::system::error_code();

	{
		jsoncons::json game_start = make_game_start();
		for (auto &s : sessions_) {
			s->send_response( { game_start } , errors[s] );
			auto request = s->receive_request( errors[s] );
		}
	}

	// 3. update simulator
	// 4. send results
	// 1. send event
	// 2. get client actions	
	
	std::vector<jsoncons::json> msgs;

	while (thread_running_) {

		for (auto tt : param_.turboAvailableTicks) {
			if (tick == tt) {
				msgs.push_back( make_turbo_available( param_.turboDurationTicks, sim_.turboFactor ) );
				break;
			}
		}

		// who crashed
		for (auto &s : sessions_) {
			simulation::car &cc = *carmap[s];
			if (cc.crashing == false && fabs(cc.angle) > 60.0) {
				cc.crashing = true;
				cc.lastCrashedTick = tick;
				cc.angle = 0;
				cc.v = 0;
				cc.w = 0;
				cc.throttle = 0;
				msgs.push_back( make_crash(cc) );
			}
		}
		// who spawned
		for (auto &s : sessions_) {
			simulation::car &cc = *carmap[s];
			if (cc.crashing == true && tick - cc.lastCrashedTick > param_.crashDurationTicks) {
				cc.crashing = false;
				msgs.push_back( make_spawn(cc) );
			}
		}
		// who turboStart
		for (auto &s : sessions_) {
			simulation::car &cc = *carmap[s];
			if (cc.turboBeginTick == tick) {
				cc.onTurbo = true;
				msgs.push_back( make_turbo_start(cc) );
			}
		}
		// who turboEnd
		for (auto &s : sessions_) {
			simulation::car &cc = *carmap[s];
			if (cc.onTurbo == true && tick - cc.turboBeginTick > param_.turboDurationTicks) {
				cc.onTurbo = false;
				msgs.push_back( make_turbo_end(cc) );
			}
		}

		// lapfinished
		for (auto &s : sessions_) {
			simulation::car &cc = *carmap[s];
			if (cc.newlap == true) {
				cc.newlap = false;
				msgs.push_back( make_lap_finished(cc) );
			}
		}

		// racefinished
		for (auto &s : sessions_) {
			simulation::car &cc = *carmap[s];
			if (cc.laps >= param_.laps) {
				cc.finishedRace = true;
				msgs.push_back( make_finish(cc) );
			}
		}

		msgs.push_back( make_car_positions(sim_.cars) );

		for (auto &s : sessions_) {
			s->send_response( msgs, errors[s] );
		}
		
		for (auto &s : sessions_) {
			auto request = s->receive_request( errors[s] );
			handle_request(request, s);
		}

		sim_.update();
		tick++;
		hwo_protocol::gameTick = tick;

		msgs.clear();

		// check if anybody there
		bool keep_running = false;
		for (auto &s : sessions_) {
			if (s->socket().is_open()) {
				keep_running = true;
				break;
			}
		}

		if (keep_running == false) break;

	}

	thread_running_ = false;
	race_finished_ = true;
}

void hwo_race::end() {
	thread_running_ = false;
}

race_param hwo_race::get_race_param() const {
	return param_;
}

int hwo_race::nPlayers() const {
	return sessions_.size();
}


bool hwo_race::race_finished() const {
	return race_finished_;
}

int hwo_race::join(hwo_session_ptr session) {

	if ( session->carCount_ != param_.carCount ) {
		session->terminate("numPlayers does not match");
		return 0;
	} else if ( sessions_.size() >= param_.carCount ) {
		session->terminate("race already full");
		return 0;
	} else if ( session->password_ != param_.password ){
		session->terminate("key incorrect");
		return 0;
	}

	for (auto &s : sessions_) {
		if (s->name_ == session->name_) {
			session->terminate("name already taken");
			return 0;
		}
	}

	sessions_.push_back(session);

	return 1;
}

// todo : for request result not immediate, need event queue
void hwo_race::handle_request(jsoncons::json &request, hwo_session_ptr session) {
	if (!request.has_member("msgType") || !request.has_member("data")) return;

	const auto& msg_type = request["msgType"].as<std::string>();
	const jsoncons::json& data = request["data"];
	auto action_it = action_map.find(msg_type);

	if (action_it != action_map.end()) {
		(action_it->second)(this, data, session);
	}
}