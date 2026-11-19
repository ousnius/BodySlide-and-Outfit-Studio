/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

// BodySlide: loading projects with their add-ons, and the outfit list

#include "BodySlideApp.h"
#include "BodySlideFrame.h"
#include "files/GameDataStream.h"
#include "utils/PlatformUtil.h"
#include "utils/StringStuff.h"
#include "utils/ProjectUtil.h"

#include <wx/filename.h>
#include <wx/wx.h>
#include <wx/xrc/xmlres.h>
#include <algorithm>
#include <mutex>
#include <set>
#include <utility>
#include <wx/dir.h>

using namespace nifly;

void BodySlideApp::LogBaseProjectStatus(const SliderSet& set, NifFile* nif) {
	const SliderSetResolveStatus& status = set.GetResolveStatus();
	if (status.newerVersion)
		wxLogWarning("Project '%s' was saved by a newer version of Outfit Studio.", set.GetName());

	for (auto& warning : status.warnings)
		wxLogWarning("Project '%s': %s", set.GetName(), wxString::FromUTF8(warning));

	if (!nif || !set.IsAddOn())
		return;

	for (auto* shape : nif->GetShapes()) {
		uint32_t vertexCount = set.GetShapeVertexCount(shape->name.get());
		if (vertexCount > 0 && vertexCount != shape->GetNumVertices())
			wxLogWarning("Add-on '%s': its slider data of shape '%s' was made for %u vertices, but the base project's shape has %u now.",
						 set.GetName(),
						 shape->name.get(),
						 vertexCount,
						 static_cast<uint32_t>(shape->GetNumVertices()));
	}
}

int BodySlideApp::AddProjectSliders(const std::string& projectFile, const std::string& setName) {
	wxLogMessage("Adding project sliders from '%s' set '%s'...", projectFile, setName);

	// Preview mode doesn't load the outfit list, which the add-ons are found in
	if (projectCatalog.Entries().empty())
		projectCatalog.Load(ProjectUtil::GetProjectPath());

	if (projects.empty()) {
		// First project: clear shared data managers
		sliderManager.ClearSliders();
		sliderManager.ClearPresets();
		multiProjectMode = true;
	}

	auto pp = std::make_unique<ProjectData>();

	if (conformAddOnSliders)
		EnsurePreviewSkeleton();

	ProjectAddOns addOns;
	int error = LoadProjectSet(projectFile, setName, pp->sliderSet, conformAddOnSliders, addOns);
	if (error)
		return error;

	LogBaseProjectStatus(pp->sliderSet);
	LogProjectAddOns(pp->sliderSet, addOns);
	pp->setName = setName;

	// What's shown for the add-ons is of the first project, like the rest of the window
	if (projects.empty())
		activeAddOns = addOns;

	// Add sliders from this set (additive)
	sliderManager.AddSlidersInSet(pp->sliderSet);

	projects.push_back(std::move(pp));
	return 0;
}

bool BodySlideApp::LoadNifFile(const std::string& fileName, NifFile& nif) {
	std::fstream file;
	PlatformUtil::OpenFileStream(file, fileName, std::ios::in | std::ios::binary);
	if (nif.Load(file))
		return false;

	if (nif.GetHeader().GetVersion().IsSF())
		for (const auto& meshPath : GameDataStream::LoadExternalGeometry(nif, Config["GameDataPath"], fileName))
			wxLogWarning("Unable to locate external mesh data '%s' of nif file: %s", meshPath, fileName);

	return true;
}

int BodySlideApp::LoadProjectSet(const std::string& fileName, const std::string& setName, SliderSet& outSet, bool conform, ProjectAddOns& outAddOns, NifFile* nif) {
	SliderSetFile sliderDoc(fileName);
	if (sliderDoc.fail())
		return 2;

	if (sliderDoc.GetSet(setName, outSet))
		return 3;

	outSet.SetBaseDataPath(ProjectUtil::GetProjectPath() + PathSepStr + "ShapeData");
	AddProjectAddOns(outSet, conform, outAddOns, nif, fileName);
	return 0;
}

