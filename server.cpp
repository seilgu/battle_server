#include <iostream>
#include <boost/bind.hpp>
#include <boost/asio/placeholders.hpp>

#include "session.h"
#include "server.h"
#include "race.h"

using boost::asio::ip::tcp;

/** hwo_server **/
hwo_server::hwo_server(boost::asio::io_service& io_service, int port) : io_service_(io_service), 
	acceptor_(io_service, tcp::endpoint(tcp::v4(), port)) {
	start_accept();
}
void hwo_server::start_accept() {
	hwo_session_ptr new_session(new hwo_session(io_service_));

	acceptor_.async_accept(new_session->socket(), 
		boost::bind(&hwo_server::handle_accept, this, new_session, 
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

int hwo_race_manager::hwo_session_join(hwo_session_ptr s) {
	mutex_.lock();
	waitlist_.push_back(s);
	mutex_.unlock();
	return 1;
}

hwo_race_ptr hwo_race_manager::query_race(const std::string &password) const {
	for (auto race : racelist_) {
		if (race->get_race_param().password == password)
			return race;
	}
	return nullptr;
}

hwo_race_ptr hwo_race_manager::set_up_race(hwo_session_ptr s) {

	if ( s->wait_for_join() ) {
		hwo_race_ptr qrace = query_race( s->password_ );
		if ( qrace != nullptr ) {
			if (qrace->join(s))		return qrace;
			else					return nullptr;

		} else {
			std::cout << "creating new race" << std::endl;

			hwo_race_ptr nrace( new hwo_race(s) );
			nrace->join( s );
			racelist_.push_back( nrace );
			return nrace;
		}
	} else {
		std::cout << "protocol fail" << std::endl;
		s->terminate("protocol failed");
		return nullptr;
	}

}

std::list< boost::shared_ptr<const hwo_race> > hwo_race_manager::get_racelist() {
	return std::list< boost::shared_ptr<const hwo_race> >(racelist_.begin(), racelist_.end());
}

void hwo_race_manager::run() {
	while (manager_running_) {
		mutex_.lock();
		for (auto s : waitlist_) {
			hwo_race_ptr prace = set_up_race( s );

			if (prace != nullptr && prace->nPlayers() == prace->get_race_param().carCount ) {
				prace->start();
			}

			waitlist_.remove(s);

			break;
		}


		for (auto r : racelist_) {
			if (r->race_finished()) {
				racelist_.remove(r);
				break;
			}
		}
		mutex_.unlock();

		boost::this_thread::sleep( boost::posix_time::milliseconds(10) );
	}
}

hwo_race_manager::hwo_race_manager() {
	waitlist_.clear();
	racelist_.clear();
	manager_running_ = true;
	manager_thread_ = boost::thread(&hwo_race_manager::run, this);
}
hwo_race_manager::~hwo_race_manager() {
	manager_running_ = false;
	manager_thread_.join();
	waitlist_.clear();
	racelist_.clear();
}