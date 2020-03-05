#include "Object3d.h"
#include "Miniball.hpp"

BoundingSphere::BoundingSphere(const std::vector<Vector3>& vertices) {
	if (vertices.empty())
		return;

	// Convert vertices to list of coordinates
	std::list<std::vector<float>> lp;
	for (int i = 0; i < vertices.size(); ++i) {
		std::vector<float> p(3);
		p[0] = vertices[i].x;
		p[1] = vertices[i].y;
		p[2] = vertices[i].z;
		lp.push_back(p);
	}

	// Create an instance of Miniball
	Miniball::Miniball < Miniball::CoordAccessor
		< std::list<std::vector<float>>::const_iterator,
		std::vector<float>::const_iterator >>
		mb(3, lp.begin(), lp.end());

	const float* pCenter = mb.center();
	center.x = pCenter[0];
	center.y = pCenter[1];
	center.z = pCenter[2];

	radius = sqrt(mb.squared_radius());
}

float Matrix3::Determinant() const {
	return
		rows[0][0]*(rows[1][1]*rows[2][2]-rows[1][2]*rows[2][1]) +
		rows[0][1]*(rows[1][2]*rows[2][0]-rows[1][0]*rows[2][2]) +
		rows[0][2]*(rows[1][0]*rows[2][1]-rows[1][1]*rows[2][0]);
}

bool Matrix3::Invert(Matrix3 *inverse) const {
	float det = Determinant();
	if (det == 0.0f) return false;
	float idet = 1/det;
	Matrix3 &im = *inverse;
	im[0][0] = (rows[1][1]*rows[2][2]-rows[1][2]*rows[2][1])*idet;
	im[1][0] = (rows[1][2]*rows[2][0]-rows[1][0]*rows[2][2])*idet;
	im[2][0] = (rows[1][0]*rows[2][1]-rows[1][1]*rows[2][0])*idet;
	im[0][1] = (rows[2][1]*rows[0][2]-rows[2][2]*rows[0][1])*idet;
	im[1][1] = (rows[2][2]*rows[0][0]-rows[2][0]*rows[0][2])*idet;
	im[2][1] = (rows[2][0]*rows[0][1]-rows[2][1]*rows[0][0])*idet;
	im[0][2] = (rows[0][1]*rows[1][2]-rows[0][2]*rows[1][1])*idet;
	im[1][2] = (rows[0][2]*rows[1][0]-rows[0][0]*rows[1][2])*idet;
	im[2][2] = (rows[0][0]*rows[1][1]-rows[0][1]*rows[1][0])*idet;
	return true;
}

Matrix3 Matrix3::Inverse() const {
	Matrix3 inv;
	Invert(&inv);
	return inv;
}

Matrix3 Matrix3::MakeRotation(const float yaw, const float pitch, const float roll) {
	float ch = std::cos(yaw);
	float sh = std::sin(yaw);
	float cp = std::cos(pitch);
	float sp = std::sin(pitch);
	float cb = std::cos(roll);
	float sb = std::sin(roll);

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

bool Matrix3::ToEulerAngles(float &y, float& p, float& r) const {
	bool canRot = false;

	if (rows[0].z < 1.0f) {
		if (rows[0].z > -1.0f) {
			y = atan2(-rows[1].z, rows[2].z);
			p = asin(rows[0].z);
			r = atan2(-rows[0].y, rows[0].x);
			canRot = true;
		}
		else {
			y = -atan2(-rows[1].x, rows[1].y);
			p = -PI / 2.0f;
			r = 0.0f;
		}
	}
	else {
		y = atan2(rows[1].x, rows[1].y);
		p = PI / 2.0f;
		r = 0.0f;
	}
	return canRot;
}

Vector3 MatTransform::ApplyTransform(const Vector3 &v) const {
	return translation + rotation * (v * scale);
}

MatTransform MatTransform::InverseTransform() const {
	MatTransform inv;
	inv.rotation = rotation.Inverse();
	inv.scale = 1 / scale;
	inv.translation = - inv.scale * (inv.rotation * translation);
	return inv;
}

MatTransform MatTransform::ComposeTransforms(const MatTransform &other) const {
	MatTransform comp;
	comp.rotation = rotation * other.rotation;
	comp.scale = scale * other.scale;
	comp.translation = translation + rotation * (scale * other.translation);
	return comp;
}

Matrix3 RotVecToMat(const Vector3 &v) {
	double angle = std::sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
	double cosang = std::cos(angle);
	double sinang = std::sin(angle);
	double onemcosang;	// One minus cosang
	// Avoid loss of precision from cancellation in calculating onemcosang
	if (cosang > .5)
		onemcosang = sinang * sinang / (1 + cosang);
	else
		onemcosang = 1 - cosang;
	Vector3 n = angle != 0 ? v / angle : Vector3(1,0,0);
	Matrix3 m;
	m[0][0] = n.x * n.x * onemcosang + cosang;
	m[1][1] = n.y * n.y * onemcosang + cosang;
	m[2][2] = n.z * n.z * onemcosang + cosang;
	m[0][1] = n.x * n.y * onemcosang + n.z * sinang;
	m[1][0] = n.x * n.y * onemcosang - n.z * sinang;
	m[1][2] = n.y * n.z * onemcosang + n.x * sinang;
	m[2][1] = n.y * n.z * onemcosang - n.x * sinang;
	m[2][0] = n.z * n.x * onemcosang + n.y * sinang;
	m[0][2] = n.z * n.x * onemcosang - n.y * sinang;
	return m;
}

Vector3 RotMatToVec(const Matrix3 &m) {
	double cosang = (m[0][0] + m[1][1] + m[2][2] - 1) * 0.5;
	if (cosang >= 1)
		return Vector3(0,0,0);
	else if (cosang > -1) {
		Vector3 v(m[1][2] - m[2][1], m[2][0] - m[0][2], m[0][1] - m[1][0]);
		v.Normalize();
		return v * std::acos(cosang);
	}
	else { // cosang <= -1, sinang == 0
		Vector3 v(
			std::sqrt((m[0][0] - cosang) * 0.5),
			std::sqrt((m[1][1] - cosang) * 0.5),
			std::sqrt((m[2][2] - cosang) * 0.5));
		v.Normalize();
		if (m[1][2] < m[2][1]) v.x = -v.x;
		if (m[2][0] < m[0][2]) v.y = -v.y;
		if (m[0][1] < m[1][0]) v.z = -v.z;
		return v * PI;
	}
}
