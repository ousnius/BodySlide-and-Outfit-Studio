#pragma once
#include <vector> 
#include <map>
#include <float.h>
#include "Object3d.h"
#include <algorithm>

using namespace std;

// A specialized KD tree that finds duplicate vertices in a point cloud.  

class kd_matcher {
public:
	class kd_node{
	public:
		Vertex* p;
		kd_node* less;
		kd_node* more;
		kd_node() {
			less = more = nullptr;
			p = nullptr;
		}
		~kd_node() {
			if (less)
				delete less;
			if (more)
				delete more;
			less = more = nullptr;

		}
		kd_node(Vertex* point) {
			less = more = nullptr;
			p = point;
		}
		Vertex* add(Vertex* point, int depth) {
			int axis = depth % 3;
			bool domore = false;
			float dx = p->x - point->x;
			float dy = p->y - point->y;
			float dz = p->z - point->z;
			if (fabs(dx) < .00001 && fabs(dy) < .00001 && fabs(dz) < .00001)
				return p;
			switch (axis) {
			case 0:
				if (dx > 0) domore = true;
				break;
			case 1:
				if (dy > 0) domore = true;
				break;
			case 2:
				if (dz > 0) domore = true;
				break;
			}
			if (domore) {
				if (more) return more->add(point, depth + 1);
				else more = new kd_node(point);
			}
			else {
				if (less) return less->add(point, depth + 1);
				else less = new kd_node(point);

			}
			return 0;
		}
	};

	kd_node* root;
	Vertex* points;
	int count;
	vector<pair<Vertex*, Vertex*>> matches;

	~kd_matcher() {
		if (root)
			delete root;
		root = nullptr;
	}
	kd_matcher(Vertex* points, int count) {
		if (count <= 0)
			return;

		Vertex* pong;
		root = new kd_node(&points[0]);
		for (int i = 1; i < count; i++) {
			pong = root->add(&points[i], 0);
			if (pong)
				matches.push_back(pair<Vertex*, Vertex*>(&points[i], pong));
		}
	}
};

class kd_query_result {
public:
	Vertex* v;
	ushort vertex_index;
	float distance;
	bool operator < (const kd_query_result& other) const {
		return distance < other.distance;
	}
	//static bool distLess(kd_query_result r1, kd_query_result r2) { return r1.distance < r2.distance; }
};

// More general purpose KD tree that assembles a tree from input points and allows nearest neighbor and radius searches on the data.
class kd_tree {
public:
	class kd_node{
	public:
		Vertex* p;
		int p_i;
		kd_node* less;
		kd_node* more;
		kd_node() {
			less = more = nullptr;
			p = nullptr;
			p_i = -1;
		}
		~kd_node() {
			if (less)
				delete less;
			if (more)
				delete more;
			less = more = nullptr;

		}
		kd_node(Vertex* point, int point_index) {
			less = more = nullptr;
			p = point;
			p_i = point_index;
		}
		void add(Vertex* point, int point_index, int depth) {
			int axis = depth % 3;
			bool domore = false;
			float dx = p->x - point->x;
			float dy = p->y - point->y;
			float dz = p->z - point->z;

			switch (axis) {
			case 0:
				if (dx > 0) domore = true;
				break;
			case 1:
				if (dy > 0) domore = true;
				break;
			case 2:
				if (dz > 0) domore = true;
				break;
			}
			if (domore) {
				if (more) return more->add(point, point_index, depth + 1);
				else more = new kd_node(point, point_index);
			}
			else {
				if (less) return less->add(point, point_index, depth + 1);
				else less = new kd_node(point, point_index);

			}
		}

		// finds the closest point(s) to querypoint within the provided radius.  If radius is 0, only the single closest point 
		// is found.  On first call, mindist should be set to FLT_MAX, and depth set to 0.
		void find_closest(Vertex* querypoint, vector<kd_query_result>& queryResult, float radius, float& mindist, int depth = 0) {
			kd_query_result kdqr;
			int axis = depth % 3;		// which separating axis to use -- based on depth
			float dx = p->x - querypoint->x; // axis sides
			float dy = p->y - querypoint->y;
			float dz = p->z - querypoint->z;
			kd_node* act = less;			// active search branch 
			kd_node* opp = more;			// opposite search branch
			float axisdist = 0;			// distance from the query point to the separating axis
			float pointdist;			// distance from the query point to the node's point

			switch (axis) {
			case 0:
				if (dx > 0)  {
					act = more;
					opp = less;
				}
				axisdist = fabs(dx);
				break;
			case 1:
				if (dy > 0) {
					act = more;
					opp = less;
				}
				axisdist = fabs(dy);
				break;
			case 2:
				if (dz > 0)  {
					act = more;
					opp = less;
				}
				axisdist = fabs(dz);
				break;
			}
			if (act) // the axis choice tells us which branch to search
				act->find_closest(querypoint, queryResult, radius, mindist, depth + 1);

			// on the way back out check current point to see if it's the closest
			pointdist = querypoint->DistanceTo(p);		// Fix? might want to use squared distance instead... probably unnecessary. 
			/*bool notopp = ((querypoint->nx*p->nx +
						querypoint->ny*p->ny +
						querypoint->nz*p->nz) > 0);*/
			bool notopp = true;
			if (pointdist <= mindist && notopp) {
				kdqr.v = p;
				kdqr.vertex_index = p_i;
				kdqr.distance = pointdist;
				queryResult.push_back(kdqr);
				mindist = pointdist;
			}
			else if (radius > mindist && notopp) {  // if there's room between the minimum distance and the search radius
				if (pointdist <= radius) {  // check to see if the point falls in that space, and if so add it
					kdqr.v = p;
					kdqr.vertex_index = p_i;
					kdqr.distance = pointdist;
					queryResult.push_back(kdqr); // (This is skipped if radius is 0, ofc)
				}
			}

			if (opp) {		// check the opposite branch if it exists
				if (radius){
					if (radius >= axisdist) {	// if separating axis is within the check radius
						opp->find_closest(querypoint, queryResult, radius, mindist, depth + 1);
					}
				}
				else  {
					// if separating axis is closer than the current minimum point
					// check if a closer point is on the other side of the axis
					if (axisdist < mindist)
						opp->find_closest(querypoint, queryResult, radius, mindist, depth + 1);
				}
			}
		}
	};

	kd_node* root;
	Vertex* points;
	int count;
	vector<kd_query_result> queryResult;

	~kd_tree() {
		if (root)
			delete root;
		root = nullptr;
	}
	kd_tree(Vertex* points, int count) {
		if (count <= 0)
			return;

		root = new kd_node(&points[0], 0);
		for (int i = 1; i < count; i++)
			root->add(&points[i], i, 0);
	}

	int kd_nn(Vertex* querypoint, float radius) {
		queryResult.clear();
		float mindist = FLT_MAX;
		if (radius != 0)
			mindist = radius;
		root->find_closest(querypoint, queryResult, radius, mindist);
		if (queryResult.size() > 1)
			std::sort(queryResult.begin(), queryResult.end());

		return queryResult.size();
	}
};
