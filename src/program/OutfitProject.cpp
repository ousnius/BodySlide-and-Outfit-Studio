/*
BodySlide and Outfit Studio
Copyright (C) 2017  Caliente & ousnius
See the included LICENSE file
*/

#include "OutfitProject.h"
#include "../files/TriFile.h"
#include "../files/FBXWrangler.h"
#include "../program/FBXImportDialog.h"

#include "FSEngine/FSManager.h"
#include "FSEngine/FSEngine.h"

#include <sstream>
#include <regex>

OutfitProject::OutfitProject(ConfigurationManager& inConfig, OutfitStudio* inOwner) : appConfig(inConfig) {
	morpherInitialized = false;
	owner = inOwner;
	string defSkelFile = Config["Anim/DefaultSkeletonReference"];
	LoadSkeletonReference(defSkelFile);

	mCopyRef = true;
	if (owner->targetGame == SKYRIM || owner->targetGame == SKYRIMSE)
		mGenWeights = true;
	else
		mGenWeights = false;
}

OutfitProject::~OutfitProject() {
}

string OutfitProject::Save(const wxString& strFileName,
	const wxString& strOutfitName,
	const wxString& strDataDir,
	const wxString& strBaseFile,
	const wxString& strGamePath,
	const wxString& strGameFile,
	bool genWeights,
	bool copyRef) {

	owner->UpdateProgress(1, _("Checking destination..."));
	string errmsg = "";
	string outfitName = strOutfitName;
	string baseFile = strBaseFile;
	string gameFile = strGameFile;

	ReplaceForbidden(outfitName);
	ReplaceForbidden(baseFile);
	ReplaceForbidden(gameFile);

	SliderSet outSet;
	outSet.SetName(outfitName);
	outSet.SetDataFolder(strDataDir.ToStdString());
	outSet.SetInputFile(baseFile);
	outSet.SetOutputPath(strGamePath.ToStdString());
	outSet.SetOutputFile(gameFile);
	outSet.SetGenWeights(genWeights);

	wxString ssFileName = strFileName;
	if (ssFileName.Find("SliderSets\\") == wxString::npos)
		ssFileName = ssFileName.Prepend("SliderSets\\");

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
		owner->UpdateProgress(prog += step, _("Adding reference shapes..."));
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

		owner->UpdateProgress(prog += step, _("Adding outfit shapes..."));
	}
	
	string osdFileName = baseFile.substr(0, baseFile.find_last_of('.')) + ".osd";

	if (activeSet.size() > 0) {
		// Copy the reference slider info and add the outfit data to them.
		int id;
		string targ;
		string targSlider;
		string targSliderData;
		string targDataName;

		prog = 10;
		step = 20 / activeSet.size();
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
					}
					else {
						targSliderData = activeSet[i].DataFileName(targSlider);
						outSet[i].AddDataFile(targ, targSlider, targSliderData, false);
					}
				}
			}
			owner->UpdateProgress(prog += step, _("Calculating slider data..."));
		}
	}

	string saveDataPath = "ShapeData\\" + strDataDir;
	SaveSliderData(saveDataPath + "\\" + osdFileName, copyRef);
	
	prog = 60;
	owner->UpdateProgress(prog, _("Creating slider set file..."));

	SliderSetFile ssf(ssFileName.ToStdString());
	if (ssf.fail()) {
		ssf.New(ssFileName.ToStdString());
		if (ssf.fail()) {
			errmsg = _("Failed to open or create slider set file: ") + ssFileName.ToStdString();
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

	owner->UpdateProgress(61, _("Saving slider set file..."));
	ssf.UpdateSet(outSet);
	if (!ssf.Save()) {
		errmsg = _("Failed to write to slider set file: ") + ssFileName.ToStdString();
		return errmsg;
	}

	owner->UpdateProgress(70, _("Saving NIF file..."));

	string saveFileName = saveDataPath + "\\" + baseFile;

	if (workNif.IsValid()) {
		NifFile clone(workNif);

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

		clone.SetShapeOrder(owner->GetShapeList());
		clone.GetHeader().SetExportInfo("Exported using Outfit Studio.");

		if (clone.Save(saveFileName)) {
			errmsg = _("Failed to write base .nif file: ") + saveFileName;
			return errmsg;
		}
	}

	owner->ShowPartition();
	owner->UpdateProgress(100, _("Finished"));
	return errmsg;
}

bool OutfitProject::SaveSliderData(const wxString& fileName, bool copyRef) {
	vector<string> shapes;
	GetShapes(shapes);
	
	if (activeSet.size() > 0) {
		string targ;
		string targSlider;

		DiffDataSets osdDiffs;
		map<string, map<string, string>> osdNames;
		
		// Copy the changed reference slider data and add the outfit data to them.
		for (int i = 0; i < activeSet.size(); i++) {
			if (copyRef && !baseShape.empty()) {
				targ = ShapeToTarget(baseShape);
				targSlider = activeSet[i].TargetDataName(targ);
				if (baseDiffData.GetDiffSet(targSlider) && baseDiffData.GetDiffSet(targSlider)->size() > 0) {
					if (activeSet[i].IsLocalData(targSlider)) {
						unordered_map<ushort, Vector3>* diff = baseDiffData.GetDiffSet(targSlider);
						osdDiffs.LoadSet(targSlider, targ, *diff);
						osdNames[fileName.ToStdString()][targSlider] = targ;
					}
				}
			}

			for (auto &s : shapes) {
				if (IsBaseShape(s))
					continue;

				targ = ShapeToTarget(s);
				targSlider = activeSet[i].TargetDataName(targ);
				if (targSlider.empty())
					targSlider = targ + activeSet[i].name;

				if (morpher.GetResultDiffSize(s, activeSet[i].name) > 0) {
					string shapeDataFolder = activeSet.ShapeToDataFolder(s);
					if (shapeDataFolder == activeSet.GetDefaultDataFolder() || activeSet[i].IsLocalData(targSlider)) {
						unordered_map<ushort, Vector3> diff;
						morpher.GetRawResultDiff(s, activeSet[i].name, diff);
						osdDiffs.LoadSet(targSlider, targ, diff);
						osdNames[fileName.ToStdString()][targSlider] = targ;
					}
				}
			}
		}

		if (!osdDiffs.SaveData(osdNames))
			return false;
	}

	return true;
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

void OutfitProject::ReplaceForbidden(string& str, const char& replacer) {
	const string forbiddenChars = "\\/:*?\"<>|";

	std::transform(str.begin(), str.end(), str.begin(), [&forbiddenChars, &replacer](char c) {
		return forbiddenChars.find(c) != string::npos ? replacer : c;
	});
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
	int sliderID = activeSet.CreateSlider(newName);
	activeSet[sliderID].bShow = true;

	if (!baseShape.empty()) {
		string target = ShapeToTarget(baseShape);
		string shapeSlider = target + newName;
		activeSet[sliderID].AddDataFile(target, shapeSlider, shapeSlider);
		activeSet.AddShapeTarget(baseShape, target);
		baseDiffData.AddEmptySet(shapeSlider, target);
	}
}

void OutfitProject::AddZapSlider(const string& newName, unordered_map<ushort, float>& verts, const string& shapeName) {
	unordered_map<ushort, Vector3> diffData;
	Vector3 moveVec(0.0f, 1.0f, 0.0f);
	for (auto &v : verts)
		diffData[v.first] = moveVec;

	string target = ShapeToTarget(shapeName);
	string shapeSlider = target + newName;

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
		string shapeSlider = target + newName;
		activeSet[sliderID].AddDataFile(target, shapeSlider, shapeSlider);
		baseDiffData.AddEmptySet(shapeSlider, target);
		GetLiveVerts(baseShape, verts);
		workNif.CalcShapeDiff(baseShape, &verts, diffData);
		for (auto &i : diffData)
			baseDiffData.SumDiff(shapeSlider, target, i.first, i.second);
	}
}

int OutfitProject::CreateNifShapeFromData(const string& shapeName, vector<Vector3>& v, vector<Triangle>& t, vector<Vector2>& uv, vector<Vector3>* norms) {
	string blankSkel;
	string defaultName = "New Outfit";

	if (owner->targetGame <= SKYRIM)
		blankSkel = "res\\SkeletonBlank_sk.nif";
	else if (owner->targetGame == FO4)
		blankSkel = "res\\SkeletonBlank_fo4.nif";
	else if (owner->targetGame == SKYRIMSE)
		blankSkel = "res\\SkeletonBlank_sse.nif";

	NifFile blank;
	blank.Load(blankSkel);
	if (!blank.IsValid()) {
		wxLogError("Could not load 'SkeletonBlank.nif' for importing data file.");
		wxMessageBox(_("Could not load 'SkeletonBlank.nif' for importing data file."), _("Import Data Error"), wxICON_ERROR, owner);
		return 2;
	}

	if (!workNif.IsValid())
		ImportNIF(blankSkel, true, defaultName);

	if (owner->targetGame <= SKYRIM) {
		NiTriShapeData* nifShapeData = new NiTriShapeData(workNif.GetHeader());
		nifShapeData->Create(&v, &t, &uv);
		if (norms) {
			nifShapeData->normals = (*norms);
			nifShapeData->SetNormals(true);
		}

		int shapeID = blank.GetHeader().AddBlock(nifShapeData, "NiTriShapeData");

		NiSkinData* nifSkinData = new NiSkinData(workNif.GetHeader());
		int skinID = blank.GetHeader().AddBlock(nifSkinData, "NiSkinData");

		NiSkinPartition* nifSkinPartition = new NiSkinPartition(workNif.GetHeader());
		int partID = blank.GetHeader().AddBlock(nifSkinPartition, "NiSkinPartition");

		BSDismemberSkinInstance* nifDismemberInst = new BSDismemberSkinInstance(workNif.GetHeader());
		int dismemberID = blank.GetHeader().AddBlock(nifDismemberInst, "BSDismemberSkinInstance");
		nifDismemberInst->SetDataRef(skinID);
		nifDismemberInst->SetSkinPartitionRef(partID);
		nifDismemberInst->SetSkeletonRootRef(0);

		BSShaderTextureSet* nifTexset = new BSShaderTextureSet(workNif.GetHeader());

		int shaderID;
		BSLightingShaderProperty* nifShader = nullptr;
		BSShaderPPLightingProperty* nifShaderPP = nullptr;
		switch (owner->targetGame) {
		case FO3:
		case FONV:
			nifShaderPP = new BSShaderPPLightingProperty(workNif.GetHeader());
			shaderID = blank.GetHeader().AddBlock(nifShaderPP, "BSShaderPPLightingProperty");
			nifShaderPP->textureSetRef = blank.GetHeader().AddBlock(nifTexset, "BSShaderTextureSet");
			break;
		case SKYRIM:
		default:
			nifShader = new BSLightingShaderProperty(workNif.GetHeader());
			shaderID = blank.GetHeader().AddBlock(nifShader, "BSLightingShaderProperty");
			nifShader->textureSetRef = blank.GetHeader().AddBlock(nifTexset, "BSShaderTextureSet");
		}

		NiTriShape* nifTriShape = new NiTriShape(workNif.GetHeader());
		blank.GetHeader().AddBlock(nifTriShape, "NiTriShape");
		if (owner->targetGame < SKYRIM) {
			nifTriShape->propertiesRef.push_back(shaderID);
			nifTriShape->numProperties++;
		}
		else
			nifTriShape->SetShaderPropertyRef(shaderID);

		nifTriShape->SetName(shapeName);
		nifTriShape->SetDataRef(shapeID);
		nifTriShape->SetSkinInstanceRef(dismemberID);

		blank.SetDefaultPartition(shapeName);
	}
	else if (owner->targetGame == FO4) {
		BSTriShape* triShapeBase;
		string wetShaderName = "template/OutfitTemplate_Wet.bgsm";
		BSSubIndexTriShape* nifBSTriShape = new BSSubIndexTriShape(workNif.GetHeader());
		nifBSTriShape->Create(&v, &t, &uv, norms);
		blank.GetHeader().AddBlock(nifBSTriShape, "BSSubIndexTriShape");

		BSSkinInstance* nifBSSkinInstance = new BSSkinInstance(workNif.GetHeader());
		int skinID = blank.GetHeader().AddBlock(nifBSSkinInstance, "BSSkin::Instance");
		nifBSSkinInstance->SetTargetRef(workNif.GetRootNodeID());

		BSSkinBoneData* nifBoneData = new BSSkinBoneData(workNif.GetHeader());
		int boneID = blank.GetHeader().AddBlock(nifBoneData, "BSSkin::BoneData");
		nifBSSkinInstance->SetDataRef(boneID);
		nifBSTriShape->SetSkinInstanceRef(skinID);
		triShapeBase = nifBSTriShape;

		BSShaderTextureSet* nifTexset = new BSShaderTextureSet(workNif.GetHeader());

		BSLightingShaderProperty* nifShader = new BSLightingShaderProperty(workNif.GetHeader());
		int shaderID = blank.GetHeader().AddBlock(nifShader, "BSLightingShaderProperty");
		nifShader->textureSetRef = blank.GetHeader().AddBlock(nifTexset, "BSShaderTextureSet");
		nifShader->SetWetMaterialName(wetShaderName);

		triShapeBase->SetName(shapeName);
		triShapeBase->SetShaderPropertyRef(shaderID);
	}
	else {
		BSTriShape* triShape = new BSTriShape(workNif.GetHeader());
		triShape->Create(&v, &t, &uv, norms);
		blank.GetHeader().AddBlock(triShape, "BSTriShape");

		NiSkinData* nifSkinData = new NiSkinData(workNif.GetHeader());
		int skinID = blank.GetHeader().AddBlock(nifSkinData, "NiSkinData");

		NiSkinPartition* nifSkinPartition = new NiSkinPartition(workNif.GetHeader());
		int partID = blank.GetHeader().AddBlock(nifSkinPartition, "NiSkinPartition");

		BSDismemberSkinInstance* nifDismemberInst = new BSDismemberSkinInstance(workNif.GetHeader());
		int dismemberID = blank.GetHeader().AddBlock(nifDismemberInst, "BSDismemberSkinInstance");
		nifDismemberInst->SetDataRef(skinID);
		nifDismemberInst->SetSkinPartitionRef(partID);
		nifDismemberInst->SetSkeletonRootRef(0);
		triShape->SetSkinInstanceRef(dismemberID);
		triShape->SetSkinned(true);

		BSShaderTextureSet* nifTexset = new BSShaderTextureSet(workNif.GetHeader());

		BSLightingShaderProperty* nifShader = new BSLightingShaderProperty(workNif.GetHeader());
		int shaderID = blank.GetHeader().AddBlock(nifShader, "BSLightingShaderProperty");
		nifShader->textureSetRef = blank.GetHeader().AddBlock(nifTexset, "BSShaderTextureSet");

		triShape->SetName(shapeName);
		triShape->SetShaderPropertyRef(shaderID);

		blank.SetDefaultPartition(shapeName);
		blank.UpdateSkinPartitions(shapeName);
	}

	workNif.CopyGeometry(shapeName, blank, shapeName);
	SetTextures(shapeName);

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

bool OutfitProject::SliderUV(int index) {
	if (!ValidSlider(index))
		return false;
	return activeSet[index].bUV;
}

wxArrayString OutfitProject::SliderZapToggles(int index) {
	wxArrayString toggles;
	if (ValidSlider(index))
		for (auto &toggle : activeSet[index].zapToggles)
			toggles.Add(toggle);

	return toggles;
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

void OutfitProject::SetSliderZap(int index, bool zap) {
	if (!ValidSlider(index))
		return;
	activeSet[index].bZap = zap;
}

void OutfitProject::SetSliderZapToggles(int index, const wxArrayString& toggles) {
	if (!ValidSlider(index))
		return;

	vector<string> zapToggles;
	for (auto &s : toggles)
		zapToggles.push_back(s.ToStdString());

	activeSet[index].zapToggles = zapToggles;
}

void OutfitProject::SetSliderInvert(int index, bool inv) {
	if (!ValidSlider(index))
		return;
	activeSet[index].bInvert = inv;
}

void OutfitProject::SetSliderUV(int index, bool uv) {
	if (!ValidSlider(index))
		return;
	activeSet[index].bUV = uv;
}

void OutfitProject::SetSliderHidden(int index, bool hidden) {
	if (!ValidSlider(index))
		return;
	activeSet[index].bHidden = hidden;
}

void OutfitProject::SetSliderDefault(int index, int val, bool isHi) {
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

void OutfitProject::MaskAffected(const string& sliderName, const string& shapeName) {
	mesh* m = owner->glView->GetMesh(shapeName);
	if (!m)
		return;

	m->ColorChannelFill(0, 0.0f);

	if (IsBaseShape(shapeName)) {
		vector<ushort> outIndices;
		string target = ShapeToTarget(shapeName);

		string sliderData = activeSet[sliderName].TargetDataName(target);
		baseDiffData.GetDiffIndices(sliderData, target, outIndices);

		for (auto &i : outIndices)
			m->vcolors[i].x = 1.0f;
	}
	else {
		unordered_map<ushort, Vector3> outDiff;
		morpher.GetRawResultDiff(shapeName, sliderName, outDiff);

		for (auto &i : outDiff)
			m->vcolors[i.first].x = 1.0f;
	}

	m->QueueUpdate(mesh::UpdateType::VertexColors);
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
		baseDiffData.SaveSet(sliderData, target, fileName);
	}
	else
		morpher.SaveResultDiff(target, sliderName, fileName);

	return 0;
}

int OutfitProject::SaveSliderOBJ(const string& sliderName, const string& shapeName, const string& fileName) {
	string target = ShapeToTarget(shapeName);
	vector<Triangle> tris;
	const vector<Vector3>* verts = workNif.GetRawVertsForShape(shapeName);
	workNif.GetTrisForShape(shapeName, &tris);
	const vector<Vector2>* uvs = workNif.GetUvsForShape(shapeName);

	vector<Vector3> outVerts = *verts;

	if (IsBaseShape(shapeName)) {
		string sliderData = activeSet[sliderName].TargetDataName(target);
		baseDiffData.ApplyDiff(sliderData, target, 1.0f, &outVerts);
	}
	else
		morpher.ApplyResultToVerts(sliderName, target, &outVerts);

	ObjFile obj;
	obj.SetScale(Vector3(0.1f, 0.1f, 0.1f));
	obj.AddGroup(shapeName, outVerts, tris, *uvs);
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

	ObjFile obj;
	obj.LoadForNif(fileName);

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
	vector<Vector2> objUVs;
	obj.CopyDataForIndex(index, &objVerts, nullptr, &objUVs);

	unordered_map<ushort, Vector3> diff;
	if (activeSet[sliderName].bUV) {
		if (workNif.CalcUVDiff(shapeName, &objUVs, diff))
			return false;
	}
	else {
		if (workNif.CalcShapeDiff(shapeName, &objVerts, diff, 10.0f))
			return false;
	}

	if (IsBaseShape(shapeName)) {
		string sliderData = activeSet[sliderName].TargetDataName(target);
		baseDiffData.LoadSet(sliderData, target, diff);
	}
	else
		morpher.SetResultDiff(target, sliderName, diff);

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

	FBXShape* shape = fbxw.GetShape(shapeName);

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

void OutfitProject::SetSliderFromDiff(const string& sliderName, const string& shapeName, unordered_map<ushort, Vector3>& diff) {
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

void OutfitProject::GetLiveVerts(const string& shapeName, vector<Vector3>& outVerts, vector<Vector2>* outUVs) {
	workNif.GetVertsForShape(shapeName, outVerts);
	if (outUVs)
		workNif.GetUvsForShape(shapeName, *outUVs);

	string target = ShapeToTarget(shapeName);
	if (IsBaseShape(shapeName)) {
		for (int i = 0; i < activeSet.size(); i++) {
			if (activeSet[i].bShow && activeSet[i].curValue != 0.0f) {
				string targetData = activeSet.ShapeToDataName(i, shapeName);
				if (targetData == "")
					continue;

				if (activeSet[i].bUV) {
					if (outUVs)
						baseDiffData.ApplyUVDiff(targetData, target, activeSet[i].curValue, outUVs);
				}
				else
					baseDiffData.ApplyDiff(targetData, target, activeSet[i].curValue, &outVerts);
			}
		}
	}
	else {
		for (int i = 0; i < activeSet.size(); i++) {
			if (activeSet[i].bShow && activeSet[i].curValue != 0.0f) {
				if (activeSet[i].bUV) {
					if (outUVs)
						morpher.ApplyResultToUVs(activeSet[i].name, target, outUVs, activeSet[i].curValue);
				}
				else
					morpher.ApplyResultToVerts(activeSet[i].name, target, &outVerts, activeSet[i].curValue);
			}
		}
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

vector<string> OutfitProject::GetShapeTextures(const string& shapeName) {
	if (shapeTextures.find(shapeName) != shapeTextures.end())
		return shapeTextures[shapeName];

	return vector<string>();
}

bool OutfitProject::GetShapeMaterialFile(const string& shapeName, MaterialFile& outMatFile) {
	if (shapeMaterialFiles.find(shapeName) != shapeMaterialFiles.end()) {
		outMatFile = shapeMaterialFiles[shapeName];
		return true;
	}

	return false;
}

void OutfitProject::SetTextures() {
	vector<string> shapes;
	GetShapes(shapes);
	for (auto &s : shapes)
		SetTextures(s);
}

void OutfitProject::SetTextures(const string& shapeName, const vector<string>& textureFiles) {
	if (shapeName.empty())
		return;

	if (textureFiles.empty()) {
		string texturesDir = appConfig["GameDataPath"];
		bool hasMat = false;
		wxString matFile;

		const byte MAX_TEXTURE_PATHS = 10;
		vector<string> texFiles(MAX_TEXTURE_PATHS);

		NiShader* shader = workNif.GetShader(shapeName);
		if (shader) {
			// Find material file
			if (shader->header->GetUserVersion() == 12 && shader->header->GetUserVersion2() >= 130) {
				matFile = shader->GetName();
				if (!matFile.IsEmpty())
					hasMat = true;
			}
		}

		MaterialFile mat(MaterialFile::BGSM);
		if (hasMat) {
			matFile = matFile.Lower();
			matFile.Replace("\\", "/");

			// Attempt to read loose material file
			mat = MaterialFile(texturesDir + matFile.ToStdString());

			if (mat.Failed()) {
				// Search for material file in archives
				wxMemoryBuffer data;
				for (FSArchiveFile *archive : FSManager::archiveList()) {
					if (archive) {
						if (archive->hasFile(matFile.ToStdString())) {
							wxMemoryBuffer outData;
							archive->fileContents(matFile.ToStdString(), outData);

							if (!outData.IsEmpty()) {
								data = move(outData);
								break;
							}
						}
					}
				}

				if (!data.IsEmpty()) {
					string content((char*)data.GetData(), data.GetDataLen());
					istringstream contentStream(content, istringstream::binary);

					mat = MaterialFile(contentStream);
				}
			}

			if (!mat.Failed()) {
				if (mat.signature == MaterialFile::BGSM) {
					texFiles[0] = mat.diffuseTexture.c_str();
					texFiles[1] = mat.normalTexture.c_str();
					texFiles[4] = mat.envmapTexture.c_str();
					texFiles[5] = mat.glowTexture.c_str();
					texFiles[7] = mat.smoothSpecTexture.c_str();
				}
				else if (mat.signature == MaterialFile::BGEM) {
					texFiles[0] = mat.baseTexture.c_str();
					texFiles[1] = mat.fxNormalTexture.c_str();
					texFiles[4] = mat.fxEnvmapTexture.c_str();
					texFiles[5] = mat.envmapMaskTexture.c_str();
				}

				shapeMaterialFiles[shapeName] = move(mat);
			}
			else {
				for (int i = 0; i < MAX_TEXTURE_PATHS; i++)
					workNif.GetTextureForShape(shapeName, texFiles[i], i);
			}
		}
		else {
			for (int i = 0; i < MAX_TEXTURE_PATHS; i++)
				workNif.GetTextureForShape(shapeName, texFiles[i], i);
		}

		for (int i = 0; i < MAX_TEXTURE_PATHS; i++) {
			if (!texFiles[i].empty()) {
				texFiles[i] = regex_replace(texFiles[i], regex("/+|\\\\+"), "\\");												// Replace multiple slashes or forward slashes with one backslash
				texFiles[i] = regex_replace(texFiles[i], regex("^(.*?)\\\\textures\\\\", regex_constants::icase), "");			// Remove everything before the first occurence of "\textures\"
				texFiles[i] = regex_replace(texFiles[i], regex("^\\\\+"), "");													// Remove all backslashes from the front
				texFiles[i] = regex_replace(texFiles[i], regex("^(?!^textures\\\\)", regex_constants::icase), "textures\\");	// If the path doesn't start with "textures\", add it to the front

				texFiles[i] = texturesDir + texFiles[i];
			}
		}

		shapeTextures[shapeName] = texFiles;
	}
	else
		shapeTextures[shapeName] = textureFiles;
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

void OutfitProject::ScaleShape(const string& shapeName, const Vector3& scale, unordered_map<ushort, float>* mask) {
	workNif.ScaleShape(shapeName, scale, mask);
}

void OutfitProject::RotateShape(const string& shapeName, const Vector3& angle, unordered_map<ushort, float>* mask) {
	workNif.RotateShape(shapeName, angle, mask);
}

void OutfitProject::CopyBoneWeights(const string& destShape, const float& proximityRadius, const int& maxResults, unordered_map<ushort, float>* mask, vector<string>* inBoneList) {
	if (baseShape.empty())
		return;

	vector<string> lboneList;
	vector<string>* boneList;

	owner->UpdateProgress(1, _("Gathering bones..."));

	if (!inBoneList) {
		for (auto &bn : workAnim.shapeBones[baseShape])
			lboneList.push_back(bn);

		boneList = &lboneList;
	}
	else
		boneList = inBoneList;

	if (boneList->size() <= 0) {
		owner->UpdateProgress(90);
		return;
	}

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

	owner->UpdateProgress(10, _("Initializing proximity data..."));

	InitConform();
	morpher.LinkRefDiffData(&dds);
	morpher.BuildProximityCache(destShape, proximityRadius);

	int step = 40 / boneList->size();
	int prog = 40;
	owner->UpdateProgress(prog);

	for (auto &boneName : *boneList) {
		string wtSet = boneName + "_WT_";
		morpher.GenerateResultDiff(destShape, wtSet, wtSet, maxResults);

		unordered_map<ushort, Vector3> diffResult;
		morpher.GetRawResultDiff(destShape, wtSet, diffResult);

		unordered_map<ushort, float> oldWeights;
		if (mask) {
			weights.clear();
			oldWeights.clear();

			workAnim.GetWeights(destShape, boneName, oldWeights);
		}

		for (auto &dr : diffResult) {
			if (mask)
				weights[dr.first] = dr.second.y * (1.0f - (*mask)[dr.first]);
			else
				weights[dr.first] = dr.second.y;
		}

		// Restore old weights from mask
		if (mask) {
			for (auto &w : oldWeights)
				if ((*mask)[w.first] > 0.0f)
					weights[w.first] = w.second;
		}

		if (diffResult.size() > 0) {
			AnimBone boneRef;
			AnimSkeleton::getInstance().GetBone(boneName, boneRef);
			if (workAnim.AddShapeBone(destShape, boneRef)) {
				if (owner->targetGame == FO4) {
					// Fallout 4 bone transforms are stored in a bonedata structure per shape versus the node transform in the skeleton data.
					SkinTransform xForm;
					workNif.GetShapeBoneTransform(baseShape, boneName, xForm);
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
		owner->UpdateProgress(prog += step, _("Copying bone weights..."));
	}

	morpher.UnlinkRefDiffData();
	owner->UpdateProgress(90);
}

void OutfitProject::TransferSelectedWeights(const string& destShape, unordered_map<ushort, float>* mask, vector<string>* inBoneList) {
	if (baseShape.empty())
		return;

	owner->UpdateProgress(10, _("Gathering bones..."));

	vector<string>* boneList;
	vector<string> allBoneList;
	if (!inBoneList) {
		for (auto &boneName : workAnim.shapeBones[baseShape])
			allBoneList.push_back(boneName);

		boneList = &allBoneList;
	}
	else
		boneList = inBoneList;

	if (boneList->size() <= 0) {
		owner->UpdateProgress(100, _("Finished"));
		return;
	}

	int step = 50 / boneList->size();
	int prog = 40;
	owner->UpdateProgress(prog, _("Transferring bone weights..."));

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
				workNif.GetShapeBoneTransform(baseShape, boneName, xForm);
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

	owner->UpdateProgress(100, _("Finished"));
}

bool OutfitProject::HasUnweighted() {
	vector<string> shapes;
	GetShapes(shapes);
	for (auto &s : shapes) {
		if (!workNif.IsShapeSkinned(s))
			continue;

		vector<Vector3> verts;
		workNif.GetVertsForShape(s, verts);

		unordered_map<int, int> influences;
		for (int i = 0; i < verts.size(); i++)
			influences.emplace(i, 0);

		unordered_map<ushort, float> boneWeights;
		if (workAnim.shapeBones.find(s) != workAnim.shapeBones.end()) {
			for (auto &b : workAnim.shapeBones[s]) {
				boneWeights.clear();
				workAnim.GetWeights(s, b, boneWeights);
				for (int i = 0; i < verts.size(); i++) {
					auto id = boneWeights.find(i);
					if (id != boneWeights.end())
						influences.at(i)++;
				}
			}
		}

		mesh* m = owner->glView->GetMesh(s);
		bool unweighted = false;
		for (auto &i : influences) {
			if (i.second == 0) {
				if (!unweighted)
					m->ColorChannelFill(0, 0.0f);
				m->vcolors[i.first].x = 1.0f;
				unweighted = true;
			}
		}

		m->QueueUpdate(mesh::UpdateType::VertexColors);

		if (unweighted)
			return true;
	}
	return false;
}

void OutfitProject::ApplyBoneScale(const string& bone, int sliderPos, bool clear) {
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

		for (auto &b : workAnim.shapeBones[s]) {
			if (b == bone) {
				vector<Vector3> boneRot;
				Vector3 boneTranslation;
				float boneScale;

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
				break;
			}
		}

		if (clear)
			owner->glView->UpdateMeshVertices(s, verts, true, true, false);
		else
			owner->glView->UpdateMeshVertices(s, verts, true, false, false);
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

				if (clear)
					owner->glView->UpdateMeshVertices(s, verts, true, true, false);
				else
					owner->glView->UpdateMeshVertices(s, verts, false, false, false);
			}
		}
	}

	boneScaleVerts.clear();
	boneScaleOffsets.clear();
}

void OutfitProject::AddBoneRef(const string& boneName) {
	AnimBone *boneRef = AnimSkeleton::getInstance().GetBonePtr(boneName);
	if (!boneRef)
		return;

	SkinTransform xForm;
	workAnim.GetBoneXForm(boneName, xForm);

	boneRef->skinRot.Set(xForm.rotation);
	boneRef->skinTrans = xForm.translation;
	boneRef->hasSkinXform = true;

	vector<string> shapes;
	GetShapes(shapes);
	for (auto &s : shapes)
		if (workAnim.AddShapeBone(s, *boneRef))
			workAnim.SetShapeBoneXForm(s, boneName, xForm);
}

void OutfitProject::AddCustomBoneRef(const string& boneName, const Vector3& translation) {
	AnimBone& customBone = AnimSkeleton::getInstance().AddBone(boneName, true);

	SkinTransform xForm;
	xForm.translation = translation;


	customBone.trans = xForm.translation;
	customBone.skinTrans = xForm.translation;
	customBone.skinRot.Set(xForm.rotation);
	customBone.scale = xForm.scale;
	customBone.hasSkinXform = true;

	vector<string> shapes;
	GetShapes(shapes);
	for (auto &s : shapes)
		if (workAnim.AddShapeBone(s, customBone))
			workAnim.SetShapeBoneXForm(s, boneName, xForm);
}

void OutfitProject::ClearWorkSliders() {
	morpher.ClearResultDiff();
}

void OutfitProject::ClearReference() {
	DeleteShape(baseShape);

	if (activeSet.size() > 0)
		activeSet.Clear();
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
	vector<string> shapes;
	GetShapes(shapes);

	for (auto &s : shapes) {
		string target = ShapeToTarget(s);
		string data = activeSet[sliderName].TargetDataName(target);

		if (IsBaseShape(s))
			baseDiffData.ClearSet(data);
		else
			morpher.ClearResultSet(data);
	}

	activeSet.DeleteSlider(sliderName);
}

int OutfitProject::LoadSkeletonReference(const string& skeletonFileName) {
	return AnimSkeleton::getInstance().LoadFromNif(skeletonFileName);
}

int OutfitProject::LoadReferenceTemplate(const string& sourceFile, const string& set, const string& shape, bool mergeSliders) {
	if (sourceFile.empty() || set.empty()) {
		wxLogError("Template source entries are invalid.");
		wxMessageBox(_("Template source entries are invalid."), _("Reference Error"), wxICON_ERROR, owner);
		return 1;
	}

	return LoadReference(sourceFile, set, mergeSliders, shape);
}

int OutfitProject::LoadReferenceNif(const string& fileName, const string& shapeName, bool mergeSliders) {
	if (mergeSliders)
		DeleteShape(baseShape);
	else
		ClearReference();

	NifFile refNif;
	int error = refNif.Load(fileName);
	if (error) {
		if (error == 2) {
			wxString errorText = wxString::Format(_("NIF version not supported!\n\nFile: %s\n%s"),
				refNif.GetFileName(), refNif.GetHeader().GetVersionInfo());

			wxLogError(errorText);
			wxMessageBox(errorText, _("Reference Error"), wxICON_ERROR, owner);
			return 3;
		}

		wxLogError("Could not load reference NIF file '%s'!", fileName);
		wxMessageBox(wxString::Format(_("Could not load reference NIF file '%s'!"), fileName), _("Reference Error"), wxICON_ERROR, owner);
		return 2;
	}

	CheckNIFTarget(refNif);

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
		// Copy only reference shape
		workNif.CopyGeometry(baseShape, refNif, baseShape);
		workAnim.LoadFromNif(&workNif, baseShape);
	}
	else {
		// Copy the full file
		workNif.CopyFrom(refNif);
		workAnim.LoadFromNif(&workNif);

		// Delete all except for reference
		GetShapes(shapes);
		for (auto &s : shapes)
			if (s != baseShape)
				DeleteShape(s);
	}

	activeSet.LoadSetDiffData(baseDiffData);
	AutoOffset(workNif);
	return 0;
}

int OutfitProject::LoadReference(const string& fileName, const string& setName, bool mergeSliders, const string& shapeName) {
	if (mergeSliders)
		DeleteShape(baseShape);
	else
		ClearReference();

	string oldTarget;
	SliderSetFile sset(fileName);

	if (sset.fail()) {
		wxLogError("Could not load slider set file '%s'!", fileName);
		wxMessageBox(wxString::Format(_("Could not load slider set file '%s'!"), fileName), _("Reference Error"), wxICON_ERROR, owner);
		return 1;
	}

	string dataFolder = activeSet.GetDefaultDataFolder();
	sset.GetSet(setName, activeSet);

	activeSet.SetBaseDataPath(Config["ShapeDataPath"]);
	string inMeshFile = activeSet.GetInputFileName();

	NifFile refNif;
	int error = refNif.Load(inMeshFile);
	if (error) {
		if (error == 2) {
			wxString errorText = wxString::Format(_("NIF version not supported!\n\nFile: %s\n%s"),
				refNif.GetFileName(), refNif.GetHeader().GetVersionInfo());

			wxLogError(errorText);
			wxMessageBox(errorText, _("Reference Error"), wxICON_ERROR, owner);
			ClearReference();
			return 5;
		}

		ClearReference();
		wxLogError("Could not load reference NIF file '%s'!", inMeshFile);
		wxMessageBox(wxString::Format(_("Could not load reference NIF file '%s'!"), inMeshFile), _("Reference Error"), wxICON_ERROR, owner);
		return 2;
	}

	CheckNIFTarget(refNif);

	vector<string> shapes;
	refNif.GetShapeList(shapes);
	if (shapes.empty()) {
		ClearReference();
		wxLogError("Reference NIF file '%s' does not contain any shapes.", refNif.GetFileName());
		wxMessageBox(wxString::Format(_("Reference NIF file '%s' does not contain any shapes."), refNif.GetFileName()), _("Reference Error"), wxICON_ERROR, owner);
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

	int newVertCount = refNif.GetVertCountForShape(shape);
	if (newVertCount == -1) {
		ClearReference();
		wxLogError("Shape '%s' not found in reference NIF file '%s'!", shape, refNif.GetFileName());
		wxMessageBox(wxString::Format(_("Shape '%s' not found in reference NIF file '%s'!"), shape, refNif.GetFileName()), _("Reference Error"), wxICON_ERROR, owner);
		return 4;
	}

	// Add cloth data block of NIF to the list
	vector<BSClothExtraData*> clothDataBlocks = refNif.GetChildren<BSClothExtraData>(refNif.GetHeader().GetBlock<NiNode>(0), true);
	for (auto &cloth : clothDataBlocks)
		clothData[inMeshFile] = *cloth;

	refNif.GetHeader().DeleteBlockByType("BSClothExtraData");

	if (workNif.IsValid()) {
		// Copy only reference shape
		workNif.CopyGeometry(shape, refNif, shape);
		workAnim.LoadFromNif(&workNif, shape);
	}
	else {
		// Copy the full file
		workNif.CopyFrom(refNif);
		workAnim.LoadFromNif(&workNif);

		// Delete all except for reference
		GetShapes(shapes);
		for (auto &s : shapes)
			if (s != shape)
				DeleteShape(s);
	}

	baseShape = shape;

	activeSet.LoadSetDiffData(baseDiffData);
	activeSet.SetReferencedData(baseShape);

	// Keep default data folder from current project if existing
	if (!dataFolder.empty())
		activeSet.SetDataFolder(dataFolder);

	AutoOffset(workNif);
	return 0;
}

int OutfitProject::OutfitFromSliderSet(const string& fileName, const string& sliderSetName) {
	owner->StartProgress(_("Loading slider set..."));
	SliderSetFile InSS(fileName);
	if (InSS.fail()) {
		owner->EndProgress();
		return 1;
	}

	owner->UpdateProgress(20, _("Retrieving sliders..."));
	if (InSS.GetSet(sliderSetName, activeSet)) {
		owner->EndProgress();
		return 3;
	}

	activeSet.SetBaseDataPath(Config["ShapeDataPath"]);
	string inputNif = activeSet.GetInputFileName();

	owner->UpdateProgress(30, _("Loading outfit shapes..."));
	if (ImportNIF(inputNif, true, sliderSetName)) {
		owner->EndProgress();
		return 4;
	}

	string newBaseShape;

	// First external target with skin shader becomes reference
	vector<string> refTargets;
	activeSet.GetReferencedTargets(refTargets);
	for (auto &target : refTargets) {
		string shape = activeSet.TargetToShape(target);
		if (workNif.IsShaderSkin(shape)) {
			newBaseShape = shape;
			break;
		}
	}

	// No external target found, first skin shaded shape becomes reference
	if (refTargets.empty()) {
		for (auto shape = activeSet.TargetShapesBegin(); shape != activeSet.TargetShapesEnd(); ++shape) {
			if (workNif.IsShaderSkin(shape->second)) {
				newBaseShape = shape->second;
				break;
			}
		}
	}

	// Prevent duplication if valid reference was found
	DeleteShape(newBaseShape);
	baseShape = newBaseShape;

	owner->UpdateProgress(90, _("Updating slider data..."));
	morpher.LoadResultDiffs(activeSet);

	wxString rest;
	mFileName = fileName;
	if (mFileName.EndsWith(".xml", &rest))
		mFileName = rest.Append(".osp");

	mOutfitName = sliderSetName;
	mDataDir = activeSet.GetDefaultDataFolder();
	mBaseFile = activeSet.GetInputFileName();
	mBaseFile = mBaseFile.AfterLast('\\');

	mGamePath = activeSet.GetOutputPath();
	mGameFile = activeSet.GetOutputFile();
	mCopyRef = true;
	mGenWeights = activeSet.GenWeights();

	owner->UpdateProgress(100, _("Finished"));
	owner->EndProgress();
	return 0;
}

void OutfitProject::AutoOffset(NifFile& nif) {
	vector<string> shapes;
	nif.GetShapeList(shapes);

	for (auto &s : shapes) {
		SkinTransform xFormSkin;
		if (!nif.GetShapeBoneTransform(s, 0xFFFFFFFF, xFormSkin))
			continue;

		Matrix4 matSkinInv = xFormSkin.ToMatrix().Inverse();

		vector<Vector3> verts;
		nif.GetVertsForShape(s, verts);

		for (auto &v : verts)
			v = matSkinInv * v;

		SkinTransform xForm;
		nif.SetShapeBoneTransform(s, 0xFFFFFFFF, xForm);
		nif.ClearShapeTransform(s);

		nif.SetVertsForShape(s, verts);
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
		if (SliderShow(i) && !SliderZap(i) && !SliderUV(i))
			morpher.GenerateResultDiff(shapeName, activeSet[i].name, activeSet[i].TargetDataName(refTarget));
}

void OutfitProject::DeleteVerts(const string& shapeName, const unordered_map<ushort, float>& mask) {
	vector<ushort> indices;
	indices.reserve(mask.size());

	for (auto &m : mask)
		indices.push_back(m.first);

	sort(indices.begin(), indices.end());
	indices.erase(unique(indices.begin(), indices.end()), indices.end());

	bool shapeDeleted = workNif.DeleteVertsForShape(shapeName, indices);
	if (!shapeDeleted) {
		workAnim.DeleteVertsForShape(shapeName, indices);

		string target = ShapeToTarget(shapeName);
		if (IsBaseShape(shapeName))
			baseDiffData.DeleteVerts(target, indices);
		else
			morpher.DeleteVerts(target, indices);
		
		activeSet.SetReferencedData(shapeName, true);
	}
	else
		DeleteShape(shapeName);
}

void OutfitProject::DuplicateShape(const string& sourceShape, const string& destShape) {
	workNif.CopyGeometry(destShape, workNif, sourceShape);
	workAnim.LoadFromNif(&workNif, destShape);
}

void OutfitProject::DeleteShape(const string& shapeName) {
	workAnim.ClearShape(shapeName);
	workNif.DeleteShape(shapeName);
	owner->glView->DeleteMesh(shapeName);

	if (IsBaseShape(shapeName)) {
		morpher.UnlinkRefDiffData();
		baseShape.clear();
	}
}

void OutfitProject::RenameShape(const string& shapeName, const string& newShapeName) {
	workNif.RenameShape(shapeName, newShapeName);
	workAnim.RenameShape(shapeName, newShapeName);

	if (!IsBaseShape(shapeName))
		morpher.RenameShape(shapeName, newShapeName);

	activeSet.RenameShape(shapeName, newShapeName);
	wxLogMessage("Renamed shape '%s' to '%s'.", shapeName, newShapeName);
}

void OutfitProject::UpdateNifNormals(NifFile* nif, const vector<mesh*>& shapeMeshes) {
	vector<Vector3> liveNorms;
	for (auto &m : shapeMeshes) {
		if (nif->IsShaderSkin(m->shapeName) && (owner->targetGame == SKYRIM || owner->targetGame == SKYRIMSE))
			continue;

		liveNorms.clear();
		for (int i = 0; i < m->nVerts; i++)
			liveNorms.emplace_back(move(Vector3(m->norms[i].x* -1, m->norms[i].z, m->norms[i].y)));

		nif->SetNormalsForShape(m->shapeName, liveNorms);
		nif->CalcTangentsForShape(m->shapeName);
	}
}

int OutfitProject::ImportNIF(const string& fileName, bool clear, const string& inOutfitName) {
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
		wxFileName file(fileName);
		mGameFile = file.GetName();
		mGamePath = file.GetPath();

		int pos = mGamePath.Lower().Find("meshes");
		if (pos != wxNOT_FOUND)
			mGamePath = mGamePath.Mid(pos);
		else
			mGamePath.Clear();

		if (owner->targetGame == SKYRIM || owner->targetGame == SKYRIMSE) {
			wxString fileRest;
			if (mGameFile.EndsWith("_0", &fileRest) || mGameFile.EndsWith("_1", &fileRest))
				mGameFile = fileRest;
		}
	}

	NifFile nif;
	int error = nif.Load(fileName);
	if (error) {
		if (error == 2) {
			wxString errorText = wxString::Format(_("NIF version not supported!\n\nFile: %s\n%s"),
				nif.GetFileName(), nif.GetHeader().GetVersionInfo());

			wxLogError(errorText);
			wxMessageBox(errorText, _("NIF Error"), wxICON_ERROR, owner);
			return 4;
		}

		wxLogError("Could not load NIF file '%s'!", fileName);
		wxMessageBox(wxString::Format(_("Could not load NIF file '%s'!"), fileName), _("NIF Error"), wxICON_ERROR, owner);
		return 1;
	}

	CheckNIFTarget(nif);

	nif.SetNodeName(0, "Scene Root");

	vector<string> nifShapes;
	nif.GetShapeList(nifShapes);
	for (auto &s : nifShapes)
		nif.RenameDuplicateShape(s);

	if (!baseShape.empty())
		nif.RenameShape(baseShape, baseShape + "_outfit");

	vector<string> shapes;
	GetShapes(shapes);

	nif.GetShapeList(nifShapes);
	for (auto &s : nifShapes) {
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
	vector<BSClothExtraData*> clothDataBlocks = nif.GetChildren<BSClothExtraData>(nif.GetHeader().GetBlock<NiNode>(0), true);
	for (auto &cloth : clothDataBlocks)
		clothData[fileName] = *cloth;

	nif.GetHeader().DeleteBlockByType("BSClothExtraData");

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

int OutfitProject::ExportNIF(const string& fileName, const vector<mesh*>& modMeshes, bool writeNormals, bool withRef) {
	NifFile clone(workNif);

	ChooseClothData(clone);

	vector<Vector3> liveVerts;
	vector<Vector3> liveNorms;
	for (auto &m : modMeshes) {
		liveVerts.clear();
		liveNorms.clear();
		for (int i = 0; i < m->nVerts; i++) {
			liveVerts.emplace_back(move(Vector3(m->verts[i].x * -10, m->verts[i].z * 10, m->verts[i].y * 10)));
			liveNorms.emplace_back(move(Vector3(m->norms[i].x * -1, m->norms[i].z, m->norms[i].y)));
		}
		clone.SetVertsForShape(m->shapeName, liveVerts);

		if (writeNormals) {
			if (clone.IsShaderSkin(m->shapeName) && (owner->targetGame == SKYRIM || owner->targetGame == SKYRIMSE))
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

	clone.SetShapeOrder(owner->GetShapeList());
	clone.GetHeader().SetExportInfo("Exported using Outfit Studio.");
	return clone.Save(fileName);
}


void OutfitProject::ChooseClothData(NifFile& nif) {
	if (!clothData.empty()) {
		wxArrayString clothFileNames;
		for (auto &cloth : clothData)
			clothFileNames.Add(cloth.first);

		wxMultiChoiceDialog clothDataChoice(owner, _("There was cloth physics data loaded at some point (BSClothExtraData). Please choose all the origins to use in the output."), _("Choose cloth data"), clothFileNames);
		if (clothDataChoice.ShowModal() == wxID_CANCEL)
			return;

		wxArrayInt sel = clothDataChoice.GetSelections();
		for (int i = 0; i < sel.Count(); i++) {
			string selString = clothFileNames[sel[i]].ToStdString();
			if (!selString.empty()) {
				auto clothBlock = new BSClothExtraData(clothData[selString]);
				int id = nif.GetHeader().AddBlock(clothBlock, "BSClothExtraData");
				if (id != 0xFFFFFFFF) {
					NiNode* root = nif.GetHeader().GetBlock<NiNode>(0);
					if (root)
						root->AddExtraDataRef(id);
				}
			}
		}
	}
}

int OutfitProject::ExportShapeNIF(const string& fileName, const vector<string>& exportShapes) {
	if (exportShapes.empty())
		return 1;

	if (!workNif.IsValid())
		return 2;

	NifFile clone(workNif);
	ChooseClothData(clone);

	vector<string> shapes;
	clone.GetShapeList(shapes);

	for (auto &s : shapes)
		if (find(exportShapes.begin(), exportShapes.end(), s) == exportShapes.end())
			clone.DeleteShape(s);

	clone.GetShapeList(shapes);
	for (auto &s : shapes)
		clone.UpdateSkinPartitions(s);

	clone.GetHeader().SetExportInfo("Exported using Outfit Studio.");
	return clone.Save(fileName);
}

int OutfitProject::ImportOBJ(const string& fileName, const string& shapeName, const string& mergeShape) {
	ObjFile obj;
	obj.SetScale(Vector3(10.0f, 10.0f, 10.0f));

	if (!shapeName.empty())
		outfitName = shapeName;
	else if (outfitName.empty())
		outfitName = "New Outfit";

	if (obj.LoadForNif(fileName)) {
		wxLogError("Could not load OBJ file '%s'!", fileName);
		wxMessageBox(wxString::Format(_("Could not load OBJ file '%s'!"), fileName), _("OBJ Error"), wxICON_ERROR, owner);
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
			wxMessageBox(wxString::Format(_("Could not copy data from OBJ file '%s'!"), fileName), _("OBJ Error"), wxICON_ERROR, owner);
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
				int ret = wxMessageBox(_("The vertex count of the selected .obj file matches the currently selected outfit shape.  Do you wish to update the current shape?  (click No to create a new shape)"), _("Merge or New"), wxYES_NO | wxICON_QUESTION, owner);
				if (ret == wxYES) {
					ret = wxMessageBox(_("Update Vertex Positions?"), _("Vertex Position Update"), wxYES_NO | wxICON_QUESTION, owner);
					if (ret == wxYES)
						workNif.SetVertsForShape(mergeShape, v);

					ret = wxMessageBox(_("Update Texture Coordinates?"), _("UV Update"), wxYES_NO | wxICON_QUESTION, owner);
					if (ret == wxYES)
						workNif.SetUvsForShape(mergeShape, uv);

					return 101;
				}
			}
			useShapeName = wxGetTextFromUser(_("Please specify a name for the new shape"), _("New Shape Name"), useShapeName, owner);
			if (useShapeName == "")
				return 100;
		}

		CreateNifShapeFromData(useShapeName, v, t, uv);
	}

	return 0;
}

int OutfitProject::ExportOBJ(const string& fileName, const vector<string>& shapes, Vector3 scale, Vector3 offset) {
	ObjFile obj;
	obj.SetScale(scale);
	obj.SetOffset(offset);

	for (auto &s : shapes) {
		vector<Triangle> tris;
		if (!workNif.GetTrisForShape(s, &tris))
			return 1;

		const vector<Vector3>* verts = workNif.GetRawVertsForShape(s);
		const vector<Vector2>* uvs = workNif.GetUvsForShape(s);

		obj.AddGroup(s, *verts, tris, *uvs);
	}

	if (obj.Save(fileName))
		return 2;

	return 0;
}

int OutfitProject::ImportFBX(const string& fileName, const string& shapeName, const string& mergeShape) {
	FBXWrangler fbxw;
	string nonRefBones;

	FBXImportDialog import(owner);
	if (import.ShowModal() != wxID_OK)
		return 1;

	bool result = fbxw.ImportScene(fileName, import.GetOptions());
	if (!result)
		return 2;

	if (!shapeName.empty())
		outfitName = shapeName;
	else if (outfitName.empty())
		outfitName = "New Outfit";

	vector<string>shapes;
	fbxw.GetShapeNames(shapes);
	for (auto &s : shapes) {
		FBXShape* shape = fbxw.GetShape(s);
		string useShapeName = s;

		if (!mergeShape.empty()) {
			int vertCount = workNif.GetVertCountForShape(mergeShape);
			if (vertCount == shape->verts.size()) {
				int ret = wxMessageBox(_("The vertex count of the selected .fbx file matches the currently selected outfit shape.  Do you wish to update the current shape?  (click No to create a new shape)"), _("Merge or New"), wxYES_NO | wxICON_QUESTION, owner);
				if (ret == wxYES) {
					ret = wxMessageBox(_("Update Vertex Positions?"), _("Vertex Position Update"), wxYES_NO | wxICON_QUESTION, owner);
					if (ret == wxYES)
						workNif.SetVertsForShape(mergeShape, shape->verts);

					ret = wxMessageBox(_("Update Texture Coordinates?"), _("UV Update"), wxYES_NO | wxICON_QUESTION, owner);
					if (ret == wxYES)
						workNif.SetUvsForShape(mergeShape, shape->uvs);

					ret = wxMessageBox(_("Update Animation Weighting?"), _("Animation Weight Update"), wxYES_NO | wxICON_QUESTION, owner);
					if (ret == wxYES)
						for (auto &bn : shape->boneNames)
							workAnim.SetWeights(mergeShape, bn, shape->boneSkin[bn].GetWeights());

					return 101;
				}
			}

			useShapeName = wxGetTextFromUser(_("Please specify a name for the new shape"), _("New Shape Name"), useShapeName, owner);
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
					nonRefBones += bn + "\n";

				AnimSkeleton::getInstance().RefBone(bn);
			}

			workAnim.shapeBones[useShapeName].push_back(bn);
			workAnim.shapeSkinning[useShapeName].boneNames[bn] = slot;
			workAnim.SetWeights(useShapeName, bn, shape->boneSkin[bn].GetWeights());
			boneIndices.push_back(slot++);
		}

		workNif.SetShapeBoneIDList(useShapeName, boneIndices);

		if (!nonRefBones.empty())
			wxLogMessage("Bones in shape '%s' not found in reference skeleton:\n%s", useShapeName, nonRefBones);
	}

	return 0;
}

int OutfitProject::ExportFBX(const string& fileName, const vector<string>& shapes) {
	FBXWrangler fbxw;
	fbxw.AddSkeleton(&AnimSkeleton::getInstance().refSkeletonNif);

	for (auto &s : shapes) {
		fbxw.AddNif(&workNif, s);
		fbxw.AddSkinning(&workAnim, s);
	}

	return fbxw.ExportScene(fileName);
}


void OutfitProject::CheckNIFTarget(NifFile& nif) {
	bool match = false;

	switch (owner->targetGame) {
	case FO3:
	case FONV:
		match = (nif.GetHeader().GetUserVersion2() == 34);
		break;
	case SKYRIM:
		match = (nif.GetHeader().GetUserVersion2() == 83);
		break;
	case FO4:
		match = (nif.GetHeader().GetUserVersion2() == 130);
		break;
	case SKYRIMSE:
		match = (nif.GetHeader().GetUserVersion2() == 100);
		break;
	}

	if (!match) {
		if (owner->targetGame == SKYRIMSE && nif.GetHeader().GetUserVersion2() == 83) {
			if (!Config.Exists("OptimizeForSSE")) {
				int res = wxMessageBox(_("Would you like Skyrim NIFs to be optimized for SSE during this session?"), _("Target Game"), wxYES_NO | wxICON_INFORMATION, owner);
				if (res == wxYES)
					Config.SetDefaultValue("OptimizeForSSE", "true");
				else
					Config.SetDefaultValue("OptimizeForSSE", "false");
			}

			if (Config["OptimizeForSSE"] == "true")
				nif.OptimizeForSSE();
		}
		else {
			wxLogWarning("Version of NIF file '%s' doesn't match current target game. To use the meshes for the target game, export to OBJ/FBX and reload them again.", nif.GetFileName());
			wxMessageBox(wxString::Format(_("Version of NIF file '%s' doesn't match current target game. To use the meshes for the target game, export to OBJ/FBX and reload them again."),
				nif.GetFileName()), _("Version"), wxICON_WARNING, owner);
		}
	}
}
