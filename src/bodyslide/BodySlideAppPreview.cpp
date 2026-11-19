/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

// BodySlide: the preview of the active outfit

#include "BodySlideApp.h"
#include "BodySlideFrame.h"
#include "ui/bodyslide/PreviewPanel.h"
#include "PreviewWindow.h"
#include "components/ClippingFixer.h"
#include "components/Mesh.h"
#include "files/GameDataStream.h"
#include "files/SFMorphFile.h"
#include "utils/PlatformUtil.h"
#include "utils/StringStuff.h"
#include "utils/ProjectUtil.h"

#include <algorithm>
#include <mutex>
#include <set>
#include <thread>
#include <unordered_map>
#include <utility>
#include <wx/filename.h>

using namespace nifly;

void BodySlideApp::CopyPreviewWeightToSliders() {
	if (!preview || !sliderView)
		return;

	int weight = preview->GetWeight();
	wxLogMessage("Copying preview shape at weight %d to both high and low sliders.", weight);

	for (size_t i = 0; i < sliderManager.slidersBig.size(); i++) {
		Slider* sliderBig = &sliderManager.slidersBig[i];
		Slider* sliderSmall = &sliderManager.slidersSmall[i];

		if (sliderBig->zap || sliderBig->clamp || sliderBig->toggle)
			continue;

		// Calculate the interpolated slider value at the current weight
		float effectiveValue = (sliderBig->value * weight + sliderSmall->value * (100.0f - weight)) / 100.0f;

		if (sliderView->GetSliderDisplay(sliderBig->name)) {
			sliderView->SetSliderPosition(sliderBig->name.c_str(), effectiveValue, SLIDER_HI);
			SetSliderValue(sliderBig->name, false, effectiveValue);
			SetSliderChanged(sliderBig->name, false);

			sliderView->SetSliderPosition(sliderSmall->name.c_str(), effectiveValue, SLIDER_LO);
			SetSliderValue(sliderSmall->name, true, effectiveValue);
			SetSliderChanged(sliderSmall->name, true);
		}
	}

	UpdateToggledSliders();
	sliderView->SetPresetChanged();
	UpdatePreview();
}

void BodySlideApp::ShowPreview() {
	// For standalone mode (cmd preview, or when no main frame exists)
	if (preview)
		return;

	int x = BodySlideConfig.GetIntValue("PreviewFrame.x");
	int y = BodySlideConfig.GetIntValue("PreviewFrame.y");
	int w = BodySlideConfig.GetIntValue("PreviewFrame.width");
	int h = BodySlideConfig.GetIntValue("PreviewFrame.height");
	std::string maximized = BodySlideConfig["PreviewFrame.maximized"];

	previewWindow = new PreviewWindow(wxPoint(x, y), wxSize(w, h), this);
	if (maximized == "true")
		previewWindow->Maximize();

	preview = previewWindow->GetPanel();

	// Set base data path for texture loading
	std::string baseGamePath = Config["GameDataPath"];
	preview->SetBaseDataPath(baseGamePath);
}

void BodySlideApp::InitPreviewPanel() {
	// Set up the embedded preview panel in the main frame
	if (sliderView && sliderView->previewPanel) {
		preview = sliderView->previewPanel;
		std::string baseGamePath = Config["GameDataPath"];
		preview->SetBaseDataPath(baseGamePath);
	}
}

void BodySlideApp::ClosePreview() {
	if (previewWindow) {
		previewWindow->Close();
		// PreviewClosed() will be called from OnClose
	}
}

void BodySlideApp::PreviewClosed() {
	// Called when the standalone preview window is closed
	previewWindow = nullptr;
	// If the main frame has an embedded panel, keep using it
	if (sliderView && sliderView->previewPanel) {
		preview = sliderView->previewPanel;
	}
	else {
		preview = nullptr;
	}
}

void BodySlideApp::PopOutPreview() {
	if (!sliderView || !sliderView->previewPanel || previewWindow)
		return;

	PreviewPanel* panel = sliderView->previewPanel;

	int x = BodySlideConfig.GetIntValue("PreviewFrame.x");
	int y = BodySlideConfig.GetIntValue("PreviewFrame.y");
	int w = BodySlideConfig.GetIntValue("PreviewFrame.width");
	int h = BodySlideConfig.GetIntValue("PreviewFrame.height");
	std::string maximized = BodySlideConfig["PreviewFrame.maximized"];

	// Unsplit: remove panel from splitter and shrink the main frame
	sliderView->UnsplitPreview();

	// Reparent into standalone window
	previewWindow = new PreviewWindow(wxPoint(x, y), wxSize(w, h), this, panel);
	if (maximized == "true")
		previewWindow->Maximize();

	preview = panel;
	bool previewAlwaysDetached = BodySlideConfig.GetBoolValue("BodySlideFrame.previewAlwaysDetached", false);
	panel->SetPopoutButtonDetachedState(true);
	panel->ShowPopoutButton(!previewAlwaysDetached);

	sliderView->previewVisible = true;
	BodySlideConfig.SetBoolValue("BodySlideFrame.previewVisible", true);
	BodySlideConfig.SetBoolValue("BodySlideFrame.previewPoppedOut", true);
	dockPoppedOutOnClose = false;
	sliderView->UpdatePreviewButtonLabel();
}

