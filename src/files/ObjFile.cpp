/*
BodySlide and Outfit Studio
Copyright (C) 2016  Caliente & ousnius
See the included LICENSE file
*/

#include "ObjFile.h"

ObjFile::ObjFile() {
	scale = Vector3(1.0f, 1.0f, 1.0f);
	uvDupThreshold = 0.005f;
}

ObjFile::~ObjFile() {
	for (auto &d : data)
		delete d.second;
}

int ObjFile::AddGroup(const string& name, const vector<Vector3>& verts, const vector<Triangle>& tris, const vector<Vector2>& uvs) {
	if (name.empty() || verts.empty())
		return 1;

	ObjData* newData = new ObjData();
	newData->name = name;
	newData->verts = verts;
	newData->tris = tris;
	newData->uvs = uvs;

	data[name] = newData;
	return 0;
}

int ObjFile::LoadForNif(const string &fileName) {
	fstream base(fileName.c_str(), ios_base::in | ios_base::binary);
	if (base.fail())
		return 1;

	LoadForNif(base);
	base.close();
	return 0;
}

int ObjFile::LoadForNif(fstream &base) {
	ObjData* di = new ObjData();

	Vector3 v;
	Vector2 uv;
	Vector2 uv2;
	Triangle t;

	string dump;
	string curgrp;
	string facept1;
	string facept2;
	string facept3;
	string facept4;
	int f[4];
	int ft[4];
	int nPoints = 0;
	int v_idx[4];

	vector<Vector3> verts;
	vector<Vector2> uvs;
	size_t pos;
	map<int, vector<VertUV>> vertMap;
	map<int, vector<VertUV>>::iterator savedVert;

	bool gotface = false;

	while (!base.eof()) {
		if (!gotface)
			base >> dump;
		else
			gotface = false;

		if (dump.compare("v") == 0) {
			base >> v.x >> v.y >> v.z;
			verts.push_back(v);
		}
		else if (dump.compare("g") == 0 || dump.compare("o") == 0) {
			base >> curgrp;

			if (di->name != "") {
				data[di->name] = di;
				di = new ObjData;
			}

			di->name = curgrp;
			objGroups.push_back(curgrp);
		}
		else if (dump.compare("vt") == 0) {
			base >> uv.u >> uv.v;
			uv.v = 1.0f - uv.v;
			uvs.push_back(uv);
		}
		else if (dump.compare("f") == 0) {
			base >> facept1 >> facept2 >> facept3;
			pos = facept1.find('/');
			f[0] = atoi(facept1.c_str()) - 1;
			ft[0] = atoi(facept1.substr(pos + 1).c_str()) - 1;
			pos = facept2.find('/');
			f[1] = atoi(facept2.c_str()) - 1;
			ft[1] = atoi(facept2.substr(pos + 1).c_str()) - 1;
			pos = facept3.find('/');
			f[2] = atoi(facept3.c_str()) - 1;
			ft[2] = atoi(facept3.substr(pos + 1).c_str()) - 1;

			if (f[0] == -1 || f[1] == -1 || f[2] == -1)
				continue;

			base >> facept4;
			dump = facept4;

			nPoints = 3;
			gotface = true;

			if (facept4.length() > 0) {
				f[3] = atoi(facept4.c_str()) - 1;

				if (f[3] != -1) {
					pos = facept4.find('/');
					ft[3] = atoi(facept4.substr(pos + 1).c_str()) - 1;
					nPoints = 4;
					gotface = false;
				}
			}

			bool skipFace = false;
			for (int i = 0; i < nPoints; i++) {
				v_idx[i] = di->verts.size();
				if ((savedVert = vertMap.find(f[i])) != vertMap.end()) {
					for (int j = 0; j < savedVert->second.size(); j++) {
						if (savedVert->second[j].uv == ft[i])
							v_idx[i] = savedVert->second[j].v;
						else if (uvs.size() > 0) {
							uv = uvs[ft[i]];
							uv2 = uvs[savedVert->second[j].uv];
							if (fabs(uv.u - uv2.u) > uvDupThreshold) {
								v_idx[i] = v_idx[i];
								continue;
							}
							else if (fabs(uv.v - uv2.v) > uvDupThreshold) {
								v_idx[i] = v_idx[i];
								continue;
							}
							v_idx[i] = savedVert->second[j].v;
						}
					}
				}

				if (v_idx[i] == di->verts.size()) {
					if (verts.size() > f[i]) {
						di->verts.push_back(verts[f[i]]);

						if (uvs.size() > ft[i]) {
							vertMap[f[i]].push_back(VertUV(v_idx[i], ft[i]));
							di->uvs.push_back(uvs[ft[i]]);
						}
					}
					else {
						skipFace = true;
						break;
					}
				}
			}

			if (skipFace)
				continue;

			t.p1 = v_idx[0];
			t.p2 = v_idx[1];
			t.p3 = v_idx[2];
			di->tris.push_back(t);
			if (nPoints == 4) {
				t.p1 = v_idx[0];
				t.p2 = v_idx[2];
				t.p3 = v_idx[3];
				di->tris.push_back(t);
			}
		}
	}

	if (di->name.empty()) {
		di->name = "object";
		objGroups.push_back(di->name);
	}

	data[di->name] = di;
	return 0;
}


