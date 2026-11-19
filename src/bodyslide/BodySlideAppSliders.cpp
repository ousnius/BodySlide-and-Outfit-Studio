/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

// BodySlide: the sliders of the active outfit, their values, zaps and toggles

#include "BodySlideApp.h"
#include "BodySlideFrame.h"
#include "ui/bodyslide/PreviewPanel.h"

#include <algorithm>
#include <utility>
#include <wx/wx.h>

using namespace nifly;

int BodySlideApp::CreateSetSliders(const std::string& outfit) {
	wxLogMessage("Creating sliders...");
	auto source = outfitNameSource.find(outfit);
	if (source == outfitNameSource.end())
		return 1;

	projects.clear();
	projects.push_back(std::make_unique<ProjectData>());
	auto& activeSet = GetActiveSet();
	activeSet.Clear();
	sliderManager.ClearSliders();

	int error = 0;
	auto conformed = conformAddOnSliders ? conformedSets.find(outfit) : conformedSets.end();
	if (conformed != conformedSets.end()) {
		activeSet = conformed->second.first;
		activeAddOns = conformed->second.second;
	}
	else {
		std::unique_ptr<wxBusyCursor> busy;
		if (conformAddOnSliders) {
			busy = std::make_unique<wxBusyCursor>();

			// Conforming uses the skinning of the shapes, which needs the skeleton
			EnsurePreviewSkeleton();
		}

		error = LoadProjectSet(source->second, outfit, activeSet, conformAddOnSliders, activeAddOns);
		if (!error) {
			LogBaseProjectStatus(activeSet);
			LogProjectAddOns(activeSet, activeAddOns);

			if (conformAddOnSliders)
				conformedSets[outfit] = {activeSet, activeAddOns};
		}
	}

	if (!error) {
		sliderManager.AddSlidersInSet(activeSet);
		DisplayActiveSet();
		return 0;
	}

	activeAddOns = ProjectAddOns();
	if (sliderView) {
		sliderView->ShowOutfitBaseProject(nullptr);
		sliderView->ShowOutfitAddOns(nullptr);
	}

	return error;
}

void BodySlideApp::RefreshSliders() {
	Slider* slider = nullptr;
	for (size_t i = 0; i < sliderManager.slidersBig.size(); i++) {
		slider = &sliderManager.slidersBig[i];
		sliderView->SetSliderPosition(slider->name.c_str(), slider->value, SLIDER_HI);

		slider = &sliderManager.slidersSmall[i];
		sliderView->SetSliderPosition(slider->name.c_str(), slider->value, SLIDER_LO);
	}

	UpdateToggledSliders();

	if (preview)
		UpdatePreview();
}

std::vector<bool> BodySlideApp::ZappedSliders() {
	std::vector<float> values = ToggledSliderValues(sliderManager.slidersBig);
	std::vector<bool> zapped(values.size(), false);
	for (size_t i = 0; i < values.size(); i++)
		zapped[i] = sliderManager.slidersBig[i].zap && !sliderManager.slidersBig[i].uv && values[i] > 0.0f;

	return zapped;
}

bool BodySlideApp::ToggleSetsZaps(const wxString& sliderName) {
	if (projects.empty())
		return false;

	std::string sstr{sliderName.ToUTF8()};
	auto& activeSet = GetActiveSet();
	if (!activeSet.SliderExists(sstr))
		return false;

	for (auto& target : activeSet[sstr].toggleSliders)
		if (activeSet.SliderExists(target.slider) && activeSet[target.slider].bZap && !activeSet[target.slider].bUV)
			return true;

	return false;
}

void BodySlideApp::UpdateToggledSliders() {
	if (!sliderView)
		return;

	std::vector<int> forcedBig;
	std::vector<int> forcedSmall;
	std::vector<float> valuesBig = ToggledSliderValues(sliderManager.slidersBig, &forcedBig);
	std::vector<float> valuesSmall = ToggledSliderValues(sliderManager.slidersSmall, &forcedSmall);

	for (size_t i = 0; i < sliderManager.slidersBig.size(); i++) {
		auto& sliderBig = sliderManager.slidersBig[i];
		SliderDisplay* sd = sliderView->GetSliderDisplay(sliderBig.name);
		if (!sd || sliderBig.toggle)
			continue;

		int forcedBy = forcedBig[i];
		if (forcedBy < 0 && i < forcedSmall.size())
			forcedBy = forcedSmall[i];

		if (forcedBy < 0 && !sd->isForced)
			continue;

		// Held sliders show the value the toggle sets, and their own value again once it's unticked
		sd->SetForcedBy(forcedBy >= 0 ? wxString::FromUTF8(sliderManager.slidersBig[forcedBy].name) : wxString());
		sliderView->SetSliderPosition(sliderBig.name.c_str(), forcedBy >= 0 ? valuesBig[i] : sliderBig.value, SLIDER_HI);

		if (i < sliderManager.slidersSmall.size()) {
			auto& sliderSmall = sliderManager.slidersSmall[i];
			sliderView->SetSliderPosition(sliderSmall.name.c_str(), forcedBy >= 0 ? valuesSmall[i] : sliderSmall.value, SLIDER_LO);
		}
	}
}