void BodySlideApp::AddProjectAddOns(SliderSet& set, bool conform, ProjectAddOns& outAddOns, NifFile* nif, const std::string& setFile) {
	outAddOns = ProjectAddOns();

	SetStackResult stack = StackSetAddOns(set, projectCatalog, outAddOns.warnings, false, setFile);
	outAddOns.stacked = std::move(stack.addOns);
	outAddOns.baseStacked = stack.baseAddOns;
	outAddOns.missing = std::move(stack.missingAddOns);

	// Add-on sliders of the body that it keeps off outfits go onto its shapes in them, even without conforming
	ConformBodyRef bodyRef = FindConformBody(set, projectCatalog, ProjectUtil::GetProjectPath());
	std::shared_ptr<ConformBody> body;
	if (bodyRef.Found())
		body = GetConformBody(bodyRef, conform, outAddOns.warnings);

	if (stack.baseAddOns.empty() && !body)
		return;

	// Fitting and conforming need the shapes
	NifFile ownNif;
	if (!nif) {
		if (!LoadNifFile(set.GetInputFileName(), ownNif)) {
			outAddOns.warnings.push_back(wxString::Format(_("The NIF file couldn't be loaded to fit add-on sliders to it: %s"), wxString::FromUTF8(set.GetInputFileName())).ToUTF8().data());
			return;
		}

		nif = &ownNif;
	}

	AnimInfo anim;
	if (conform)
		LoadConformAnim(anim, *nif);

	const std::string baseDataPath = ProjectUtil::GetProjectPath() + PathSepStr + "ShapeData";
	auto loadNif = [](const std::string& fileName, NifFile& loadInto) { return LoadNifFile(fileName, loadInto); };

	if (!stack.baseAddOns.empty())
		FitBaseAddOnsToVariant(set, *nif, conform ? &anim : nullptr, stack.baseAddOns, projectCatalog, baseDataPath, loadNif, conform, outAddOns.warnings);

	if (!body)
		return;

	outAddOns.conformBody = bodyRef.setName;
	outAddOns.conformed = ConformAddOnSliders(set, *nif, anim, *body);
}

std::shared_ptr<ConformBody> BodySlideApp::GetConformBody(const ConformBodyRef& ref, bool conform, std::vector<std::string>& warnings) {
	std::string key = ref.fileName + "|" + ref.setName + "|" + ref.shapeName + (conform ? "" : "|noconform");

	// Outfits of the same body wait for it to be loaded once
	std::lock_guard<std::mutex> lock(conformBodiesMutex);
	auto loaded = conformBodies.find(key);
	if (loaded != conformBodies.end())
		return loaded->second;

	wxLogMessage("Loading body '%s' for its add-on sliders...", ref.setName);

	auto body = std::make_shared<ConformBody>();
	const std::string baseDataPath = ProjectUtil::GetProjectPath() + PathSepStr + "ShapeData";
	auto loadNif = [](const std::string& fileName, NifFile& loadInto) { return LoadNifFile(fileName, loadInto); };
	if (!body->Load(ref, projectCatalog, baseDataPath, loadNif, warnings, conform))
		body.reset();

	conformBodies[key] = body;
	return body;
}

