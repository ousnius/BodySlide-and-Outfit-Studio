/*
BodySlide and Outfit Studio
Copyright (C) 2018  Caliente & ousnius
See the included LICENSE file
*/

#pragma once

#include <vector>
#include <algorithm>

#pragma warning (disable : 4018 4244 4267 4389)

const double EPSILON = 0.0001;

const float PI = 3.141592f;
const float DEG2RAD = PI / 180.0f;

typedef unsigned char byte;
typedef unsigned short ushort;
typedef unsigned int uint;

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

	bool operator == (const Vector2& other) {
		if (u == other.u && v == other.v)
			return true;
		return false;
	}
	bool operator != (const Vector2& other) {
		if (u != other.u && v != other.v)
			return true;
		return false;
	}

	Vector2& operator -= (const Vector2& other) {
		u -= other.u;
		v -= other.v;
		return (*this);
	}
	Vector2 operator - (const Vector2& other) const {
		Vector2 tmp = (*this);
		tmp -= other;
		return tmp;
	}

	Vector2& operator += (const Vector2& other) {
		u += other.u;
		v += other.v;
		return (*this);
	}
	Vector2 operator + (const Vector2& other) const {
		Vector2 tmp = (*this);
		tmp += other;
		return tmp;
	}

	Vector2& operator *= (float val) {
		u *= val;
		v *= val;
		return(*this);
	}
	Vector2 operator * (float val) const {
		Vector2 tmp = (*this);
		tmp *= val;
		return tmp;
	}

	Vector2& operator /= (float val) {
		u /= val;
		v /= val;
		return (*this);
	}
	Vector2 operator / (float val) const {
		Vector2 tmp = (*this);
		tmp /= val;
		return tmp;
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

	void Zero() {
		x = y = z = 0.0f;
	}

	bool IsZero(bool bUseEpsilon = false) {
		if (bUseEpsilon) {
			if (std::fabs(x) < EPSILON && std::fabs(y) < EPSILON && std::fabs(z) < EPSILON)
				return true;
		}
		else {
			if (x == 0.0f && y == 0.0f && z == 0.0f)
				return true;
		}

		return false;
	}

	void Normalize() {
		float d = std::sqrt(x*x + y*y + z*z);
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

	float DistanceTo(const Vector3& target) {
		float dx = target.x - x;
		float dy = target.y - y;
		float dz = target.z - z;
		return (float)std::sqrt(dx*dx + dy*dy + dz*dz);
	}

	float DistanceSquaredTo(const Vector3& target) {
		float dx = target.x - x;
		float dy = target.y - y;
		float dz = target.z - z;
		return (float)dx*dx + dy*dy + dz*dz;
	}

	float angle(const Vector3& other) const {
		Vector3 A(x, y, z);
		Vector3 B(other.x, other.y, other.z);
		A.Normalize();
		B.Normalize();

		float dot = A.dot(B);
		if (dot > 1.0)
			return 0.0f;
		else if (dot < -1.0f)
			return PI;
		else if (dot == 0.0f)
			return PI / 2.0f;

		return acosf(dot);
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

struct Vector4 {
	float x;
	float y;
	float z;
	float w;

	Vector4() {
		x = y = z = w = 0.0f;
	}
	Vector4(float X, float Y, float Z, float W) {
		x = X;
		y = Y;
		z = Z;
		w = W;
	}
};

struct Color3 {
	float r;
	float g;
	float b;

	Color3() {
		r = g = b = 0.0f;
	}
	Color3(const float& r, const float& g, const float& b) {
		this->r = r;
		this->g = g;
		this->b = b;
	}

	bool operator == (const Color3& other) {
		return (r == other.r && g == other.g && b == other.b);
	}
	bool operator != (const Color3& other) {
		return !(*this == other);
	}

	Color3& operator *= (const float& val) {
		r *= val;
		g *= val;
		b *= val;
		return *this;
	}
	Color3 operator * (const float& val) const {
		Color3 tmp = *this;
		tmp *= val;
		return tmp;
	}

	Color3& operator /= (const float& val) {
		r /= val;
		g /= val;
		b /= val;
		return *this;
	}
	Color3 operator / (const float& val) const {
		Color3 tmp = *this;
		tmp /= val;
		return tmp;
	}
};

struct Color4 {
	float r;
	float g;
	float b;
	float a;

	Color4() {
		r = g = b = a = 0.0f;
	}
	Color4(const float& r, const float& g, const float& b, const float& a) {
		this->r = r;
		this->g = g;
		this->b = b;
		this->a = a;
	}

	bool operator == (const Color4& other) {
		return (r == other.r && g == other.g && b == other.b && a == other.a);
	}
	bool operator != (const Color4& other) {
		return !(*this == other);
	}

	Color4& operator *= (const float& val) {
		r *= val;
		g *= val;
		b *= val;
		a *= val;
		return *this;
	}
	Color4 operator * (const float& val) const {
		Color4 tmp = *this;
		tmp *= val;
		return tmp;
	}

	Color4& operator /= (const float& val) {
		r /= val;
		g /= val;
		b /= val;
		a /= val;
		return *this;
	}
	Color4 operator / (const float& val) const {
		Color4 tmp = *this;
		tmp /= val;
		return tmp;
	}
};

struct ByteColor3 {
	byte r = 0;
	byte g = 0;
	byte b = 0;

	bool operator == (const ByteColor3& other) {
		return (r == other.r && g == other.g && b == other.b);
	}
	bool operator != (const ByteColor3& other) {
		return !(*this == other);
	}
};

struct ByteColor4 {
	byte r = 0;
	byte g = 0;
	byte b = 0;
	byte a = 0;

	bool operator == (const ByteColor4& other) {
		return (r == other.r && g == other.g && b == other.b && a == other.a);
	}
	bool operator != (const ByteColor4& other) {
		return !(*this == other);
	}
};

class Matrix3 {
	Vector3 rows[3] = {
		Vector3(1.0f, 0.0f, 0.0f),
		Vector3(0.0f, 1.0f, 0.0f),
		Vector3(0.0f, 0.0f, 1.0f)
	};

public:
	Vector3& operator[] (int index) {
		return rows[index];
	}

	const Vector3& operator[] (int index) const {
		return rows[index];
	}

	bool operator==(const Matrix3& other) {
		return rows[0] == other[0]
			&& rows[1] == other[1]
			&& rows[2] == other[2];
	}

	bool IsIdentity() {
		return *this == Matrix3();
	}

	Matrix3& Identity() {
		//1.0f, 0.0f, 0.0f
		//0.0f, 1.0f, 0.0f
		//0.0f, 0.0f, 1.0f

		rows[0].Zero();
		rows[1].Zero();
		rows[2].Zero();
		rows[0].x = 1.0f;
		rows[1].y = 1.0f;
		rows[2].z = 1.0f;
		return *this;
	}

	Matrix3 operator+(const Matrix3& other) const {
		Matrix3 res(*this);
		res += other;
		return res;
	}

	Matrix3& operator+=(const Matrix3& other) {
		rows[0] += other[0];
		rows[1] += other[1];
		rows[2] += other[2];
		return (*this);
	}

	Matrix3 operator-(const Matrix3& other) const {
		Matrix3 res(*this);
		res -= other;
		return res;
	}

	Matrix3& operator-=(const Matrix3& other) {
		rows[0] -= other[0];
		rows[1] -= other[1];
		rows[2] -= other[2];
		return(*this);
	}

	Matrix3& operator*=(const Matrix3& other) {
		Matrix3 res;
		res[0].x = rows[0].x * other[0].x + rows[1].x * other[0].y + rows[2].x * other[0].z;
		res[0].y = rows[0].y * other[0].x + rows[1].y * other[0].y + rows[2].y * other[0].z;
		res[0].z = rows[0].z * other[0].x + rows[1].z * other[0].y + rows[2].z * other[0].z;
		res[1].x = rows[0].x * other[1].x + rows[1].x * other[1].y + rows[2].x * other[1].z;
		res[1].y = rows[0].y * other[1].x + rows[1].y * other[1].y + rows[2].y * other[1].z;
		res[1].z = rows[0].z * other[1].x + rows[1].z * other[1].y + rows[2].z * other[1].z;
		res[2].x = rows[0].x * other[2].x + rows[1].x * other[2].y + rows[2].x * other[2].z;
		res[2].y = rows[0].y * other[2].x + rows[1].y * other[2].y + rows[2].y * other[2].z;
		res[2].z = rows[0].z * other[2].x + rows[1].z * other[2].y + rows[2].z * other[2].z;

		*this = res;
		return (*this);
	}

	Matrix3 operator*(const Matrix3& other) {
		Matrix3 res;
		res *= other;
		return res;
	}

	// Set rotation matrix from yaw, pitch and roll
	static Matrix3 MakeRotation(const float yaw, const float pitch, const float roll) {
		float ch = std::cosf(yaw);
		float sh = std::sinf(yaw);
		float cp = std::cosf(pitch);
		float sp = std::sinf(pitch);
		float cb = std::cosf(roll);
		float sb = std::sinf(roll);

		Matrix3 rot;
		rot[0].x = ch * cb + sh * sp * sb;
		rot[0].y = sb * cp;
		rot[0].z = -sh * cb + ch * sp * sb;

		rot[1].x = -ch * sb + sh * sp * cb;
		rot[1].y = cb * cp;
		rot[1].z = sb * sh + ch * sp * cb;

		rot[2].x = sh * cp;
		rot[2].y = -sp;
		rot[2].z = ch * cp;

		return rot;
	}
};

// 4D Matrix class for calculating and applying transformations.
class Matrix4 {
	float m[16];

public:
	Matrix4(){
		Identity();
	}

	Matrix4(const std::vector<Vector3>& mat33) {
		Set(mat33);
	}

	void Set(Vector3 mat33[3]) {
		m[0] = mat33[0].x; m[1] = mat33[0].y; m[2] = mat33[0].z; m[3] = 0;
		m[4] = mat33[1].x; m[5] = mat33[1].y; m[6] = mat33[1].z; m[7] = 0;
		m[8] = mat33[2].x; m[9] = mat33[2].y; m[10] = mat33[2].z; m[11] = 0;
		m[12] = 0;		   m[13] = 0;		  m[14] = 0;	      m[15] = 1;
	}

	void Set(const std::vector<Vector3>& mat33) {
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

	bool operator==(const Matrix4& other) {
		return (std::equal(m, m + sizeof m / sizeof *m, other.m));
	}

	bool IsIdentity() {
		return *this == Matrix4();
	}

	Matrix4& Identity() {
		std::memset(m, 0, sizeof(float) * 16);
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
		if (det == 0.0f) {
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
		float c = std::cosf(radAngle);
		float s = std::sinf(radAngle);

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
		t.m[3] = 0.0f;

		t.m[4] = xy * ic + z * s;
		t.m[5] = yy * ic + c;
		t.m[6] = yz * ic - x * s;
		t.m[7] = 0.0f;

		t.m[8] = xz * ic - y * s;
		t.m[9] = yz * ic + x * s;
		t.m[10] = zz * ic + c;

		t.m[11] = t.m[12] = t.m[13] = t.m[14] = 0.0f;
		t.m[15] = 1.0f;

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
	float radius = 0.0f;

	BoundingSphere() {}

	BoundingSphere(const Vector3& center, const float radius) {
		this->center = center;
		this->radius = radius;
	}

	// Miniball algorithm
	BoundingSphere(const std::vector<Vector3>& vertices);
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

struct QuaternionXYZW {
	float x;
	float y;
	float z;
	float w;

	QuaternionXYZW() {
		x = 0.0f;
		y = 0.0f;
		z = 0.0f;
		w = 1.0f;
	}

	QuaternionXYZW(float x, float y, float z, float w) {
		this->x = x;
		this->y = y;
		this->z = z;
		this->w = w;
	}
};


struct QuatTransform {
	Vector3 translation;
	Quaternion rotation;
	float scale = 1.0f;
};

struct MatTransform {
	Vector3 translation;
	Matrix3 rotation;
	float scale = 1.0f;

	void Clear() {
		translation.Zero();
		rotation.Identity();
		scale = 1.0f;
	}

	// Rotation in euler degrees (Yaw, Pitch, Roll)
	bool ToEulerDegrees(float &y, float& p, float& r) {
		float rx, ry, rz;
		bool canRot = false;

		if (rotation[0].z < 1.0f) {
			if (rotation[0].z > -1.0f) {
				rx = atan2(-rotation[1].z, rotation[2].z);
				ry = asin(rotation[0].z);
				rz = atan2(-rotation[0].y, rotation[0].x);
				canRot = true;
			}
			else {
				rx = -atan2(-rotation[1].x, rotation[1].y);
				ry = -PI / 2.0f;
				rz = 0.0f;
			}
		}
		else {
			rx = atan2(rotation[1].x, rotation[1].y);
			ry = PI / 2.0f;
			rz = 0.0f;
		}

		y = rx * 180.0f / PI;
		p = ry * 180.0f / PI;
		r = rz * 180.0f / PI;
		return canRot;
	}

	// Full matrix of translation, rotation and scale
	Matrix4 ToMatrix() {
		Matrix4 mat;
		mat[0] = rotation[0].x * scale;
		mat[1] = rotation[0].y;
		mat[2] = rotation[0].z;
		mat[3] = translation.x;
		mat[4] = rotation[1].x;
		mat[5] = rotation[1].y * scale;
		mat[6] = rotation[1].z;
		mat[7] = translation.y;
		mat[8] = rotation[2].x;
		mat[9] = rotation[2].y;
		mat[10] = rotation[2].z * scale;
		mat[11] = translation.z;
		return mat;
	}
};


struct Triangle {
	ushort p1;
	ushort p2;
	ushort p3;

	Triangle() {
		p1 = p2 = p3 = 0.0f;
	}
	Triangle(ushort P1, ushort P2, ushort P3) {
		p1 = P1;
		p2 = P2;
		p3 = P3;
	}

	void set(ushort P1, ushort P2, ushort P3) {
		p1 = P1; p2 = P2; p3 = P3;
	}

	void trinormal(Vector3* vertref, Vector3* outNormal) {
		outNormal->x = (vertref[p2].y - vertref[p1].y) * (vertref[p3].z - vertref[p1].z) - (vertref[p2].z - vertref[p1].z) * (vertref[p3].y - vertref[p1].y);
		outNormal->y = (vertref[p2].z - vertref[p1].z) * (vertref[p3].x - vertref[p1].x) - (vertref[p2].x - vertref[p1].x) * (vertref[p3].z - vertref[p1].z);
		outNormal->z = (vertref[p2].x - vertref[p1].x) * (vertref[p3].y - vertref[p1].y) - (vertref[p2].y - vertref[p1].y) * (vertref[p3].x - vertref[p1].x);
	}

	void trinormal(const std::vector<Vector3>& vertref, Vector3* outNormal) {
		outNormal->x = (vertref[p2].y - vertref[p1].y) * (vertref[p3].z - vertref[p1].z) - (vertref[p2].z - vertref[p1].z) * (vertref[p3].y - vertref[p1].y);
		outNormal->y = (vertref[p2].z - vertref[p1].z) * (vertref[p3].x - vertref[p1].x) - (vertref[p2].x - vertref[p1].x) * (vertref[p3].z - vertref[p1].z);
		outNormal->z = (vertref[p2].x - vertref[p1].x) * (vertref[p3].y - vertref[p1].y) - (vertref[p2].y - vertref[p1].y) * (vertref[p3].x - vertref[p1].x);
	}

	void midpoint(Vector3* vertref, Vector3& outPoint) {
		outPoint = vertref[p1];
		outPoint += vertref[p2];
		outPoint += vertref[p3];
		outPoint /= 3;
	}

	float AxisMidPointY(Vector3* vertref) {
		return (vertref[p1].y + vertref[p2].y + vertref[p3].y) / 3.0f;
	}

	float AxisMidPointX(Vector3* vertref) {
		return (vertref[p1].x + vertref[p2].x + vertref[p3].x) / 3.0f;
	}

	float AxisMidPointZ(Vector3* vertref) {
		return (vertref[p1].z + vertref[p2].z + vertref[p3].z) / 3.0f;
	}

	bool IntersectRay(Vector3* vertref, Vector3& origin, Vector3& direction, float* outDistance = nullptr, Vector3* worldPos = nullptr) {
		Vector3 c0(vertref[p1].x, vertref[p1].y, vertref[p1].z);
		Vector3 c1(vertref[p2].x, vertref[p2].y, vertref[p2].z);
		Vector3 c2(vertref[p3].x, vertref[p3].y, vertref[p3].z);

		Vector3 e1 = c1 - c0;
		Vector3 e2 = c2 - c0;
		float u, v;

		Vector3 pvec = direction.cross(e2);
		float det = e1.dot(pvec);

		if (det < 0.000001f)
			return false;

		Vector3 tvec = origin - c0;
		u = tvec.dot(pvec);
		if (u < 0 || u > det)
			return false;

		Vector3 qvec = tvec.cross(e1);
		v = direction.dot(qvec);
		if (v < 0 || u + v > det)
			return false;

		float dist = e2.dot(qvec);
		if (dist < 0)
			return false;

		dist *= (1.0f / det);

		if (outDistance) (*outDistance) = dist;
		if (worldPos) (*worldPos) = origin + (direction * dist);

		return true;
	}

	// Triangle/Sphere collision psuedocode by Christer Ericson: http://realtimecollisiondetection.net/blog/?p=103
	//   separating axis test on seven features --  3 points, 3 edges, and the tri plane.  For a sphere, this
	//   involves finding the minimum distance to each feature from the sphere origin and comparing it to the sphere radius.
	bool IntersectSphere(Vector3 *vertref, Vector3 &origin, float radius, float* outDistance = nullptr) {
		//A = A - P
		//B = B - P
		//C = C - P

		// Triangle points A,B,C.  translate them so the sphere's origin is their origin
		Vector3 A(vertref[p1].x, vertref[p1].y, vertref[p1].z);
		A = A - origin;
		Vector3 B(vertref[p2].x, vertref[p2].y, vertref[p2].z);
		B = B - origin;
		Vector3 C(vertref[p3].x, vertref[p3].y, vertref[p3].z);
		C = C - origin;

		//rr = r * r
		// Squared radius to avoid sqrts.
		float rr = radius * radius;
		//V = cross(B - A, C - A)

		// first test: tri plane.  Calculate the normal V
		Vector3 AB = B - A;
		Vector3 AC = C - A;
		Vector3 V = AB.cross(AC);
		//d = dot(A, V)
		//e = dot(V, V)
		// optimized distance test of the plane to the sphere -- removing sqrts and divides
		float d = A.dot(V);
		float e = V.dot(V);		// e = squared normal vector length -- the normalization factor
		//sep1 = d * d > rr * e
		if (d*d > rr * e)
			return false;

		//aa = dot(A, A)
		//ab = dot(A, B)
		//ac = dot(A, C)
		//bb = dot(B, B)
		//bc = dot(B, C)
		//cc = dot(C, C)

		// second test: tri points.  A sparating axis exists if a point lies outside the sphere, and the other tri points aren't on the other side of the sphere.
		float aa = A.dot(A);	// dist to point A
		float ab = A.dot(B);
		float ac = A.dot(C);
		float bb = B.dot(B);	// dist to point B
		float bc = B.dot(C);
		float cc = C.dot(C);	// dist to point C
		bool sep2 = (aa > rr) && (ab > aa) && (ac > aa);
		bool sep3 = (bb > rr) && (ab > bb) && (bc > bb);
		bool sep4 = (cc > rr) && (ac > cc) && (bc > cc);

		if (sep2 | sep3 | sep4)
			return false;

		//AB = B - A
		Vector3 BC = C - B;
		Vector3 CA = A - C;

		float d1 = ab - aa;
		d1 = A.dot(AB);
		float d2 = bc - bb;
		d2 = B.dot(BC);
		float d3 = ac - cc;
		d3 = C.dot(CA);

		//e1 = dot(AB, AB)
		//e2 = dot(BC, BC)
		//e3 = dot(CA, CA)
		float e1 = AB.dot(AB);
		float e2 = BC.dot(BC);
		float e3 = CA.dot(CA);

		//Q1 = A * e1 - d1 * AB
		//Q2 = B * e2 - d2 * BC
		//Q3 = C * e3 - d3 * CA
		//QC = C * e1 - Q1
		//QA = A * e2 - Q2
		//QB = B * e3 - Q3
		Vector3 Q1 = (A * e1) - (AB * d1);
		Vector3 Q2 = (B * e2) - (BC * d2);
		Vector3 Q3 = (C * e3) - (CA * d3);
		Vector3 QC = (C * e1) - Q1;
		Vector3 QA = (A * e2) - Q2;
		Vector3 QB = (B * e3) - Q3;

		//sep5 = [dot(Q1, Q1) > rr * e1 * e1] & [dot(Q1, QC) > 0]
		//sep6 = [dot(Q2, Q2) > rr * e2 * e2] & [dot(Q2, QA) > 0]
		//sep7 = [dot(Q3, Q3) > rr * e3 * e3] & [dot(Q3, QB) > 0]

		bool sep5 = (Q1.dot(Q1) > (rr * e1 * e1)) && (Q1.dot(QC) > 0);
		bool sep6 = (Q2.dot(Q2) > (rr * e2 * e2)) && (Q2.dot(QA) > 0);
		bool sep7 = (Q3.dot(Q3) > (rr * e3 * e3)) && (Q3.dot(QB) > 0);
		//separated = sep1 | sep2 | sep3 | sep4 | sep5 | sep6 | sep7
		if (sep5 | sep6 | sep7)
			return false;

		if (outDistance)
			(*outDistance) = std::min({ vertref[p1].DistanceTo(origin), vertref[p2].DistanceTo(origin), vertref[p3].DistanceTo(origin) });

		return true;
	}

	bool operator < (const Triangle& other) const {
		int d = 0;
		if (d == 0) d = p1 - other.p1;
		if (d == 0) d = p2 - other.p2;
		if (d == 0) d = p3 - other.p3;
		return d < 0;
	}

	bool operator == (const Triangle& other) const {
		return (p1 == other.p1 && p2 == other.p2 && p3 == other.p3);
	}

	bool CompareIndices(const Triangle& other) const {
		return ((p1 == other.p1 || p1 == other.p2 || p1 == other.p3)
			&& (p2 == other.p1 || p2 == other.p2 || p2 == other.p3)
			&& (p3 == other.p1 || p3 == other.p2 || p3 == other.p3));
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
	template<> struct std::hash < Edge > {
		std::size_t operator() (const Edge& t) const {
			return ((t.p2 << 16) | (t.p1 & 0xFFFF));
		}
	};

	template <> struct std::equal_to < Edge > {
		bool operator() (const Edge& t1, const Edge& t2) const {
			return ((t1.p1 == t2.p1) && (t1.p2 == t2.p2));
		}
	};

	template <> struct std::hash < Triangle > {
		std::size_t operator() (const Triangle& t) const {
			char* d = (char*)&t;
			std::size_t len = sizeof(Triangle);
			std::size_t hash, i;
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

	template <> struct std::equal_to < Triangle > {
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
