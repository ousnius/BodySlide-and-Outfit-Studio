#pragma once

#include <Windows.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glext.h>
#include <GL/wglext.h>
#include <string>
#include <vector>
#include <set>
#include <math.h>

using namespace std;

struct vtx;
#ifndef EPSILON
	#define EPSILON (1.0E-4)
#endif
#define PI  3.14159265359
#define DEG2RAD  PI/180.0f

struct vec2 {
	float u;
	float v;
};
typedef struct vec2 vector2;

struct vec3 {
	float x;
	float y;
	float z;
	vec3() {
		x = y = z = 0.0f;
	}
	vec3(float X, float Y, float Z) {
		x = X;
		y = Y;
		z = Z;
	}
	vec3(const vtx& other);

	void Zero() {
		x = y = z = 0.0f;
	}

	bool IsZero(bool bUseEpsilon = false) {
		if (bUseEpsilon) {
			if (x < EPSILON && x > -EPSILON && y < EPSILON && y > -EPSILON && z < EPSILON && z > -EPSILON)
				return true;
		}
		else {
			if (x == 0.0f && y == 0.0f && z == 0.0f)
				return true;
		}

		return false;
	}


	void Normalize() {
		float d = sqrt(x*x + y*y + z*z);
		if (d == 0)
			d = 1.0f;
		x /= d;
		y /= d;
		z /= d;
	}
	unsigned int hash() {
		unsigned int *h = (unsigned int*) this;
		unsigned int f = (h[0] + h[1] * 11 - h[2] * 17) & 0x7fffffff;
		return (f >> 22) ^ (f >> 12) ^ (f);
	}

	inline vec3& operator = (const vtx& other);
	inline vec3& operator += (const vtx& other);

	bool operator == (const vec3& other) {
		if (x == other.x && y == other.y && z == other.z)
			return true;
		return false;
	}
	bool operator != (const vec3& other) {
		if (x != other.x && y != other.y && z != other.z)
			return true;
		return false;
	}

	vec3& operator -= (const vec3& other) {
		x -= other.x;
		y -= other.y;
		z -= other.z;
		return (*this);
	}
	vec3 operator - (const vec3& other) const {
		vec3 tmp = (*this);
		tmp -= other;
		return tmp;
	}
	vec3& operator += (const vec3& other) {
		x += other.x;
		y += other.y;
		z += other.z;
		return (*this);
	}
	vec3 operator + (const vec3& other) const {
		vec3 tmp = (*this);
		tmp += other;
		return tmp;
	}
	vec3& operator *= (float val) {
		x *= val;
		y *= val;
		z *= val;
		return(*this);
	}
	vec3 operator * (float val) const {
		vec3 tmp = (*this);
		tmp *= val;
		return tmp;
	}
	vec3& operator /= (float val) {
		x /= val;
		y /= val;
		z /= val;
		return (*this);
	}
	vec3 operator / (float val) const {
		vec3 tmp = (*this);
		tmp /= val;
		return tmp;
	}

	vec3 cross(const vec3& other) const {
		vec3 tmp;
		tmp.x = y*other.z - z*other.y;
		tmp.y = z*other.x - x*other.z;
		tmp.z = x*other.y - y*other.x;
		return tmp;
	}

	float dot(const vec3& other) const {
		return x*other.x + y*other.y + z*other.z;
	}

	float DistanceTo(vec3 target) {
		float dx = target.x - x;
		float dy = target.y - y;
		float dz = target.z - z;
		return (float)sqrt(dx*dx + dy*dy + dz*dz);
	}

	float angle(const vec3& other) const {
		vec3 A(x, y, z);
		vec3 B(other.x, other.y, other.z);
		A.Normalize();
		B.Normalize();
		return acosf(A.dot(B));
	}
};

typedef struct vec3 vector3;

