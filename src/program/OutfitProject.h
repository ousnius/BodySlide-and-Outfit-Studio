/*
BodySlide and Outfit Studio
Copyright (C) 2017  Caliente & ousnius
See the included LICENSE file
*/

#pragma once

#include "../components/Automorph.h"
#include "../components/Mesh.h"
#include "../components/Anim.h"
#include "OutfitStudio.h"

#include <wx/arrstr.h>

#pragma warning (disable: 4018)
using namespace std;

class OutfitStudio;

class OutfitProject {
	ConfigurationManager& appConfig;
	OutfitStudio* owner;

	NifFile workNif;
	AnimInfo workAnim;
	string baseShape;

	// All cloth data blocks that have been loaded during work
	unordered_map<string, BSClothExtraData> clothData;

	void CheckNIFTarget(NifFile& nif);

public:
	string outfitName;
	DiffDataSets baseDiffData;
	SliderSet activeSet;
	Automorph morpher;
	bool morpherInitialized;

	map<string, vector<Vector3>> boneScaleOffsets;
	map<string, vector<Vector3>> boneScaleVerts;
	map<string, unordered_map<ushort, float>> workWeights;

	unordered_map<string, vector<string>> shapeTextures;
	unordered_map<string, MaterialFile> shapeMaterialFiles;

	// inOwner is meant to provide access to OutfitStudio for the purposes of reporting process status only.
	OutfitProject(ConfigurationManager& inConfig, OutfitStudio* inOwner = nullptr);
	~OutfitProject();
	wxString mFileName;
	wxString mOutfitName;
	wxString mDataDir;
	wxString mBaseFile;
	wxString mGamePath;
	wxString mGameFile;
	bool mCopyRef;
	bool mGenWeights;

	// Returns a string error message or empty string on success.
	string Save(const wxString& strFileName,
		const wxString& strOutfitName,
		const wxString& strDataDir,
		const wxString& strBaseFile,
		const wxString& strGamePath,
		const wxString& strGameFile,
		bool genWeights,
		bool copyRef);

	bool SaveSliderData(const wxString& fileName, bool copyRef = true);

	NifFile* GetWorkNif() { return &workNif; }
	AnimInfo* GetWorkAnim() { return &workAnim; }
	unordered_map<string, BSClothExtraData>& GetClothData() { return clothData; }

	string GetBaseShape() { return baseShape; }
	void SetBaseShape(const string& shapeName) { baseShape = shapeName; }

	bool IsBaseShape(const string& shapeName) {
		return shapeName == baseShape;
	}

	string SliderSetName();
	string SliderSetFileName();
	string OutfitName();

	void ReplaceForbidden(string& str, const char& replacer = ' ');

	bool ValidSlider(int index);
	bool ValidSlider(const string& sliderName);
	bool AllSlidersZero();
	int SliderCount(void);
	string GetSliderName(int index);
	void GetSliderList(vector<string>& sliderNames);
	void AddEmptySlider(const string& newName);
	void AddZapSlider(const string& newName, unordered_map<ushort, float>& verts, const string& shapeName);
	void AddCombinedSlider(const string& newName);

	int AddShapeFromObjFile(const string& fileName, const string& shapeName, const string& mergeShape = "");
	int CreateNifShapeFromData(const string& shapeName,  vector<Vector3>& v,  vector<Triangle>& t,  vector<Vector2>& uv, vector<Vector3>* norms = nullptr);

	// Slider data can have a separate name from the shape target.
	string SliderShapeDataName(int index, const string& shapeName);
	bool SliderClamp(int index);
	bool SliderZap(int index);
	bool SliderUV(int index);
	wxArrayString SliderZapToggles(int index);
	bool SliderInvert(int index);
	bool SliderHidden(int index);
	int SliderIndexFromName(const string& sliderName);

	void SetSliderZap(int index, bool zap);
	void SetSliderZapToggles(int index, const wxArrayString& toggles);
	void SetSliderInvert(int index, bool inv);
	void SetSliderUV(int index, bool uv);
	void SetSliderHidden(int index, bool hidden);
	void SetSliderDefault(int index, int val, bool isHi);
	void SetSliderName(int index, const string& newName);

	void NegateSlider(const string& sliderName, const string& shapeName);
	void MaskAffected(const string& sliderName, const string& shapeName);

	void SetSliderFromBSD(const string& sliderName, const string& shapeName, const string& fileName);
	bool SetSliderFromOBJ(const string& sliderName, const string& shapeName, const string& fileName);
	bool SetSliderFromFBX(const string& sliderName, const string& shapeName, const string& fileName);
	void SetSliderFromDiff(const string& sliderName, const string& shapeName, unordered_map<ushort, Vector3>& diff);
	int SaveSliderBSD(const string& sliderName, const string& shapeName, const string& fileName);
	int SaveSliderOBJ(const string& sliderName, const string& shapeName, const string& fileName);
	int WriteMorphTRI(const string& triPath);

