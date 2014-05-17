#ifndef HWO_CONNECTION_H
#define HWO_CONNECTION_H

#include <string>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/assert.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/thread.hpp>
#include <utility>
#include <list>
#include <jsoncons/json.hpp>

#include "protocol.h"

using boost::asio::deadline_timer;
using boost::asio::ip::tcp;

class hwo_session;
typedef boost::shared_ptr<hwo_session> hwo_session_ptr;

class hwo_race : public boost::enable_shared_from_this<hwo_race> {
public:
	hwo_race(std::string racename, int maxPlayers);
	~hwo_race();

	void run();

	int join(hwo_session_ptr session);
	std::string racename();
	int nPlayers();
	int maxPlayers();

private:
	std::list<hwo_session_ptr> sessions_;

	std::string racename_;
	int nPlayers_;
	int maxPlayers_;
};

typedef boost::shared_ptr<hwo_race> hwo_race_ptr;

class hwo_session : public boost::enable_shared_from_this<hwo_session> {
public:
	hwo_session(boost::asio::io_service& io_service);
	~hwo_session();

	tcp::socket& socket() {
		return socket_;
	}

	int wait_for_join(std::string& name, std::string& key, std::string &racename, int &maxPlayers);
	void run();

	jsoncons::json receive_request(boost::system::error_code& error);
	void send_response(const std::vector<jsoncons::json>& msgs);

	void terminate(std::string reason);

private:
	hwo_race_ptr race_;
	deadline_timer deadline_;

	void handle_write(const boost::system::error_code& error, size_t bytes_transferred);
	void handle_read(const boost::system::error_code& error, size_t bytes_transferred, 
		boost::system::error_code* out_error, size_t* out_bytes_transferred);

	void check_deadline();

	tcp::socket socket_;
	boost::asio::streambuf buffer_;
};

class hwo_race_manager {	// Singleton
public:
	static hwo_race_manager &Instance() {
		static hwo_race_manager instance;
		return instance;
	}

	int hwo_session_join(hwo_session_ptr hwo_session) {
		mutex_.lock();
		waitlist_.push_back(hwo_session);
		mutex_.unlock();
		return 1;
	}

	hwo_race_ptr query_race(std::string name) {
		for (auto &race : racelist_) {
			if (race->racename() == name)
				return race;
		}
		return nullptr;
	}

	void run() {
		while (1) {
			mutex_.lock();
			for (auto &s : waitlist_) {

				std::string name, key, racename;
				int maxPlayers;

				if (s->wait_for_join(name, key, racename, maxPlayers)) {
					// if (racename == "") ...
					hwo_race_ptr qrace(query_race(racename));
					if ( qrace != nullptr ) {
						if ( maxPlayers != qrace->maxPlayers() ) {
							std::cout << "numPlayers incorrect" << std::endl;
							s->terminate("numPlayers does not match");
							waitlist_.remove(s);
						} else if ( qrace->nPlayers() < qrace->maxPlayers() ) {
							qrace->join(s);
							waitlist_.remove(s);
						} else {	// race already full
							std::cout << "race full" << std::endl;
							s->terminate("race already full");
							waitlist_.remove(s);
						}
					} else {
						std::cout << "creating new race" << std::endl;
						hwo_race_ptr nrace(new hwo_race(racename, maxPlayers));
						nrace->join(s);
						racelist_.push_back(nrace);
						waitlist_.remove(s);
					}
				} else {
					std::cout << "protocol fail" << std::endl;
					s->terminate("protocol failed");
					waitlist_.remove(s);
				}

				break;
			}
			mutex_.unlock();

			boost::this_thread::sleep( boost::posix_time::milliseconds(10) );
		}
	}

private:
	std::list<hwo_session_ptr> waitlist_;
	std::list<hwo_race_ptr> racelist_;

	boost::thread manager_thread_;
	boost::mutex mutex_;

	hwo_race_manager() {
		waitlist_.resize(0);
		racelist_.resize(0);
		manager_thread_ = boost::thread(&hwo_race_manager::run, this);
	}
	~hwo_race_manager() {
		for (auto &s : waitlist_) 		s.reset();
		for (auto &race : racelist_) 	race.reset();
	}
};

class hwo_server {
public:
	hwo_server(boost::asio::io_service& io_service, int port);
	void start_accept();
	void handle_accept(hwo_session_ptr s, const boost::system::error_code& error);

private:
	boost::asio::io_service& io_service_;
	tcp::acceptor acceptor_;
};


#endif