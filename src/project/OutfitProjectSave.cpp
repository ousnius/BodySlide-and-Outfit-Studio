/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

// OutfitProject: saving the project and where its slider data is kept

#include "OutfitProject.h"
#include "components/SliderDataFileUtil.h"
#include "components/SliderSetIndex.h"
#include "files/GameDataStream.h"
#include "files/TriFile.h"
#include "utils/PlatformUtil.h"
#include "utils/StringStuff.h"
#include "NifUtil.hpp"
#include "utils/ProjectUtil.h"

#include <algorithm>
#include <atomic>
#include <cfloat>
#include <filesystem>
#include <limits>
#include <regex>
#include <sstream>
#include <wx/log.h>

using namespace nifly;

std::string OutfitProject::Save(const wxFileName& sliderSetFile,
								const wxString& strOutfitName,
								const wxString& strDataDir,
								const wxString& strBaseFile,
								const wxString& strGamePath,
								const wxString& strGameFile,
								bool genWeights,
								bool copyRef,
								bool preventMorphFile,
								bool keepZappedShapes,
								const wxString& strSFMorphPath,
								const wxString& strSFMorphTargetShape,
								SharedFilesCheck sharedFilesCheck,
								const SliderSetIndex* sharedFilesIndex) {
	std::string errmsg = "";

	auto shapes = workNif.GetShapes();
	if (shapes.empty()) {
		errmsg = _("Project has no shapes!");
		return errmsg;
	}

	UpdateProgress(1, _("Checking destination..."));
	std::string outfit{strOutfitName.ToUTF8()};
	std::string baseFile{strBaseFile.ToUTF8()};
	std::string gameFile{strGameFile.ToUTF8()};
	wxString dataDir = strDataDir;

	// An add-on uses the NIF file of its base project, which it never writes
	bool addOn = activeSet.IsAddOn();
	if (addOn && AddOnGeometryChanged()) {
		wxString msg = _("This add-on uses the shapes of its base project, so changes to them can't be saved with it.\n\nSave it as a variant of the base project with a NIF file of its own instead?");
		if (host->Ask(msg, _("Slider Add-on"), false, HostAnswer::No) != HostAnswer::Yes) {
			errmsg = _("Saving was cancelled.");
			return errmsg;
		}

		baseFile = ConvertAddOnToVariant(outfit);
		addOn = false;
	}
	else if (addOn) {
		baseFile = activeSet.GetInputFile();
		dataDir = wxString::FromUTF8(activeSet.GetDefaultDataFolder());
	}

	ReplaceForbidden(outfit);
	ReplaceForbidden(baseFile);
	ReplaceForbidden(gameFile);

	// A variant writes a NIF file of its own, never the one of its base project
	const SliderSetResolveStatus& resolveStatus = activeSet.GetResolveStatus();
	if (!addOn && activeSet.HasBaseProject() && StringsEqualInsens(baseFile.c_str(), resolveStatus.baseInputFile.c_str())
		&& StringsEqualInsens(ToOSSlashes(dataDir.ToUTF8().data()).c_str(), ToOSSlashes(resolveStatus.baseDataFolder).c_str())) {
		errmsg = wxString::Format(_("A variant needs a NIF file of its own, '%s' is the one of its base project."), wxString::FromUTF8(baseFile));
		return errmsg;
	}

	SliderSet outSet;
	outSet.SetName(outfit);
	outSet.SetDataFolder(dataDir.ToUTF8().data());
	outSet.SetInputFile(baseFile);
	outSet.SetOutputPath(strGamePath.ToUTF8().data());
	outSet.SetOutputFile(gameFile);
	outSet.SetGenWeights(genWeights);
	outSet.SetPreventMorphFile(preventMorphFile);
	outSet.SetKeepZappedShapes(keepZappedShapes);
	outSet.SetSFMorphPath(strSFMorphPath.ToUTF8().data());
	outSet.SetSFMorphTargetShape(strSFMorphTargetShape.ToUTF8().data());
	outSet.SetNotes(activeSet.GetNotes());

	const wxString sliderSetsStr = "SliderSets";

	wxFileName ssFileName(sliderSetFile);
	int sliderSetsStrIndex = ssFileName.GetDirs().Index(sliderSetsStr);
	if (sliderSetsStrIndex == wxNOT_FOUND) {
		// Make path relative to "SliderSets\", only use file name
		ssFileName = wxFileName(sliderSetsStr + PathSepStr + sliderSetFile.GetFullName());
	}

	if (ssFileName.IsRelative())
		ssFileName.MakeAbsolute(wxString::FromUTF8(ProjectUtil::GetProjectPath()));

	// Other projects can have the same NIF and .osd files
	std::string saveFolderPath = ProjectUtil::GetProjectPath() + PathSepStr + "ShapeData" + PathSepStr + dataDir.ToUTF8().data();
	std::string nifFilePath = saveFolderPath + PathSepStr + baseFile;
	std::string osdName = addOn ? activeSet.GetSliderDataFile() : baseFile.substr(0, baseFile.find_last_of('.')) + ".osd";
	std::string osdFilePath = saveFolderPath + PathSepStr + osdName;
	std::set<std::string> otherProjectData;
	if ((!addOn && PlatformUtil::FileExists(nifFilePath)) || PlatformUtil::FileExists(osdFilePath)) {
		// An index loaded before only helps if it was for these files
		if (sharedFilesCheck == SharedFilesCheck::Index && !(sharedFilesIndex && sharedFilesIndex->Covers({baseFile, osdName})))
			sharedFilesCheck = SharedFilesCheck::Skip;

		std::unique_ptr<SliderSetIndex> ownIndex;
		if (sharedFilesCheck == SharedFilesCheck::Now) {
			// Only projects that mention the NIF or .osd file can use them
			ownIndex = std::make_unique<SliderSetIndex>(ProjectUtil::GetProjectPath(), std::vector<std::string>{baseFile, osdName});
			sharedFilesIndex = ownIndex.get();
		}

		if (sharedFilesCheck != SharedFilesCheck::Skip) {
			std::string setFile{ssFileName.GetFullPath().ToUTF8()};
			if (!addOn && !ConfirmSharedNifShapes(*sharedFilesIndex, nifFilePath, setFile, outfit, copyRef)) {
				errmsg = _("Saving was cancelled.");
				return errmsg;
			}

			otherProjectData = sharedFilesIndex->DataUsedInFile(osdFilePath, setFile, outfit);
		}
		else {
			// Without knowing which data other projects use, none of the data in the file is removed
			OSDataFile oldFile;
			if (oldFile.Read(osdFilePath)) {
				std::vector<std::string> dataNames = oldFile.GetDataNames();
				otherProjectData.insert(dataNames.begin(), dataNames.end());
			}

			wxLogMessage("Saving without looking for other projects that use '%s', so the data of '%s' that the project doesn't save is kept.", baseFile, osdName);
		}
	}

	// Saving under another name or into another file makes a new project
	bool sameProject = !mFileName.empty() && wxFileName(mFileName).SameAs(ssFileName) && mOutfitName == wxString::FromUTF8(outfit);
	outSet.SetProjectID(sameProject ? activeSet.GetProjectID() : SliderSet::GenerateProjectID());

	if (activeSet.HasBaseProject()) {
		outSet.SetBaseProject(activeSet.GetBaseProjectFile(), activeSet.GetBaseProjectName(), activeSet.GetBaseProjectID());
		outSet.SetOtherBaseProjects(activeSet.GetOtherBaseProjects());
		outSet.SetAddOn(addOn);
		if (addOn)
			outSet.SetSliderDataFile(activeSet.GetSliderDataFile());

		// A slider of the project's own with the name takes the place of the base project's
		std::vector<std::string> removedSliders;
		for (auto& removedSlider : activeSet.GetRemovedSliders())
			if (!activeSet.SliderExists(removedSlider))
				removedSliders.push_back(removedSlider);
		outSet.SetRemovedSliders(removedSliders);
	}

	outSet.SetUnknownElements(activeSet.GetUnknownElements());

	// Inherited data that was found in the data folder is in the base project's folder after moving
	const std::string prevDataFolder = activeSet.GetDefaultDataFolder();
	const bool dataFolderChanges = !StringsEqualInsens(ToOSSlashes(prevDataFolder).c_str(), ToOSSlashes(dataDir.ToUTF8().data()).c_str());

	std::string prevOSDFileName = ProjectOSDFileName();

	mFileName = ssFileName.GetFullPath();
	mOutfitName = wxString::FromUTF8(outfit);
	mDataDir = dataDir;
	mBaseFile = wxString::FromUTF8(baseFile);
	mGamePath = strGamePath;
	mGameFile = wxString::FromUTF8(gameFile);
	// Without a reference there's nothing to copy, keep the choice for when one is set
	if (baseShape)
		mCopyRef = copyRef;
	mGenWeights = genWeights;
	bPreventMorphFile = preventMorphFile;
	bKeepZappedShapes = keepZappedShapes;
	mSFMorphPath = strSFMorphPath;
	mSFMorphTargetShape = strSFMorphTargetShape;
	activeSet.ClearLocalOnlyDataFolders();

	wxString folder(wxString::Format("%s/%s/%s", wxString::FromUTF8(ProjectUtil::GetProjectPath()), "ShapeData", dataDir));
	wxFileName::Mkdir(folder, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);

	int prog = 5;
	int step = 10 / shapes.size();
	UpdateProgress(prog);

	if (copyRef && baseShape) {
		// Add all the reference shapes to the target list.
		std::string baseShapeName = baseShape->name.get();
		std::string baseTarget = ShapeToTarget(baseShapeName);
		outSet.AddShapeTarget(baseShapeName, baseTarget);
		if (activeSet.TargetHasExternalData(baseTarget))
			outSet.SetTargetDataFolders(baseTarget, activeSet.GetShapeDataFolders(baseShapeName));
		outSet.SetSmoothSeamNormals(baseShapeName, activeSet.GetSmoothSeamNormals(baseShapeName));
		outSet.SetSmoothSeamNormalsAngle(baseShapeName, activeSet.GetSmoothSeamNormalsAngle(baseShapeName));
		outSet.SetLockNormals(baseShapeName, activeSet.GetLockNormals(baseShapeName));
		outSet.SetBonesPerVertex(baseShapeName, activeSet.GetBonesPerVertex(baseShapeName));
		UpdateProgress(prog += step, _("Adding reference shapes..."));
	}

	if (!mRefProjectFile.empty() && !mRefProjectName.empty() && !mRefShapeName.empty()) {
		outSet.SetReferenceInfo(mRefProjectFile, mRefProjectName, mRefShapeName);
	}
	else if (baseShape) {
		outSet.SetReferenceInfo("", "", baseShape->name.get());
	}

	// Add all the outfit shapes to the target list.
	for (auto& s : shapes) {
		if (IsBaseShape(s))
			continue;

		std::string shapeName = s->name.get();
		std::string targetName = ShapeToTarget(shapeName);
		outSet.AddShapeTarget(shapeName, targetName);

		// Reference only if not local folder
		if (activeSet.TargetHasExternalData(targetName)) {
			std::vector<std::string> shapeDataFolders = activeSet.GetShapeDataFolders(shapeName);
			for (auto& df : shapeDataFolders) {
				if (df != activeSet.GetDefaultDataFolder())
					outSet.AddTargetDataFolder(targetName, df);
			}
		}

		outSet.SetSmoothSeamNormals(shapeName, activeSet.GetSmoothSeamNormals(shapeName));
		outSet.SetSmoothSeamNormalsAngle(shapeName, activeSet.GetSmoothSeamNormalsAngle(shapeName));
		outSet.SetLockNormals(shapeName, activeSet.GetLockNormals(shapeName));
		outSet.SetBonesPerVertex(shapeName, activeSet.GetBonesPerVertex(shapeName));
		UpdateProgress(prog += step, _("Adding outfit shapes..."));
	}

	// The data of an add-on fits the base project's shapes as they are now
	if (addOn)
		for (auto& s : shapes)
			outSet.SetShapeVertexCount(s->name.get(), s->GetNumVertices());

	std::string osdFileName = ProjectOSDFileName();

	if (activeSet.size() > 0) {
		// Copy the reference slider info and add the outfit data to them.
		std::string targ;
		std::string targSlider;

		prog = 10;
		step = 20 / activeSet.size();
		UpdateProgress(prog);

		for (size_t i = 0; i < activeSet.size(); i++) {
			size_t id = outSet.CopySlider(&activeSet[i]);
			outSet[id].Clear();

			auto addDataFile = [&](const std::string& shapeName, const bool isBaseShape, const bool hasDiff) {
				const DiffInfo* dataFile = activeSet[i].GetDataFile(targSlider);
				if (dataFile && dataFile->bInherited) {
					bool local = dataFile->bLocal;
					if (local && dataFolderChanges) {
						local = false;
						outSet.AddInheritedDataFolders(targ, {prevDataFolder});
					}

					outSet[id].AddDataFile(targ, targSlider, dataFile->fileName, local, true);
				}
				else if (KeepSliderDataLink(dataFile, shapeName, isBaseShape, hasDiff, osdFileName, prevOSDFileName))
					outSet[id].AddDataFile(targ, targSlider, dataFile->fileName, dataFile->bLocal);
				else if (hasDiff)
					outSet[id].AddDataFile(targ, targSlider, osdFileName + PathSepStr + targSlider);
			};

			if (copyRef && baseShape) {
				std::string baseShapeName = baseShape->name.get();
				targ = ShapeToTarget(baseShapeName);
				targSlider = activeSet[i].TargetDataName(targ);
				if (!targSlider.empty()) {
					TargetDataDiffs* baseDiff = baseDiffData.GetDiffSet(targSlider);
					addDataFile(baseShapeName, true, baseDiff && !baseDiff->empty());
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

				addDataFile(shapeName, false, morpher.GetResultDiffSize(targ, activeSet[i].name) > 0);
			}
			UpdateProgress(prog += step, _("Calculating slider data..."));
		}
	}

	std::string saveDataPath = ProjectUtil::GetProjectPath() + PathSepStr + "ShapeData" + PathSepStr + mDataDir.ToUTF8().data();
	SaveSliderData(saveDataPath + PathSepStr + osdFileName, copyRef, osdFileName, prevOSDFileName, &otherProjectData);

	prog = 60;
	UpdateProgress(prog, _("Creating slider set file..."));

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

	UpdateProgress(61, _("Saving slider set file..."));
	ssf.UpdateSet(outSet);
	if (!ssf.Save()) {
		errmsg = _("Failed to write to slider set file: ") + ssUFileName;
		return errmsg;
	}

	activeSet.SetProjectID(outSet.GetProjectID());
	if (activeSet.HasBaseProject())
		activeSet.SetRemovedSliders(outSet.GetRemovedSliders());

	AdoptSavedDataLocations(outSet);

	UpdateProgress(70, _("Saving NIF file..."));

	std::string saveFileName = saveDataPath + PathSepStr + baseFile;

	if (workNif.IsValid() && !addOn) {
		workAnim.CleanupBones();
		host->OnAnimationChanged();

		NifFile clone(workNif);
		ChoosePhysicsData(clone);

		if (!copyRef && baseShape) {
			std::string baseShapeName = baseShape->name.get();
			auto shape = clone.FindBlockByName<NiShape>(baseShapeName);
			clone.DeleteShape(shape);
			WriteAnimToNif(clone, baseShapeName);
		}
		else
			WriteAnimToNif(clone);

		ChooseClothData(clone);

		for (auto& s : clone.GetShapes())
			clone.UpdateSkinPartitions(s);

		clone.SetShapeOrder(GetShapeOrder());
		clone.GetHeader().SetExportInfo("Exported using Outfit Studio.");

		// Project ShapeData NIFs always use internal geometry for simplicity, with meshlets like exported NIFs
		GameDataStream::PrepareGeometryForSave(clone, true);

		std::fstream file;
		PlatformUtil::OpenFileStream(file, saveFileName, std::ios::out | std::ios::binary);

		if (clone.Save(file)) {
			errmsg = _("Failed to write base .nif file: ") + saveFileName;
			return errmsg;
		}
	}

	host->OnPartitionsChanged();
	UpdateProgress(100, _("Finished"));
	return errmsg;
}

bool OutfitProject::SaveSliderData(const std::string& fileName,
								   bool copyRef,
								   const std::string& osdFileName,
								   const std::string& prevOSDFileName,
								   const std::set<std::string>* otherProjectData) {
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
				std::unordered_map<uint16_t, Vector3>* diff = baseDiffData.GetDiffSet(targSlider);
				if (diff && !diff->empty() && !KeepSliderDataLink(activeSet[i].GetDataFile(targSlider), baseShapeName, true, true, osdFileName, prevOSDFileName)) {
					osdDiffs.LoadSet(targSlider, targ, *diff);
					osdNames[fileName][targSlider] = targ;
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

				if (morpher.GetResultDiffSize(targ, activeSet[i].name) > 0
					&& !KeepSliderDataLink(activeSet[i].GetDataFile(targSlider), shapeName, false, true, osdFileName, prevOSDFileName)) {
					std::unordered_map<uint16_t, Vector3> diff;
					morpher.GetRawResultDiff(targ, activeSet[i].name, diff);
					osdDiffs.LoadSet(targSlider, targ, diff);
					osdNames[fileName][targSlider] = targ;
				}
			}
		}

		// Data of other projects in the same file stays in it
		std::map<std::string, std::set<std::string>> keepData;
		if (otherProjectData && !otherProjectData->empty()) {
			auto& savedNames = osdNames[fileName];
			size_t keptCount = 0;
			for (auto& dataName : *otherProjectData)
				if (savedNames.find(dataName) == savedNames.end())
					keptCount++;

			if (keptCount > 0) {
				keepData[fileName] = *otherProjectData;
				wxLogMessage("Keeping %zu slider data entries of other projects in '%s'.", keptCount, fileName);
			}

			if (savedNames.empty())
				osdNames.erase(fileName);
		}

		if (!osdDiffs.SaveData(osdNames, &keepData))
			return false;
	}

	return true;
}

