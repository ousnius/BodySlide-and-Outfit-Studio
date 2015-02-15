#pragma once
#include "NifFile.h"
#include "ObjFile.h"
#include "DiffData.h"
#include "SliderData.h"
#include "Automorph.h"
#include <map>
#include <string>
#include <vector>
#include "Mesh.h"
#include "shlobj.h"
#include "OutfitStudio.h"
#include "ConfigurationManager.h"
#include "Anim.h"

#pragma warning (disable: 4018)
using namespace std;

class OutfitStudio;

class OutfitProject {
	string emptyname;
	string defaultTexFile;
	ConfigurationManager& appConfig;
	OutfitStudio* owner;

public:
	ObjFile testFile;

	NifFile baseNif;
	NifFile workNif;

	string baseShapeName;
	string outfitName;

	DiffDataSets baseDiffData;
	
	SliderSet activeSet;
	
	Automorph morpher;
	bool morpherInitialized;

	AnimInfo baseAnim;
	AnimInfo workAnim;

	map<string, bool> shapeDirty;
	unordered_map<string, string> outfitTextures;
	unordered_map<string, string> baseTextures;

	// inOwner is meant to provide access to OutfitStudio for the purposes of reporting process status only.
	OutfitProject(ConfigurationManager& inConfig, OutfitStudio* inOwner = NULL);
	~OutfitProject(void);

	string mFileName;
	string mOutfitName;
	string mDataDir;
	string mBaseFile;
	string mGamePath;
	string mGameFile;
	bool mCopyRef;
	bool mGenWeights;

	// returns a string error message or empty string on success.
	string Save(const string& strFileName,
			 const string& strOutfitName,
			 const string& strDataDir,
			 const string& strBaseFile,
			 const string& strGamePath,
			 const string& strGameFile,
			 bool genWeights,
			 bool copyRef);

	string SliderSetName(void);
	string SliderSetFileName(void);
	string OutfitName(void);

	bool IsDirty();
	void Clean() { 
		shapeDirty.clear();
	}
	void Clean(const string& specificShape);
	void SetDirty(const string& specificShape);
	bool IsDirty(const string& specificShape);

	bool ValidSlider(int index);
	bool ValidSlider(const string& sliderName);
	bool AllSlidersZero();					
	int SliderCount(void);
	const string& SliderName(int index);
	void GetSliderList(vector<string>& sliderNames);
	void AddEmptySlider(const string& newName);
	void AddZapSlider(const string& newName,  unordered_map<int,float>& verts,const string& shapeName,bool bIsOutfit);
	void AddCombinedSlider(const string& newName);

	int AddShapeFromObjFile(const string& fileName,const string& shapeName, const string& mergeShape = "");

	// Slider data can have a separate name from the shape target. 
	string SliderShapeDataName(int index, const string& shapeName);
	bool SliderClamp(int index);
	bool SliderZap(int index);
	bool SliderInvert(int index);
	bool SliderHidden(int index);
	int SliderIndexFromName(const string& sliderName);

	void SetSliderZap(int index, bool zap);
	void SetSliderInvert(int index, bool inv);
	void SetSliderHidden(int index, bool hidden);
	void SetSliderDefault(int index, int val, bool isHi);
	void SetSliderName(int index, const string& newName);

	void SetSliderFromBSD(const string& sliderName, const string& shapeName, const string& fileName, bool bIsOutfit);
	bool SetSliderFromOBJ(const string& sliderName, const string& shapeName, const string& fileName, bool bIsOutfit);
	void SaveSliderBSD(const string& sliderName, const string& shapeName, const string& fileName, bool bIsOutfit);
	void NegateSlider(const string& sliderName, const string& shapeName, bool bIsOutfit);
	
	float& SliderValue(int index);
	float& SliderValue(const string& name);
	float SliderDefault(int index, bool hi);

	void InitConform();
	void ConformShape(const string& shapeName);

	const string& ShapeToTarget(const string& shapeName);
	void GetLiveVerts(const string& shapeName, vector<vec3>& outVerts, bool bIsOutfit);
	void GetLiveRefVerts(const string& shapeName, vector<vec3>& outVerts);
	void GetLiveOutfitVerts(const string& shapeName, vector<vec3>& outVerts);
	void RefShapes(vector<string>& outShapeNames);
	void OutfitShapes(vector<string>& outShapeNames);
	void RefBones(vector<string>& outBoneNames);
	void OutfitBones(vector<string>& outBoneNames);

	string OutfitTexture(const string& shapeName);
	string RefTexture(const string& shapeName);

	void SetOutfitTexturesDefault(const string& defaultChoice);
	void SetOutfitTextures(const string& textureFile);
	void SetOutfitTexture(const string& shapeName, const string& textureFile);
	void SetRefTexture(const string& shapeName, const string& textureFile);

