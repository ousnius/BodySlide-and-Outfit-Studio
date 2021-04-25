/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "ObjFile.h"
#include "../utils/PlatformUtil.h"

using namespace nifly;

ObjFile::ObjFile() {
}

ObjFile::~ObjFile() {
	for (auto &d : data)
		delete d.second;
}

int ObjFile::AddGroup(const std::string& name, const std::vector<Vector3>& verts, const std::vector<Triangle>& tris, const std::vector<Vector2>& uvs, const std::vector<Vector3>& norms) {
	if (name.empty() || verts.empty())
		return 1;

	ObjData* newData = new ObjData();
	newData->name = name;
	newData->verts = verts;
	newData->tris = tris;
	newData->uvs = uvs;
	newData->norms = norms;

	data[name] = newData;
	return 0;
}

int ObjFile::LoadForNif(const std::string& fileName, const ObjOptionsImport& options) {
	std::fstream file;
	PlatformUtil::OpenFileStream(file, fileName, std::ios::in | std::ios::binary);
	if (file.fail())
		return 1;

	LoadForNif(file, options);
	return 0;
}

int ObjFile::LoadForNif(std::fstream& base, const ObjOptionsImport& options) {
	constexpr auto maxVertexCount = std::numeric_limits<uint16_t>::max();

	auto current = new ObjData();

	Vector3 v;
	Vector2 uv;
	Vector3 vn;
	Triangle t;

	std::string dump;
	std::string curgrp;
	std::string facept1;
	std::string facept2;
	std::string facept3;
	std::string facept4;
	uint32_t f[4];
	uint32_t ft[4];
	uint32_t fn[4];
	uint32_t nPoints = 0;
	uint32_t v_idx[4];

	std::vector<Vector3> verts;
	std::vector<Vector2> uvs;
	std::vector<Vector3> norms;
	size_t pos;

	std::map<std::string, std::vector<ObjPoint>> grpPoints;

	bool gotface = false;

	while (!base.eof()) {
		if (!gotface)
			base >> dump;
		else
			gotface = false;

		if (options.noFaces) {
			if (dump.compare("v") == 0) {
				base >> v.x >> v.y >> v.z;
				current->verts.push_back(v);
			}
			else if (dump.compare("o") == 0 || dump.compare("g") == 0) {
				base >> curgrp;

				if (!current->name.empty()) {
					data[current->name] = current;
					current = new ObjData();
				}

				current->name = curgrp;
			}
			else if (dump.compare("vt") == 0) {
				base >> uv.u >> uv.v;
				uv.v = 1.0f - uv.v;
				current->uvs.push_back(uv);
			}

			continue;
		}

		if (dump.compare("v") == 0) {
			base >> v.x >> v.y >> v.z;

			if (verts.size() < maxVertexCount)
				verts.push_back(v);
		}
		else if (dump.compare("o") == 0 || dump.compare("g") == 0) {
			base >> curgrp;

			if (!current->name.empty()) {
				if (!current->tris.empty()) {
					// Keep current group if it's not empty
					data[current->name] = current;
				}
				else {
					// Don't keep empty group
					data.erase(current->name);
					current->name.clear();
				}
			}

			auto grp = data.find(curgrp);
			if (grp != data.end()) {
				// Re-use group with matching name
				current = grp->second;
			}
			else {
				// Add new group
				if (!current->name.empty())
					current = new ObjData();

				current->name = curgrp;
			}
		}
		else if (dump.compare("vt") == 0) {
			base >> uv.u >> uv.v;
			uv.v = 1.0f - uv.v;
			uvs.push_back(uv);
		}
		else if (dump.compare("vn") == 0) {
			base >> vn.x >> vn.y >> vn.z;
			norms.push_back(vn);
		}
		else if (dump.compare("f") == 0) {
			base >> facept1 >> facept2 >> facept3;

			f[0] = atoi(facept1.c_str()) - 1;
			pos = facept1.find('/');
			ft[0] = atoi(facept1.substr(pos + 1).c_str()) - 1;
			pos = facept1.find('/', pos + 1);
			fn[0] = atoi(facept1.substr(pos + 1).c_str()) - 1;

			f[1] = atoi(facept2.c_str()) - 1;
			pos = facept2.find('/');
			ft[1] = atoi(facept2.substr(pos + 1).c_str()) - 1;
			pos = facept2.find('/', pos + 1);
			fn[1] = atoi(facept2.substr(pos + 1).c_str()) - 1;

			f[2] = atoi(facept3.c_str()) - 1;
			pos = facept3.find('/');
			ft[2] = atoi(facept3.substr(pos + 1).c_str()) - 1;
			pos = facept3.find('/', pos + 1);
			fn[2] = atoi(facept3.substr(pos + 1).c_str()) - 1;

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
					pos = facept4.find('/', pos + 1);
					fn[3] = atoi(facept4.substr(pos + 1).c_str()) - 1;
					nPoints = 4;
					gotface = false;
				}
			}

			auto& curPoints = grpPoints[curgrp];

			bool skipFace = false;
			for (size_t i = 0; i < nPoints; i++) {
				v_idx[i] = current->verts.size();

				ObjPoint pt(f[i], ft[i], fn[i]);

				auto ptIt = std::find(curPoints.begin(), curPoints.end(), pt);
				if (ptIt != curPoints.end())
					v_idx[i] = (uint32_t)(ptIt - curPoints.begin());
				else
					curPoints.push_back(pt);

				if (v_idx[i] == current->verts.size()) {
					if (verts.size() > f[i]) {
						current->verts.push_back(verts[f[i]]);

						if (uvs.size() > ft[i])
							current->uvs.push_back(uvs[ft[i]]);

						if (norms.size() > fn[i])
							current->norms.push_back(norms[fn[i]]);
					}
					else {
						skipFace = true;
						break;
					}
				}
			}

			if (skipFace)
				continue;

			t.p1 = (uint16_t)v_idx[0];
			t.p2 = (uint16_t)v_idx[1];
			t.p3 = (uint16_t)v_idx[2];
			current->tris.push_back(t);
			if (nPoints == 4) {
				t.p1 = (uint16_t)v_idx[0];
				t.p2 = (uint16_t)v_idx[2];
				t.p3 = (uint16_t)v_idx[3];
				current->tris.push_back(t);
			}
		}
	}

	if (current->name.empty())
		current->name = "object";

	if (!current->verts.empty()) {
		data[current->name] = current;
	}
	else {
		data.erase(current->name);
		delete current;
	}

	return 0;
}


