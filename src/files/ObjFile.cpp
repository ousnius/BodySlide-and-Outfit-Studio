/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "ObjFile.h"
#include "../utils/PlatformUtil.h"
#include <sstream>

using namespace nifly;

int ObjFile::AddGroup(
	const std::string& name, const std::vector<Vector3>& verts, const std::vector<Triangle>& tris, const std::vector<Vector2>& uvs, const std::vector<Vector3>& norms) {
	if (name.empty() || verts.empty())
		return 1;

	ObjData& newData = data[name];
	newData.name = name;
	newData.verts = verts;
	newData.tris = tris;
	newData.uvs = uvs;
	newData.norms = norms;

	return 0;
}

int ObjFile::LoadForNif(const std::string& fileName, const ObjOptionsImport& options) {
	std::fstream file;
	PlatformUtil::OpenFileStream(file, fileName, std::ios::in | std::ios::binary);
	if (file.fail())
		return 1;

	if (options.noFaces)
		LoadNoFaces(file);
	else
		LoadForNif(file);
	return 0;
}

void ObjFile::LoadNoFaces(std::istream& ins) {
	ObjData d;

	while (!ins.eof()) {
		std::string line;
		std::getline(ins, line);
		if (line.empty() || line[0] == '#')
			continue;

		std::istringstream lineiss(line);
		std::string cmd;
		lineiss >> cmd;

		if (cmd == "v") {
			Vector3 v;
			lineiss >> v.x >> v.y >> v.z;
			d.verts.push_back(v);
		}
		else if (cmd == "o" || cmd == "g") {
			std::string curgrp;
			lineiss >> curgrp;

			if (!d.name.empty()) {
				data[d.name] = std::move(d);
				d = ObjData();
			}

			d.name = curgrp;
		}
		else if (cmd == "vt") {
			Vector2 uv;
			lineiss >> uv.u >> uv.v;
			uv.v = 1.0f - uv.v;
			d.uvs.push_back(uv);
		}
	}

	if (d.name.empty())
		d.name = "object";

	if (!d.verts.empty())
		data[d.name] = std::move(d);
	else
		data.erase(d.name);
}