void BodySlideApp::DockPreview(bool attachToMain, bool preservePoppedOutState) {
	if (!previewWindow || !sliderView)
		return;

	PreviewPanel* panel = previewWindow->ReleasePanel();
	if (!panel)
		return;

	// Reparent back into splitter
	panel->Reparent(sliderView->splitter);
	sliderView->previewPanel = panel;
	preview = panel;

	if (attachToMain) {
		sliderView->SplitPreview(panel);
		sliderView->previewVisible = true;
		panel->SetPopoutButtonDetachedState(false);
		panel->ShowPopoutButton(true);
	}
	else {
		panel->Hide();
		sliderView->previewVisible = false;
		panel->SetPopoutButtonDetachedState(false);
		panel->ShowPopoutButton(true);
	}

	BodySlideConfig.SetBoolValue("BodySlideFrame.previewVisible", sliderView->previewVisible);
	BodySlideConfig.SetBoolValue("BodySlideFrame.previewPoppedOut", !attachToMain && preservePoppedOutState);
	sliderView->UpdatePreviewButtonLabel();
	panel->Layout();

	previewWindow->Destroy();
	previewWindow = nullptr;
	dockPoppedOutOnClose = false;
}

bool BodySlideApp::IsPreviewRenderable() const {
	if (!preview)
		return false;

	if (previewWindow)
		return previewWindow->IsShownOnScreen() && preview->IsShownOnScreen();

	if (sliderView)
		return sliderView->previewVisible && preview->IsShownOnScreen();

	return preview->IsShownOnScreen();
}

