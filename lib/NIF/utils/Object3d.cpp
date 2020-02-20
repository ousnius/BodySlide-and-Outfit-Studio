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