bool OutfitProject::ConfirmSharedNifShapes(const SliderSetIndex& index, const std::string& nifFilePath, const std::string& setFile, const std::string& setName, bool copyRef) {
	auto otherSets = index.SetsUsingNif(nifFilePath, setFile, setName);
	if (otherSets.empty())
		return true;

	std::fstream file;
	PlatformUtil::OpenFileStream(file, nifFilePath, std::ios::in | std::ios::binary);

	NifFile oldNif;
	if (oldNif.Load(file) != 0)
		return true;

	std::set<std::string> savedShapes;
	for (auto& shape : workNif.GetShapes())
		if (copyRef || !IsBaseShape(shape))
			savedShapes.insert(shape->name.get());

	wxString lostShapes;
	for (auto* entry : otherSets) {
		std::vector<std::string> shapeNames;
		for (auto& shapeName : oldNif.GetShapeNames())
			if (savedShapes.find(shapeName) == savedShapes.end() && entry->set.HasShape(shapeName))
				shapeNames.push_back(shapeName);

		if (!shapeNames.empty())
			lostShapes += wxString::Format("\n%s: %s", wxString::FromUTF8(entry->setName), wxString::FromUTF8(JoinStrings(shapeNames, ", ")));
	}

	if (lostShapes.empty())
		return true;

	wxString msg = wxString::Format(_("Other projects use '%s' too. Saving would remove shapes from it that they need:\n%s\n\nSave anyway?"),
									wxString::FromUTF8(std::filesystem::u8path(nifFilePath).filename().u8string()),
									lostShapes);
	return host->Ask(msg, _("Shared NIF File"), false, HostAnswer::No) == HostAnswer::Yes;
}

