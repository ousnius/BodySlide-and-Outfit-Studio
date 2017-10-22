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

class OutfitStudio;

class OutfitProject {
	ConfigurationManager& appConfig;
	OutfitStudio* owner;

	NifFile workNif;
	AnimInfo workAnim;
	std::string baseShape;

	// All cloth data blocks that have been loaded during work
	std::unordered_map<std::string, BSClothExtraData*> clothData;

	void ValidateNIF(NifFile& nif);

public:
	std::string outfitName = "New Outfit";
	DiffDataSets baseDiffData;
	SliderSet activeSet;
	Automorph morpher;
	bool morpherInitialized;

	std::map<std::string, std::vector<Vector3>> boneScaleOffsets;
	std::map<std::string, std::vector<Vector3>> boneScaleVerts;

	std::unordered_map<std::string, std::vector<std::string>> shapeTextures;
	std::unordered_map<std::string, MaterialFile> shapeMaterialFiles;

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
	std::string Save(const wxString& strFileName,
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
	std::unordered_map<std::string, BSClothExtraData*>& GetClothData() { return clothData; }

	std::string GetBaseShape() { return baseShape; }
	void SetBaseShape(const std::string& shapeName) { baseShape = shapeName; }

	bool IsBaseShape(const std::string& shapeName) {
		return shapeName == baseShape;
	}

	std::string SliderSetName();
	std::string SliderSetFileName();
	std::string OutfitName();

	void ReplaceForbidden(std::string& str, const char& replacer = ' ');

	bool ValidSlider(int index);
	bool ValidSlider(const std::string& sliderName);
	bool AllSlidersZero();
	int SliderCount(void);
	std::string GetSliderName(int index);
	void GetSliderList(std::vector<std::string>& sliderNames);
	void AddEmptySlider(const std::string& newName);
	void AddZapSlider(const std::string& newName, std::unordered_map<ushort, float>& verts, const std::string& shapeName);
	void AddCombinedSlider(const std::string& newName);

	int CreateNifShapeFromData(const std::string& shapeName, std::vector<Vector3>& v, std::vector<Triangle>& t, std::vector<Vector2>& uv, std::vector<Vector3>* norms = nullptr);

	// Slider data can have a separate name from the shape target.
	std::string SliderShapeDataName(int index, const std::string& shapeName);
	bool SliderClamp(int index);
	bool SliderZap(int index);
	bool SliderUV(int index);
	wxArrayString SliderZapToggles(int index);
	bool SliderInvert(int index);
	bool SliderHidden(int index);
	int SliderIndexFromName(const std::string& sliderName);

	void SetSliderZap(int index, bool zap);
	void SetSliderZapToggles(int index, const wxArrayString& toggles);
	void SetSliderInvert(int index, bool inv);
	void SetSliderUV(int index, bool uv);
	void SetSliderHidden(int index, bool hidden);
	void SetSliderDefault(int index, int val, bool isHi);
	void SetSliderName(int index, const std::string& newName);

	void NegateSlider(const std::string& sliderName, const std::string& shapeName);
	void MaskAffected(const std::string& sliderName, const std::string& shapeName);

	void SetSliderFromBSD(const std::string& sliderName, const std::string& shapeName, const std::string& fileName);
	bool SetSliderFromOBJ(const std::string& sliderName, const std::string& shapeName, const std::string& fileName);
	bool SetSliderFromFBX(const std::string& sliderName, const std::string& shapeName, const std::string& fileName);
	void SetSliderFromDiff(const std::string& sliderName, const std::string& shapeName, std::unordered_map<ushort, Vector3>& diff);
	int SaveSliderBSD(const std::string& sliderName, const std::string& shapeName, const std::string& fileName);
	int SaveSliderOBJ(const std::string& sliderName, const std::string& shapeName, const std::string& fileName);
	bool WriteMorphTRI(const std::string& triPath);

	float& SliderValue(int index);
	float& SliderValue(const std::string& name);
	float SliderDefault(int index, bool hi);

	void InitConform();
	void ConformShape(const std::string& shapeName);

	const std::string& ShapeToTarget(const std::string& shapeName);
	int GetVertexCount(const std::string& shapeName);
	void GetLiveVerts(const std::string& shapeName, std::vector<Vector3>& outVerts, std::vector<Vector2>* outUVs = nullptr);
	void GetActiveBones(std::vector<std::string>& outBoneNames);

	std::vector<std::string> GetShapeTextures(const std::string& shapeName);
	bool GetShapeMaterialFile(const std::string& shapeName, MaterialFile& outMatFile);

	void SetTextures();
	void SetTextures(const std::vector<std::string>& textureFiles);
	void SetTextures(NiShape* shape, const std::vector<std::string>& textureFiles = std::vector<std::string>());

	bool IsValidShape(const std::string& shapeName);

	bool& SliderShow(int index);
	bool& SliderShow(const std::string& sliderName);

	void RefreshMorphShape(const std::string& shapeName);
	void UpdateShapeFromMesh(const std::string& shapeName, const mesh* m);
	void UpdateMorphResult(const std::string& shapeName, const std::string& sliderName, std::unordered_map<ushort, Vector3>& vertUpdates);
	void ScaleMorphResult(const std::string& shapeName, const std::string& sliderName, float scaleValue);
	void MoveVertex(const std::string& shapeName, const Vector3& pos, const int& id);
	void OffsetShape(const std::string& shapeName, const Vector3& xlate, std::unordered_map<ushort, float>* mask = nullptr);
	void ScaleShape(const std::string& shapeName, const Vector3& scale, std::unordered_map<ushort, float>* mask = nullptr);
	void RotateShape(const std::string& shapeName, const Vector3& angle, std::unordered_map<ushort, float>* mask = nullptr);

	// Uses the AutoMorph class to generate proximity values for bone weights.
	// This is done by creating several virtual sliders that contain weight offsets for each vertex per bone.
	// These data sets are then temporarily linked to the AutoMorph class and result 'diffs' are generated.
	// The resulting data is then written back to the outfit shape as the green color channel.
	void CopyBoneWeights(const std::string& destShape, const float& proximityRadius, const int& maxResults, std::unordered_map<ushort, float>* mask = nullptr, std::vector<std::string>* inBoneList = nullptr);
	// Transfers the weights of the selected bones from reference to chosen shape 1:1. Requires same vertex count and order.
	void TransferSelectedWeights(const std::string& destShape, std::unordered_map<ushort, float>* mask = nullptr, std::vector<std::string>* inBoneList = nullptr);
	bool HasUnweighted();

	void ApplyBoneScale(const std::string& bone, int sliderPos, bool clear = false);
	void ClearBoneScale(bool clear = true);

	void AddBoneRef(const std::string& boneName);
	void AddCustomBoneRef(const std::string& boneName, const Vector3& translation);

	void ClearWorkSliders();
	void ClearReference();
	void ClearOutfit();
	void ClearSlider(const std::string& shapeName, const std::string& sliderName);
	void ClearUnmaskedDiff(const std::string& shapeName, const std::string& sliderName, std::unordered_map<ushort, float>* mask);
	void DeleteSlider(const std::string& sliderName);

	int LoadSkeletonReference(const std::string& skeletonFileName);
	int LoadReferenceTemplate(const std::string& sourceFile, const std::string& set, const std::string& shape, bool mergeSliders = false);
	int LoadReferenceNif(const std::string& fileName, const std::string& shapeName, bool mergeSliders = false);
	int LoadReference(const std::string& fileName, const std::string& setName, bool mergeSliders = false, const std::string& shapeName = "");

	int OutfitFromSliderSet(const std::string& fileName, const std::string& setName, std::vector<std::string>* origShapeOrder = nullptr);

	void DeleteVerts(const std::string& shapeName, const std::unordered_map<ushort, float>& mask);
	void DuplicateShape(const std::string& sourceShape, const std::string& destShape);
	void DeleteShape(const std::string& shapeName);

	void DeleteBone(const std::string& boneName) {
		if (workNif.IsValid()) {
			for (auto &s : workNif.GetShapeNames())
				workAnim.RemoveShapeBone(s, boneName);

			int blockID = workNif.GetBlockID(workNif.FindNodeByName(boneName));
			if (blockID >= 0)
				workNif.GetHeader().DeleteBlock(blockID);
		}
	}

	void RenameShape(const std::string& shapeName, const std::string& newShapeName);
	void UpdateNifNormals(NifFile* nif, const std::vector<mesh*>& shapemeshes);

	void ChooseClothData(NifFile& nif);

	int ImportNIF(const std::string& fileName, bool clear = true, const std::string& inOutfitName = "");
	int ExportNIF(const std::string& fileName, const std::vector<mesh*>& modMeshes, bool withRef = false);
	int ExportShapeNIF(const std::string& fileName, const std::vector<std::string>& exportShapes);

	int ImportOBJ(const std::string& fileName, const std::string& shapeName, const std::string& mergeShape = "");
	int ExportOBJ(const std::string& fileName, const std::vector<std::string>& shapes, const Vector3& scale = Vector3(1.0f, 1.0f, 1.0f), const Vector3& offset = Vector3());

	int ImportFBX(const std::string& fileName, const std::string& shapeName = "", const std::string& mergeShape = "");
	int ExportFBX(const std::string& fileName, const std::vector<std::string>& shapes);
};