void BodySlideApp::LogProjectAddOns(const SliderSet& set, const ProjectAddOns& addOns, NifFile* nif) {
	if (!nif) {
		for (auto& warning : addOns.warnings)
			wxLogWarning("Project '%s': %s", set.GetName(), wxString::FromUTF8(warning));

		std::vector<std::string> stackedNames;
		for (auto* list : {&addOns.stacked, &addOns.baseStacked})
			for (auto& addOn : *list)
				stackedNames.push_back(addOn.setName);
		if (!stackedNames.empty())
			wxLogMessage("Project '%s': added the sliders of add-ons: %s", set.GetName(), wxString::FromUTF8(JoinStrings(stackedNames, ", ")));

		if (!addOns.missing.empty())
			wxLogMessage("Project '%s' has sliders for add-ons that aren't installed, which are left out: %s", set.GetName(), wxString::FromUTF8(JoinStrings(addOns.missing, ", ")));

		if (!addOns.conformed.empty())
			wxLogMessage("Project '%s': conformed the sliders of add-ons of '%s': %s",
						 set.GetName(),
						 wxString::FromUTF8(addOns.conformBody),
						 wxString::FromUTF8(JoinStrings(addOns.conformed, ", ")));
		return;
	}

	for (auto& addOn : addOns.stacked) {
		for (auto& vertexCount : addOn.vertexCounts) {
			auto shape = nif->FindBlockByName<NiShape>(vertexCount.first);
			if (shape && vertexCount.second > 0 && vertexCount.second != shape->GetNumVertices())
				wxLogWarning("Add-on '%s': its slider data of shape '%s' was made for %u vertices, but the shape has %u now.",
							 addOn.setName,
							 vertexCount.first,
							 vertexCount.second,
							 static_cast<uint32_t>(shape->GetNumVertices()));
		}
	}
}

void BodySlideApp::SetConformAddOnSliders(bool conform) {
	if (conformAddOnSliders == conform)
		return;

	conformAddOnSliders = conform;
	conformedSets.clear();

	if (!HasActiveProject() || multiProjectMode)
		return;

	// The sliders keep the values they were changed to
	pendingSliderValues.clear();
	for (size_t i = 0; i < sliderManager.slidersBig.size() && i < sliderManager.slidersSmall.size(); i++) {
		auto& big = sliderManager.slidersBig[i];
		auto& small = sliderManager.slidersSmall[i];
		if (big.changed || small.changed)
			pendingSliderValues[big.name] = {small.value, big.value};
	}

	ActivateOutfit(BodySlideConfig["SelectedOutfit"]);
}

std::string BodySlideApp::GetOutputDataPath() const {
	std::string res = Config["OutputDataPath"];
	return res.empty() ? Config["GameDataPath"] : res;
}

void BodySlideApp::RefreshOutfitList() {
	LoadSliderSets();
	PopulateOutfitList("");
}