std::string OutfitProject::ProjectOSDFileName() {
	// The NIF file of an add-on is the base project's
	if (activeSet.IsAddOn())
		return activeSet.GetSliderDataFile();

	std::string baseFile{mBaseFile.ToUTF8()};
	if (baseFile.empty())
		return "";

	return baseFile.substr(0, baseFile.find_last_of('.')) + ".osd";
}

bool OutfitProject::SliderDataIsLinked(const DiffInfo& dataFile) {
	if (dataFile.bInherited || !dataFile.bLocal || SliderDataFileIsBSD(dataFile.fileName))
		return false;

	std::string projectOSDFileName = ProjectOSDFileName();
	std::string osdFileName;
	std::string dataNameInFile;
	return !projectOSDFileName.empty() && SplitSliderDataFileName(dataFile.fileName, osdFileName, dataNameInFile)
		&& !StringsEqualInsens(osdFileName.c_str(), projectOSDFileName.c_str());
}

void OutfitProject::UnlinkSliderData(DiffInfo& dataFile) {
	std::string projectOSDFileName = ProjectOSDFileName();
	std::string osdFileName;
	std::string dataNameInFile;
	if (SliderDataFileIsBSD(dataFile.fileName) || !SplitSliderDataFileName(dataFile.fileName, osdFileName, dataNameInFile)
		|| StringsEqualInsens(osdFileName.c_str(), projectOSDFileName.c_str()))
		return;

	// Point to the project's own file, the data is moved there on save
	if (projectOSDFileName.empty())
		dataFile.fileName = dataFile.dataName;
	else
		dataFile.fileName = projectOSDFileName + PathSepStr + dataFile.dataName;
}

