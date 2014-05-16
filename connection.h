#ifndef HWO_CONNECTION_H
#define HWO_CONNECTION_H

#include <string>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/array.hpp>
#include <utility>
#include <list>
#include <jsoncons/json.hpp>

using boost::asio::ip::tcp;

class hwo_session;
typedef boost::shared_ptr<hwo_session> hwo_session_ptr;

class hwo_race : public boost::enable_shared_from_this<hwo_race> {
public:
	hwo_race();
	~hwo_race();

	int join(hwo_session_ptr session);

private:
	std::list<hwo_session_ptr> sessions_;
};

typedef boost::shared_ptr<hwo_race> hwo_race_ptr;

class hwo_race_manager {
public:
	static hwo_race_manager &Instance() {
		static hwo_race_manager instance;
		return instance;
	}

	hwo_race_ptr hwo_session_join(hwo_session_ptr hwo_session) {
		hwo_race_ptr new_race(new hwo_race);
		new_race->join(hwo_session);
		racelist_.push_back(new_race);
		return new_race;
	}

private:

	std::list<hwo_race_ptr> racelist_;

	hwo_race_manager() {
		racelist_.resize(0);
	}
	~hwo_race_manager() {
		for (auto race : racelist_) {
			race.reset();
		}
	}
};

class hwo_session : public boost::enable_shared_from_this<hwo_session> {
public:
	hwo_session(boost::asio::io_service& io_service) : socket_(io_service) {}
	~hwo_session();

	tcp::socket& socket() {
		return socket_;
	}

	void start(hwo_race_ptr race);

private:
	hwo_race_ptr race_;

	void handle_write(const boost::system::error_code& error, size_t bytes_transferred);
	void handle_read(const boost::system::error_code& error, size_t bytes_transferred);

	tcp::socket socket_;
	boost::asio::streambuf buffer_;
};

class hwo_server {
public:
	hwo_server(boost::asio::io_service& io_service, int port) : io_service_(io_service), 
		acceptor_(io_service, tcp::endpoint(tcp::v4(), port)) {
		start_accept();
	}
	void start_accept();
	void handle_accept(hwo_session_ptr s, const boost::system::error_code& error);

private:
	boost::asio::io_service& io_service_;
	tcp::acceptor acceptor_;
};


#endif