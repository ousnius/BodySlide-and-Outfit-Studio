/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "OutfitProject.h"
#include "../components/WeightNorm.h"
#include "../files/FBXWrangler.h"
#include "../files/ObjFile.h"
#include "../files/TriFile.h"
#include "../program/FBXImportDialog.h"
#include "../program/ObjImportDialog.h"
#include "../utils/PlatformUtil.h"
#include "NifUtil.hpp"

#include "../FSEngine/FSEngine.h"
#include "../FSEngine/FSManager.h"

#include <regex>
#include <sstream>

extern ConfigurationManager Config;

using namespace nifly;

OutfitProject::OutfitProject(OutfitStudioFrame* inOwner) {
	owner = inOwner;
	workAnim.SetRefNif(&workNif);

	std::string defSkelFile = Config["Anim/DefaultSkeletonReference"];
	if (wxFileName(wxString::FromUTF8(defSkelFile)).IsRelative())
		LoadSkeletonReference(Config["AppDir"] + PathSepStr + defSkelFile);
	else
		LoadSkeletonReference(defSkelFile);

	auto targetGame = (TargetGame)Config.GetIntValue("TargetGame");
	if (targetGame == SKYRIM || targetGame == SKYRIMSE || targetGame == SKYRIMVR)
		mGenWeights = true;
}

OutfitProject::~OutfitProject() {}

