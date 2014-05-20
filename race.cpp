#include "race.h"

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
	engine.reset();

	race_finished_ = false;

	thread_running_ = true;
	race_thread_ = boost::thread(&hwo_race::run, this);
}

void hwo_race::run() {

	int tick = 0;
	while (thread_running_) {
		for (auto &s : sessions_) {
			if ( !(s->socket().is_open()) )  continue;
			boost::system::error_code error;
			std::vector<jsoncons::json> msgs = { make_ping() };
			s->send_response( msgs, error );
			if (!error) {	//if (msgs.size() > 0)	std::cout << "response sent: " << msgs[0] << std::endl;
			} else {}

			auto request = s->receive_request(error);
			if (!error) {	//std::cout << "request received: " << request << std::endl;
			} else {}
		}
		tick++;

		// check if anybody alive
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