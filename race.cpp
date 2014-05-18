#include "race.h"

using namespace hwo_protocol;

/** hwo_race **/
hwo_race::hwo_race(std::string racename, int maxPlayers) 
	: racename_(racename), maxPlayers_(maxPlayers) {
	BOOST_ASSERT(maxPlayers > 0);
	sessions_.resize(0);
	thread_running_ = false;
}
hwo_race::~hwo_race() {
	thread_running_ = false;
	race_thread_.join();
	for (auto &s : sessions_) {
		s.reset();
	}
}

int hwo_race::maxPlayers() {
	return maxPlayers_;
}

int hwo_race::nPlayers() {
	return sessions_.size();
}

std::string hwo_race::racename() {
	return racename_;
}

int hwo_race::join(hwo_session_ptr session) {
	if (sessions_.size() == maxPlayers_) {
		return 0;
	}

	sessions_.push_back(session);

	return 1;
}

void hwo_race::start() {
	thread_running_ = true;
	race_thread_ = boost::thread(&hwo_race::run, this);

	// setup engine
	engine.reset();
}

void hwo_race::run() {
	// simulation starts

	int tick = 0;
	while (thread_running_) {
		for (auto &s : sessions_) {
			if ( !(s->socket().is_open()) )  continue;
			boost::system::error_code error;
			s->send_response( { make_ping() } );
			auto request = s->receive_request(error);
			if (!error) {
				std::cout << "request received: " << request << std::endl;
			} else {}
		}
		tick++;
	}
}