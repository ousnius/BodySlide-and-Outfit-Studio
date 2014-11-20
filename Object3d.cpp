#include "Object3d.h"

vec3& vec3::operator =(const vtx& other) {	 
	x = other.x;
	y = other.y;
	z = other.z;
	return (*this);
}	
vec3& vec3::operator += (const vtx& other) {
	x += other.x; y += other.y; z += other.z;
	return (*this);
}

tri::tri() {
	p1=p2=p3=0.0f;
}
tri::tri(unsigned short P1, unsigned short P2, unsigned short P3) {
	p1=P1; p2=P2; p3=P3;
}
void tri::trinormal(vtx* vertref, vec3* outNormal) {
	vtx va(vertref[p2].x-vertref[p1].x,vertref[p2].y-vertref[p1].y,vertref[p2].z-vertref[p1].z);
	vtx vb(vertref[p3].x-vertref[p1].x,vertref[p3].y-vertref[p1].y,vertref[p3].z-vertref[p1].z);

	outNormal->x = va.y*vb.z - va.z*vb.y;
	outNormal->y = va.z*vb.x - va.x*vb.z;
	outNormal->z = va.x*vb.y - va.y*vb.x;
}
void tri::midpoint(vtx* vertref, vec3& outPoint) {
	outPoint = vertref[p1];
	outPoint += vertref[p2];
	outPoint += vertref[p3];
	outPoint /= 3;

}
float tri::AxisMidPointY(vtx* vertref) {
	return (vertref[p1].y+vertref[p2].y+vertref[p3].y)/3.0f;
}
float tri::AxisMidPointX(vtx* vertref) {
	return (vertref[p1].x+vertref[p2].x+vertref[p3].x)/3.0f;
}
float tri::AxisMidPointZ(vtx* vertref) {
	return (vertref[p1].z+vertref[p2].z+vertref[p3].z)/3.0f;
}

bool tri::IntersectRay(vtx* vertref, vec3& origin, vec3& direction, float* outDistance, vec3* worldPos) {
	vec3 c0 (vertref[p1].x,vertref[p1].y,vertref[p1].z); //(*(vec3*)&vertref[p1]);
	vec3 c1 (vertref[p2].x,vertref[p2].y,vertref[p2].z); //(*(vec3*)&vertref[p2]);
	vec3 c2 (vertref[p3].x,vertref[p3].y,vertref[p3].z); //(*(vec3*)&vertref[p3]);

	vec3 e1 = c1 - c0;
    vec3 e2 = c2 - c0;
	float u, v;

    vec3 pvec = direction.cross(e2);
    float det = e1.dot(pvec);

    if(det<.000001f)
        return false;

    vec3 tvec = origin - c0;
    u = tvec.dot(pvec);
    if (u < 0 || u > det) return false;
    
    vec3 qvec = tvec.cross(e1);
    v = direction.dot(qvec);
    if (v < 0 || u + v > det) return false;
    float dist = e2.dot(qvec);
    if (dist < 0)
        return false;
    dist *= (1.0f / det);

	if(outDistance) (*outDistance) = dist;
	if(worldPos)    (*worldPos) = origin + (direction*dist);

    return true;
}

// Triangle/Sphere collision psuedocode by Christer Ericson: http://realtimecollisiondetection.net/blog/?p=103
//   separating axis test on seven features --  3 points, 3 edges, and the triangle plane.  For a sphere, this
//   involves finding the minimum distance to each feature from the sphere origin and comparing it to the sphere radius.
bool tri::IntersectSphere(vtx *vertref, vec3 &origin, float radius) {
//A = A - P
//B = B - P
//C = C - P
	// Triangle points A,B,C.  translate them so the sphere's origin is their origin
	vec3 A(vertref[p1].x,vertref[p1].y,vertref[p1].z);
	A = A - origin;
	vec3 B(vertref[p2].x,vertref[p2].y,vertref[p2].z); 
	B = B - origin;
	vec3 C(vertref[p3].x,vertref[p3].y,vertref[p3].z) ;
	C = C - origin;
//rr = r * r
	// Squared radius to avoid sqrts.
	float rr = radius*radius;
//V = cross(B - A, C - A)

	// first test: triangle plane.  Calculate the normal V
	vec3 AB = B - A;
	vec3 AC = C - A;
	vec3 V = AB.cross(AC);
//d = dot(A, V)
//e = dot(V, V)
	// optimized distance test of the plane to the sphere -- removing sqrts and divides
	float d = A.dot(V);
	float e = V.dot(V);		// e = squared normal vector length -- the normalization factor
//sep1 = d * d > rr * e
	if(d*d > rr * e) 
		return false;
//aa = dot(A, A)
//ab = dot(A, B)
//ac = dot(A, C)
//bb = dot(B, B)
//bc = dot(B, C)
//cc = dot(C, C)

	// second test: triangle points.  A sparating axis exists if a point lies outside the sphere, and the other triangle points aren't on the other side of the sphere.
	float aa = A.dot(A);	// dist to point A
	float ab = A.dot(B);
	float ac = A.dot(C);
	float bb = B.dot(B);	// dist to point B
	float bc = B.dot(C);
	float cc = C.dot(C);	// dist to point C
	bool sep2 = (aa > rr) && (ab > aa) && (ac > aa);
	bool sep3 = (bb > rr) && (ab > bb) && (bc > bb);
	bool sep4 = (cc > rr) && (ac > cc) && (bc > cc);
	
	if(sep2 | sep3 | sep4) {
		return false;
	}

//AB = B - A
	vec3 BC = C - B;
	vec3 CA = A - C;

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
	vec3 Q1 = (A * e1) - (AB * d1);
	vec3 Q2 = (B * e2) - (BC * d2);
	vec3 Q3 = (C * e1) - (CA * d3);
	vec3 QC = (C * e1) - Q1;
	vec3 QA = (A * e2) - Q2;
	vec3 QB = (B * e3) - Q3;

//sep5 = [dot(Q1, Q1) > rr * e1 * e1] & [dot(Q1, QC) > 0]
//sep6 = [dot(Q2, Q2) > rr * e2 * e2] & [dot(Q2, QA) > 0]
//sep7 = [dot(Q3, Q3) > rr * e3 * e3] & [dot(Q3, QB) > 0]

	bool sep5 = (Q1.dot(Q1) > rr * e1 * e1) && (Q1.dot(QC) > 0);
	bool sep6 = (Q2.dot(Q2) > rr * e2 * e2) && (Q2.dot(QA) > 0);
	bool sep7 = (Q3.dot(Q3) > rr * e3 * e3) && (Q3.dot(QB) > 0);
	//separated = sep1 | sep2 | sep3 | sep4 | sep5 | sep6 | sep7
	if(sep5 | sep6 | sep7)
		return false;
	return true;
}
