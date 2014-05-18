#ifndef HWO_RACE_H
#define HWO_RACE_H

#include "protocol.h"
#include "connection.h"

#include "simulation.h"

class hwo_race : public boost::enable_shared_from_this<hwo_race> {
public:
	hwo_race(std::string racename, int maxPlayers);
	~hwo_race();

	void start();
	void run();

	int join(hwo_session_ptr session);
	std::string racename();
	int nPlayers();
	int maxPlayers();

private:
	std::list<hwo_session_ptr> sessions_;
	boost::thread race_thread_;
	bool thread_running_;

	std::string racename_;
	int maxPlayers_;

	simulation engine;
};


#endif