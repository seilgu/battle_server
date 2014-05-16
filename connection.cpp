#include "connection.h"

using boost::asio::ip::tcp;

class hwo_session;
typedef boost::shared_ptr<hwo_session> hwo_session_ptr;

/** hwo_race **/
hwo_race::hwo_race() {
	sessions_.resize(0);
}
hwo_race::~hwo_race() {
	for (auto s : sessions_) {
		s.reset();
	}
}

int hwo_race::join(hwo_session_ptr session) {
	session->start(shared_from_this());
	sessions_.push_back(session);
	return 1;
}


/** hwo_session **/
hwo_session::~hwo_session() {}

void hwo_session::start(hwo_race_ptr race) {
	race_ = race;
	boost::asio::async_read_until(socket_, buffer_, "\n", 
		boost::bind(&hwo_session::handle_read, shared_from_this(), 
			boost::asio::placeholders::error, 
			boost::asio::placeholders::bytes_transferred ));
}

void hwo_session::handle_write(const boost::system::error_code& error, size_t bytes_transferred) {
	if (error) {
		std::cout << "handle_write error:" << error.message() << std::endl;
	}
}

void hwo_session::handle_read(const boost::system::error_code& error, size_t bytes_transferred) {
	if (!error) {
		std::cout << "bytes_transferred:" << bytes_transferred << std::endl;

		std::string s;
		std::istream is(&buffer_);
		is >> s;
		for (auto &c : s) c++;

		buffer_.consume(bytes_transferred);

		std::ostream os(&buffer_);
		os << s;

		boost::asio::async_write(socket_, buffer_,
			boost::bind(&hwo_session::handle_write, shared_from_this(),
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred));

		boost::asio::async_read_until(socket_, buffer_, "\n", 
			boost::bind(&hwo_session::handle_read, shared_from_this(), 
				boost::asio::placeholders::error, 
				boost::asio::placeholders::bytes_transferred ));
	} else {
		std::cout << "handle_read error:" << error.message() << std::endl;
	}
}

/** hwo_server **/
void hwo_server::start_accept() {
	hwo_session_ptr new_session(new hwo_session(io_service_));

	acceptor_.async_accept(new_session->socket(), boost::bind(&hwo_server::handle_accept, this, new_session, 
		boost::asio::placeholders::error));
}
void hwo_server::handle_accept(hwo_session_ptr s, const boost::system::error_code& error) {
	if (!error) {
		// successfully connected
		hwo_race_manager::Instance().hwo_session_join(s);
	}

	start_accept();
}