void BodySlideApp::InitPreview() {
	if (!IsPreviewRenderable())
		return;

	if (projects.empty())
		return;

	// Cancel any in-flight load
	uint64_t gen = ++previewLoadGeneration;

	if (previewLoadThread.joinable())
		previewLoadThread.join();

	preview->ShowLoadingIndicator(true);
	previewLoading = true;

	// Capture per-project info for the background thread
	struct ProjectInfo {
		size_t index;
		std::string inputFileName;
		std::string setName;
		SliderSet sliderSetCopy;
		bool needsReload;
	};

	auto projectInfos = std::make_shared<std::vector<ProjectInfo>>();
	for (size_t i = 0; i < projects.size(); ++i) {
		auto& pp = projects[i];
		ProjectInfo info;
		info.index = i;
		info.inputFileName = pp->sliderSet.GetInputFileName();
		info.setName = pp->sliderSet.GetName();
		info.sliderSetCopy = pp->sliderSet;

		if (multiProjectMode) {
			// Multi-project always reloads
			info.needsReload = true;
		}
		else {
			// Single-project: skip if NIF hasn't changed
			info.needsReload = !pp->baseNif || pp->inputFileName != info.inputFileName || pp->setName != info.setName || sliderManager.NeedReload();
		}

		projectInfos->push_back(std::move(info));
	}

	auto extraNifPaths = multiProjectMode ? preview->GetExtraNifPaths() : std::vector<std::string>{};
	bool isMultiProject = multiProjectMode;
	PreviewPanel* targetPreview = preview;
	std::string dataPath = Config["GameDataPath"];

	previewLoadThread = std::thread([this, gen, projectInfos, extraNifPaths, isMultiProject, targetPreview, dataPath]() {
		struct ProjectResult {
			size_t index;
			nifly::NifFile* baseNif = nullptr;
			nifly::NifFile modNif;
			bool loaded = false;
		};

		std::vector<ProjectResult> results(projectInfos->size());

		for (size_t i = 0; i < projectInfos->size(); ++i) {
			if (previewLoadGeneration.load() != gen) {
				for (size_t j = 0; j < i; ++j)
					delete results[j].baseNif;
				return;
			}

			auto& info = (*projectInfos)[i];
			auto& result = results[i];
			result.index = info.index;

			if (!info.needsReload)
				continue;

			result.baseNif = new nifly::NifFile();
			std::fstream file;
			PlatformUtil::OpenFileStream(file, info.inputFileName, std::ios::in | std::ios::binary);
			if (result.baseNif->Load(file)) {
				delete result.baseNif;
				result.baseNif = nullptr;
				continue;
			}

			if (result.baseNif->GetHeader().GetVersion().IsSF())
				for (const auto& meshPath : GameDataStream::LoadExternalGeometry(*result.baseNif, dataPath, info.inputFileName))
					wxLogWarning("Unable to locate external mesh data '%s' of nif file: %s", meshPath, info.inputFileName);

			result.modNif.CopyFrom(*result.baseNif);

			DiffDataSets dataSets;
			info.sliderSetCopy.LoadSetDiffData(dataSets);

			// Build preview mesh (apply sliders, zap verts)
			bool keepZappedShapes = info.sliderSetCopy.KeepZappedShapes();
			std::vector<nifly::Vector3> verts;
			std::vector<nifly::Vector2> uvs;
			std::vector<uint16_t> zapIdx;
			for (auto it = info.sliderSetCopy.ShapesBegin(); it != info.sliderSetCopy.ShapesEnd(); ++it) {
				zapIdx.clear();
				auto shape = result.baseNif->FindBlockByName<nifly::NiShape>(it->first);
				if (!result.baseNif->GetVertsForShape(shape, verts))
					continue;

				result.baseNif->GetUvsForShape(shape, uvs);
				ApplySliders(it->second.targetShape, sliderManager.slidersBig, dataSets, verts, zapIdx, &uvs);

				shape = result.modNif.FindBlockByName<nifly::NiShape>(it->first);
				if (zapIdx.size() > 0) {
					result.modNif.SetVertsForShape(shape, verts);
					result.modNif.SetUvsForShape(shape, uvs);
					if (result.modNif.DeleteVertsForShape(shape, zapIdx) && !keepZappedShapes)
						result.modNif.DeleteShape(shape);
				}
				else {
					result.modNif.SetVertsForShape(shape, verts);
					result.modNif.SetUvsForShape(shape, uvs);
				}
			}
			result.loaded = true;
		}

		if (previewLoadGeneration.load() != gen) {
			for (auto& r : results)
				delete r.baseNif;
			return;
		}

		CallAfter([this, gen, targetPreview, results = std::move(results), projectInfos, extraNifPaths, isMultiProject]() mutable {
			if (previewLoadGeneration.load() != gen) {
				for (auto& r : results)
					delete r.baseNif;
				return;
			}

			// Verify the target panel is still the active preview.
			// Prevents stale callbacks from writing to the wrong panel
			// (e.g. conflicts preview closed while async load was in-flight).
			if (!IsPreviewRenderable() || !preview || preview != targetPreview) {
				for (auto& r : results)
					delete r.baseNif;
				previewLoading = false;
				return;
			}

			// The skinning and simulation describe the NIFs about to be replaced
			ReleasePreviewSkinning();

			bool anyGenWeights = false;
			std::string baseGamePath = Config["GameDataPath"];
			preview->SetBaseDataPath(baseGamePath);

			for (size_t i = 0; i < results.size() && i < projects.size(); ++i) {
				auto& result = results[i];
				auto& pp = projects[result.index];
				auto& info = (*projectInfos)[i];

				if (result.loaded) {
					delete pp->baseNif;
					pp->baseNif = result.baseNif;
					result.baseNif = nullptr; // Ownership transferred
					pp->modNif = std::move(result.modNif);
					pp->inputFileName = info.inputFileName;
					pp->setName = info.setName;
				}

				if (pp->sliderSet.GenWeights())
					anyGenWeights = true;

				pp->sliderSet.LoadSetDiffData(pp->dataSets);
				preview->AddMeshFromNif(&pp->modNif);

				for (auto& s : pp->modNif.GetShapeNames())
					preview->AddNifShapeTextures(&pp->modNif, s);

				UpdateMeshesFromSet(pp->sliderSet);
			}

			if (!isMultiProject)
				sliderManager.FlagReload(false);

			preview->ShowWeight(anyGenWeights);
			if (sliderView && !preview->IsReadOnlyMode())
				preview->ShowLockShapeButton(anyGenWeights);

			if (isMultiProject) {
				preview->LoadNifFiles(extraNifPaths);
			}
			else if (!projects.empty()) {
				// Single-project extras: normals gen layers, reference shape
				auto* pp = projects[0].get();
				preview->SetNormalsGenerationLayers(pp->sliderSet.GetNormalsGenLayers());

				bool hasBuiltInRef = false;
				std::string refInfoShape = pp->sliderSet.GetReferenceShapeName();
				if (!refInfoShape.empty() && pp->baseNif && pp->baseNif->FindBlockByName<nifly::NiShape>(refInfoShape))
					hasBuiltInRef = true;
				if (!hasBuiltInRef && pp->baseNif && ClippingFixer::FindReferenceShape(*pp->baseNif))
					hasBuiltInRef = true;

				if (hasBuiltInRef) {
					preview->ShowReferenceCheckbox(false);
					referenceNif.reset();
				}
				else {
					if (!referenceNif)
						LoadExternalReference(pp->sliderSet);

					if (referenceNif) {
						bool hasClippingFix = clippingFixStrength > 0.0f;
						preview->ShowReferenceCheckbox(true);
						if (hasClippingFix)
							preview->SetReferenceCheckboxState(true, false);
						else
							preview->SetReferenceCheckboxState(false, true);

						preview->AddMeshFromNif(referenceNif.get(), const_cast<char*>(referenceShapeName.c_str()));
						preview->AddNifShapeTextures(referenceNif.get(), referenceShapeName);
						preview->SetMeshVisibility(referenceShapeName, preview->IsShowReferenceChecked());
					}
					else {
						preview->ShowReferenceCheckbox(false);
					}
				}

				if (!hasBuiltInRef && !referenceNif) {
					preview->ShowReferenceCheckbox(false);
					referenceNif.reset();
				}
			}

			previewLoading = false;
			UpdatePreview();
			UpdatePreviewPhysicsAvailability();
			preview->ShowLoadingIndicator(false);
		});
	});
}