/* Vertex structure. This contains information about a mesh vertex, including position, normal, and index
   numbering.  In order to allow the data to be dropped directly to the graphics card, the values are split
   into floats and not vec3's. This means that various vector operations that are already implemented for
   vec3s are re-implemented here, so various math tasks can be done directly on the vertex position vectors
   instead of creating temporary vec3 values for the purpose.
*/
struct vtx {
	float x;
	float y;
	float z;
	float nx;
	float ny;
	float nz;
	int indexRef;		// Original index reference storage for use in K-d tree NN searches.

	vtx() {
		x = y = z = 0;
		nx = ny = nz = 0;
		indexRef = -1;
	}
	vtx(float X, float Y, float Z) : x(X), y(Y), z(Z) {
		indexRef = -1;
	}

	vtx(float X, float Y, float Z, int refIndex) : x(X), y(Y), z(Z), indexRef(refIndex) {
	}

	float DistanceTo(vtx* target) {
		float dx = target->x - x;
		float dy = target->y - y;
		float dz = target->z - z;
		return (float)sqrt(dx*dx + dy*dy + dz*dz);
	}

	float DistanceSquaredTo(vtx* target) {
		float dx = target->x - x;
		float dy = target->y - y;
		float dz = target->z - z;
		return (float)dx*dx + dy*dy + dz*dz;
	}

	unsigned int hash() {
		unsigned int *h = (unsigned int*) this;
		unsigned int f = (h[0] + h[1] * 11 - h[2] * 17) & 0x7fffffff;
		return (f >> 22) ^ (f >> 12) ^ (f);
	}

	vtx& operator =(const vec3& inVec) {
		x = inVec.x;
		y = inVec.y;
		z = inVec.z;
		return (*this);
	};
	void pos(const vtx& invtx) {
		x = invtx.x;
		y = invtx.y;
		z = invtx.z;
	}

	vtx& operator -= (const vtx& other) {
		x -= other.x;
		y -= other.y;
		z -= other.z;
		return (*this);
	}
	vtx& operator -= (const vec3& other) {
		x -= other.x;
		y -= other.y;
		z -= other.z;
		return (*this);
	}
	vtx operator - (const vtx& other) {
		vtx tmp = (*this);
		tmp -= other;
		return tmp;
	}
	vtx operator - (const vec3& other) {
		vtx tmp = (*this);
		tmp -= other;
		return tmp;
	}
	vtx& operator += (const vtx& other) {
		x += other.x;
		y += other.y;
		z += other.z;
		return (*this);
	}
	vtx& operator += (const vec3& other) {
		x += other.x;
		y += other.y;
		z += other.z;
		return (*this);
	}
	vtx operator + (const vtx& other) {
		vtx tmp = (*this);
		tmp += other;
		return tmp;
	}
	vtx operator + (const vec3& other) {
		vtx tmp = (*this);
		tmp += other;
		return tmp;
	}
	vtx& operator *= (float val) {
		x *= val;
		y *= val;
		z *= val;
		return(*this);
	}
	vtx operator * (float val) {
		vtx tmp = (*this);
		tmp *= val;
		return tmp;
	}
	vtx& operator /= (float val) {
		x /= val;
		y /= val;
		z /= val;
		return (*this);
	}
	vtx operator / (float val) {
		vtx tmp = (*this);
		tmp /= val;
		return tmp;
	}
};


// 4D Matrix class for calculating and applying transformations.
class Mat4 {
	float m[16];

public:
	Mat4(){
		Identity();
	}
	Mat4(const Mat4& other) {
		memcpy(m, other.m, sizeof(float) * 16);
	}
	Mat4(vector<vec3>& mat33) {
		Set(mat33);
	}

	void Set(vector<vec3>& mat33) {
		m[0] = mat33[0].x; m[1] = mat33[0].y; m[2] = mat33[0].z; m[3] = 0;
		m[4] = mat33[1].x; m[5] = mat33[1].y; m[6] = mat33[1].z; m[7] = 0;
		m[8] = mat33[2].x; m[9] = mat33[2].y; m[10] = mat33[2].z; m[11] = 0;
		m[12] = 0;		   m[13] = 0;		  m[14] = 0;	      m[15] = 1;
	}