	int RefShapeShaderType(const string& shapeName) {
		auto s = baseNif.GetShaderForShape((string)shapeName);
		if (s && s->IsSkinShader())
			return 1;
		else
			return 0;
	}

	int OutfitShapeShaderType(const string& shapeName) {
		auto s = workNif.GetShaderForShape((string)shapeName);
		if (s && s->IsSkinShader())
			return 1;
		else
			return 0;
	}

	bool IsValidShape(const string& shapeName);

	bool& SliderShow(int index);
	bool& SliderShow(const string& sliderName);

	void RefreshMorphOutfitShape(const string& shapeName, bool bIsOutfit = true);
	void UpdateShapeFromMesh(const string& shapeName, const mesh* m, bool IsOutfit);
	void UpdateMorphResult(const string& shapeName, const string& sliderName, unordered_map<int, vector3>& vertUpdates, bool IsOutfit);
	void MoveVertex(const string& shapeName, vec3& pos, int& id, bool IsOutfit);
	void OffsetShape(const string& shapeName, vec3& xlate, bool IsOutfit, unordered_map<int, float>* mask = NULL);
	void ScaleShape(const string& shapeName, float& scale, bool IsOutfit, unordered_map<int, float>* mask = NULL);
	void RotateShape(const string& shapeName, vec3& angle, bool IsOutfit, unordered_map<int, float>* mask = NULL);
	
	void AutoOffset(bool IsOutfit);

	// uses the automorph class to generate proximity values for bone weights.  This is done by
	//   creating several virtual sliders that contain weight offsets for each vertex per bone.  
	//   these data sets are then temporarily linked to the automorph class, and result 'diffs' are generated.
	//   the resulting data is then written back to the outfit shape as the Green color channel.
	void CopyBoneWeights(const string& destShape, unordered_map<int, float>* mask = NULL, vector<string>* inBoneList = NULL);
	bool OutfitHasUnweighted();

	void AddBoneRef(const string& boneName, bool IsOutfit = true);

	// Rebuilds skin partitions in the nif.  Games use the skin partition (As opposed to the skindata) for animation,
	//  so fresh meshes need to have the partitions created.  Note, when updating bone weights, rebuilding the skin partions is not
	//  typically required, and bone weight assignment is taken care of in the nif save operations.
	void BuildShapeSkinPartions(const string& destShape, bool IsOutfit);

	void ClearWorkSliders();
	void ClearReference();
	void ClearOutfit();
	void ClearSlider(const string& shapeName, const string& sliderName, bool isOutfit);
	void ClearUnmaskedDiff( const string& shapeName, const string& sliderName, unordered_map<int, float>* mask, bool isOutfit);
	void DeleteSlider(const string& sliderName);
	
	int LoadSkeletonReference(const string& skeletonFileName);
	int LoadReferenceTemplate(const string& templateName, bool ClearRef = true);
	int LoadReferenceNif(const string& fileName, const string& shapeName, bool ClearRef = true);
	int LoadReference(const string& filename, const string& setName, bool ClearRef = true, const string& shapeName = "");
	int LoadOutfit(const string& filename, const string& inOutfitName);
	int AddNif(const string& filename);

	int LoadProject(const string& filename);
	int OutfitFromSliderSet(const string& filename, const string& setName);

	/* shape duplication -- resulting shape ends up in worknif. */
	void DuplicateOutfitShape(const string& sourceShape, const string& destShape, const mesh* curMesh);
	void DuplicateRefShape(const string& sourceShape, const string& destShape, const mesh* curMesh);

	void DeleteOutfitShape(const string& shapeName) {
		workAnim.ClearShape(shapeName);
		workNif.DeleteShape(shapeName);
	}	
	void DeleteRefShape(const string& shapeName) {
		baseAnim.ClearShape(shapeName);
		baseNif.DeleteShape(shapeName);
	}

	void DeleteBone(const string& boneName) {
		vector<string> shapes;
		OutfitShapes(shapes);
		for(auto s : shapes) {
			workAnim.RemoveShapeBone(s,boneName);
		}
		RefShapes(shapes);
		for(auto rs : shapes) {
			baseAnim.RemoveShapeBone(rs, boneName);
		}


		int blockID = workNif.GetNodeID(boneName);
		if(blockID >=0) {
			//refNif->RemoveBoneForShape(shape,blockID);
			workNif.DeleteBlock(blockID);
		}
	}


	void RenameShape(const string& shapeName, const string& newShapeName, bool isOutfit);


	int SaveOutfitNif(const string& filename);
	int SaveModifiedOutfitNif(const string& filename, const vector<mesh*>& modMeshes, bool writeNormals);
	void UpdateNifNormals(NifFile* nif, const vector<mesh*>& shapemeshes) ;

	int ExportShape (const string& shapeName, const string& fname, bool isOutfit);

	/* creates an abbreviated name for use in data file identifiers. Mostly removes spaces and other special characters. */
	string NameAbbreviate(const string& inputName);
};

