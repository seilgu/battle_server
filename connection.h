#ifndef HWO_CONNECTION_H
#define HWO_CONNECTION_H

#include <string>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <utility>
#include <jsoncons/json.hpp>

using boost::asio::ip::tcp;

class session : public boost::enable_shared_from_this<session>{
public:
	session(boost::asio::io_service& io_service) : socket_(io_service) {
	}

	tcp::socket& socket() {
		return socket_;
	}

	void start() {
		//data_ = "hello world!\n";
		//boost::system::error_code ignored_error;
		//boost::asio::write(socket_, boost::asio::buffer(data_), boost::asio::transfer_all(), ignored_error);
		//boost::asio::read(socket_, boost::asio::buffer(data_, max_length));
		boost::asio::async_read(socket_, boost::asio::buffer(buffer_, max_length), 
			boost::bind(&session::handle_read, shared_from_this(), 
				boost::asio::placeholders::error, 
				boost::asio::placeholders::bytes_transferred ));

		//boost::asio::async_write(socket_, boost::asio::buffer(data_),
		//		boost::bind(&session::handle_write, shared_from_this(),
		//		boost::asio::placeholders::error,
		//		boost::asio::placeholders::bytes_transferred));
	}

private:
	void handle_write(const boost::system::error_code& error, size_t bytes_transferred) {
		if (error) {
			std::cout << "handle_write error:" << error << std::endl;
		}
	}

	void handle_read(const boost::system::error_code& error, size_t bytes_transferred) {
		if (!error) {
			std::cout << "bytes_transferred:" << bytes_transferred << std::endl;
			for (int i=0; i<bytes_transferred; i++) {
				buffer_[i]++;
			}

			boost::asio::async_write(socket_, boost::asio::buffer(buffer_, bytes_transferred),
				boost::bind(&session::handle_write, shared_from_this(),
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred));

			boost::asio::async_read(socket_, boost::asio::buffer(buffer_, max_length), 
				boost::bind(&session::handle_read, shared_from_this(), 
					boost::asio::placeholders::error, 
					boost::asio::placeholders::bytes_transferred ));
		} else {
			std::cout << "handle_read error:" << error << std::endl;
		}
	}

	tcp::socket socket_;
	enum { max_length = 1024 };
	//std::string data_;
	char buffer_[max_length];
};

class hwo_server {
public:
	hwo_server(boost::asio::io_service& io_service, int port) : io_service_(io_service), acceptor_(io_service, tcp::endpoint(tcp::v4(), port)) {
		start_accept();
	}
	void start_accept() {
		boost::shared_ptr<session> new_session(new session(io_service_));

		acceptor_.async_accept(new_session->socket(), boost::bind(&hwo_server::handle_accept, this, new_session, boost::asio::placeholders::error));
	}
	void handle_accept(boost::shared_ptr<session> s, const boost::system::error_code& error) {
		if (!error) {
			s->start();
		}

		start_accept();
	}

private:
	boost::asio::io_service& io_service_;
	tcp::acceptor acceptor_;
};


#endif