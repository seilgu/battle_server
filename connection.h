#ifndef HWO_CONNECTION_H
#define HWO_CONNECTION_H

#include <jsoncons/json.hpp>

using boost::asio::ip::tcp;

jsoncons::json connection::receive_json(tcp::socket &socket, int tick, int millis, boost::system::error_code& error) {
	if ( !socket.is_open() ) return jsoncons::json();

	size_t bytes_transferred;
	error = boost::asio::error::would_block;

	if (millis == 0) {

	} else {

	}
	deadline_.expires_from_now(boost::posix_time::milliseconds(millis));
	deadline_.async_wait(boost::bind(&connection::check_deadline, shared_from_this()));

	boost::asio::async_read_until(socket, request_buf_, "\n", 
			boost::bind(&connection::handle_read, shared_from_this(), _1, _2, &error, &bytes_transferred ));

	do {
		boost::this_thread::sleep( boost::posix_time::milliseconds(0) );
	} while (error == boost::asio::error::would_block);

	if ( error || !socket.is_open() ) {	//std::cout << error.message() << std::endl;
		return jsoncons::json();
	}

	auto buf = request_buf_.data();
	std::string reply(boost::asio::buffers_begin(buf), boost::asio::buffers_begin(buf) + bytes_transferred);
	request_buf_.consume(bytes_transferred);
	return jsoncons::json::parse_string(reply);
}

void connection::send_json(tcp::socket &socket, const std::vector<jsoncons::json>& msgs, int tick, int millis, boost::system::error_code &error) {
	if ( !socket.is_open() ) return;
	
	jsoncons::output_format format;
	format.escape_all_non_ascii(true);

	response_buf_.consume(response_buf_.size());
	std::ostream s(&response_buf_);
	for (const auto& m : msgs) {
		m.to_stream(s, format);
		s << std::endl;
	}
	
	boost::asio::async_write(socket, response_buf_,
			boost::bind(&connection::handle_write, shared_from_this(),
				error,
				boost::asio::placeholders::bytes_transferred));

	if ( error || !socket.is_open() ) { 	//std::cout << error.message() << std::endl;
	}
}

void connection::check_deadline() {
	if (deadline_.expires_at() <= deadline_timer::traits_type::now()) {
		terminate("exceeded time limit");
		deadline_.expires_at(boost::posix_time::pos_infin);
	}
	deadline_.async_wait(boost::bind(&connection::check_deadline, shared_from_this()));
}


void connection::handle_write(const boost::system::error_code& error, size_t bytes_transferred) {
	if (!error) {
	} else {	//std::cout << "handle_write error:" << error.message() << std::endl;
		terminate("handle_write error");
	}
}

void connection::handle_read(const boost::system::error_code& error, size_t bytes_transferred, 
	boost::system::error_code *out_error, 	size_t *out_bytes_transferred) {

	deadline_.expires_at(boost::posix_time::pos_infin);

	if (!error) {
	} else {
		terminate("handle_read error");
	}

	*out_bytes_transferred = bytes_transferred;
	*out_error = error;
}

#endif