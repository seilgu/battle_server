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

class hwo_race;
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
	void send_response(const std::vector<jsoncons::json>& msgs, boost::system::error_code &error);

	void terminate(std::string reason);

private:
	deadline_timer deadline_;

	void handle_write(const boost::system::error_code& error, size_t bytes_transferred);
	//void timed_read(boost::asio::streambuf &buffer, const boost::system::error_code& error, 
	//	size_t bytes_transferred);
	void handle_read(const boost::system::error_code& error, size_t bytes_transferred, 
		boost::system::error_code *out_error, 	size_t *out_bytes_transferred);

	void check_deadline();

	tcp::socket socket_;
	boost::asio::streambuf response_buf_;
	boost::asio::streambuf request_buf_;
};

class hwo_race_manager {	// Singleton
public:
	static hwo_race_manager &Instance();

	int hwo_session_join(hwo_session_ptr hwo_session);

	hwo_race_ptr query_race(std::string name);

	hwo_race_ptr set_up_race(hwo_session_ptr s);

	void run();

private:
	std::list<hwo_session_ptr> waitlist_;
	std::list<hwo_race_ptr> racelist_;

	boost::thread manager_thread_;
	boost::mutex mutex_;

	hwo_race_manager();
	~hwo_race_manager();
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