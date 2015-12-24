/*
BodySlide and Outfit Studio
Copyright (C) 2015  Caliente & ousnius
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


int  ObjFile::AddGroup(const string& name, const vector<Vector3>& verts, const vector<Face>& faces, const vector<Vector2>& uvs) {
	if (name.empty() || verts.empty())
		return 1;

	ObjData* newData = new ObjData();
	newData->name = name;
	newData->verts = verts;
	newData->faces = faces;
	newData->uvs = uvs;

	data[name] = newData;
	return 0;
}

int ObjFile::LoadSimple(const string &inFn, const string& groupName){
	fstream base(inFn.c_str(), ios_base::in | ios_base::binary);
	if (base.fail())
		return 1;

	ObjData* di = new ObjData();

	string dump;
	Vector3 v;
	Vector2 uv;
	Triangle t;
	string curgrp;
	string facept1;
	string facept2;
	string facept3;
	string facept4;
	size_t pos;
	int ft[4];
	vector<Vector3> verts;
	vector<Vector2> uvs;
	vector<Triangle> tris;
	bool readgroup = true;

	base >> dump;
	while (!base.eof()) {
		if (dump.compare("v") == 0) {
			base >> v.x >> v.y >> v.z;
			di->verts.push_back(v);
		}
		else if (dump.compare("g") == 0 || dump.compare("o") == 0) {
			base >> curgrp;

			if (di->name != "") {
				data[di->name] = di;
				di = new ObjData;
			}

			di->name = curgrp;
			objGroups.push_back(curgrp);

			if (groupName.length() > 0) {
				if (curgrp.compare(groupName) == 0)
					readgroup = true;
				else
					readgroup = false;
			}
		}
		else if (dump.compare("vt") == 0) {
			base >> uv.u >> uv.v;
			uv.v = 1.0f - uv.v;
			di->uvs.push_back(uv);
		}
		else if (dump.compare("f") == 0) {
			base >> facept1 >> facept2 >> facept3;
			pos = facept1.find('/');
			t.p1 = atoi(facept1.c_str()) - 1;
			ft[0] = atoi(facept1.substr(pos + 1).c_str()) - 1;
			pos = facept2.find('/');
			t.p2 = atoi(facept2.c_str()) - 1;
			ft[1] = atoi(facept2.substr(pos + 1).c_str()) - 1;
			pos = facept3.find('/');
			t.p3 = atoi(facept3.c_str()) - 1;
			ft[2] = atoi(facept3.substr(pos + 1).c_str()) - 1;

			di->tris.push_back(t);
		}
		base >> dump;

	}
	data[di->name] = di;
	base.close();
	return 0;
}

int ObjFile::LoadForNif(const string &inFn, const string& groupName) {
	fstream base(inFn.c_str(), ios_base::in | ios_base::binary);
	if (base.fail())
		return 1;

	LoadForNif(base, groupName);
	base.close();
	return 0;
}

int ObjFile::LoadForNif(fstream &base, const string& groupName) {
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
	vector<Triangle> tris;
	size_t pos;
	map<int, vector<VertUV>> vertMap;
	map<int, vector<VertUV>>::iterator savedVert;

	bool gotface = false;
	bool readgroup = true;

	while (!base.eof()) {
		if (!gotface)
			base >> dump;
		else
			gotface = false;

		if (dump.compare("v") == 0) {
			base >> v.x >> v.y >> v.z;
			verts.push_back(v);
		}
		else if (dump.compare("g") == 0 || dump.compare("o") == 0 || dump.compare("s")==0) {
			base >> curgrp;

			if (di->name != "") {
				data[di->name] = di;
				di = new ObjData;
			}

			di->name = curgrp;
			objGroups.push_back(curgrp);

			if (groupName.length() > 0) {
				if (curgrp.compare(groupName) == 0)
					readgroup = true;
				else
					readgroup = false;
			}
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
			base >> facept4;

			if (facept4 == "f") {
				gotface = true;
				dump = "f";
				nPoints = 3;
			}
			else if (facept4 == "g") {
				gotface = true;
				dump = "g";
				nPoints = 3;
			}
			else if (facept4 == "s") {
				gotface = true;
				dump = "s";
				nPoints = 3;
			}
			else if (facept4.length() > 0) {
				pos = facept4.find('/');
				if (pos == string::npos) {
					gotface = true;
					dump = "f";
					nPoints = 3;
				}
				else {
					f[3] = atoi(facept4.c_str()) - 1;
					ft[3] = atoi(facept4.substr(pos + 1).c_str()) - 1;
					nPoints = 4;
				}
			}

			if (f[0] == -1 || f[1] == -1 || f[2] == -1)
				continue;

			if (!readgroup)
				continue;

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
					vertMap[f[i]].push_back(VertUV(v_idx[i], ft[i]));
					di->verts.push_back(verts[f[i]]);
					if (uvs.size() > 0) {
						di->uvs.push_back(uvs[ft[i]]);
					}
				}
			}
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
	data[di->name] = di;
	return 0;
}

int ObjFile::LoadVertOrderMap(const string &inFn, map<int, int>& outMap, vector<Face>& origFaces, vector<Vector2>& origUVs, const string& groupName) {
	fstream base(inFn.c_str(), ios_base::in | ios_base::binary);
	if (base.fail())
		return 1;

	LoadVertOrderMap(base,outMap, origFaces, origUVs, groupName);
	base.close();
	return 0;
}

int ObjFile::LoadVertOrderMap(fstream &base, map<int, int>& outMap, vector<Face>& origFaces, vector<Vector2>& origUVs, const string& groupName) {
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
	vector<Triangle> tris;
	size_t pos;
	map<int, vector<VertUV>> vertMap;
	map<int, vector<VertUV>>::iterator savedVert;

	bool gotface = false;
	bool readgroup = true;

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

			if (groupName.length() > 0) {
				if (curgrp.compare(groupName) == 0)
					readgroup = true;
				else
					readgroup = false;
			}
		}
		else if (dump.compare("vt") == 0) {
			base >> uv.u >> uv.v;
			uv.v = 1.0f - uv.v;
			uvs.push_back(uv);
			origUVs.push_back(uv);
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
			base >> facept4;

			if (facept4 == "f") {
				gotface = true;
				dump = "f";
				nPoints = 3;
			}
			else if (facept4 == "g") {
				gotface = true;
				dump = "g";
				nPoints = 3;
			}
			else if (facept4 == "s") {
				gotface = true;
				dump = "s";
				nPoints = 3;
			}
			else if (facept4.length() > 0) {
				pos = facept4.find('/');
				if (pos == string::npos) {
					gotface = true;
					dump = "f";
					nPoints = 3;
				}
				else {
					f[3] = atoi(facept4.c_str()) - 1;
					ft[3] = atoi(facept4.substr(pos + 1).c_str()) - 1;
					nPoints = 4;
				}
			}

			if (f[0] == -1 || f[1] == -1 || f[2] == -1)
				continue;

			origFaces.emplace_back(nPoints, f, ft);

			if (!readgroup)
				continue;

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
					vertMap[f[i]].push_back(VertUV(v_idx[i], ft[i]));
					outMap[f[i]] = di->verts.size();
					di->verts.push_back(verts[f[i]]);
					if (uvs.size() > 0) {
						di->uvs.push_back(uvs[ft[i]]);
					}
				}
			}
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
	//data[di->name] = di;
	delete di;
	return 0;
}

int ObjFile::Load(const string &inFn, const string& groupName) {
	ifstream base(inFn.c_str(), ios_base::binary);
	if (base.is_open())
		Load(base, groupName);
	else
		return 1;

	return 0;
}

int ObjFile::Load(ifstream &base, const string& groupName) {
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

	Vector3 * verts;
	Vector2 * uvs;
	di->verts.resize(65536);
	di->uvs.resize(65536);

	int vcursor = 0;

	verts = (Vector3*)malloc(sizeof(Vector3) * 65536);
	uvs = (Vector2*)malloc(sizeof(Vector2) * 65536);

	size_t pos;
	unordered_map<int, vector<VertUV>> vertMap;
	unordered_map<int, vector<VertUV>>::iterator savedVert;

	int vc = 0;
	int uvc = 0;

	int stage = 0;
	bool gotface = false;
	bool readgroup = true;

	while (!base.eof()) {
		if (!gotface)
			base >> dump;
		else
			gotface = false;

		// Loading Vertices
		if (dump.compare("v") == 0) {
			if (stage == 0)
				stage++;

			base >> verts[vc].x >> verts[vc].y >> verts[vc].z;
			vc++;
		}
		else if (dump.compare("g") == 0 || dump.compare("o") == 0) {
			base >> curgrp;

			if (di->name != "") {
				data[di->name] = di;
				di = new ObjData;
			}

			di->name = curgrp;
			objGroups.push_back(curgrp);

			if (groupName.length() > 0) {
				if (curgrp.compare(groupName) == 0)
					readgroup = true;
				else
					readgroup = false;
			}
		}
		// Loading UVs
		else if (dump.compare("vt") == 0) {
			if (stage == 1)
				stage++;

			base >> uvs[uvc].u >> uvs[uvc].v;
			uvc++;
		}
		// Loading Faces
		else if (dump.compare("f") == 0) {
			if (stage == 2)
				stage++;

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
			base >> facept4;

			if (facept4 == "f") {
				gotface = true;
				dump = "f";
				nPoints = 3;
			}
			else if (facept4 == "g") {
				gotface = true;
				dump = "g";
				nPoints = 3;
			}
			else if (facept4 == "s"){
				gotface = true;
				dump = "s";
				nPoints = 3;
			}
			else if (facept4.length() > 0) {
				pos = facept4.find('/');
				if (pos == string::npos) {
					gotface = true;
					dump = "f";
					nPoints = 3;
				}
				else {
					f[3] = atoi(facept4.c_str()) - 1;
					ft[3] = atoi(facept4.substr(pos + 1).c_str()) - 1;
					nPoints = 4;
				}
			}

			if (f[0] == -1 || f[1] == -1 || f[2] == -1)
				continue;

			if (!readgroup)
				continue;

			for (int i = 0; i < nPoints; i++) {
				v_idx[i] = vcursor;
				if ((savedVert = vertMap.find(f[i])) != vertMap.end()) {
					for (int j = 0; j < savedVert->second.size(); j++) {
						if (savedVert->second[j].uv == ft[i])
							v_idx[i] = savedVert->second[j].v;
						else {
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
				/**/
				if (v_idx[i] == vcursor) {
					vertMap[f[i]].push_back(VertUV(v_idx[i], ft[i]));
					di->verts[vcursor] = verts[f[i]];
					di->uvs[vcursor] = uvs[f[i]];
					vcursor++;
				}
			}
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
	data[di->name] = di;
	free(verts);
	free(uvs);

	return 0;
}

