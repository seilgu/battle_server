#ifndef HWO_SIMULATION_H
#define HWO_SIMULATION_H

#include <vector>
#include <map>
#include <string>
#include <cmath>

#include <iostream>
#include <jsoncons/json.hpp>

using namespace std;

class simulation {
public:

	enum segment_type { LINEAR, CURVED };
	
	struct segment {
		segment_type type;
		bool switchable;
		double length;
		double radius;
		double angle;
	};

	struct x_rad {
		double x;
		double rad;
	};
	struct piece_rad {
		std::vector<x_rad> *r;
		int np, nl;
		piece_rad() { r = 0; }
		~piece_rad() { clean(); }
		void clean() {
			if (r) delete[] r;
			r = 0;
		}
		void init(int nump, int numl) {
			clean();
			nl = numl;
			np = nump;
			r = new std::vector<x_rad>[np*nl*nl];
		}
		void pushRad(int p, int sl, int el, x_rad xr) {
			r[p*nl*nl + sl*nl + el].push_back(xr);
		}
		double getRad(int p, int sl, int el, double x) {
			int segsize = r[p*nl*nl + sl*nl + el].size();
			if (segsize == 0) {
				double ssr = r[p*nl*nl + sl*nl + sl][0].rad; 
				double eer = r[p*nl*nl + el*nl + el][0].rad;
				if (ssr > 0) 	return fmin(ssr, eer);
				else			return fmax(ssr, eer);
			}

			double mindist = 1e10;
			int mini = 0;
			for (int i=0; i<segsize; i++) {
				double dist = fabs(x - r[p*nl*nl + sl*nl + el][i].x);
				if (dist < mindist) {
					mindist = dist;
					mini = i;
				}
			}
			return r[p*nl*nl + sl*nl + el][mini].rad;
		}
	};

	struct piece_len {
		double *d;
		int np, nl;
		piece_len() { d = 0; }
		~piece_len() { clean(); }
		void clean() {
			if (d) delete[] d;
			d = 0;
		}
		void init(int nump, int numl) {
			clean();
			nl = numl;
			np = nump;
			d = new double[np*nl*nl];
		}
		void setLen(int p, int sl, int el, double v) {
			d[p*nl*nl + sl*nl + el] = v;
		}
		double getLen(int p, int sl, int el) {
			return d[p*nl*nl + sl*nl + el];
		}
	};


	struct car {
		bool crashing;
		bool onTurbo;
		bool dnf;
		int lastCrashedTick;
		bool finishedRace;
		double turboBeginTick;
		int turboAvailable;

		double length, width;
		int p;
		double x;
		int startLane, endLane;
		double angle;
		bool newlap;
		int laps;

		bool useTurbo;
		int direction;
		double throttle;

		double v, w;

		std::string name;
		std::string color;
	};

	std::vector<segment> pieces;
	piece_len piecelen;
	piece_rad piecerad;

	double k1;
	double k2;
	double A;

	double turboFactor;

	std::vector<int> lanes_dist;
	std::map<std::string, car> cars;

	simulation();
	~simulation();

	int set_track(jsoncons::json& data);
	void update();

	double distToCar(car source, car target);
	static void set_empty_car(car &cc);
	void correct_x(car &ic);
	int find_collison(std::string &srcname, std::string &tgtname);
	void resolve_collisons();
	void update_one_step(car& ic);
};


#endif