#ifndef HWO_CONNECTION_H
#define HWO_CONNECTION_H

#include <string>
#include <iostream>
#include <boost/asio.hpp>
#include <jsoncons/json.hpp>

using boost::asio::ip::tcp;

class session {
public:
	session(boost::asio::io_service& io_service) : socket_(io_service) {
	}

	tcp::socket& socket() {
		return socket_;
	}

	void start() {
		boost::asio::async_write(socket_, boost::asio::buffer(data_),
				boost::bind(&session::handle_write, shared_from_this(),
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred));

		//boost::asio::async_read(socket_, boost::asio::response_buffer(data_, max_length), 
		//	boost::bind(&session::handle_read, shared_from_this(), 
		//	boost::asio::placeholders::error);
	}

private:
	void handle_write(const boost::system::error_code& error, size_t bytes_transferred) {
	}

	void handle_read(const boost::system::error_code& error) {

	}

	tcp::socket socket_;
	enum { max_length = 1024 };
	std::string data_;
};

class hwo_server {
public:
	hwo_server(boost:asio::io_service& io_service) : acceptor_(io_service, tcp::endpoint(tcp::v4(), 8091)) {
		start_accept();
	}
	void start_accpet() {
		boost::shared_ptr<session> new_session(new session(io_service));

		acceptor_.async_accept(new_session->socket(), boost::bind(&hwo_server::handle_accept, this, new_session, boost:asio::placeholders::error));
	}
	void handle_accept(boost::shared_ptr<session> s, const boost::system::error_code& error) {
		if (!error) {
			s->start();
		}

		start_accept();
	}

private:

	tcp::acceptor acceptor_;
	tc::socket socket_;
	session session_;
};

/*class hwo_connection
{
public:
	hwo_connection(const std::string& host, const std::string& port);
	~hwo_connection();
	jsoncons::json receive_response(boost::system::error_code& error);
	void send_requests(const std::vector<jsoncons::json>& msgs);

private:
	boost::asio::io_service io_service;
	tcp::socket socket;
	boost::asio::streambuf response_buf;
};*/

#endif