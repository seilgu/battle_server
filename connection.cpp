#include "connection.h"

using boost::asio::ip::tcp;

using namespace hwo_protocol;

class hwo_session;
typedef boost::shared_ptr<hwo_session> hwo_session_ptr;

/** hwo_race **/
hwo_race::hwo_race(std::string racename, int maxPlayers) 
	: racename_(racename), maxPlayers_(maxPlayers) {
	BOOST_ASSERT(maxPlayers > 0);
	sessions_.resize(0);
	thread_running_ = false;
}
hwo_race::~hwo_race() {
	thread_running_ = false;
	race_thread_.join();
	for (auto &s : sessions_) {
		s.reset();
	}
}

int hwo_race::maxPlayers() {
	return maxPlayers_;
}

int hwo_race::nPlayers() {
	return sessions_.size();
}

std::string hwo_race::racename() {
	return racename_;
}

int hwo_race::join(hwo_session_ptr session) {
	if (sessions_.size() == maxPlayers_) {
		return 0;
	}

	sessions_.push_back(session);

	return 1;
}

void hwo_race::start() {
	thread_running_ = true;
	race_thread_ = boost::thread(&hwo_race::run, this);
}

void hwo_race::run() {
	int tick = 0;
	while (thread_running_) {
		for (auto &s : sessions_) {
			boost::system::error_code error;
			std::cout << "A" << std::endl;
			s->send_response( { make_ping() } );
			std::cout << "B" << std::endl;
			auto request = s->receive_request(error);
			std::cout << "C" << std::endl;
			if (!error)
				std::cout << "request received: " << request << std::endl;
			else {

			}
		}
		tick++;
	}
}

/** hwo_session **/

hwo_session::hwo_session(boost::asio::io_service& io_service)
	: socket_(io_service), deadline_(io_service) {
	buffer_.prepare(8192);
}
hwo_session::~hwo_session() {
	socket_.close();
}

void hwo_session::terminate(std::string reason) {

	if (socket_.is_open()) {
		std::ostream os(&buffer_);
		os << reason << std::endl;

		try {
			boost::asio::async_write(socket_, buffer_, boost::bind(&hwo_session::handle_write, this, 
				boost::asio::placeholders::error, 
				boost::asio::placeholders::bytes_transferred ));
		} catch (std::exception &e) {
		}
	}

	socket_.close();
}

jsoncons::json hwo_session::receive_request(boost::system::error_code& error) {
	size_t bytes_transferred;
	error = boost::asio::error::would_block;

	boost::asio::async_read_until(socket_, buffer_, "\n", 
			boost::bind(&hwo_session::handle_read, shared_from_this(), _1, _2, &error, &bytes_transferred ));

	deadline_.expires_from_now(boost::posix_time::seconds(2));
	deadline_.async_wait(boost::bind(&hwo_session::check_deadline, shared_from_this()));

	do {
		boost::this_thread::sleep( boost::posix_time::milliseconds(0) );
	} while (error == boost::asio::error::would_block);

	if ( error || !socket_.is_open() ) {
		std::cout << error.message() << std::endl;
		return jsoncons::json();
	}

	std::istream is(&buffer_);
	return jsoncons::json::parse(is);
}

void hwo_session::send_response(const std::vector<jsoncons::json>& msgs) {
	
	jsoncons::output_format format;
	format.escape_all_non_ascii(true);
	boost::asio::streambuf response_buf;

	std::ostream s(&response_buf);
	for (const auto& m : msgs) {
		m.to_stream(s, format);
		s << std::endl;
	}
	
	boost::asio::async_write(socket_, response_buf,
			boost::bind(&hwo_session::handle_write, shared_from_this(),
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred));

}

void hwo_session::check_deadline() {
	if (deadline_.expires_at() <= deadline_timer::traits_type::now()) {
		terminate("exceeded 2 seconds");
		deadline_.expires_at(boost::posix_time::pos_infin);
	}
	deadline_.async_wait(boost::bind(&hwo_session::check_deadline, shared_from_this()));
}

int hwo_session::wait_for_join(std::string& name, std::string& key, std::string &racename, int &maxPlayers) {
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

		if (botId.has_member("name"))		name = botId["name"].as<std::string>();
		else {
			terminate("no name field"); return 0;
		}

		if (botId.has_member("key"))		key = botId["key"].as<std::string>();
		else {
			terminate("no key specified"); return 0;
		}

		if (data.has_member("raceName")) 	racename = data["raceName"].as<std::string>();
		else								racename = "";

		if (data.has_member("numPlayers"))  maxPlayers = data["numPlayers"].as<int>();
		else								maxPlayers = 1;

		if (maxPlayers <= 0) std::cout << "nPlayers <= 0 !" << std::endl;

		return 1;

	} else {
		terminate("expected joinRace");
		return 0;
	}
}

void hwo_session::run() {

}

void hwo_session::handle_write(const boost::system::error_code& error, size_t bytes_transferred) {
	if (!error) {
	} else {
		std::cout << "handle_write error:" << error.message() << std::endl;
		terminate("handle_write error");
	}
}

void hwo_session::handle_read(const boost::system::error_code& error, size_t bytes_transferred, 
	boost::system::error_code *out_error, 	size_t *out_bytes_transferred) {

	deadline_.expires_at(boost::posix_time::pos_infin);

	if (!error) {
	} else {
		std::cout << "handle_read error:" << error.message() << std::endl;
		terminate("handle_read error");
	}

	
	*out_bytes_transferred = bytes_transferred;
	*out_error = error;

}

/** hwo_server **/
hwo_server::hwo_server(boost::asio::io_service& io_service, int port) : io_service_(io_service), 
	acceptor_(io_service, tcp::endpoint(tcp::v4(), port)) {
	start_accept();
}
void hwo_server::start_accept() {
	hwo_session_ptr new_session(new hwo_session(io_service_));

	acceptor_.async_accept(new_session->socket(), boost::bind(&hwo_server::handle_accept, this, new_session, 
		boost::asio::placeholders::error));
}
void hwo_server::handle_accept(hwo_session_ptr s, const boost::system::error_code& error) {
	if (!error) {		// successfully connected
		hwo_race_manager::Instance().hwo_session_join(s);
	}

	start_accept();
}