void OutfitProject::DetachSliderData(DiffInfo& dataFile) {
	if (!dataFile.bInherited)
		return;

	dataFile.bInherited = false;
	dataFile.bLocal = true;
	UnlinkSliderData(dataFile);
}

bool OutfitProject::KeepSliderDataLink(const DiffInfo* dataFile,
									   const std::string& shapeName,
									   const bool isBaseShape,
									   const bool hasDiff,
									   const std::string& osdFileName,
									   const std::string& prevOSDFileName) {
	if (!dataFile)
		return false;

	// The base project owns inherited data
	if (dataFile->bInherited)
		return true;

	if (!dataFile->bLocal) {
		// Reference data, external data outside of the default data folder and external data that wasn't loaded keep their link
		std::vector<std::string> shapeDataFolders = activeSet.GetShapeDataFolders(shapeName);
		bool isDefaultDataFolder = shapeDataFolders.size() == 1 && shapeDataFolders.front() == activeSet.GetDefaultDataFolder();
		if (isBaseShape || !isDefaultDataFolder || !hasDiff)
			return true;
	}

	if (osdFileName.empty() || SliderDataFileIsBSD(dataFile->fileName))
		return false;

	// The data folder changes, so links within it can't be kept
	if (activeSet.GetDefaultDataFolder() != mDataDir.ToUTF8().data())
		return false;

	std::string dataOSDFileName;
	std::string dataNameInFile;
	if (!SplitSliderDataFileName(dataFile->fileName, dataOSDFileName, dataNameInFile))
		return false;

	if (StringsEqualInsens(dataOSDFileName.c_str(), osdFileName.c_str()) || StringsEqualInsens(dataOSDFileName.c_str(), prevOSDFileName.c_str()))
		return false;

	// Data in another project's .osd file of the data folder keeps its link, unless it was merged from a project in another folder
	return activeSet.ResolveSliderDataFile(*dataFile).resolved;
}

void OutfitProject::AdoptSavedDataLocations(const SliderSet& savedSet) {
	// Shapes that weren't saved, like the reference without its copy, keep their data folders. Those of shapes loaded
	// from a set always name the folder of their external data, so it's still found from the new data folder.
	for (auto shape = savedSet.ShapesBegin(); shape != savedSet.ShapesEnd(); ++shape)
		if (activeSet.ShapeToTarget(shape->first) == shape->second.targetShape && !shape->second.targetShape.empty())
			activeSet.SetTargetDataFolders(shape->second.targetShape, shape->second.dataFolders);

	// The saved entries say where the data is now, e.g. in the project's own .osd file under its new name
	for (size_t i = 0; i < activeSet.size(); i++) {
		SliderData& slider = activeSet[i];
		const SliderData* savedSlider = nullptr;
		for (size_t s = 0; s < savedSet.size() && !savedSlider; s++)
			if (savedSet[s].name == slider.name)
				savedSlider = &savedSet[s];
		if (!savedSlider)
			continue;

		for (auto& dataFile : slider.dataFiles) {
			for (auto& savedFile : savedSlider->dataFiles) {
				if (savedFile.targetName == dataFile.targetName && savedFile.dataName == dataFile.dataName) {
					dataFile.fileName = savedFile.fileName;
					dataFile.bLocal = savedFile.bLocal;
					dataFile.bInherited = savedFile.bInherited;
					break;
				}
			}
		}
	}

	activeSet.SetDataFolder(savedSet.GetDefaultDataFolder());
}

bool OutfitProject::ShapeSliderDataIsLocalOnly(const std::string& shapeName) {
	std::string target = ShapeToTarget(shapeName);
	for (size_t i = 0; i < activeSet.size(); i++) {
		for (auto& dataFile : activeSet[i].dataFiles) {
			if (dataFile.targetName == target && !dataFile.bLocal)
				return false;
		}
	}

	return true;
}

bool OutfitProject::GetShapeReferenceSource(const std::string& shapeName, std::string& outProjectFile, std::string& outProjectName) const {
	auto it = shapeRefSources.find(shapeName);
	if (it == shapeRefSources.end()) {
		outProjectFile.clear();
		outProjectName.clear();
		return false;
	}

	outProjectFile = it->second.projectFile;
	outProjectName = it->second.projectName;
	return true;
}

void OutfitProject::SetShapeReferenceSource(const std::string& shapeName, const std::string& projectFile, const std::string& projectName) {
	shapeRefSources[shapeName] = {projectFile, projectName};
}

std::string OutfitProject::SliderShapeDataName(const size_t index, const std::string& shapeName) {
	if (!ValidSlider(index))
		return "";

	return activeSet.ShapeToDataName(index, shapeName);
}

std::string OutfitProject::SliderDataTargetForShape(NiShape* shape) {
	if (!shape)
		return "";

	std::string shapeName = shape->name.get();
	std::string target = ShapeToTarget(shapeName);
	if (target.empty()) {
		target = shapeName;
		activeSet.AddShapeTarget(shapeName, target);
	}

	return target;
}

