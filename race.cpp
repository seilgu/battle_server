#include "race.h"

#include "util.h"

using namespace hwo_protocol;

/** hwo_race **/
hwo_race::hwo_race(race_param param) : param_(param) {
	BOOST_ASSERT(param_.maxPlayers > 0);
	sessions_.resize(0);
	race_finished_ = false;
	thread_running_ = false;
}
hwo_race::~hwo_race() {
	thread_running_ = false;
	race_thread_.join();
	for (auto &s : sessions_) {
		s.reset();
	}
}
std::string hwo_race::getUniqueId(race_param &p) {
	//
	return "lalala";
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

	if ( session->maxPlayers_ != param_.maxPlayers ) {
		session->terminate("numPlayers does not match");
		return 0;
	} else if ( sessions_.size() >= param_.maxPlayers ) {
		session->terminate("race already full");
		return 0;
	} else if ( session->key_ != param_.key ){
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

void hwo_race::start() {

	std::string trackfileName = "keimola.track";
	std::ifstream trackfile(trackfileName);
	if (!trackfile.is_open()) {
		std::cout << "can't open track file \"" << trackfileName << "\"";
		return;
	}
	jsoncons::json trackjson = jsoncons::json::parse(trackfile);
	trackfile.close();

	jsoncons::json msg_track;
	msg_track["msgType"] = "gameInit";
	msg_track["data"] = trackjson;

	boost::system::error_code error;
	for (auto &s : sessions_) {
		s->send_response( { msg_track }, error );
		s->receive_request( error );
	}

	// setup engine
	sim_.reset();
	sim_.cars.clear();

	for (auto &s : sessions_) {
		simulation::car cc;
		simulation::set_empty_car(cc);
		cc.name = s->name_;
		cc.color = "red";	
		sim_.cars.push_back(cc);
	}

	sim_.turboFactor = 3.0;
	turboDurationTicks = 30;
	turboAvailableTicks.clear();
	turboAvailableTicks.push_back(600);
	turboAvailableTicks.push_back(1200);
	turboAvailableTicks.push_back(1800);

	race_finished_ = false;

	tick = 0;

	thread_running_ = true;
	race_thread_ = boost::thread(&hwo_race::run, this);
}

// todo : for request result not immediate, need event queue
void hwo_race::handle_request(jsoncons::json &request, hwo_session_ptr session) {
	for (auto &cc : sim_.cars) {
		if (cc.name != session->name_) continue;

		/*if (cc.useTurbo && cc.turboAvailable > 0) {
			cc.turboAvailable = 0;
			cc.useTurbo = false;
			cc.turboBeginTick = tick;
			cc.onTurbo = true;
		}

		if (cc.tick - cc.turboBeginTick > turboDurationTicks 
			|| cc.tick < cc.turboBeginTick) cc.onTurbo = false;

		*/
	}
}

void hwo_race::run() {
	
	boost::system::error_code error;

	while (thread_running_) {

		// 3. update simulator
		// 4. send results
		// 1. send event
		// 2. get client actions

		sim_.update();

		std::vector<jsoncons::json> msgs;

		msgs.push_back( make_car_positions(sim_.cars) );

		for (auto tt : turboAvailableTicks) {
			if (tick == tt) {
				msgs.push_back( make_turbo_available() );
				break;
			}
		}

		for (auto &s : sessions_) {
			error = boost::system::error_code(); // need correct value or io will fail
			s->send_response( msgs, error );
			std::cout << s->name_ << " send_resp:" << error.message() << std::endl;
		}

		for (auto &s : sessions_) {
			error = boost::system::error_code(); // need correct value or io will fail
			auto request = s->receive_request(error);
			std::cout << s->name_ << " recv_resp:" << error.message() << std::endl;
			handle_request(request, s);
		}

		tick++;

		// check if anybody there
		bool keep_running = false;
		//LOG("size ", sessions_.size());
		for (auto &s : sessions_) {
			if (s->socket().is_open()) {
		//		LOG1("A");
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