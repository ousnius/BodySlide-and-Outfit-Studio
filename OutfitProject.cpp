/*
BodySlide and Outfit Studio
Copyright (C) 2015  Caliente & ousnius
See the included LICENSE file
*/

#include "OutfitProject.h"
#include "TriFile.h"
#include "FBXWrangler.h"

OutfitProject::OutfitProject(ConfigurationManager& inConfig, OutfitStudio* inOwner) : appConfig(inConfig) {
	morpherInitialized = false;
	owner = inOwner;
	string defSkelFile = Config.GetCString("Anim/DefaultSkeletonReference");
	LoadSkeletonReference(defSkelFile);

	mCopyRef = true;
	if (owner->targetGame == SKYRIM)
		mGenWeights = true;
	else
		mGenWeights = false;
}

OutfitProject::~OutfitProject() {
}

string OutfitProject::Save(const string& strFileName,
	const string& strOutfitName,
	const string& strDataDir,
	const string& strBaseFile,
	const string& strGamePath,
	const string& strGameFile,
	bool genWeights,
	bool copyRef) {

	owner->UpdateProgress(1, "Checking destination...");
	string errmsg = "";
	string outfitName = strOutfitName, baseFile = strBaseFile, gameFile = strGameFile;

	char chars[] = { '\\', '/', '?', ':', '*', '>', '<', '|', '"' };
	for (uint i = 0; i < sizeof(chars); ++i) {
		replace(outfitName.begin(), outfitName.end(), chars[i], '_');
		replace(baseFile.begin(), baseFile.end(), chars[i], '_');
		replace(gameFile.begin(), gameFile.end(), chars[i], '_');
	}

	SliderSet outSet;
	outSet.SetName(outfitName);
	outSet.SetDataFolder(strDataDir);
	outSet.SetInputFile(baseFile);
	outSet.SetOutputPath(strGamePath);
	outSet.SetOutputFile(gameFile);
	outSet.SetGenWeights(genWeights);

	string ssFileName = strFileName;
	if (ssFileName.find("SliderSets\\") == string::npos)
		ssFileName = "SliderSets\\" + ssFileName;

	mFileName = ssFileName;
	mOutfitName = outfitName;
	mDataDir = strDataDir;
	mBaseFile = baseFile;
	mGamePath = strGamePath;
	mGameFile = gameFile;
	mCopyRef = copyRef;
	mGenWeights = genWeights;

	vector<string> shapes;
	GetShapes(shapes);

	wxString curDir(wxGetCwd());
	wxString folder(wxString::Format("%s/%s/%s", curDir, "ShapeData", strDataDir));
	wxFileName::Mkdir(folder, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);

	int prog = 5;
	int step = 10 / shapes.size();
	owner->UpdateProgress(prog);

	if (copyRef && !baseShape.empty()) {
		// Add all the reference shapes to the target list.
		outSet.AddShapeTarget(baseShape, ShapeToTarget(baseShape));
		outSet.AddTargetDataFolder(ShapeToTarget(baseShape), activeSet.ShapeToDataFolder(baseShape));
		owner->UpdateProgress(prog += step, "Adding reference shapes...");
	}

	// Add all the outfit shapes to the target list.
	for (auto &s : shapes) {
		if (IsBaseShape(s))
			continue;

		outSet.AddShapeTarget(s, ShapeToTarget(s));

		// Reference only if not local folder
		string shapeDataFolder = activeSet.ShapeToDataFolder(s);
		if (shapeDataFolder != activeSet.GetDefaultDataFolder())
			outSet.AddTargetDataFolder(ShapeToTarget(s), activeSet.ShapeToDataFolder(s));

		owner->UpdateProgress(prog += step, "Adding outfit shapes...");
	}

	// Copy the reference slider info and add the outfit data to them.
	int id;
	string targ;
	string targSlider;
	string targSliderData;
	string targDataName;

	string osdFileName = baseFile.substr(0, baseFile.find_last_of('.')) + ".osd";
	string saveDataPath = "ShapeData\\" + strDataDir;

	DiffDataSets osdDiffs;
	map<string, map<string, string>> osdNames;

	prog = 10;
	step = 50 / activeSet.size();
	owner->UpdateProgress(prog);
	for (int i = 0; i < activeSet.size(); i++) {
		id = outSet.CopySlider(&activeSet[i]);
		outSet[id].Clear();
		if (copyRef && !baseShape.empty()) {
			targ = ShapeToTarget(baseShape);
			targSlider = activeSet[i].TargetDataName(targ);
			if (baseDiffData.GetDiffSet(targSlider) && baseDiffData.GetDiffSet(targSlider)->size() > 0) {
				if (activeSet[i].IsLocalData(targSlider)) {
					targDataName = activeSet[i].DataFileName(targSlider);
					int lastIndex = targDataName.find_last_of('\\') + 1;

					targSliderData = osdFileName + "\\" + targDataName.substr(lastIndex, targDataName.length() - lastIndex);
					outSet[id].AddDataFile(targ, targSlider, targSliderData);

					unordered_map<ushort, Vector3>* diff = baseDiffData.GetDiffSet(targSlider);
					osdDiffs.LoadSet(targSlider, targ, *diff);
					osdNames[saveDataPath + "\\" + osdFileName][targSlider] = targ;
				}
				else {
					targSliderData = activeSet[i].DataFileName(targSlider);
					outSet[id].AddDataFile(targ, targSlider, targSliderData, false);
				}
			}
		}

		for (auto &s : shapes) {
			if (IsBaseShape(s))
				continue;

			targ = ShapeToTarget(s);
			targSlider = activeSet[i].TargetDataName(targ);
			if (targSlider.empty())
				targSlider = targ + outSet[i].name;

			if (morpher.GetResultDiffSize(s, activeSet[i].name) > 0) {
				string shapeDataFolder = activeSet.ShapeToDataFolder(s);
				if (shapeDataFolder == activeSet.GetDefaultDataFolder() || activeSet[i].IsLocalData(targSlider)) {
					targSliderData = osdFileName + "\\" + targSlider;
					outSet[i].AddDataFile(targ, targSlider, targSliderData);

					unordered_map<ushort, Vector3> diff;
					morpher.GetRawResultDiff(s, activeSet[i].name, diff);
					osdDiffs.LoadSet(targSlider, targ, diff);
					osdNames[saveDataPath + "\\" + osdFileName][targSlider] = targ;
				}
				else {
					targSliderData = activeSet[i].DataFileName(targSlider);
					outSet[i].AddDataFile(targ, targSlider, targSliderData, false);
				}
			}
		}
		owner->UpdateProgress(prog += step, "Calculating slider data...");
	}

	osdDiffs.SaveData(osdNames);

	prog = 60;
	owner->UpdateProgress(prog, "Creating slider set file...");

	SliderSetFile ssf(ssFileName);
	if (ssf.fail()) {
		ssf.New(ssFileName);
		if (ssf.fail()) {
			errmsg = "Failed to open or create slider set file: " + ssFileName;
			return errmsg;
		}
	}

	auto it = strFileName.rfind('\\');
	if (it != string::npos) {
		wxString ssNewFolder(wxString::Format("%s/%s", curDir, strFileName.substr(0, it)));
		wxFileName::Mkdir(ssNewFolder, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
	}
	else {
		wxString ssNewFolder(wxString::Format("%s/%s", curDir, "SliderSets"));
		wxFileName::Mkdir(ssNewFolder, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
	}

	owner->UpdateProgress(61, "Saving slider set file...");
	ssf.UpdateSet(outSet);
	if (!ssf.Save()) {
		errmsg = "Failed to write to slider set file: " + ssFileName;
		return errmsg;
	}

	owner->UpdateProgress(70, "Saving NIF file...");

	string saveFileName = saveDataPath + "\\" + baseFile;

	if (workNif.IsValid()) {
		NifFile clone(workNif);
		clone.SetNodeName(0, "Scene Root");

		ChooseClothData(clone);

		if (!copyRef && !baseShape.empty()) {
			clone.DeleteShape(baseShape);
			workAnim.WriteToNif(&clone, true, baseShape);
		}
		else
			workAnim.WriteToNif(&clone);

		clone.GetShapeList(shapes);

		for (auto &s : shapes)
			clone.UpdateSkinPartitions(s);

		if (!clone.HasUnknown()) {
			clone.SetShapeOrder(owner->GetShapeList());
			clone.PrettySortBlocks();
		}
		if (clone.Save(saveFileName)) {
			errmsg = "Failed to write base .nif file: " + saveFileName;
			return errmsg;
		}
	}

	owner->UpdateProgress(100, "Finished");
	return errmsg;
}

string OutfitProject::SliderSetName() {
	return activeSet.GetName();
}

string OutfitProject::SliderSetFileName() {
	return activeSet.GetInputFileName();
}

string OutfitProject::OutfitName() {
	return outfitName;
}

bool OutfitProject::IsDirty() {
	return (shapeDirty.size() > 0);
}

void OutfitProject::Clean(const string& specificShape) {
	shapeDirty.erase(specificShape);
}

void  OutfitProject::SetDirty(const string& specificShape) {
	shapeDirty[specificShape] = true;
}

bool OutfitProject::IsDirty(const string& specificShape) {
	return (shapeDirty.find(specificShape) != shapeDirty.end());
}

bool OutfitProject::ValidSlider(int index) {
	if (index >= 0 && index < activeSet.size())
		return true;
	return false;
}

bool OutfitProject::ValidSlider(const string& sliderName) {
	return activeSet.SliderExists(sliderName);
}

bool OutfitProject::AllSlidersZero() {
	for (int i = 0; i < activeSet.size(); i++)
		if (activeSet[i].curValue != 0.0f)
			return false;
	return true;
}

int OutfitProject::SliderCount() {
	return activeSet.size();
}

void OutfitProject::GetSliderList(vector<string>& sliderNames) {
	for (int i = 0; i < activeSet.size(); i++)
		sliderNames.push_back(activeSet[i].name);
}

string OutfitProject::GetSliderName(int index) {
	if (!ValidSlider(index))
		return "";
	return activeSet[index].name;
}

void OutfitProject::AddEmptySlider(const string& newName) {
	string sliderAbbrev = NameAbbreviate(newName);
	int sliderID = activeSet.CreateSlider(newName);
	activeSet[sliderID].bShow = true;

	if (!baseShape.empty()) {
		string target = ShapeToTarget(baseShape);
		string shapeAbbrev = NameAbbreviate(baseShape);
		string shapeSlider = target + sliderAbbrev;
		activeSet[sliderID].AddDataFile(target, shapeSlider, shapeSlider);
		activeSet.AddShapeTarget(baseShape, target);
		baseDiffData.AddEmptySet(shapeSlider, target);
	}
}

void OutfitProject::AddZapSlider(const string& newName, unordered_map<ushort, float>& verts, const string& shapeName) {
	string sliderAbbrev = NameAbbreviate(newName);
	unordered_map<ushort, Vector3> diffData;
	Vector3 moveVec(0.0f, 1.0f, 0.0f);
	for (auto &v : verts)
		diffData[v.first] = moveVec;

	string target = ShapeToTarget(shapeName);
	string shapeAbbrev = NameAbbreviate(shapeName);
	string shapeSlider = target + sliderAbbrev;

	int sliderID = activeSet.CreateSlider(newName);
	activeSet[sliderID].bZap = true;
	activeSet[sliderID].defBigValue = 0.0f;
	activeSet[sliderID].defSmallValue = 0.0f;

	if (IsBaseShape(shapeName)) {
		activeSet[sliderID].AddDataFile(target, shapeSlider, shapeSlider);
		activeSet.AddShapeTarget(shapeName, target);
		baseDiffData.AddEmptySet(shapeSlider, target);
		for (auto &i : diffData)
			baseDiffData.SumDiff(shapeSlider, target, i.first, i.second);
	}
	else
		morpher.SetResultDiff(shapeName, newName, diffData);
}

void OutfitProject::AddCombinedSlider(const string& newName) {
	string sliderAbbrev = NameAbbreviate(newName);
	vector<Vector3> verts;
	unordered_map<ushort, Vector3> diffData;

	vector<string> shapes;
	GetShapes(shapes);
	for (auto &s : shapes) {
		if (IsBaseShape(s))
			continue;

		diffData.clear();
		GetLiveVerts(s, verts);
		workNif.CalcShapeDiff(s, &verts, diffData);
		morpher.SetResultDiff(s, newName, diffData);
	}

	int sliderID = activeSet.CreateSlider(newName);
	if (!baseShape.empty()) {
		string target = ShapeToTarget(baseShape);
		string shapeSlider = target + sliderAbbrev;
		activeSet[sliderID].AddDataFile(target, shapeSlider, shapeSlider);
		baseDiffData.AddEmptySet(shapeSlider, target);
		GetLiveVerts(baseShape, verts);
		workNif.CalcShapeDiff(baseShape, &verts, diffData);
		for (auto &i : diffData)
			baseDiffData.SumDiff(shapeSlider, target, i.first, i.second);
	}
}

int OutfitProject::AddShapeFromObjFile(const string& fileName, const string& shapeName, const string& mergeShape) {
	ObjFile obj;
	obj.SetScale(Vector3(10.0f, 10.0f, 10.0f));

	if (!shapeName.empty())
		outfitName = shapeName;
	else if (outfitName.empty())
		outfitName = "New Outfit";

	if (obj.LoadForNif(fileName)) {
		wxLogError("Could not load OBJ file '%s'!", fileName);
		wxMessageBox(wxString::Format("Could not load OBJ file '%s'!", fileName), "OBJ Error", wxICON_ERROR);
		return 1;
	}
	vector<string> objGroupNames;
	obj.GetGroupList(objGroupNames);
	for (int i = 0; i < objGroupNames.size(); i++) {
		vector<Vector3> v;
		vector<Triangle> t;
		vector<Vector2> uv;
		if (!obj.CopyDataForIndex(i, &v, &t, &uv)) {
			wxLogError("Could not copy data from OBJ file '%s'!", fileName);
			wxMessageBox(wxString::Format("Could not copy data from OBJ file '%s'!", fileName), "OBJ Error", wxICON_ERROR);
			return 3;
		}

		// Skip zero size groups.  
		if (v.size() == 0)
			continue;

		string useShapeName = objGroupNames[i];

		if (mergeShape != "") {
			vector<Vector3> shapeVerts;
			workNif.GetVertsForShape(mergeShape, shapeVerts);
			if (shapeVerts.size() == v.size()) {
				int r = wxMessageBox("The vertex count of the selected .obj file matches the currently selected outfit shape.  Do you wish to update the current shape?  (click No to create a new shape)", "Merge or New", wxYES_NO | wxICON_QUESTION);
				if (r == wxYES) {
					wxLogMessage("Updated shape '%s' using '%s'.", mergeShape, fileName);
					workNif.SetVertsForShape(mergeShape, v);
					return 101;
				}
			}
			useShapeName = wxGetTextFromUser("Please specify a name for the new shape", "New Shape Name", useShapeName);
			if (useShapeName == "")
				return 100;
		}

		CreateNifShapeFromData(useShapeName, v, t, uv);
	}

	return 0;
}


int OutfitProject::CreateNifShapeFromData(const string& shapeName, vector<Vector3>& v, vector<Triangle>& t, vector<Vector2>& uv, vector<Vector3>* norms) {
	bool staticMode = Config["StaticMeshMode"] == "True";

	string blankSkel = "res\\SkeletonBlank.nif";
	string defaultName = "New Outfit";
	if (staticMode) {
		blankSkel = "res\\GameObjectBlank.nif";
		defaultName = "New Object";
	}
	else if (owner->targetGame == FO4)
		blankSkel = "res\\SkeletonBlank_fo4.nif";

	NifFile blank;
	blank.Load(blankSkel);
	if (!blank.IsValid()) {
		wxLogError("Could not load 'SkeletonBlank.nif' for importing data file.");
		wxMessageBox("Could not load 'SkeletonBlank.nif' for importing data file.", "import data Error", wxICON_ERROR);
		return 2;
	}

	if (!workNif.IsValid())
		AddNif(blankSkel, true, defaultName);

	if (owner->targetGame != FO4) {
		NiTriShapeData* nifShapeData = new NiTriShapeData(workNif.hdr);
		nifShapeData->Create(&v, &t, &uv);
		if (norms) {
			nifShapeData->normals = (*norms);
			nifShapeData->hasNormals = true;
		}
		int shapeID = blank.AddBlock((NiObject*)nifShapeData, "NiTriShapeData");

		int dismemberID = -1;
		if (!staticMode){
			NiSkinData* nifSkinData = new NiSkinData(workNif.hdr);
			int skinID = blank.AddBlock((NiObject*)nifSkinData, "NiSkinData");

			NiSkinPartition* nifSkinPartition = new NiSkinPartition(workNif.hdr);
			int partID = blank.AddBlock((NiObject*)nifSkinPartition, "NiSkinPartition");

			BSDismemberSkinInstance* nifDismemberInst = new BSDismemberSkinInstance(workNif.hdr);
			dismemberID = blank.AddBlock((NiObject*)nifDismemberInst, "BSDismemberSkinInstance");
			nifDismemberInst->dataRef = skinID;
			nifDismemberInst->skinPartitionRef = partID;
			nifDismemberInst->skeletonRootRef = 0;
		}

		BSShaderTextureSet* nifTexset = new BSShaderTextureSet(workNif.hdr);

		int shaderID;
		BSLightingShaderProperty* nifShader = nullptr;
		BSShaderPPLightingProperty* nifShaderPP = nullptr;
		switch (owner->targetGame) {
		case FO3:
		case FONV:
			nifShaderPP = new BSShaderPPLightingProperty(workNif.hdr);
			shaderID = blank.AddBlock((NiObject*)nifShaderPP, "BSShaderPPLightingProperty");
			nifShaderPP->textureSetRef = blank.AddBlock((NiObject*)nifTexset, "BSShaderTextureSet");
			break;
		case SKYRIM:
		case FO4:
		default:
			nifShader = new BSLightingShaderProperty(workNif.hdr);
			shaderID = blank.AddBlock((NiObject*)nifShader, "BSLightingShaderProperty");
			nifShader->textureSetRef = blank.AddBlock((NiObject*)nifTexset, "BSShaderTextureSet");
		}

		NiTriShape* nifTriShape = new NiTriShape(workNif.hdr);
		blank.AddBlock((NiObject*)nifTriShape, "NiTriShape");
		nifTriShape->propertiesRef1 = shaderID;

		int nameID = blank.AddOrFindStringId(shapeName);
		nifTriShape->dataRef = shapeID;
		nifTriShape->skinInstanceRef = dismemberID;
		nifTriShape->nameRef = nameID;
		nifTriShape->name = shapeName;
	}
	else {
		BSTriShape* triShapeBase;
		string shaderName = "Materials\\actors\\Character\\BaseHumanFemale\\basehumanFemaleskin.bgsm";
		string wetShaderName = "template/SkinTemplate_Wet.bgsm";
		if (staticMode) {
			triShapeBase = new BSTriShape(workNif.hdr);
			triShapeBase->Create(&v, &t, &uv, norms);
			blank.AddBlock((NiObject*)triShapeBase, "BSTriShape");
			shaderName = "Materials\\Clothes\\Hats\\WigGO.BGSM";
			wetShaderName = "";
		}
		else {
			BSSubIndexTriShape* nifBSTriShape = new BSSubIndexTriShape(workNif.hdr);
			nifBSTriShape->Create(&v, &t, &uv, norms);
			nifBSTriShape->ssfFile = "Meshes\\Actors\\Character\\CharacterAssets\\FemaleBody.ssf";
			blank.AddBlock((NiObject*)nifBSTriShape, "BSSubIndexTriShape");

			BSSkinInstance* nifBSSkinInstance = new BSSkinInstance(workNif.hdr);
			int skinID = blank.AddBlock((NiObject*)nifBSSkinInstance, "BSSkin::Instance");

			BSSkinBoneData* nifBoneData = new BSSkinBoneData(workNif.hdr);
			int boneID = blank.AddBlock((NiObject*)nifBoneData, "BSSkin::BoneData");
			nifBSSkinInstance->boneDataRef = boneID;
			nifBSTriShape->skinInstanceRef = skinID;
			triShapeBase = nifBSTriShape;
		}

		BSShaderTextureSet* nifTexset = new BSShaderTextureSet(workNif.hdr);

		int shaderID;
		BSLightingShaderProperty* nifShader = new BSLightingShaderProperty(workNif.hdr);
		nifShader->nameRef = blank.AddOrFindStringId(shaderName);
		nifShader->name = shaderName;
		nifShader->wetMaterialNameRef = blank.AddOrFindStringId(wetShaderName);
		shaderID = blank.AddBlock((NiObject*)nifShader, "BSLightingShaderProperty");
		nifShader->textureSetRef = blank.AddBlock((NiObject*)nifTexset, "BSShaderTextureSet");

		int nameID = blank.AddOrFindStringId(shapeName);
		triShapeBase->shaderPropertyRef = shaderID;
		triShapeBase->nameRef = nameID;
		triShapeBase->name = shapeName;


	}

	workNif.CopyGeometry(shapeName, blank, shapeName);
	SetTexture(shapeName, "_AUTO_");

	return 0;
}

string OutfitProject::SliderShapeDataName(int index, const string& shapeName) {
	if (!ValidSlider(index))
		return "";
	return activeSet.ShapeToDataName(index, shapeName);
}

bool OutfitProject::SliderClamp(int index) {
	if (!ValidSlider(index))
		return false;
	return activeSet[index].bClamp;
}

bool OutfitProject::SliderZap(int index) {
	if (!ValidSlider(index))
		return false;
	return activeSet[index].bZap;
}

bool OutfitProject::SliderInvert(int index) {
	if (!ValidSlider(index))
		return false;
	return activeSet[index].bInvert;
}

bool OutfitProject::SliderHidden(int index) {
	if (!ValidSlider(index))
		return false;
	return activeSet[index].bHidden;
}

void  OutfitProject::SetSliderZap(int index, bool zap) {
	if (!ValidSlider(index))
		return;
	activeSet[index].bZap = zap;
}

void  OutfitProject::SetSliderInvert(int index, bool inv) {
	if (!ValidSlider(index))
		return;
	activeSet[index].bInvert = inv;
}

void  OutfitProject::SetSliderHidden(int index, bool hidden) {
	if (!ValidSlider(index))
		return;
	activeSet[index].bHidden = hidden;
}

void  OutfitProject::SetSliderDefault(int index, int val, bool isHi) {
	if (!ValidSlider(index))
		return;

	if (!isHi)
		activeSet[index].defSmallValue = val;
	else
		activeSet[index].defBigValue = val;
}

void OutfitProject::SetSliderName(int index, const string& newName) {
	if (!ValidSlider(index))
		return;

	string oldName = activeSet[index].name;
	vector<string> shapes;
	GetShapes(shapes);
	for (auto &s : shapes) {
		if (IsBaseShape(s))
			continue;

		morpher.RenameResultDiffData(s, oldName, newName);
	}

	activeSet[index].name = newName;
}

float& OutfitProject::SliderValue(int index) {
	return activeSet[index].curValue;
}

float& OutfitProject::SliderValue(const string& name) {
	return activeSet[name].curValue;
}

float OutfitProject::SliderDefault(int index, bool hi) {
	if (hi)
		return activeSet[index].defBigValue;

	return activeSet[index].defSmallValue;
}

bool& OutfitProject::SliderShow(int index) {
	return activeSet[index].bShow;
}

bool& OutfitProject::SliderShow(const string& sliderName) {
	return activeSet[sliderName].bShow;
}

int OutfitProject::SliderIndexFromName(const string& sliderName) {
	for (int i = 0; i < activeSet.size(); i++)
		if (activeSet[i].name == sliderName)
			return i;

	return -1;
}

void OutfitProject::NegateSlider(const string& sliderName, const string& shapeName) {
	string target = ShapeToTarget(shapeName);

	if (IsBaseShape(shapeName)) {
		string sliderData = activeSet[sliderName].TargetDataName(target);
		baseDiffData.ScaleDiff(sliderData, target, -1.0f);
	}
	else
		morpher.ScaleResultDiff(target, sliderName, -1.0f);
}

int OutfitProject::WriteMorphTRI(const string& triPath) {
	vector<string> shapes;
	GetShapes(shapes);

	DiffDataSets currentDiffs;
	activeSet.LoadSetDiffData(currentDiffs);

	TriFile tri;
	string triFilePath = triPath;

	for (auto &shape : shapes) {
		bool bIsOutfit = true;
		if (IsBaseShape(shape))
			bIsOutfit = false;

		for (int s = 0; s < activeSet.size(); s++) {
			if (!activeSet[s].bUV && !activeSet[s].bClamp && !activeSet[s].bZap) {
				MorphDataPtr morph = make_shared<MorphData>();
				morph->name = activeSet[s].name;

				vector<Vector3> verts;
				int shapeVertCount = GetVertexCount(shape);

				if (shapeVertCount > 0)
					verts.resize(shapeVertCount);
				else
					continue;
				
				string target = ShapeToTarget(shape);
				if (!bIsOutfit) {
					string dn = activeSet[s].TargetDataName(target);
					if (dn.empty())
						continue;

					currentDiffs.ApplyDiff(dn, target, 1.0f, &verts);
				}
				else
					morpher.ApplyResultToVerts(morph->name, target, &verts);

				int i = 0;
				for (auto &v : verts) {
					if (!v.IsZero(true))
						morph->offsets.emplace(i, v);
					i++;
				}

				if (morph->offsets.size() > 0)
					tri.AddMorph(shape, morph);
			}
		}
	}

	if (!tri.Write(triFilePath))
		return false;

	return true;
}

int OutfitProject::SaveSliderBSD(const string& sliderName, const string& shapeName, const string& fileName) {
	string target = ShapeToTarget(shapeName);

	if (IsBaseShape(shapeName)) {
		string sliderData = activeSet[sliderName].TargetDataName(target);
		baseDiffData.SaveSet(sliderData, target, fileName + ".bsd");
	}
	else
		morpher.SaveResultDiff(target, sliderName, fileName + ".bsd");

	return 0;
}

int OutfitProject::SaveSliderOBJ(const string& sliderName, const string& shapeName, const string& fileName) {
	string target = ShapeToTarget(shapeName);
	vector<Triangle> tris;
	const vector<Vector3>* verts = workNif.GetRawVertsForShape(shapeName);
	workNif.GetTrisForShape(shapeName, &tris);

	vector<Vector3> outVerts = *verts;

	if (IsBaseShape(shapeName)) {
		string sliderData = activeSet[sliderName].TargetDataName(target);
		baseDiffData.ApplyDiff(sliderData, target, 1.0f, &outVerts);
	}
	else
		morpher.ApplyResultToVerts(sliderName, target, &outVerts);


	string mapfn = wxFileSelector("Choose vertex map source .obj", wxEmptyString, wxEmptyString, ".obj", "*.obj", wxFD_OPEN | wxFD_FILE_MUST_EXIST, owner);
	if (mapfn.empty())
		return 1;

	map<int, int> orderMap;
	vector<Face> origFaces;
	vector<Vector2> origUvs;
	ObjFile orderFile;
	orderFile.LoadVertOrderMap(mapfn, orderMap, origFaces, origUvs);

	vector<Vector3> swizzleverts(orderMap.size());
	for (int i = 0; i < orderMap.size(); i++)
		swizzleverts[i] = outVerts[orderMap[i]];

	ObjFile obj;
	obj.SetScale(Vector3(0.1f, 0.1f, 0.1f));
	obj.AddGroup(shapeName, swizzleverts, origFaces, origUvs);
	if (obj.Save(fileName))
		return 1;

	return 0;
}

void OutfitProject::SetSliderFromBSD(const string& sliderName, const string& shapeName, const string& fileName) {
	string target = ShapeToTarget(shapeName);
	if (IsBaseShape(shapeName)) {
		string sliderData = activeSet[sliderName].TargetDataName(target);
		baseDiffData.LoadSet(sliderData, target, fileName);
	}
	else {
		DiffDataSets tmpSet;
		tmpSet.LoadSet(sliderName, target, fileName);
		unordered_map<ushort, Vector3>* diff = tmpSet.GetDiffSet(sliderName);
		morpher.SetResultDiff(target, sliderName, (*diff));
	}
}

bool OutfitProject::SetSliderFromOBJ(const string& sliderName, const string& shapeName, const string& fileName) {
	string target = ShapeToTarget(shapeName);
	int type= workNif.GetShapeType(shapeName);
	ObjFile obj;
	if (type == BSSUBINDEXTRISHAPE) {
		obj.LoadForNif(fileName);
	//	obj.LoadSimple(fileName);
	}
	else {
		obj.LoadForNif(fileName);

	}
	vector<string> groupNames;
	obj.GetGroupList(groupNames);

	int i = 0;
	int index = 0;
	for (auto &n : groupNames) {
		if (n == shapeName) {
			index = i;
			break;
		}
		i++;
	}

	vector<Vector3> objVerts;
	obj.CopyDataForIndex(index, &objVerts, nullptr, nullptr);

	unordered_map<ushort, Vector3> diff;
	if (IsBaseShape(shapeName)) {
		if (workNif.CalcShapeDiff(shapeName, &objVerts, diff, 10.0f))
			return false;

		string sliderData = activeSet[sliderName].TargetDataName(target);
		baseDiffData.LoadSet(sliderData, target, diff);
	}
	else {
		if (workNif.CalcShapeDiff(shapeName, &objVerts, diff, 10.0f))
			return false;

		morpher.SetResultDiff(target, sliderName, diff);
	}
	return true;
}

bool OutfitProject::SetSliderFromFBX(const string& sliderName, const string& shapeName, const string& fileName) {
	string target = ShapeToTarget(shapeName);

	FBXWrangler fbxw;
	string invalidBones;

	bool result = fbxw.ImportScene(fileName);
	if (!result)
		return 1;

	vector<string>shapes;
	fbxw.GetShapeNames(shapes);
	bool found = false;
	for (auto &s : shapes)
		if (s == shapeName)
			found = true;

	if (!found)
		return false;

	FBXShape* shape = fbxw.InShape(shapeName);

	unordered_map<ushort, Vector3> diff;
	if (IsBaseShape(shapeName)) {
		if (workNif.CalcShapeDiff(shapeName, &shape->verts, diff, 1.0f))
			return false;

		string sliderData = activeSet[sliderName].TargetDataName(target);
		baseDiffData.LoadSet(sliderData, target, diff);
	}
	else {
		if (workNif.CalcShapeDiff(shapeName, &shape->verts, diff, 1.0f))
			return false;

		morpher.SetResultDiff(target, sliderName, diff);
	}

	return true;
}

void OutfitProject::SetSliderFromTRI(const string& sliderName, const string& shapeName, unordered_map<ushort, Vector3>& diff) {
	string target = ShapeToTarget(shapeName);
	if (IsBaseShape(shapeName)) {
		string sliderData = activeSet[sliderName].TargetDataName(target);
		baseDiffData.LoadSet(sliderData, target, diff);
	}
	else {
		morpher.EmptyResultDiff(target, sliderName);
		morpher.SetResultDiff(target, sliderName, diff);
	}
}

int OutfitProject::GetVertexCount(const string& shapeName) {
	if (workNif.IsValid())
		return workNif.GetVertCountForShape(shapeName);

	return -1;
}

void OutfitProject::GetLiveVerts(const string& shapeName, vector<Vector3>& outVerts) {
	string target = ShapeToTarget(shapeName);
	if (IsBaseShape(shapeName)) {
		string targetData;
		workNif.GetVertsForShape(shapeName, outVerts);
		for (int i = 0; i < activeSet.size(); i++) {
			if (activeSet[i].bShow && activeSet[i].curValue != 0.0f) {
				targetData = activeSet.ShapeToDataName(i, shapeName);
				if (targetData == "")
					continue;

				baseDiffData.ApplyDiff(targetData, target, activeSet[i].curValue, &outVerts);
			}
		}
	}
	else {
		workNif.GetVertsForShape(shapeName, outVerts);
		for (int i = 0; i < activeSet.size(); i++)
			if (activeSet[i].bShow && activeSet[i].curValue != 0.0f)
				morpher.ApplyResultToVerts(activeSet[i].name, target, &outVerts, activeSet[i].curValue);
	}
}

const string& OutfitProject::ShapeToTarget(const string& shapeName) {
	for (auto it = activeSet.TargetShapesBegin(); it != activeSet.TargetShapesEnd(); ++it)
		if (it->second == shapeName)
			return it->first;

	return shapeName;
}

void OutfitProject::GetShapes(vector<string>& outShapeNames) {
	workNif.GetShapeList(outShapeNames);
}

void OutfitProject::GetActiveBones(vector<string>& outBoneNames) {
	AnimSkeleton::getInstance().GetActiveBoneNames(outBoneNames);
}

void OutfitProject::GetBones(vector<string>& outBoneNames) {
	set<string> boneSet;

	vector<string> shapes;
	GetShapes(shapes);
	for (auto &s : shapes) {
		vector<string> curBones;
		workNif.GetShapeBoneList(s, curBones);
		boneSet.insert(curBones.begin(), curBones.end());
	}

	outBoneNames.assign(boneSet.begin(), boneSet.end());
}

string OutfitProject::GetShapeTexture(const string& shapeName) {
	if (shapeTextures.find(shapeName) != shapeTextures.end())
		return shapeTextures[shapeName];
	else
		return defaultTexFile;
}

void OutfitProject::SetTextures(const string& textureFile) {
	vector<string> shapes;
	GetShapes(shapes);
	for (auto &s : shapes)
		SetTexture(s, textureFile);
}

void OutfitProject::SetTexture(const string& shapeName, const string& textureFile) {
	if (shapeName.empty())
		return;

	if (textureFile == "_AUTO_") {
		string nifTexFile;
		workNif.GetTextureForShape(shapeName, nifTexFile);
		if (nifTexFile.empty())
			nifTexFile = "noimg.dds";

		string texturesDir = appConfig["GameDataPath"];
		wxString combinedTexFile = texturesDir + nifTexFile;
		shapeTextures[shapeName] = combinedTexFile.ToStdString();
	}
	else
		shapeTextures[shapeName] = textureFile;
}

bool OutfitProject::IsValidShape(const string& shapeName) {
	vector<string> shapes;
	GetShapes(shapes);
	for (auto &s : shapes)
		if (s == shapeName)
			return true;

	return false;
}

void OutfitProject::RefreshMorphShape(const string& shapeName) {
	morpher.UpdateMeshFromNif(workNif, shapeName);
}

void OutfitProject::UpdateShapeFromMesh(const string& shapeName, const mesh* m) {
	vector<Vector3> liveVerts;
	for (int i = 0; i < m->nVerts; i++)
		liveVerts.emplace_back(move(Vector3(m->verts[i].x * -10, m->verts[i].z * 10, m->verts[i].y * 10)));

	workNif.SetVertsForShape(shapeName, liveVerts);
	Clean(shapeName);
}

void OutfitProject::UpdateMorphResult(const string& shapeName, const string& sliderName, unordered_map<ushort, Vector3>& vertUpdates) {
	// Morph results are stored in two different places depending on whether it's an outfit or the base shape.
	// The outfit morphs are stored in the automorpher, whereas the base shape diff info is stored in directly in basediffdata.
	
	string target = ShapeToTarget(shapeName);
	string dataName = activeSet[sliderName].TargetDataName(target);
	if (!vertUpdates.empty()) {
		if (dataName.empty())
			activeSet[sliderName].AddDataFile(target, target + sliderName, target + sliderName);
		else
			activeSet[sliderName].SetLocalData(dataName);
	}

	if (IsBaseShape(shapeName)) {
		for (auto &i : vertUpdates) {
			Vector3 diffscale = Vector3(i.second.x * -10, i.second.z * 10, i.second.y * 10);
			baseDiffData.SumDiff(dataName, target, i.first, diffscale);
		}
	}
	else
		morpher.UpdateResultDiff(shapeName, sliderName, vertUpdates);
}

void OutfitProject::ScaleMorphResult(const string& shapeName, const string& sliderName, float scaleValue) {
	if (IsBaseShape(shapeName)) {
		string target = ShapeToTarget(shapeName);
		string dataName = activeSet[sliderName].TargetDataName(target);
		baseDiffData.ScaleDiff(dataName, target, scaleValue);
	}
	else
		morpher.ScaleResultDiff(shapeName, sliderName, scaleValue);
}

void OutfitProject::MoveVertex(const string& shapeName, const Vector3& pos, const int& id) {
	workNif.MoveVertex(shapeName, pos, id);
}

void OutfitProject::OffsetShape(const string& shapeName, const Vector3& xlate, unordered_map<ushort, float>* mask) {
	workNif.OffsetShape(shapeName, xlate, mask);
}

void OutfitProject::ScaleShape(const string& shapeName, const float& scale, unordered_map<ushort, float>* mask) {
	workNif.ScaleShape(shapeName, scale, mask);
}

void OutfitProject::RotateShape(const string& shapeName, const Vector3& angle, unordered_map<ushort, float>* mask) {
	workNif.RotateShape(shapeName, angle, mask);
}

void OutfitProject::CopyBoneWeights(const string& destShape, unordered_map<ushort, float>* mask, vector<string>* inBoneList) {
	if (baseShape.empty())
		return;

	vector<string> lboneList;
	vector<string>* boneList;

	owner->UpdateProgress(1, "Gathering bones...");

	if (!inBoneList) {
		for (auto &bn : workAnim.shapeBones[baseShape])
			lboneList.push_back(bn);

		boneList = &lboneList;
	}
	else
		boneList = inBoneList;

	DiffDataSets dds;
	unordered_map<ushort, float> weights;
	for (auto &bone : *boneList) {
		weights.clear();
		dds.AddEmptySet(bone + "_WT_", "Weight");
		workAnim.GetWeights(baseShape, bone, weights);
		for (auto &w : weights) {
			Vector3 tmp;
			tmp.y = w.second;
			dds.UpdateDiff(bone + "_WT_", "Weight", w.first, tmp);
		}
	}

	owner->UpdateProgress(10, "Initializing proximity data...");

	InitConform();
	morpher.LinkRefDiffData(&dds);
	morpher.BuildProximityCache(destShape);

	int step = 40 / boneList->size();
	int prog = 40;
	owner->UpdateProgress(prog);

	for (auto &boneName : *boneList) {
		string wtSet = boneName + "_WT_";
		morpher.GenerateResultDiff(destShape, wtSet, wtSet);

		unordered_map<ushort, Vector3> diffResult;
		morpher.GetRawResultDiff(destShape, wtSet, diffResult);

		unordered_map<ushort, float> oldWeights;
		if (mask) {
			weights.clear();
			oldWeights.clear();
			workAnim.GetWeights(destShape, boneName, oldWeights);
		}

		for (auto &dr : diffResult) {
			if (mask) {
				if (1.0f - (*mask)[dr.first] > 0.0f)
					weights[dr.first] = dr.second.y * (1.0f - (*mask)[dr.first]);
				else
					weights[dr.first] = oldWeights[dr.first];
			}
			else
				weights[dr.first] = dr.second.y;
		}

		if (diffResult.size() > 0) {
			AnimBone boneRef;
			AnimSkeleton::getInstance().GetBone(boneName, boneRef);
			if (workAnim.AddShapeBone(destShape, boneRef)) {
				if (owner->targetGame == FO4) {
					// Fallout 4 bone transforms are stored in a bonedata structure per shape versus the node transform in the skeleton data.  
					SkinTransform xForm;
					Vector3 sphereOffset;
					float sphereRadius;
					workNif.GetShapeBoneTransform(baseShape, boneName, xForm, sphereOffset, sphereRadius);

					workAnim.SetShapeBoneXForm(destShape, boneName, xForm);
				}
				else {
					SkinTransform xForm;
					workAnim.GetBoneXForm(boneName, xForm);
					workAnim.SetShapeBoneXForm(destShape, boneName, xForm);

				}
			}
		}

		workAnim.SetWeights(destShape, boneName, weights);
		owner->UpdateProgress(prog += step, "Copying bone weights...");
	}

	morpher.UnlinkRefDiffData();
	owner->UpdateProgress(90);
}

void OutfitProject::TransferSelectedWeights(const string& destShape, unordered_map<ushort, float>* mask, vector<string>* inBoneList) {
	if (baseShape.empty())
		return;

	owner->UpdateProgress(10, "Gathering bones...");

	vector<string>* boneList;
	if (!inBoneList) {
		vector<string> allBoneList;
		for (auto &boneName : workAnim.shapeBones[baseShape])
			allBoneList.push_back(boneName);

		boneList = &allBoneList;
	}
	else
		boneList = inBoneList;

	int step = 50 / boneList->size();
	int prog = 40;
	owner->UpdateProgress(prog, "Transferring bone weights...");

	unordered_map<ushort, float> weights;
	unordered_map<ushort, float> oldWeights;
	for (auto &boneName : *boneList) {
		weights.clear();
		oldWeights.clear();
		workAnim.GetWeights(baseShape, boneName, weights);
		workAnim.GetWeights(destShape, boneName, oldWeights);

		for (auto &w : weights) {
			if (mask) {
				if (1.0f - (*mask)[w.first] > 0.0f)
					weights[w.first] = w.second * (1.0f - (*mask)[w.first]);
				else
					weights[w.first] = oldWeights[w.first];
			}
			else
				weights[w.first] = w.second;
		}

		AnimBone boneRef;
		AnimSkeleton::getInstance().GetBone(boneName, boneRef);
		if (workAnim.AddShapeBone(destShape, boneRef)) {
			if (owner->targetGame == FO4) {
				// Fallout 4 bone transforms are stored in a bonedata structure per shape versus the node transform in the skeleton data.  
				SkinTransform xForm;
				Vector3 sphereOffset;
				float sphereRadius;
				workNif.GetShapeBoneTransform(baseShape, boneName, xForm, sphereOffset, sphereRadius);

				workAnim.SetShapeBoneXForm(destShape, boneName, xForm);
			}
			else {
				SkinTransform xForm;
				workAnim.GetBoneXForm(boneName, xForm);
				workAnim.SetShapeBoneXForm(destShape, boneName, xForm);

			}
		}

		workAnim.SetWeights(destShape, boneName, weights);
		owner->UpdateProgress(prog += step, "");
	}

	owner->UpdateProgress(100, "Finished");
}

bool OutfitProject::HasUnweighted() {
	vector<string> shapes;
	GetShapes(shapes);
	for (auto &s : shapes) {
		vector<string> bones;
		vector<Vector3> verts;
		GetBones(bones);
		workNif.GetVertsForShape(s, verts);

		unordered_map<int, int> influences;
		for (int i = 0; i < verts.size(); i++)
			influences.emplace(i, 0);

		unordered_map<ushort, float> boneWeights;
		for (auto &b : bones) {
			boneWeights.clear();
			workAnim.GetWeights(s, b, boneWeights);
			for (int i = 0; i < verts.size(); i++) {
				auto id = boneWeights.find(i);
				if (id != boneWeights.end())
					influences.at(i)++;
			}
		}

		mesh* m = owner->glView->GetMesh(s);
		bool unweighted = false;
		for (auto &i : influences) {
			if (i.second == 0) {
				if (!unweighted)
					m->ColorFill(Vector3(0.0f, 0.0f, 0.0f));
				m->vcolors[i.first].x = 1.0f;
				unweighted = true;
			}
		}
		if (unweighted)
			return true;
	}
	return false;
}

void OutfitProject::ApplyBoneScale(const string& bone, int sliderPos, bool clear) {
	vector<string> bones;
	vector<Vector3> boneRot;
	Vector3 boneTranslation;
	float boneScale;

	ClearBoneScale(false);

	vector<string> shapes;
	GetShapes(shapes);
	for (auto &s : shapes) {
		auto it = boneScaleVerts.find(s);
		if (it == boneScaleVerts.end()) {
			mesh* m = owner->glView->GetMesh(s);
			boneScaleVerts.emplace(s, vector<Vector3>(m->nVerts));
			it = boneScaleVerts.find(s);
			for (int i = 0; i < m->nVerts; i++)
				it->second[i] = move(Vector3(m->verts[i].x * -10, m->verts[i].z * 10, m->verts[i].y * 10));
		}

		vector<Vector3>* verts = &it->second;

		it = boneScaleOffsets.find(s);
		if (it == boneScaleOffsets.end())
			boneScaleOffsets.emplace(s, vector<Vector3>(verts->size()));
		it = boneScaleOffsets.find(s);

		workNif.GetShapeBoneList(s, bones);
		for (auto &b : bones) {
			if (b == bone) {
				workNif.GetNodeTransform(b, boneRot, boneTranslation, boneScale);
				if (workWeights[s].empty())
					workAnim.GetWeights(s, b, workWeights[s]);

				for (auto &w : workWeights[s]) {
					Vector3 dir = (*verts)[w.first] - boneTranslation;
					dir.Normalize();
					Vector3 offset = dir * w.second * sliderPos / 5.0f;
					(*verts)[w.first] += offset;
					it->second[w.first] += offset;
				}
			}
		}

		if (clear)
			owner->glView->UpdateMeshVertices(s, verts);
		else
			owner->glView->UpdateMeshVertices(s, verts, false, false);
	}
}

void OutfitProject::ClearBoneScale(bool clear) {
	if (boneScaleOffsets.empty())
		return;

	vector<string> shapes;
	GetShapes(shapes);

	for (auto &s : shapes) {
		auto it = boneScaleVerts.find(s);
		vector<Vector3>* verts = &it->second;

		it = boneScaleOffsets.find(s);
		if (it != boneScaleOffsets.end()) {
			if (verts->size() == it->second.size()) {
				for (int i = 0; i < verts->size(); i++)
					(*verts)[i] -= it->second[i];

				if (clear) {
					owner->glView->UpdateMeshVertices(s, verts);
					workWeights.clear();
				}
				else
					owner->glView->UpdateMeshVertices(s, verts, false, false);
			}
		}
	}

	boneScaleVerts.clear();
	boneScaleOffsets.clear();
}

void OutfitProject::AddBoneRef(const string& boneName) {
	AnimBone boneRef;
	AnimSkeleton::getInstance().GetBone(boneName, boneRef);

	vector<string> shapes;
	GetShapes(shapes);
	for (auto &s : shapes) {
		if (IsBaseShape(s))
			continue;

		if (workAnim.AddShapeBone(s, boneRef)) {
			SkinTransform xForm;
			workAnim.GetBoneXForm(boneName, xForm);
			workAnim.SetShapeBoneXForm(s, boneName, xForm);
		}
	}
}

void OutfitProject::BuildShapeSkinPartions(const string& destShape) {
	workAnim.WriteToNif(&workNif);
	workNif.BuildSkinPartitions(destShape);
}

void OutfitProject::ClearWorkSliders() {
	morpher.ClearResultDiff();
}

void OutfitProject::ClearReference() {
	if (!baseShape.empty())
		DeleteShape(baseShape);

	if (activeSet.size() > 0)
		activeSet.Clear();

	morpher.UnlinkRefDiffData();

	baseShape = "";
}

void OutfitProject::ClearOutfit() {
	vector<string> shapes;
	GetShapes(shapes);
	for (auto &s : shapes) {
		if (IsBaseShape(s))
			continue;

		DeleteShape(s);
	}
	ClearWorkSliders();
}

void OutfitProject::ClearSlider(const string& shapeName, const string& sliderName) {
	string target = ShapeToTarget(shapeName);

	if (IsBaseShape(shapeName)) {
		string data = activeSet[sliderName].TargetDataName(target);
		baseDiffData.EmptySet(data, target);
	}
	else
		morpher.EmptyResultDiff(target, sliderName);
}

void OutfitProject::ClearUnmaskedDiff(const string& shapeName, const string& sliderName, unordered_map<ushort, float>* mask) {
	string target = ShapeToTarget(shapeName);

	if (IsBaseShape(shapeName)) {
		string data = activeSet[sliderName].TargetDataName(target);
		baseDiffData.ZeroVertDiff(data, target, nullptr, mask);
	}
	else
		morpher.ZeroVertDiff(target, sliderName, nullptr, mask);
}

void OutfitProject::DeleteSlider(const string& sliderName) {
	baseDiffData.ClearSet(sliderName);
	morpher.ClearResultSet(sliderName);
	activeSet.DeleteSlider(sliderName);
}

int OutfitProject::LoadSkeletonReference(const string& skeletonFileName) {
	return AnimSkeleton::getInstance().LoadFromNif(skeletonFileName);
}

int OutfitProject::LoadReferenceTemplate(const string& templateName, bool clearRef) {
	vector<string> templateNames;
	vector<string> templateFiles;
	vector<string> templateSets;
	vector<string> templateShapes;
	Config.GetValueArray("ReferenceTemplates", "Template", templateNames);
	Config.GetValueAttributeArray("ReferenceTemplates", "Template", "sourcefile", templateFiles);
	Config.GetValueAttributeArray("ReferenceTemplates", "Template", "set", templateSets);
	Config.GetValueAttributeArray("ReferenceTemplates", "Template", "shape", templateShapes);

	string srcFile;
	string srcSet;
	string srcShape;

	for (int i = 0; i < templateNames.size(); i++) {
		if (templateNames[i] == templateName) {
			if (templateFiles.size() >= templateNames.size())
				srcFile = templateFiles[i];
			if (templateSets.size() >= templateNames.size())
				srcSet = templateSets[i];
			if (templateShapes.size() >= templateNames.size())
				srcShape = templateShapes[i];
		}
	}
	if (srcFile.empty() || srcSet.empty()) {
		wxLogError("Template source entries are invalid.");
		wxMessageBox("Template source entries are invalid.", "Reference Error", wxICON_ERROR);
		return 1;
	}

	return LoadReference(srcFile, srcSet, clearRef, srcShape);
}

int OutfitProject::LoadReferenceNif(const string& fileName, const string& shapeName, bool ClearRef) {
	if (ClearRef)
		ClearReference();

	NifFile refNif;
	int error = refNif.Load(fileName);
	if (error) {
		if (error == 2) {
			wxString errorText = wxString::Format("NIF version not supported!\n\nFile: %s\n%s\nUser Version: %i\nUser Version 2: %i",
				refNif.GetFileName(), refNif.hdr.verStr, refNif.hdr.userVersion, refNif.hdr.userVersion2);

			wxLogError(errorText);
			wxMessageBox(errorText, "Reference Error", wxICON_ERROR);
			return 3;
		}

		wxLogError("Could not load reference NIF file '%s'!", fileName);
		wxMessageBox(wxString::Format("Could not load reference NIF file '%s'!", fileName), "Reference Error", wxICON_ERROR);
		return 2;
	}

	baseShape = shapeName;

	vector<string> shapes;
	GetShapes(shapes);
	for (auto &s : shapes) {
		if (IsBaseShape(s)) {
			string newName = s + "_ref";
			refNif.RenameShape(s, newName);
			baseShape = newName;
			break;
		}
	}

	if (workNif.IsValid()) {
		workNif.CopyGeometry(baseShape, refNif, baseShape);
		workAnim.LoadFromNif(&workNif, baseShape);
	}
	else {
		workNif.CopyFrom(refNif);
		workAnim.LoadFromNif(&workNif);
	}

	activeSet.LoadSetDiffData(baseDiffData);
	AutoOffset(workNif);
	return 0;
}

int OutfitProject::LoadReference(const string& fileName, const string& setName, bool ClearRef, const string& shapeName) {
	if (ClearRef)
		ClearReference();

	string oldTarget;
	SliderSetFile sset(fileName);
	int oldVertCount = -1;
	int newVertCount;

	if (sset.fail()) {
		wxLogError("Could not load slider set file '%s'!", fileName);
		wxMessageBox(wxString::Format("Could not load slider set file '%s'!", fileName), "Reference Error", wxICON_ERROR);
		return 1;
	}

	sset.GetSet(setName, activeSet);
	activeSet.SetBaseDataPath(Config["ShapeDataPath"]);
	activeSet.SetAllReferenced();

	string inMeshFile = activeSet.GetInputFileName();

	if (!ClearRef)
		oldVertCount = GetVertexCount(baseShape);

	NifFile refNif;
	int error = refNif.Load(inMeshFile);
	if (error) {
		if (error == 2) {
			wxString errorText = wxString::Format("NIF version not supported!\n\nFile: %s\n%s\nUser Version: %i\nUser Version 2: %i",
				refNif.GetFileName(), refNif.hdr.verStr, refNif.hdr.userVersion, refNif.hdr.userVersion2);

			wxLogError(errorText);
			wxMessageBox(errorText, "Reference Error", wxICON_ERROR);
			ClearReference();
			return 5;
		}

		ClearReference();
		wxLogError("Could not load reference NIF file '%s'!", inMeshFile);
		wxMessageBox(wxString::Format("Could not load reference NIF file '%s'!", inMeshFile), "Reference Error", wxICON_ERROR);
		return 2;
	}

	vector<string> shapes;
	refNif.GetShapeList(shapes);
	if (shapes.empty()) {
		ClearReference();
		wxLogError("Reference NIF file '%s' does not contain any shapes.", refNif.GetFileName());
		wxMessageBox(wxString::Format("Reference NIF file '%s' does not contain any shapes.", refNif.GetFileName()), "Reference Error", wxICON_ERROR);
		return 3;
	}

	string shape = shapeName;
	if (shape.empty())
		shape = shapes[0];

	GetShapes(shapes);
	for (auto &s : shapes) {
		if (s == shape) {
			string newName = s + "_ref";
			refNif.RenameShape(s, newName);
			shape = newName;
			break;
		}
	}

	if (!ClearRef) {
		SliderSet tmpSS;
		sset.GetSet(setName, tmpSS);
		for (int i = 0; i < tmpSS.size(); i++)
			activeSet.DeleteSlider(tmpSS[i].name);

		oldTarget = ShapeToTarget(baseShape);
		activeSet.RenameShape(baseShape, shape);
		activeSet.ClearTargets(oldTarget);
	}

	newVertCount = refNif.GetVertCountForShape(shape);
	if (newVertCount == -1) {
		ClearReference();
		wxLogError("Shape '%s' not found in reference NIF file '%s'!", shape, refNif.GetFileName());
		wxMessageBox(wxString::Format("Shape '%s' not found in reference NIF file '%s'!", shape, refNif.GetFileName()), "Reference Error", wxICON_ERROR);
		return 4;
	}

	if (workNif.IsValid()) {
		workNif.CopyGeometry(shape, refNif, shape);
		workAnim.LoadFromNif(&workNif, shape);
	}
	else {
		workNif.CopyFrom(refNif);
		workAnim.LoadFromNif(&workNif);
	}

	if (oldVertCount > 0 && oldVertCount == newVertCount) {
		string newTarget = ShapeToTarget(shape);
		if (!oldTarget.empty() && newTarget != oldTarget)
			activeSet.Retarget(oldTarget, newTarget);
	}

	baseShape = shape;

	activeSet.LoadSetDiffData(baseDiffData);
	AutoOffset(workNif);

	return 0;
}

int OutfitProject::AddNif(const string& fileName, bool clear, const string& inOutfitName) {
	if (clear)
		ClearOutfit();

	if (fileName.empty()) {
		wxLogMessage("No outfit selected.");
		return 0;
	}

	if (!inOutfitName.empty())
		outfitName = inOutfitName;
	else if (outfitName.empty())
		outfitName = "New Outfit";

	if (clear) {
		size_t fnpos = fileName.rfind("\\");
		if (fnpos == string::npos) {
			mGameFile = fileName;
		}
		else {
			mGameFile = fileName.substr(fnpos + 1);
			size_t pos;
			pos = mGameFile.rfind("_1.nif");
			if (pos == string::npos)
				pos = mGameFile.rfind("_0.nif");

			if (pos == string::npos)
				pos = mGameFile.rfind(".nif");

			if (pos != string::npos)
				mGameFile = mGameFile.substr(0, pos);

			pos = fileName.find("meshes");
			if (pos != string::npos)
				mGamePath = fileName.substr(pos, fnpos - pos);
			else
				mGamePath = "";
		}
	}

	NifFile nif;
	int error = nif.Load(fileName);
	if (error) {
		if (error == 2) {
			wxString errorText = wxString::Format("NIF version not supported!\n\nFile: %s\n%s\nUser Version: %i\nUser Version 2: %i",
				nif.GetFileName(), nif.hdr.verStr, nif.hdr.userVersion, nif.hdr.userVersion2);

			wxLogError(errorText);
			wxMessageBox(errorText, "NIF Error", wxICON_ERROR);
			return 4;
		}

		wxLogError("Could not load NIF file '%s'!", fileName);
		wxMessageBox(wxString::Format("Could not load NIF file '%s'!", fileName), "NIF Error", wxICON_ERROR);
		return 1;
	}

	vector<string> nifShapes;
	nif.GetShapeList(nifShapes);
	for (auto &s : nifShapes)
		nif.RenameDuplicateShape(s);

	if (!baseShape.empty())
		nif.RenameShape(baseShape, baseShape + "_outfit");

	char chars[] = { '\\', '/', '?', ':', '*', '>', '<', '|', '"' };
	vector<string> shapes;
	GetShapes(shapes);

	nif.GetShapeList(nifShapes);
	for (auto &s : nifShapes) {
		string oldName = s;
		for (uint i = 0; i < sizeof(chars); ++i)
			replace(s.begin(), s.end(), chars[i], '_');

		if (oldName != s)
			nif.RenameShape(oldName, s);

		vector<string> uniqueShapes;
		nif.GetShapeList(uniqueShapes);
		uniqueShapes.insert(uniqueShapes.end(), shapes.begin(), shapes.end());

		string newName = s;
		int uniqueCount = 0;
		for (;;) {
			auto foundShape = find(uniqueShapes.begin(), uniqueShapes.end(), newName);
			if (foundShape != uniqueShapes.end()) {
				uniqueShapes.erase(foundShape);
				uniqueCount++;
				if (uniqueCount > 1)
					newName = s + wxString::Format("_%d", uniqueCount).ToStdString();
			}
			else {
				if (uniqueCount > 1)
					nif.RenameShape(s, newName);
				break;
			}
		}
	}

	AutoOffset(nif);

	// Add cloth data block of NIF to the list
	vector<BSClothExtraData*> clothDataBlocks = nif.GetChildren<BSClothExtraData>((NiNode*)nif.GetBlock(0), true);
	for (auto &cloth : clothDataBlocks)
		clothData[fileName] = *cloth;

	nif.DeleteBlockByType("BSClothExtraData");

	nif.GetShapeList(nifShapes);
	if (workNif.IsValid()) {
		for (auto &s : nifShapes) {
			workNif.CopyGeometry(s, nif, s);
			workAnim.LoadFromNif(&workNif, s);
		}
	}
	else {
		workNif.CopyFrom(nif);
		workAnim.LoadFromNif(&workNif);
	}

	return 0;
}

int OutfitProject::OutfitFromSliderSet(const string& fileName, const string& sliderSetName) {
	owner->StartProgress("Loading slider set...");
	SliderSetFile InSS(fileName);
	if (InSS.fail()) {
		owner->EndProgress();
		return 1;
	}

	owner->UpdateProgress(20, "Retrieving sliders...");
	if (InSS.GetSet(sliderSetName, activeSet)) {
		owner->EndProgress();
		return 3;
	}

	activeSet.SetBaseDataPath(Config["ShapeDataPath"]);
	string inputNif = activeSet.GetInputFileName();

	owner->UpdateProgress(30, "Loading outfit shapes...");
	if (AddNif(inputNif, true, sliderSetName)) {
		owner->EndProgress();
		return 4;
	}

	// First external target with skin shader becomes reference
	vector<string> refTargets;
	activeSet.GetReferencedTargets(refTargets);
	for (auto &target : refTargets) {
		if (workNif.IsShaderSkin(target)) {
			baseShape = activeSet.TargetToShape(target);
			break;
		}
	}

	// Prevent duplication if valid reference was found
	if (!baseShape.empty())
		DeleteShape(baseShape);

	owner->UpdateProgress(90, "Updating slider data...");
	morpher.LoadResultDiffs(activeSet);

	mFileName = fileName;
	mOutfitName = sliderSetName;
	mDataDir = activeSet.GetDefaultDataFolder();
	mBaseFile = activeSet.GetInputFileName();
	size_t slashpos = mBaseFile.rfind("\\");
	if (slashpos != string::npos)
		mBaseFile = mBaseFile.substr(slashpos + 1);

	mGamePath = activeSet.GetOutputPath();
	mGameFile = activeSet.GetOutputFile();
	mCopyRef = true;
	mGenWeights = activeSet.GenWeights();

	owner->UpdateProgress(100, "Finished");
	owner->EndProgress();
	return 0;
}

void OutfitProject::AutoOffset(NifFile& nif) {
	vector<string> shapes;
	nif.GetShapeList(shapes);

	bool applyOverallSkin = true;
	wxMenuBar* menu = (wxMenuBar*)owner->GetMenuBar();
	if (menu)
		applyOverallSkin = menu->IsChecked(XRCID("applyOverallSkin"));

	for (auto &s : shapes) {
		Matrix4 localGeom;
		nif.GetShapeTransform(s, localGeom);

		Vector3 dummyVec3;
		float dummyFloat;
		SkinTransform xFormSkinAll;
		nif.GetShapeBoneTransform(s, -1, xFormSkinAll, dummyVec3, dummyFloat);

		Matrix4 skinAllInv = xFormSkinAll.ToMatrix().Inverse();

		vector<Vector3> verts;
		nif.GetVertsForShape(s, verts);
		for (auto &v : verts) {
			if (applyOverallSkin)
				v = (localGeom * skinAllInv) * v;
			else
				v = localGeom * v;
		}
		nif.SetVertsForShape(s, verts);

		SkinTransform defXForm;
		nif.SetShapeBoneTransform(s, -1, defXForm, dummyVec3, dummyFloat);
		nif.ClearShapeTransform(s);
	}

	nif.ClearRootTransform();
}

void OutfitProject::InitConform() {
	morpher.SetRef(workNif, baseShape);
	morpher.LinkRefDiffData(&baseDiffData);
	morpher.SourceShapesFromNif(workNif);
}

void OutfitProject::ConformShape(const string& shapeName) {
	if (!workNif.IsValid() || baseShape.empty())
		return;

	morpher.BuildProximityCache(shapeName);

	string refTarget = ShapeToTarget(baseShape);
	for (int i = 0; i < activeSet.size(); i++)
		if (SliderShow(i))
			morpher.GenerateResultDiff(shapeName, activeSet[i].name, activeSet[i].TargetDataName(refTarget));
}

void OutfitProject::DuplicateShape(const string& sourceShape, const string& destShape, const mesh* m) {
	NifFile clone(workNif);
	workAnim.WriteToNif(&clone);

	vector<Vector3> liveVerts;
	//vector<Vector3> liveNorms;
	for (int i = 0; i < m->nVerts; i++) {
		liveVerts.emplace_back(move(Vector3(m->verts[i].x * -10, m->verts[i].z * 10, m->verts[i].y * 10)));
		//liveNorms.emplace_back(move(Vector3(m->verts[i].nx* -1, m->verts[i].nz, m->verts[i].ny)));
	}
	clone.SetVertsForShape(m->shapeName, liveVerts);

	workNif.CopyGeometry(destShape, clone, sourceShape);
	workAnim.LoadFromNif(&workNif, destShape);
}

void OutfitProject::RenameShape(const string& shapeName, const string& newShapeName) {
	workNif.RenameShape(shapeName, newShapeName);
	workAnim.RenameShape(shapeName, newShapeName);

	if (!IsBaseShape(shapeName))
		morpher.RenameShape(shapeName, newShapeName);

	activeSet.RenameShape(shapeName, newShapeName);

	if (shapeDirty.find(shapeName) != shapeDirty.end()) {
		shapeDirty.erase(shapeName);
		shapeDirty[newShapeName] = true;
	}

	wxLogMessage("Renamed shape '%s' to '%s'.", shapeName, newShapeName);
}

void OutfitProject::UpdateNifNormals(NifFile* nif, const vector<mesh*>& shapeMeshes) {
	vector<Vector3> liveNorms;
	for (auto &m : shapeMeshes) {
		if (nif->IsShaderSkin(m->shapeName) && owner->targetGame != FO4)
			continue;

		liveNorms.clear();
		for (int i = 0; i < m->nVerts; i++)
			liveNorms.emplace_back(move(Vector3(m->verts[i].nx* -1, m->verts[i].nz, m->verts[i].ny)));

		nif->SetNormalsForShape(m->shapeName, liveNorms);
		nif->CalcTangentsForShape(m->shapeName);
	}
}

int OutfitProject::SaveOutfitNif(const string& fileName, const vector<mesh*>& modMeshes, bool writeNormals, bool withRef) {
	NifFile clone(workNif);
	clone.SetNodeName(0, "Scene Root");

	ChooseClothData(clone);

	vector<Vector3> liveVerts;
	vector<Vector3> liveNorms;
	for (auto &m : modMeshes) {
		liveVerts.clear();
		liveNorms.clear();
		for (int i = 0; i < m->nVerts; i++) {
			liveVerts.emplace_back(move(Vector3(m->verts[i].x * -10, m->verts[i].z * 10, m->verts[i].y * 10)));
			liveNorms.emplace_back(move(Vector3(m->verts[i].nx * -1, m->verts[i].nz, m->verts[i].ny)));
		}
		clone.SetVertsForShape(m->shapeName, liveVerts);

		if (writeNormals) {
			if (clone.IsShaderSkin(m->shapeName) && owner->targetGame != FO4)
				continue;

			clone.SetNormalsForShape(m->shapeName, liveNorms);
			clone.CalcTangentsForShape(m->shapeName);
		}
	}

	if (!withRef && !baseShape.empty()) {
		clone.DeleteShape(baseShape);
		workAnim.WriteToNif(&clone, true, baseShape);
	}
	else
		workAnim.WriteToNif(&clone);

	vector<string> shapes;
	clone.GetShapeList(shapes);
	for (auto &s : shapes)
		clone.UpdateSkinPartitions(s);

	if (!clone.HasUnknown()) {
		clone.SetShapeOrder(owner->GetShapeList());
		clone.PrettySortBlocks();
	}
	return clone.Save(fileName);
}


void OutfitProject::ChooseClothData(NifFile& nif) {
	if (!clothData.empty()) {
		wxArrayString clothFileNames;
		clothFileNames.Add("None");
		for (auto &cloth : clothData)
			clothFileNames.Add(cloth.first);

		wxSingleChoiceDialog clothDataChoice(owner, "There was cloth physics data loaded at some point (BSClothExtraData). Please choose which origin to use in the output.", "Choose cloth data", clothFileNames);
		if (clothDataChoice.ShowModal() == wxID_CANCEL)
			return;

		string sel = clothDataChoice.GetStringSelection().ToStdString();
		if (sel != "None") {
			BSClothExtraData* clothBlock = new BSClothExtraData(nif.hdr);
			clothBlock->Clone(&clothData[sel]);

			int id = nif.AddBlock(clothBlock, "BSClothExtraData");
			if (id != -1) {
				NiNode* root = (NiNode*)nif.GetBlock(0);
				if (root) {
					root->numExtraData++;
					root->extraDataRef.push_back(id);
				}
			}
		}
	}
}

int OutfitProject::ImportShapeFBX(const string& fileName, const string& shapeName, const string& mergeShape) {
	FBXWrangler fbxw;
	string invalidBones;

	bool result = fbxw.ImportScene(fileName);
	if (!result)
		return 1;

	if (!shapeName.empty())
		outfitName = shapeName;
	else if (outfitName.empty())
		outfitName = "New Outfit";

	vector<string>shapes;
	fbxw.GetShapeNames(shapes);
	for (auto &s : shapes) {
		FBXShape* shape = fbxw.InShape(s);
		string useShapeName = s;

		if (!mergeShape.empty()) {
			vector<Vector3> shapeVerts;
			workNif.GetVertsForShape(mergeShape, shapeVerts);
			if (shapeVerts.size() == shape->numverts) {
				int ret = wxMessageBox("The vertex count of the selected .fbx file matches the currently selected outfit shape.  Do you wish to update the current shape?  (click No to create a new shape)", "Merge or New", wxYES_NO | wxICON_QUESTION);
				if (ret == wxYES) {
					ret = wxMessageBox("Update Vertex Positions?", "Vertex Position Update", wxYES_NO | wxICON_QUESTION);
					if (ret == wxYES)
						workNif.SetVertsForShape(mergeShape, shape->verts);

					ret = wxMessageBox("Update Animation Weighting?", "Animation Weight Update", wxYES_NO | wxICON_QUESTION);
					if (ret == wxYES)
						for (auto &bn : shape->boneNames)
							workAnim.SetWeights(mergeShape, bn, shape->boneSkin[bn].vertweights);

					return 101;
				}
			}

			useShapeName = wxGetTextFromUser("Please specify a name for the new shape", "New Shape Name", useShapeName);
			if (useShapeName.empty())
				return 100;
		}

		CreateNifShapeFromData(s, shape->verts, shape->tris, shape->uvs, &shape->normals);

		int slot = 0;
		vector<int> boneIndices;
		for (auto &bn : shape->boneNames) {
			if (!AnimSkeleton::getInstance().RefBone(bn)) {
				// Not found in reference skeleton, use default values
				AnimBone& cstm = AnimSkeleton::getInstance().AddBone(bn, true);
				if (!cstm.isValidBone)
					invalidBones += bn + "\n";

				AnimSkeleton::getInstance().RefBone(bn);
			}

			workAnim.shapeBones[useShapeName].push_back(bn);
			workAnim.shapeSkinning[useShapeName].boneNames[bn] = slot;
			workAnim.SetWeights(useShapeName, bn, shape->boneSkin[bn].vertweights);
			boneIndices.push_back(slot++);
		}

		workNif.SetShapeBoneIDList(useShapeName, boneIndices);

		if (!invalidBones.empty()) {
			wxLogWarning("Bones in shape '%s' not found in reference skeleton:\n%s", useShapeName, invalidBones);
			wxMessageBox(wxString::Format("Bones in shape '%s' not found in reference skeleton:\n\n%s", useShapeName, invalidBones), "Invalid Bones");
		}
	}

	return 0;
}

int OutfitProject::ExportShapeFBX(const string& fileName, const string& shapeName) {
	FBXWrangler fbxw;
	fbxw.NewScene();
	/*if (shapeName != "") {	
		vector<Triangle> tris;
		workNif.GetTrisForShape(shapeName, &tris);
		const vector<Vector3>* verts = workNif.GetRawVertsForShape(shapeName);
		const vector<Vector3>* norms = workNif.GetNormalsForShape(shapeName);
		const vector<Vector2>* uvs = workNif.GetUvsForShape(shapeName);
		fbxw.AddGeometry(shapeName, verts, norms, &tris, uvs);
	}*/

	fbxw.AddSkeleton(&AnimSkeleton::getInstance().refSkeletonNif);
	fbxw.AddNif(&workNif, shapeName, false);
	fbxw.AddSkinning(&workAnim, shapeName);
	//fbxw.AddNif(&AnimSkeleton::getInstance().refSkeletonNif);
	//fbxw.AddSkeleton();

	return fbxw.ExportScene(fileName);
}

int OutfitProject::ExportShapeObj(const string& fileName, const string& shapeName, Vector3 scale, Vector3 offset) {
	vector<Triangle> tris;
	workNif.GetTrisForShape(shapeName, &tris);
	const vector<Vector3>* verts = workNif.GetRawVertsForShape(shapeName);
	const vector<Vector2>* uvs = workNif.GetUvsForShape(shapeName);

	Vector3 shapeTrans;
	workNif.GetShapeTranslation(shapeName, shapeTrans);
	Vector3 offs(shapeTrans.x + offset.x, shapeTrans.y + offset.y, shapeTrans.z + offset.z);

	ObjFile obj;
	obj.SetScale(scale);
	obj.SetOffset(offs);

	obj.AddGroup(shapeName, *verts, tris, *uvs);
	if (obj.Save(fileName))
		return 1;

	return 0;
}

string OutfitProject::NameAbbreviate(const string& inputName) {
	string o;
	string stripChars = "\\/?:*><|\"";
	//string stripChars = "\"'\t\n\\/";
	for (auto &c : inputName) {
		if (stripChars.find(c) != string::npos)
			continue;

		o += c;
	}
	return o;
}