std::string OutfitProject::ShapeTargetOrDefault(const std::string& shapeName) {
	for (auto it = activeSet.ShapesBegin(); it != activeSet.ShapesEnd(); ++it) {
		if (it->first == shapeName) {
			if (it->second.targetShape.empty())
				return shapeName;

			return it->second.targetShape;
		}
	}

	return shapeName;
}

bool OutfitProject::TargetNameInUse(const std::string& targetName, const std::string& exceptShapeName) {
	if (targetName.empty())
		return false;

	std::set<std::string> shapeNames;
	if (workNif.IsValid()) {
		for (auto* shape : workNif.GetShapes())
			shapeNames.insert(shape->name.get());
	}

	for (auto it = activeSet.ShapesBegin(); it != activeSet.ShapesEnd(); ++it)
		shapeNames.insert(it->first);

	for (auto& shapeName : shapeNames) {
		if (shapeName == exceptShapeName)
			continue;

		if (ShapeTargetOrDefault(shapeName) == targetName)
			return true;
	}

	return false;
}

std::string OutfitProject::UniqueTargetNameForShape(const std::string& shapeName, const std::set<std::string>& reservedTargets) {
	std::string baseTarget = shapeName.empty() ? "Shape" : shapeName;
	std::string target = baseTarget;
	int targetSuffix = 2;

	while (reservedTargets.find(target) != reservedTargets.end() || TargetNameInUse(target, shapeName))
		target = baseTarget + "_" + std::to_string(targetSuffix++);

	return target;
}

void OutfitProject::RetargetShapeData(const std::string& shapeName, const std::string& newTarget) {
	if (shapeName.empty() || newTarget.empty())
		return;

	std::string oldTarget = ShapeTargetOrDefault(shapeName);
	if (oldTarget.empty() || oldTarget == newTarget)
		return;

	NiShape* shape = workNif.FindBlockByName<NiShape>(shapeName);
	bool isBaseShape = IsBaseShape(shape);
	std::vector<std::pair<std::string, std::string>> sliderDataNames;
	for (size_t i = 0; i < activeSet.size(); i++) {
		SliderData& slider = activeSet[i];
		for (auto& dataFile : slider.dataFiles) {
			if (dataFile.targetName != oldTarget)
				continue;

			std::string oldDataName = dataFile.dataName;
			std::string newDataName = oldDataName;
			if (oldDataName == oldTarget + slider.name)
				newDataName = newTarget + slider.name;
			else if (StringStartsWith(oldDataName, oldTarget))
				newDataName = newTarget + oldDataName.substr(oldTarget.length());
			else if (StringEndsWith(oldDataName, slider.name))
				newDataName = newTarget + slider.name;
			else
				newDataName = newTarget + slider.name;

			if (oldDataName != newDataName) {
				if (isBaseShape)
					baseDiffData.RenameSet(oldDataName, newDataName);
				else
					morpher.RenameSet(oldDataName, newDataName);
			}

			dataFile.targetName = newTarget;
			dataFile.dataName = newDataName;
			dataFile.fileName = newDataName;
			dataFile.bLocal = true;
			dataFile.bInherited = false;
			sliderDataNames.emplace_back(slider.name, newDataName);
		}
	}

	activeSet.AddShapeTarget(shapeName, newTarget);

	if (isBaseShape) {
		baseDiffData.RenameDataTarget(oldTarget, newTarget);
	}
	else {
		morpher.RenameDataTarget(oldTarget, newTarget);
		for (auto& sliderDataName : sliderDataNames)
			morpher.SetResultDataName(newTarget, sliderDataName.first, sliderDataName.second);
	}

	wxLogMessage("Retargeted shape '%s' slider data from target '%s' to local target '%s' to avoid a target name conflict.", shapeName, oldTarget, newTarget);
}

void OutfitProject::ResolveTargetConflictsForIncomingShapes(const std::vector<std::pair<std::string, std::string>>& incomingShapeTargets) {
	if (incomingShapeTargets.empty())
		return;

	std::set<std::string> incomingShapeNames;
	std::set<std::string> incomingTargets;
	for (auto& incomingShapeTarget : incomingShapeTargets) {
		if (incomingShapeTarget.first.empty())
			continue;

		incomingShapeNames.insert(incomingShapeTarget.first);
		if (incomingShapeTarget.second.empty())
			incomingTargets.insert(incomingShapeTarget.first);
		else
			incomingTargets.insert(incomingShapeTarget.second);
	}

	if (incomingTargets.empty())
		return;

	std::set<std::string> existingShapeNames;
	if (workNif.IsValid()) {
		for (auto* shape : workNif.GetShapes())
			existingShapeNames.insert(shape->name.get());
	}

	for (auto it = activeSet.ShapesBegin(); it != activeSet.ShapesEnd(); ++it)
		existingShapeNames.insert(it->first);

	for (auto& shapeName : existingShapeNames) {
		if (incomingShapeNames.find(shapeName) != incomingShapeNames.end())
			continue;

		std::string target = ShapeTargetOrDefault(shapeName);
		if (incomingTargets.find(target) == incomingTargets.end())
			continue;

		RetargetShapeData(shapeName, UniqueTargetNameForShape(shapeName, incomingTargets));
	}
}

bool OutfitProject::ResolveSliderDataEntry(const SliderDataKey& key, size_t& sliderIndex, size_t& dataIndex) {
	for (size_t curSliderIndex = 0; curSliderIndex < activeSet.size(); curSliderIndex++) {
		auto& slider = activeSet[curSliderIndex];
		if (slider.name != key.sliderName)
			continue;

		for (size_t curDataIndex = 0; curDataIndex < slider.dataFiles.size(); curDataIndex++) {
			auto& dataFile = slider.dataFiles[curDataIndex];
			if (dataFile.targetName != key.targetName || dataFile.dataName != key.dataName)
				continue;

			sliderIndex = curSliderIndex;
			dataIndex = curDataIndex;
			return true;
		}
	}

	return false;
}