int BodySlideApp::LoadSliderSets() {
	wxLogMessage("Loading all slider sets...");
	outfitNameSource.clear();
	outfitNameOrder.clear();
	outfitHasZaps.clear();
	outfitBaseProjects.clear();
	outFileCount.clear();
	projectCatalog.Clear();
	projectCatalog.SetProjectPath(ProjectUtil::GetProjectPath()); // The relative files of base projects are in it
	conformedSets.clear();
	{
		std::lock_guard<std::mutex> lock(conformBodiesMutex);
		conformBodies.clear();
	}

	wxArrayString files;
	wxDir::GetAllFiles(wxString::FromUTF8(ProjectUtil::GetProjectPath()) + "/SliderSets", &files, "*.osp");
	wxDir::GetAllFiles(wxString::FromUTF8(ProjectUtil::GetProjectPath()) + "/SliderSets", &files, "*.xml");

	bool filterHasZaps = false;

	auto menuOutfitSrchContext = sliderView->outfitsearch->GetMenu();
	if (menuOutfitSrchContext) {
		auto menuFilterHasZaps = menuOutfitSrchContext->FindItem(XRCID("menuFilterHasZaps"));
		if (menuFilterHasZaps)
			filterHasZaps = menuFilterHasZaps->IsChecked();
	}

	for (auto& file : files) {
		std::string fileName{file.ToUTF8()};

		SliderSetFile sliderDoc;
		sliderDoc.Open(fileName);
		if (sliderDoc.fail())
			continue;

		std::string outFilePath;
		std::vector<std::string> outfitNames;
		sliderDoc.GetSetNamesUnsorted(outfitNames, false);
		for (auto& o : outfitNames) {
			if (outfitNameSource.find(o) != outfitNameSource.end())
				continue;

			projectCatalog.Add(sliderDoc, o);
			outfitNameSource[o] = fileName;
			outfitNameOrder.push_back(o);

			std::string baseProject = sliderDoc.GetSetBaseProjectName(o);
			if (!baseProject.empty())
				outfitBaseProjects[o] = {baseProject, sliderDoc.IsAddOn(o)};

			if (filterHasZaps) {
				bool hasZaps = false;

				SliderSet sliderSet;
				if (sliderDoc.GetSet(o, sliderSet) == 0) {
					for (size_t s = 0; s < sliderSet.size(); s++) {
						auto& slider = sliderSet[s];
						if (slider.bZap && !slider.bHidden) {
							hasZaps = true;
							break;
						}
					}
				}

				if (hasZaps)
					outfitHasZaps.push_back(o);
			}

			sliderDoc.GetSetOutputFilePath(o, outFilePath);
			if (!outFilePath.empty())
				outFileCount[outFilePath].push_back(o);
		}
	}

	// Add-ons that build the same file as their base project are built with it, so they aren't outfits of their own
	projectCatalog.Finish();
	for (auto& entry : projectCatalog.Entries()) {
		if (!entry.stackable)
			continue;

		const std::string& o = entry.setName;
		outfitNameSource.erase(o);
		outfitNameOrder.erase(std::remove(outfitNameOrder.begin(), outfitNameOrder.end(), o), outfitNameOrder.end());
		outfitHasZaps.erase(std::remove(outfitHasZaps.begin(), outfitHasZaps.end(), o), outfitHasZaps.end());
		outfitBaseProjects.erase(o);

		auto outFile = outFileCount.find(entry.outputFilePath);
		if (outFile != outFileCount.end()) {
			outFile->second.erase(std::remove(outFile->second.begin(), outFile->second.end(), o), outFile->second.end());
			if (outFile->second.empty())
				outFileCount.erase(outFile);
		}
	}

	ungroupedOutfits.clear();
	for (auto& o : outfitNameSource) {
		std::vector<std::string> groups;
		gCollection.GetOutfitGroups(o.first, groups);
		if (groups.empty())
			ungroupedOutfits.push_back(o.first);
	}

	bool favoritesChanged = false;
	for (auto it = favoriteOutfits.begin(); it != favoriteOutfits.end();) {
		// Add-ons built with their base project stay favorites, they're listed again once they build a file of their own
		if (OutfitExists(*it) || projectCatalog.IsStackedAddOn(*it))
			++it;
		else {
			favoriteOutfitSet.erase(*it);
			it = favoriteOutfits.erase(it);
			favoritesChanged = true;
		}
	}

	if (favoritesChanged)
		SaveFavorites();

	return 0;
}

void BodySlideApp::ActivateOutfit(const std::string& name) {
	// An add-on that is built with its base project shows as part of it
	std::string outfitName = name;
	projectCatalog.IsStackedAddOn(name, &outfitName);

	wxLogMessage("Activating set '%s'...", outfitName);

	BodySlideConfig.SetValue("SelectedOutfit", outfitName);
	sliderView->Freeze();

	sliderView->ClearPresetList();
	sliderView->ClearSliderGUI();

	CleanupPreview();

	std::string activePreset = BodySlideConfig["SelectedPreset"];

	sliderManager.ClearPresets();
	SetPresetGroups(outfitName);
	LoadPresets(outfitName);
	PopulatePresetList(activePreset);

	int error = CreateSetSliders(outfitName);
	if (error)
		wxLogError("Failed to load set '%s' from slider set list (%d).", outfitName, error);

	PopulateOutfitList(outfitName);

	ActivatePreset(activePreset, false);

	// Sliders that were changed before the outfit was loaded again keep their values
	if (!pendingSliderValues.empty()) {
		bool changed = false;
		for (auto& value : pendingSliderValues) {
			auto& sliders = sliderManager.slidersBig;
			if (std::none_of(sliders.begin(), sliders.end(), [&value](const Slider& s) { return s.name == value.first; }))
				continue;

			sliderManager.SetSlider(value.first, true, value.second.first);
			sliderManager.SetSlider(value.first, false, value.second.second);
			sliderManager.SetChanged(value.first, true);
			sliderManager.SetChanged(value.first, false);
			sliderView->SetSliderPosition(value.first.c_str(), value.second.first, SLIDER_LO);
			sliderView->SetSliderPosition(value.first.c_str(), value.second.second, SLIDER_HI);
			changed = true;
		}

		pendingSliderValues.clear();
		if (changed) {
			UpdateToggledSliders();
			sliderView->SetPresetChanged(true);
		}
	}

	InitPreview();

	sliderView->Layout();
	sliderView->Refresh();
	sliderView->Thaw();
	wxLogMessage("Finished activating set '%s'.", outfitName);
}

