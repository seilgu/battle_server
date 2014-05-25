#ifndef HWO_SERVER_H
#define HWO_SERVER_H

#include <list>
#include <boost/asio/ip/tcp.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/shared_ptr.hpp>

class hwo_session;
typedef boost::shared_ptr<hwo_session> hwo_session_ptr;

#include "protocol.h"

using boost::asio::deadline_timer;
using boost::asio::ip::tcp;


class hwo_race;
typedef boost::shared_ptr<hwo_race> hwo_race_ptr;


class hwo_server {
public:
	hwo_server(boost::asio::io_service& io_service, int port);
	void start_accept();
	void handle_accept(hwo_session_ptr s, const boost::system::error_code& error);

private:
	boost::asio::io_service& io_service_;
	tcp::acceptor acceptor_;
};


class hwo_race_manager {	// Singleton
public:
	static hwo_race_manager &Instance();

	int hwo_session_join(hwo_session_ptr s);
	hwo_race_ptr query_race(const std::string &password) const;
	hwo_race_ptr set_up_race(hwo_session_ptr s);
	std::list< boost::shared_ptr<const hwo_race> > get_racelist();

	void run();

	boost::mutex mutex_;

private:
	std::list<hwo_session_ptr> waitlist_;
	std::list<hwo_race_ptr> racelist_;

	boost::thread manager_thread_;
	bool manager_running_;

	hwo_race_manager();
	~hwo_race_manager();
};


#endif