void BodySlideApp::LoadPreviewNifs(const std::vector<std::string>& filePaths) {
	if (!preview)
		return;

	// Parse entries: split each by '?' to get file path and optional set names
	struct ParsedEntry {
		std::string filePath;
		std::vector<std::string> setNames;
		bool isOsp = false;
	};

	std::vector<ParsedEntry> entries;
	bool hasExplicitSets = false;

	for (auto& rawPath : filePaths) {
		ParsedEntry entry;

		// Split by '?'
		size_t qPos = rawPath.find('?');
		if (qPos != std::string::npos) {
			entry.filePath = rawPath.substr(0, qPos);
			std::string remainder = rawPath.substr(qPos + 1);
			// Split set names by '?'
			size_t pos = 0;
			while (pos < remainder.size()) {
				size_t nextQ = remainder.find('?', pos);
				std::string setName;
				if (nextQ != std::string::npos) {
					setName = remainder.substr(pos, nextQ - pos);
					pos = nextQ + 1;
				}
				else {
					setName = remainder.substr(pos);
					pos = remainder.size();
				}
				if (!setName.empty())
					entry.setNames.push_back(setName);
			}
			hasExplicitSets = true;
		}
		else {
			entry.filePath = rawPath;
		}

		// Detect OSP extension
		size_t dotPos = entry.filePath.find_last_of('.');
		if (dotPos != std::string::npos) {
			std::string ext = ToLower(entry.filePath.substr(dotPos + 1));
			entry.isOsp = (ext == "osp");
		}

		entries.push_back(entry);
	}

	// Collect non-OSP file paths for extra NIF loading alongside projects
	std::vector<std::string> nifPaths;
	for (auto& e : entries) {
		if (!e.isOsp)
			nifPaths.push_back(e.filePath);
	}

	// Multi-project mode: explicit set names specified with '?'
	if (hasExplicitSets) {
		std::vector<PreviewProjectEntry> projEntries;

		for (auto& e : entries) {
			if (e.isOsp) {
				if (!e.setNames.empty()) {
					for (auto& setName : e.setNames)
						projEntries.push_back({e.filePath, setName});
				}
				else {
					// OSP without specific sets: ignore when other files have explicit sets
					wxLogWarning("Ignoring OSP file without specified set names: %s", e.filePath);
				}
			}
		}

		if (!projEntries.empty()) {
			wxLogMessage("Loading %zu combined project(s) in preview mode...", projEntries.size());
			if (!nifPaths.empty())
				preview->SetExtraNifPaths(nifPaths);
			preview->SetProjectData(projEntries, true, cmdPreset);
			return;
		}
	}

	// OSP files without explicit sets: show dropdown with all sets from all OSP files
	{
		std::vector<PreviewProjectEntry> projEntries;
		bool anyOsp = false;

		for (auto& e : entries) {
			if (!e.isOsp)
				continue;

			anyOsp = true;
			SliderSetFile sliderDoc;
			sliderDoc.Open(e.filePath);
			if (sliderDoc.fail()) {
				wxLogError("Failed to load BodySlide project file: %s", e.filePath);
				continue;
			}

			std::vector<std::string> setNames;
			sliderDoc.GetSetNamesUnsorted(setNames, false);
			for (auto& setName : setNames)
				projEntries.push_back({e.filePath, setName});
		}

		if (!projEntries.empty()) {
			wxLogMessage("Loading %zu slider set(s) from %zu OSP file(s)", projEntries.size(), entries.size());
			if (!nifPaths.empty())
				preview->SetExtraNifPaths(nifPaths);
			preview->SetProjectData(projEntries, false, cmdPreset);
			return;
		}

		if (anyOsp)
			return;
	}

	if (!cmdPreset.empty())
		wxLogWarning("Preset '%s' from the command line is ignored, presets only apply to project files.", cmdPreset);

	// Load as regular NIF files
	std::vector<std::string> paths;
	for (auto& e : entries)
		paths.push_back(e.filePath);
	preview->SetExtraNifPaths(paths);
}

void BodySlideApp::BuildPreviewMesh(ProjectData* pp, bool freshLoad) {
	bool keepZappedShapes = pp->sliderSet.KeepZappedShapes();

	std::vector<Vector3> verts;
	std::vector<Vector2> uvs;
	std::vector<uint16_t> zapIdx;
	for (auto it = pp->sliderSet.ShapesBegin(); it != pp->sliderSet.ShapesEnd(); ++it) {
		zapIdx.clear();

		auto shape = pp->baseNif->FindBlockByName<NiShape>(it->first);
		if (!pp->baseNif->GetVertsForShape(shape, verts))
			continue;

		pp->baseNif->GetUvsForShape(shape, uvs);

		ApplySliders(it->second.targetShape, sliderManager.slidersBig, pp->dataSets, verts, zapIdx, &uvs);

		// Zap deleted verts before preview
		shape = pp->modNif.FindBlockByName<NiShape>(it->first);
		if (freshLoad && zapIdx.size() > 0) {
			// Freshly loaded, need to actually delete verts and tris in the modified .nif
			pp->modNif.SetVertsForShape(shape, verts);
			pp->modNif.SetUvsForShape(shape, uvs);
			if (pp->modNif.DeleteVertsForShape(shape, zapIdx) && !keepZappedShapes)
				pp->modNif.DeleteShape(shape);
		}
		else if (zapIdx.size() > 0) {
			// Preview Window has been opened for this shape before, zap the diff verts before applying them to the shape
			for (int z = zapIdx.size() - 1; z >= 0; z--) {
				if (zapIdx[z] >= verts.size())
					continue;
				verts.erase(verts.begin() + zapIdx[z]);
				uvs.erase(uvs.begin() + zapIdx[z]);
			}
			pp->modNif.SetVertsForShape(shape, verts);
			pp->modNif.SetUvsForShape(shape, uvs);
		}
		else {
			// No zapping needed - just show all the verts.
			pp->modNif.SetVertsForShape(shape, verts);
			pp->modNif.SetUvsForShape(shape, uvs);
		}
	}
}