void OutfitProject::GetSliderDataLocations(std::vector<SliderDataLocation>& outLocations, const std::string& sliderName) {
	outLocations.clear();
	activeSet.ClearLocalOnlyDataFolders();

	for (size_t sliderIndex = 0; sliderIndex < activeSet.size(); sliderIndex++) {
		if (!sliderName.empty() && activeSet[sliderIndex].name != sliderName)
			continue;

		for (size_t dataFileIndex = 0; dataFileIndex < activeSet[sliderIndex].dataFiles.size(); dataFileIndex++) {
			auto& dataFile = activeSet[sliderIndex].dataFiles[dataFileIndex];
			SliderDataFileResolution resolution = activeSet.ResolveSliderDataFile(dataFile);

			SliderDataLocation location;
			location.key.sliderName = activeSet[sliderIndex].name;
			location.shapeName = activeSet.TargetToShape(dataFile.targetName);
			if (location.shapeName.empty())
				location.shapeName = dataFile.targetName;
			location.key.targetName = dataFile.targetName;
			location.key.dataName = dataFile.dataName;
			location.fileName = dataFile.fileName;
			location.dataFileName = resolution.dataFileName;
			location.dataNameInFile = resolution.dataNameInFile;
			location.inherited = dataFile.bInherited;
			location.overridden = !dataFile.bInherited && activeSet.BaseData(location.key.sliderName, dataFile.targetName);
			location.linked = SliderDataIsLinked(dataFile);
			location.local = dataFile.bLocal && !location.linked && !location.inherited;
			location.resolved = resolution.resolved;
			location.isBSD = resolution.isBSD;
			location.resolvedPath = resolution.resolvedPath;
			location.candidatePath = resolution.candidatePath;
			location.dataFolders = resolution.dataFolders;

			outLocations.push_back(std::move(location));
		}
	}
}

bool OutfitProject::SliderDataIsExternal(const std::string& sliderName, NiShape* shape) {
	size_t sliderIndex = 0;
	if (!shape || !SliderIndexFromName(sliderName, sliderIndex))
		return false;

	std::string target = ShapeToTarget(shape->name.get());
	if (target.empty())
		target = shape->name.get();
	if (target.empty())
		return false;

	for (auto& dataFile : activeSet[sliderIndex].dataFiles)
		if (dataFile.targetName == target && (dataFile.bInherited || !dataFile.bLocal || SliderDataIsLinked(dataFile)))
			return true;

	return false;
}

bool OutfitProject::SliderFromBase(const std::string& sliderName) {
	return activeSet.HasBaseProject() && activeSet.BaseHasSlider(sliderName);
}

std::vector<SliderDataKey> OutfitProject::RenameDisconnectsFromBase(const std::string& sliderName, const std::string& shapeName) {
	std::vector<SliderDataKey> keys;
	std::string target = shapeName.empty() ? "" : ShapeTargetOrDefault(shapeName);

	for (size_t i = 0; i < activeSet.size(); i++) {
		if (!sliderName.empty() && activeSet[i].name != sliderName)
			continue;

		for (auto& dataFile : activeSet[i].dataFiles) {
			if (!dataFile.bInherited || (!target.empty() && dataFile.targetName != target))
				continue;

			SliderDataKey key;
			key.sliderName = activeSet[i].name;
			key.targetName = dataFile.targetName;
			key.dataName = dataFile.dataName;
			keys.push_back(std::move(key));
		}
	}

	return keys;
}

bool OutfitProject::RevertSliderDataToBase(const SliderDataKey& key, std::string* errorMessage) {
	size_t sliderIndex = 0;
	size_t dataIndex = 0;
	if (!ResolveSliderDataEntry(key, sliderIndex, dataIndex)) {
		if (errorMessage)
			*errorMessage = "Slider data entry no longer exists.";
		return false;
	}

	DiffInfo* dataFile = activeSet.GetSliderDataFile(sliderIndex, dataIndex);
	if (dataFile->bInherited)
		return true;

	std::string sliderName = activeSet[sliderIndex].name;
	const DiffInfo* baseData = activeSet.BaseData(sliderName, dataFile->targetName);
	if (!baseData) {
		if (errorMessage)
			*errorMessage = "The base project has no data of the slider for this shape.";
		return false;
	}

	DiffInfo inherited = *baseData;
	const auto& baseDataFolders = activeSet.GetResolveStatus().baseDataFolders;
	auto targetFolders = baseDataFolders.find(inherited.targetName);
	if (targetFolders != baseDataFolders.end())
		activeSet.AddInheritedDataFolders(inherited.targetName, targetFolders->second);

	SliderDataFileResolution resolution = activeSet.ResolveSliderDataFile(inherited);
	if (!resolution.resolved) {
		if (errorMessage)
			*errorMessage = "The slider data file of the base project wasn't found.";
		return false;
	}

	DiffDataSets loaded;
	std::string loadedName = resolution.isBSD ? inherited.dataName : resolution.dataNameInFile;
	if (resolution.isBSD)
		loaded.LoadSet(loadedName, inherited.targetName, resolution.resolvedPath);
	else
		loaded.LoadData({{resolution.resolvedPath, {{loadedName, inherited.targetName}}}});

	TargetDataDiffs baseDiff;
	if (TargetDataDiffs* diff = loaded.GetDiffSet(loadedName))
		baseDiff = *diff;

	// The project's own data makes way for the base project's
	NiShape* shape = workNif.FindBlockByName<NiShape>(activeSet.TargetToShape(inherited.targetName));
	if (shape && IsBaseShape(shape)) {
		baseDiffData.ClearSet(dataFile->dataName);
		baseDiffData.LoadSet(inherited.dataName, inherited.targetName, baseDiff);
	}
	else {
		morpher.ClearResultSet(dataFile->dataName);
		morpher.SetResultDataName(inherited.targetName, sliderName, inherited.dataName);
		morpher.SetResultDiff(inherited.targetName, sliderName, baseDiff);
	}

	*dataFile = inherited;
	return true;
}

void OutfitProject::RecordAddOnGeometry(bool addOn) {
	addOnRefShapeName.clear();
	addOnShapeNames.clear();
	addOnGeometryHash = 0;
	addOnRefShapeHash = 0;
	if (!addOn)
		return;

	for (auto& shapeName : workNif.GetShapeNames())
		addOnShapeNames.insert(shapeName);

	// The reference shape counts on its own, since loading another reference replaces it
	if (baseShape)
		addOnRefShapeName = baseShape->name.get();

	addOnGeometryHash = ShapeGeometryHash([this](const std::string& shapeName) { return shapeName != addOnRefShapeName; });
	addOnRefShapeHash = addOnRefShapeName.empty() ? 0 : ShapeGeometryHash([this](const std::string& shapeName) { return shapeName == addOnRefShapeName; });
}

std::string OutfitProject::ConvertAddOnToVariant(const std::string& projectName) {
	// The data of the add-on stays in its .osd file, which is named like the NIF file of a variant
	std::string dataFile = activeSet.GetSliderDataFile();
	std::string nifFile = dataFile.empty() ? projectName + ".nif" : dataFile.substr(0, dataFile.find_last_of('.')) + ".nif";
	activeSet.SetAddOn(false);
	activeSet.SetSliderDataFile("");
	return nifFile;
}