void BodySlideApp::DisplayActiveSet() {
	if (projects.empty())
		return;

	auto& activeSet = GetActiveSet();
	sliderView->ShowOutfitBaseProject(&activeSet);
	sliderView->ShowOutfitAddOns(&activeAddOns);

	// Sliders of add-ons are marked, conformed ones the most
	auto markerOf = [](const SliderData& slider, wxString& tip) {
		if (slider.bAutoConformed) {
			tip = wxString::Format(_("Conformed automatically from add-on '%s'. The result can be rough. An add-on of this outfit with handcrafted sliders takes its place."),
								   wxString::FromUTF8(slider.stackedFrom));
			return SliderMarker::AutoConformed;
		}

		if (!slider.stackedFrom.empty()) {
			tip = wxString::Format(_("From add-on '%s'."), wxString::FromUTF8(slider.stackedFrom));
			if (slider.NeedsAddOn() && slider.addOnName != slider.stackedFrom)
				tip += "\n" + wxString::Format(_("Made for add-on '%s'."), wxString::FromUTF8(slider.addOnName));
			return SliderMarker::AddOn;
		}

		if (slider.NeedsAddOn()) {
			tip = wxString::Format(_("Made for add-on '%s'."), wxString::FromUTF8(slider.addOnName.empty() ? slider.addOnID : slider.addOnName));
			return SliderMarker::AddOn;
		}

		return SliderMarker::None;
	};

	if (activeSet.GenWeights())
		sliderView->ShowLowColumn(true);
	else
		sliderView->ShowLowColumn(false);

	// Category name, slider names, hidden flag
	std::vector<std::tuple<std::string, std::vector<std::string>, bool>> sliderCategories;
	std::vector<std::string> cats;
	cCollection.GetAllCategories(cats);

	// Populate category data
	for (auto& cat : cats) {
		std::vector<std::string> catSliders;
		cCollection.GetCategorySliders(cat, catSliders);

		if (catSliders.size() > 0) {
			bool catHidden = cCollection.GetCategoryHidden(cat);
			sliderCategories.push_back(make_tuple(cat, catSliders, !catHidden));
		}
		else
			continue;
	}

	// Loop slider set
	std::vector<std::vector<int>> catSliders;
	for (size_t i = 0; i < activeSet.size(); i++) {
		if (activeSet[i].bHidden)
			continue;

		// Find the category in the list
		bool regularSlider = true;
		int iter = 0;
		for (auto& cat : sliderCategories) {
			catSliders.push_back(std::vector<int>());
			if (std::find(std::get<1>(cat).begin(), std::get<1>(cat).end(), activeSet[i].name) != std::get<1>(cat).end()) {
				catSliders[iter].push_back(i);
				regularSlider = false;
				break;
			}
			iter++;
		}

		// Not in a category
		if (regularSlider) {
			wxString tip;
			SliderMarker marker = markerOf(activeSet[i], tip);
			sliderView->AddSliderGUI(activeSet[i].name, activeSet[i].name, "", activeSet[i].bZap || activeSet[i].bToggle, !activeSet.GenWeights(), marker, tip);
		}
	}

	// Create category UI
	size_t iter = 0;
	if (catSliders.size() > 0) {
		for (auto& cat : sliderCategories) {
			std::string categoryName = std::get<0>(cat);
			std::vector<std::string> sliderNames = std::get<1>(cat);
			bool show = std::get<2>(cat);
			std::string displayName;

			if (catSliders.size() > iter && catSliders[iter].size() > 0) {
				sliderView->AddCategorySliderUI(categoryName, sliderNames, show, !activeSet.GenWeights());

				for (auto& s : catSliders[iter]) {
					displayName = cCollection.GetSliderDisplayName(categoryName, activeSet[s].name);
					if (displayName.empty())
						displayName = activeSet[s].name;

					wxString tip;
					SliderMarker marker = markerOf(activeSet[s], tip);
					sliderView->AddSliderGUI(activeSet[s].name, displayName, categoryName, activeSet[s].bZap || activeSet[s].bToggle, !activeSet.GenWeights(), marker, tip);
				}
			}
			iter++;
		}
	}

	sliderView->DoFilterSliders();
	UpdateConflictManager();
}