void BodySlideApp::ApplyClippingFix(NifFile& nif,
									const std::vector<Vector3>& bodyVerts,
									const std::vector<Triangle>& bodyTris,
									std::unordered_map<std::string, std::vector<Vector3>*>& shapeVerts) {
	ClippingFixOptions fixOpts;
	fixOpts.strength = clippingFixStrength / 100.0f;

	for (auto& [shapeName, vertsPtr] : shapeVerts) {
		if (!vertsPtr || vertsPtr->empty())
			continue;

		auto shape = nif.FindBlockByName<NiShape>(shapeName);
		if (!ClippingFixer::IsEligibleForFix(nif, shape))
			continue;

		std::vector<Triangle> outfitTris;
		shape->GetTriangles(outfitTris);
		if (outfitTris.empty())
			continue;

		ClippingFixer::FixClipping(bodyVerts, bodyTris, *vertsPtr, outfitTris, fixOpts);
	}
}

bool BodySlideApp::LoadExternalReference(const SliderSet& sliderSet) {
	if (!sliderSet.HasExternalReferenceInfo())
		return false;

	std::string projectFile = sliderSet.GetReferenceProjectFile();
	std::string projectName = sliderSet.GetReferenceProjectName();
	referenceShapeName = sliderSet.GetReferenceShapeName();

	// Resolve project file path relative to project directory
	wxFileName refProjectFileName(wxString::FromUTF8(projectFile));
	if (refProjectFileName.IsRelative())
		refProjectFileName.MakeAbsolute(wxString::FromUTF8(ProjectUtil::GetProjectPath()));

	std::string projectFilePath = refProjectFileName.GetFullPath().ToUTF8().data();

	// Load the slider set from the referenced project file
	SliderSetFile refSSF(projectFilePath);
	if (refSSF.fail()) {
		wxLogWarning("Could not load reference project file: %s", projectFilePath);
		return false;
	}

	referenceSliderSet.Clear();
	if (refSSF.GetSet(projectName, referenceSliderSet)) {
		wxLogWarning("Could not find reference project '%s' in file: %s", projectName, projectFilePath);
		return false;
	}

	referenceSliderSet.SetBaseDataPath(ProjectUtil::GetProjectPath() + PathSepStr + "ShapeData");

	// The reference body follows the sliders of its add-ons too
	ProjectAddOns referenceAddOns;
	AddProjectAddOns(referenceSliderSet, false, referenceAddOns, nullptr, projectFilePath);
	LogProjectAddOns(referenceSliderSet, referenceAddOns);

	// Load diff data for the reference shape
	referenceDiffData.Clear();
	referenceSliderSet.LoadSetDiffData(referenceDiffData, referenceShapeName);

	// Load the NIF file from the referenced project
	std::string refInputFile = referenceSliderSet.GetInputFileName();
	std::fstream file;
	PlatformUtil::OpenFileStream(file, refInputFile, std::ios::in | std::ios::binary);

	referenceNif = std::make_unique<NifFile>();
	if (referenceNif->Load(file)) {
		wxLogWarning("Could not load reference NIF file: %s", refInputFile);
		referenceNif.reset();
		return false;
	}

	if (referenceNif->GetHeader().GetVersion().IsSF())
		for (const auto& meshPath : GameDataStream::LoadExternalGeometry(*referenceNif, Config["GameDataPath"], refInputFile))
			wxLogWarning("Unable to locate external mesh data '%s' of nif file: %s", meshPath, refInputFile);

	// Verify the shape exists
	auto refShape = referenceNif->FindBlockByName<NiShape>(referenceShapeName);
	if (!refShape) {
		wxLogWarning("Reference shape '%s' not found in NIF: %s", referenceShapeName, refInputFile);
		referenceNif.reset();
		return false;
	}

	return true;
}

void BodySlideApp::UpdateReferenceCheckboxState() {
	if (!preview || !referenceNif || multiProjectMode)
		return;

	bool hasClippingFix = clippingFixStrength > 0.0f;
	if (hasClippingFix) {
		preview->SetReferenceCheckboxState(true, false);
	}
	else {
		preview->SetReferenceCheckboxState(false, true);
	}
}

void BodySlideApp::UpdatePreview() {
	if (previewLoading)
		return;

	if (!IsPreviewRenderable())
		return;

	if (projects.empty())
		return;

	int weight = preview->GetWeight();
	auto shapeData = ComputeMorphedShapeData(weight);
	PostProcessPreview(shapeData, weight);
}

