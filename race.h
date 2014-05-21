#ifndef HWO_RACE_H
#define HWO_RACE_H

#include "protocol.h"
#include "connection.h"

#include "simulation.h"

struct race_param {
	std::string raceId;
	std::string racename;
	std::string key;

	std::string trackfile;
	int maxPlayers;

	race_param() {}
	race_param(const race_param &p)
		: raceId(p.raceId), racename(p.racename), key(p.key), trackfile(p.trackfile), maxPlayers(p.maxPlayers)
	{}
};

class hwo_race : public boost::enable_shared_from_this<hwo_race> {
public:
	hwo_race(race_param param);
	~hwo_race();

	static std::string getUniqueId(race_param &param);

	void start();
	void run();
	void end();

	int join(hwo_session_ptr session);
	race_param get_race_param() const;
	int nPlayers() const;
	bool race_finished() const;

	void handle_request(jsoncons::json &request, hwo_session_ptr session);

private:
	std::list<hwo_session_ptr> sessions_;
	boost::thread race_thread_;
	bool thread_running_;
	bool race_finished_;

	race_param param_;

	simulation sim_;

	int turboDurationTicks;
	std::vector<int> turboAvailableTicks;

	int tick;
};


#endif