std::vector<std::string> BodySlideApp::GetDerivedProjects(const std::string& outfitName) {
	std::vector<std::string> derived;
	const ProjectCatalogEntry* project = projectCatalog.Find(outfitName);
	if (!project)
		return derived;

	auto& entries = projectCatalog.Entries();
	for (auto& entry : entries)
		if (entry.baseIndex != ProjectCatalog::npos && &entries[entry.baseIndex] == project)
			derived.push_back(entry.setName);

	return derived;
}

void BodySlideApp::DeleteOutfit(const std::string& outfitName) {
	auto outfit = outfitNameSource.find(outfitName);
	if (outfit == outfitNameSource.end())
		return;

	int select = wxNOT_FOUND;
	if (sliderView->outfitChoice)
		select = sliderView->outfitChoice->GetSelection();

	wxLogMessage("Loading project file '%s'...", outfit->second);

	SliderSetFile sliderDoc;
	sliderDoc.Open(outfit->second);
	if (!sliderDoc.fail()) {
		wxLogMessage("Deleting project '%s'...", outfitName);

		if (!sliderDoc.DeleteSet(outfit->first)) {
			if (sliderDoc.Save()) {
				RemoveFavoriteOutfit(outfitName);
				RefreshOutfitList();

				if (sliderView->outfitChoice) {
					int count = sliderView->outfitChoice->GetCount();
					if (count > select)
						sliderView->outfitChoice->Select(select);
					else if (count > select - 1)
						sliderView->outfitChoice->Select(select - 1);

					std::string selectedOutfit = sliderView->GetSelectedOutfitName();
					if (!selectedOutfit.empty())
						ActivateOutfit(selectedOutfit);
				}
			}
			else
				wxLogMessage("Failed to delete the slider set!");
		}
		else
			wxLogMessage("Failed to delete the slider set!");
	}
	else
		wxLogMessage("Failed to load the project file!");
}

void BodySlideApp::PopulateOutfitList(const std::string& select) {
	std::string myselect = BodySlideConfig["SelectedOutfit"];
	if (!select.empty())
		myselect = select;

	wxArrayString items;

	size_t n = outfitNameSource.size();
	if (n == 0)
		return;

	ApplyOutfitFilter();

	items.reserve(filteredOutfits.size());
	for (auto& fo : filteredOutfits)
		items.Add(wxString::FromUTF8(fo));

	sliderView->PopulateOutfitList(items, wxString::FromUTF8(myselect));
}

void BodySlideApp::EditProject(const std::string& projectName) {
	auto project = outfitNameSource.find(projectName);
	if (project == outfitNameSource.end())
		return;

	wxLogMessage("Launching Outfit Studio with project file '%s' and project '%s'...", project->second, project->first);
	LaunchOutfitStudio(wxString::Format("-proj \"%s\" \"%s\"", wxString::FromUTF8(project->first), wxString::FromUTF8(project->second)));
}

void BodySlideApp::LaunchOutfitStudio(const wxString& args) {
#ifdef _WIN32
	const wxString osExec = "OutfitStudio.exe";
#else
	const wxString osExec = "OutfitStudio";
#endif

	wxFileName osExecFile(wxString::FromUTF8(Config["AppDir"]), osExec);
	wxString osExecCmd = wxString::Format("\"%s\" %s", osExecFile.GetFullPath(), args);

	if (!wxExecute(osExecCmd, wxEXEC_ASYNC)) {
		wxLogError("Failed to execute '%s' process.", osExecCmd);
		wxMessageBox(_("Failed to launch Outfit Studio executable!"), _("Error"), wxICON_ERROR);
	}
}