	void SetRow(int row, vec3& inVec) {
		m[row * 4 + 0] = inVec.x;
		m[row * 4 + 1] = inVec.y;
		m[row * 4 + 2] = inVec.z;
	}


	~Mat4()	{}

	float& operator[] (int index) {
		return m[index];
	}

	Mat4& Identity(){
		memset(m, 0, sizeof(float) * 16);
		m[0] = m[5] = m[10] = m[15] = 1.0f;
		return *this;
	}

	void GetRow(int row, vec3& outVec) {
		outVec.x = m[row * 4 + 0];
		outVec.y = m[row * 4 + 1];
		outVec.z = m[row * 4 + 2];
	}

	void Get33(float* o, int r = 3, int c = 3) {
		int p = 0;
		for (int i = 0; i < 4; i++) {
			if (i == r)
				continue;
			for (int j = 0; j < 4; j++){
				if (j == c) continue;
				o[p++] = m[4 * i + j];
			}
		}
	}

	Mat4 Inverse() {
		Mat4 c;
		float det = Det();
		if (det == 0) {
			c[0] = FLT_MAX;
			return c;
		}
		return(Adjoint()*(1.0f / det));
	}

	Mat4 Cofactor() {
		Mat4 c;
		float minor[9];
		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < 4; j++) {
				Get33(minor, i, j);
				c[4 * i + j] = Det33(minor);
			}
		}
		return c;
	}

	//  i&1   j&1	   xor
	//	0000 0101    0 1 0 1
	//	1111 0101    1 0 1 0
	//	0000 0101    0 1 0 1
	//	1111 0101    1 0 1 0
	// Adjoint is the transpose of the cofactor.
	Mat4 Adjoint() {
		Mat4 c;
		float minor[9];
		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < 4; j++) {
				Get33(minor, i, j);
				if ((i & 1) ^ (j & 1))
					c[i + j * 4] = -Det33(minor);
				else
					c[i + j * 4] = Det33(minor);
			}
		}
		return c;
	}

	float Det() {
		//  |a	b	c	d|	|0	1	2	3 |
		//	|e	f	g	h|	|4	5	6	7 |
		//	|i	j	k	l|	|8	9	10	11|
		//	|m	n	o	p|	|12 13	14	15|		
		float A = m[0] * ((m[5] * m[10] * m[15] + m[6] * m[11] * m[13] + m[7] * m[9] * m[14]) -
			(m[7] * m[10] * m[13] + m[6] * m[9] * m[15] + m[5] * m[11] * m[14]));
		float B = m[1] * ((m[4] * m[10] * m[15] + m[6] * m[11] * m[12] + m[7] * m[8] * m[14]) -
			(m[7] * m[10] * m[12] + m[6] * m[8] * m[15] + m[4] * m[11] * m[14]));
		float C = m[2] * ((m[4] * m[9] * m[15] + m[5] * m[11] * m[12] + m[7] * m[8] * m[13]) -
			(m[7] * m[9] * m[12] + m[5] * m[8] * m[15] + m[4] * m[11] * m[13]));
		float D = m[3] * ((m[4] * m[9] * m[14] + m[5] * m[10] * m[12] + m[6] * m[8] * m[13]) -
			(m[6] * m[9] * m[12] + m[5] * m[8] * m[14] + m[4] * m[10] * m[13]));
		return A - B + C - D;
	}

	float Det33(float* t) {
		//  |a	b  c|	|0	1	2 |
		//	|d	e  f|	|3	4	5 |
		//	|g	h  i|	|6  7	8 |
		//  (aei + bfg + cdh) - (ceg + bdi + afh)
		//  (0*4*8 + 1*5*6 + 2*3*7) - (2*4*6 + 1*3*8 + 0*5*7)
		return ((t[0] * t[4] * t[8] + t[1] * t[5] * t[6] + t[2] * t[3] * t[7]) -
			(t[2] * t[4] * t[6] + t[1] * t[3] * t[8] + t[0] * t[5] * t[7]));

	}

	Mat4 operator+(const Mat4& other) const {
		Mat4 t(*this);
		t += other;
		return t;
	}
	Mat4& operator+=(const Mat4& other) {
		for (int i = 0; i < 16; i++)
			m[i] += other.m[i];

		return (*this);
	}
	Mat4 operator-(const Mat4& other) const {
		Mat4 t(*this);
		t -= other;
		return t;
	}
	Mat4& operator-=(const Mat4& other) {
		for (int i = 0; i < 16; i++)
			m[i] -= other.m[i];

		return(*this);
	}
	vec3 operator*(const vec3& v) const {
		return vec3
			(m[0] * v.x + m[1] * v.y + m[2] * v.z + m[3],
			m[4] * v.x + m[5] * v.y + m[6] * v.z + m[7],
			m[8] * v.x + m[9] * v.y + m[10] * v.z + m[11]);
	}
	vtx operator*(const vtx& v) const {
		return vtx
			(m[0] * v.x + m[1] * v.y + m[2] * v.z + m[3],
			m[4] * v.x + m[5] * v.y + m[6] * v.z + m[7],
			m[8] * v.x + m[9] * v.y + m[10] * v.z + m[11]);
	}

	Mat4& operator*=(const Mat4& r) {
		float v1, v2, v3, v4;
		for (int n = 0; n < 16; n += 4) {
			v1 = m[n] * r.m[0] + m[n + 1] * r.m[4] + m[n + 2] * r.m[8] + m[n + 3] * r.m[12];
			v2 = m[n] * r.m[1] + m[n + 1] * r.m[5] + m[n + 2] * r.m[9] + m[n + 3] * r.m[13];
			v3 = m[n] * r.m[2] + m[n + 1] * r.m[6] + m[n + 2] * r.m[10] + m[n + 3] * r.m[14];
			v4 = m[n] * r.m[3] + m[n + 1] * r.m[7] + m[n + 2] * r.m[11] + m[n + 3] * r.m[15];
			m[n] = v1;		m[n + 1] = v2;		m[n + 2] = v3;		m[n + 3] = v4;
		}
		return *this;
	}
	Mat4 operator * (const Mat4& other) {
		Mat4 t(*this);
		t *= other;
		return t;
	}
	Mat4 operator * (float val) {
		Mat4 t(*this);
		for (int i = 0; i < 16; i++)
			t[i] *= val;

		return t;
	}

	void PushTranslate(const vec3& byvec) {
		Mat4 tmp;
		tmp.Translate(byvec);
		(*this) *= tmp;
	}

	Mat4& Translate(const vec3& byVec) {
		return Translate(byVec.x, byVec.y, byVec.z);

	}
	Mat4& Translate(float x, float y, float z) {
		m[3] += x;
		m[7] += y;
		m[11] += z;
		return(*this);
	}

	void PushScale(float x, float y, float z) {
		Mat4 tmp;
		tmp.Scale(x, y, z);
		(*this) *= tmp;
	}


	Mat4& Scale(float x, float y, float z) {
		m[0] *= x;   m[1] *= x;  m[2] *= x;  m[3] *= x;
		m[4] *= y;   m[5] *= y;  m[6] *= y;  m[7] *= y;
		m[8] *= z;   m[9] *= z;  m[10] *= z;  m[11] *= z;
		return (*this);
	}

	void PushRotate(float radAngle, const vec3& axis) {
		Mat4 tmp;
		tmp.Rotate(radAngle, axis);
		(*this) *= tmp;
	}

	Mat4& Rotate(float radAngle, const vec3& axis) {
		return Rotate(radAngle, axis.x, axis.y, axis.z);
	}

	Mat4& Rotate(float radAngle, float x, float y, float z) {
		float c = cosf(radAngle);
		float s = sinf(radAngle);

		float xx = x*x;
		float xy = x*y;
		float xz = x*z;
		float yy = y*y;
		float yz = y*z;
		float zz = z*z;

		float ic = 1 - c;

		Mat4 t;
		t.m[0] = xx * ic + c;
		t.m[1] = xy * ic - z * s;
		t.m[2] = xz * ic + y * s;
		t.m[3] = 0;

		t.m[4] = xy * ic + z * s;
		t.m[5] = yy * ic + c;
		t.m[6] = yz * ic - x * s;
		t.m[7] = 0;

		t.m[8] = xz * ic - y * s;
		t.m[9] = yz * ic + x * s;
		t.m[10] = zz * ic + c;

		t.m[11] = t.m[12] = t.m[13] = t.m[14] = 0;
		t.m[15] = 1;

		*this = t * (*this);

		return (*this);
	}

	Mat4& Align(const vec3& sourceVec, const vec3& destVec) {
		Identity();
		float angle = sourceVec.angle(destVec);
		vec3 axis = sourceVec.cross(destVec);
		axis.Normalize();

		return Rotate(angle, axis);
	}
};