int ObjFile::Save(const string &fileName) {
	ofstream file(fileName.c_str(), ios_base::binary);
	if (file.fail())
		return 1;

	file << "# Outfit Studio - OBJ Export" << endl;
	file << "# https://github.com/ousnius/BodySlide-and-Outfit-Studio" << endl << endl;

	for (auto& d : data) {
		file << "g " << d.first << endl;
		file << "usemtl NoMaterial" << endl << endl;

		for (int i = 0; i < d.second->verts.size(); i++) {
			file << "v " << (d.second->verts[i].x + offset.x) * scale.x
				<< " " << (d.second->verts[i].y + offset.y) * scale.y
				<< " " << (d.second->verts[i].z + offset.z) * scale.z
				<< endl;
		}
		file << endl;

		for (int i = 0; i < d.second->uvs.size(); i++)
			file << "vt " << d.second->uvs[i].u << " " << (1.0f - d.second->uvs[i].v) << endl;
		file << endl;

		for (int i = 0; i < d.second->tris.size(); i++) {
			file << "f " << d.second->tris[i].p1 + 1 << "/" << d.second->tris[i].p1 + 1 << " "
				<< d.second->tris[i].p2 + 1 << "/" << d.second->tris[i].p2 + 1 << " "
				<< d.second->tris[i].p3 + 1 << "/" << d.second->tris[i].p3 + 1
				<< endl;
		}
		file << endl;
	}

	file.close();
	return 0;
}

bool ObjFile::CopyDataForGroup(const string &name, vector<Vector3> *v, vector<Triangle> *t, vector<Vector2> *uv) {
	if (data.find(name) == data.end())
		return false;

	ObjData* od = data[name];
	if (v) {
		v->clear();
		v->resize(od->verts.size());
		for (int i = 0; i < od->verts.size(); i++) {
			(*v)[i].x = (od->verts[i].x + offset.x) * scale.x;
			(*v)[i].y = (od->verts[i].y + offset.y) * scale.y;
			(*v)[i].z = (od->verts[i].z + offset.z) * scale.z;
		}
	}
	if (t) {
		t->clear();
		t->resize(od->tris.size());
		for (int i = 0; i < od->tris.size(); i++) {
			(*t)[i] = od->tris[i];
		}
	}
	if (uv) {
		uv->clear();
		uv->resize(od->uvs.size());
		for (int i = 0; i < od->uvs.size(); i++) {
			(*uv)[i] = od->uvs[i];
		}
	}
	return true;
}

bool ObjFile::CopyDataForIndex(int index, vector<Vector3> *v, vector<Triangle> *t, vector<Vector2> *uv) {
	if (objGroups.size() > index)
		return CopyDataForGroup(objGroups[index], v, t, uv);
	else
		return false;
}

void ObjFile::GetGroupList(std::vector<string> &shapeNames) {
	shapeNames.clear();
	for (int i = 0; i < objGroups.size(); i++)
		shapeNames.push_back(objGroups[i]);
}
