#ifndef HWO_RACE_H
#define HWO_RACE_H

#include "protocol.h"
#include "server.h"

#include "simulation.h"

#include <map>
#include <functional>

struct race_param {
	// setup at creation
	std::string raceId;
	std::string trackname;
	std::string password;
	int carCount;

	// setup after
	std::vector<int> turboAvailableTicks;
	int turboDurationTicks;
	int crashDurationTicks;
	int laps;

	race_param() {}
};

class hwo_race : public boost::enable_shared_from_this<hwo_race> {
public:
	hwo_race( hwo_session_ptr &s );
	~hwo_race();

	static std::string getUniqueId(race_param &param);	// todo : make it private

	void start();
	void run();
	void end();

	int add_session(hwo_session_ptr session);
	race_param get_race_param() const;
	int nPlayers() const;
	bool race_finished() const;

	int load_track( std::string trackname, jsoncons::json &js );
	void handle_request(jsoncons::json &request, hwo_session_ptr session);

	void on_ping(const jsoncons::json& data, const hwo_session_ptr s);
	void on_throttle(const jsoncons::json& data, const hwo_session_ptr s);
	void on_switch_lane(const jsoncons::json& data, const hwo_session_ptr s);
	void on_turbo(const jsoncons::json& data, const hwo_session_ptr s);

private:
	typedef std::function<void(hwo_race*, const jsoncons::json&, const hwo_session_ptr)> action_fun;
	const std::map<std::string, action_fun> action_map;

	std::list<hwo_session_ptr> sessions_;

	boost::thread race_thread_;
	bool thread_running_;
	bool race_finished_;

	race_param param_;

	simulation sim_;

	int tick;

	std::ofstream racelog_;
};


#endif