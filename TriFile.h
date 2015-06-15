#pragma once

#include "Object3d.h"

#include <map>
#include <fstream>
#include <memory>
#include <algorithm>

using namespace std;


struct MorphData {
	string name;
	map<int, Vector3> offsets;
};

typedef shared_ptr<MorphData> MorphDataPtr;

class TriFile {
	map<string, vector<MorphDataPtr>> shapeMorphs;

public:
	int Read(string fileName);
	int Write(string fileName, bool packed = false);
	
	void AddMorph(string shapeName, MorphDataPtr data);
	void DeleteMorph(string shapeName, string morphName);
	void DeleteMorphs(string shapeName);
	void DeleteMorphFromAll(string morphName);

	MorphDataPtr GetMorph(string shapeName, string morphName);
};
