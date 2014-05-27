#include "simulation.h"

#include <iostream>
#include <jsoncons/json.hpp>
#include <algorithm>

#include "util.h"

#define PI 3.1415926535

inline int clamp(int id, int range) {
	if (id >= range) return range-1;
	else if (id < 0) return 0;
	else return id;
}

simulation::simulation() {}
simulation::~simulation() {}

double simulation::distToCar(car source, car target) {
	if (source.p == target.p && 
		source.startLane == target.startLane &&
		source.endLane == target.endLane && 
		target.x >= source.x) return target.x - source.x;

	// find a path through lanes to the target
	double dist = -source.x;
	do {
		dist += piecelen.getLen(source.p, source.startLane, source.endLane);
		source.startLane = source.endLane;
		source.p = (source.p + 1) % pieces.size();
		if (source.endLane != target.startLane && pieces[source.p].switchable == true)
			source.endLane += ( target.startLane > source.endLane ? 1 : -1 );		
	} while (source.startLane != target.startLane || source.p != target.p);
	dist += target.x;

	return dist;
}

void simulation::update_one_step(car& ic) {
	double radius = fabs(piecerad.getRad(ic.p, ic.startLane, ic.endLane, ic.x));
	double alpha = - 0.00125*ic.v*ic.angle- 0.1*ic.w;
	double torque = 0;
	if (pieces[ic.p].type == CURVED && ic.v > (0.3/A)*sqrt(radius)) {
		if (pieces[ic.p].angle > 0) 	torque = A/sqrt(radius)*ic.v*ic.v - 0.3*ic.v;
		else							torque = - A/sqrt(radius)*ic.v*ic.v + 0.3*ic.v;
	}
	alpha += torque;
	ic.w += alpha;
	ic.angle += ic.w;

	double a;
	if (ic.onTurbo) 		a = k1*ic.throttle*turboFactor - k2*ic.v;
	else					a = k1*ic.throttle - k2*ic.v;
	ic.v += a;
	ic.x += ic.v;

	correct_x(ic);
}

int simulation::set_track(jsoncons::json& data) {
	lanes_dist.clear();
	pieces.clear();
	piecelen.clean();
	piecerad.clean();
	
	// building segment data
	const jsoncons::json& pcs = data["pieces"];
	for (int i=0; i<pcs.size(); i++) {
		segment seg;
		if (pcs[i].has_member("length")) {
			seg.type = LINEAR;
			seg.length = pcs[i]["length"].as<double>();
			seg.radius = 1.0/0.0;
		} else {
			seg.type = CURVED;
			seg.angle = pcs[i]["angle"].as<double>();
			seg.radius = pcs[i]["radius"].as<double>();
		}

		seg.switchable = pcs[i].has_member("switch");

		pieces.push_back(seg);
	}

	const jsoncons::json& lns = data["lanes"];
	int nlanes = lns.size();
	for (int j=0; j<nlanes; j++) {
		for (int k=0; k<nlanes; k++) {
			if (lns[k]["index"] == j) {
				lanes_dist.push_back(lns[k]["distanceFromCenter"].as<double>());
				break;
			}
		}
	}

	// piecerad
	piecerad.init(pieces.size(), nlanes);
	for (int i=0; i<pieces.size(); i++) {
		for (int j=0; j<lanes_dist.size(); j++) {
			x_rad xr;
			xr.x = 0;
			xr.rad = (pieces[i].radius + lanes_dist[j]*(pieces[i].angle > 0 ? -1 : 1))
				* (pieces[i].angle > 0 ? 1 : -1); // signed radius
			piecerad.pushRad(i, j, j, xr);	// dummy radius for same lane
			// different lane gives undefined radius in interpolation
		}
	}

	// piecelen
	piecelen.init(pieces.size(), nlanes);
	for (int i=0; i<pieces.size(); i++) {
		for (int j=0; j<lanes_dist.size(); j++) {
			double distj = ( pieces[i].type == LINEAR ? pieces[i].length : 
				( pieces[i].angle*PI/180.0*piecerad.getRad(i, j, j, 0) ) );	// we already have same-lane radius
			piecelen.setLen(i, j, j, distj);	// same lane curved length

			if (pieces[i].switchable == false) continue;

			if (j+1 < lanes_dist.size()) {
				double distjplus = ( pieces[i].type == LINEAR ? pieces[i].length : 
					( pieces[i].angle*PI/180.0*piecerad.getRad(i, j+1, j+1, 0) ) );
				piecelen.setLen(i, j, j+1, fmax(distj, distjplus)+3);	// different lane using max of two lanes
				piecelen.setLen(i, j+1, j, fmax(distj, distjplus)+3);
			}
			if (j-1 >= 0) {
				double distjminus = ( pieces[i].type == LINEAR ? pieces[i].length : 
					( pieces[i].angle*PI/180.0*piecerad.getRad(i, j-1, j-1, 0) ) );
				piecelen.setLen(i, j, j-1, fmax(distj, distjminus)+3);
				piecelen.setLen(i, j-1, j, fmax(distj, distjminus)+3);
			}
		}
	}

	return 1;

}

void simulation::set_empty_car(car &cc) {
	cc.crashing = false;
	cc.dnf = false;
	cc.onTurbo = false;
	cc.lastCrashedTick = -1000;
	cc.finishedRace = false;
	cc.turboBeginTick = -1000;
	cc.turboAvailable = 0;
	cc.length = 40.0;
	cc.width = 20.0;
	cc.p = 0;
	cc.x = 0.0;
	cc.startLane = 0;
	cc.endLane = 0;
	cc.angle = 0.0;
	cc.newlap = false;
	cc.laps = 0;
	cc.useTurbo = false;
	cc.direction = 0;
	cc.throttle = 0.0;
	cc.v = 0.0;
	cc.w = 0.0;
	cc.direction = 0;
	cc.dnf = false;
}

void simulation::correct_x(car &ic) {
	double lastDist = piecelen.getLen(ic.p, ic.startLane, ic.endLane);
	if (ic.x > lastDist) {
		ic.x -= lastDist;
		ic.p = (ic.p + 1) % pieces.size();
		
		if (ic.p == 0) {
			ic.newlap = true;
			ic.laps++;
		}

		ic.startLane = ic.endLane;
		if (pieces[ic.p].switchable == true) {
			ic.endLane = clamp(ic.startLane + ic.direction, lanes_dist.size());	
			ic.direction = 0;
		}
	}
}

int simulation::find_collison(std::string &srcname, std::string &tgtname) {
	for (auto &sc : cars) {
		car &src = sc.second;
		if (src.crashing == false && src.finishedRace == false && src.dnf == false) continue;
		for (auto &tc : cars) {
			car &tgt = tc.second;
			if (tgt.crashing == false && tgt.finishedRace == false && tgt.dnf == false) continue;
			if (src.name == tgt.name) continue;

			double dist = distToCar(src, tgt);
			if (dist < 0.25*(src.length + tgt.length)) {
				srcname = src.name;
				tgtname = tgt.name;
				return 1;
			}
		}
	}
	return 0;
}

void simulation::resolve_collisons() {
	std::string srcname, tgtname;
	while ( find_collison(srcname, tgtname) ) {
		car &source = cars[srcname];
		car &target = cars[tgtname];
		target.x = source.x + 0.25*(source.length + target.length) + 0.000000001;
		correct_x(target);
		std::swap(source.v, target.v);
	}
}

void simulation::update() {
	for (auto &pc : cars) {
		car &cc = pc.second;
		if (cc.crashing == false && cc.finishedRace == false && cc.dnf == false) {
			update_one_step(cc);
		}
	}

	resolve_collisons();
}