bool BodySlideApp::UpdateZapChoices() {
	if (projects.empty())
		return false;

	auto& activeSet = GetActiveSet();
	BuildSelectionFile buildSelFile;
	BuildSelection buildSelection;
	GetBuildSelection(buildSelFile, buildSelection);

	bool zapChanged = false;

	for (size_t s = 0; s < activeSet.size(); s++) {
		if (!activeSet[s].bZap || activeSet[s].bHidden)
			continue;

		std::string project = activeSet.GetName();
		std::string zap = activeSet[s].name;
		if (buildSelection.HasZapChoice(project, zap)) {
			bool zapChoice = buildSelection.GetZapChoice(project, zap);

			SliderDisplay* sd = sliderView->GetSliderDisplay(zap);
			if (sd && sd->isZap) {
				if (sd->zapCheckHi->IsChecked() != zapChoice) {
					sd->zapCheckHi->SetValue(zapChoice);

					// Trigger checkbox event
					wxEvtHandler* handler = sd->zapCheckHi->GetEventHandler();
					wxCommandEvent event(wxEVT_COMMAND_CHECKBOX_CLICKED, sd->zapCheckHi->GetId());
					event.SetEventObject(sd->zapCheckHi);
					event.SetInt(zapChoice ? 1 : 0);
					handler->ProcessEvent(event);

					zapChanged = true;
				}
			}
		}
	}

	return zapChanged;
}

void BodySlideApp::SetZapChoice(const std::string& zap, bool choice) {
	if (projects.empty())
		return;

	BuildSelectionFile buildSelFile;
	BuildSelection buildSelection;
	GetBuildSelection(buildSelFile, buildSelection);

	std::string project = GetActiveSet().GetName();

	buildSelection.SetZapChoice(project, zap, choice);
	buildSelFile.UpdateZapChoices(buildSelection);

	buildSelFile.Save();
}

void BodySlideApp::CopySliderValues(bool toHigh) {
	wxLogMessage("Copying slider values to %s weight.", toHigh ? "high" : "low");

	if (toHigh) {
		for (size_t i = 0; i < sliderManager.slidersSmall.size(); i++) {
			Slider* slider = &sliderManager.slidersSmall[i];
			if (slider->zap || slider->clamp || slider->toggle)
				continue;

			if (sliderView->GetSliderDisplay(slider->name)) {
				sliderView->SetSliderPosition(slider->name.c_str(), slider->value, SLIDER_HI);
				SetSliderValue(slider->name, false, slider->value);
				SetSliderChanged(slider->name, false);
			}
		}
	}
	else {
		for (size_t i = 0; i < sliderManager.slidersBig.size(); i++) {
			Slider* slider = &sliderManager.slidersBig[i];
			if (slider->zap || slider->clamp || slider->toggle)
				continue;

			if (sliderView->GetSliderDisplay(slider->name)) {
				sliderView->SetSliderPosition(slider->name.c_str(), slider->value, SLIDER_LO);
				SetSliderValue(slider->name, true, slider->value);
				SetSliderChanged(slider->name, true);
			}
		}
	}

	UpdateToggledSliders();
	sliderView->SetPresetChanged();

	if (preview)
		UpdatePreview();
}

float BodySlideApp::GetSliderValue(const wxString& sliderName, bool isLo) {
	std::string sstr{sliderName.ToUTF8()};
	return sliderManager.GetSlider(sstr, isLo);
}

bool BodySlideApp::IsUVSlider(const wxString& sliderName) {
	if (projects.empty())
		return false;

	std::string sstr{sliderName.ToUTF8()};
	return GetActiveSet()[sstr].bUV;
}

bool BodySlideApp::IsToggleSlider(const wxString& sliderName) {
	if (projects.empty())
		return false;

	std::string sstr{sliderName.ToUTF8()};
	auto& activeSet = GetActiveSet();
	return activeSet.SliderExists(sstr) && activeSet[sstr].bToggle && !activeSet[sstr].bZap;
}

std::vector<std::string> BodySlideApp::GetSliderZapToggles(const wxString& sliderName) {
	return sliderManager.GetSliderZapToggles(sliderName.ToUTF8().data());
}

void BodySlideApp::SetSliderValue(const wxString& sliderName, bool isLo, float val) {
	std::string sstr{sliderName.ToUTF8()};
	sliderManager.SetSlider(sstr, isLo, val);
}

void BodySlideApp::SetSliderChanged(const wxString& sliderName, bool isLo) {
	std::string sstr{sliderName.ToUTF8()};
	sliderManager.SetChanged(sstr, isLo);
}
