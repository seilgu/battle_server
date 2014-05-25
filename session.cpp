#include "session.h"

#include <boost/bind.hpp>

#include <boost/asio/write.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/thread/thread.hpp>

using boost::asio::ip::tcp;

/** hwo_session **/
hwo_session::hwo_session(boost::asio::io_service& io_service)
	: socket_(io_service), deadline_(io_service) {
	response_buf_.prepare(8192);
	request_buf_.prepare(8192);
}
hwo_session::~hwo_session() {
	std::cout << "destro" << std::endl;
	socket_.close();
}
tcp::socket& hwo_session::socket() {
	return socket_;
}

void hwo_session::terminate(std::string reason) {

	if (socket_.is_open()) {
		std::cout << name_ << " terminate reason : " << reason << std::endl;
		std::ostream os(&response_buf_);
		os << reason << std::endl;

		try {
			boost::asio::async_write(socket_, response_buf_, boost::bind(&hwo_session::handle_write, shared_from_this(), 
				boost::asio::placeholders::error, 
				boost::asio::placeholders::bytes_transferred ));
		} catch (std::exception &e) {
		}
	}

	socket_.close();
}

jsoncons::json hwo_session::receive_request(boost::system::error_code& error) {
	if ( !socket_.is_open() ) return jsoncons::json();

	size_t bytes_transferred;
	error = boost::asio::error::would_block;

	deadline_.expires_from_now(boost::posix_time::seconds(2));
	deadline_.async_wait(boost::bind(&hwo_session::check_deadline, shared_from_this()));

	boost::asio::async_read_until(socket_, request_buf_, "\n", 
			boost::bind(&hwo_session::handle_read, shared_from_this(), _1, _2, &error, &bytes_transferred ));

	do {
		boost::this_thread::sleep( boost::posix_time::milliseconds(0) );
	} while (error == boost::asio::error::would_block);

	if ( error || !socket_.is_open() ) {	//std::cout << error.message() << std::endl;
		return jsoncons::json();
	}

	auto buf = request_buf_.data();
	std::string reply(boost::asio::buffers_begin(buf), boost::asio::buffers_begin(buf) + bytes_transferred);
	request_buf_.consume(bytes_transferred);
	return jsoncons::json::parse_string(reply);
}

void hwo_session::send_response(const std::vector<jsoncons::json>& msgs, boost::system::error_code &error) {
	if ( !socket_.is_open() ) return;
	
	jsoncons::output_format format;
	format.escape_all_non_ascii(true);

	response_buf_.consume(response_buf_.size());
	std::ostream s(&response_buf_);
	for (const auto& m : msgs) {
		m.to_stream(s, format);
		s << std::endl;
	}
	
	boost::asio::async_write(socket_, response_buf_,
			boost::bind(&hwo_session::handle_write, shared_from_this(),
				error,
				boost::asio::placeholders::bytes_transferred));

	if ( error || !socket_.is_open() ) { 	//std::cout << error.message() << std::endl;
	}
}

void hwo_session::check_deadline() {
	if (deadline_.expires_at() <= deadline_timer::traits_type::now()) {
		terminate("exceeded 2 seconds");
		deadline_.expires_at(boost::posix_time::pos_infin);
	}
	deadline_.async_wait(boost::bind(&hwo_session::check_deadline, shared_from_this()));
}

int hwo_session::wait_for_join() {
	boost::system::error_code error;
	jsoncons::json request = receive_request(error);

	if (!error) {
		std::cout << "wait_for_join received: " << request << std::endl;
	}

	if (request.has_member("msgType") && request["msgType"].as<std::string>() == "joinRace") {
		if (!request.has_member("data") || !request["data"].has_member("botId") ) {
			terminate("no data/botId fields");
			return 0;
		}
		jsoncons::json &data = request["data"];
		jsoncons::json &botId = data["botId"];

		if (botId.has_member("name"))		name_ = botId["name"].as<std::string>();
		else 								{ terminate("no name field"); return 0; }

		if (botId.has_member("key"))		key_ = botId["key"].as<std::string>();
		else								{ terminate("no key specified"); return 0; }

		if (data.has_member("trackName")) 	trackname_ = data["trackName"].as<std::string>();
		else								trackname_ = "keimola";

		if (data.has_member("password")) 	password_ = data["password"].as<std::string>();
		else								password_ = "";

		if (data.has_member("carCount"))  	carCount_ = data["carCount"].as<int>();
		else								carCount_ = 1;

		if (carCount_ <= 0) {
			std::cout << "carCount <= 0 !" << std::endl;
			return 0;
		}

		return 1;

	} else {
		terminate("expected joinRace");
		return 0;
	}
}

void hwo_session::handle_write(const boost::system::error_code& error, size_t bytes_transferred) {
	if (!error) {
	} else {	//std::cout << "handle_write error:" << error.message() << std::endl;
		terminate("handle_write error");
	}
}

void hwo_session::handle_read(const boost::system::error_code& error, size_t bytes_transferred, 
	boost::system::error_code *out_error, 	size_t *out_bytes_transferred) {

	deadline_.expires_at(boost::posix_time::pos_infin);

	if (!error) {
	} else {	//std::cout << "handle_read error:" << error.message() << std::endl;
		terminate("handle_read error");
	}

	*out_bytes_transferred = bytes_transferred;
	*out_error = error;
}