	float& SliderValue(int index);
	float& SliderValue(const string& name);
	float SliderDefault(int index, bool hi);

	void InitConform();
	void ConformShape(const string& shapeName);

	const string& ShapeToTarget(const string& shapeName);
	int GetVertexCount(const string& shapeName);
	void GetLiveVerts(const string& shapeName, vector<Vector3>& outVerts, vector<Vector2>* outUVs = nullptr);
	void GetShapes(vector<string>& outShapeNames);
	void GetActiveBones(vector<string>& outBoneNames);

	vector<string> GetShapeTextures(const string& shapeName);
	bool GetShapeMaterialFile(const string& shapeName, MaterialFile& outMatFile);

	void SetTextures();
	void SetTextures(const string& shapeName, const vector<string>& textureFiles = vector<string>());

	bool IsValidShape(const string& shapeName);

	bool& SliderShow(int index);
	bool& SliderShow(const string& sliderName);

	void RefreshMorphShape(const string& shapeName);
	void UpdateShapeFromMesh(const string& shapeName, const mesh* m);
	void UpdateMorphResult(const string& shapeName, const string& sliderName, unordered_map<ushort, Vector3>& vertUpdates);
	void ScaleMorphResult(const string& shapeName, const string& sliderName, float scaleValue);
	void MoveVertex(const string& shapeName, const Vector3& pos, const int& id);
	void OffsetShape(const string& shapeName, const Vector3& xlate, unordered_map<ushort, float>* mask = nullptr);
	void ScaleShape(const string& shapeName, const Vector3& scale, unordered_map<ushort, float>* mask = nullptr);
	void RotateShape(const string& shapeName, const Vector3& angle, unordered_map<ushort, float>* mask = nullptr);

	void AutoOffset(NifFile& nif);

	// Uses the AutoMorph class to generate proximity values for bone weights.
	// This is done by creating several virtual sliders that contain weight offsets for each vertex per bone.
	// These data sets are then temporarily linked to the AutoMorph class and result 'diffs' are generated.
	// The resulting data is then written back to the outfit shape as the green color channel.
	void CopyBoneWeights(const string& destShape, const float& proximityRadius, const int& maxResults, unordered_map<ushort, float>* mask = nullptr, vector<string>* inBoneList = nullptr);
	// Transfers the weights of the selected bones from reference to chosen shape 1:1. Requires same vertex count and order.
	void TransferSelectedWeights(const string& destShape, unordered_map<ushort, float>* mask = nullptr, vector<string>* inBoneList = nullptr);
	bool HasUnweighted();

	void ApplyBoneScale(const string& bone, int sliderPos, bool clear = false);
	void ClearBoneScale(bool clear = true);

	void AddBoneRef(const string& boneName);
	void AddCustomBoneRef(const string& boneName, const Vector3& translation);

	void ClearWorkSliders();
	void ClearReference();
	void ClearOutfit();
	void ClearSlider(const string& shapeName, const string& sliderName);
	void ClearUnmaskedDiff(const string& shapeName, const string& sliderName, unordered_map<ushort, float>* mask);
	void DeleteSlider(const string& sliderName);

	int LoadSkeletonReference(const string& skeletonFileName);
	int LoadReferenceTemplate(const string& sourceFile, const string& set, const string& shape, bool mergeSliders = false);
	int LoadReferenceNif(const string& fileName, const string& shapeName, bool mergeSliders = false);
	int LoadReference(const string& fileName, const string& setName, bool mergeSliders = false, const string& shapeName = "");
	int AddNif(const string& fileName, bool clear = true, const string& inOutfitName = "");

	int OutfitFromSliderSet(const string& fileName, const string& setName);

	void DeleteVerts(const string& shapeName, const unordered_map<ushort, float>& mask);
	void DuplicateShape(const string& sourceShape, const string& destShape);
	void DeleteShape(const string& shapeName);

	void DeleteBone(const string& boneName) {
		vector<string> shapes;
		if (workNif.IsValid()) {
			GetShapes(shapes);
			for (auto &s : shapes)
				workAnim.RemoveShapeBone(s, boneName);

			int blockID = workNif.GetBlockID(workNif.FindNodeByName(boneName));
			if (blockID >= 0)
				workNif.GetHeader().DeleteBlock(blockID);
		}
	}


	void RenameShape(const string& shapeName, const string& newShapeName);

	void UpdateNifNormals(NifFile* nif, const vector<mesh*>& shapemeshes);
	int SaveOutfitNif(const string& fileName, const vector<mesh*>& modMeshes, bool writeNormals, bool withRef = false);

	void ChooseClothData(NifFile& nif);

	int ImportShapeFBX(const string& fileName, const string& shapeName = "", const string& mergeShape = "");
	int ExportShapeFBX(const string& fileName, const string& shapeName = "");
	int ExportShapeObj(const string& fileName, const string& shapeName, Vector3 scale = Vector3(1.0f, 1.0f, 1.0f), Vector3 offset = Vector3());
};
