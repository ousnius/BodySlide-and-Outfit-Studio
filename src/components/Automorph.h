/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "../NIF/NifFile.h"
#include "../NIF/utils/KDMatcher.h"
#include "Mesh.h"
#include "SliderSet.h"

class Automorph {
	std::unique_ptr<kd_tree> refTree;
	std::map<std::string, mesh*> sourceShapes;
	// Class - to prevent AutoMorph from deleting it. Golly, smart pointers would be nice.
	std::map<int, std::vector<kd_query_result>> prox_cache;
	DiffDataSets __srcDiffData;				// Unternally loaded and stored diff data.diffs loaded from existing reference .bsd files.
	DiffDataSets* srcDiffData = nullptr;	// Either __srcDiffData or an external linked data set.
	DiffDataSets resultDiffData;			// Diffs calculated by AutoMorph.

	bool bEnableMask = true;				// Use red component of mesh vertex color as a mask for morphing.

	// A translation between shapetarget + slidername and the data name for the result diff data set.
	// This only has values when a sliderset is loaded from disk and a slider's data name for a target
	// doesn't match the format targetname + slidername.
	std::unordered_map<std::string, std::string> targetSliderDataNames;

public:
	std::unique_ptr<mesh> morphRef;

	Automorph();
	~Automorph();

	void ClearResultDiff() {
		resultDiffData.Clear();
	}
	void ClearSourceShapes();

	void RenameResultDiffData(const std::string& shape, const std::string& oldName, const std::string& newName);
	void RenameShape(const std::string& oldShapeName, const std::string& oldTarget, const std::string& newShapeName);
	void CopyShape(const std::string& srcShapeName, const std::string& srcTarget, const std::string& destShapeName);

	void EnableMasking(bool enable = true) {
		bEnableMask = enable;
	}

	void SetRef(NifFile& Ref, NiShape* refShape);

	void LinkRefDiffData(DiffDataSets* diffData);
	void UnlinkRefDiffData();

	bool ApplyDiffToVerts(const std::string& sliderName, const std::string& shapeTargetName, std::vector<Vector3>* inOutResult, float strength = 1.0f);
	bool ApplyResultToVerts(const std::string& sliderName, const std::string& shapeTargetName, std::vector<Vector3>* inOutResult, float strength = 1.0f);
	bool ApplyResultToUVs(const std::string& sliderName, const std::string& shapeTargetName, std::vector<Vector2>* inOutResult, float strength = 1.0f);

	void SourceShapesFromNif(NifFile& baseNif);
	void UpdateMeshFromNif(NifFile &baseNif, const std::string& shapeName);
	void CopyMeshMask(mesh*m, const std::string& shapeName);

	void MeshFromNifShape(mesh* m, NifFile& ref, NiShape* shape);
	void DeleteVerts(const std::string& shapeName, const std::vector<ushort>& indices);

	void ClearProximityCache();
	void BuildProximityCache(const std::string& shapeName, const float proximityRadius = 10.0f);

	// shapeName = name of the mesh to morph (eg "IronArmor") also known as target name.
	// sliderName = name of the morph to apply (eg "BreastsSH").
	void GenerateResultDiff(const std::string& shapeName, const std::string& sliderName, const std::string& refDataName, const int maxResults = 10);

	void SetResultDataName(const std::string& shapeName, const std::string& sliderName, const std::string& dataName);
	std::string ResultDataName(const std::string& shapeName, const std::string& sliderName);

	void GetRawResultDiff(const std::string& shapeName, const std::string& sliderName, std::unordered_map<ushort, Vector3>& outDiff);
	int GetResultDiffSize(const std::string& shapeName, const std::string& sliderName);

	void SetResultDiff(const std::string& shapeName, const std::string& sliderName, std::unordered_map<ushort, Vector3>& diff);
	void UpdateResultDiff(const std::string& shapeName, const std::string& sliderName, std::unordered_map<ushort, Vector3>& diff);
	void UpdateRefDiff(const std::string& shapeName, const std::string& sliderName, std::unordered_map<ushort, Vector3>& diff);
	void EmptyResultDiff(const std::string& shapeName, const std::string& sliderName);
	void ZeroVertDiff(const std::string& shapeName, const std::string& sliderName, std::vector<ushort>* vertSet, std::unordered_map<ushort, float>* mask);
	void ScaleResultDiff(const std::string& shapeName, const std::string& sliderName, float scaleValue);

	void LoadResultDiffs(SliderSet& fromSet);
	void MergeResultDiffs(SliderSet& fromSet, SliderSet& mergeSet, DiffDataSets& baseDiffData, const std::string& baseShape);

	void ClearResultSet(const std::string& sliderName);

	void SaveResultDiff(const std::string& shapeName, const std::string& sliderName, const std::string& fileName);
};
