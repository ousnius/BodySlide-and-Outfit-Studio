/*
BodySlide and Outfit Studio
Copyright (C) 2016  Caliente & ousnius
See the included LICENSE file
*/

#pragma once

#include <vector>

using namespace std;

#pragma warning (disable : 4018 4244 4267 4389)

const double EPSILON = 0.0001;

const float PI = 3.141592f;
const float DEG2RAD = PI / 180.0f;

typedef unsigned char byte;
typedef unsigned short ushort;
typedef unsigned int uint;

struct Vertex;

struct Vector2 {
	float u;
	float v;

	Vector2() {
		u = v = 0.0f;
	}
	Vector2(float U, float V) {
		u = U;
		v = V;
	}
};

struct Vector3 {
	float x;
	float y;
	float z;

	Vector3() {
		x = y = z = 0.0f;
	}
	Vector3(float X, float Y, float Z) {
		x = X;
		y = Y;
		z = Z;
	}
	Vector3(const Vertex& other);

	void Zero() {
		x = y = z = 0.0f;
	}

	bool IsZero(bool bUseEpsilon = false) {
		if (bUseEpsilon) {
			if (fabs(x) < EPSILON && fabs(y) < EPSILON && fabs(z) < EPSILON)
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

	uint hash() {
		uint *h = (uint*) this;
		uint f = (h[0] + h[1] * 11 - h[2] * 17) & 0x7fffffff;
		return (f >> 22) ^ (f >> 12) ^ (f);
	}

	inline Vector3& operator = (const Vertex& other);
	inline Vector3& operator += (const Vertex& other);


	bool operator == (const Vector3& other) {
		if (x == other.x && y == other.y && z == other.z)
			return true;
		return false;
	}
	bool operator != (const Vector3& other) {
		if (x != other.x && y != other.y && z != other.z)
			return true;
		return false;
	}

	Vector3& operator -= (const Vector3& other) {
		x -= other.x;
		y -= other.y;
		z -= other.z;
		return (*this);
	}
	Vector3 operator - (const Vector3& other) const {
		Vector3 tmp = (*this);
		tmp -= other;
		return tmp;
	}
	Vector3& operator += (const Vector3& other) {
		x += other.x;
		y += other.y;
		z += other.z;
		return (*this);
	}
	Vector3 operator + (const Vector3& other) const {
		Vector3 tmp = (*this);
		tmp += other;
		return tmp;
	}
	Vector3& operator *= (float val) {
		x *= val;
		y *= val;
		z *= val;
		return(*this);
	}
	Vector3 operator * (float val) const {
		Vector3 tmp = (*this);
		tmp *= val;
		return tmp;
	}
	Vector3& operator /= (float val) {
		x /= val;
		y /= val;
		z /= val;
		return (*this);
	}
	Vector3 operator / (float val) const {
		Vector3 tmp = (*this);
		tmp /= val;
		return tmp;
	}

	Vector3 cross(const Vector3& other) const {
		Vector3 tmp;
		tmp.x = y*other.z - z*other.y;
		tmp.y = z*other.x - x*other.z;
		tmp.z = x*other.y - y*other.x;
		return tmp;
	}

	float dot(const Vector3& other) const {
		return x*other.x + y*other.y + z*other.z;
	}

	float DistanceTo(Vector3 target) {
		float dx = target.x - x;
		float dy = target.y - y;
		float dz = target.z - z;
		return (float)sqrt(dx*dx + dy*dy + dz*dz);
	}

	float angle(const Vector3& other) const {
		Vector3 A(x, y, z);
		Vector3 B(other.x, other.y, other.z);
		A.Normalize();
		B.Normalize();
		return acosf(A.dot(B));
	}

	void clampEpsilon() {
		if (fabs(x) < EPSILON)
			x = 0.0f;
		if (fabs(y) < EPSILON)
			y = 0.0f;
		if (fabs(z) < EPSILON)
			z = 0.0f;
	}
};

/* Vertex structure. This contains information about a mesh vertex, including position, normal, and index
   numbering.  In order to allow the data to be dropped directly to the graphics card, the values are split
   into floats and not Vector3's. This means that various vector operations that are already implemented for
   vec3s are re-implemented here, so various math tasks can be done directly on the vertex position vectors
   instead of creating temporary Vector3 values for the purpose.
*/
struct Vertex {
	float x;
	float y;
	float z;
	float nx;
	float ny;
	float nz;
	int indexRef;		// Original index reference storage for use in K-d tree NN searches.

	Vertex() {
		x = y = z = 0;
		nx = ny = nz = 0;
		indexRef = -1;
	}
	Vertex(float X, float Y, float Z) : x(X), y(Y), z(Z) {
		indexRef = -1;
	}

	Vertex(float X, float Y, float Z, int refIndex) : x(X), y(Y), z(Z), indexRef(refIndex) {
	}

	float DistanceTo(Vertex* target) {
		float dx = target->x - x;
		float dy = target->y - y;
		float dz = target->z - z;
		return (float)sqrt(dx*dx + dy*dy + dz*dz);
	}

	float DistanceSquaredTo(Vertex* target) {
		float dx = target->x - x;
		float dy = target->y - y;
		float dz = target->z - z;
		return (float)dx*dx + dy*dy + dz*dz;
	}

	uint hash() {
		uint *h = (uint*) this;
		uint f = (h[0] + h[1] * 11 - h[2] * 17) & 0x7fffffff;
		return (f >> 22) ^ (f >> 12) ^ (f);
	}

	Vertex& operator =(const Vector3& inVec) {
		x = inVec.x;
		y = inVec.y;
		z = inVec.z;
		return (*this);
	};
	void pos(const Vertex& invtx) {
		x = invtx.x;
		y = invtx.y;
		z = invtx.z;
	}

	Vertex& operator -= (const Vertex& other) {
		x -= other.x;
		y -= other.y;
		z -= other.z;
		return (*this);
	}
	Vertex& operator -= (const Vector3& other) {
		x -= other.x;
		y -= other.y;
		z -= other.z;
		return (*this);
	}
	Vertex operator - (const Vertex& other) {
		Vertex tmp = (*this);
		tmp -= other;
		return tmp;
	}
	Vertex operator - (const Vector3& other) {
		Vertex tmp = (*this);
		tmp -= other;
		return tmp;
	}
	Vertex& operator += (const Vertex& other) {
		x += other.x;
		y += other.y;
		z += other.z;
		return (*this);
	}
	Vertex& operator += (const Vector3& other) {
		x += other.x;
		y += other.y;
		z += other.z;
		return (*this);
	}
	Vertex operator + (const Vertex& other) {
		Vertex tmp = (*this);
		tmp += other;
		return tmp;
	}
	Vertex operator + (const Vector3& other) {
		Vertex tmp = (*this);
		tmp += other;
		return tmp;
	}
	Vertex& operator *= (float val) {
		x *= val;
		y *= val;
		z *= val;
		return(*this);
	}
	Vertex operator * (float val) {
		Vertex tmp = (*this);
		tmp *= val;
		return tmp;
	}
	Vertex& operator /= (float val) {
		x /= val;
		y /= val;
		z /= val;
		return (*this);
	}
	Vertex operator / (float val) {
		Vertex tmp = (*this);
		tmp /= val;
		return tmp;
	}
};

Vector3& Vector3::operator = (const Vertex& other) {
	x = other.x;
	y = other.y;
	z = other.z;
	return (*this);
}

Vector3& Vector3::operator += (const Vertex& other) {
	x += other.x;
	y += other.y;
	z += other.z;
	return (*this);
}


// 4D Matrix class for calculating and applying transformations.
class Matrix4 {
	float m[16];

public:
	Matrix4(){
		Identity();
	}
	Matrix4(const Matrix4& other) {
		memcpy(m, other.m, sizeof(float) * 16);
	}
	Matrix4(const vector<Vector3>& mat33) {
		Set(mat33);
	}
	~Matrix4() {}

	void Set(Vector3 mat33[3]) {
		m[0] = mat33[0].x; m[1] = mat33[0].y; m[2] = mat33[0].z; m[3] = 0;
		m[4] = mat33[1].x; m[5] = mat33[1].y; m[6] = mat33[1].z; m[7] = 0;
		m[8] = mat33[2].x; m[9] = mat33[2].y; m[10] = mat33[2].z; m[11] = 0;
		m[12] = 0;		   m[13] = 0;		  m[14] = 0;	      m[15] = 1;
	}

	void Set(const vector<Vector3>& mat33) {
		m[0] = mat33[0].x; m[1] = mat33[0].y; m[2] = mat33[0].z; m[3] = 0;
		m[4] = mat33[1].x; m[5] = mat33[1].y; m[6] = mat33[1].z; m[7] = 0;
		m[8] = mat33[2].x; m[9] = mat33[2].y; m[10] = mat33[2].z; m[11] = 0;
		m[12] = 0;		   m[13] = 0;		  m[14] = 0;	      m[15] = 1;
	}

	void SetRow(int row, Vector3& inVec) {
		m[row * 4 + 0] = inVec.x;
		m[row * 4 + 1] = inVec.y;
		m[row * 4 + 2] = inVec.z;
	}

	float& operator[] (int index) {
		return m[index];
	}

	Matrix4& Identity(){
		memset(m, 0, sizeof(float) * 16);
		m[0] = m[5] = m[10] = m[15] = 1.0f;
		return *this;
	}

	void GetRow(int row, Vector3& outVec) {
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

	Matrix4 Inverse() {
		Matrix4 c;
		float det = Det();
		if (det == 0) {
			c[0] = std::numeric_limits<float>::max();
			return c;
		}
		return(Adjoint()*(1.0f / det));
	}

	Matrix4 Cofactor() {
		Matrix4 c;
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
	Matrix4 Adjoint() {
		Matrix4 c;
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

	Matrix4 operator+(const Matrix4& other) const {
		Matrix4 t(*this);
		t += other;
		return t;
	}
	Matrix4& operator+=(const Matrix4& other) {
		for (int i = 0; i < 16; i++)
			m[i] += other.m[i];

		return (*this);
	}
	Matrix4 operator-(const Matrix4& other) const {
		Matrix4 t(*this);
		t -= other;
		return t;
	}
	Matrix4& operator-=(const Matrix4& other) {
		for (int i = 0; i < 16; i++)
			m[i] -= other.m[i];

		return(*this);
	}
	Vector3 operator*(const Vector3& v) const {
		return Vector3
			(m[0] * v.x + m[1] * v.y + m[2] * v.z + m[3],
			m[4] * v.x + m[5] * v.y + m[6] * v.z + m[7],
			m[8] * v.x + m[9] * v.y + m[10] * v.z + m[11]);
	}
	Vertex operator*(const Vertex& v) const {
		return Vertex
			(m[0] * v.x + m[1] * v.y + m[2] * v.z + m[3],
			m[4] * v.x + m[5] * v.y + m[6] * v.z + m[7],
			m[8] * v.x + m[9] * v.y + m[10] * v.z + m[11]);
	}

	Matrix4& operator*=(const Matrix4& r) {
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
	Matrix4 operator * (const Matrix4& other) {
		Matrix4 t(*this);
		t *= other;
		return t;
	}
	Matrix4 operator * (float val) {
		Matrix4 t(*this);
		for (int i = 0; i < 16; i++)
			t[i] *= val;

		return t;
	}

	void PushTranslate(const Vector3& byvec) {
		Matrix4 tmp;
		tmp.Translate(byvec);
		(*this) *= tmp;
	}

	Matrix4& Translate(const Vector3& byVec) {
		return Translate(byVec.x, byVec.y, byVec.z);

	}
	Matrix4& Translate(float x, float y, float z) {
		m[3] += x;
		m[7] += y;
		m[11] += z;
		return(*this);
	}

	void PushScale(float x, float y, float z) {
		Matrix4 tmp;
		tmp.Scale(x, y, z);
		(*this) *= tmp;
	}


	Matrix4& Scale(float x, float y, float z) {
		m[0] *= x;   m[1] *= x;  m[2] *= x;  m[3] *= x;
		m[4] *= y;   m[5] *= y;  m[6] *= y;  m[7] *= y;
		m[8] *= z;   m[9] *= z;  m[10] *= z;  m[11] *= z;
		return (*this);
	}

	void PushRotate(float radAngle, const Vector3& axis) {
		Matrix4 tmp;
		tmp.Rotate(radAngle, axis);
		(*this) *= tmp;
	}

	Matrix4& Rotate(float radAngle, const Vector3& axis) {
		return Rotate(radAngle, axis.x, axis.y, axis.z);
	}

	Matrix4& Rotate(float radAngle, float x, float y, float z) {
		float c = cosf(radAngle);
		float s = sinf(radAngle);

		float xx = x*x;
		float xy = x*y;
		float xz = x*z;
		float yy = y*y;
		float yz = y*z;
		float zz = z*z;

		float ic = 1 - c;

		Matrix4 t;
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

	Matrix4& Align(const Vector3& sourceVec, const Vector3& destVec) {
		Identity();
		float angle = sourceVec.angle(destVec);
		Vector3 axis = sourceVec.cross(destVec);
		axis.Normalize();

		return Rotate(angle, axis);
	}
};


struct BoundingSphere {
	Vector3 center;
	float radius;

	BoundingSphere() {
		center.Zero();
		radius = 0.0f;
	}

	BoundingSphere(const Vector3& center, const float& radius) {
		this->center = center;
		this->radius = radius;
	}

	// Miniball algorithm
	BoundingSphere(const vector<Vector3>& vertices);
};


struct Quaternion {
	float w;
	float x;
	float y;
	float z;

	Quaternion() {
		w = 1.0f;
		x = 0.0f;
		y = 0.0f;
		z = 0.0f;
	}

	Quaternion(float w, float x, float y, float z) {
		this->w = w;
		this->x = x;
		this->y = y;
		this->z = z;
	}
};


struct Triangle {
	ushort p1;
	ushort p2;
	ushort p3;

	Triangle();
	Triangle(ushort P1, ushort P2, ushort P3);
	void set(ushort P1, ushort P2, ushort P3)	{
		p1 = P1; p2 = P2; p3 = P3;
	}

	void trinormal(Vertex* vertref, Vector3* outNormal);

	void trinormal(const vector<Vector3>& vertref, Vector3* outNormal) {
		outNormal->x = (vertref[p2].y - vertref[p1].y) * (vertref[p3].z - vertref[p1].z) - (vertref[p2].z - vertref[p1].z) * (vertref[p3].y - vertref[p1].y);
		outNormal->y = (vertref[p2].z - vertref[p1].z) * (vertref[p3].x - vertref[p1].x) - (vertref[p2].x - vertref[p1].x) * (vertref[p3].z - vertref[p1].z);
		outNormal->z = (vertref[p2].x - vertref[p1].x) * (vertref[p3].y - vertref[p1].y) - (vertref[p2].y - vertref[p1].y) * (vertref[p3].x - vertref[p1].x);
	}

	void midpoint(Vertex* vertref, Vector3& outPoint);
	float AxisMidPointY(Vertex* vertref);
	float AxisMidPointX(Vertex* vertref);
	float AxisMidPointZ(Vertex* vertref);

	bool IntersectRay(Vertex* vertref, Vector3& origin, Vector3& direction, float* outDistance = nullptr, Vector3* worldPos = nullptr);

	bool IntersectSphere(Vertex* vertref, Vector3& origin, float radius);

	bool operator < (const Triangle& other) const {
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

struct Edge {
	ushort p1;
	ushort p2;

	Edge() {
		p1 = p2 = 0;
	}
	Edge(ushort P1, ushort P2) {
		p1 = P1; p2 = P2;
	}
};

namespace std {
	template<> struct hash < Edge > {
		std::size_t operator() (const Edge& t) const {
			return ((t.p2 << 16) | (t.p1 & 0xFFFF));
		}
	};

	template <> struct equal_to < Edge > {
		bool operator() (const Edge& t1, const Edge& t2) const {
			return ((t1.p1 == t2.p1) && (t1.p2 == t2.p2));
		}
	};

	template <> struct hash < Triangle > {
		std::size_t operator() (const Triangle& t) const {
			char* d = (char*)&t;
			size_t len = sizeof(Triangle);
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

	template <> struct equal_to < Triangle > {
		bool operator() (const Triangle& t1, const Triangle& t2) const {
			return ((t1.p1 == t2.p1) && (t1.p2 == t2.p2) && (t1.p3 == t2.p3));
		}
	};
}

struct Face {
	byte nPoints;
	ushort p1;
	ushort uv1;
	ushort p2;
	ushort uv2;
	ushort p3;
	ushort uv3;
	ushort p4;
	ushort uv4;

	Face(int npts = 0, int* points = nullptr, int* tc = nullptr) {
		nPoints = npts;
		if (npts < 3)
			return;

		p1 = points[0]; p2 = points[1];  p3 = points[2];
		uv1 = tc[0]; uv2 = tc[1]; uv3 = tc[2];
		if (npts == 4) {
			p4 = points[3];
			uv4 = tc[3];
		}
	}
};
