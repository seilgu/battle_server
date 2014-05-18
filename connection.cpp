#include <iostream>
#include <boost/bind.hpp>

#include <boost/asio/write.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/placeholders.hpp>

#include "connection.h"
#include "race.h"

using boost::asio::ip::tcp;

using namespace hwo_protocol;

class hwo_session;
typedef boost::shared_ptr<hwo_session> hwo_session_ptr;


/** hwo_session **/

hwo_session::hwo_session(boost::asio::io_service& io_service)
	: socket_(io_service), deadline_(io_service) {
	response_buf_.prepare(8192);
	request_buf_.prepare(8192);
}
hwo_session::~hwo_session() {
	socket_.close();
}
tcp::socket& hwo_session::socket() {
	return socket_;
}

void hwo_session::terminate(std::string reason) {

	if (socket_.is_open()) {
		std::ostream os(&response_buf_);
		os << reason << std::endl;

		try {
			boost::asio::async_write(socket_, response_buf_, boost::bind(&hwo_session::handle_write, this, 
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

	boost::asio::async_read_until(socket_, request_buf_, "\n", 
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

	std::istream is(&request_buf_);
	return jsoncons::json::parse(is);
}

void hwo_session::send_response(const std::vector<jsoncons::json>& msgs, boost::system::error_code &error) {
	
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

	if ( error || !socket_.is_open() ) {
		std::cout << error.message() << std::endl;
	}
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


/** hwo_manager **/

hwo_race_manager &hwo_race_manager::Instance() {
	static hwo_race_manager instance;
	return instance;
}

int hwo_race_manager::hwo_session_join(hwo_session_ptr hwo_session) {
	mutex_.lock();
	waitlist_.push_back(hwo_session);
	mutex_.unlock();
	return 1;
}

hwo_race_ptr hwo_race_manager::query_race(std::string name) {
	for (auto &race : racelist_) {
		if (race->racename() == name)
			return race;
	}
	return nullptr;
}

hwo_race_ptr hwo_race_manager::set_up_race(hwo_session_ptr s) {

	std::string name, key, racename;
	int maxPlayers;

	if (s->wait_for_join(name, key, racename, maxPlayers)) {

		// if (racename == "") ... 
		hwo_race_ptr qrace = query_race(racename);
		if ( qrace != nullptr ) {
			if ( maxPlayers != qrace->maxPlayers() ) {
				s->terminate("numPlayers does not match");
				return nullptr;
			} else if ( qrace->nPlayers() < qrace->maxPlayers() ) {
				qrace->join(s);
				return qrace;
			} else {	// race already full
				std::cout << "race full" << std::endl;
				s->terminate("race already full");
				return nullptr;
			}
		} else {
			std::cout << "creating new race" << std::endl;
			hwo_race_ptr nrace(new hwo_race(racename, maxPlayers));
			nrace->join(s);
			racelist_.push_back(nrace);
			return nrace;
		}
	} else {
		std::cout << "protocol fail" << std::endl;
		s->terminate("protocol failed");
		return nullptr;
	}

}

void hwo_race_manager::run() {
	while (1) {
		mutex_.lock();
		for (auto &s : waitlist_) {
			hwo_race_ptr prace = set_up_race(s);
			
			if (prace != nullptr && prace->nPlayers() == prace->maxPlayers()) {
				prace->start();
			}

			waitlist_.remove(s);
			break;
		}
		mutex_.unlock();

		boost::this_thread::sleep( boost::posix_time::milliseconds(10) );
	}
}

hwo_race_manager::hwo_race_manager() {
	waitlist_.resize(0);
	racelist_.resize(0);
	manager_thread_ = boost::thread(&hwo_race_manager::run, this);
}
hwo_race_manager::~hwo_race_manager() {
	for (auto &s : waitlist_) 		s.reset();
	for (auto &race : racelist_) 	race.reset();
}