struct tri {
	unsigned short p1;
	unsigned short p2;
	unsigned short p3;

	tri();
	tri(unsigned short P1, unsigned short P2, unsigned short P3);
	void set(unsigned short P1, unsigned short P2, unsigned short P3)	{
		p1 = P1; p2 = P2; p3 = P3;
	}

	void trinormal(vtx* vertref, vec3* outNormal);

	void trinormal(vector<vector3>& vertref, vector3* outNormal) {
		vtx va(vertref[p2].x - vertref[p1].x, vertref[p2].y - vertref[p1].y, vertref[p2].z - vertref[p1].z);
		vtx vb(vertref[p3].x - vertref[p1].x, vertref[p3].y - vertref[p1].y, vertref[p3].z - vertref[p1].z);

		outNormal->x = va.y*vb.z - va.z*vb.y;
		outNormal->y = va.z*vb.x - va.x*vb.z;
		outNormal->z = va.x*vb.y - va.y*vb.x;
	}
	void midpoint(vtx* vertref, vec3& outPoint);
	float AxisMidPointY(vtx* vertref);
	float AxisMidPointX(vtx* vertref);
	float AxisMidPointZ(vtx* vertref);

	bool IntersectRay(vtx* vertref, vec3& origin, vec3& direction, float* outDistance = NULL, vec3* worldPos = NULL);