bool OutfitProject::AddOnGeometryChanged() {
	// A reference shape that was loaded since isn't one of the base project's shapes. One of them can become the reference too.
	std::string loadedRefShapeName;
	if (baseShape && addOnShapeNames.find(baseShape->name.get()) == addOnShapeNames.end())
		loadedRefShapeName = baseShape->name.get();

	const std::string refShapeName = baseShape ? baseShape->name.get() : "";
	auto isOutfitShape = [&](const std::string& shapeName) { return shapeName != addOnRefShapeName && shapeName != loadedRefShapeName; };
	if (ShapeGeometryHash(isOutfitShape) != addOnGeometryHash)
		return true;

	return !addOnRefShapeName.empty() && refShapeName == addOnRefShapeName
		&& ShapeGeometryHash([this](const std::string& shapeName) { return shapeName == addOnRefShapeName; }) != addOnRefShapeHash;
}

uint64_t OutfitProject::ShapeGeometryHash(const std::function<bool(const std::string&)>& includeShape) {
	uint64_t hash = 14695981039346656037ull;
	auto add = [&hash](const void* data, size_t size) {
		auto bytes = static_cast<const uint8_t*>(data);
		for (size_t i = 0; i < size; i++) {
			hash ^= bytes[i];
			hash *= 1099511628211ull;
		}
	};
	auto addString = [&add](const std::string& text) { add(text.c_str(), text.size() + 1); };

	auto shapes = workNif.GetShapes();
	std::sort(shapes.begin(), shapes.end(), [](NiShape* a, NiShape* b) { return a->name.get() < b->name.get(); });

	for (auto* shape : shapes) {
		std::string shapeName = shape->name.get();
		if (!includeShape(shapeName))
			continue;

		addString(shapeName);

		std::vector<Vector3> verts;
		workNif.GetVertsForShape(shape, verts);
		add(verts.data(), verts.size() * sizeof(Vector3));

		std::vector<Triangle> tris;
		shape->GetTriangles(tris);
		add(tris.data(), tris.size() * sizeof(Triangle));

		std::vector<Vector2> uvs;
		workNif.GetUvsForShape(shape, uvs);
		add(uvs.data(), uvs.size() * sizeof(Vector2));

		auto bones = workAnim.shapeBones.find(shapeName);
		if (bones == workAnim.shapeBones.end())
			continue;

		for (auto& bone : bones->second) {
			addString(bone);

			std::unordered_map<uint16_t, float> weights;
			workAnim.GetWeights(shapeName, bone, weights);
			std::map<uint16_t, float> sortedWeights(weights.begin(), weights.end());
			for (auto& weight : sortedWeights) {
				add(&weight.first, sizeof(weight.first));
				add(&weight.second, sizeof(weight.second));
			}
		}
	}

	return hash;
}

void OutfitProject::ReportResolveStatus() {
	const SliderSetResolveStatus& status = activeSet.GetResolveStatus();

	wxString msg;
	if (status.newerVersion)
		msg += _("This project was saved by a newer version of Outfit Studio. Saving it with this version can lose what only the newer version knows.") + "\n";

	for (auto& warning : status.warnings)
		msg += wxString::FromUTF8(warning) + "\n";

	// The base project's shapes may have changed since the add-on's data was made for them
	if (activeSet.IsAddOn()) {
		for (auto* shape : workNif.GetShapes()) {
			uint32_t vertexCount = activeSet.GetShapeVertexCount(shape->name.get());
			if (vertexCount > 0 && vertexCount != shape->GetNumVertices())
				msg += wxString::Format(_("The add-on's slider data of shape '%s' was made for %u vertices, but the base project's shape has %u now."),
										wxString::FromUTF8(shape->name.get()),
										vertexCount,
										static_cast<uint32_t>(shape->GetNumVertices()))
					+ "\n";
		}
	}

	if (!msg.empty())
		host->ShowMessage(HostMessageLevel::Warning, msg.Trim(), _("Base Project"));
}

std::string OutfitProject::EnsureSliderDataLocal(const std::string& sliderName, NiShape* shape) {
	size_t sliderIndex = 0;
	if (!shape || !SliderIndexFromName(sliderName, sliderIndex))
		return "";

	std::string target = SliderDataTargetForShape(shape);
	if (target.empty())
		return "";

	std::string dataName = activeSet[sliderIndex].TargetDataName(target);
	if (dataName.empty()) {
		dataName = target + sliderName;
		activeSet[sliderIndex].AddDataFile(target, dataName, dataName);
	}
	else {
		// Inherited data becomes the project's own, which overrides the base project's
		activeSet[sliderIndex].SetLocalData(dataName);
		if (DiffInfo* dataFile = activeSet[sliderIndex].GetDataFile(dataName)) {
			dataFile->bInherited = false;
			UnlinkSliderData(*dataFile);
		}
	}

	if (!activeSet.TargetHasExternalData(target))
		activeSet.ClearLocalOnlyDataFolders();

	return dataName;
}

bool OutfitProject::SetSliderDataLocal(const SliderDataKey& key, std::string* errorMessage) {
	size_t sliderIndex = 0;
	size_t dataIndex = 0;
	if (!ResolveSliderDataEntry(key, sliderIndex, dataIndex)) {
		if (errorMessage)
			*errorMessage = "Slider data entry no longer exists.";
		return false;
	}

	return SetSliderDataLocal(sliderIndex, dataIndex, errorMessage);
}

bool OutfitProject::SetSliderDataLocal(const size_t sliderIndex, const size_t dataIndex, std::string* errorMessage) {
	DiffInfo* dataFile = activeSet.GetSliderDataFile(sliderIndex, dataIndex);
	if (!dataFile) {
		if (errorMessage)
			*errorMessage = "Slider data entry no longer exists.";
		return false;
	}

	std::string targetName = dataFile->targetName;
	activeSet.SetSliderDataFileLocal(sliderIndex, dataIndex, true);
	dataFile->bInherited = false;
	UnlinkSliderData(*dataFile);
	if (!activeSet.TargetHasExternalData(targetName))
		activeSet.ClearLocalOnlyDataFolders();

	return true;
}

bool OutfitProject::SetSliderDataExternal(const size_t sliderIndex, const size_t dataIndex, const std::vector<std::string>& dataFolders, std::string* errorMessage) {
	return SetSliderDataExternal(sliderIndex, dataIndex, dataFolders, std::string(), errorMessage);
}

bool OutfitProject::SetSliderDataExternal(const size_t sliderIndex, const size_t dataIndex, const std::vector<std::string>& dataFolders, const std::string& osdFileName, std::string* errorMessage) {
	std::vector<std::pair<size_t, size_t>> dataEntries;
	dataEntries.emplace_back(sliderIndex, dataIndex);
	return SetSliderDataExternal(dataEntries, dataFolders, osdFileName, errorMessage);
}

