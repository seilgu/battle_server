#ifndef HWO_SESSION_H
#define HWO_SESSION_H

#include <list>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio/deadline_timer.hpp>

#include <iostream>
#include <jsoncons/json.hpp>

using boost::asio::deadline_timer;
using boost::asio::ip::tcp;

class hwo_session : public boost::enable_shared_from_this<hwo_session> {
public:
	hwo_session(boost::asio::io_service& io_service);
	~hwo_session();

	tcp::socket& socket() ;
	int wait_for_join();
	jsoncons::json receive_request(boost::system::error_code& error);
	void send_response(const std::vector<jsoncons::json>& msgs, boost::system::error_code &error);

	void terminate(std::string reason);

	// shouldn't be public
	std::string name_, key_, trackname_, password_;
	int carCount_;

private:
	deadline_timer deadline_;
	tcp::socket socket_;
	boost::asio::streambuf response_buf_;
	boost::asio::streambuf request_buf_;

	void handle_write(const boost::system::error_code& error, size_t bytes_transferred);
	void handle_read(const boost::system::error_code& error, size_t bytes_transferred, 
		boost::system::error_code *out_error, 	size_t *out_bytes_transferred);
	void check_deadline();
};

#endif