int ObjFile::Save(const string &fileName) {
	ofstream file(fileName.c_str(), ios_base::binary);
	if (file.fail())
		return 1;

	file << "# BodySlide OBJ export" << endl << endl;

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

		if (d.second->faces.size() > 0) {
			for (int i = 0; i < d.second->faces.size(); i++) {
				file << "f " << d.second->faces[i].p1 + 1 << "/" << d.second->faces[i].uv1 + 1 << " "
					<< d.second->faces[i].p2 + 1 << "/" << d.second->faces[i].uv2 + 1 << " "
					<< d.second->faces[i].p3 + 1 << "/" << d.second->faces[i].uv3 + 1;
				if (d.second->faces[i].nPoints == 4) {
					file << " " << d.second->faces[i].p4 + 1 << "/" << d.second->faces[i].uv4 + 1;
				}
				file << endl;
			}
		}
		else {
			for (int i = 0; i < d.second->tris.size(); i++) {
				file << "f " << d.second->tris[i].p1 + 1 << "/" << d.second->tris[i].p1 + 1 << " "
					<< d.second->tris[i].p2 + 1 << "/" << d.second->tris[i].p2 + 1 << " "
					<< d.second->tris[i].p3 + 1 << "/" << d.second->tris[i].p3 + 1
					<< endl;
			}
		}
		file << endl;
	}

	file.close();
	return 0;
}

bool ObjFile::CopyDataForGroup(const string &name, vector<Vector3> *v, vector<Triangle> *t, vector<Vector2> *uv) {
	int i;
	if (data.find(name) == data.end())
		return false;
	ObjData* od = data[name];
	if (v) {
		v->clear();
		v->resize(od->verts.size());
		for (i = 0; i < od->verts.size(); i++) {
			(*v)[i].x = (od->verts[i].x + offset.x) * scale.x;
			(*v)[i].y = (od->verts[i].y + offset.y) * scale.y;
			(*v)[i].z = (od->verts[i].z + offset.z) * scale.z;
		}
	}
	if (t) {
		t->clear();
		t->resize(od->tris.size());
		for (i = 0; i < od->tris.size(); i++) {
			(*t)[i] = od->tris[i];
		}
	}
	if (uv) {
		uv->clear();
		uv->resize(od->uvs.size());
		for (i = 0; i < od->uvs.size(); i++) {
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