void ObjFile::LoadForNif(std::istream& ins) {
	// First, parse the file into verts, uvs, norms, and faces.  Faces
	// are grouped by group/object name; the rest are not.
	std::vector<Vector3> verts;
	std::vector<Vector2> uvs;
	std::vector<Vector3> norms;
	struct FaceVertex {
		// vi, ti, and ni are indexes into verts, uvs, and norms.  In the
		// file, the three are offset by 1 so vi==1 means verts[0].
		// Negative values are also possible, and indicate offsets from the end
		// of the respective array using its size at the moment the face line
		// is encountered.
		int vi = 0, ti = 0, ni = 0;
		void Parse(const std::string& s) {
			vi = atoi(s.c_str());
			size_t pos = s.find('/');
			ti = atoi(s.substr(pos + 1).c_str());
			pos = s.find('/', pos + 1);
			ni = atoi(s.substr(pos + 1).c_str());
		}
	};
	// groups[group/object name][face index][face-vertex index]
	std::unordered_map<std::string, std::vector<std::vector<FaceVertex>>> groups;
	std::vector<std::vector<FaceVertex>>* currentgroup = &groups["object"];

	while (!ins.eof()) {
		std::string line;
		std::getline(ins, line);
		if (line.empty() || line[0] == '#')
			continue;

		std::istringstream lineiss(line);
		std::string cmd;
		lineiss >> cmd;

		if (cmd == "v") {
			Vector3 v;
			lineiss >> v.x >> v.y >> v.z;
			verts.push_back(v);
		}
		else if (cmd == "o" || cmd == "g") {
			std::string grp;
			lineiss >> grp;
			currentgroup = &groups[grp];
		}
		else if (cmd == "vt") {
			Vector2 uv;
			lineiss >> uv.u >> uv.v;
			uv.v = 1.0f - uv.v;
			uvs.push_back(uv);
		}
		else if (cmd == "vn") {
			Vector3 vn;
			lineiss >> vn.x >> vn.y >> vn.z;
			norms.push_back(vn);
		}
		else if (cmd == "f") {
			currentgroup->emplace_back();
			std::vector<FaceVertex>& face = currentgroup->back();
			while (true) {
				std::string vertstring;
				lineiss >> vertstring;
				FaceVertex v;
				v.Parse(vertstring);
				if (v.vi == 0)
					break;
				if (v.vi < 0)
					v.vi += static_cast<int>(verts.size());
				else
					--v.vi;
				if (v.ti < 0)
					v.ti += static_cast<int>(uvs.size());
				else
					--v.ti;
				if (v.ni < 0)
					v.ni += static_cast<int>(norms.size());
				else
					--v.ni;
				face.push_back(v);
			}
		}
	}

	int nVerts = static_cast<int>(verts.size());
	int nUvs = static_cast<int>(uvs.size());
	int nNorms = static_cast<int>(norms.size());

	// Each vertex in verts may need to be duplicated if it's used with
	// different texture coordinates or normal.  vdata keeps track of all the
	// duplicates for a vertex in a group.
	struct VertData {
		// grpvi: index into vertices used by this group, in order encountered
		// in the group's face vertices.  This is not the same as the final
		// vertex ordering.
		int grpvi;
		Vector2 uv;
		Vector3 nrm;
	};
	std::vector<std::vector<VertData>> vdata;
	std::vector<int> facegrpvi;

	// We're done parsing the file, but the data needs a lot more work to
	// get it into the right form.  We work on one group/object at a time.
	for (auto& gfp : groups) {
		std::string groupName = gfp.first;
		const std::vector<std::vector<FaceVertex>>& faces = gfp.second;
		if (faces.empty())
			continue;

		// Determine if we have UVs or normals
		bool haveUVs = true, haveNorms = true, goodVerts = true;
		for (const std::vector<FaceVertex>& face : faces)
			for (const FaceVertex& fv : face) {
				if (fv.vi < 0 || fv.vi >= nVerts)
					goodVerts = false;
				if (fv.ti < 0 || fv.ti >= nUvs)
					haveUVs = false;
				if (fv.ni < 0 || fv.ni >= nNorms)
					haveNorms = false;
			}
		if (!goodVerts)
			continue;

		ObjData& d = data[groupName];
		d.name = groupName;
		vdata.clear();
		vdata.resize(nVerts);

		// Collect list of all vertices used by group, temporarily indexed
		// by grpvi and listed in vdata; also fill in d.tris.
		int grpvi = 0;
		for (const std::vector<FaceVertex>& face : faces) {
			// Find grpvi for each vertex of this face.
			facegrpvi.clear();
			for (const FaceVertex& fv : face) {
				// Look up vertex's uv and nrm.
				Vector2 uv;
				if (haveUVs)
					uv = uvs[fv.ti];
				Vector3 nrm;
				if (haveNorms)
					nrm = norms[fv.ni];

				// Search for vertex in vdata so we can get its grpvi
				std::vector<VertData>& vds = vdata[fv.vi];
				bool found = false;
				for (VertData& vd : vds) {
					if (haveUVs && uv != vd.uv)
						continue;
					if (haveNorms && nrm != vd.nrm)
						continue;
					found = true;
					// Found existing grpvi
					facegrpvi.push_back(vd.grpvi);
					break;
				}
				if (!found) {
					// New vertex: add to vdata and use new grpvi
					vds.push_back(VertData{grpvi, uv, nrm});
					facegrpvi.push_back(grpvi++);
				}
			}
			// facegrpvi now has the grpvi for each of the face's vertices.
			// Turn this into triangles.  Note that the vertex indices are
			// _group_ vertex indices (grpvi), not finished vertex indices.
			for (size_t fvi = 2; fvi < facegrpvi.size(); ++fvi)
				d.tris.emplace_back(facegrpvi[0], facegrpvi[fvi - 1], facegrpvi[fvi]);
		}

		// vdata now has a complete list of vertices used by this group.
		// The number of vertices used is grpvi.  We create
		// the "finished" vertex ordering by going through vdata in order,
		// assigning a finvi to each used vertex found.  We prepare
		// a map from grpvi to finvi.  We store the data in d.verts, d.uvs,
		// and d.norms.
		int finvi = 0;
		std::vector<int> grpviToFinvi(grpvi);
		d.verts.resize(grpvi);
		if (haveUVs)
			d.uvs.resize(grpvi);
		if (haveNorms)
			d.norms.resize(grpvi);
		for (int filevi = 0; filevi < nVerts; ++filevi)
			for (VertData& vd : vdata[filevi]) {
				grpviToFinvi[vd.grpvi] = finvi;
				d.verts[finvi] = verts[filevi];
				if (haveUVs)
					d.uvs[finvi] = vd.uv;
				if (haveNorms)
					d.norms[finvi] = vd.nrm;
				++finvi;
			}
		// assert(finvi == grpvi);

		// Map the triangle vertex indices from grpvi to finvi
		for (Triangle& t : d.tris)
			for (int tvi = 0; tvi < 3; ++tvi)
				t[tvi] = grpviToFinvi[t[tvi]];
	}
}

