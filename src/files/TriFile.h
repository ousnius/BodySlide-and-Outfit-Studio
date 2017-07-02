/*
BodySlide and Outfit Studio
Copyright (C) 2017  Caliente & ousnius
See the included LICENSE file
*/

#pragma once

#include "../NIF/utils/Object3d.h"

#include <map>
#include <fstream>
#include <memory>
#include <algorithm>

struct MorphData {
	std::string name;
	std::map<int, Vector3> offsets;
};

typedef std::shared_ptr<MorphData> MorphDataPtr;

class TriFile {
	std::map<std::string, std::vector<MorphDataPtr>> shapeMorphs;

public:
	bool Read(const std::string& fileName);
	bool Write(const std::string& fileName);
	
	void AddMorph(const std::string& shapeName, MorphDataPtr data);
	void DeleteMorph(const std::string& shapeName, const std::string& morphName);
	void DeleteMorphs(const std::string& shapeName);
	void DeleteMorphFromAll(const std::string& morphName);

	MorphDataPtr GetMorph(const std::string& shapeName, const std::string& morphName);
	std::map<std::string, std::vector<MorphDataPtr>> GetMorphs();
};
