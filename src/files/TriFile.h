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
	int Read(std::string fileName);
	int Write(std::string fileName);
	
	void AddMorph(std::string shapeName, MorphDataPtr data);
	void DeleteMorph(std::string shapeName, std::string morphName);
	void DeleteMorphs(std::string shapeName);
	void DeleteMorphFromAll(std::string morphName);

	MorphDataPtr GetMorph(std::string shapeName, std::string morphName);
	std::map<std::string, std::vector<MorphDataPtr>> GetMorphs();
};
