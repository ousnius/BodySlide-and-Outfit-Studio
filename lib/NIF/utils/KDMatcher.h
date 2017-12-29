/*
BodySlide and Outfit Studio
Copyright (C) 2018  Caliente & ousnius
See the included LICENSE file
*/

#pragma once

#include "Object3d.h"
#include <algorithm>
#include <memory>

// A specialized KD tree that finds duplicate vertices in a point cloud.  

class kd_matcher {
public:
	class kd_node {
	public:
		std::pair<Vector3*, int> p = std::pair<Vector3*, int>(nullptr, -1);
		std::unique_ptr<kd_node> less;
		std::unique_ptr<kd_node> more;

		kd_node(const std::pair<Vector3*, int>& point) {
			p = point;
		}

		std::pair<Vector3*, int> add(const std::pair<Vector3*, int>& point, int depth) {
			int axis = depth % 3;
			bool domore = false;
			float dx = p.first->x - point.first->x;
			float dy = p.first->y - point.first->y;
			float dz = p.first->z - point.first->z;

			if (std::fabs(dx) < EPSILON && std::fabs(dy) < EPSILON && std::fabs(dz) < EPSILON)
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
				else more = std::make_unique<kd_node>(point);
			}
			else {
				if (less) return less->add(point, depth + 1);
				else less = std::make_unique<kd_node>(point);
			}
			return std::pair<Vector3*, int>(nullptr, -1);
		}
	};

	std::unique_ptr<kd_node> root;
	Vector3* points = nullptr;
	int count;
	std::vector<std::pair<std::pair<Vector3*, int>, std::pair<Vector3*, int>>> matches;

	kd_matcher(Vector3* pts, int cnt) {
		if (cnt <= 0)
			return;

		std::pair<Vector3*, int> pong;
		root = std::make_unique<kd_node>(std::pair<Vector3*, int>(&pts[0], 0));
		for (int i = 1; i < cnt; i++) {
			std::pair<Vector3*, int> point(&pts[i], i);
			pong = root->add(point, 0);
			if (pong.first)
				matches.push_back(std::pair<std::pair<Vector3*, int>, std::pair<Vector3*, int>>(point, pong));
		}
	}
};

class kd_query_result {
public:
	Vector3* v;
	ushort vertex_index;
	float distance;
	bool operator < (const kd_query_result& other) const {
		return distance < other.distance;
	}
};

// More general purpose KD tree that assembles a tree from input points and allows nearest neighbor and radius searches on the data.
class kd_tree {
public:
	class kd_node {
	public:
		Vector3* p = nullptr;
		int p_i = -1;
		std::unique_ptr<kd_node> less;
		std::unique_ptr<kd_node> more;

		kd_node(Vector3* point, int point_index) {
			p = point;
			p_i = point_index;
		}

		void add(Vector3* point, int point_index, int depth) {
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
				else more = std::make_unique<kd_node>(point, point_index);
			}
			else {
				if (less) return less->add(point, point_index, depth + 1);
				else less = std::make_unique<kd_node>(point, point_index);
			}
		}

		// Finds the closest point(s) to "querypoint" within the provided radius. If radius is 0, only the single closest point is found.
		// On first call, "mindist" should be set to FLT_MAX and depth set to 0.
		void find_closest(Vector3* querypoint, std::vector<kd_query_result>& queryResult, float radius, float& mindist, int depth = 0) {
			kd_query_result kdqr;
			int axis = depth % 3;				// Which separating axis to use based on depth
			float dx = p->x - querypoint->x;	// Axis sides
			float dy = p->y - querypoint->y;
			float dz = p->z - querypoint->z;
			kd_node* act = less.get();			// Active search branch
			kd_node* opp = more.get();			// Opposite search branch
			float axisdist = 0.0f;				// Distance from the query point to the separating axis
			float pointdist;					// Distance from the query point to the node's point

			switch (axis) {
			case 0:
				if (dx > 0.0f)  {
					act = more.get();
					opp = less.get();
				}
				axisdist = std::fabs(dx);
				break;
			case 1:
				if (dy > 0.0f) {
					act = more.get();
					opp = less.get();
				}
				axisdist = std::fabs(dy);
				break;
			case 2:
				if (dz > 0.0f)  {
					act = more.get();
					opp = less.get();
				}
				axisdist = std::fabs(dz);
				break;
			}

			// The axis choice tells us which branch to search
			if (act)
				act->find_closest(querypoint, queryResult, radius, mindist, depth + 1);

			// On the way back out check current point to see if it's the closest
			// Fix? Might want to use squared distance instead... probably unnecessary.
			pointdist = querypoint->DistanceTo(*p);

			// No opposites
			bool notOpp = true;
			//notOpp = (querypoint->nx * p->nx + querypoint->ny * p->ny + querypoint->nz * p->nz) > 0.0f;

			if (pointdist <= mindist && notOpp) {
				kdqr.v = p;
				kdqr.vertex_index = p_i;
				kdqr.distance = pointdist;
				queryResult.push_back(kdqr);
				mindist = pointdist;
			}
			else if (radius > mindist && notOpp) {	// If there's room between the minimum distance and the search radius
				if (pointdist <= radius) {			// check to see if the point falls in that space, and if so, add it.
					kdqr.v = p;
					kdqr.vertex_index = p_i;
					kdqr.distance = pointdist;
					queryResult.push_back(kdqr);	// This is skipped if radius is 0
				}
			}

			// Check the opposite branch if it exists
			if (opp) {
				if (radius > 0.0f) {
					if (radius >= axisdist)	// If separating axis is within the check radius
						opp->find_closest(querypoint, queryResult, radius, mindist, depth + 1);
				}
				else  {
					// If separating axis is closer than the current minimum point
					// check if a closer point is on the other side of the axis.
					if (axisdist < mindist)
						opp->find_closest(querypoint, queryResult, radius, mindist, depth + 1);
				}
			}
		}
	};

	std::unique_ptr<kd_node> root;
	std::vector<kd_query_result> queryResult;

	kd_tree(Vector3* points, int count) {
		if (count <= 0)
			return;

		root = std::make_unique<kd_node>(&points[0], 0);
		for (int i = 1; i < count; i++)
			root->add(&points[i], i, 0);
	}

	int kd_nn(Vector3* querypoint, float radius) {
		float mindist = std::numeric_limits<float>().max();
		if (radius > 0.0f)
			mindist = radius;

		queryResult.clear();
		root->find_closest(querypoint, queryResult, radius, mindist);
		std::sort(queryResult.begin(), queryResult.end());

		return queryResult.size();
	}
};