	bool IntersectSphere(vtx* vertref, vec3& origin, float radius);

	bool operator < (const tri& other) const {
		int d = 0;
		if (d == 0) d = p1 - other.p1;
		if (d == 0) d = p2 - other.p2;
		if (d == 0) d = p3 - other.p3;
		return d < 0;
	}
	void rot() {
		if (p2 < p1 && p2 < p3) {
			set(p2, p3, p1);
		}
		else if (p3 < p1) {
			set(p3, p1, p2);
		}
	}
};

typedef struct tri triangle;

struct edge {
	unsigned short p1;
	unsigned short p2;
	edge() {
		p1 = p2 = 0;
	}
	edge(unsigned short P1, unsigned short P2) {
		p1 = P1; p2 = P2;
	}
};

namespace std {
	template<> struct hash < edge > {
		std::size_t operator() (const edge& t) const {
			return ((t.p2 << 16) | (t.p1 & 0xFFFF));
		}
	};

	template <> struct equal_to < edge > {
		bool operator() (const edge& t1, const edge& t2) const {
			return ((t1.p1 == t2.p1) && (t1.p2 == t2.p2));
		}
	};

	template <> struct hash < tri > {
		std::size_t operator() (const tri& t) const {
			char* d = (char*)&t;
			size_t len = sizeof(tri);
			size_t hash, i;
			for (hash = i = 0; i < len; ++i) {
				hash += d[i];
				hash += (hash << 10);
				hash ^= (hash >> 6);
			}
			hash += (hash << 3);
			hash ^= (hash >> 11);
			hash += (hash << 15);
			return hash;
		}
	};

	template <> struct equal_to < tri > {
		bool operator() (const tri& t1, const tri& t2) const {
			return ((t1.p1 == t2.p1) && (t1.p2 == t2.p2) && (t1.p3 == t2.p3));
		}
	};
}