std::vector<ShapePreviewData> BodySlideApp::ComputeMorphedShapeData(int weight) {
	std::vector<ShapePreviewData> shapeData;
	std::vector<Vector3> verts, vertsLow, vertsHigh;
	std::vector<Vector2> uvs, uvsLow, uvsHigh;
	std::vector<uint16_t> zapIdx;

	for (int pi = 0; pi < static_cast<int>(projects.size()); pi++) {
		auto& pp = projects[pi];
		if (!pp->baseNif)
			continue;

		for (auto it = pp->sliderSet.ShapesBegin(); it != pp->sliderSet.ShapesEnd(); ++it) {
			zapIdx.clear();

			auto shape = pp->baseNif->FindBlockByName<NiShape>(it->first);
			if (!pp->baseNif->GetVertsForShape(shape, verts))
				continue;

			pp->baseNif->GetUvsForShape(shape, uvs);
			vertsHigh = verts;
			vertsLow = verts;
			uvsHigh = uvs;
			uvsLow = uvs;

			ApplySliders(it->second.targetShape, sliderManager.slidersBig, pp->dataSets, vertsHigh, zapIdx, &uvsHigh);
			if (pp->sliderSet.GenWeights())
				ApplySliders(it->second.targetShape, sliderManager.slidersSmall, pp->dataSets, vertsLow, zapIdx, &uvsLow);

			// Calculate result of weight
			auto uvsz = uvs.size();
			for (size_t i = 0; i < verts.size(); i++) {
				verts[i] = (vertsHigh[i] / 100.0f * weight) + (vertsLow[i] / 100.0f * (100.0f - weight));
				if (uvsz > i)
					uvs[i] = (uvsHigh[i] / 100.0f * weight) + (uvsLow[i] / 100.0f * (100.0f - weight));
			}

			ShapePreviewData spd;
			spd.name = it->first;
			spd.verts = std::move(verts);
			spd.uvs = std::move(uvs);
			spd.zapIdx = zapIdx;
			spd.projectIdx = pi;
			shapeData.push_back(std::move(spd));
		}
	}

	return shapeData;
}

void BodySlideApp::PostProcessPreview(std::vector<ShapePreviewData>& shapeData, int weight) {
	// A pose or the simulation moves the vertices by their bones, after
	// everything else was done to them
	const bool skinned = IsPreviewSkinned() && EnsurePreviewSkinning();
	if (!skinned) {
		previewMorphedVerts.clear();
		previewReferenceMorphedVerts.clear();
	}

	// Apply clipping fix and handle external reference
	bool useExternalReference = !multiProjectMode && referenceNif && preview &&
							  (clippingFixStrength > 0.0f || preview->IsShowReferenceChecked());

	std::vector<Vector3> extRefVerts;
	if (useExternalReference)
		UpdateExternalReferenceMesh(weight, &extRefVerts);

	// Hide external reference mesh when not in use
	if (!useExternalReference && referenceNif && preview) {
		preview->SetMeshVisibility(referenceShapeName, false);
		previewReferenceMorphedVerts.clear();
	}

	if (clippingFixStrength > 0.0f) {
		for (int pi = 0; pi < static_cast<int>(projects.size()); pi++) {
			auto& pp = projects[pi];
			if (!pp->baseNif)
				continue;

			// Try built-in reference shape first
			auto refShape = ClippingFixer::FindReferenceShape(*pp->baseNif);
			const std::vector<Vector3>* bodyVerts = nullptr;
			std::vector<Triangle> bodyTris;

			if (refShape) {
				// Use built-in reference shape
				ShapePreviewData* refData = nullptr;
				for (auto& sd : shapeData) {
					if (sd.projectIdx == pi && sd.name == refShape->name.get()) {
						refData = &sd;
						break;
					}
				}
				if (refData && !refData->verts.empty()) {
					refShape->GetTriangles(bodyTris);
					bodyVerts = &refData->verts;
				}
			}

			// Fall back to external reference if no built-in reference
			if (!bodyVerts && useExternalReference && pi == 0 && !extRefVerts.empty()) {
				auto extRefShape = referenceNif->FindBlockByName<NiShape>(referenceShapeName);
				if (extRefShape) {
					extRefShape->GetTriangles(bodyTris);
					bodyVerts = &extRefVerts;
					refShape = extRefShape;
				}
			}

			if (!bodyVerts || bodyTris.empty())
				continue;

			std::string refShapeName = refShape->name.get();
			std::unordered_map<std::string, std::vector<Vector3>*> shapeVerts;
			for (auto& sd : shapeData) {
				if (sd.projectIdx == pi && sd.name != refShapeName)
					shapeVerts[sd.name] = &sd.verts;
			}

			ApplyClippingFix(*pp->baseNif, *bodyVerts, bodyTris, shapeVerts);
		}
	}

	// Phase 3: Zap and update meshes
	std::vector<Matrix3> poseRotations;
	for (auto& sd : shapeData) {
		if (sd.zapIdx.size() > 0) {
			for (int z = sd.zapIdx.size() - 1; z >= 0; z--) {
				if (sd.zapIdx[z] >= sd.verts.size())
					continue;

				sd.verts.erase(sd.verts.begin() + sd.zapIdx[z]);
				sd.uvs.erase(sd.uvs.begin() + sd.zapIdx[z]);
			}
		}

		if (skinned) {
			// Remember the morphed shape so a physics or animation tick can skin
			// it again without running all sliders, then show it skinned
			previewMorphedVerts[sd.name] = sd.verts;

			// Collide with the shape the sliders describe, not the base shape
			for (auto& physics : previewPhysics)
				if (physics.projectIdx == static_cast<size_t>(sd.projectIdx))
					physics.controller->SetShapeVertices(sd.name, sd.verts);

			ApplyPreviewSkinning(sd.projectIdx, sd.name, sd.verts, &poseRotations);
		}
		else
			poseRotations.clear();

		preview->UpdateMeshes(sd.name, &sd.verts, &sd.uvs, &poseRotations);
	}

	preview->Render();
}