int ObjFile::Save(const std::string &fileName) {
	std::fstream file;
	PlatformUtil::OpenFileStream(file, fileName, std::ios::out | std::ios::binary);
	if (file.fail())
		return 1;

	file << "# Outfit Studio - OBJ Export" << std::endl;
	file << "# https://github.com/ousnius/BodySlide-and-Outfit-Studio" << std::endl << std::endl;

	size_t groupCount = 1;
	size_t pointOffset = 1;

	for (auto& d : data) {
		file << "# object " << d.first << std::endl;
		file << "# " << d.second->verts.size() << " vertices" << std::endl;
		file << "# " << d.second->uvs.size() << " texture coordinates" << std::endl;
		file << "# " << d.second->norms.size() << " normals" << std::endl;
		file << "# " << d.second->tris.size() << " triangles" << std::endl;

		for (size_t i = 0; i < d.second->verts.size(); i++) {
			file << "v " << (d.second->verts[i].x + offset.x) * scale.x
				<< " " << (d.second->verts[i].y + offset.y) * scale.y
				<< " " << (d.second->verts[i].z + offset.z) * scale.z
				<< std::endl;
		}
		file << std::endl;

		for (size_t i = 0; i < d.second->uvs.size(); i++) {
			file << "vt " << d.second->uvs[i].u << " "
				<< (1.0f - d.second->uvs[i].v)
				<< std::endl;
		}
		file << std::endl;

		for (size_t i = 0; i < d.second->norms.size(); i++) {
			file << "vn " << d.second->norms[i].x << " "
				<< d.second->norms[i].y << " "
				<< d.second->norms[i].z
				<< std::endl;
		}
		file << std::endl;

		file << "g " << d.first << std::endl;
		file << "usemtl NoMaterial_" << groupCount << std::endl << std::endl;

		file << "s 1" << std::endl;

		for (size_t i = 0; i < d.second->tris.size(); i++) {
			file << "f " << d.second->tris[i].p1 + pointOffset;

			if (!d.second->uvs.empty())
				file << "/" << d.second->tris[i].p1 + pointOffset;

			if (!d.second->norms.empty()) {
				if (d.second->uvs.empty())
					file << "/";

				file << "/" << d.second->tris[i].p1 + pointOffset;
			}

			file << " " << d.second->tris[i].p2 + pointOffset;

			if (!d.second->uvs.empty())
				file << "/" << d.second->tris[i].p2 + pointOffset;

			if (!d.second->norms.empty()) {
				if (d.second->uvs.empty())
					file << "/";

				file << "/" << d.second->tris[i].p2 + pointOffset;
			}

			file << " " << d.second->tris[i].p3 + pointOffset;

			if (!d.second->uvs.empty())
				file << "/" << d.second->tris[i].p3 + pointOffset;

			if (!d.second->norms.empty()) {
				if (d.second->uvs.empty())
					file << "/";

				file << "/" << d.second->tris[i].p3 + pointOffset;
			}

			file << std::endl;
		}
		file << std::endl;

		pointOffset += d.second->verts.size();
		groupCount++;
	}

	file.close();
	return 0;
}

bool ObjFile::CopyDataForGroup(const std::string &name, std::vector<Vector3> *v, std::vector<Triangle> *t, std::vector<Vector2> *uv, std::vector<Vector3>* norms) {
	if (data.find(name) == data.end())
		return false;

	ObjData* od = data[name];
	if (v) {
		v->clear();
		v->resize(od->verts.size());
		for (size_t i = 0; i < od->verts.size(); i++) {
			(*v)[i].x = (od->verts[i].x + offset.x) * scale.x;
			(*v)[i].y = (od->verts[i].y + offset.y) * scale.y;
			(*v)[i].z = (od->verts[i].z + offset.z) * scale.z;
		}
	}

	if (t) {
		t->clear();
		t->resize(od->tris.size());
		for (size_t i = 0; i < od->tris.size(); i++)
			(*t)[i] = od->tris[i];
	}

	if (uv) {
		uv->clear();
		uv->resize(od->uvs.size());
		for (size_t i = 0; i < od->uvs.size(); i++)
			(*uv)[i] = od->uvs[i];
	}

	if (norms) {
		norms->clear();
		norms->resize(od->norms.size());
		for (size_t i = 0; i < od->norms.size(); i++)
			(*norms)[i] = od->norms[i];
	}

	return true;
}

std::vector<std::string> ObjFile::GetGroupList() {
	std::vector<std::string> groupList;
	groupList.reserve(data.size());

	for (const auto& group : data)
		groupList.push_back(group.first);

	return groupList;
}
