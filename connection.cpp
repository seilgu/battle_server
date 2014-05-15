#include "connection.h"

hwo_connection::hwo_connection(boost:asio::io_service& io_service, const tcp::endpoint& endpoint)
	: acceptor_(io_service, endpoint), socket(io_service)
{
	do_accept();

	//response_buf.prepare(8192);
}

hwo_connection::do_accept() {
	acceptor_.async_accept(socket_, [this](boost::system::error_code ec) {
		if (!ec) {
			std::make_shared<session>(std::move(socket_), room_)->start();
		}

		do_accept();
	})
}

hwo_connection::~hwo_connection()
{
	socket.close();
}

jsoncons::json hwo_connection::receive_response(boost::system::error_code& error)
{
	boost::asio::read_until(socket, response_buf, "\n", error);
	if (error)
	{
		return jsoncons::json();
	}
	std::istream s(&response_buf);
	return jsoncons::json::parse(s);
}

void hwo_connection::send_requests(const std::vector<jsoncons::json>& msgs)
{
	jsoncons::output_format format;
	format.escape_all_non_ascii(true);
	boost::asio::streambuf request_buf;
	std::ostream s(&request_buf);
	for (const auto& m : msgs) {
		m.to_stream(s, format);
		s << std::endl;
	}
	socket.send(request_buf.data());
}