void BodySlideApp::CleanupPreview() {
	if (!preview)
		return;

	// The pose and animation stay selected for the next preview, but nothing
	// keeps moving without one
	previewAnimPlaying = false;
	ReleasePreviewSkinning();
	previewPhysicsAvailable = false;

	// Cancel async load and wait for it to finish
	++previewLoadGeneration;
	if (previewLoadThread.joinable())
		previewLoadThread.join();
	previewLoading = false;

	preview->Cleanup();
	preview->ShowLoadingIndicator(false);
	referenceNif.reset();
	previewLooseNifs.clear();

	if (multiProjectMode) {
		projects.clear();
		multiProjectMode = false;
	}
	else if (!projects.empty()) {
		auto* pp = projects[0].get();
		delete pp->baseNif;
		pp->baseNif = nullptr;
		pp->inputFileName.clear();
	}
}

void BodySlideApp::RebuildPreviewMeshes() {
	if (!IsPreviewRenderable())
		return;

	if (projects.empty())
		return;

	int weight = preview->GetWeight();
	auto shapeData = ComputeMorphedShapeData(weight);

	// The skinning and simulation describe the NIFs about to be rewritten
	const bool physicsWasRunning = previewPhysicsRunning;
	ReleasePreviewSkinning();

	// Multi-project mode
	if (multiProjectMode) {
		for (auto& pp : projects) {
			if (!pp->baseNif)
				continue;

			pp->modNif.CopyFrom(*pp->baseNif);

			bool keepZappedShapes = pp->sliderSet.KeepZappedShapes();

			for (auto it = pp->sliderSet.ShapesBegin(); it != pp->sliderSet.ShapesEnd(); ++it) {
				// Find the matching shape data
				ShapePreviewData* spd = nullptr;
				for (auto& sd : shapeData) {
					if (sd.name == it->first) {
						spd = &sd;
						break;
					}
				}
				if (!spd)
					continue;

				// Zap deleted verts before preview
				auto shape = pp->modNif.FindBlockByName<NiShape>(it->first);
				if (spd->zapIdx.size() > 0) {
					pp->modNif.SetVertsForShape(shape, spd->verts);
					pp->modNif.SetUvsForShape(shape, spd->uvs);
					if (pp->modNif.DeleteVertsForShape(shape, spd->zapIdx) && !keepZappedShapes)
						pp->modNif.DeleteShape(shape);
				}
				else {
					pp->modNif.SetVertsForShape(shape, spd->verts);
					pp->modNif.SetUvsForShape(shape, spd->uvs);
				}
			}
		}

		// Refresh all meshes from all projects
		std::vector<NifFile*> modNifs;
		for (auto& pp : projects)
			modNifs.push_back(&pp->modNif);
		preview->RefreshMeshFromNif(modNifs);

		// Update mesh settings from all sets
		for (auto& pp : projects) {
			for (auto it = pp->sliderSet.ShapesBegin(); it != pp->sliderSet.ShapesEnd(); ++it) {
				Mesh* m = preview->GetMesh(it->first);
				if (m) {
					m->smoothSeamNormals = it->second.smoothSeamNormals;
					m->lockNormals = it->second.lockNormals;
					m->SmoothNormals();
				}
			}
		}

		PostProcessPreview(shapeData, weight);
		UpdatePreviewPhysicsAvailability(physicsWasRunning);
		return;
	}

	// Single-project mode
	auto* pp = projects[0].get();
	if (!pp->baseNif)
		return;

	pp->modNif.CopyFrom(*pp->baseNif);

	bool keepZappedShapes = pp->sliderSet.KeepZappedShapes();

	for (auto& sd : shapeData) {
		auto shape = pp->modNif.FindBlockByName<NiShape>(sd.name);
		if (!shape)
			continue;

		if (sd.zapIdx.size() > 0) {
			pp->modNif.SetVertsForShape(shape, sd.verts);
			pp->modNif.SetUvsForShape(shape, sd.uvs);
			if (pp->modNif.DeleteVertsForShape(shape, sd.zapIdx) && !keepZappedShapes)
				pp->modNif.DeleteShape(shape);
		}
		else {
			pp->modNif.SetVertsForShape(shape, sd.verts);
			pp->modNif.SetUvsForShape(shape, sd.uvs);
		}
	}

	preview->RefreshMeshFromNif({&pp->modNif});
	UpdateMeshesFromSet(pp->sliderSet);

	// Re-add external reference mesh after refresh (which clears all meshes)
	if (referenceNif && !multiProjectMode) {
		preview->AddMeshFromNif(referenceNif.get(), const_cast<char*>(referenceShapeName.c_str()));
		preview->AddNifShapeTextures(referenceNif.get(), referenceShapeName);
		preview->SetMeshVisibility(referenceShapeName, false);
	}

	PostProcessPreview(shapeData, weight);
	UpdatePreviewPhysicsAvailability(physicsWasRunning);
}