bool OutfitProject::SetSliderDataExternal(const std::vector<SliderDataKey>& dataKeys, const std::vector<std::string>& dataFolders, std::string* errorMessage) {
	return SetSliderDataExternal(dataKeys, dataFolders, std::string(), errorMessage);
}

bool OutfitProject::SetSliderDataExternal(const std::vector<SliderDataKey>& dataKeys, const std::vector<std::string>& dataFolders, const std::string& osdFileName, std::string* errorMessage) {
	std::vector<std::pair<size_t, size_t>> dataEntries;
	dataEntries.reserve(dataKeys.size());

	for (auto& dataKey : dataKeys) {
		size_t sliderIndex = 0;
		size_t dataIndex = 0;
		if (!ResolveSliderDataEntry(dataKey, sliderIndex, dataIndex)) {
			if (errorMessage)
				*errorMessage = "Slider data entry no longer exists.";
			return false;
		}

		dataEntries.emplace_back(sliderIndex, dataIndex);
	}

	return SetSliderDataExternal(dataEntries, dataFolders, osdFileName, errorMessage);
}

bool OutfitProject::SetSliderDataExternal(const std::vector<std::pair<size_t, size_t>>& dataEntries, const std::vector<std::string>& dataFolders, std::string* errorMessage) {
	return SetSliderDataExternal(dataEntries, dataFolders, std::string(), errorMessage);
}

bool OutfitProject::SetSliderDataExternal(const std::vector<std::pair<size_t, size_t>>& dataEntries, const std::vector<std::string>& dataFolders, const std::string& osdFileName, std::string* errorMessage) {
	std::vector<std::string> checkedFolders;
	for (auto& dataFolder : dataFolders)
		if (!dataFolder.empty())
			checkedFolders.push_back(dataFolder);

	std::string checkedOSDFileName = ToOSSlashes(osdFileName);
	if (!checkedOSDFileName.empty() && !SliderDataFileNameIsOSD(checkedOSDFileName)) {
		if (errorMessage)
			*errorMessage = "Enter an .osd file name.";
		return false;
	}

	if (checkedFolders.empty()) {
		if (errorMessage)
			*errorMessage = "Enter at least one shape data folder.";
		return false;
	}

	if (dataEntries.empty()) {
		if (errorMessage)
			*errorMessage = "Select at least one slider data entry.";
		return false;
	}

	struct CheckedDataEntry {
		size_t sliderIndex = 0;
		size_t dataIndex = 0;
		std::string fileName;
	};

	std::vector<CheckedDataEntry> checkedEntries;
	std::vector<std::string> targetNames;
	for (auto& dataEntry : dataEntries) {
		DiffInfo* selectedDataFile = activeSet.GetSliderDataFile(dataEntry.first, dataEntry.second);
		if (!selectedDataFile) {
			if (errorMessage)
				*errorMessage = "Slider data entry no longer exists.";
			return false;
		}

		bool duplicateEntry = false;
		for (auto& checkedEntry : checkedEntries) {
			if (checkedEntry.sliderIndex == dataEntry.first && checkedEntry.dataIndex == dataEntry.second) {
				duplicateEntry = true;
				break;
			}
		}
		if (!duplicateEntry) {
			CheckedDataEntry checkedEntry;
			checkedEntry.sliderIndex = dataEntry.first;
			checkedEntry.dataIndex = dataEntry.second;
			checkedEntry.fileName = BuildSliderDataFileName(*selectedDataFile, checkedOSDFileName);
			checkedEntries.push_back(std::move(checkedEntry));
		}

		bool knownTarget = false;
		for (auto& targetName : targetNames) {
			if (targetName == selectedDataFile->targetName) {
				knownTarget = true;
				break;
			}
		}
		if (!knownTarget)
			targetNames.push_back(selectedDataFile->targetName);
	}

	auto targetSelected = [&](const std::string& targetName) {
		for (auto& selectedTarget : targetNames)
			if (selectedTarget == targetName)
				return true;

		return false;
	};

	auto entrySelected = [&](size_t sliderIndex, size_t dataIndex) {
		for (auto& checkedEntry : checkedEntries)
			if (checkedEntry.sliderIndex == sliderIndex && checkedEntry.dataIndex == dataIndex)
				return true;

		return false;
	};

	auto selectedFileName = [&](size_t sliderIndex, size_t dataIndex) -> const std::string* {
		for (auto& checkedEntry : checkedEntries)
			if (checkedEntry.sliderIndex == sliderIndex && checkedEntry.dataIndex == dataIndex)
				return &checkedEntry.fileName;

		return nullptr;
	};

	for (size_t curSliderIndex = 0; curSliderIndex < activeSet.size(); curSliderIndex++) {
		for (size_t curDataIndex = 0; curDataIndex < activeSet[curSliderIndex].dataFiles.size(); curDataIndex++) {
			auto& dataFile = activeSet[curSliderIndex].dataFiles[curDataIndex];
			if (!targetSelected(dataFile.targetName))
				continue;

			bool selectedEntry = entrySelected(curSliderIndex, curDataIndex);
			if (dataFile.bLocal && !selectedEntry)
				continue;

			DiffInfo resolvedDataFile = dataFile;
			if (selectedEntry)
				resolvedDataFile.bLocal = false;

			if (const std::string* fileName = selectedFileName(curSliderIndex, curDataIndex))
				resolvedDataFile.fileName = *fileName;

			SliderDataFileResolution resolution = activeSet.ResolveSliderDataFile(resolvedDataFile, &checkedFolders);
			if (!resolution.resolved) {
				if (errorMessage) {
					*errorMessage = "Could not find slider data for '";
					*errorMessage += activeSet[curSliderIndex].name;
					*errorMessage += "' / '";
					*errorMessage += dataFile.dataName;
					*errorMessage += "' in the selected data folder(s).";
				}
				return false;
			}
		}
	}

	for (auto& targetName : targetNames)
		activeSet.SetTargetDataFolders(targetName, checkedFolders);

	for (auto& dataEntry : checkedEntries) {
		activeSet.SetSliderDataFileName(dataEntry.sliderIndex, dataEntry.dataIndex, dataEntry.fileName);
		activeSet.SetSliderDataFileLocal(dataEntry.sliderIndex, dataEntry.dataIndex, false);
		if (DiffInfo* dataFile = activeSet.GetSliderDataFile(dataEntry.sliderIndex, dataEntry.dataIndex))
			dataFile->bInherited = false;
	}

	return true;
}
