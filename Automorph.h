/*
BodySlide and Outfit Studio
Copyright (C) 2015  Caliente & ousnius
See the included LICENSE file
*/

#pragma once

#include "Object3d.h"
#include "NifFile.h"
#include "KDMatcher.h"
#include "Mesh.h"
#include "SliderData.h"
#include "DiffData.h"
#include "ObjFile.h"

#include <vector>
#include <map>

#pragma warning (disable: 4018)
using namespace std;

class Automorph {
	kd_tree* refTree;
	map<string, mesh*> sourceShapes;
	map<string, mesh*> foreignShapes;	// Meshes linked by LinkSourceShapeMesh loaded and managed outside the.
	// Class - to prevent AutoMorph from deleting it. Golly, smart pointers would be nice.
	map<int, vector<kd_query_result>> prox_cache;
	DiffDataSets __srcDiffData;			// Unternally loaded and stored diff data.diffs loaded from existing reference .bsd files.
	DiffDataSets* srcDiffData;			// Either __srcDiffData or an external linked data set.
	DiffDataSets resultDiffData;		// Diffs calculated by AutoMorph.

	float proximity_radius;
	float max_prox_points;

	bool bEnableMask;					// Use red component of mesh vertex color as a mask for morphing.

	// A translation between shapetarget + slidername and the data name for the result diff data set.
	// This only has values when a sliderset is loaded from disk and a slider's data name for a target
	// doesn't match the format targetname + slidername.
	unordered_map<string, string> targetSliderDataNames;

public:
	mesh* morphRef;
	int totalCount;
	float avgCount;
	int maxCount;
	int minCount;

	Automorph();
	Automorph(NifFile& ref, const string& refShape = "BaseShape");
	~Automorph();

	void ClearResultDiff() {
		resultDiffData.Clear();
	}
	void ClearSourceShapes();

	void RenameResultDiffData(const string& shape, const string& oldName, const string& newName);
	void RenameShape(const string& shapeName, const string& newShapeName);

	void EnableMasking(bool enable = true) {
		bEnableMask = enable;
	}

	void SetRef(NifFile& Ref, const string& refShape = "BaseShape");

	int InitRefDiffData(const string& srcFileName, const string& dataSetName, const string& baseDataPath);
	void LinkRefDiffData(DiffDataSets* diffData);
	void UnlinkRefDiffData();

	void ApplyDiffToVerts(const string& sliderName, const string& shapeTargetName, vector<Vector3>* inOutResult, float strength = 1.0f);
	void ApplyResultToVerts(const string& sliderName, const string& shapeTargetName, vector<Vector3>* inOutResult, float strength = 1.0f);

	void SourceShapesFromNif(NifFile& baseNif);
	void SourceShapesFromObj(ObjFile& baseObj);
	void LinkSourceShapeMesh(mesh* m, const string& shapeName);
	void UpdateMeshFromNif(NifFile &baseNif, const string& shapeName);
	void CopyMeshMask(mesh*m, const string& shapeName);

	void MeshFromNifShape(mesh* m, NifFile& ref, const string& shapeName);
	void MeshFromObjShape(mesh* m, ObjFile& ref, const string& shapeName);

	void BuildProximityCache(const string& shapeName);

	// shapeName = name of the mesh to morph (eg "IronArmor") also known as target name.
	// sliderName = name of the morph to apply (eg "BreastsSH").
	void GenerateResultDiff(const string& shapeName, const string& sliderName, const string& refDataName);

	void SetResultDataName(const string& shapeName, const string& sliderName, const string& dataName);
	string ResultDataName(const string& shapeName, const string& sliderName);

	void GetRawResultDiff(const string& shapeName, const string& sliderName, unordered_map<ushort, Vector3>& outDiff);
	int GetResultDiffSize(const string& shapeName, const string& sliderName);

	void SetResultDiff(const string& shapeName, const string& sliderName, unordered_map<ushort, Vector3>& diff);
	void UpdateResultDiff(const string& shapeName, const string& sliderName, unordered_map<ushort, Vector3>& diff);
	void UpdateRefDiff(const string& shapeName, const string& sliderName, unordered_map<ushort, Vector3>& diff);
	void EmptyResultDiff(const string& shapeName, const string& sliderName);
	void ZeroVertDiff(const string& shapeName, const string& sliderName, vector<ushort>* vertSet, unordered_map<ushort, float>* mask);
	void ScaleResultDiff(const string& shapeName, const string& sliderName, float scaleValue);

	void LoadResultDiffs(SliderSet &fromSet);

	void ClearResultSet(const string& sliderName);

	void SaveResultDiff(const string& shapeName, const string& sliderName, const string& fileName);
};