void BodySlideApp::UpdateExternalReferenceMesh(int weight, std::vector<Vector3>* outVerts) {
	if (!preview || !referenceNif)
		return;

	std::vector<Vector3> extRefVerts;
	std::vector<Vector2> extRefUvs;
	auto refShape = referenceNif->FindBlockByName<NiShape>(referenceShapeName);
	if (!refShape || !referenceNif->GetVertsForShape(refShape, extRefVerts))
		return;

	referenceNif->GetUvsForShape(refShape, extRefUvs);

	std::string targetName = referenceSliderSet.ShapeToTarget(referenceShapeName);
	if (!targetName.empty()) {
		// Build slider vectors from the reference project's slider set.
		// We can't use sliderManager.slidersBig/slidersSmall directly because
		// their linkedDataSets contain data names from the outfit project,
		// which don't match the data names in referenceDiffData.
		std::vector<Slider> refSlidersBig;
		std::vector<Slider> refSlidersSmall;
		for (size_t si = 0; si < referenceSliderSet.size(); si++) {
			auto& sd = referenceSliderSet[si];
			Slider s = Slider::FromData(sd);
			s.linkedDataSets.clear();
			for (auto& df : sd.dataFiles)
				if (df.targetName == targetName)
					s.linkedDataSets.push_back(df.dataName);

			s.value = sliderManager.GetSlider(sd.name, false);
			refSlidersBig.push_back(s);

			s.value = sliderManager.GetSlider(sd.name, true);
			refSlidersSmall.push_back(std::move(s));
		}

		std::vector<Vector3> refVertsHigh = extRefVerts;
		std::vector<Vector3> refVertsLow = extRefVerts;
		std::vector<Vector2> refUvsHigh = extRefUvs;
		std::vector<Vector2> refUvsLow = extRefUvs;
		std::vector<uint16_t> extZapIdx;

		ApplySliders(targetName, refSlidersBig, referenceDiffData, refVertsHigh, extZapIdx, &refUvsHigh);
		if (referenceSliderSet.GenWeights())
			ApplySliders(targetName, refSlidersSmall, referenceDiffData, refVertsLow, extZapIdx, &refUvsLow);

		auto uvsz = extRefUvs.size();
		for (size_t i = 0; i < extRefVerts.size(); i++) {
			extRefVerts[i] = (refVertsHigh[i] / 100.0f * weight) + (refVertsLow[i] / 100.0f * (100.0f - weight));
			if (uvsz > i)
				extRefUvs[i] = (refUvsHigh[i] / 100.0f * weight) + (refUvsLow[i] / 100.0f * (100.0f - weight));
		}
	}

	if (IsPreviewPosed() && previewAnimsBuilt) {
		// The clipping fix works on the unposed body, only the display is posed
		previewReferenceMorphedVerts = extRefVerts;

		std::vector<Vector3> skinnedVerts = extRefVerts;
		std::vector<Matrix3> poseRotations;
		ApplyPreviewReferenceSkinning(skinnedVerts, &poseRotations);
		preview->UpdateMeshes(referenceShapeName, &skinnedVerts, &extRefUvs, &poseRotations);
	}
	else {
		previewReferenceMorphedVerts.clear();
		preview->UpdateMeshes(referenceShapeName, &extRefVerts, &extRefUvs);
	}

	preview->SetMeshVisibility(referenceShapeName, true);

	if (outVerts)
		*outVerts = std::move(extRefVerts);
}

void BodySlideApp::UpdateMeshesFromSet(SliderSet& set) {
	for (auto it = set.ShapesBegin(); it != set.ShapesEnd(); ++it) {
		Mesh* m = preview->GetMesh(it->first);
		if (m) {
			m->smoothSeamNormals = it->second.smoothSeamNormals;
			m->lockNormals = it->second.lockNormals;

			m->SmoothNormals();
		}
	}
}

void BodySlideApp::ApplyReferenceNormals(NifFile& nif) {
	static std::mutex refNormalsCacheMutex;
	std::lock_guard<std::mutex> cacheLock(refNormalsCacheMutex);

	for (auto& s : nif.GetShapes()) {
		std::string shapeName = s->name.get();

		if (refNormalsCache.find(shapeName) != refNormalsCache.end()) {
			// Apply normals from file cache
			NifFile& srcNif = refNormalsCache[shapeName];
			nif.ApplyNormalsFromFile(srcNif, shapeName);
		}
		else {
			// Check if reference normals file exists
			wxString fileName = wxString::Format("%s/RefNormals/%s.nif", wxString::FromUTF8(Config["AppDir"]), wxString::FromUTF8(shapeName));
			if (wxFile::Exists(fileName)) {
				std::fstream file;
				PlatformUtil::OpenFileStream(file, fileName.ToUTF8().data(), std::ios::in | std::ios::binary);

				NifFile srcNif;
				if (srcNif.Load(file) != 0)
					continue;

				// Apply normals from file
				nif.ApplyNormalsFromFile(srcNif, shapeName);

				// Move file to cache
				refNormalsCache[shapeName] = std::move(srcNif);
			}
		}
	}
}