int ObjFile::Save(const std::string& fileName) {
	std::fstream file;
	PlatformUtil::OpenFileStream(file, fileName, std::ios::out | std::ios::binary);
	if (file.fail())
		return 1;

	file << "# Outfit Studio - OBJ Export" << std::endl;
	file << "# https://github.com/ousnius/BodySlide-and-Outfit-Studio" << std::endl << std::endl;

	size_t groupCount = 1;
	size_t pointOffset = 1;

	for (auto& odp : data) {
		const ObjData& d = odp.second;
		file << "# object " << d.name << std::endl;
		file << "# " << d.verts.size() << " vertices" << std::endl;
		file << "# " << d.uvs.size() << " texture coordinates" << std::endl;
		file << "# " << d.norms.size() << " normals" << std::endl;
		file << "# " << d.tris.size() << " triangles" << std::endl;

		for (size_t i = 0; i < d.verts.size(); i++) {
			file << "v " << (d.verts[i].x + offset.x) * scale.x << " " << (d.verts[i].y + offset.y) * scale.y << " " << (d.verts[i].z + offset.z) * scale.z
				 << std::endl;
		}
		file << std::endl;

		for (size_t i = 0; i < d.uvs.size(); i++) {
			file << "vt " << d.uvs[i].u << " " << (1.0f - d.uvs[i].v) << std::endl;
		}
		file << std::endl;

		for (size_t i = 0; i < d.norms.size(); i++) {
			file << "vn " << d.norms[i].x << " " << d.norms[i].y << " " << d.norms[i].z << std::endl;
		}
		file << std::endl;

		file << "g " << d.name << std::endl;
		file << "usemtl NoMaterial_" << groupCount << std::endl << std::endl;

		file << "s 1" << std::endl;

		for (size_t i = 0; i < d.tris.size(); i++) {
			file << "f " << d.tris[i].p1 + pointOffset;

			if (!d.uvs.empty())
				file << "/" << d.tris[i].p1 + pointOffset;

			if (!d.norms.empty()) {
				if (d.uvs.empty())
					file << "/";

				file << "/" << d.tris[i].p1 + pointOffset;
			}

			file << " " << d.tris[i].p2 + pointOffset;

			if (!d.uvs.empty())
				file << "/" << d.tris[i].p2 + pointOffset;

			if (!d.norms.empty()) {
				if (d.uvs.empty())
					file << "/";

				file << "/" << d.tris[i].p2 + pointOffset;
			}

			file << " " << d.tris[i].p3 + pointOffset;

			if (!d.uvs.empty())
				file << "/" << d.tris[i].p3 + pointOffset;

			if (!d.norms.empty()) {
				if (d.uvs.empty())
					file << "/";

				file << "/" << d.tris[i].p3 + pointOffset;
			}

			file << std::endl;
		}
		file << std::endl;

		pointOffset += d.verts.size();
		groupCount++;
	}

	file.close();
	return 0;
}

bool ObjFile::CopyDataForGroup(const std::string& name, std::vector<Vector3>* v, std::vector<Triangle>* t, std::vector<Vector2>* uv, std::vector<Vector3>* norms) {
	if (data.find(name) == data.end())
		return false;

	ObjData& d = data[name];
	if (v) {
		v->clear();
		v->resize(d.verts.size());
		for (size_t i = 0; i < d.verts.size(); i++) {
			(*v)[i].x = (d.verts[i].x + offset.x) * scale.x;
			(*v)[i].y = (d.verts[i].y + offset.y) * scale.y;
			(*v)[i].z = (d.verts[i].z + offset.z) * scale.z;
		}
	}

	if (t)
		*t = d.tris;
	if (uv)
		*uv = d.uvs;
	if (norms)
		*norms = d.norms;

	return true;
}

std::vector<std::string> ObjFile::GetGroupList() {
	std::vector<std::string> groupList;
	groupList.reserve(data.size());

	for (const auto& group : data)
		groupList.push_back(group.first);

	return groupList;
}