struct IntersectResult;

struct AABB {
	vec3 min;
	vec3 max;

	AABB();
	AABB(const vec3 newMin, const vec3 newMax);

	AABB(vec3* points, int nPoints);

	AABB(vtx* points, int nPoints);

	AABB(vtx* points, unsigned short* indices, int nPoints);

	void AddBoxToMesh(vector<vtx>& verts, vector<edge>& edges);

	void Merge(vtx* points, unsigned short* indices, int nPoints);
	void Merge(AABB& other);

	bool IntersectAABB(AABB& other);

	bool IntersectRay(vec3& Origin, vec3& Direction, vec3* outCoord);

	bool IntersectSphere(vec3& Origin, float radius);
};


class AABBTree {
	int max_depth;
	int min_facets;
	vtx* vertexRef;
	tri* triRef;

public:
	bool bFlag;
	int depthCounter;
	int sentinel;
	class AABBTreeNode {
		AABBTreeNode* N;
		AABBTreeNode* P;
		AABBTreeNode* parent;
		AABB mBB;
		AABBTree* tree;
		int* mIFacets;
		int nFacets;
		int id;
	public:
		AABBTreeNode();
		~AABBTreeNode();

		// Recursively generates AABB Tree nodes using the referenced data.
		AABBTreeNode(vector<int>& facetIndices, AABBTree* treeRef, AABBTreeNode* parent, int depth);

		// As above, but facetIndices is modified with in-place sorting rather than using vector::push_back to generate sub lists.
		// Sorting swaps from front of list to end when pos midpoints are found at the beginning of the list.
		AABBTreeNode(vector<int>& facetIndices, int start, int end, AABBTree* treeRef, AABBTreeNode* parent, int depth);

		vec3 Center();

		void AddDebugFrames(vector<vtx>& verts, vector<edge>& edges, int maxdepth = 0, int curdepth = 0);

		void AddRayIntersectFrames(vec3& origin, vec3& direction, vector<vtx>& verts, vector<edge>& edges);

		bool IntersectRay(vec3& origin, vec3& direction, vector<IntersectResult>* results);

		bool IntersectSphere(vec3& origin, float radius, vector<IntersectResult>* results);

		void UpdateAABB(AABB* childBB = NULL);
	};

	AABBTreeNode* root;


public:
	AABBTree();

	AABBTree(vtx* vertices, tri* facets, int nFacets, int maxDepth, int minFacets);

	~AABBTree();

	int MinFacets();
	int MaxDepth();

	vec3 Center();

	// Calculate bounding box and geometric average.
	void CalcAABBandGeoAvg(vector<int>& forFacets, AABB& outBB, vec3& outAxisAvg);

	// Calculate bounding box and geometric average for sub list.
	void CalcAABBandGeoAvg(vector<int>& forFacets, int start, int end, AABB& outBB, vec3& outAxisAvg);

	void CalcAABBandGeoAvg(int forFacets[], int start, int end, AABB& outBB, vec3& outAxisAvg);

	void BuildDebugFrames(vtx** outVerts, int* outNumVerts, edge** outEdges, int* outNumEdges);

	void BuildRayIntersectFrames(vec3& origin, vec3& direction, vtx** outVerts, int* outNumVerts, edge** outEdges, int* outNumEdges);

	bool IntersectRay(vec3& origin, vec3& direction, vector<IntersectResult>* results = NULL);

	bool IntersectSphere(vec3& origin, float radius, vector<IntersectResult>* results = NULL);
};

struct IntersectResult {
	int HitFacet;
	float HitDistance;
	vec3 HitCoord;
	AABBTree::AABBTreeNode* bvhNode;
};

vec3& vec3::operator =(const vtx& other) {
	x = other.x;
	y = other.y;
	z = other.z;
	return (*this);
}

vec3& vec3::operator += (const vtx& other) {
	x += other.x;
	y += other.y;
	z += other.z;
	return (*this);
}
