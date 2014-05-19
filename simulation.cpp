#include "simulation.h"

#include <iostream>
#include <jsoncons/json.hpp>


simulation::simulation() {

}

simulation::~simulation() {

}

int simulation::reset() {
	k1 = 0.2;
	k2 = 0.02;
	A = 0.5303;

	turboDurationTicks = 30;
	turboFactor = 3.0;
	turboAvailableTicks.clear();
	turboAvailableTicks.push_back(600);
	turboAvailableTicks.push_back(1200);
	turboAvailableTicks.push_back(1800);

	std::string trackfileName = "keimola.track";
	std::ifstream trackfile(trackfileName);
	if (!trackfile.is_open()) {
		std::cout << "can't open track file \"" << trackfileName << "\"";
		return 0;
	}
	jsoncons::json trackjson = jsoncons::json::parse(trackfile);
	trackfile.close();


	// set up segments

	return 1;

}