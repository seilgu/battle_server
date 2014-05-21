#ifndef HWO_RACE_H
#define HWO_RACE_H

#include "protocol.h"
#include "connection.h"

#include "simulation.h"

#include <map>
#include <functional>

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

	void on_ping(const jsoncons::json& data, const hwo_session_ptr s);
	void on_throttle(const jsoncons::json& data, const hwo_session_ptr s);

private:
	typedef std::function<void(hwo_race*, const jsoncons::json&, const hwo_session_ptr)> action_fun;
	const std::map<std::string, action_fun> action_map;

	std::list<hwo_session_ptr> sessions_;
	std::map<hwo_session_ptr, simulation::car*> carmap;

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