std::string OutfitProject::Save(const wxFileName& sliderSetFile,
								const wxString& strOutfitName,
								const wxString& strDataDir,
								const wxString& strBaseFile,
								const wxString& strGamePath,
								const wxString& strGameFile,
								bool genWeights,
								bool copyRef) {
	owner->UpdateProgress(1, _("Checking destination..."));
	std::string errmsg = "";
	std::string outfit{strOutfitName.ToUTF8()};
	std::string baseFile{strBaseFile.ToUTF8()};
	std::string gameFile{strGameFile.ToUTF8()};

	ReplaceForbidden(outfit);
	ReplaceForbidden(baseFile);
	ReplaceForbidden(gameFile);

	SliderSet outSet;
	outSet.SetName(outfit);
	outSet.SetDataFolder(strDataDir.ToUTF8().data());
	outSet.SetInputFile(baseFile);
	outSet.SetOutputPath(strGamePath.ToUTF8().data());
	outSet.SetOutputFile(gameFile);
	outSet.SetGenWeights(genWeights);

	const wxString sliderSetsStr = "SliderSets";

	wxFileName ssFileName(sliderSetFile);
	int sliderSetsStrIndex = ssFileName.GetDirs().Index(sliderSetsStr);
	if (sliderSetsStrIndex == wxNOT_FOUND) {
		// Make path relative to "SliderSets\", only use file name
		ssFileName = wxFileName(sliderSetsStr + PathSepStr + sliderSetFile.GetFullName());
	}

	if (ssFileName.IsRelative())
		ssFileName.MakeAbsolute(wxString::FromUTF8(GetProjectPath()));

	mFileName = ssFileName.GetFullPath();
	mOutfitName = wxString::FromUTF8(outfit);
	mDataDir = strDataDir;
	mBaseFile = wxString::FromUTF8(baseFile);
	mGamePath = strGamePath;
	mGameFile = wxString::FromUTF8(gameFile);
	mCopyRef = copyRef;
	mGenWeights = genWeights;

	auto shapes = workNif.GetShapes();

	wxString folder(wxString::Format("%s/%s/%s", wxString::FromUTF8(GetProjectPath()), "ShapeData", strDataDir));
	wxFileName::Mkdir(folder, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);

	int prog = 5;
	int step = 10 / shapes.size();
	owner->UpdateProgress(prog);

	if (copyRef && baseShape) {
		// Add all the reference shapes to the target list.
		std::string baseShapeName = baseShape->name.get();
		outSet.AddShapeTarget(baseShapeName, ShapeToTarget(baseShapeName));
		outSet.AddTargetDataFolder(ShapeToTarget(baseShapeName), activeSet.ShapeToDataFolder(baseShapeName));
		outSet.SetSmoothSeamNormals(baseShapeName, activeSet.GetSmoothSeamNormals(baseShapeName));
		outSet.SetLockNormals(baseShapeName, activeSet.GetLockNormals(baseShapeName));
		owner->UpdateProgress(prog += step, _("Adding reference shapes..."));
	}

	// Add all the outfit shapes to the target list.
	for (auto& s : shapes) {
		if (IsBaseShape(s))
			continue;

		std::string shapeName = s->name.get();
		outSet.AddShapeTarget(shapeName, ShapeToTarget(shapeName));

		// Reference only if not local folder
		std::string shapeDataFolder = activeSet.ShapeToDataFolder(shapeName);
		if (shapeDataFolder != activeSet.GetDefaultDataFolder())
			outSet.AddTargetDataFolder(ShapeToTarget(shapeName), activeSet.ShapeToDataFolder(shapeName));

		outSet.SetSmoothSeamNormals(shapeName, activeSet.GetSmoothSeamNormals(shapeName));
		outSet.SetLockNormals(shapeName, activeSet.GetLockNormals(shapeName));
		owner->UpdateProgress(prog += step, _("Adding outfit shapes..."));
	}

	std::string osdFileName = baseFile.substr(0, baseFile.find_last_of('.')) + ".osd";

	if (activeSet.size() > 0) {
		// Copy the reference slider info and add the outfit data to them.
		std::string targ;
		std::string targSlider;
		std::string targSliderData;

		prog = 10;
		step = 20 / activeSet.size();
		owner->UpdateProgress(prog);

		for (size_t i = 0; i < activeSet.size(); i++) {
			size_t id = outSet.CopySlider(&activeSet[i]);
			outSet[id].Clear();

			if (copyRef && baseShape) {
				std::string baseShapeName = baseShape->name.get();
				targ = ShapeToTarget(baseShapeName);
				targSlider = activeSet[i].TargetDataName(targ);
				if (baseDiffData.GetDiffSet(targSlider) && baseDiffData.GetDiffSet(targSlider)->size() > 0) {
					if (activeSet[i].IsLocalData(targSlider)) {
						targSliderData = osdFileName + PathSepStr + targSlider;
						outSet[id].AddDataFile(targ, targSlider, targSliderData);
					}
					else {
						targSliderData = activeSet[i].DataFileName(targSlider);
						outSet[id].AddDataFile(targ, targSlider, targSliderData, false);
					}
				}
			}

			for (auto& s : shapes) {
				if (IsBaseShape(s))
					continue;

				std::string shapeName = s->name.get();
				targ = ShapeToTarget(shapeName);
				targSlider = activeSet[i].TargetDataName(targ);
				if (targSlider.empty())
					targSlider = targ + outSet[i].name;

				if (morpher.GetResultDiffSize(shapeName, activeSet[i].name) > 0) {
					std::string shapeDataFolder = activeSet.ShapeToDataFolder(shapeName);
					if (shapeDataFolder == activeSet.GetDefaultDataFolder() || activeSet[i].IsLocalData(targSlider)) {
						targSliderData = osdFileName + PathSepStr + targSlider;
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

	std::string saveDataPath = GetProjectPath() + PathSepStr + "ShapeData" + PathSepStr + mDataDir.ToUTF8().data();
	SaveSliderData(saveDataPath + PathSepStr + osdFileName, copyRef);

	prog = 60;
	owner->UpdateProgress(prog, _("Creating slider set file..."));

	std::string ssUFileName{mFileName.ToUTF8()};
	SliderSetFile ssf(ssUFileName);
	if (ssf.fail()) {
		ssf.New(ssUFileName);
		if (ssf.fail()) {
			errmsg = _("Failed to open or create slider set file: ") + ssUFileName;
			return errmsg;
		}
	}

	ssFileName.Mkdir(wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);

	owner->UpdateProgress(61, _("Saving slider set file..."));
	ssf.UpdateSet(outSet);
	if (!ssf.Save()) {
		errmsg = _("Failed to write to slider set file: ") + ssUFileName;
		return errmsg;
	}

	owner->UpdateProgress(70, _("Saving NIF file..."));

	std::string saveFileName = saveDataPath + PathSepStr + baseFile;

	if (workNif.IsValid()) {
		workAnim.CleanupBones();
		owner->UpdateAnimationGUI();

		NifFile clone(workNif);
		ChooseClothData(clone);

		if (!copyRef && baseShape) {
			std::string baseShapeName = baseShape->name.get();
			auto shape = clone.FindBlockByName<NiShape>(baseShapeName);
			clone.DeleteShape(shape);
			workAnim.WriteToNif(&clone, baseShapeName);
		}
		else
			workAnim.WriteToNif(&clone);

		for (auto& s : clone.GetShapes())
			clone.UpdateSkinPartitions(s);

		clone.SetShapeOrder(owner->GetShapeList());
		clone.GetHeader().SetExportInfo("Exported using Outfit Studio.");

		std::fstream file;
		PlatformUtil::OpenFileStream(file, saveFileName, std::ios::out | std::ios::binary);

		if (clone.Save(file)) {
			errmsg = _("Failed to write base .nif file: ") + saveFileName;
			return errmsg;
		}
	}

	owner->ShowPartition();
	owner->UpdateProgress(100, _("Finished"));
	return errmsg;
}

bool OutfitProject::SaveSliderData(const std::string& fileName, bool copyRef) {
	auto shapes = workNif.GetShapes();

	if (activeSet.size() > 0) {
		std::string targ;
		std::string targSlider;

		DiffDataSets osdDiffs;
		std::map<std::string, std::map<std::string, std::string>> osdNames;

		// Copy the changed reference slider data and add the outfit data to them.
		for (size_t i = 0; i < activeSet.size(); i++) {
			if (copyRef && baseShape) {
				std::string baseShapeName = baseShape->name.get();
				targ = ShapeToTarget(baseShapeName);
				targSlider = activeSet[i].TargetDataName(targ);
				if (baseDiffData.GetDiffSet(targSlider) && baseDiffData.GetDiffSet(targSlider)->size() > 0) {
					if (activeSet[i].IsLocalData(targSlider)) {
						std::unordered_map<uint16_t, Vector3>* diff = baseDiffData.GetDiffSet(targSlider);
						osdDiffs.LoadSet(targSlider, targ, *diff);
						osdNames[fileName][targSlider] = targ;
					}
				}
			}

			for (auto& s : shapes) {
				if (IsBaseShape(s))
					continue;

				std::string shapeName = s->name.get();
				targ = ShapeToTarget(shapeName);
				targSlider = activeSet[i].TargetDataName(targ);
				if (targSlider.empty())
					targSlider = targ + activeSet[i].name;

				if (morpher.GetResultDiffSize(shapeName, activeSet[i].name) > 0) {
					std::string shapeDataFolder = activeSet.ShapeToDataFolder(shapeName);
					if (shapeDataFolder == activeSet.GetDefaultDataFolder() || activeSet[i].IsLocalData(targSlider)) {
						std::unordered_map<uint16_t, Vector3> diff;
						morpher.GetRawResultDiff(shapeName, activeSet[i].name, diff);
						osdDiffs.LoadSet(targSlider, targ, diff);
						osdNames[fileName][targSlider] = targ;
					}
				}
			}
		}

		if (!osdDiffs.SaveData(osdNames))
			return false;
	}

	return true;
}

void OutfitProject::SetBaseShape(NiShape* shape, const bool moveData) {
	if (moveData) {
		if (baseShape != shape) {
			// Copy data from base shape to regular shape
			if (baseShape) {
				std::string shapeName = baseShape->name.get();
				std::string srcTarget = ShapeToTarget(shapeName);

				for (size_t i = 0; i < activeSet.size(); i++) {
					std::string srcTargetData = activeSet[i].TargetDataName(srcTarget);

					auto diff = baseDiffData.GetDiffSet(srcTargetData);
					if (diff)
						morpher.SetResultDiff(shapeName, activeSet[i].name, *diff);

					activeSet[i].RenameTarget(srcTarget, shapeName);
					baseDiffData.ClearSet(srcTargetData);
				}

				activeSet.AddShapeTarget(shapeName, shapeName);
				activeSet.Retarget(srcTarget, shapeName);
			}

			// Copy data from regular shape to base shape
			if (shape) {
				std::string shapeName = shape->name.get();
				std::string target = ShapeToTarget(shapeName);

				for (size_t i = 0; i < activeSet.size(); i++) {
					std::string sliderName = activeSet[i].name;
					std::string targetData = activeSet[i].TargetDataName(target);
					if (targetData.empty()) {
						targetData = target + sliderName;
						activeSet[i].AddDataFile(target, target + sliderName, target + sliderName);
					}
					else
						activeSet[i].SetLocalData(targetData);

					std::unordered_map<uint16_t, Vector3> diff;
					morpher.GetRawResultDiff(shapeName, sliderName, diff);
					morpher.ClearResultSet(targetData);

					baseDiffData.LoadSet(targetData, shapeName, diff);
				}

				activeSet.AddShapeTarget(shapeName, shapeName);
			}
		}
	}

	baseShape = shape;
}


std::string OutfitProject::SliderSetName() {
	return activeSet.GetName();
}

std::string OutfitProject::SliderSetFileName() {
	return activeSet.GetInputFileName();
}

std::string OutfitProject::OutfitName() {
	return outfitName;
}

void OutfitProject::ReplaceForbidden(std::string& str, const char& replacer) {
	const std::string forbiddenChars = "\\/:*?\"<>|";

	std::transform(str.begin(), str.end(), str.begin(), [&forbiddenChars, &replacer](char c) { return forbiddenChars.find(c) != std::string::npos ? replacer : c; });
}

bool OutfitProject::ValidSlider(const size_t index) {
	if (index < activeSet.size())
		return true;

	return false;
}

bool OutfitProject::ValidSlider(const std::string& sliderName) {
	return activeSet.SliderExists(sliderName);
}

bool OutfitProject::AllSlidersZero() {
	for (size_t i = 0; i < activeSet.size(); i++)
		if (activeSet[i].curValue != 0.0f)
			return false;
	return true;
}

size_t OutfitProject::SliderCount() {
	return activeSet.size();
}

void OutfitProject::GetSliderList(std::vector<std::string>& sliderNames) {
	for (size_t i = 0; i < activeSet.size(); i++)
		sliderNames.push_back(activeSet[i].name);
}

std::string OutfitProject::GetSliderName(const size_t index) {
	if (!ValidSlider(index))
		return std::string();

	return activeSet[index].name;
}

void OutfitProject::AddEmptySlider(const std::string& newName) {
	size_t sliderID = activeSet.CreateSlider(newName);
	activeSet[sliderID].bShow = true;

	if (baseShape) {
		std::string baseShapeName = baseShape->name.get();
		std::string target = ShapeToTarget(baseShapeName);
		std::string shapeSlider = target + newName;
		activeSet[sliderID].AddDataFile(target, shapeSlider, shapeSlider);
		activeSet.AddShapeTarget(baseShapeName, target);
		baseDiffData.AddEmptySet(shapeSlider, target);
	}
}

void OutfitProject::AddZapSlider(const std::string& newName, std::unordered_map<uint16_t, float>& verts, NiShape* shape) {
	std::unordered_map<uint16_t, Vector3> diffData;
	Vector3 moveVec(0.0f, 1.0f, 0.0f);
	for (auto& v : verts)
		diffData[v.first] = moveVec;

	std::string target = ShapeToTarget(shape->name.get());
	std::string shapeSlider = target + newName;

	size_t sliderID = 0;
	if (!SliderIndexFromName(newName, sliderID))
		sliderID = activeSet.CreateSlider(newName);

	activeSet[sliderID].bZap = true;
	activeSet[sliderID].defBigValue = 0.0f;
	activeSet[sliderID].defSmallValue = 0.0f;

	if (IsBaseShape(shape)) {
		activeSet[sliderID].AddDataFile(target, shapeSlider, shapeSlider);
		activeSet.AddShapeTarget(shape->name.get(), target);
		baseDiffData.AddEmptySet(shapeSlider, target);
		for (auto& i : diffData)
			baseDiffData.SumDiff(shapeSlider, target, i.first, i.second);
	}
	else
		morpher.SetResultDiff(shape->name.get(), newName, diffData);
}

void OutfitProject::AddCombinedSlider(const std::string& newName) {
	std::vector<Vector3> verts;
	std::unordered_map<uint16_t, Vector3> diffData;

	for (auto& s : workNif.GetShapes()) {
		if (IsBaseShape(s))
			continue;

		diffData.clear();
		GetLiveVerts(s, verts);
		workNif.CalcShapeDiff(s, &verts, diffData);
		morpher.SetResultDiff(s->name.get(), newName, diffData);
	}

	size_t sliderID = activeSet.CreateSlider(newName);
	if (baseShape) {
		std::string baseShapeName = baseShape->name.get();
		std::string target = ShapeToTarget(baseShapeName);
		std::string shapeSlider = target + newName;
		activeSet[sliderID].AddDataFile(target, shapeSlider, shapeSlider);
		baseDiffData.AddEmptySet(shapeSlider, target);

		GetLiveVerts(baseShape, verts);
		workNif.CalcShapeDiff(baseShape, &verts, diffData);

		for (auto& i : diffData)
			baseDiffData.SumDiff(shapeSlider, target, i.first, i.second);
	}
}

NiShape* OutfitProject::CreateNifShapeFromData(
	const std::string& shapeName, const std::vector<Vector3>* v, const std::vector<Triangle>* t, const std::vector<Vector2>* uv, const std::vector<Vector3>* norms) {
	auto targetGame = (TargetGame)Config.GetIntValue("TargetGame");

	if (!workNif.IsValid()) {
		NiVersion version;

		switch (targetGame) {
			case OB: version = NiVersion::getOB(); break;
			case FO3:
			case FONV: version = NiVersion::getFO3(); break;
			case SKYRIM: version = NiVersion::getSK(); break;
			case FO4:
			case FO4VR: version = NiVersion::getFO4(); break;
			case SKYRIMSE:
			case SKYRIMVR: version = NiVersion::getSSE(); break;
			case FO76: version = NiVersion::getFO76(); break;
		}

		workNif.Create(version);
	}

	auto shapeResult = workNif.CreateShapeFromData(shapeName, v, t, uv, norms);
	if (shapeResult)
		SetTextures(shapeResult);

	return shapeResult;
}

std::string OutfitProject::SliderShapeDataName(const size_t index, const std::string& shapeName) {
	if (!ValidSlider(index))
		return "";

	return activeSet.ShapeToDataName(index, shapeName);
}

bool OutfitProject::SliderClamp(const size_t index) {
	if (!ValidSlider(index))
		return false;

	return activeSet[index].bClamp;
}

bool OutfitProject::SliderZap(const size_t index) {
	if (!ValidSlider(index))
		return false;

	return activeSet[index].bZap;
}

bool OutfitProject::SliderUV(const size_t index) {
	if (!ValidSlider(index))
		return false;

	return activeSet[index].bUV;
}

wxArrayString OutfitProject::SliderZapToggles(const size_t index) {
	wxArrayString toggles;
	if (ValidSlider(index))
		for (auto& toggle : activeSet[index].zapToggles)
			toggles.Add(wxString::FromUTF8(toggle));

	return toggles;
}

bool OutfitProject::SliderInvert(const size_t index) {
	if (!ValidSlider(index))
		return false;

	return activeSet[index].bInvert;
}

bool OutfitProject::SliderHidden(const size_t index) {
	if (!ValidSlider(index))
		return false;

	return activeSet[index].bHidden;
}

void OutfitProject::SetSliderZap(const size_t index, const bool zap) {
	if (!ValidSlider(index))
		return;

	activeSet[index].bZap = zap;
}

void OutfitProject::SetSliderZapToggles(const size_t index, const wxArrayString& toggles) {
	if (!ValidSlider(index))
		return;

	std::vector<std::string> zapToggles;
	for (auto& s : toggles)
		zapToggles.push_back(s.ToUTF8().data());

	activeSet[index].zapToggles = zapToggles;
}

void OutfitProject::SetSliderInvert(const size_t index, const bool inv) {
	if (!ValidSlider(index))
		return;

	activeSet[index].bInvert = inv;
}

void OutfitProject::SetSliderUV(const size_t index, const bool uv) {
	if (!ValidSlider(index))
		return;

	activeSet[index].bUV = uv;
}

void OutfitProject::SetSliderHidden(const size_t index, const bool hidden) {
	if (!ValidSlider(index))
		return;

	activeSet[index].bHidden = hidden;
}

void OutfitProject::SetSliderDefault(const size_t index, const int val, const bool isHi) {
	if (!ValidSlider(index))
		return;

	if (!isHi)
		activeSet[index].defSmallValue = val;
	else
		activeSet[index].defBigValue = val;
}

void OutfitProject::SetSliderName(const size_t index, const std::string& newName) {
	if (!ValidSlider(index))
		return;

	std::string oldName = activeSet[index].name;
	for (auto& s : workNif.GetShapes()) {
		std::string shapeName = s->name.get();
		std::string oldDT = shapeName + oldName;
		std::string newDT = shapeName + newName;

		if (IsBaseShape(s))
			baseDiffData.RenameSet(oldDT, newDT);
		else
			morpher.RenameResultDiffData(shapeName, oldName, newName);

		activeSet[index].RenameData(oldDT, newDT);
		activeSet[index].SetLocalData(newDT);
	}

	activeSet[index].name = newName;
}

float& OutfitProject::SliderValue(const size_t index) {
	return activeSet[index].curValue;
}

float& OutfitProject::SliderValue(const std::string& name) {
	return activeSet[name].curValue;
}

float OutfitProject::SliderDefault(const size_t index, const bool hi) {
	if (hi)
		return activeSet[index].defBigValue;

	return activeSet[index].defSmallValue;
}

bool& OutfitProject::SliderShow(const size_t index) {
	return activeSet[index].bShow;
}

bool& OutfitProject::SliderShow(const std::string& sliderName) {
	return activeSet[sliderName].bShow;
}

bool OutfitProject::SliderIndexFromName(const std::string& sliderName, size_t& index) {
	for (size_t i = 0; i < activeSet.size(); i++) {
		if (activeSet[i].name == sliderName) {
			index = i;
			return true;
		}
	}

	return false;
}

void OutfitProject::NegateSlider(const std::string& sliderName, NiShape* shape) {
	std::string target = ShapeToTarget(shape->name.get());

	if (IsBaseShape(shape)) {
		std::string sliderData = activeSet[sliderName].TargetDataName(target);
		baseDiffData.ScaleDiff(sliderData, target, -1.0f);
	}
	else
		morpher.ScaleResultDiff(target, sliderName, -1.0f);
}

void OutfitProject::MaskAffected(const std::string& sliderName, NiShape* shape) {
	Mesh* m = owner->glView->GetMesh(shape->name.get());
	if (!m)
		return;

	m->MaskFill(0.0f);

	if (IsBaseShape(shape)) {
		std::vector<uint16_t> outIndices;
		std::string target = ShapeToTarget(shape->name.get());

		std::string sliderData = activeSet[sliderName].TargetDataName(target);
		baseDiffData.GetDiffIndices(sliderData, target, outIndices);

		for (auto& i : outIndices) {
			if (m->nVerts > i)
				m->mask[i] = 1.0f;
		}
	}
	else {
		std::unordered_map<uint16_t, Vector3> outDiff;
		morpher.GetRawResultDiff(shape->name.get(), sliderName, outDiff);

		for (auto& i : outDiff) {
			if (m->nVerts > i.first)
				m->mask[i.first] = 1.0f;
		}
	}

	m->QueueUpdate(Mesh::UpdateType::Mask);
}

bool OutfitProject::WriteMorphTRI(const std::string& triPath) {
	DiffDataSets currentDiffs;
	activeSet.LoadSetDiffData(currentDiffs);

	TriFile tri;
	std::string triFilePath = triPath;

	for (auto& shape : workNif.GetShapes()) {
		int shapeVertCount = GetVertexCount(shape);
		if (shapeVertCount <= 0)
			continue;

		std::string shapeName = shape->name.get();
		bool bIsOutfit = true;
		if (IsBaseShape(shape))
			bIsOutfit = false;

		for (size_t s = 0; s < activeSet.size(); s++) {
			if (!activeSet[s].bClamp && !activeSet[s].bZap) {
				MorphDataPtr morph = std::make_shared<MorphData>();
				morph->name = activeSet[s].name;

				if (activeSet[s].bUV) {
					morph->type = MORPHTYPE_UV;

					std::vector<Vector2> uvs;
					uvs.resize(shapeVertCount);

					std::string target = ShapeToTarget(shapeName);
					if (!bIsOutfit) {
						std::string dn = activeSet[s].TargetDataName(target);
						if (dn.empty())
							continue;

						currentDiffs.ApplyUVDiff(dn, target, 1.0f, &uvs);
					}
					else
						morpher.ApplyResultToUVs(morph->name, target, &uvs);

					int i = 0;
					for (auto& uv : uvs) {
						Vector3 v(uv.u, uv.v, 0.0f);
						if (!v.IsZero(true))
							morph->offsets.emplace(i, v);
						i++;
					}
				}
				else {
					morph->type = MORPHTYPE_POSITION;

					std::vector<Vector3> verts;
					verts.resize(shapeVertCount);

					std::string target = ShapeToTarget(shapeName);
					if (!bIsOutfit) {
						std::string dn = activeSet[s].TargetDataName(target);
						if (dn.empty())
							continue;

						currentDiffs.ApplyDiff(dn, target, 1.0f, &verts);
					}
					else
						morpher.ApplyResultToVerts(morph->name, target, &verts);

					int i = 0;
					for (auto& v : verts) {
						if (!v.IsZero(true))
							morph->offsets.emplace(i, v);
						i++;
					}
				}

				if (morph->offsets.size() > 0)
					tri.AddMorph(shapeName, morph);
			}
		}
	}

	if (!tri.Write(triFilePath))
		return false;

	return true;
}

bool OutfitProject::WriteHeadTRI(NiShape* shape, const std::string& triPath) {
	if (!shape)
		return false;

	DiffDataSets currentDiffs;
	activeSet.LoadSetDiffData(currentDiffs);

	TriHeadFile tri;
	std::string triFilePath = triPath;

	int shapeVertCount = GetVertexCount(shape);
	if (shapeVertCount <= 0)
		return false;

	std::string shapeName = shape->name.get();
	bool bIsOutfit = true;
	if (IsBaseShape(shape))
		bIsOutfit = false;

	std::vector<Triangle> tris;
	if (!shape->GetTriangles(tris))
		return false;

	const std::vector<Vector3>* verts = workNif.GetVertsForShape(shape);
	if (!verts)
		return false;

	tri.SetVertices(*verts);
	tri.SetTriangles(tris);

	const std::vector<Vector2>* uvs = workNif.GetUvsForShape(shape);
	if (uvs)
		tri.SetUV(*uvs);

	for (size_t s = 0; s < activeSet.size(); s++) {
		if (!activeSet[s].bClamp && !activeSet[s].bZap && !activeSet[s].bUV) {
			TriHeadMorph morph;
			morph.morphName = activeSet[s].name;

			std::vector<Vector3> morphVerts;
			morphVerts.resize(shapeVertCount);

			std::string target = ShapeToTarget(shapeName);
			if (!bIsOutfit) {
				std::string dn = activeSet[s].TargetDataName(target);
				if (dn.empty())
					continue;

				currentDiffs.ApplyDiff(dn, target, 1.0f, &morphVerts);
			}
			else
				morpher.ApplyResultToVerts(morph.morphName, target, &morphVerts);

			morph.vertices = morphVerts;
			tri.AddMorph(morph);
		}
	}

	if (!tri.Write(triFilePath))
		return false;

	return true;
}

int OutfitProject::SaveSliderNIF(const std::string& sliderName, NiShape* shape, const std::string& fileName) {
	if (!shape)
		return 1;

	std::string target = ShapeToTarget(shape->name.get());

	std::vector<Vector3> outVerts;
	std::vector<Vector2> outUVs;

	if (!workNif.GetVertsForShape(shape, outVerts))
		return 2;

	workNif.GetUvsForShape(shape, outUVs);

	GetSliderDiff(shape, sliderName, outVerts);
	GetSliderDiffUV(shape, sliderName, outUVs);

	NifFile nif;
	nif.CopyFrom(workNif);

	auto sliderShape = nif.FindBlockByName<NiShape>(shape->name.get());
	if (sliderShape) {
		for (auto& s : nif.GetShapes()) {
			if (s != sliderShape)
				nif.DeleteShape(s);
		}

		nif.SetVertsForShape(sliderShape, outVerts);
		nif.SetUvsForShape(sliderShape, outUVs);

		nif.DeleteUnreferencedNodes();
	}

	std::fstream file;
	PlatformUtil::OpenFileStream(file, fileName, std::ios::out | std::ios::binary);

	if (nif.Save(file) != 0)
		return 3;

	return 0;
}

int OutfitProject::SaveSliderBSD(const std::string& sliderName, NiShape* shape, const std::string& fileName) {
	std::string target = ShapeToTarget(shape->name.get());

	if (IsBaseShape(shape)) {
		std::string sliderData = activeSet[sliderName].TargetDataName(target);
		baseDiffData.SaveSet(sliderData, target, fileName);
	}
	else
		morpher.SaveResultDiff(target, sliderName, fileName);

	return 0;
}

int OutfitProject::SaveSliderOBJ(const std::string& sliderName, NiShape* shape, const std::string& fileName, const bool onlyDiff) {
	if (!shape)
		return 1;

	std::vector<Triangle> tris;
	shape->GetTriangles(tris);

	std::string target = ShapeToTarget(shape->name.get());
	const std::vector<Vector3>* verts = workNif.GetVertsForShape(shape);
	if (!verts)
		return 2;

	const std::vector<Vector2>* uvs = workNif.GetUvsForShape(shape);
	const std::vector<Vector3> norms;

	std::vector<Vector3> outVerts = *verts;

	if (IsBaseShape(shape)) {
		std::string sliderData = activeSet[sliderName].TargetDataName(target);
		bool foundDiff = baseDiffData.ApplyDiff(sliderData, target, 1.0f, &outVerts);
		if (onlyDiff && !foundDiff)
			return 0;
	}
	else {
		bool foundDiff = morpher.ApplyResultToVerts(sliderName, target, &outVerts);
		if (onlyDiff && !foundDiff)
			return 0;
	}

	ObjFile obj;
	obj.SetScale(Vector3(0.1f, 0.1f, 0.1f));
	obj.AddGroup(shape->name.get(), outVerts, tris, uvs ? *uvs : std::vector<Vector2>(), norms);
	if (obj.Save(fileName))
		return 3;

	return 0;
}

bool OutfitProject::SetSliderFromNIF(const std::string& sliderName, NiShape* shape, const std::string& fileName) {
	std::string target = ShapeToTarget(shape->name.get());

	std::fstream file;
	PlatformUtil::OpenFileStream(file, fileName, std::ios::in | std::ios::binary);

	NifFile nif;
	if (nif.Load(file) != 0)
		return false;

	// File needs at least one shape
	auto shapeNames = nif.GetShapeNames();
	if (shapeNames.empty())
		return false;

	// Use first shape or shape with matching name
	std::string srcShapeName = shapeNames.front();
	if (std::find(shapeNames.begin(), shapeNames.end(), shape->name.get()) != shapeNames.end())
		srcShapeName = shape->name.get();

	std::vector<Vector3> verts;
	std::vector<Vector2> uvs;

	auto srcShape = nif.FindBlockByName<NiShape>(srcShapeName);
	if (!srcShape)
		return false;

	if (!nif.GetVertsForShape(srcShape, verts))
		return false;

	nif.GetUvsForShape(srcShape, uvs);

	std::unordered_map<uint16_t, Vector3> diff;
	if (activeSet[sliderName].bUV) {
		if (workNif.CalcUVDiff(shape, &uvs, diff))
			return false;
	}
	else {
		if (workNif.CalcShapeDiff(shape, &verts, diff))
			return false;
	}

	if (IsBaseShape(shape)) {
		std::string sliderData = activeSet[sliderName].TargetDataName(target);
		baseDiffData.LoadSet(sliderData, target, diff);
	}
	else
		morpher.SetResultDiff(target, sliderName, diff);

	return true;
}

void OutfitProject::SetSliderFromBSD(const std::string& sliderName, NiShape* shape, const std::string& fileName) {
	std::string target = ShapeToTarget(shape->name.get());
	if (IsBaseShape(shape)) {
		std::string sliderData = activeSet[sliderName].TargetDataName(target);
		baseDiffData.LoadSet(sliderData, target, fileName);
	}
	else {
		DiffDataSets tmpSet;
		tmpSet.LoadSet(sliderName, target, fileName);
		std::unordered_map<uint16_t, Vector3>* diff = tmpSet.GetDiffSet(sliderName);
		morpher.SetResultDiff(target, sliderName, (*diff));
	}
}

bool OutfitProject::SetSliderFromOBJ(const std::string& sliderName, NiShape* shape, const std::string& fileName) {
	std::string target = ShapeToTarget(shape->name.get());

	ObjImportOptions options;
	options.NoFaces = true;

	ObjFile obj;
	obj.LoadForNif(fileName, options);

	// File needs at least one group
	std::vector<std::string> groupNames = obj.GetGroupList();
	if (groupNames.empty())
		return false;

	// Use first shape or shape with matching name
	std::string sourceShape = groupNames.front();
	if (std::find(groupNames.begin(), groupNames.end(), shape->name.get()) != groupNames.end())
		sourceShape = shape->name.get();

	std::vector<Vector3> objVerts;
	std::vector<Vector2> objUVs;
	if (!obj.CopyDataForGroup(sourceShape, &objVerts, nullptr, &objUVs, nullptr))
		return false;

	std::unordered_map<uint16_t, Vector3> diff;
	if (activeSet[sliderName].bUV) {
		if (workNif.CalcUVDiff(shape, &objUVs, diff))
			return false;
	}
	else {
		if (workNif.CalcShapeDiff(shape, &objVerts, diff, 10.0f))
			return false;
	}

	if (IsBaseShape(shape)) {
		std::string sliderData = activeSet[sliderName].TargetDataName(target);
		baseDiffData.LoadSet(sliderData, target, diff);
	}
	else
		morpher.SetResultDiff(target, sliderName, diff);

	return true;
}

bool OutfitProject::SetSliderFromFBX(const std::string& sliderName, NiShape* shape, const std::string& fileName) {
	std::string target = ShapeToTarget(shape->name.get());

	FBXWrangler fbxw;
	bool result = fbxw.ImportScene(fileName);
	if (!result)
		return 1;

	std::vector<std::string> shapes;
	fbxw.GetShapeNames(shapes);
	bool found = false;
	for (auto& s : shapes)
		if (s == shape->name.get())
			found = true;

	if (!found)
		return false;

	FBXShape* fbxShape = fbxw.GetShape(shape->name.get());

	std::unordered_map<uint16_t, Vector3> diff;
	if (IsBaseShape(shape)) {
		if (workNif.CalcShapeDiff(shape, &fbxShape->verts, diff, 1.0f))
			return false;

		std::string sliderData = activeSet[sliderName].TargetDataName(target);
		baseDiffData.LoadSet(sliderData, target, diff);
	}
	else {
		if (workNif.CalcShapeDiff(shape, &fbxShape->verts, diff, 1.0f))
			return false;

		morpher.SetResultDiff(target, sliderName, diff);
	}

	return true;
}

void OutfitProject::SetSliderFromDiff(const std::string& sliderName, NiShape* shape, std::unordered_map<uint16_t, Vector3>& diff) {
	std::string target = ShapeToTarget(shape->name.get());
	if (IsBaseShape(shape)) {
		std::string sliderData = activeSet[sliderName].TargetDataName(target);
		baseDiffData.LoadSet(sliderData, target, diff);
	}
	else {
		morpher.EmptyResultDiff(target, sliderName);
		morpher.SetResultDiff(target, sliderName, diff);
	}
}

int OutfitProject::GetVertexCount(NiShape* shape) {
	if (workNif.IsValid()) {
		if (shape)
			return shape->GetNumVertices();
	}

	return 0;
}

void OutfitProject::GetLiveVerts(NiShape* shape, std::vector<Vector3>& outVerts, std::vector<Vector2>* outUVs) {
	workNif.GetVertsForShape(shape, outVerts);
	if (outUVs)
		workNif.GetUvsForShape(shape, *outUVs);

	std::string target = ShapeToTarget(shape->name.get());
	if (IsBaseShape(shape)) {
		for (size_t i = 0; i < activeSet.size(); i++) {
			if (activeSet[i].bShow && activeSet[i].curValue != 0.0f) {
				std::string targetData = activeSet.ShapeToDataName(i, shape->name.get());
				if (targetData.empty())
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
		for (size_t i = 0; i < activeSet.size(); i++) {
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

	if (bPose) {
		int nv = outVerts.size();
		std::vector<Vector3> pv(nv);
		std::vector<float> wv(nv, 0.0f);
		AnimSkin& animSkin = workAnim.shapeSkinning[shape->name.get()];
		MatTransform globalToSkin = workAnim.GetTransformGlobalToShape(shape);

		for (auto& boneNamesIt : animSkin.boneNames) {
			AnimBone* animB = AnimSkeleton::getInstance().GetBonePtr(boneNamesIt.first);
			if (animB) {
				AnimWeight& animW = animSkin.boneWeights[boneNamesIt.second];
				// Compose transform: skin -> (posed) bone -> global -> skin
				MatTransform t = globalToSkin.ComposeTransforms(animB->xformPoseToGlobal.ComposeTransforms(animW.xformSkinToBone));
				// Add weighted contributions to vertex for this bone
				for (auto& wIt : animW.weights) {
					int ind = wIt.first;
					float w = wIt.second;
					pv[ind] += w * t.ApplyTransform(outVerts[ind]);
					wv[ind] += w;
				}
			}
		}

		// Check if total weight for each vertex was 1
		for (int ind = 0; ind < nv; ++ind) {
			if (wv[ind] < EPSILON) // If weights are missing for this vertex
				pv[ind] = outVerts[ind];
			else if (std::fabs(wv[ind] - 1.0f) >= EPSILON) // If weights are bad for this vertex
				pv[ind] /= wv[ind];
			// else do nothing because weights totaled 1.
		}

		outVerts.swap(pv);
	}
}

void OutfitProject::GetSliderDiff(NiShape* shape, const std::string& sliderName, std::vector<Vector3>& outVerts) {
	size_t sliderIndex = 0;
	if (!SliderIndexFromName(sliderName, sliderIndex))
		return;

	std::string target = ShapeToTarget(shape->name.get());
	if (IsBaseShape(shape)) {
		std::string targetData = activeSet.ShapeToDataName(sliderIndex, shape->name.get());
		if (targetData.empty())
			return;

		if (!activeSet[sliderIndex].bUV)
			baseDiffData.ApplyDiff(targetData, target, 1.0f, &outVerts);
	}
	else {
		if (!activeSet[sliderIndex].bUV)
			morpher.ApplyResultToVerts(activeSet[sliderIndex].name, target, &outVerts);
	}
}

void OutfitProject::GetSliderDiffUV(NiShape* shape, const std::string& sliderName, std::vector<Vector2>& outUVs) {
	size_t sliderIndex = 0;
	if (!SliderIndexFromName(sliderName, sliderIndex))
		return;

	std::string target = ShapeToTarget(shape->name.get());
	if (IsBaseShape(shape)) {
		std::string targetData = activeSet.ShapeToDataName(sliderIndex, shape->name.get());
		if (targetData.empty())
			return;

		if (activeSet[sliderIndex].bUV)
			baseDiffData.ApplyUVDiff(targetData, target, 1.0f, &outUVs);
	}
	else {
		if (activeSet[sliderIndex].bUV)
			morpher.ApplyResultToUVs(activeSet[sliderIndex].name, target, &outUVs);
	}
}

const std::string& OutfitProject::ShapeToTarget(const std::string& shapeName) {
	for (auto it = activeSet.ShapesBegin(); it != activeSet.ShapesEnd(); ++it)
		if (it->first == shapeName)
			return it->second.targetShape;

	return shapeName;
}

const std::string& OutfitProject::TargetToShape(const std::string& targetName) {
	for (auto it = activeSet.ShapesBegin(); it != activeSet.ShapesEnd(); ++it)
		if (it->second.targetShape == targetName)
			return it->first;

	return targetName;
}

size_t OutfitProject::GetActiveBoneCount() {
	return AnimSkeleton::getInstance().GetActiveBoneCount();
}

void OutfitProject::GetActiveBones(std::vector<std::string>& outBoneNames) {
	AnimSkeleton::getInstance().GetActiveBoneNames(outBoneNames);
}

std::vector<std::string> OutfitProject::GetShapeTextures(NiShape* shape) {
	std::string shapeName = shape->name.get();

	if (shapeTextures.find(shapeName) != shapeTextures.end())
		return shapeTextures[shapeName];

	return std::vector<std::string>();
}

bool OutfitProject::GetShapeMaterialFile(NiShape* shape, MaterialFile& outMatFile) {
	std::string shapeName = shape->name.get();

	if (shapeMaterialFiles.find(shapeName) != shapeMaterialFiles.end()) {
		outMatFile = shapeMaterialFiles[shapeName];
		return true;
	}

	return false;
}

void OutfitProject::SetTextures() {
	for (auto& s : workNif.GetShapes())
		SetTextures(s);
}

void OutfitProject::SetTextures(const std::vector<std::string>& textureFiles) {
	for (auto& s : workNif.GetShapes())
		SetTextures(s, textureFiles);
}

void OutfitProject::SetTextures(NiShape* shape, const std::vector<std::string>& textureFiles) {
	if (!shape)
		return;

	std::string shapeName = shape->name.get();
	if (shapeName.empty())
		return;

	if (textureFiles.empty()) {
		std::string texturesDir = Config["GameDataPath"];
		bool hasMat = false;
		std::string matFile;

		const uint8_t MAX_TEXTURE_PATHS = 10;
		std::vector<std::string> texFiles(MAX_TEXTURE_PATHS);

		NiShader* shader = workNif.GetShader(shape);
		if (shader) {
			// Find material file
			if (workNif.GetHeader().GetVersion().User() == 12 && workNif.GetHeader().GetVersion().Stream() >= 130) {
				matFile = shader->name.get();
				if (!matFile.empty())
					hasMat = true;
			}
		}

		shapeMaterialFiles.erase(shapeName);

		MaterialFile mat(MaterialFile::BGSM);
		if (hasMat) {
			// Replace all backward slashes with one forward slash
			matFile = std::regex_replace(matFile, std::regex("\\\\+"), "/");

			// Remove everything before the first occurence of "/materials/"
			matFile = std::regex_replace(matFile, std::regex("^(.*?)/materials/", std::regex_constants::icase), "");

			// Remove all slashes from the front
			matFile = std::regex_replace(matFile, std::regex("^/+"), "");

			// If the path doesn't start with "materials/", add it to the front
			matFile = std::regex_replace(matFile, std::regex("^(?!^materials/)", std::regex_constants::icase), "materials/");

			// Attempt to read loose material file
			mat = MaterialFile(texturesDir + matFile);

			if (mat.Failed()) {
				// Search for material file in archives
				wxMemoryBuffer data;
				for (FSArchiveFile* archive : FSManager::archiveList()) {
					if (archive) {
						if (archive->hasFile(matFile)) {
							wxMemoryBuffer outData;
							archive->fileContents(matFile, outData);

							if (!outData.IsEmpty()) {
								data = std::move(outData);
								break;
							}
						}
					}
				}

				if (!data.IsEmpty()) {
					std::string content((char*)data.GetData(), data.GetDataLen());
					std::istringstream contentStream(content, std::istringstream::binary);

					mat = MaterialFile(contentStream);
				}
			}

			if (!mat.Failed()) {
				if (mat.signature == MaterialFile::BGSM) {
					texFiles[0] = mat.diffuseTexture.c_str();
					texFiles[1] = mat.normalTexture.c_str();
					texFiles[2] = mat.glowTexture.c_str();
					texFiles[3] = mat.greyscaleTexture.c_str();
					texFiles[4] = mat.envmapTexture.c_str();
					texFiles[7] = mat.smoothSpecTexture.c_str();
				}
				else if (mat.signature == MaterialFile::BGEM) {
					texFiles[0] = mat.baseTexture.c_str();
					texFiles[1] = mat.fxNormalTexture.c_str();
					texFiles[3] = mat.grayscaleTexture.c_str();
					texFiles[4] = mat.fxEnvmapTexture.c_str();
					texFiles[5] = mat.envmapMaskTexture.c_str();
				}

				shapeMaterialFiles[shapeName] = std::move(mat);
			}
			else if (shader) {
				for (int i = 0; i < MAX_TEXTURE_PATHS; i++)
					workNif.GetTextureSlot(shape, texFiles[i], i);
			}
		}
		else if (shader) {
			for (int i = 0; i < MAX_TEXTURE_PATHS; i++)
				workNif.GetTextureSlot(shape, texFiles[i], i);
		}

		for (int i = 0; i < MAX_TEXTURE_PATHS; i++) {
			if (!texFiles[i].empty()) {
				texFiles[i] = std::regex_replace(texFiles[i], std::regex("\\\\+"), "/"); // Replace all backward slashes with one forward slash
				texFiles[i] = std::regex_replace(texFiles[i],
												 std::regex("^(.*?)/textures/", std::regex_constants::icase),
												 "");								  // Remove everything before the first occurence of "/textures/"
				texFiles[i] = std::regex_replace(texFiles[i], std::regex("^/+"), ""); // Remove all slashes from the front
				texFiles[i] = std::regex_replace(texFiles[i],
												 std::regex("^(?!^textures/)", std::regex_constants::icase),
												 "textures/"); // If the path doesn't start with "textures/", add it to the front

				texFiles[i] = texturesDir + texFiles[i];
			}
		}

		shapeTextures[shapeName] = texFiles;
	}
	else
		shapeTextures[shapeName] = textureFiles;
}

bool OutfitProject::IsValidShape(const std::string& shapeName) {
	for (auto& s : workNif.GetShapeNames())
		if (s == shapeName)
			return true;

	return false;
}

void OutfitProject::RefreshMorphShape(NiShape* shape) {
	morpher.UpdateMeshFromNif(workNif, shape->name.get());
}

void OutfitProject::UpdateShapeFromMesh(NiShape* shape, const Mesh* m) {
	std::vector<Vector3> liveVerts(m->nVerts);

	for (int i = 0; i < m->nVerts; i++)
		liveVerts[i] = Mesh::TransformPosMeshToNif(m->verts[i]);

	workNif.SetVertsForShape(shape, liveVerts);
}

void OutfitProject::UpdateMorphResult(NiShape* shape, const std::string& sliderName, std::unordered_map<uint16_t, Vector3>& vertUpdates) {
	// Morph results are stored in two different places depending on whether it's an outfit or the base shape.
	// The outfit morphs are stored in the automorpher, whereas the base shape diff info is stored in directly in basediffdata.

	std::string target = ShapeToTarget(shape->name.get());
	std::string dataName = activeSet[sliderName].TargetDataName(target);
	if (!vertUpdates.empty()) {
		if (dataName.empty())
			activeSet[sliderName].AddDataFile(target, target + sliderName, target + sliderName);
		else
			activeSet[sliderName].SetLocalData(dataName);
	}

	if (IsBaseShape(shape)) {
		for (auto& i : vertUpdates) {
			Vector3 diffscale = Mesh::TransformDiffMeshToNif(i.second);
			baseDiffData.SumDiff(dataName, target, i.first, diffscale);
		}
	}
	else
		morpher.UpdateResultDiff(shape->name.get(), sliderName, vertUpdates);
}

void OutfitProject::ScaleMorphResult(NiShape* shape, const std::string& sliderName, float scaleValue) {
	if (IsBaseShape(shape)) {
		std::string target = ShapeToTarget(shape->name.get());
		std::string dataName = activeSet[sliderName].TargetDataName(target);
		baseDiffData.ScaleDiff(dataName, target, scaleValue);
	}
	else
		morpher.ScaleResultDiff(shape->name.get(), sliderName, scaleValue);
}

void OutfitProject::MoveVertex(NiShape* shape, const Vector3& pos, const int& id) {
	workNif.MoveVertex(shape, pos, id);
}

void OutfitProject::OffsetShape(NiShape* shape, const Vector3& xlate, std::unordered_map<uint16_t, float>* mask) {
	workNif.OffsetShape(shape, xlate, mask);
}

void OutfitProject::ScaleShape(NiShape* shape, const Vector3& scale, std::unordered_map<uint16_t, float>* mask) {
	workNif.ScaleShape(shape, scale, mask);
}

void OutfitProject::RotateShape(NiShape* shape, const Vector3& angle, std::unordered_map<uint16_t, float>* mask) {
	workNif.RotateShape(shape, angle, mask);
}

void OutfitProject::ApplyTransformToShapeGeometry(NiShape* shape, const MatTransform& t) {
	if (!shape)
		return;

	// Vertices
	const std::vector<Vector3>* oldVerts = workNif.GetVertsForShape(shape);
	if (!oldVerts || oldVerts->empty())
		return;

	size_t nVerts = oldVerts->size();
	std::vector<Vector3> verts(nVerts);
	for (size_t i = 0; i < nVerts; ++i)
		verts[i] = t.ApplyTransform((*oldVerts)[i]);

	workNif.SetVertsForShape(shape, verts);

	// Position diffs
	for (size_t si = 0; si < activeSet.size(); ++si) {
		SliderData& sd = activeSet[si];
		if (sd.bUV || sd.bClamp || sd.bZap)
			continue;

		std::unordered_map<uint16_t, Vector3>* diffSet = GetDiffSet(sd,  shape);
		if (!diffSet)
			continue;

		for (auto& diffp : *diffSet)
			diffp.second = t.ApplyTransformToDiff(diffp.second);
	}

	// Normals
	if (t.rotation.IsNearlyEqualTo(Matrix3()))
		return;

	const std::vector<Vector3>* oldNorms = workNif.GetNormalsForShape(shape);
	if (!oldNorms || oldNorms->size() != nVerts)
		return;

	std::vector<Vector3> norms(nVerts);
	for (size_t i = 0; i < nVerts; ++i)
		norms[i] = t.ApplyTransformToDir((*oldNorms)[i]);

	workNif.SetNormalsForShape(shape, norms);
}

void OutfitProject::CopyBoneWeights(NiShape* shape,
									const float proximityRadius,
									const int maxResults,
									std::unordered_map<uint16_t, float>& mask,
									const std::vector<std::string>& boneList,
									int nCopyBones,
									const std::vector<std::string>& lockedBones,
									UndoStateShape& uss,
									bool bSpreadWeight) {
	if (!shape || !baseShape)
		return;

	std::string shapeName = shape->name.get();
	std::string baseShapeName = baseShape->name.get();

	owner->UpdateProgress(1, _("Gathering bones..."));

	int nBones = boneList.size();
	if (nBones <= 0 || nCopyBones <= 0) {
		owner->UpdateProgress(90);
		return;
	}

	DiffDataSets dds;
	for (int bi = 0; bi < nCopyBones; ++bi) {
		const std::string& bone = boneList[bi];
		std::string wtSet = bone + "_WT_";
		dds.AddEmptySet(wtSet, "Weight");

		auto weights = workAnim.GetWeightsPtr(baseShapeName, bone);
		if (weights) {
			for (auto& w : *weights) {
				Vector3 tmp;
				tmp.y = w.second;
				dds.UpdateDiff(wtSet, "Weight", w.first, tmp);
			}
		}
	}

	BoneWeightAutoNormalizer nzer;
	nzer.SetUp(&uss, &workAnim, shapeName, boneList, lockedBones, nCopyBones, bSpreadWeight);
	std::unordered_set<int> vertList;

	owner->UpdateProgress(10, _("Initializing proximity data..."));

	InitConform();
	morpher.LinkRefDiffData(&dds);
	morpher.BuildProximityCache(shapeName, proximityRadius);
	CreateSkinning(shape);

	int step = 40 / nCopyBones;
	int prog = 40;
	owner->UpdateProgress(prog);

	for (int bi = 0; bi < nCopyBones; ++bi) {
		const std::string& boneName = boneList[bi];
		auto& ubw = uss.boneWeights[bi].weights;
		// Zero out unmasked weights
		auto weights = workAnim.GetWeightsPtr(shapeName, boneName);
		if (weights) {
			for (auto& pi : *weights) {
				if (mask[pi.first] > 0.0f)
					continue;
				if (vertList.find(pi.first) == vertList.end()) {
					vertList.insert(pi.first);
					nzer.GrabOneVertexStartingWeights(pi.first);
				}
				ubw[pi.first].endVal = 0.0;
			}
		}

		// Calculate new values for bone's weights
		std::string wtSet = boneName + "_WT_";
		morpher.GenerateResultDiff(shapeName, wtSet, wtSet, false, maxResults);

		std::unordered_map<uint16_t, Vector3> diffResult;
		morpher.GetRawResultDiff(shapeName, wtSet, diffResult);

		// Copy unmasked weights from diffResult into uss
		for (auto& dr : diffResult) {
			if (mask[dr.first] > 0.0f)
				continue;
			if (vertList.find(dr.first) == vertList.end()) {
				vertList.insert(dr.first);
				nzer.GrabOneVertexStartingWeights(dr.first);
			}
			ubw[dr.first].endVal = dr.second.y;
		}

		owner->UpdateProgress(prog += step, _("Copying bone weights..."));
	}
	morpher.UnlinkRefDiffData();

	// Normalize
	for (auto vInd : vertList)
		nzer.AdjustWeights(vInd);

	owner->UpdateProgress(90);
}

void OutfitProject::TransferSelectedWeights(NiShape* shape, std::unordered_map<uint16_t, float>* mask, std::vector<std::string>* inBoneList) {
	if (!shape || !baseShape)
		return;

	std::string shapeName = shape->name.get();
	std::string baseShapeName = baseShape->name.get();

	owner->UpdateProgress(10, _("Gathering bones..."));

	std::vector<std::string>* boneList;
	std::vector<std::string> allBoneList;
	if (!inBoneList) {
		for (auto& boneName : workAnim.shapeBones[baseShapeName])
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

	for (auto& boneName : *boneList) {
		std::unordered_map<uint16_t, float> weights;
		std::unordered_map<uint16_t, float> oldWeights;
		workAnim.GetWeights(baseShapeName, boneName, weights);
		workAnim.GetWeights(shapeName, boneName, oldWeights);

		for (auto& w : weights) {
			if (mask) {
				if (1.0f - (*mask)[w.first] > 0.0f)
					weights[w.first] = w.second * (1.0f - (*mask)[w.first]);
				else
					weights[w.first] = oldWeights[w.first];
			}
			else
				weights[w.first] = w.second;
		}

		workAnim.AddShapeBone(shapeName, boneName);
		workAnim.SetWeights(shapeName, boneName, weights);
		owner->UpdateProgress(prog += step, "");
	}

	owner->UpdateProgress(100, _("Finished"));
}

bool OutfitProject::HasUnweighted(std::vector<std::string>* shapeNames) {
	bool hasUnweighted = false;

	for (auto& shape : workNif.GetShapes()) {
		if (!shape || !shape->IsSkinned())
			continue;

		std::string shapeName = shape->name.get();
		std::vector<Vector3> verts;
		workNif.GetVertsForShape(shape, verts);

		std::unordered_map<int, int> influences;
		for (size_t i = 0; i < verts.size(); i++)
			influences.emplace(i, 0);

		if (workAnim.shapeBones.find(shapeName) != workAnim.shapeBones.end()) {
			for (auto& b : workAnim.shapeBones[shapeName]) {
				auto weights = workAnim.GetWeightsPtr(shapeName, b);
				if (weights) {
					for (size_t i = 0; i < verts.size(); i++) {
						auto id = weights->find(i);
						if (id != weights->end() && id->second > 0.0f)
							influences.at(i)++;
					}
				}
			}
		}

		bool shapeUnweighted = false;
		Mesh* m = owner->glView->GetMesh(shapeName);
		if (m) {
			for (auto& i : influences) {
				if (i.second == 0) {
					if (!shapeUnweighted)
						m->MaskFill(0.0f);

					m->mask[i.first] = 1.0f;
					shapeUnweighted = true;
				}
			}

			if (shapeUnweighted) {
				hasUnweighted = true;

				if (shapeNames)
					shapeNames->push_back(shapeName);
			}

			m->QueueUpdate(Mesh::UpdateType::Mask);
		}
	}

	return hasUnweighted;
}

void OutfitProject::AddBoneRef(const std::string& boneName) {
	for (auto& s : workNif.GetShapeNames())
		workAnim.AddShapeBone(s, boneName);
}

void OutfitProject::AddCustomBoneRef(const std::string& boneName, const std::string& parentBone, const MatTransform& xformToParent) {
	AnimBone& customBone = AnimSkeleton::getInstance().AddCustomBone(boneName);
	customBone.SetTransformBoneToParent(xformToParent);
	customBone.SetParentBone(AnimSkeleton::getInstance().GetBonePtr(parentBone));

	for (auto& s : workNif.GetShapeNames())
		workAnim.AddShapeBone(s, boneName);
}

void OutfitProject::ModifyCustomBone(AnimBone* bPtr, const std::string& parentBone, const MatTransform& xformToParent) {
	bPtr->SetTransformBoneToParent(xformToParent);
	bPtr->SetParentBone(AnimSkeleton::getInstance().GetBonePtr(parentBone));

	for (auto& s : workNif.GetShapeNames())
		workAnim.RecursiveRecalcXFormSkinToBone(s, bPtr);
}

int OutfitProject::CopySegPart(NiShape* shape) {
	// Gather baseShape's triangles and vertices
	std::vector<Triangle> tris;
	baseShape->GetTriangles(tris);
	std::vector<Vector3> verts;
	workNif.GetVertsForShape(baseShape, verts);

	// Transform baseShape vertices to nif global coordinates
	MatTransform xformToGlobal = workAnim.GetTransformShapeToGlobal(baseShape);
	for (Vector3& v : verts)
		v = xformToGlobal.ApplyTransform(v);

	// Calculate center of each triangle (times 3)
	std::vector<Vector3> tricenters(tris.size());
	for (size_t ti = 0; ti < tris.size(); ++ti)
		tricenters[ti] = verts[tris[ti].p1] + verts[tris[ti].p2] + verts[tris[ti].p3];

	// Build proximity cache for triangle centers of baseShape.
	// Note that kd_tree keeps pointers into tricenters.
	kd_tree<uint32_t> refTree(&tricenters[0], tricenters.size());

	// Get baseShape's segment/partition data
	std::vector<int> bstriParts;
	NifSegmentationInfo inf;
	bool gotsegs = workNif.GetShapeSegments(baseShape, inf, bstriParts);
	NiVector<BSDismemberSkinInstance::PartitionInfo> partitionInfo;
	bool gotparts = false;
	if (!gotsegs)
		gotparts = workNif.GetShapePartitions(baseShape, partitionInfo, bstriParts);

	// Gather shape's triangles and vertices
	shape->GetTriangles(tris);
	workNif.GetVertsForShape(shape, verts);

	// Transform shape's vertices to nif global coordinates
	xformToGlobal = workAnim.GetTransformShapeToGlobal(shape);
	for (Vector3& v : verts)
		v = xformToGlobal.ApplyTransform(v);

	// Calculate new partition/segment for each triangle
	std::vector<int> triParts(tris.size());
	int failcount = 0;
	for (size_t ti = 0; ti < tris.size(); ++ti) {
		// Calculate center of triangle (times 3)
		Vector3 tricenter = verts[tris[ti].p1] + verts[tris[ti].p2] + verts[tris[ti].p3];

		// Find closest triangle center in proximity cache.
		uint32_t resultcount = refTree.kd_nn(&tricenter, 0);
		if (resultcount < 1) {
			++failcount;
			triParts[ti] = -1;
			continue;
		}

		// Look up closest triangle and copy its partition/segment ID.
		size_t bti = refTree.queryResult[0].vertex_index;
		triParts[ti] = bstriParts[bti];
		if (triParts[ti] == -1)
			++failcount;
	}

	// Refuse to continue if we have any triangles without segments/partitions.
	if (failcount)
		return failcount;

	// Store new information in NIF.
	if (gotsegs)
		workNif.SetShapeSegments(shape, inf, triParts);

	if (gotparts) {
		workNif.SetShapePartitions(shape, partitionInfo, triParts);
		workNif.RemoveEmptyPartitions(shape);
	}

	return 0;
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
	for (auto& s : workNif.GetShapes()) {
		if (IsBaseShape(s))
			continue;

		DeleteShape(s);
	}
	ClearWorkSliders();
}

void OutfitProject::ClearSlider(NiShape* shape, const std::string& sliderName) {
	std::string target = ShapeToTarget(shape->name.get());

	if (IsBaseShape(shape)) {
		std::string data = activeSet[sliderName].TargetDataName(target);
		baseDiffData.EmptySet(data, target);
	}
	else
		morpher.EmptyResultDiff(target, sliderName);
}

void OutfitProject::ClearUnmaskedDiff(NiShape* shape, const std::string& sliderName, std::unordered_map<uint16_t, float>* mask) {
	std::string target = ShapeToTarget(shape->name.get());

	if (IsBaseShape(shape)) {
		std::string data = activeSet[sliderName].TargetDataName(target);
		baseDiffData.ZeroVertDiff(data, target, nullptr, mask);
	}
	else
		morpher.ZeroVertDiff(target, sliderName, nullptr, mask);
}

void OutfitProject::DeleteSlider(const std::string& sliderName) {
	for (auto& s : workNif.GetShapes()) {
		std::string target = ShapeToTarget(s->name.get());
		std::string data = activeSet[sliderName].TargetDataName(target);

		if (IsBaseShape(s))
			baseDiffData.ClearSet(data);
		else
			morpher.ClearResultSet(data);
	}

	activeSet.DeleteSlider(sliderName);
}

int OutfitProject::LoadSkeletonReference(const std::string& skeletonFileName) {
	return AnimSkeleton::getInstance().LoadFromNif(skeletonFileName);
}

int OutfitProject::LoadReferenceTemplate(const std::string& sourceFile, const std::string& set, const std::string& shape, bool loadAll, bool mergeSliders, bool mergeZaps) {
	if (sourceFile.empty() || set.empty()) {
		wxLogError("Template source entries are invalid.");
		wxMessageBox(_("Template source entries are invalid."), _("Reference Error"), wxICON_ERROR, owner);
		return 1;
	}

	if (loadAll) {
		owner->StartSubProgress(10, 20);
		return AddFromSliderSet(sourceFile, set);
	}
	else
		return LoadReference(sourceFile, set, shape, mergeSliders, mergeZaps);
}

int OutfitProject::LoadReferenceNif(const std::string& fileName, const std::string& shapeName, bool mergeSliders, bool mergeZaps) {
	if (mergeZaps || mergeSliders) {
		owner->DeleteSliders(mergeSliders, mergeZaps);
		DeleteShape(baseShape);
	}
	else
		ClearReference();

	std::fstream file;
	PlatformUtil::OpenFileStream(file, fileName, std::ios::in | std::ios::binary);

	NifFile refNif;
	int error = refNif.Load(file);
	if (error) {
		if (error == 2) {
			wxString errorText = wxString::Format(_("NIF version not supported!\n\nFile: %s\n%s"), fileName, refNif.GetHeader().GetVersion().GetVersionInfo());

			wxLogError(errorText);
			wxMessageBox(errorText, _("Reference Error"), wxICON_ERROR, owner);
			return 3;
		}

		wxLogError("Could not load reference NIF file '%s'!", fileName);
		wxMessageBox(wxString::Format(_("Could not load reference NIF file '%s'!"), fileName), _("Reference Error"), wxICON_ERROR, owner);
		return 2;
	}

	ValidateNIF(refNif);

	auto refShapeDup = workNif.FindBlockByName<NiShape>(shapeName);
	auto refShape = refNif.FindBlockByName<NiShape>(shapeName);
	if (refShapeDup && refShape) {
		std::string newName = shapeName + "_ref";
		refNif.RenameShape(refShape, newName);
	}

	if (workNif.IsValid()) {
		// Copy only reference shape
		auto clonedShape = workNif.CloneShape(refShape, shapeName, &refNif);
		workAnim.LoadFromNif(&workNif, clonedShape);
	}
	else {
		// Copy the full file
		workNif.CopyFrom(refNif);
		workAnim.LoadFromNif(&workNif);

		// Delete all except for reference
		for (auto& s : workNif.GetShapes())
			if (s->name != shapeName)
				DeleteShape(s);
	}

	baseShape = workNif.FindBlockByName<NiShape>(shapeName);
	activeSet.LoadSetDiffData(baseDiffData);
	return 0;
}

int OutfitProject::LoadReference(const std::string& fileName, const std::string& setName, const std::string& shapeName, bool mergeSliders, bool mergeZaps) {
	if (mergeZaps || mergeSliders) {
		owner->DeleteSliders(mergeSliders, mergeZaps);
		DeleteShape(baseShape);
	}
	else
		ClearReference();

	SliderSetFile sset(fileName);
	if (sset.fail()) {
		wxLogError("Could not load slider set file '%s'!", fileName);
		wxMessageBox(wxString::Format(_("Could not load slider set file '%s'!"), fileName), _("Reference Error"), wxICON_ERROR, owner);
		return 1;
	}

	std::string dataFolder = activeSet.GetDefaultDataFolder();
	std::vector<std::string> dataNames = activeSet.GetLocalData(shapeName);

	sset.GetSet(setName, activeSet);

	activeSet.SetBaseDataPath(GetProjectPath() + PathSepStr + "ShapeData");
	std::string refFile = activeSet.GetInputFileName();

	std::fstream file;
	PlatformUtil::OpenFileStream(file, refFile, std::ios::in | std::ios::binary);

	NifFile refNif;
	int error = refNif.Load(file);
	if (error) {
		if (error == 2) {
			wxString errorText = wxString::Format(_("NIF version not supported!\n\nFile: %s\n%s"), refFile, refNif.GetHeader().GetVersion().GetVersionInfo());

			wxLogError(errorText);
			wxMessageBox(errorText, _("Reference Error"), wxICON_ERROR, owner);
			ClearReference();
			return 5;
		}

		ClearReference();
		wxLogError("Could not load reference NIF file '%s'!", refFile);
		wxMessageBox(wxString::Format(_("Could not load reference NIF file '%s'!"), refFile), _("Reference Error"), wxICON_ERROR, owner);
		return 2;
	}

	ValidateNIF(refNif);

	std::vector<std::string> shapes = refNif.GetShapeNames();
	if (shapes.empty()) {
		ClearReference();
		wxLogError("Reference NIF file '%s' does not contain any shapes.", refFile);
		wxMessageBox(wxString::Format(_("Reference NIF file '%s' does not contain any shapes."), refFile), _("Reference Error"), wxICON_ERROR, owner);
		return 3;
	}

	std::string shape = shapeName;
	if (shape.empty())
		shape = shapes[0];

	auto refShape = refNif.FindBlockByName<NiShape>(shape);
	if (!refShape) {
		ClearReference();
		wxLogError("Shape '%s' not found in reference NIF file '%s'!", shape, refFile);
		wxMessageBox(wxString::Format(_("Shape '%s' not found in reference NIF file '%s'!"), shape, refFile), _("Reference Error"), wxICON_ERROR, owner);
		return 4;
	}

	auto refShapeDup = workNif.FindBlockByName<NiShape>(shape);
	if (refShapeDup) {
		std::string newName = shape + "_ref";
		refNif.RenameShape(refShape, newName);
	}

	// Add cloth data block of NIF to the list
	std::vector<BSClothExtraData*> clothDataBlocks = refNif.GetChildren<BSClothExtraData>(nullptr, true);
	for (auto& cloth : clothDataBlocks)
		clothData[refFile] = cloth->Clone();

	refNif.GetHeader().DeleteBlockByType("BSClothExtraData");

	if (workNif.IsValid()) {
		// Copy only reference shape
		auto clonedShape = workNif.CloneShape(refShape, shape, &refNif);
		workAnim.LoadFromNif(&workNif, clonedShape);
	}
	else {
		// Copy the full file
		workNif.CopyFrom(refNif);
		workAnim.LoadFromNif(&workNif);

		// Delete all except for reference
		for (auto& s : workNif.GetShapes())
			if (s->name != shape)
				DeleteShape(s);
	}

	baseShape = workNif.FindBlockByName<NiShape>(shape);

	if (mergeSliders)
		activeSet.LoadSetDiffData(baseDiffData, shape);
	else
		activeSet.LoadSetDiffData(baseDiffData);

	activeSet.SetReferencedData(shape);
	for (auto& dn : dataNames)
		activeSet.SetReferencedDataByName(shape, dn, true);

	// Keep default data folder from current project if existing
	if (!dataFolder.empty())
		activeSet.SetDataFolder(dataFolder);

	return 0;
}

int OutfitProject::LoadFromSliderSet(const std::string& fileName, const std::string& sliderSetName, std::vector<std::string>* origShapeOrder) {
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

	activeSet.SetBaseDataPath(GetProjectPath() + PathSepStr + "ShapeData");

	std::string inputNif = activeSet.GetInputFileName();

	owner->UpdateProgress(30, _("Loading outfit shapes..."));
	if (ImportNIF(inputNif, true, sliderSetName)) {
		owner->EndProgress();
		return 4;
	}

	if (origShapeOrder)
		*origShapeOrder = workNif.GetShapeNames();

	NiShape* newBaseShape = nullptr;

	// First external target with skin shader becomes reference
	std::vector<std::string> refTargets;
	activeSet.GetReferencedTargets(refTargets);
	for (auto& target : refTargets) {
		std::string shapeName = activeSet.TargetToShape(target);
		auto shape = workNif.FindBlockByName<NiShape>(shapeName);
		if (shape) {
			NiShader* shader = workNif.GetShader(shape);
			if (shader && shader->IsSkinTinted()) {
				newBaseShape = shape;
				break;
			}
		}
	}

	// No external target found, first skin shaded shape becomes reference
	if (refTargets.empty()) {
		for (auto shapeTarget = activeSet.ShapesBegin(); shapeTarget != activeSet.ShapesEnd(); ++shapeTarget) {
			auto shape = workNif.FindBlockByName<NiShape>(shapeTarget->first);
			if (shape) {
				NiShader* shader = workNif.GetShader(shape);
				if (shader && shader->IsSkinTinted()) {
					newBaseShape = shape;
					break;
				}
			}
		}
	}

	// Store base shape for later deletion
	baseShape = newBaseShape;

	owner->UpdateProgress(90, _("Updating slider data..."));
	morpher.LoadResultDiffs(activeSet);

	wxString rest;
	mFileName = wxString::FromUTF8(fileName);
	if (mFileName.EndsWith(".xml", &rest))
		mFileName = rest.Append(".osp");

	mOutfitName = wxString::FromUTF8(sliderSetName);
	mDataDir = wxString::FromUTF8(activeSet.GetDefaultDataFolder());
	mBaseFile = wxString::FromUTF8(activeSet.GetInputFileName());
	mBaseFile = mBaseFile.AfterLast('/').AfterLast('\\');

	mGamePath = wxString::FromUTF8(activeSet.GetOutputPath());
	mGameFile = wxString::FromUTF8(activeSet.GetOutputFile());
	mCopyRef = true;
	mGenWeights = activeSet.GenWeights();

	owner->UpdateProgress(100, _("Finished"));
	owner->EndProgress();
	return 0;
}

int OutfitProject::AddFromSliderSet(const std::string& fileName, const std::string& sliderSetName, const bool newDataLocal) {
	owner->StartProgress(_("Adding slider set..."));
	SliderSetFile InSS(fileName);
	if (InSS.fail()) {
		owner->EndProgress();
		return 1;
	}

	SliderSet addSet;
	owner->UpdateProgress(20, _("Retrieving sliders..."));
	if (InSS.GetSet(sliderSetName, addSet)) {
		owner->EndProgress();
		return 2;
	}

	addSet.SetBaseDataPath(GetProjectPath() + PathSepStr + "ShapeData");
	std::string inputNif = addSet.GetInputFileName();

	std::map<std::string, std::string> renamedShapes;
	owner->UpdateProgress(30, _("Adding outfit shapes..."));
	if (ImportNIF(inputNif, false, "", &renamedShapes)) {
		owner->EndProgress();
		return 3;
	}

	if (!baseShape) {
		NiShape* newBaseShape = nullptr;

		// First external target with skin shader becomes reference
		std::vector<std::string> refTargets;
		addSet.GetReferencedTargets(refTargets);
		for (auto& target : refTargets) {
			std::string shapeName = addSet.TargetToShape(target);
			auto shape = workNif.FindBlockByName<NiShape>(shapeName);
			if (shape) {
				NiShader* shader = workNif.GetShader(shape);
				if (shader && shader->IsSkinTinted()) {
					newBaseShape = shape;
					break;
				}
			}
		}

		// No external target found, first skin shaded shape becomes reference
		if (refTargets.empty()) {
			for (auto shapeTarget = addSet.ShapesBegin(); shapeTarget != addSet.ShapesEnd(); ++shapeTarget) {
				auto shape = workNif.FindBlockByName<NiShape>(shapeTarget->first);
				if (shape) {
					NiShader* shader = workNif.GetShader(shape);
					if (shader && shader->IsSkinTinted()) {
						newBaseShape = shape;
						break;
					}
				}
			}
		}

		baseShape = newBaseShape;
	}

	if (!renamedShapes.empty()) {
		std::vector<std::string> renamedShapesOrig;
		renamedShapesOrig.reserve(renamedShapes.size());
		for (auto& rs : renamedShapes)
			renamedShapesOrig.push_back(rs.first);

		std::string shapesJoin = JoinStrings(renamedShapesOrig, "; ");
		wxMessageBox(wxString::Format("%s\n \n%s",
									  _("The following shapes were renamed and won't have slider data attached. Rename the duplicates yourself beforehand."),
									  shapesJoin),
					 _("Renamed Shapes"),
					 wxOK | wxICON_WARNING,
					 owner);
	}

	owner->UpdateProgress(70, _("Updating slider data..."));
	morpher.MergeResultDiffs(activeSet, addSet, baseDiffData, baseShape ? baseShape->name.get() : "", newDataLocal);

	owner->UpdateProgress(100, _("Finished"));
	owner->EndProgress();
	return 0;
}

void OutfitProject::InitConform() {
	if (baseShape) {
		morpher.SetRef(workNif, baseShape, &workAnim);
		morpher.LinkRefDiffData(&baseDiffData);
		morpher.SourceShapesFromNif(workNif, &workAnim);
	}
}

void OutfitProject::ConformShape(NiShape* shape, const ConformOptions& options) {
	if (!workNif.IsValid() || !baseShape)
		return;

	std::unordered_map<uint16_t, float> mask;
	owner->glView->GetShapeMask(mask, baseShape->name.get());

	std::set<uint16_t> maskIndices;
	for (auto& m : mask)
		maskIndices.insert(m.first);

	morpher.BuildProximityCache(shape->name.get(), options.proximityRadius, &maskIndices);

	std::string refTarget = ShapeToTarget(baseShape->name.get());
	for (size_t i = 0; i < activeSet.size(); i++)
		if (SliderShow(i) && !SliderZap(i) && !SliderUV(i))
			morpher.GenerateResultDiff(shape->name.get(),
									   activeSet[i].name,
									   activeSet[i].TargetDataName(refTarget),
									   true,
									   options.maxResults,
									   options.noSqueeze,
									   options.solidMode,
									   options.axisX,
									   options.axisY,
									   options.axisZ);
}

std::unordered_map<uint16_t, Vector3>* OutfitProject::GetDiffSet(SliderData& sliderData, NiShape* shape) {
	std::string target = ShapeToTarget(shape->name.get());
	std::string targetDataName = sliderData.TargetDataName(target);
	if (targetDataName.empty())
		targetDataName = target + sliderData.name;

	if (IsBaseShape(shape))
		return baseDiffData.GetDiffSet(targetDataName);
	else
		return morpher.GetDiffSet(targetDataName);
}

void OutfitProject::CollectVertexData(NiShape* shape, UndoStateShape& uss, const std::vector<uint16_t>& indices) {
	uss.delVerts.resize(indices.size());

	const std::vector<Vector3>* verts = workNif.GetVertsForShape(shape);
	const std::vector<Vector2>* uvs = workNif.GetUvsForShape(shape);
	const std::vector<Color4>* colors = workNif.GetColorsForShape(shape->name.get());
	const std::vector<Vector3>* normals = workNif.GetNormalsForShape(shape);
	const std::vector<Vector3>* tangents = workNif.GetTangentsForShape(shape);
	const std::vector<Vector3>* bitangents = workNif.GetBitangentsForShape(shape);
	const std::vector<float>* eyeData = workNif.GetEyeDataForShape(shape);
	AnimSkin& skin = workAnim.shapeSkinning[shape->name.get()];
	Mesh* m = owner->glView->GetMesh(shape->name.get());
	const float* mask = m ? m->mask.get() : nullptr;

	for (uint16_t di = 0; di < static_cast<uint16_t>(indices.size()); ++di) {
		UndoStateVertex& usv = uss.delVerts[di];
		uint16_t vi = indices[di];
		usv.index = vi;

		if (verts && verts->size() > vi)
			usv.pos = (*verts)[vi];
		if (uvs && uvs->size() > vi)
			usv.uv = (*uvs)[vi];
		if (colors && colors->size() > vi)
			usv.color = (*colors)[vi];
		if (normals && normals->size() > vi)
			usv.normal = (*normals)[vi];
		if (tangents && tangents->size() > vi)
			usv.tangent = (*tangents)[vi];
		if (bitangents && bitangents->size() > vi)
			usv.bitangent = (*bitangents)[vi];
		if (eyeData && eyeData->size() > vi)
			usv.eyeData = (*eyeData)[vi];
		if (mask)
			usv.mask = mask[vi];

		for (auto bnp : skin.boneNames) {
			AnimWeight& aw = skin.boneWeights[bnp.second];
			auto wit = aw.weights.find(vi);
			if (wit != aw.weights.end())
				usv.weights.emplace_back(UndoStateVertexBoneWeight{bnp.first, wit->second});
		}
	}

	// For diffs, it's more efficient to reverse the loop nesting.
	for (size_t si = 0; si < activeSet.size(); ++si) {
		std::unordered_map<uint16_t, Vector3>* diffSet = GetDiffSet(activeSet[si], shape);

		if (!diffSet)
			continue;

		for (UndoStateVertex& usv : uss.delVerts) {
			auto dit = diffSet->find(usv.index);
			if (dit == diffSet->end())
				continue;

			usv.diffs.push_back(UndoStateVertexSliderDiff{activeSet[si].name, dit->second});
		}
	}
}

void OutfitProject::CollectTriangleData(NiShape* shape, UndoStateShape& uss, const std::vector<uint32_t>& indices) {
	uss.delTris.resize(indices.size());

	std::vector<Triangle> tris;
	shape->GetTriangles(tris);

	std::vector<int> triParts;
	NifSegmentationInfo inf;
	if (!workNif.GetShapeSegments(shape, inf, triParts)) {
		NiVector<BSDismemberSkinInstance::PartitionInfo> partitionInfo;
		workNif.GetShapePartitions(shape, partitionInfo, triParts);
	}

	for (uint32_t di = 0; di < static_cast<uint32_t>(indices.size()); ++di) {
		UndoStateTriangle& ust = uss.delTris[di];
		uint32_t ti = indices[di];
		ust.index = ti;
		ust.t = tris[ti];
		if (ti < triParts.size())
			ust.partID = triParts[ti];
		else
			ust.partID = -1;
	}
}

bool OutfitProject::PrepareDeleteVerts(NiShape* shape, const std::unordered_map<uint16_t, float>& mask, UndoStateShape& uss) {
	uint16_t numVerts = shape->GetNumVertices();

	// Set flag for every vertex index in mask
	std::vector<bool> delVertFlags(numVerts, false);
	for (auto& m : mask)
		delVertFlags[m.first] = true;

	// Generate list of triangles to delete.  Also count how many
	// non-deleted triangles each vertex belongs to.
	std::vector<Triangle> tris;
	shape->GetTriangles(tris);

	std::vector<uint32_t> delTriInds;
	std::vector<uint32_t> vertTriCounts(numVerts, 0);

	for (uint32_t ti = 0; ti < static_cast<uint32_t>(tris.size()); ++ti) {
		if (delVertFlags[tris[ti].p1] || delVertFlags[tris[ti].p2] || delVertFlags[tris[ti].p3])
			delTriInds.push_back(ti);
		else {
			++vertTriCounts[tris[ti].p1];
			++vertTriCounts[tris[ti].p2];
			++vertTriCounts[tris[ti].p3];
		}
	}

	// If all triangles are deleted, then delete the whole shape.
	if (delTriInds.size() >= tris.size())
		return true;

	// Generate new list of vertices to delete: vertices that are not
	// used by any non-deleted triangle.
	std::vector<uint16_t> delVertInds;
	for (uint16_t vi = 0; vi < numVerts; ++vi)
		if (vertTriCounts[vi] <= 0)
			delVertInds.push_back(vi);

	// If all vertices are deleted, then delete the whole shape.
	if (delVertInds.size() >= numVerts)
		return true;

	// Now collect the vertex and triangle data.
	CollectVertexData(shape, uss, delVertInds);
	CollectTriangleData(shape, uss, delTriInds);

	return false;
}

void OutfitProject::ApplyShapeMeshUndo(NiShape* shape, std::vector<float>& mask, const UndoStateShape& uss, bool bUndo) {
	const std::vector<UndoStateVertex>& delVerts = bUndo ? uss.addVerts : uss.delVerts;
	const std::vector<UndoStateVertex>& addVerts = bUndo ? uss.delVerts : uss.addVerts;
	const std::vector<UndoStateTriangle>& delTris = bUndo ? uss.addTris : uss.delTris;
	const std::vector<UndoStateTriangle>& addTris = bUndo ? uss.delTris : uss.addTris;

	// Gather data
	std::vector<Triangle> tris;
	shape->GetTriangles(tris);

	std::vector<int> triParts;
	NifSegmentationInfo inf;
	bool gotsegs = workNif.GetShapeSegments(shape, inf, triParts);
	NiVector<BSDismemberSkinInstance::PartitionInfo> partitionInfo;
	bool gotparts = false;
	if (!gotsegs)
		gotparts = workNif.GetShapePartitions(shape, partitionInfo, triParts);

	std::vector<Vector3> verts;
	workNif.GetVertsForShape(shape, verts);

	const std::vector<Vector2>* uvsp = workNif.GetUvsForShape(shape);
	const std::vector<Color4>* colorsp = workNif.GetColorsForShape(shape->name.get());
	const std::vector<Vector3>* normalsp = workNif.GetNormalsForShape(shape);
	const std::vector<Vector3>* tangentsp = workNif.GetTangentsForShape(shape);
	const std::vector<Vector3>* bitangentsp = workNif.GetBitangentsForShape(shape);
	const std::vector<float>* eyeDatap = workNif.GetEyeDataForShape(shape);

	std::vector<Vector2> uvs;
	std::vector<Color4> colors;
	std::vector<Vector3> normals;
	std::vector<Vector3> tangents;
	std::vector<Vector3> bitangents;
	std::vector<float> eyeData;
	if (uvsp)
		uvs = *uvsp;
	if (colorsp)
		colors = *colorsp;
	if (normalsp)
		normals = *normalsp;
	if (tangentsp)
		tangents = *tangentsp;
	if (bitangentsp)
		bitangents = *bitangentsp;
	if (eyeDatap)
		eyeData = *eyeDatap;
	if (mask.size() != verts.size())
		mask.resize(verts.size());

	AnimSkin& skin = workAnim.shapeSkinning[shape->name.get()];
	std::string target = ShapeToTarget(shape->name.get());

	if (!delTris.empty()) {
		// Delete triangles
		std::vector<uint32_t> delTriInds(delTris.size());
		for (uint32_t di = 0; di < static_cast<uint32_t>(delTris.size()); ++di)
			delTriInds[di] = delTris[di].index;

		EraseVectorIndices(tris, delTriInds);

		if (gotsegs || gotparts)
			EraseVectorIndices(triParts, delTriInds);
	}

	bool makeLocal = false;

	if (!delVerts.empty()) {
		// Delete vertices...
		std::vector<uint16_t> delVertInds(delVerts.size());
		for (uint16_t di = 0; di < static_cast<uint16_t>(delVerts.size()); ++di)
			delVertInds[di] = delVerts[di].index;

		std::vector<int> vertCollapseMap = GenerateIndexCollapseMap(delVertInds, verts.size());

		// ...from triangles
		ApplyMapToTriangles(tris, vertCollapseMap);

		// ...from workAnim
		workAnim.DeleteVertsForShape(shape->name.get(), delVertInds);

		// ...from diff data
		if (IsBaseShape(shape))
			baseDiffData.DeleteVerts(target, delVertInds);
		else
			morpher.DeleteVerts(target, delVertInds);

		// ...from nif arrays
		EraseVectorIndices(verts, delVertInds);
		EraseVectorIndices(mask, delVertInds);
		if (uvsp)
			EraseVectorIndices(uvs, delVertInds);
		if (colorsp)
			EraseVectorIndices(colors, delVertInds);
		if (normalsp)
			EraseVectorIndices(normals, delVertInds);
		if (tangentsp)
			EraseVectorIndices(tangents, delVertInds);
		if (bitangentsp)
			EraseVectorIndices(bitangents, delVertInds);
		if (eyeDatap)
			EraseVectorIndices(eyeData, delVertInds);

		makeLocal = true;
	}

	if (!addVerts.empty()) {
		// Insert new vertex indices...
		std::vector<uint16_t> insVertInds(addVerts.size());
		for (uint16_t di = 0; di < static_cast<uint16_t>(addVerts.size()); ++di)
			insVertInds[di] = addVerts[di].index;

		std::vector<int> vertExpandMap = GenerateIndexExpandMap(insVertInds, verts.size());

		// ...into triangles
		ApplyMapToTriangles(tris, vertExpandMap);

		// ...into workAnim
		skin.InsertVertexIndices(insVertInds);

		// ...into diff data
		if (IsBaseShape(shape))
			baseDiffData.InsertVertexIndices(target, insVertInds);
		else
			morpher.InsertVertexIndices(target, insVertInds);

		// ...into nif arrays
		InsertVectorIndices(verts, insVertInds);
		InsertVectorIndices(mask, insVertInds);
		if (uvsp)
			InsertVectorIndices(uvs, insVertInds);
		if (colorsp)
			InsertVectorIndices(colors, insVertInds);
		if (normalsp)
			InsertVectorIndices(normals, insVertInds);
		if (tangentsp)
			InsertVectorIndices(tangents, insVertInds);
		if (bitangentsp)
			InsertVectorIndices(bitangents, insVertInds);
		if (eyeDatap)
			InsertVectorIndices(eyeData, insVertInds);

		// Store vertex data...
		for (const UndoStateVertex& usv : addVerts) {
			// ...in nif arrays
			verts[usv.index] = usv.pos;
			mask[usv.index] = usv.mask;
			if (uvsp && uvs.size() > usv.index)
				uvs[usv.index] = usv.uv;
			if (colorsp && colors.size() > usv.index)
				colors[usv.index] = usv.color;
			if (normalsp && normals.size() > usv.index)
				normals[usv.index] = usv.normal;
			if (tangentsp && tangents.size() > usv.index)
				tangents[usv.index] = usv.tangent;
			if (bitangentsp && bitangents.size() > usv.index)
				bitangents[usv.index] = usv.bitangent;
			if (eyeDatap && eyeData.size() > usv.index)
				eyeData[usv.index] = usv.eyeData;

			// ...in workAnim
			for (const auto& usvbw : usv.weights) {
				auto bnit = skin.boneNames.find(usvbw.boneName);
				if (bnit == skin.boneNames.end()) {
					workAnim.AddShapeBone(shape->name.get(), usvbw.boneName);
					bnit = skin.boneNames.find(usvbw.boneName);
				}
				skin.boneWeights[bnit->second].weights[usv.index] = usvbw.w;
			}

			// ...in diff data
			for (const UndoStateVertexSliderDiff& diff : usv.diffs) {
				std::string targetDataName = activeSet[diff.sliderName].TargetDataName(target);
				if (targetDataName.empty()) {
					targetDataName = target + diff.sliderName;

					if (IsBaseShape(shape))
						baseDiffData.AddEmptySet(targetDataName, target);
					else
						morpher.AddEmptySet(shape->name.get(), diff.sliderName);
				}

				std::unordered_map<uint16_t, Vector3>* diffSet;
				if (IsBaseShape(shape))
					diffSet = baseDiffData.GetDiffSet(targetDataName);
				else
					diffSet = morpher.GetDiffSet(targetDataName);

				if (!diffSet) // should be impossible
					continue;

				(*diffSet)[usv.index] = diff.diff;
			}
		}

		makeLocal = true;
	}

	if (makeLocal) {
		for (size_t i = 0; i < activeSet.size(); i++) {
			std::string targetData = activeSet[i].TargetDataName(target);

			if (!targetData.empty())
				activeSet[i].SetLocalData(targetData);
		}
	}

	if (!addTris.empty()) {
		// Insert new triangle indices
		std::vector<uint32_t> insTriInds(addTris.size());
		for (uint32_t di = 0; di < static_cast<uint32_t>(addTris.size()); ++di)
			insTriInds[di] = addTris[di].index;

		InsertVectorIndices(tris, insTriInds);

		if (gotsegs || gotparts)
			InsertVectorIndices(triParts, insTriInds);

		// Store triangle data
		for (const UndoStateTriangle& ust : addTris) {
			tris[ust.index] = ust.t;
			if (gotsegs || gotparts)
				triParts[ust.index] = ust.partID;
		}
	}

	// Put data back in nif
	workNif.SetVertsForShape(shape, verts);
	if (uvsp)
		workNif.SetUvsForShape(shape, uvs);
	if (colorsp)
		workNif.SetColorsForShape(shape, colors);
	if (normalsp)
		workNif.SetNormalsForShape(shape, normals);
	if (tangentsp)
		workNif.SetTangentsForShape(shape, tangents);
	if (bitangentsp)
		workNif.SetBitangentsForShape(shape, bitangents);
	if (eyeDatap)
		workNif.SetEyeDataForShape(shape, eyeData);

	shape->SetTriangles(tris);

	if (gotsegs)
		workNif.SetShapeSegments(shape, inf, triParts);

	if (gotparts) {
		workNif.SetShapePartitions(shape, partitionInfo, triParts);
		workNif.RemoveEmptyPartitions(shape);
	}

	// Note that we do not restore the nif's vertex bone weights.
	// That should happen when the file is saved.
}

bool OutfitProject::PrepareCollapseVertex(NiShape* shape, UndoStateShape& uss, const std::vector<uint16_t>& indices) {
	// Get triangle data
	uint16_t numVerts = shape->GetNumVertices();

	std::vector<Triangle> tris;
	shape->GetTriangles(tris);

	std::vector<int> triParts;
	NifSegmentationInfo inf;

	if (!workNif.GetShapeSegments(shape, inf, triParts)) {
		NiVector<BSDismemberSkinInstance::PartitionInfo> partitionInfo;
		workNif.GetShapePartitions(shape, partitionInfo, triParts);
	}

	// Note that indices is already in sorted order.
	std::vector<uint16_t> nverts;
	std::vector<uint32_t> ntris;
	auto addneighbor = [&nverts](const uint16_t vi) {
		for (const auto evi : nverts)
			if (vi == evi)
				return;

		nverts.push_back(vi);
	};

	std::vector<uint32_t> nrtris; // non-replaced triangles

	for (const auto vi : indices) {
		ntris.clear();
		nverts.clear();

		// Collect lists of neighboring triangles and vertices.
		for (uint32_t ti = 0; ti < static_cast<uint32_t>(tris.size()); ++ti) {
			const Triangle& t = tris[ti];
			if (!t.HasVertex(vi))
				continue;

			if (ntris.size() >= 3)
				return false;

			ntris.push_back(ti);

			// Order of vertex collection is important, or we'll end up
			// with a backwards triangle.
			if (t.p1 == vi) {
				addneighbor(t.p2);
				addneighbor(t.p3);
			}
			else if (t.p2 == vi) {
				addneighbor(t.p3);
				addneighbor(t.p1);
			}
			else {
				addneighbor(t.p1);
				addneighbor(t.p2);
			}

			if (nverts.size() > 3)
				return false;
		}

		// Make sure no neighboring vertices are welded to this vertex.
		for (uint16_t nvi = 0; nvi < static_cast<uint16_t>(nverts.size()); ++nvi)
			for (uint16_t ii = 0; ii < static_cast<uint16_t>(indices.size()); ++ii)
				if (indices[ii] == nverts[nvi])
					return false;

		// Put triangles to delete in uss.delTris.
		for (uint32_t vti = 0; vti < static_cast<uint32_t>(ntris.size()); ++vti) {
			uint32_t ti = ntris[vti];
			uss.delTris.push_back(UndoStateTriangle{ti, tris[ti], ti < triParts.size() ? triParts[ti] : -1});
		}

		uint32_t pti = NIF_NPOS;
		if (nverts.size() == 3) {
			// Determine preferred triangle to replace.
			pti = 0;

			if (!triParts.empty() && ntris.size() == 3 && triParts[ntris[0]] != triParts[ntris[1]] && triParts[ntris[1]] == triParts[ntris[2]]) {
				pti = 1;
			}

			// Put new triangle in uss.
			uint32_t ti = ntris[pti];
			uss.addTris.push_back(UndoStateTriangle{ti, Triangle(nverts[0], nverts[1], nverts[2]), ti < triParts.size() ? triParts[ti] : -1});
		}

		// Collect list of non-replaced triangles for triangle renumbering.
		for (uint32_t vti = 0; vti < static_cast<uint32_t>(ntris.size()); ++vti)
			if (vti != pti)
				nrtris.push_back(ntris[vti]);
	}

	// We've loaded all the triangle data; now load the vertex data.
	CollectVertexData(shape, uss, indices);

	// Renumber vertex and triangle indices in uss.addTris.
	std::vector<int> vCollapse = GenerateIndexCollapseMap(indices, numVerts);

	std::sort(nrtris.begin(), nrtris.end());
	std::vector<int> tCollapse = GenerateIndexCollapseMap(nrtris, tris.size());

	for (UndoStateTriangle& ust : uss.addTris) {
		ust.t.p1 = vCollapse[ust.t.p1];
		ust.t.p2 = vCollapse[ust.t.p2];
		ust.t.p3 = vCollapse[ust.t.p3];
		ust.index = tCollapse[ust.index];
	}

	// Sort uss.addTris and uss.delTris, since they weren't added in any
	// particular order.
	std::sort(uss.addTris.begin(), uss.addTris.end());
	std::sort(uss.delTris.begin(), uss.delTris.end());

	return true;
}

static uint16_t TriangleOppositeVertex(const Triangle& t, const uint16_t p1) {
	if (t.p1 == p1)
		return t.p3;
	else if (t.p2 == p1)
		return t.p1;
	else
		return t.p2;
}

bool OutfitProject::PrepareFlipEdge(NiShape* shape, UndoStateShape& uss, const Edge& edge) {
	// Get triangle data
	std::vector<Triangle> tris;
	shape->GetTriangles(tris);

	std::vector<int> triParts;
	NifSegmentationInfo inf;
	if (!workNif.GetShapeSegments(shape, inf, triParts)) {
		NiVector<BSDismemberSkinInstance::PartitionInfo> partitionInfo;
		workNif.GetShapePartitions(shape, partitionInfo, triParts);
	}

	// Find the two neighboring triangles
	bool t1found = false;
	bool t2found = false;
	uint32_t t1 = 0;
	uint32_t t2 = 0;
	const Edge redge(edge.p2, edge.p1);

	for (uint32_t ti = 0; ti < static_cast<uint32_t>(tris.size()); ++ti) {
		if (tris[ti].HasOrientedEdge(edge)) {
			if (t1found)
				return false;

			t1 = ti;
			t1found = true;
		}

		if (tris[ti].HasOrientedEdge(redge)) {
			if (t2found)
				return false;

			t2 = ti;
			t2found = true;
		}
	}

	if (!t1found || !t2found)
		return false;

	// Find the non-edge vertex for each neighboring triangle.
	uint16_t nev1 = TriangleOppositeVertex(tris[t1], edge.p1);
	uint16_t nev2 = TriangleOppositeVertex(tris[t2], edge.p2);

	// Put data into uss.
	int tp1 = t1 < triParts.size() ? triParts[t1] : -1;
	int tp2 = t2 < triParts.size() ? triParts[t2] : -1;
	uss.delTris.push_back(UndoStateTriangle{t1, tris[t1], tp1});
	uss.delTris.push_back(UndoStateTriangle{t2, tris[t2], tp2});
	uss.addTris.push_back(UndoStateTriangle{t1, Triangle(edge.p1, nev2, nev1), tp1});
	uss.addTris.push_back(UndoStateTriangle{t2, Triangle(edge.p2, nev1, nev2), tp2});

	// Sort delTris and addTris by index.
	if (t2 < t1) {
		std::swap(uss.delTris[0], uss.delTris[1]);
		std::swap(uss.addTris[0], uss.addTris[1]);
	}

	return true;
}

/* Ideally, PrepareRefineMesh would take a list of edges, not a list of
vertices.  But OutfitStudio doesn't yet have an edge-mask tool. */
bool OutfitProject::PrepareRefineMesh(NiShape* shape, UndoStateShape& uss, std::vector<bool>& pincs, const Mesh::WeldVertsType& weldVerts) {
	// Get vertex coordinates and triangle data
	size_t nverts = pincs.size();
	const std::vector<Vector3>* verts = workNif.GetVertsForShape(shape);
	if (!verts || verts->size() != nverts)
		return false;	// Shouldn't be possible

	std::vector<Triangle> tris;
	shape->GetTriangles(tris);

	std::vector<int> triParts;
	NifSegmentationInfo inf;
	if (!workNif.GetShapeSegments(shape, inf, triParts)) {
		NiVector<BSDismemberSkinInstance::PartitionInfo> partitionInfo;
		workNif.GetShapePartitions(shape, partitionInfo, triParts);
	}

	// Ensure welded vertices are included in pincs
	for (size_t vi = 0; vi < nverts; ++vi) {
		if (!pincs[vi])
			continue;
		Mesh::DoForEachWeldedVertex(weldVerts, vi, [&](int p){pincs[p] = true;});
	}

	// Get data for all the vertices (in a secondary data-collection
	// UndoStateShape).
	UndoStateShape duss;
	std::vector<uint16_t> vertexInds, dussInds(nverts);
	for (size_t vi = 0; vi < nverts; ++vi)
		if (pincs[vi]) {
			dussInds[vi] = vertexInds.size();
			vertexInds.push_back(vi);
		}

	CollectVertexData(shape, duss, vertexInds);

	// Calculate the normal at each point by averaging triangle normals.
	std::vector<Vector3> nsums(nverts);
	for (uint16_t vi : vertexInds) {
		Vector3 nsum;
		for (const Triangle& t : tris)
			if (t.HasVertex(vi)) {
				Vector3 tn = t.trinormal(*verts);
				tn.Normalize();
				nsum += tn;
			}

		nsums[vi] = nsum;
	}
	std::vector<Vector3> normals(nverts);
	for (uint16_t vi : vertexInds) {
		Vector3 normal(nsums[vi]);
		Mesh::DoForEachWeldedVertex(weldVerts, vi, [&](int p){normal += nsums[p];});
		normal.Normalize();
		normals[vi] = normal;
	}

	// Search through the list of triangles for edges to include.
	// The boolean in singleEdges indicates whether the edge has been
	// split yet (in the following big loop).
	std::unordered_map<Edge, bool> singleEdges;
	for (size_t ti = 0; ti < tris.size(); ++ti) {
		if (tris[ti].p1 >= nverts || tris[ti].p2 >= nverts ||
			tris[ti].p3 >= nverts)
			return false;	// Out-of-range indices; shouldn't be possible.
		for (int tei = 0; tei < 3; ++tei) {
			Edge e = tris[ti].GetEdge(tei);
			if (!pincs[e.p1] || !pincs[e.p2])
				continue;
			if (singleEdges.find(e) != singleEdges.end())
				return false;	// Multiple triangles for this edge
			singleEdges[e] = false;
		}
	}

	uint16_t newvi = nverts;

	// emls contains the data that will be needed for updating the
	// triangulation.  We fill it in in the big edge loop, sort it, and
	// then update the triangles (far below).
	struct EdgeWithMidAndLen: public Edge {
		uint16_t mid;
		float len;

		bool operator<(const EdgeWithMidAndLen &e2) {
			return e2.len < len;
		}
	};
	std::vector<EdgeWithMidAndLen> emls;

	// begin big loop through the edges
	std::vector<uint16_t> p1s, p2s;
	for (auto& e : singleEdges) {
		if (e.second) continue;
		e.second = true;

		Edge edge(e.first), redge;
		bool hasre = false;

		// Collect lists of welded vertices for the edge's two points.
		Mesh::GetWeldSet(weldVerts, edge.p1, p1s);
		Mesh::GetWeldSet(weldVerts, edge.p2, p2s);

		// Search for edge and reverse-edge matches in singleEdges
		for (const auto p1 : p1s) {
			for (const auto p2 : p2s) {
				auto seit = singleEdges.find(Edge(p1, p2));
				if (seit != singleEdges.end() && !seit->second)
					return false;	// Multiple triangles for this edge
				seit = singleEdges.find(Edge(p2, p1));
				if (seit == singleEdges.end())
					continue;
				if (seit->second)
					return false;	// Shouldn't be possible
				seit->second = true;
				if (hasre)
					return false;	// Multiple reverse edges for this edge
				hasre = true;
				redge = Edge(p2, p1);
			}
		}

		// Find the edge and redge vertex data
		const UndoStateVertex& p1d = duss.delVerts[dussInds[edge.p1]];
		const UndoStateVertex& p2d = duss.delVerts[dussInds[edge.p2]];
		const UndoStateVertex& rp1d = duss.delVerts[dussInds[redge.p1]];
		const UndoStateVertex& rp2d = duss.delVerts[dussInds[redge.p2]];
		const Vector3& p1 = p1d.pos;
		const Vector3& p2 = p2d.pos;
		const Vector3& np1 = normals[edge.p1];
		const Vector3& np2 = normals[edge.p2];

		bool welded = hasre && (redge.p1 != edge.p2 || redge.p2 != edge.p1);

		// Create entry for new vertex and (if welded) new welded vertex
		uss.addVerts.emplace_back();
		uss.addVerts.back().index = newvi++;
		if (welded) {
			uss.addVerts.emplace_back();
			uss.addVerts.back().index = newvi++;
		}
		UndoStateVertex& usv = uss.addVerts[uss.addVerts.size() - (welded ? 2 : 1)];
		UndoStateVertex& rusv = uss.addVerts.back();

		/* Now comes the hard part: determining a good location for the
		new vertex.  Clearly it should lie somewhere on the plane bisecting
		the segment between p1 and p2.  If we wanted to be lazy, we could
		just pick the midpoint.  But that would require the user to adjust
		the position every time.  Better would be if we could fit a circle
		through p1, p2, and the new point so that all three have normals
		perpendicular to the circle.  But there's no guarantee that that's
		possible: the old normals could have different angles to the edge;
		and they could even be non-coplanar.  So we need to average the
		angles for the two normals somehow.  There are several stages
		where this could be done; I choose to do it early. */

		// Starting position of new point: midpoint between p1 and p2
		usv.pos = (p1 + p2) * 0.5f;

		Vector3 u12 = p2 - p1;	   // unit vector from p1 to p2 (along the edge)
		float elen = u12.length(); // edge length
		u12.Normalize();

		// Working normal for new point: average of np1 and np2, made
		// perpendicular to u12.  (If you want something fancy for usv.normal,
		// calculate it later.  This calculation needs to be done this
		// way for the circle fitter.)
		usv.normal = np1 + np2;
		usv.normal -= u12 * u12.dot(usv.normal);
		usv.normal.Normalize();

		// Now, the angle between npi and usv.normal, in the plane of
		// npi and u12 (since usv.normal isn't necessarily in that plane)
		// would be asin(u12.dot(npi)).  We want to average this for np1
		// and np2 and carefully preserve the sign.
		float angle = asin(u12.dot(np2 - np1) * 0.5);
		// Now, "angle" is the desired circle angle between the new point and
		// either p1 or p2.  It's positive for convex, negative for concave.
		// To figure out how far off of the edge we need to go, we need to
		// take the trigonometric tangent of the correct angle.  It turns out
		// the correct angle is the angle we just calculated divided by 2.
		float curveOffsetFactor = tan(angle * 0.5);
		// Now apply the offset to the new point.  (curveOffsetFactor is
		// positive for convex, negative for concave, and will be no larger
		// than 1.)
		usv.pos += usv.normal * (curveOffsetFactor * elen * 0.5f);

		// Calculate uv, color, tangent, bitangent, and eyeData by averaging.
		// We need to make sure normal, tangent, and bitangent are
		// perpendicular.
		usv.uv = (p1d.uv + p2d.uv) * 0.5;
		usv.color.r = (p1d.color.r + p2d.color.r) * 0.5f;
		usv.color.g = (p1d.color.g + p2d.color.g) * 0.5f;
		usv.color.b = (p1d.color.b + p2d.color.b) * 0.5f;
		usv.color.a = (p1d.color.a + p2d.color.a) * 0.5f;
		usv.eyeData = (p1d.eyeData + p2d.eyeData) * 0.5f;
		usv.mask = std::min(p1d.mask, p2d.mask);
		usv.tangent = (p1d.tangent + p2d.tangent) * 0.5f;
		usv.bitangent = (p1d.bitangent + p2d.bitangent) * 0.5f;
		usv.tangent -= usv.normal * usv.normal.dot(usv.tangent);
		usv.tangent.Normalize();
		usv.bitangent -= usv.normal * usv.normal.dot(usv.bitangent);
		usv.bitangent -= usv.tangent * usv.tangent.dot(usv.bitangent);
		usv.bitangent.Normalize();

		// Calculate weights by averaging.  Note that the resulting weights
		// will add up to 1 if each source's weights do.
		for (int si = 0; si < 2; ++si) {
			const std::vector<UndoStateVertexBoneWeight>& swts = si ? p2d.weights : p1d.weights;
			for (const UndoStateVertexBoneWeight& sw : swts) {
				bool found = false;
				for (size_t dwi = 0; dwi < usv.weights.size() && !found; ++dwi) {
					if (usv.weights[dwi].boneName == sw.boneName) {
						found = true;
						usv.weights[dwi].w += sw.w;
					}
				}

				if (!found)
					usv.weights.push_back(sw);
			}
		}

		for (auto& dw : usv.weights)
			dw.w *= 0.5;

		if (welded) {
			// Welded vertex gets exactly the same data except uv and uv-diffs.
			uint16_t rvi = rusv.index;
			rusv = usv;
			rusv.index = rvi;
			rusv.uv = (rp1d.uv + rp2d.uv) * 0.5;
		}

		// Unfortunately, we can't just calculate diffs by averaging.  p1
		// and p2 may have moved farther apart, which would require greater
		// curve offset.

		// diffpairs: key is sliderName.
		std::unordered_map<std::string, std::pair<Vector3, Vector3>> diffpairs;
		for (auto& sd : p1d.diffs)
			diffpairs[sd.sliderName].first = sd.diff;
		for (auto& sd : p2d.diffs)
			diffpairs[sd.sliderName].second = sd.diff;

		for (auto& dp : diffpairs) {
			// First, just average the diffs.
			Vector3 diff = (dp.second.first + dp.second.second) * 0.5f;
			const SliderData& sd = activeSet[dp.first];
			if (!sd.bUV && !sd.bClamp && !sd.bZap) {
				// Calculate the distance between the moved p1 and p2.
				float delen = (dp.second.second + p2 - dp.second.first - p1).length();
				// Apply more curve offset (for delen > elen)
				// or less (for delen < elen).
				diff += usv.normal * (curveOffsetFactor * (delen - elen) * 0.5f);
			}

			usv.diffs.push_back(UndoStateVertexSliderDiff{dp.first, diff});
			if (welded && !sd.bUV)
				rusv.diffs.push_back(UndoStateVertexSliderDiff{dp.first, diff});
		}

		if (welded) {
			// Repeat the diff calculation, but only for UV diffs.
			std::unordered_map<std::string, std::pair<Vector3, Vector3>> rdiffpairs;
			for (auto& sd : rp1d.diffs)
				rdiffpairs[sd.sliderName].first = sd.diff;
			for (auto& sd : rp2d.diffs)
				rdiffpairs[sd.sliderName].second = sd.diff;

			for (auto& dp : rdiffpairs) {
				const SliderData& sd = activeSet[dp.first];
				if (!sd.bUV)
					continue;

				Vector3 diff = (dp.second.first + dp.second.second) * 0.5f;
				rusv.diffs.push_back(UndoStateVertexSliderDiff{dp.first, diff});
			}
		}

		// Now usv and rusv are finished.  Add the edge and reverse edge to
		// the list of edges that need triangle updates.
		emls.emplace_back(EdgeWithMidAndLen{edge, usv.index, elen});
		if (hasre)
			emls.emplace_back(EdgeWithMidAndLen{redge, rusv.index, elen});
	}
	// end big loop through the edges

	// Sort the edges from longest to shortest so that we'll split the
	// triangles for the longest edges first.
	std::sort(emls.begin(), emls.end());

	for (auto& eml : emls) {
		/* We know (because we've already checked) that there is
		exactly one triangle in the old triangle list with the oriented
		edge eml.  However, it might have already been split into two
		or more triangles.  So we have to look in the new triangle
		list and then the old.  We also don't know ahead of time
		how many times an original triangle will be split in two,
		so we can't know the new triangle numbering until the end.
		So we assign each triangle of the pair created the same index
		as the original, for now, and we'll update those indices later,
		after the new-triangle list has been sorted. */
		bool found = false;

		// Loop through the new triangles
		for (auto& ust : uss.addTris) {
			if (!ust.t.HasOrientedEdge(eml))
				continue;
			found = true;
			uint16_t ovi = TriangleOppositeVertex(ust.t, eml.p1);
			ust.t.p1 = eml.p1;
			ust.t.p2 = eml.mid;
			ust.t.p3 = ovi;
			UndoStateTriangle newust = ust; // copies index and partID too
			newust.t.p1 = eml.mid;
			newust.t.p2 = eml.p2;
			uss.addTris.push_back(newust);
			break;
		}
		if (found)
			continue;

		// Loop through the old triangles
		for (size_t ti = 0; ti < tris.size(); ++ti) {
			if (!tris[ti].HasOrientedEdge(eml))
				continue;
			found = true;
			uint16_t ovi = TriangleOppositeVertex(tris[ti], eml.p1);
			int tp = ti < triParts.size() ? triParts[ti] : -1;
			uint32_t sti = static_cast<uint32_t>(ti);
			uss.delTris.push_back(UndoStateTriangle{sti, tris[ti], tp});
			uss.addTris.push_back(UndoStateTriangle{sti, Triangle(eml.p1, eml.mid, ovi), tp});
			uss.addTris.push_back(UndoStateTriangle{sti, Triangle(eml.mid, eml.p2, ovi), tp});
			break;
		}
		// assert(found)
	}

	// Sort delTris and addTris by index.
	std::sort(uss.delTris.begin(), uss.delTris.end());
	std::sort(uss.addTris.begin(), uss.addTris.end());

	// Update the triangle indices in addTris so indices are not duplicated
	uint32_t tioffset = 0, lastindex = 0;
	for (size_t ati = 0; ati < uss.addTris.size(); ++ati) {
		uint32_t& ti = uss.addTris[ati].index;
		if (ati != 0 && ti == lastindex)
			++tioffset;
		lastindex = ti;
		ti += tioffset;
	}

	return true;
}

bool OutfitProject::IsVertexOnBoundary(NiShape* shape, int vi) {
	std::vector<Triangle> tris;
	shape->GetTriangles(tris);

	// Count how many times each neighboring vertex is in a triangle with
	// this vertex
	std::unordered_map<uint16_t, int> vcounts;
	for (const Triangle &t : tris) {
		if (!t.HasVertex(vi))
			continue;

		if (t.p1 != vi) vcounts[t.p1]++;
		if (t.p2 != vi) vcounts[t.p2]++;
		if (t.p3 != vi) vcounts[t.p3]++;
	}

	// If any vertex count is not even, the corresponding edge is on the
	// boundary.  (Note that if any vc is greater than 2, the surface is
	// messed up.)
	for (auto& vc : vcounts)
		if (vc.second % 2 != 0)
			return true;

	return false;
}

bool OutfitProject::PointsHaveDifferingWeightsOrDiffs(NiShape* shape1, int p1, NiShape* shape2, int p2) {
	// Check for position differences
	const AnimSkin& skin1 = shape1->IsSkinned() ? workAnim.shapeSkinning[shape1->name.get()] : AnimSkin();
	const AnimSkin& skin2 = shape2->IsSkinned() ? workAnim.shapeSkinning[shape2->name.get()] : AnimSkin();
	MatTransform skin1ToGlobal = workAnim.GetTransformShapeToGlobal(shape1);
	MatTransform skin2ToGlobal = workAnim.GetTransformShapeToGlobal(shape2);
	const std::vector<Vector3>* verts1 = workNif.GetVertsForShape(shape1);
	const std::vector<Vector3>* verts2 = workNif.GetVertsForShape(shape2);
	if (verts1 && verts2 && (*verts1)[p1] != (*verts2)[p2])
		return true;

	// Check for bone weight differences
	for (auto bnp : skin1.boneNames) {
		const AnimWeight& aw1 = skin1.boneWeights.at(bnp.second);
		auto wit1 = aw1.weights.find(p1);
		if (wit1 == aw1.weights.end() || wit1->second == 0.0f)
			continue;
		auto bnit = skin2.boneNames.find(bnp.first);
		if (bnit == skin2.boneNames.end())
			return true;
		const AnimWeight& aw2 = skin2.boneWeights.at(bnit->second);
		auto wit2 = aw2.weights.find(p2);
		if (wit2 == aw2.weights.end() || wit2->second != wit1->second)
			return true;
	}
	for (auto bnp : skin2.boneNames) {
		const AnimWeight& aw2 = skin2.boneWeights.at(bnp.second);
		auto wit2 = aw2.weights.find(p2);
		if (wit2 == aw2.weights.end() || wit2->second == 0.0f)
			continue;
		auto bnit = skin1.boneNames.find(bnp.first);
		if (bnit == skin1.boneNames.end())
			return true;
		const AnimWeight& aw1 = skin1.boneWeights.at(bnit->second);
		auto wit1 = aw1.weights.find(p1);
		if (wit1 == aw1.weights.end() || wit1->second != wit2->second)
			return true;
	}

	// Check for position slider differences.  We skip uv, clamp, and zap.
	for (size_t si = 0; si < activeSet.size(); ++si) {
		if (activeSet[si].bUV || activeSet[si].bClamp || activeSet[si].bZap)
			continue;

		std::unordered_map<uint16_t, Vector3>* diffSet1 = GetDiffSet(activeSet[si], shape1);
		std::unordered_map<uint16_t, Vector3>* diffSet2 = GetDiffSet(activeSet[si], shape2);

		if (!diffSet1 && !diffSet2)
			continue;

		Vector3 diff1, diff2;
		if (diffSet1) {
			auto dit1 = diffSet1->find(p1);
			if (dit1 != diffSet1->end())
				diff1 = skin1ToGlobal.ApplyTransformToDiff(dit1->second);
		}
		if (diffSet2) {
			auto dit2 = diffSet2->find(p2);
			if (dit2 != diffSet2->end())
				diff2 = skin2ToGlobal.ApplyTransformToDiff(dit2->second);
		}
		if (diff1 != diff2)
			return true;
	}

	return false;
}

void OutfitProject::PrepareMergeVertex(NiShape* shape, UndoStateShape& uss, int selVert, int targVert) {
	// Get triangle data
	uint16_t numVerts = shape->GetNumVertices();

	std::vector<Triangle> tris;
	shape->GetTriangles(tris);

	std::vector<int> triParts;
	NifSegmentationInfo inf;

	if (!workNif.GetShapeSegments(shape, inf, triParts)) {
		NiVector<BSDismemberSkinInstance::PartitionInfo> partitionInfo;
		workNif.GetShapePartitions(shape, partitionInfo, triParts);
	}

	// Delete vertex
	std::vector<uint16_t> indices(1, selVert);
	CollectVertexData(shape, uss, indices);
	std::vector<int> vCollapse = GenerateIndexCollapseMap(indices, numVerts);

	// Replace triangles
	for (uint32_t ti = 0; ti < tris.size(); ++ti) {
		const Triangle& t = tris[ti];
		if (!t.HasVertex(selVert))
			continue;

		// Replace selVert with targVert
		Triangle newt = t;
		if (newt.p1 == selVert)
			newt.p1 = targVert;
		if (newt.p2 == selVert)
			newt.p2 = targVert;
		if (newt.p3 == selVert)
			newt.p3 = targVert;

		// Get partition number
		int part = -1;
		if (ti < triParts.size())
			part = triParts[ti];

		// Collapse vertex indices
		newt.p1 = vCollapse[newt.p1];
		newt.p2 = vCollapse[newt.p2];
		newt.p3 = vCollapse[newt.p3];

		uss.delTris.push_back(UndoStateTriangle{ti, t, part});
		uss.addTris.push_back(UndoStateTriangle{ti, newt, part});
	}
}

void OutfitProject::PrepareWeldVertex(NiShape* shape, UndoStateShape& uss, int selVert, int targVert, NiShape* targShape) {
	// Get triangle data
	std::vector<Triangle> tris;
	shape->GetTriangles(tris);

	std::vector<int> triParts;
	NifSegmentationInfo inf;

	if (!workNif.GetShapeSegments(shape, inf, triParts)) {
		NiVector<BSDismemberSkinInstance::PartitionInfo> partitionInfo;
		workNif.GetShapePartitions(shape, partitionInfo, triParts);
	}

	// Delete and restore all triangles that use selVert
	for (uint32_t ti = 0; ti < tris.size(); ++ti) {
		const Triangle& t = tris[ti];
		if (!t.HasVertex(selVert))
			continue;

		// Get partition number
		int part = -1;
		if (ti < triParts.size())
			part = triParts[ti];

		uss.delTris.push_back(UndoStateTriangle{ti, t, part});
		uss.addTris.push_back(UndoStateTriangle{ti, t, part});
	}

	// Get data for the target vertex (in a secondary data-collection
	// UndoStateShape).
	UndoStateShape duss;
	std::vector<uint16_t> targInds(1, targVert);
	CollectVertexData(targShape, duss, targInds);
	UndoStateVertex &tusv = duss.delVerts[0];

	// Calculate transform from target's skin coordinates to selected's.
	MatTransform skx = workAnim.GetTransformGlobalToShape(shape).ComposeTransforms(workAnim.GetTransformShapeToGlobal(targShape));

	// Delete vertex
	std::vector<uint16_t> selInds(1, selVert);
	CollectVertexData(shape, uss, selInds);
	UndoStateVertex &susv = uss.delVerts.back();

	// Build replacement vertex from a mix of selected and target vertex data
	uss.addVerts.emplace_back();
	UndoStateVertex& usv = uss.addVerts.back();
	usv.index = selVert;
	usv.pos = skx.ApplyTransform(tusv.pos);
	usv.uv = susv.uv;
	usv.color = tusv.color;
	usv.normal = skx.ApplyTransformToDir(tusv.normal);
	usv.tangent = skx.ApplyTransformToDir(tusv.tangent);
	usv.bitangent = skx.ApplyTransformToDir(tusv.bitangent);
	usv.eyeData = tusv.eyeData;
	usv.mask = tusv.mask;
	usv.weights = std::move(tusv.weights);

	// UV, clamp, and zap diffs come from selected vertex; position diffs
	// come from target
	for (const auto& diff : susv.diffs) {
		const SliderData& sd = activeSet[diff.sliderName];
		if (sd.bUV || sd.bClamp || sd.bZap)
			usv.diffs.push_back(diff);
	}
	for (const auto& diff : tusv.diffs) {
		const SliderData& sd = activeSet[diff.sliderName];
		if (!sd.bUV && !sd.bClamp && !sd.bZap) {
			Vector3 d = skx.ApplyTransformToDiff(diff.diff);
			usv.diffs.push_back(UndoStateVertexSliderDiff{diff.sliderName, d});
		}
	}
}

void OutfitProject::CheckMerge(const std::string& sourceName, const std::string& targetName, MergeCheckErrors& e) {
	if (sourceName == targetName) {
		e.shapesSame = true;
		return;
	}

	NiShape* source = workNif.FindBlockByName<NiShape>(sourceName);
	if (!source)
		return;
	NiShape* target = workNif.FindBlockByName<NiShape>(targetName);
	if (!target)
		return;

	constexpr size_t maxVertIndex = std::numeric_limits<uint16_t>().max();
	size_t maxTriIndex = std::numeric_limits<uint16_t>().max();
	if (workNif.GetHeader().GetVersion().IsFO4() || workNif.GetHeader().GetVersion().IsFO76())
		maxTriIndex = std::numeric_limits<uint32_t>().max();

	size_t snVerts = source->GetNumVertices();
	size_t tnVerts = target->GetNumVertices();
	if (snVerts + tnVerts > maxVertIndex)
		e.tooManyVertices = true;

	size_t snTris = source->GetNumTriangles();
	size_t tnTris = target->GetNumTriangles();
	if (snTris + tnTris > maxTriIndex)
		e.tooManyTriangles = true;

	std::vector<int> triParts;
	NifSegmentationInfo sinf, tinf;
	bool gotssegs = workNif.GetShapeSegments(source, sinf, triParts);
	bool gottsegs = workNif.GetShapeSegments(target, tinf, triParts);
	if (gotssegs != gottsegs) {
		// Shape with segments and the other without
		e.segmentsMismatch = true;
	}
	else if (gotssegs) {
		// Both shapes have segments
		if (sinf.ssfFile != tinf.ssfFile) {
			// Segment definition file path differs
			e.segmentsMismatch = true;
		}

		if (sinf.segs.size() != tinf.segs.size()) {
			// Shapes have different amount of segments
			e.segmentsMismatch = true;
		}

		for (size_t si = 0; !e.segmentsMismatch && si < sinf.segs.size(); ++si) {
			if (sinf.segs[si].subs.size() != tinf.segs[si].subs.size()) {
				// Shapes have different amount of sub segments
				e.segmentsMismatch = true;
			}

			for (size_t ssi = 0; !e.segmentsMismatch && ssi < sinf.segs[si].subs.size(); ++ssi) {
				const NifSubSegmentInfo& sssinf = sinf.segs[si].subs[ssi];
				const NifSubSegmentInfo& tssinf = tinf.segs[si].subs[ssi];
				if (sssinf.userSlotID != tssinf.userSlotID || sssinf.material != tssinf.material || sssinf.extraData != tssinf.extraData) {
					// Sub segment information differs
					e.segmentsMismatch = true;
				}
			}
		}
	}

	NiVector<BSDismemberSkinInstance::PartitionInfo> spinf, tpinf;
	bool gotspar = workNif.GetShapePartitions(source, spinf, triParts);
	bool gottpar = workNif.GetShapePartitions(target, tpinf, triParts);
	if (gotspar != gottpar) {
		// Shape with partitions and the other without
		e.partitionsMismatch = true;
	}
	else if (gotspar) {
		// Both shapes have partitions
		if (spinf.size() != tpinf.size()) {
			// Shapes have different amount of partitions
			e.partitionsMismatch = true;
		}

		for (uint32_t pi = 0; !e.partitionsMismatch && pi < spinf.size(); ++pi) {
			if (spinf[pi].partID != tpinf[pi].partID) {
				// Partition slot differs
				e.partitionsMismatch = true;
			}
		}
	}

	auto sShader = workNif.GetShader(source);
	auto tShader = workNif.GetShader(target);
	if ((sShader != nullptr) != (tShader != nullptr)) {
		// Shape with shader and the other without
		e.shaderMismatch = true;
	}
	else if (sShader) {
		// Both shapes have a shader
		if (sShader->GetShaderType() != tShader->GetShaderType()) {
			// Shader type differs
			e.shaderMismatch = true;
		}
		else if (workNif.GetHeader().GetVersion().IsFO4() || workNif.GetHeader().GetVersion().IsFO76()) {
			if (!StringsEqualInsens(sShader->name.get().c_str(), tShader->name.get().c_str())
				|| !StringsEqualInsens(sShader->GetWetMaterialName().c_str(), tShader->GetWetMaterialName().c_str())) {
				// Material file paths differ
				e.shaderMismatch = true;
			}
		}

		std::string sTexBase, tTexBase;
		workNif.GetTextureSlot(source, sTexBase);
		workNif.GetTextureSlot(target, tTexBase);

		if (!StringsEqualInsens(sTexBase.c_str(), tTexBase.c_str())) {
			// Base texture path differs (and possibly UV layout)
			e.textureMismatch = true;
		}
	}

	auto sAlphaProp = workNif.GetAlphaProperty(source);
	auto tAlphaProp = workNif.GetAlphaProperty(target);
	if ((sAlphaProp != nullptr) != (tAlphaProp != nullptr)) {
		// Shape with alpha property and the other without
		e.alphaPropMismatch = true;
	}
	else if (sAlphaProp) {
		// Both shapes have an alpha property
		if (sAlphaProp->flags != tAlphaProp->flags || sAlphaProp->threshold != tAlphaProp->threshold) {
			// Flags or threshold differs
			e.alphaPropMismatch = true;
		}
	}

	e.canMerge = !e.partitionsMismatch && !e.segmentsMismatch && !e.tooManyVertices && !e.tooManyTriangles && !e.shaderMismatch && !e.textureMismatch && !e.alphaPropMismatch;
}

void OutfitProject::PrepareCopyGeo(NiShape* source, NiShape* target, UndoStateShape& uss) {
	if (!source || !target)
		return;

	uint16_t snVerts = source->GetNumVertices();
	uint16_t tnVerts = target->GetNumVertices();
	uint32_t snTris = source->GetNumTriangles();
	uint32_t tnTris = target->GetNumTriangles();

	std::vector<uint16_t> vinds(snVerts);
	std::vector<uint32_t> tinds(snTris);
	for (uint16_t i = 0; i < snVerts; ++i)
		vinds[i] = i;
	for (uint32_t i = 0; i < snTris; ++i)
		tinds[i] = i;

	CollectVertexData(source, uss, vinds);
	CollectTriangleData(source, uss, tinds);

	for (uint16_t vi = 0; vi < snVerts; ++vi)
		uss.delVerts[vi].index += tnVerts;

	for (uint32_t ti = 0; ti < snTris; ++ti) {
		uss.delTris[ti].index += tnTris;
		uss.delTris[ti].t.p1 += tnVerts;
		uss.delTris[ti].t.p2 += tnVerts;
		uss.delTris[ti].t.p3 += tnVerts;
	}

	uss.delVerts.swap(uss.addVerts);
	uss.delTris.swap(uss.addTris);
}

NiShape* OutfitProject::DuplicateShape(NiShape* sourceShape, const std::string& destShapeName) {
	if (!sourceShape)
		return nullptr;

	workAnim.CloneShape(&workNif, sourceShape, destShapeName);

	auto newShape = workNif.CloneShape(sourceShape, destShapeName);

	std::string shapeName = sourceShape->name.get();
	std::string srcTarget = ShapeToTarget(shapeName);

	if (IsBaseShape(sourceShape)) {
		for (size_t i = 0; i < activeSet.size(); i++) {
			std::string srcTargetData = activeSet[i].TargetDataName(srcTarget);

			auto diff = baseDiffData.GetDiffSet(srcTargetData);
			if (diff)
				morpher.SetResultDiff(destShapeName, activeSet[i].name, *diff);
		}
	}
	else
		morpher.CopyShape(shapeName, srcTarget, destShapeName);

	return newShape;
}

void OutfitProject::DeleteShape(NiShape* shape) {
	if (!shape)
		return;

	std::string shapeName = shape->name.get();
	workAnim.ClearShape(shapeName);
	owner->glView->DeleteMesh(shapeName);
	shapeTextures.erase(shapeName);
	shapeMaterialFiles.erase(shapeName);

	if (IsBaseShape(shape)) {
		morpher.UnlinkRefDiffData();
		baseShape = nullptr;
	}

	owner->ClearSelected(shape);
	workNif.DeleteShape(shape);
}

void OutfitProject::RenameShape(NiShape* shape, const std::string& newShapeName) {
	std::string shapeName = shape->name.get();
	std::string oldTarget = ShapeToTarget(shapeName);
	workNif.RenameShape(shape, newShapeName);
	workAnim.RenameShape(shapeName, newShapeName);
	activeSet.RenameShape(shapeName, newShapeName);

	auto tex = shapeTextures.find(shapeName);
	if (tex != shapeTextures.end()) {
		auto value = tex->second;
		shapeTextures.erase(tex);
		shapeTextures[newShapeName] = value;
	}

	auto mat = shapeMaterialFiles.find(shapeName);
	if (mat != shapeMaterialFiles.end()) {
		auto value = mat->second;
		shapeMaterialFiles.erase(mat);
		shapeMaterialFiles[newShapeName] = value;
	}

	if (IsBaseShape(shape)) {
		activeSet.SetReferencedData(newShapeName, true);
		baseDiffData.DeepRename(oldTarget, newShapeName);
	}
	else
		morpher.RenameShape(shapeName, oldTarget, newShapeName);

	wxLogMessage("Renamed shape '%s' to '%s'.", shapeName, newShapeName);
}

void OutfitProject::UpdateNifNormals(NifFile* nif, const std::vector<Mesh*>& shapeMeshes) {
	std::vector<Vector3> liveNorms;
	for (auto& m : shapeMeshes) {
		auto shape = nif->FindBlockByName<NiShape>(m->shapeName);
		if (shape) {
			if (nif->GetHeader().GetVersion().IsSK() || nif->GetHeader().GetVersion().IsSSE()) {
				NiShader* shader = nif->GetShader(shape);
				if (shader && shader->IsModelSpace())
					continue;
			}

			liveNorms.clear();
			for (int i = 0; i < m->nVerts; i++)
				liveNorms.push_back(Mesh::TransformDirMeshToNif(m->norms[i]));

			nif->SetNormalsForShape(shape, liveNorms);
			nif->CalcTangentsForShape(shape);
		}
	}
}

void OutfitProject::MatchSymmetricVertices(NiShape* shape, const Mesh::WeldVertsType& weldVerts, SymmetricVertices& r) {
	// Gather shape's vertices
	std::vector<Vector3> verts;
	workNif.GetVertsForShape(shape, verts);
	int origNVerts = static_cast<int>(verts.size());

	// Dealing with welded vertices is potentially very messy.  So we
	// remove all but one vertex of each welded set from the vertex list,
	// producing a reduced vertex list.  The surviving vertex of each
	// welded set is the one with the lowest index.
	std::vector<int> indRedToOrig(origNVerts);
	int redNVerts = 0;
	for (int vi = 0; vi < origNVerts; ++vi) {
		if (Mesh::LeastWeldedVertexIndex(weldVerts, vi) == vi)
			indRedToOrig[redNVerts++] = vi;
	}

	// Delete welded vertices from our copy of the vertex list.
	for (int vi = 0; vi < redNVerts; ++vi)
		verts[vi] = verts[indRedToOrig[vi]];

	// Build proximity cache for vertices.
	// Note that kd_tree keeps pointers into verts.
	kd_tree<uint16_t> vertTree(&verts[0], redNVerts);

	struct VertData {
		// bad: no match or a bad match
		bool bad = false;
		// mi: matched vertex's index.  The value of -1 is not meaningful,
		// except it's not a valid vertex index.
		int mi = -1;
		int matchcount = 0;
	};
	std::vector<VertData> vdata(redNVerts);

	// Main loop through the reduced vertex set.
	for (int vi = 0; vi < redNVerts; ++vi) {
		// Construct mirror coordinates
		Vector3 mc = verts[vi];
		mc.x = -mc.x;

		// Match
		vertTree.kd_nn(&mc, 0);

		// mi: matched vertex's index
		int mi = vertTree.queryResult[0].vertex_index;
		vdata[vi].mi = mi;
		++vdata[mi].matchcount;
	}

	// Mark a vertex bad if two vertices matched to it, if its matched
	// vertex didn't match to it, or if its match is bad.
	for (int vi = 0; vi < redNVerts; ++vi)
		if (vdata[vi].matchcount != 1 || vdata[vdata[vi].mi].mi != vi) {
			vdata[vi].bad = true;
			vdata[vdata[vi].mi].bad = true;
		}

	// Put results in r.
	for (int vi = 0; vi < redNVerts; ++vi) {
		const VertData& vd = vdata[vi];
		if (vd.bad)
			r.unmatched.push_back(indRedToOrig[vi]);
		else if (vi <= vd.mi)
			r.matches.emplace_back(indRedToOrig[vi], indRedToOrig[vd.mi]);
	}
}

void OutfitProject::MatchSymmetricBoneNames(std::vector<std::pair<std::string, std::string>>& pairs, std::vector<std::string>& singles) {
	// Note that there is very similar code to this in OutfitStudioFrame::CalcAutoXMirrorBone.

	std::vector<std::string> bones;
	GetActiveBones(bones);

	std::vector<bool> done(bones.size(), false);
	for (size_t bi1 = 0; bi1 < bones.size(); ++bi1) {
		if (done[bi1])
			continue;
		done[bi1] = true;

		const size_t b1len = bones[bi1].length();
		int bestFlips = 0;
		size_t bestbi2 = 0;
		for (size_t bi2 = 0; bi2 < bones.size(); ++bi2) {
			if (done[bi2])
				continue;
			if (b1len != bones[bi2].length())
				continue;

			int flips = 0;
			bool nomatch = false;
			for (size_t i = 0; i < b1len && !nomatch; ++i) {
				char b1c = std::tolower(bones[bi1][i]);
				char b2c = std::tolower(bones[bi2][i]);
				if (b1c == 'l' && b2c == 'r')
					++flips;
				else if (b1c == 'r' && b2c == 'l')
					++flips;
				else if (b1c != b2c)
					nomatch = true;
			}

			if (nomatch)
				continue;
			if (flips <= bestFlips)
				continue;

			bestFlips = flips;
			bestbi2 = bi2;
		}
		if (bestFlips > 0) {
			pairs.emplace_back(bones[bi1], bones[bestbi2]);
			done[bestbi2] = true;
		}
		else
			singles.push_back(bones[bi1]);
	}
}

void OutfitProject::FindVertexAsymmetries(NiShape* shape, const SymmetricVertices& symverts, const Mesh::WeldVertsType& weldVerts, VertexAsymmetries& r) {
	// Get shape's vertices
	std::vector<Vector3> verts;
	workNif.GetVertsForShape(shape, verts);
	int nVerts = static_cast<int>(shape->GetNumVertices());

	// Create welded-vertex set for every vertex
	std::vector<std::vector<int>> weldSets(nVerts);
	for (int vi = 0; vi < nVerts; ++vi)
		Mesh::GetWeldSet(weldVerts, vi, weldSets[vi]);

	// Initialize global result arrays
	int nMatches = static_cast<int>(symverts.matches.size());
	r.positions.resize(nMatches, false);
	r.anyslider.resize(nMatches, false);
	r.anybone.resize(nMatches, false);
	r.poserr.resize(nMatches, 0.0f);

	// Find position asymmetries
	for (int mpi = 0; mpi < nMatches; ++mpi) {
		bool asym = false;
		float err = 0.0f;
		int errCount = 0;
		for (int p1 : weldSets[symverts.matches[mpi].first]) {
			Vector3 v1 = verts[p1];
			v1.x = -v1.x;
			for (int p2 : weldSets[symverts.matches[mpi].second])
				if (v1 != verts[p2]) {
					asym = true;
					err += (v1 - verts[p2]).length();
					++errCount;
				}
		}
		if (asym) {
			r.positions[mpi] = true;
			r.poserr[mpi] = err / errCount;
		}
	}

	// Find position slider asymmetries
	for (size_t si = 0; si < activeSet.size(); ++si) {
		SliderData& sd = activeSet[si];
		if (sd.bUV || sd.bClamp || sd.bZap)
			continue;

		// Get the diffs for this slider
		const std::unordered_map<uint16_t, Vector3>* diffSet = GetDiffSet(sd, shape);
		if (!diffSet)
			continue;

		// Initialize all the data we'll be collecting for this slider
		std::vector<bool> aflags(nMatches, false);
		std::vector<float> differr(nMatches, 0.0f);
		bool sliderasym = false;

		// Main loop through matches (for sliders).
		for (int mpi = 0; mpi < nMatches; ++mpi) {
			bool asym = false;
			float err = 0.0f;
			int errCount = 0;

			// Loop through weld set of match's first point
			for (int p1 : weldSets[symverts.matches[mpi].first]) {
				// Get p1's diff
				Vector3 p1diff;
				auto dit1 = diffSet->find(p1);
				if (dit1 != diffSet->end())
					p1diff = dit1->second;
				p1diff.x = -p1diff.x;

				// Loop through weld set of match's second point
				for (int p2 : weldSets[symverts.matches[mpi].second]) {
					// Get p2's diff
					Vector3 p2diff;
					auto dit2 = diffSet->find(p2);
					if (dit2 != diffSet->end())
						p2diff = dit2->second;

					// Check for asymmetry
					if (p1diff != p2diff) {
						asym = true;
						err += (p1diff - p2diff).length();
						++errCount;
					}
				}
			}

			// If this match had an asymmetry, record it
			if (asym) {
				aflags[mpi] = true;
				differr[mpi] = err / errCount;
				sliderasym = true;
				r.anyslider[mpi] = true;
			}
		}

		// If this slider had an asymmetry, store the results
		if (sliderasym) {
			r.sliders.emplace_back();
			r.sliders.back().sliderName = sd.name;
			r.sliders.back().aflags = std::move(aflags);
			r.sliders.back().differr = std::move(differr);
		}
	}

	// Get list of symmetric bone pairings and unpaired bones
	std::vector<std::pair<std::string, std::string>> bPairs;
	std::vector<std::string> bSingles;
	MatchSymmetricBoneNames(bPairs, bSingles);
	int nbPairs = static_cast<int>(bPairs.size());
	int nbSingles = static_cast<int>(bSingles.size());
	AnimSkin& skin = workAnim.shapeSkinning[shape->name.get()];

	// Find bone weight asymmetries
	std::unordered_map<uint16_t, float> dummyWeights;
	for (int bpi = 0; bpi < nbPairs + nbSingles; ++bpi) {
		// Get the names of our one or two bones
		std::string b1name, b2name;
		bool isBonePair = bpi < nbPairs;
		if (isBonePair) {
			b1name = bPairs[bpi].first;
			b2name = bPairs[bpi].second;
		}
		else {
			b1name = bSingles[bpi - nbPairs];
			b2name = b1name;
		}

		// Look up the weights for both bones
		auto bnit = skin.boneNames.find(b1name);
		const std::unordered_map<uint16_t, float>& b1w = bnit != skin.boneNames.end() ? skin.boneWeights[bnit->second].weights : dummyWeights;
		bnit = skin.boneNames.find(b2name);
		const std::unordered_map<uint16_t, float>& b2w = bnit != skin.boneNames.end() ? skin.boneWeights[bnit->second].weights : dummyWeights;

		// Initialize all the data we'll be calculating for this bone/bones.
		std::vector<bool> aflags1(nMatches, false);
		std::vector<bool> aflags2(nMatches, false);
		std::vector<float> weighterr1(nMatches, 0.0f);
		std::vector<float> weighterr2(nMatches, 0.0f);
		bool boneasym = false;

		// Main loop through matches for a bone or bone pair.
		for (int mpi = 0; mpi < nMatches; ++mpi) {
			bool asym1 = false, asym2 = false;
			float werr1 = 0.0f, werr2 = 0.0f;
			int werr1count = 0, werr2count = 0;

			// Loop through weld set for match's first point
			for (int p1 : weldSets[symverts.matches[mpi].first]) {
				// Find weights of p1 for bone 1 and bone 2
				float p1b1w = 0.0f, p1b2w = 0.0f;
				auto wit = b1w.find(p1);
				if (wit != b1w.end())
					p1b1w = wit->second;
				wit = b2w.find(p1);
				if (wit != b2w.end())
					p1b2w = wit->second;

				// Loop through weld set for match's second point
				for (int p2 : weldSets[symverts.matches[mpi].second]) {
					// Find weights of p2 for bone 1 and bone 2
					float p2b1w = 0.0f, p2b2w = 0.0f;
					wit = b1w.find(p2);
					if (wit != b1w.end())
						p2b1w = wit->second;
					wit = b2w.find(p2);
					if (wit != b2w.end())
						p2b2w = wit->second;

					// Check for asymmetry between p1's bone 1 weight and
					// p2's bone 2 weight.
					if (p1b1w != p2b2w) {
						asym1 = true;
						werr1 += std::fabs(p1b1w - p2b2w);
						++werr1count;
					}

					// Check for asymmetry between p1's bone 2 weight and
					// p2's bone 1 weight.
					if (p1b2w != p2b1w) {
						asym2 = true;
						werr2 += std::fabs(p1b2w - p2b1w);
						++werr2count;
					}
				}
			}
			// end loops through weld sets of the match's two points

			// Record results for this match for this bone or bone pair
			if (asym1) {
				aflags1[mpi] = true;
				weighterr1[mpi] = werr1 / werr1count;
			}
			if (asym2) {
				aflags2[mpi] = true;
				weighterr2[mpi] = werr2 / werr2count;
			}
			if (asym1 || asym2) {
				r.anybone[mpi] = true;
				boneasym = true;
			}
		}
		// end main loop through matches for a bone or bone pair

		// If we have any weight asymmetries for this bone or bone pair,
		// store the results.
		if (boneasym) {
			r.bones.emplace_back();
			r.bones.back().boneName = b1name;
			r.bones.back().mirroroffset = isBonePair ? 1 : 0;
			r.bones.back().aflags = std::move(aflags1);
			r.bones.back().weighterr = std::move(weighterr1);
			if (isBonePair) {
				r.bones.emplace_back();
				r.bones.back().boneName = b2name;
				r.bones.back().mirroroffset = -1;
				r.bones.back().aflags = std::move(aflags2);
				r.bones.back().weighterr = std::move(weighterr2);
			}
		}
	}
}

std::vector<bool> CalcVertexListForAsymmetryTasks(const SymmetricVertices& symverts, const VertexAsymmetries& asyms, const VertexAsymmetryTasks& tasks, int nVerts) {
	int nSliders = static_cast<int>(asyms.sliders.size());
	int nBones = static_cast<int>(asyms.bones.size());
	int nMatches = static_cast<int>(asyms.positions.size());

	std::vector<bool> doVert(nVerts, false);
	for (int mi = 0; mi < nMatches; ++mi) {
		int p1 = symverts.matches[mi].first;
		int p2 = symverts.matches[mi].second;

		// Positions
		if (tasks.doPos && asyms.positions[mi])
			doVert[p1] = doVert[p2] = true;

		// Sliders
		for (int sai = 0; sai < nSliders; ++sai)
			if (tasks.doSliders[sai] && asyms.sliders[sai].aflags[mi])
				doVert[p1] = doVert[p2] = true;

		// Bones
		for (int bi = 0; bi < nBones; ++bi) {
			if (!asyms.bones[bi].aflags[mi])
				continue;
			if (tasks.doBones[bi])
				doVert[p1] = true;
			if (tasks.doBones[bi + asyms.bones[bi].mirroroffset])
				doVert[p2] = true;
		}
	}

	if (tasks.doUnmatched)
		for (int vi : symverts.unmatched)
			doVert[vi] = true;

	return doVert;
}

void AddWeldedToVertexList(const Mesh::WeldVertsType& weldVerts, std::vector<bool>& verts) {
	for (size_t i = 0; i < verts.size(); ++i) {
		if (!verts[i])
			continue;
		Mesh::DoForEachWeldedVertex(weldVerts, i, [&](int p){verts[p] = true;});
	}
}

static int SelCount(const std::vector<bool>& sel) {
	int c = 0;
	for (size_t i = 0; i < sel.size(); ++i)
		c += sel[i];
	return c;
}

static int SelCount(const std::vector<bool>& sel, const std::vector<bool>& osel) {
	int c = 0;
	for (size_t i = 0; i < sel.size(); ++i)
		if (sel[i] && osel[i])
			++c;
	return c;
}

static float SelAvg(const std::vector<bool>& sel, const std::vector<float>& vals) {
	float sum = 0.0f;
	int c = 0;
	for (size_t i = 0; i < sel.size(); ++i)
		if (sel[i]) {
			sum += vals[i];
			++c;
		}
	return c ? sum / c : 0.0f;
}

void CalcVertexAsymmetryStats(const SymmetricVertices& symverts, const VertexAsymmetries& asyms, const std::vector<bool>& selVerts, VertexAsymmetryStats& stats) {
	int nSliders = static_cast<int>(asyms.sliders.size());
	int nBones = static_cast<int>(asyms.bones.size());
	int nMatches = static_cast<int>(asyms.positions.size());

	// Calculate which matches are not masked
	std::vector<bool> selMatches(nMatches);
	for (int mi = 0; mi < nMatches; ++mi)
		selMatches[mi] = selVerts[symverts.matches[mi].first] || selVerts[symverts.matches[mi].second];

	stats.boneCounts.resize(nBones, 0);
	stats.boneAvgs.resize(nBones, 0.0f);
	stats.sliderCounts.resize(nSliders, 0);
	stats.sliderAvgs.resize(nSliders, 0.0f);
	stats.unmaskedCount = SelCount(selVerts);
	stats.posCount = SelCount(selMatches, asyms.positions);
	stats.posAvg = SelAvg(selMatches, asyms.poserr);
	stats.anySliderCount = SelCount(selMatches, asyms.anyslider);
	stats.anyBoneCount = SelCount(selMatches, asyms.anybone);

	for (int si = 0; si < nSliders; ++si) {
		stats.sliderCounts[si] = SelCount(selMatches, asyms.sliders[si].aflags);
		stats.sliderAvgs[si] = SelAvg(selMatches, asyms.sliders[si].differr);
	}

	for (int bi = 0; bi < nBones; ++bi) {
		int bi2 = bi + asyms.bones[bi].mirroroffset;
		float sum = 0.0f;
		int count = 0;

		for (int mi = 0; mi < nMatches; ++mi) {
			int p1 = symverts.matches[mi].first;
			int p2 = symverts.matches[mi].second;
			if (!selVerts[p1] && !selVerts[p2])
				continue;

			// b1 aflags says: p1 b1 mismatches p2 b2
			// b2 aflags says: p1 b2 mismatches p2 b1
			// We need to sum the data for b1; that is, the first and fourth
			// cases.
			if (selVerts[p1] && asyms.bones[bi].aflags[mi]) {
				sum += asyms.bones[bi].weighterr[mi];
				++count;
			}
			if (p1 != p2 && selVerts[p2] && asyms.bones[bi2].aflags[mi]) {
				sum += asyms.bones[bi2].weighterr[mi];
				++count;
			}
		}

		stats.boneCounts[bi] = count;
		if (count)
			stats.boneAvgs[bi] = sum / count;
	}
}

static Vector3 GetUSliderDiff(const UndoStateVertex& usv, const std::string& name) {
	for (const auto& sd : usv.diffs)
		if (sd.sliderName == name)
			return sd.diff;
	return Vector3();
}

static void SetUSliderDiff(UndoStateVertex& usv, const std::string& name, const Vector3& diff) {
	for (auto& sd : usv.diffs)
		if (sd.sliderName == name) {
			sd.diff = diff;
			return;
		}
	usv.diffs.push_back(UndoStateVertexSliderDiff{name, diff});
}

static float GetUWeight(const UndoStateVertex& usv, const std::string& name) {
	for (const auto& bw : usv.weights)
		if (bw.boneName == name)
			return bw.w;
	return 0.0f;
}

// The two versions of SetUWeight are for storing it in uss.boneWeights
// (first version) or usv.weights (second version).  The first is needed
// by the normalizer; the second is the final location.
static void SetUWeight(UndoStateShape& uss, int p, const std::string& bn, float w) {
	for (auto& bw : uss.boneWeights)
		if (bw.boneName == bn) {
			bw.weights[p].endVal = w;
			return;
		}
}

static void SetUWeight(UndoStateVertex& usv, const std::string& bn, float w) {
	for (auto& bw : usv.weights)
		if (bw.boneName == bn) {
			bw.w = w;
			return;
		}
	usv.weights.push_back(UndoStateVertexBoneWeight{bn, w});
}

void OutfitProject::PrepareSymmetrizeVertices(NiShape* shape, UndoStateShape& uss, const SymmetricVertices& symverts, const VertexAsymmetries& asyms, const VertexAsymmetryTasks& tasks, const Mesh::WeldVertsType& weldVerts, const std::vector<bool>& selVerts, const std::vector<std::string>& userNormBones, const std::vector<std::string>& userNotNormBones) {
	int nMatches = static_cast<int>(symverts.matches.size());
	int nVerts = static_cast<int>(shape->GetNumVertices());
	int nSliders = static_cast<int>(asyms.sliders.size());
	int nBones = static_cast<int>(asyms.bones.size());

	// To simplify the handling of vertex data, we collect the data for
	// all vertices.  We will keep track of which vertices we actually
	// modify and put just the modified data in uss at the end.  The new
	// vertex data (tuss.addVerts) is initially a copy of the old vertex
	// data (tuss.delVerts).
	std::vector<uint16_t> allvinds(nVerts);
	for (uint16_t i = 0; i < nVerts; ++i)
		allvinds[i] = i;
	UndoStateShape tuss;
	CollectVertexData(shape, tuss, allvinds);
	tuss.addVerts = tuss.delVerts;
	std::vector<bool> vChanged(nVerts, false);

	// Prepare bone list for bone weight normalizer.  First, we add the
	// bones that are being symmetrized to normBones.
	std::vector<std::string> normBones;	// order is important: sel before unsel
	std::unordered_set<std::string> normBonesSet;
	for (int bai = 0; bai < nBones; ++bai)
		if (tasks.doBones[bai]) {
			normBones.push_back(asyms.bones[bai].boneName);
			normBonesSet.insert(asyms.bones[bai].boneName);
		}
	int nMBones = static_cast<int>(normBones.size());

	// Next, we add the norm-bones from the user to normBones, if they
	// aren't already there.
	bool hasNormBones = false;
	for (const std::string& bone : userNormBones)
		if (!normBonesSet.count(bone)) {
			normBones.push_back(bone);
			normBonesSet.insert(bone);
			hasNormBones = true;
		}

	// Finally, we add the not-norm-bones from the user.  If the user has
	// picked some norm bones that aren't being symmetrized, then we can
	// lock all the not-norm-bones.  Otherwise, they become norm bones.
	std::vector<std::string> lockedBones;
	for (const std::string& bone : userNotNormBones)
		if (!normBonesSet.count(bone)) {
			if (hasNormBones)
				lockedBones.push_back(bone);
			else
				normBones.push_back(bone);
		}

	// Initialize the bone weight normalizer
	BoneWeightAutoNormalizer nzer;
	nzer.SetUp(&tuss, &workAnim, shape->name.get(), normBones, lockedBones, nMBones, hasNormBones);

	// Because we might be adjusting the weights for several different bones
	// for a given point, we have to delay normalization until after the
	// main loop.  Only the first point in each weld set is normalized;
	// the result is copied to the other points in the weld set.
	std::vector<bool> weightAdjustPoint(nVerts, false);

	// Main loop through matches
	std::vector<int> p1s, p2s;
	for (int mi = 0; mi < nMatches; ++mi) {
		int p1 = symverts.matches[mi].first;
		int p2 = symverts.matches[mi].second;
		if (!selVerts[p1] && !selVerts[p2])
			continue;

		Mesh::GetWeldSet(weldVerts, p1, p1s);
		Mesh::GetWeldSet(weldVerts, p2, p2s);

		// Symmetrize position
		if (tasks.doPos && asyms.positions[mi]) {
			// Calculate average
			Vector3 avg;
			int avgcount = 0;
			if (selVerts[p1]) {
				Vector3 sum;
				for (int p : p2s)
					sum += tuss.addVerts[p].pos;
				avg += sum / p2s.size();
				++avgcount;
			}
			if (p1 == p2)
				avg.x = 0;
			else {
				avg.x = -avg.x;
				if (selVerts[p2]) {
					Vector3 sum;
					for (int p : p1s)
						sum += tuss.addVerts[p].pos;
					avg += sum / p1s.size();
					++avgcount;
				}
				avg /= avgcount;
			}

			// Store average
			if (selVerts[p1]) {
				for (int p : p1s) {
					tuss.addVerts[p].pos = avg;
					vChanged[p] = true;
				}
			}
			if (selVerts[p2]) {
				avg.x = -avg.x;
				for (int p : p2s) {
					tuss.addVerts[p].pos = avg;
					vChanged[p] = true;
				}
			}
		}

		// Symmetrize position slider diffs
		for (int si = 0; si < nSliders; ++si) {
			if (!tasks.doSliders[si])
				continue;
			const auto& sd = asyms.sliders[si];
			if (!sd.aflags[mi])
				continue;

			// Calculate average
			Vector3 avg;
			int avgcount = 0;
			if (selVerts[p1]) {
				Vector3 sum;
				for (int p : p2s)
					sum += GetUSliderDiff(tuss.addVerts[p], sd.sliderName);
				avg = sum / p2s.size();
				++avgcount;
			}
			if (p1 == p2)
				avg.x = 0;
			else {
				avg.x = -avg.x;
				if (selVerts[p2]) {
					Vector3 sum;
					for (int p : p1s)
						sum += GetUSliderDiff(tuss.addVerts[p], sd.sliderName);
					avg += sum / p1s.size();
					++avgcount;
				}
				avg /= avgcount;
			}

			// Store average
			if (selVerts[p1]) {
				for (int p : p1s) {
					SetUSliderDiff(tuss.addVerts[p], sd.sliderName, avg);
					vChanged[p] = true;
				}
			}
			if (selVerts[p2]) {
				avg.x = -avg.x;
				for (int p : p2s) {
					SetUSliderDiff(tuss.addVerts[p], sd.sliderName, avg);
					vChanged[p] = true;
				}
			}
		}

		// Symmetrize bone weights
		for (int bi = 0; bi < nBones; ++bi) {
			// Figure out exactly what we need to do for this match, given
			// all our flags.  Note that there are cases (such as with
			// self-matches) where we update a weight twice or sum some
			// values twice, but the result is always the same.  Trying to
			// make this code update things just once or sum things just
			// once would make it much more complex.
			const auto& bd1 = asyms.bones[bi];
			const auto& bd2 = asyms.bones[bi + bd1.mirroroffset];
			if (!bd1.aflags[mi])
				continue;
			bool doThisBone = selVerts[p1] && tasks.doBones[bi];
			bool doMirrorBone = selVerts[p2] && tasks.doBones[bi + bd1.mirroroffset];
			if (!doThisBone && !doMirrorBone)
				continue;

			// Calculate average
			float avg = 0.0f;
			int avgcount = 0;
			if (doThisBone) {
				float sum = 0.0f;
				for (int p : p2s)
					sum += GetUWeight(tuss.addVerts[p], bd2.boneName);
				avg += sum / p2s.size();
				++avgcount;
			}
			if (doMirrorBone) {
				float sum = 0.0f;
				for (int p : p1s)
					sum += GetUWeight(tuss.addVerts[p], bd1.boneName);
				avg += sum / p1s.size();
				++avgcount;
			}
			avg /= avgcount;

			// Store average
			if (doThisBone) {
				if (!weightAdjustPoint[p1s[0]]) {
					weightAdjustPoint[p1s[0]] = true;
					nzer.GrabOneVertexStartingWeights(p1s[0]);
				}
				SetUWeight(tuss, p1s[0], bd1.boneName, avg);
			}
			if (doMirrorBone) {
				if (!weightAdjustPoint[p2s[0]]) {
					weightAdjustPoint[p2s[0]] = true;
					nzer.GrabOneVertexStartingWeights(p2s[0]);
				}
				SetUWeight(tuss, p2s[0], bd2.boneName, avg);
			}
		}
	}

	// Normalize weights and transfer them from tuss.boneWeights to
	// tuss.addVerts.
	for (int p = 0; p < nVerts; ++p) {
		if (!weightAdjustPoint[p])
			continue;

		Mesh::GetWeldSet(weldVerts, p, p1s);
		for (int p1 : p1s)
			vChanged[p1] = true;

		// Normalize weights
		nzer.AdjustWeights(p);

		// Transfer results
		for (const auto& bw : tuss.boneWeights) {
			auto wit = bw.weights.find(p);
			if (wit == bw.weights.end())
				continue;
			float w = wit->second.endVal;
			for (int p1 : p1s)
				SetUWeight(tuss.addVerts[p1], bw.boneName, w);
		}
	}

	// Transfer the data for just the changed vertices from tuss to uss.
	for (int vi = 0; vi < nVerts; ++vi) {
		if (!vChanged[vi])
			continue;
		uss.delVerts.push_back(std::move(tuss.delVerts[vi]));
		uss.addVerts.push_back(std::move(tuss.addVerts[vi]));
	}

	// Collect triangle data.  Every triangle that uses any changed vertex
	// must be deleted and restored.
	std::vector<Triangle> tris;
	shape->GetTriangles(tris);
	int nTris = static_cast<int>(tris.size());
	std::vector<uint32_t> triInds;
	for (int ti = 0; ti < nTris; ++ti)
		if (vChanged[tris[ti].p1] || vChanged[tris[ti].p2] || vChanged[tris[ti].p3])
			triInds.push_back(ti);
	CollectTriangleData(shape, uss, triInds);
	uss.addTris = uss.delTris;
}

std::vector<bool> OutfitProject::CalculateAsymmetricTriangleVertexMask(NiShape* shape, const Mesh::WeldVertsType& weldVerts) {
	// Ideally, this function would return a triangle mask.  But triangle
	// masks aren't implemented in Outfit Studio yet.  So we have to
	// convert the information to a vertex mask.

	// Gather shape's vertices and triangles
	std::vector<Vector3> verts;
	workNif.GetVertsForShape(shape, verts);
	std::vector<Triangle> tris;
	shape->GetTriangles(tris);

	// Find vertex symmetries
	SymmetricVertices symverts;
	MatchSymmetricVertices(shape, weldVerts, symverts);

	// Create welded-vertex set for every vertex
	std::vector<std::vector<int>> weldSets(verts.size());
	for (int i = 0; i < static_cast<int>(verts.size()); ++i)
		Mesh::GetWeldSet(weldVerts, i, weldSets[i]);

	// Create mirror vertex map
	std::vector<int> mVerts(verts.size(), -1);
	for (const auto& mp : symverts.matches) {
		for (int vi : weldSets[mp.first])
			mVerts[vi] = mp.second;
		for (int vi : weldSets[mp.second])
			mVerts[vi] = mp.first;
	}

	// Create set of Triangles so we can quickly check if a triangle exists
	std::unordered_set<Triangle> triSet;
	for (Triangle t : tris) {
		t.rot();
		triSet.insert(t);
	}

	// Main loop through triangles
	std::vector<bool> mask(verts.size(), false);
	for (const Triangle& t : tris) {
		// m1, m2, m3: mirror of each of the triangle's points.  Note that
		// p2 and p3 are swapped, as we flip orientation when mirroring.
		int m1 = mVerts[t.p1];
		int m2 = mVerts[t.p3];
		int m3 = mVerts[t.p2];
		bool bad = false;
		// If any of m1, m2, or m3 is negative, that means the point does
		// not have a mirror match.
		if (m1 < 0 || m2 < 0 || m3 < 0)
			bad = true;
		if (!bad) {
			// Search through the weld sets for our three mirror points,
			// looking for a mirror triangle.
			bool gotmatch = false;
			for (int wm1 : weldSets[m1])
			for (int wm2 : weldSets[m2])
			for (int wm3 : weldSets[m3]) {
				Triangle mt(wm1, wm2, wm3);
				mt.rot();
				if (triSet.count(mt))
					gotmatch = true;
			}
			bad = !gotmatch;
		}
		if (bad) {
			// We've found a triangle that does not have a mirror.
			for (int i : weldSets[t.p1])
				mask[i] = true;
			for (int i : weldSets[t.p2])
				mask[i] = true;
			for (int i : weldSets[t.p3])
				mask[i] = true;
		}
	}
	return mask;
}

int OutfitProject::ImportNIF(const std::string& fileName, bool clear, const std::string& inOutfitName, std::map<std::string, std::string>* renamedShapes) {
	if (clear)
		ClearOutfit();

	if (fileName.empty()) {
		wxLogMessage("No outfit selected.");
		return 0;
	}

	auto targetGame = (TargetGame)Config.GetIntValue("TargetGame");

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

		if (targetGame == SKYRIM || targetGame == SKYRIMSE || targetGame == SKYRIMVR) {
			wxString fileRest;
			if (mGameFile.EndsWith("_0", &fileRest) || mGameFile.EndsWith("_1", &fileRest))
				mGameFile = fileRest;
		}
	}

	std::fstream file;
	PlatformUtil::OpenFileStream(file, fileName, std::ios::in | std::ios::binary);

	NifFile nif;
	int error = nif.Load(file);
	if (error) {
		if (error == 2) {
			wxString errorText = wxString::Format(_("NIF version not supported!\n\nFile: %s\n%s"), fileName, nif.GetHeader().GetVersion().GetVersionInfo());

			wxLogError(errorText);
			wxMessageBox(errorText, _("NIF Error"), wxICON_ERROR, owner);
			return 4;
		}

		wxLogError("Could not load NIF file '%s'!", fileName);
		wxMessageBox(wxString::Format(_("Could not load NIF file '%s'!"), fileName), _("NIF Error"), wxICON_ERROR, owner);
		return 1;
	}

	ValidateNIF(nif);

	nif.SetNodeName(0, "Scene Root");
	nif.RenameDuplicateShapes();

	if (baseShape) {
		std::string baseShapeName = baseShape->name.get();
		auto bshape = nif.FindBlockByName<NiShape>(baseShapeName);
		if (nif.RenameShape(bshape, baseShapeName + "_outfit")) {
			if (renamedShapes)
				(*renamedShapes)[baseShapeName] = baseShapeName + "_outfit";
		}
	}

	std::vector<std::string> shapes = workNif.GetShapeNames();
	auto nifShapes = nif.GetShapes();

	for (auto& s : nifShapes) {
		std::vector<std::string> uniqueShapes = nif.GetShapeNames();
		uniqueShapes.insert(uniqueShapes.end(), shapes.begin(), shapes.end());

		std::string shapeName = s->name.get();
		std::string newName = shapeName;
		int uniqueCount = 0;
		for (;;) {
			auto foundShape = find(uniqueShapes.begin(), uniqueShapes.end(), newName);
			if (foundShape != uniqueShapes.end()) {
				uniqueShapes.erase(foundShape);
				uniqueCount++;
				if (uniqueCount > 1)
					newName = shapeName + wxString::Format("_%d", uniqueCount).ToStdString();
			}
			else {
				if (uniqueCount > 1) {
					if (nif.RenameShape(s, newName)) {
						if (renamedShapes)
							(*renamedShapes)[shapeName] = newName;
					}
				}
				break;
			}
		}
	}

	// Add cloth data block of NIF to the list
	std::vector<BSClothExtraData*> clothDataBlocks = nif.GetChildren<BSClothExtraData>(nullptr, true);
	for (auto& cloth : clothDataBlocks)
		clothData[fileName] = cloth->Clone();

	nif.GetHeader().DeleteBlockByType("BSClothExtraData");

	if (workNif.IsValid()) {
		for (auto& s : nif.GetShapes()) {
			std::string shapeName = s->name.get();
			auto clonedShape = workNif.CloneShape(s, shapeName, &nif);
			workAnim.LoadFromNif(&workNif, clonedShape);
		}
	}
	else {
		workNif.CopyFrom(nif);
		workAnim.LoadFromNif(&workNif);
	}

	return 0;
}

int OutfitProject::ExportNIF(const std::string& fileName, const std::vector<Mesh*>& modMeshes, bool withRef) {
	workAnim.CleanupBones();
	owner->UpdateAnimationGUI();

	NifFile clone(workNif);
	ChooseClothData(clone);

	std::vector<Vector3> liveVerts;
	std::vector<Vector3> liveNorms;
	for (auto& m : modMeshes) {
		auto shape = clone.FindBlockByName<NiShape>(m->shapeName);
		if (shape) {
			liveVerts.clear();
			liveNorms.clear();

			for (int i = 0; i < m->nVerts; i++) {
				liveVerts.push_back(Mesh::TransformPosMeshToNif(m->verts[i]));
				liveNorms.push_back(Mesh::TransformDirMeshToNif(m->norms[i]));
			}

			clone.SetVertsForShape(shape, liveVerts);

			if (clone.GetHeader().GetVersion().IsSK() || clone.GetHeader().GetVersion().IsSSE()) {
				NiShader* shader = clone.GetShader(shape);
				if (shader && shader->IsModelSpace())
					continue;
			}

			clone.SetNormalsForShape(shape, liveNorms);
			clone.CalcTangentsForShape(shape);
		}
	}

	if (!withRef && baseShape) {
		std::string baseShapeName = baseShape->name.get();
		auto bshape = clone.FindBlockByName<NiShape>(baseShapeName);
		clone.DeleteShape(bshape);
		workAnim.WriteToNif(&clone, baseShapeName);
	}
	else
		workAnim.WriteToNif(&clone);

	for (auto& s : clone.GetShapes())
		clone.UpdateSkinPartitions(s);

	clone.SetShapeOrder(owner->GetShapeList());
	clone.GetHeader().SetExportInfo("Exported using Outfit Studio.");

	std::fstream file;
	PlatformUtil::OpenFileStream(file, fileName, std::ios::out | std::ios::binary);

	return clone.Save(file);
}


void OutfitProject::ChooseClothData(NifFile& nif) {
	if (!clothData.empty()) {
		wxArrayString clothFileNames;
		for (auto& cloth : clothData)
			clothFileNames.Add(wxString::FromUTF8(cloth.first));

		wxMultiChoiceDialog clothDataChoice(owner,
											_("There was cloth physics data loaded at some point (BSClothExtraData). Please choose all the origins to use in the output."),
											_("Choose cloth data"),
											clothFileNames);
		if (clothDataChoice.ShowModal() == wxID_CANCEL)
			return;

		wxArrayInt sel = clothDataChoice.GetSelections();
		for (size_t i = 0; i < sel.Count(); i++) {
			std::string selString{clothFileNames[sel[i]].ToUTF8()};
			if (!selString.empty()) {
				auto clothBlock = clothData[selString]->Clone();
				uint32_t id = nif.GetHeader().AddBlock(std::move(clothBlock));
				if (id != 0xFFFFFFFF) {
					auto root = nif.GetRootNode();
					if (root)
						root->extraDataRefs.AddBlockRef(id);
				}
			}
		}
	}
}

int OutfitProject::ExportShapeNIF(const std::string& fileName, const std::vector<std::string>& exportShapes) {
	if (exportShapes.empty())
		return 1;

	if (!workNif.IsValid())
		return 2;

	workAnim.CleanupBones();

	NifFile clone(workNif);
	ChooseClothData(clone);

	for (auto& s : clone.GetShapes())
		if (find(exportShapes.begin(), exportShapes.end(), s->name.get()) == exportShapes.end())
			clone.DeleteShape(s);

	workAnim.WriteToNif(&clone);

	for (auto& s : clone.GetShapes())
		clone.UpdateSkinPartitions(s);

	clone.GetHeader().SetExportInfo("Exported using Outfit Studio.");

	std::fstream file;
	PlatformUtil::OpenFileStream(file, fileName, std::ios::out | std::ios::binary);

	return clone.Save(file);
}

int OutfitProject::ImportOBJ(const std::string& fileName, const std::string& shapeName, NiShape* mergeShape) {
	// Set reference NIF in case nothing was loaded yet
	if (!workAnim.GetRefNif())
		workAnim.SetRefNif(&workNif);

	ObjFile obj;
	obj.SetScale(Vector3(10.0f, 10.0f, 10.0f));

	if (!shapeName.empty())
		outfitName = shapeName;
	else if (outfitName.empty())
		outfitName = "New Outfit";

	size_t vertexLimit = workNif.GetVertexLimit();
	size_t triLimit = workNif.GetTriangleLimit();

	wxString warningLabel;
	if (!baseShape)
		warningLabel = _("No reference has been loaded.  For correct bone transforms, you might need to load a reference before importing OBJ files.");

	ObjImportDialog import(owner, fileName, vertexLimit, triLimit, warningLabel);
	if (import.ShowModal() != wxID_OK)
		return 1;

	if (obj.LoadForNif(fileName, import.GetOptions())) {
		wxLogError("Could not load OBJ file '%s'!", fileName);
		wxMessageBox(wxString::Format(_("Could not load OBJ file '%s'!"), fileName), _("OBJ Error"), wxICON_ERROR, owner);
		return 1;
	}

	bool copyBaseSkinTrans = false;
	if (baseShape && !workAnim.GetTransformGlobalToShape(baseShape).IsNearlyEqualTo(MatTransform())) {
		int res = wxMessageBox(_("The reference shape has a skin coordinate system that is different from the global coordinate system.  Would you like to copy the reference's "
								 "global-to-skin transform to the imported shapes?"),
							   _("Copy skin coordinates"),
							   wxYES_NO | wxCANCEL);
		if (res == wxCANCEL)
			return 1;
		if (res == wxYES)
			copyBaseSkinTrans = true;
	}

	std::vector<std::string> groupNames = obj.GetGroupList();
	for (const auto& group : groupNames) {
		std::vector<Vector3> v;
		std::vector<Triangle> t;
		std::vector<Vector2> uv;
		std::vector<Vector3> n;
		if (!obj.CopyDataForGroup(group, &v, &t, &uv, &n)) {
			wxLogError("Could not copy data from OBJ file '%s'!", fileName);
			wxMessageBox(wxString::Format(_("Could not copy data from OBJ file '%s'!"), fileName), _("OBJ Error"), wxICON_ERROR, owner);
			return 3;
		}

		size_t vertCount = v.size();
		size_t triCount = t.size();

		// Skip zero size groups.
		if (vertCount == 0)
			continue;

		std::string useShapeName = group;

		if (mergeShape) {
			std::vector<Vector3> shapeVerts;
			workNif.GetVertsForShape(mergeShape, shapeVerts);
			if (shapeVerts.size() == vertCount) {
				int ret = wxMessageBox(_("The vertex count of the selected .obj file matches the currently selected outfit shape.  Do you wish to update the current shape?  "
										 "(click No to create a new shape)"),
									   _("Merge or New"),
									   wxYES_NO | wxICON_QUESTION,
									   owner);
				if (ret == wxYES) {
					ret = wxMessageBox(_("Update Vertex Positions?"), _("Vertex Position Update"), wxYES_NO | wxICON_QUESTION, owner);
					if (ret == wxYES)
						workNif.SetVertsForShape(mergeShape, v);

					if (uv.size() == vertCount) {
						ret = wxMessageBox(_("Update Texture Coordinates?"), _("UV Update"), wxYES_NO | wxICON_QUESTION, owner);
						if (ret == wxYES)
							workNif.SetUvsForShape(mergeShape, uv);
					}

					if (n.size() == vertCount) {
						ret = wxMessageBox(_("Update Normals?"), _("Normals Update"), wxYES_NO | wxICON_QUESTION, owner);
						if (ret == wxYES) {
							workNif.SetNormalsForShape(mergeShape, n);
							workNif.CalcTangentsForShape(mergeShape);
						}
					}

					return 101;
				}
			}
			useShapeName = wxGetTextFromUser(_("Please specify a name for the new shape"), _("New Shape Name"), useShapeName, owner).ToUTF8();
			if (useShapeName.empty())
				return 100;
		}

		auto newShape = CreateNifShapeFromData(useShapeName, &v, &t, &uv, &n);
		if (newShape) {
			uint16_t vertCountNew = newShape->GetNumVertices();
			uint32_t triCountNew = newShape->GetNumTriangles();

			if (vertCountNew < vertCount || triCountNew < triCount) {
				wxMessageBox(wxString::Format(_("The vertex or triangle limit for '%s' was exceeded.\nRemaining data was dropped.\n\nVertices (current/max): %zu/%zu\nTriangles "
												"(current/max): %zu/%zu"),
											  useShapeName,
											  vertCount,
											  vertexLimit,
											  triCount,
											  triLimit),
							 _("OBJ Error"),
							 wxICON_WARNING,
							 owner);
			}

			if (copyBaseSkinTrans)
				workAnim.SetTransformGlobalToShape(newShape, workAnim.GetTransformGlobalToShape(baseShape));
		}
	}

	return 0;
}

int OutfitProject::ExportOBJ(const std::string& fileName, const std::vector<NiShape*>& shapes, bool transToGlobal, const Vector3& scale, const Vector3& offset) {
	ObjFile obj;
	obj.SetScale(scale);
	obj.SetOffset(offset);

	for (auto& shape : shapes) {
		if (!shape)
			return 1;

		std::vector<Triangle> tris;
		if (!shape->GetTriangles(tris))
			return 2;

		const std::vector<Vector3>* verts = workNif.GetVertsForShape(shape);
		if (!verts)
			return 3;

		const std::vector<Vector2>* uvs = workNif.GetUvsForShape(shape);
		const std::vector<Vector3>* norms = workNif.GetNormalsForShape(shape);

		std::vector<Vector3> gVerts, gNorms;
		if (transToGlobal) {
			MatTransform toGlobal = workAnim.GetTransformShapeToGlobal(shape);
			gVerts.resize(verts->size());

			for (size_t i = 0; i < gVerts.size(); ++i)
				gVerts[i] = toGlobal.ApplyTransform((*verts)[i]);

			verts = &gVerts;
			if (norms) {
				gNorms.resize(norms->size());

				for (size_t i = 0; i < gNorms.size(); ++i)
					gNorms[i] = toGlobal.ApplyTransformToDir((*norms)[i]);

				norms = &gNorms;
			}
		}

		obj.AddGroup(shape->name.get(), *verts, tris, uvs ? *uvs : std::vector<Vector2>(), norms ? *norms : std::vector<Vector3>());
	}

	if (obj.Save(fileName))
		return 4;

	return 0;
}

int OutfitProject::ImportFBX(const std::string& fileName, const std::string& shapeName, NiShape* mergeShape) {
	// Set reference NIF in case nothing was loaded yet
	if (!workAnim.GetRefNif())
		workAnim.SetRefNif(&workNif);

	FBXWrangler fbxw;
	std::string nonRefBones;
	size_t vertexLimit = workNif.GetVertexLimit();
	size_t triLimit = workNif.GetTriangleLimit();

	wxString warningLabel;
	if (!baseShape)
		warningLabel = _("No reference has been loaded.  For correct bone transforms, you might need to load a reference before importing FBX files.");

	FBXImportDialog import(owner, fileName, vertexLimit, triLimit, warningLabel);
	if (import.ShowModal() != wxID_OK)
		return 1;

	bool result = fbxw.ImportScene(fileName, import.GetOptions());
	if (!result)
		return 2;

	bool copyBaseSkinTrans = false;
	if (baseShape && !workAnim.GetTransformGlobalToShape(baseShape).IsNearlyEqualTo(MatTransform())) {
		int res = wxMessageBox(_("The reference shape has a skin coordinate system that is different from the global coordinate system.  Would you like to copy the reference's "
								 "global-to-skin transform to the imported shapes?"),
							   _("Copy skin coordinates"),
							   wxYES_NO | wxCANCEL);
		if (res == wxCANCEL)
			return 1;
		if (res == wxYES)
			copyBaseSkinTrans = true;
	}

	if (!shapeName.empty())
		outfitName = shapeName;
	else if (outfitName.empty())
		outfitName = "New Outfit";

	std::vector<std::string> shapes;
	fbxw.GetShapeNames(shapes);
	for (auto& s : shapes) {
		FBXShape* fbxShape = fbxw.GetShape(s);
		std::string useShapeName = s;

		size_t vertCount = fbxShape->verts.size();
		size_t triCount = fbxShape->tris.size();

		if (mergeShape) {
			if (mergeShape->GetNumVertices() == vertCount) {
				int ret = wxMessageBox(_("The vertex count of the selected .fbx file matches the currently selected outfit shape.  Do you wish to update the current shape?  "
										 "(click No to create a new shape)"),
									   _("Merge or New"),
									   wxYES_NO | wxICON_QUESTION,
									   owner);
				if (ret == wxYES) {
					ret = wxMessageBox(_("Update Vertex Positions?"), _("Vertex Position Update"), wxYES_NO | wxICON_QUESTION, owner);
					if (ret == wxYES)
						workNif.SetVertsForShape(mergeShape, fbxShape->verts);

					if (fbxShape->uvs.size() == vertCount) {
						ret = wxMessageBox(_("Update Texture Coordinates?"), _("UV Update"), wxYES_NO | wxICON_QUESTION, owner);
						if (ret == wxYES)
							workNif.SetUvsForShape(mergeShape, fbxShape->uvs);
					}

					if (fbxShape->normals.size() == vertCount) {
						ret = wxMessageBox(_("Update Normals?"), _("Normals Update"), wxYES_NO | wxICON_QUESTION, owner);
						if (ret == wxYES) {
							workNif.SetNormalsForShape(mergeShape, fbxShape->normals);
							workNif.CalcTangentsForShape(mergeShape);
						}
					}

					ret = wxMessageBox(_("Update Animation Weighting?"), _("Animation Weight Update"), wxYES_NO | wxICON_QUESTION, owner);
					if (ret == wxYES)
						for (auto& bn : fbxShape->boneNames)
							workAnim.SetWeights(mergeShape->name.get(), bn, fbxShape->boneSkin[bn].GetWeights());

					return 101;
				}
			}

			useShapeName = wxGetTextFromUser(_("Please specify a name for the new shape"), _("New Shape Name"), useShapeName, owner).ToUTF8();
			if (useShapeName.empty())
				return 100;
		}

		auto newShape = CreateNifShapeFromData(useShapeName, &fbxShape->verts, &fbxShape->tris, &fbxShape->uvs, &fbxShape->normals);
		if (!newShape)
			continue;

		CreateSkinning(newShape);

		uint16_t vertCountNew = newShape->GetNumVertices();
		uint32_t triCountNew = newShape->GetNumTriangles();

		if (vertCountNew < vertCount || triCountNew < triCount) {
			wxMessageBox(wxString::Format(_("The vertex or triangle limit for '%s' was exceeded.\nRemaining data was dropped.\n\nVertices (current/max): %zu/%zu\nTriangles "
											"(current/max): %zu/%zu"),
										  useShapeName,
										  vertCount,
										  vertexLimit,
										  triCount,
										  triLimit),
						 _("OBJ Error"),
						 wxICON_WARNING,
						 owner);
		}

		if (copyBaseSkinTrans)
			workAnim.SetTransformGlobalToShape(newShape, workAnim.GetTransformGlobalToShape(baseShape));

		for (auto& bn : fbxShape->boneNames) {
			if (!AnimSkeleton::getInstance().GetBonePtr(bn)) {
				// Not found in reference skeleton, use default values
				AnimSkeleton::getInstance().AddCustomBone(bn);
				//AnimBone& cstm = AnimSkeleton::getInstance().AddCustomBone(bn);
				// TODO: call SetParentBone (FbxNode::GetParent?)
				// TODO: call SetTransformBoneToParent (FbxNode::LclTranslation and LclRotation?)
				nonRefBones += bn + "\n";
			}

			workAnim.AddShapeBone(useShapeName, bn);
			workAnim.SetWeights(useShapeName, bn, fbxShape->boneSkin[bn].GetWeights());
		}

		if (!nonRefBones.empty())
			wxLogMessage("Bones in shape '%s' not found in reference skeleton:\n%s", useShapeName, nonRefBones);
	}

	return 0;
}

int OutfitProject::ExportFBX(const std::string& fileName, const std::vector<NiShape*>& shapes, bool transToGlobal) {
	FBXWrangler fbxw;
	fbxw.AddSkeleton(&AnimSkeleton::getInstance().refSkeletonNif);

	for (auto& s : shapes) {
		fbxw.AddNif(&workNif, &workAnim, transToGlobal, s);
		fbxw.AddSkinning(&workAnim, s);
	}

	return fbxw.ExportScene(fileName);
}


void OutfitProject::ValidateNIF(NifFile& nif) {
	auto targetGame = (TargetGame)Config.GetIntValue("TargetGame");
	bool match = false;

	switch (targetGame) {
		case OB: match = nif.GetHeader().GetVersion().IsOB(); break;
		case FO3:
		case FONV: match = nif.GetHeader().GetVersion().IsFO3(); break;
		case SKYRIM: match = nif.GetHeader().GetVersion().IsSK(); break;
		case FO4:
		case FO4VR: match = nif.GetHeader().GetVersion().IsFO4(); break;
		case SKYRIMSE:
		case SKYRIMVR: match = nif.GetHeader().GetVersion().IsSSE(); break;
		case FO76: match = nif.GetHeader().GetVersion().IsFO76(); break;
	}

	if (nif.GetHeader().GetVersion().IsFO76()) {
		wxLogWarning("NIFs of this version can not be resaved (will throw errors).");
		return;
	}

	if (!match) {
		if ((targetGame == SKYRIMSE || targetGame == SKYRIMVR) && nif.GetHeader().GetVersion().IsSK()) {
			if (!Config.Exists("OptimizeForSSE")) {
				int res = wxMessageBox(_("Would you like Skyrim NIFs to be optimized for SSE during this session?"), _("Target Game"), wxYES_NO | wxICON_INFORMATION, owner);
				if (res == wxYES)
					Config.SetDefaultBoolValue("OptimizeForSSE", true);
				else
					Config.SetDefaultBoolValue("OptimizeForSSE", false);
			}

			if (Config["OptimizeForSSE"] == "true") {
				OptOptions options;
				options.targetVersion.SetFile(V20_2_0_7);
				options.targetVersion.SetUser(12);
				options.targetVersion.SetStream(100);
				nif.OptimizeFor(options);
			}
		}
		else {
			wxLogWarning("Version of NIF file doesn't match current target game.");
			wxMessageBox(wxString::Format(_("File format doesn't match the current game. Use FBX export, then start a new project and import the FBX file there.")),
						 _("Version"),
						 wxICON_WARNING,
						 owner);
		}
	}

	for (auto& s : nif.GetShapes())
		nif.TriangulateShape(s);
}

void OutfitProject::ResetTransforms() {
	bool clearRoot = false;
	bool unskinnedFound = false;

	for (auto& s : workNif.GetShapes()) {
		if (s->IsSkinned()) {
			/*
			 * Root node, shape and global-to-skin transform aren't rendered directly for skinned meshes.
			 * They only affect different things, e.g. bounds and the global-to-parent transforms.
			 *
			 * By clearing these and recalculating bounds on export we make sure that
			 * nothing but the individual bone transforms affect visuals.
			 */

			if (!unskinnedFound)
				clearRoot = true;

			MatTransform oldXformGlobalToSkin = workAnim.shapeSkinning[s->name.get()].xformGlobalToSkin;
			MatTransform newXformGlobalToSkin;

			// Apply global-to-skin transform to vertices
			if (!newXformGlobalToSkin.IsNearlyEqualTo(oldXformGlobalToSkin)) {
				ApplyTransformToShapeGeometry(s, newXformGlobalToSkin.ComposeTransforms(oldXformGlobalToSkin.InverseTransform()));

				workAnim.ChangeGlobalToSkinTransform(s->name.get(), newXformGlobalToSkin);
				workNif.SetShapeTransformGlobalToSkin(s, newXformGlobalToSkin);
			}

			// Clear shape transform
			s->SetTransformToParent(MatTransform());

			// Clear global-to-skin transform
			workAnim.shapeSkinning[s->name.get()].xformGlobalToSkin.Clear();
			workNif.SetShapeTransformGlobalToSkin(s, MatTransform());
		}
		else {
			clearRoot = false;
			unskinnedFound = true;
		}
	}
 
	if (clearRoot) {
		// Clear root node transform
		auto rootNode = workNif.GetRootNode();
		if (rootNode)
			rootNode->SetTransformToParent(MatTransform());
	}
}

void OutfitProject::CreateSkinning(NiShape* s) {
	if (s->IsSkinned())
		return;
	MatTransform globalToShape = workAnim.GetTransformGlobalToShape(s);
	workNif.CreateSkinning(s);
	workAnim.SetTransformGlobalToShape(s, globalToShape);
}

void OutfitProject::RemoveSkinning(NiShape* s) {
	if (!s->IsSkinned())
		return;
	MatTransform shapeToGlobal = workAnim.GetTransformShapeToGlobal(s);
	workNif.DeleteSkinning(s);
	workAnim.ClearShape(s->name.get());
	workAnim.SetTransformShapeToGlobal(s, shapeToGlobal);
}

void OutfitProject::RemoveSkinning() {
	for (auto& s : workNif.GetShapes()) {
		RemoveSkinning(s);
	}

	workNif.DeleteUnreferencedNodes();
}

bool OutfitProject::CheckForBadBones() {
	struct ShapeBadBones {
		std::unordered_map<std::string, MatTransform> badStandard, badCustom;
		bool fixStanSkin = false;
	};
	std::unordered_map<std::string, ShapeBadBones> shapeBBs;
	bool gotAnyBad = false;

	// Find bad bones for every skinned shape, initializing shapeBBs.
	for (NiShape* s : workNif.GetShapes()) {
		if (!s->IsSkinned())
			continue;

		std::string shapeName = s->name.get();
		ShapeBadBones& sbb = shapeBBs[shapeName];
		workAnim.FindBonesWithInconsistentTransforms(shapeName, sbb.badStandard, sbb.badCustom);
		if (!sbb.badStandard.empty() || !sbb.badCustom.empty())
			gotAnyBad = true;
		if (!sbb.badStandard.empty())
			sbb.fixStanSkin = true;
	}
	if (!gotAnyBad) {
		wxMessageBox(_("No Bad Bones Found."), _("No Bad Bones"), wxOK, owner);
		return true;
	}

	// For bad custom bones, we need to rearrange the data so it's keyed
	// on bone name rather than shape name.
	struct BadCustomBone {
		std::unordered_set<std::string> badShapes, goodShapes;
		enum FixType {DoNothing, TrustNode, TrustSkin};
		int fixtype = DoNothing;
		std::string trustShape;
	};
	std::unordered_map<std::string, BadCustomBone> badCBs;
	for (auto& sbbp : shapeBBs) {
		const std::string& shapeName = sbbp.first;
		ShapeBadBones& sbb = sbbp.second;
		for (auto& brp : sbb.badCustom)
			badCBs[brp.first].badShapes.insert(shapeName);
	}

	// For each bad custom bone...
	for (auto& bcbp : badCBs) {
		const std::string& bone = bcbp.first;
		BadCustomBone& bcb = bcbp.second;

		// ...fill in goodShapes
		for (auto& sbbp : shapeBBs) {
			const std::string& shapeName = sbbp.first;
			if (bcb.badShapes.count(shapeName) == 0 && workAnim.GetShapeBoneIndex(shapeName, bone) >= 0)
				bcb.goodShapes.insert(shapeName);
		}

		// ...and calculate a recommendation:
		// If a skin matches the node, trust them.
		if (!bcb.goodShapes.empty())
			bcb.fixtype = BadCustomBone::TrustNode;
		// Otherwise, if there's only one skin, trust it.
		if (bcb.fixtype == BadCustomBone::DoNothing && bcb.badShapes.size() == 1) {
			bcb.fixtype = BadCustomBone::TrustSkin;
			bcb.trustShape = *bcb.badShapes.begin();
		}
		// Otherwise, if one of the skins is the reference, trust it.
		if (bcb.fixtype == BadCustomBone::DoNothing && baseShape) {
			std::string baseShapeName = baseShape->name.get();
			for (const std::string& shapeName : bcb.badShapes)
				if (shapeName == baseShapeName) {
					bcb.fixtype = BadCustomBone::TrustSkin;
					bcb.trustShape = baseShapeName;
				}
		}
		// Otherwise, pick the skin with the lowest residual error.
		// This is pretty arbitrary.  It's only marginally better than
		// picking randomly.
		if (bcb.fixtype == BadCustomBone::DoNothing) {
			// Calculate smallest rotation residual error
			float minRotErr = 10.0f;
			for (const std::string& shapeName : bcb.badShapes) {
				float rotErr = RotMatToVec(shapeBBs[shapeName].badCustom[bone].rotation).length();
				if (rotErr < minRotErr)
					minRotErr = rotErr;
			}

			// Winnow shapes that have a significantly larger rotation error
			// than the smallest.
			std::unordered_set<std::string> shapesLeft;
			for (const std::string& shapeName : bcb.badShapes) {
				float rotErr = RotMatToVec(shapeBBs[shapeName].badCustom[bone].rotation).length();
				if (FloatsAreNearlyEqual(rotErr, minRotErr))
					shapesLeft.insert(shapeName);
			}

			// Calculate smallest translation residual error
			float minTrErr = FLT_MAX;
			for (const std::string& shapeName : shapesLeft) {
				float trErr = shapeBBs[shapeName].badCustom[bone].translation.length();
				if (trErr < minTrErr)
					minTrErr = trErr;
			}

			// Winnow shapes that have a significantly larger translation error
			// than the smallest.
			std::unordered_set<std::string> shapesLeft2;
			for (const std::string& shapeName : shapesLeft) {
				float trErr = shapeBBs[shapeName].badCustom[bone].translation.length();
				if (FloatsAreNearlyEqual(trErr, minTrErr))
					shapesLeft2.insert(shapeName);
			}
			shapesLeft = std::move(shapesLeft2);
			shapesLeft2.clear();

			// Calculate smallest scale residual error
			float minScErr = FLT_MAX;
			for (const std::string& shapeName : shapesLeft) {
				float scErr = std::fabs(shapeBBs[shapeName].badCustom[bone].scale - 1.0f);
				if (scErr < minScErr)
					minScErr = scErr;
			}

			// Winnow shapes that have a significantly larger scale error
			// than the smallest.
			for (const std::string& shapeName : shapesLeft) {
				float scErr = std::fabs(shapeBBs[shapeName].badCustom[bone].scale - 1.0f);
				if (FloatsAreNearlyEqual(scErr, minScErr))
					shapesLeft2.insert(shapeName);
			}
			shapesLeft = std::move(shapesLeft2);

			// We might have more than one shape left, but it's highly unlikely.
			// Pick the first of the remaining shapes.
			bcb.fixtype = BadCustomBone::TrustSkin;
			bcb.trustShape = *shapesLeft.begin();
		}
	}

	// Create dialog window
	wxDialog dlg(owner, -1, _("Bad Bones"), wxDefaultPosition, wxSize(800,600), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);
	wxBoxSizer* topBox = new wxBoxSizer(wxVERTICAL);
	wxSizerFlags sizerFlags = wxSizerFlags().Expand().Border(wxALL, 5);
	constexpr int wrapPixels = 800;
	wxScrolledWindow* wnd = new wxScrolledWindow(&dlg);
	topBox->Add(wnd, wxSizerFlags().Expand().Proportion(1));
	wxBoxSizer* scrollBox = new wxBoxSizer(wxVERTICAL);
	wnd->SetScrollRate(30, 50);

	// A helper class for creating the collapsible panes
	struct CollapsePane {
		wxScrolledWindow* wnd;
		wxCollapsiblePane* collapse;
		wxFlexGridSizer* collPaneBox;
		CollapsePane(wxScrolledWindow* wi, wxSizer* boxSizer, const wxString& label, const wxString& col1Label):
			wnd(wi) {
			wxSizerFlags sizerFlags = wxSizerFlags().Expand().Border(wxALL, 5);
			collapse = new wxCollapsiblePane(wnd, wxID_ANY, label, wxDefaultPosition, wxDefaultSize, wxCP_DEFAULT_STYLE | wxCP_NO_TLW_RESIZE);
			boxSizer->Add(collapse, sizerFlags);
			collPaneBox = new wxFlexGridSizer(4);
			collPaneBox->Add(new wxStaticText(collapse->GetPane(), wxID_ANY, col1Label), sizerFlags);
			collPaneBox->Add(new wxStaticText(collapse->GetPane(), wxID_ANY, _("Error in rotation")), sizerFlags);
			collPaneBox->Add(new wxStaticText(collapse->GetPane(), wxID_ANY, _("Error in translation")), sizerFlags);
			collPaneBox->Add(new wxStaticText(collapse->GetPane(), wxID_ANY, _("Error in scale")), sizerFlags);
		}
		void AddRow(const wxString& label, const MatTransform& t) {
			wxSizerFlags sizerFlags = wxSizerFlags().Expand().Border(wxALL, 5);
			float rotErr = RotMatToVec(t.rotation).length();
			float trErr = t.translation.length();
			float scErr = std::fabs(t.scale - 1.0f);
			collPaneBox->Add(new wxStaticText(collapse->GetPane(), wxID_ANY, label), sizerFlags);
			collPaneBox->Add(new wxStaticText(collapse->GetPane(), wxID_ANY, wxString() << rotErr), sizerFlags);
			collPaneBox->Add(new wxStaticText(collapse->GetPane(), wxID_ANY, wxString() << trErr), sizerFlags);
			collPaneBox->Add(new wxStaticText(collapse->GetPane(), wxID_ANY, wxString() << scErr), sizerFlags);
		}
		void Finish() {
			collapse->GetPane()->SetSizerAndFit(collPaneBox);
			wxScrolledWindow* wndl = wnd;
			collapse->Bind(wxEVT_COLLAPSIBLEPANE_CHANGED, [wndl](wxCollapsiblePaneEvent&) { wndl->FitInside(); });
		}
	};

	// Add a frame for each shape with bad standard bones
	for (auto& sbbp : shapeBBs) {
		const std::string &shapeName = sbbp.first;
		ShapeBadBones& sbb = sbbp.second;
		auto& bb = sbb.badStandard;
		if (bb.empty())
			continue;

		wxStaticBoxSizer* boxSizer = new wxStaticBoxSizer(wxVERTICAL, wnd, wxString::Format(_("Bad standard bones for shape \"%s\""), shapeName));
		scrollBox->Add(boxSizer, sizerFlags);

		wxString label = wxString::Format(_("%zu bones in shape \"%s\" had inconsistencies between their NIF skin transforms and the standard skeleton:\n"), bb.size(), shapeName);

		auto brit = bb.begin();
		label << brit->first;
		++brit;

		while (brit != bb.end()) {
			label << ", " << brit->first;
			++brit;
		}

		wxStaticText* ctrl = new wxStaticText(wnd, -1, label);
		ctrl->Wrap(wrapPixels);
		boxSizer->Add(ctrl, sizerFlags);

		wxRadioButton* rb = new wxRadioButton(wnd, wxID_ANY, _("Update skin (recommended)"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
		rb->SetValue(1);
		rb->Bind(wxEVT_RADIOBUTTON, [&](wxCommandEvent&) {
			sbb.fixStanSkin = true;
		});
		boxSizer->Add(rb, sizerFlags);
		sbb.fixStanSkin = true;

		CollapsePane bcp(wnd, boxSizer, _("Details"), _("Bone"));
		for (auto& brp : bb)
			bcp.AddRow(brp.first, brp.second);
		bcp.Finish();

		rb = new wxRadioButton(wnd, wxID_ANY, _("Do nothing"));
		rb->Bind(wxEVT_RADIOBUTTON, [&](wxCommandEvent&) {
			sbb.fixStanSkin = false;
		});
		boxSizer->Add(rb, sizerFlags);
	}

	// Add a frame for each bad custom bone
	for (auto& bcbp : badCBs) {
		const std::string& bone = bcbp.first;
		BadCustomBone& bcb = bcbp.second;

		wxStaticBoxSizer* boxSizer = new wxStaticBoxSizer(wxVERTICAL, wnd, wxString::Format(_("Bad Custom Bone \"%s\""), bone));
		scrollBox->Add(boxSizer, sizerFlags);

		wxString label = wxString::Format(_("Custom bone \"%s\" had inconsistent NIF node and skin transforms for the following shapes:\n\""), bone);

		auto bsit = bcb.badShapes.begin();
		label << *bsit;
		++bsit;

		while (bsit != bcb.badShapes.end()) {
			label << "\", \"" << *bsit;
			++bsit;
		}
		label << "\"";

		wxStaticText* ctrl = new wxStaticText(wnd, -1, label);
		ctrl->Wrap(wrapPixels);
		boxSizer->Add(ctrl, sizerFlags);

		wxString tnLabel;
		if (bcb.fixtype == BadCustomBone::TrustNode) {
			tnLabel = "";
			if (bcb.goodShapes.size() > 1)
				tnLabel << _("Trust node and skins \"");
			else
				tnLabel << _("Trust node and skin \"");
			auto gsit = bcb.goodShapes.begin();
			tnLabel << *gsit;
			++gsit;
			while (gsit != bcb.goodShapes.end()) {
				tnLabel << "\", \"" << *gsit;
				++gsit;
			}
			tnLabel << _("\", and update other skins (recommended)");
		}
		else if (bcb.badShapes.size() > 1)
			tnLabel = _("Trust node, and update skins");
		else
			tnLabel = _("Trust node, and update skin");
		wxRadioButton* rb = new wxRadioButton(wnd, wxID_ANY, tnLabel, wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
		if (bcb.fixtype == BadCustomBone::TrustNode)
			rb->SetValue(1);
		rb->Bind(wxEVT_RADIOBUTTON, [&](wxCommandEvent&) {
			bcb.fixtype = BadCustomBone::TrustNode;
		});
		boxSizer->Add(rb, sizerFlags);

		CollapsePane ncp(wnd, boxSizer, _("Details"), _("Skin"));
		for (const std::string& shapeName : bcb.badShapes) {
			MatTransform t = shapeBBs[shapeName].badCustom[bone];
			ncp.AddRow(shapeName, t);
		}
		ncp.Finish();

		for (const std::string& shapeName : bcb.badShapes) {
			wxString sLabel;
			sLabel << _("Trust skin \"") << shapeName;
			if (bcb.badShapes.size() == 1 && bcb.goodShapes.empty())
				sLabel << _("\", and update node");
			else
				sLabel << _("\", and update node and other skins");
			rb = new wxRadioButton(wnd, wxID_ANY, sLabel);
			rb->Bind(wxEVT_RADIOBUTTON, [&](wxCommandEvent&) {
				bcb.fixtype = BadCustomBone::TrustSkin;
				bcb.trustShape = shapeName;
			});
			if (bcb.fixtype == BadCustomBone::TrustSkin && bcb.trustShape == shapeName)
				rb->SetValue(1);
			boxSizer->Add(rb, sizerFlags);

			CollapsePane scp(wnd, boxSizer, _("Details"), _("With"));
			scp.AddRow(_("Node"), shapeBBs[shapeName].badCustom[bone]);
			for (const std::string& shapeName2 : bcb.badShapes) {
				if (shapeName == shapeName2)
					continue;
				MatTransform skinToBone1, skinToBone2;
				workAnim.GetXFormSkinToBone(shapeName, bone, skinToBone1);
				workAnim.GetXFormSkinToBone(shapeName, bone, skinToBone2);
				MatTransform residual = skinToBone1.ComposeTransforms(skinToBone2.InverseTransform());
				scp.AddRow(shapeName2, residual);
			}
			scp.Finish();
		}

		rb = new wxRadioButton(wnd, wxID_ANY, _("Do nothing"));
		rb->Bind(wxEVT_RADIOBUTTON, [&](wxCommandEvent&) {
			bcb.fixtype = BadCustomBone::DoNothing;
		});
		if (bcb.fixtype == BadCustomBone::DoNothing)
			rb->SetValue(1);
		boxSizer->Add(rb, sizerFlags);
	}

	// Finish building the dialog window
	wnd->SetSizer(scrollBox);
	wxStdDialogButtonSizer* buttonSizer = new wxStdDialogButtonSizer;
	buttonSizer->AddButton(new wxButton(&dlg, wxID_OK));
	buttonSizer->AddButton(new wxButton(&dlg, wxID_CANCEL, _("Fix nothing")));
	buttonSizer->Realize();
	topBox->Add(buttonSizer, sizerFlags);
	dlg.SetSizer(topBox);

	if (dlg.ShowModal() == wxID_CANCEL)
		return false;

	// Execute the user's choices
	for (auto& sbbp : shapeBBs) {
		const std::string &shapeName = sbbp.first;
		ShapeBadBones& sbb = sbbp.second;

		if (sbb.fixStanSkin)
			for (auto& brp : sbb.badStandard)
				workAnim.RecalcXFormSkinToBone(shapeName, brp.first);
	}
	for (auto& bcbp : badCBs) {
		const std::string& bone = bcbp.first;
		BadCustomBone& bcb = bcbp.second;
		if (bcb.fixtype == BadCustomBone::TrustSkin) {
			workAnim.RecalcCustomBoneXFormsFromSkin(bcb.trustShape, bone);
			// After updating the node's bone-to-global transform, any shapes
			// that had good skin-to-bone transforms now have bad ones and
			// have to be updated.
			for (const std::string& shapeName : bcb.goodShapes)
				workAnim.RecalcXFormSkinToBone(shapeName, bone);
		}
		if (bcb.fixtype != BadCustomBone::DoNothing)
			for (const std::string& shapeName : bcb.badShapes)
				workAnim.RecalcXFormSkinToBone(shapeName, bone);
	}
	return true;
}

bool OutfitProject::ShapeHasBadBones(NiShape* s) {
	if (!s->IsSkinned())
		return false;
	std::unordered_map<std::string, MatTransform> badStandard, badCustom;
	std::string shapeName = s->name.get();
	workAnim.FindBonesWithInconsistentTransforms(shapeName, badStandard, badCustom);
	if (!badStandard.empty() || !badCustom.empty())
		return true;
	return false;
}

// GetAllPoseTransforms: this function calculates the pose transform (in
// shape coordinates) for every vertex.  Note that the resulting transforms
// do not follow the rules: the "rotation" is not necessarily a rotation;
// it might contain a scale, and it'll almost certainly contain rule violations
// because of linear interpolation.
void OutfitProject::GetAllPoseTransforms(NiShape* s, std::vector<MatTransform>& ts) {
	int nVerts = static_cast<int>(s->GetNumVertices());
	ts.resize(nVerts);

	// We're going to _sum_ rotations, so we have to start with zero matrices.
	for (int i = 0; i < nVerts; ++i)
		ts[i].rotation[0][0] = ts[i].rotation[1][1] = ts[i].rotation[2][2] = 0.0f;
	std::vector<float> tws(nVerts, 0.0f); // Total weight for each vertex

	AnimSkin& animSkin = workAnim.shapeSkinning[s->name.get()];
	MatTransform globalToSkin = workAnim.GetTransformGlobalToShape(s);

	for (auto& boneNamesIt : animSkin.boneNames) {
		AnimBone* animB = AnimSkeleton::getInstance().GetBonePtr(boneNamesIt.first);
		if (!animB)
			continue;
		AnimWeight& animW = animSkin.boneWeights[boneNamesIt.second];
		// Compose transform: skin -> (posed) bone -> global -> skin
		MatTransform t = globalToSkin.ComposeTransforms(animB->xformPoseToGlobal.ComposeTransforms(animW.xformSkinToBone));
		// Add weighted contributions to vertex transforms for this bone
		for (auto& wIt : animW.weights) {
			int vi = wIt.first;
			float w = wIt.second;
			ts[vi].rotation += t.rotation * (w * t.scale);
			ts[vi].translation += t.translation * w;
			tws[vi] += w;
		}
	}

	// Check if total weight for each vertex was 1
	for (int vi = 0; vi < nVerts; ++vi) {
		if (tws[vi] < EPSILON) { // If weights are missing for this vertex
			ts[vi] = MatTransform();
		}
		else if (std::fabs(tws[vi] - 1.0f) >= EPSILON) { // If weights are not normalized for this vertex
			float normAdjust = 1.0f / tws[vi];
			ts[vi].rotation *= normAdjust;
			ts[vi].translation *= normAdjust;
		}
		// else do nothing because weights totaled 1.
	}

	// Note that the rotations still contain scale and interpolation
	// artifacts that make them not rotations!
}

// Note that ApplyTransformToOneVertexGeometry does not assume that t.rotation
// is a proper rotation, so it can be used with GetAllPoseTransforms.
void OutfitProject::ApplyTransformToOneVertexGeometry(UndoStateVertex& usv, const MatTransform& t) {
	usv.pos = t.ApplyTransform(usv.pos);
	usv.normal = t.ApplyTransformToDir(usv.normal);
	usv.normal.Normalize();
	usv.tangent = t.ApplyTransformToDir(usv.tangent);
	usv.tangent.Normalize();
	usv.bitangent = t.ApplyTransformToDir(usv.bitangent);
	usv.bitangent.Normalize();
	for (auto& usvsd : usv.diffs) {
		SliderData& sd = activeSet[usvsd.sliderName];
		if (sd.bUV || sd.bClamp || sd.bZap)
			continue;
		usvsd.diff = t.ApplyTransformToDiff(usvsd.diff);
	}
}

void OutfitProject::ApplyPoseTransformsToShapeGeometry(NiShape* s, UndoStateShape& uss) {
	int nVerts = static_cast<int>(s->GetNumVertices());
	int nTris = static_cast<int>(s->GetNumTriangles());

	// Get all the shape's vertices
	std::vector<uint16_t> vinds(nVerts);
	for (int i = 0; i < nVerts; ++i)
		vinds[i] = i;
	CollectVertexData(s, uss, vinds);
	uss.addVerts = uss.delVerts;

	// Get all the shape's triangles
	std::vector<uint32_t> tinds(nTris);
	for (int i = 0; i < nTris; ++i)
		tinds[i] = i;
	CollectTriangleData(s, uss, tinds);
	uss.addTris = uss.delTris;

	// Get all the pose transforms
	std::vector<MatTransform> ts;
	GetAllPoseTransforms(s, ts);

	// Apply all the pose transforms
	for (int i = 0; i < nVerts; ++i)
		ApplyTransformToOneVertexGeometry(uss.addVerts[i], ts[i]);
}

void OutfitProject::ApplyPoseTransformsToAllShapeGeometry(UndoStateProject& usp) {
	usp.undoType = UndoType::Mesh;
	std::vector<NiShape*> shapes = workNif.GetShapes();
	usp.usss.resize(shapes.size());
	for (size_t si = 0; si < shapes.size(); ++si) {
		UndoStateShape& uss = usp.usss[si];
		uss.shapeName = shapes[si]->name.get();
		ApplyPoseTransformsToShapeGeometry(shapes[si], uss);
	}
}
