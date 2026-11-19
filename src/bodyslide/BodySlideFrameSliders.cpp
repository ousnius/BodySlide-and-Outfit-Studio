/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

// The BodySlide window: the slider list

#include "BodySlideFrame.h"
#include "ui/wxStateButton.h"

#include <algorithm>
#include <set>
#include <utility>
#include <wx/textctrl.h>
#include <wx/tokenzr.h>
#include <wx/wx.h>
#include <wx/xrc/xmlres.h>

using namespace nifly;

void BodySlideFrame::OnEnterSliderWindow(wxMouseEvent& event) {
	if (this->IsActive()) {
		// Nothing can have the focus, e.g. on GTK after scrolling the outfit list with the mouse wheel
		wxWindow* focus = FindFocus();
		if (!focus || (!focus->IsKindOf(wxClassInfo::FindClass("wxTextCtrl")) && !focus->IsKindOf(wxClassInfo::FindClass("wxSearchCtrl")))) {
			wxScrolledWindow* sw = (wxScrolledWindow*)event.GetEventObject();
			sw->SetFocusIgnoringChildren();
		}
	}
}

void BodySlideFrame::HideSlider(SliderDisplay* slider) {
	if (!slider)
		return;

	slider->zapCheckLo->Unbind(wxEVT_CHECKBOX, &BodySlideFrame::OnZapCheckChanged, this);
	slider->sliderLo->Unbind(wxEVT_ERASE_BACKGROUND, &BodySlideFrame::OnEraseBackground, this);
	slider->sliderReadoutLo->Disconnect(wxEVT_KILL_FOCUS, wxCommandEventHandler(BodySlideFrame::OnSliderReadoutChange), nullptr, this);

	slider->zapCheckHi->Unbind(wxEVT_CHECKBOX, &BodySlideFrame::OnZapCheckChanged, this);
	slider->sliderHi->Unbind(wxEVT_ERASE_BACKGROUND, &BodySlideFrame::OnEraseBackground, this);
	slider->sliderReadoutHi->Disconnect(wxEVT_KILL_FOCUS, wxCommandEventHandler(BodySlideFrame::OnSliderReadoutChange), nullptr, this);

	slider->Show(false);
}

void BodySlideFrame::ShowLowColumn(bool show) {
	if (show) {
		XRCCTRL(*this, "lblLowWt", wxStaticText)->Show();
		XRCCTRL(*this, "lblHighWt", wxStaticText)->Show();
		XRCCTRL(*this, "lblSingleWt", wxStaticText)->Hide();
		XRCCTRL(*this, "btnLowToHigh", wxButton)->Show();
		XRCCTRL(*this, "btnHighToLow", wxButton)->Show();
		sliderLayout->SetCols(6);
	}
	else {
		XRCCTRL(*this, "lblLowWt", wxStaticText)->Hide();
		XRCCTRL(*this, "lblHighWt", wxStaticText)->Hide();
		XRCCTRL(*this, "lblSingleWt", wxStaticText)->Show();
		XRCCTRL(*this, "btnLowToHigh", wxButton)->Hide();
		XRCCTRL(*this, "btnHighToLow", wxButton)->Hide();
		sliderLayout->SetCols(3);
	}
}

void BodySlideFrame::AddCategorySliderUI(const std::string& name, const std::vector<std::string>& sliders, bool enabled, bool oneSize) {
	SliderCategoryUI* cat = new SliderCategoryUI();

	if (!cat->Create(sliderScroll, sliderLayout, categoryTabSizer, name, sliders, enabled, oneSize))
		return;

	cat->check->Bind(wxEVT_CHECKBOX, &BodySlideFrame::OnCategoryCheckChanged, this);

	if (cat->tabButton)
		cat->tabButton->Bind(wxEVT_BUTTON, &BodySlideFrame::OnCategoryTabButton, this);

	if (!cat->isShown)
		cat->Show();

	sliderCategories[cat->categoryName] = cat;
}

void BodySlideFrame::AddSliderGUI(
	const std::string& name, const std::string& display, const std::string& category, bool isZap, bool oneSize, SliderMarker marker, const wxString& markerTip) {
	SliderDisplay* sd = sliderPool.GetNext();
	if (!sd)
		return;

	int minValue = Config.GetIntValue("Input/SliderMinimum");
	int maxValue = Config.GetIntValue("Input/SliderMaximum");

	if (!sd->Create(sliderScroll, sliderLayout, name, display, category, minValue, maxValue, isZap, oneSize))
		return;

	sd->SetMarker(marker, markerTip);

	sd->zapCheckLo->Bind(wxEVT_CHECKBOX, &BodySlideFrame::OnZapCheckChanged, this);
	sd->sliderLo->Bind(wxEVT_ERASE_BACKGROUND, &BodySlideFrame::OnEraseBackground, this);
	sd->sliderReadoutLo->Connect(wxEVT_KILL_FOCUS, wxCommandEventHandler(BodySlideFrame::OnSliderReadoutChange), nullptr, this);

	sd->zapCheckHi->Bind(wxEVT_CHECKBOX, &BodySlideFrame::OnZapCheckChanged, this);
	sd->sliderHi->Bind(wxEVT_ERASE_BACKGROUND, &BodySlideFrame::OnEraseBackground, this);
	sd->sliderReadoutHi->Connect(wxEVT_KILL_FOCUS, wxCommandEventHandler(BodySlideFrame::OnSliderReadoutChange), nullptr, this);

	if (!sd->isShown)
		sd->Show();

	sliderDisplays[sd->sliderName] = sd;
}

void BodySlideFrame::ClearSliderGUI() {
	for (auto& sd : sliderDisplays)
		HideSlider(sd.second);

	sliderScroll->GetSizer()->Clear();

	for (auto& cat : sliderCategories) {
		cat.second->Destroy();
		delete cat.second;
	}

	sliderDisplays.clear();
	sliderCategories.clear();
}

void BodySlideFrame::SetSliderPosition(const wxString& name, float newValue, short HiLo) {
	int intval = (int)(newValue * 100.0f);

	SliderDisplay* sd = GetSliderDisplay(name.ToUTF8().data());
	if (!sd)
		return;

	if (HiLo == SLIDER_HI) {
		if (!sd->isZap) {
			sd->sliderReadoutHi->ChangeValue(wxString::Format("%d%%", intval));
			sd->sliderHi->SetValue(intval);
		}
		else
			sd->zapCheckHi->SetValue((intval > 0));
	}
	else if (!sd->oneSize) {
		if (!sd->isZap) {
			sd->sliderReadoutLo->ChangeValue(wxString::Format("%d%%", intval));
			sd->sliderLo->SetValue(intval);
		}
		else
			sd->zapCheckLo->SetValue((intval > 0));
	}
}

void BodySlideFrame::OnSliderChange(wxScrollEvent& event) {
	wxWindow* w = (wxWindow*)event.GetEventObject();
	if (!w)
		return;

	wxString fullname = w->GetName();
	wxString name;
	bool isLo = fullname.EndsWith("|LO", &name);
	if (!isLo) {
		if (!fullname.EndsWith("|HI", &name))
			return;
	}

	SliderDisplay* sd = GetSliderDisplay(name.ToUTF8().data());
	if (!sd)
		return;

	app->SetSliderValue(name, isLo, event.GetPosition() / 100.0f);
	app->SetSliderChanged(name, isLo);
	if (isLo)
		sd->sliderReadoutLo->ChangeValue(wxString::Format("%d%%", event.GetPosition()));
	else
		sd->sliderReadoutHi->ChangeValue(wxString::Format("%d%%", event.GetPosition()));

	SetPresetChanged();
	app->UpdatePreview();
}

void BodySlideFrame::OnSliderReadoutChange(wxCommandEvent& event) {
	wxTextCtrl* w = (wxTextCtrl*)event.GetEventObject();
	event.Skip();
	if (!w)
		return;

	wxString fullname = w->GetName();
	wxString name;
	bool isLo = fullname.EndsWith("|RLO", &name);
	if (!isLo) {
		if (!fullname.EndsWith("|RHI", &name))
			return;
	}

	SliderDisplay* sd = GetSliderDisplay(name.ToUTF8().data());
	if (!sd)
		return;

	double v;
	wxString val = w->GetValue();
	val.Replace("%", "");
	if (!val.ToDouble(&v))
		return;

	w->ChangeValue(wxString::Format("%0.0f%%", v));
	app->SetSliderValue(name, isLo, (float)v / 100.0f);
	app->SetSliderChanged(name, isLo);

	if (isLo)
		sd->sliderLo->SetValue(v);
	else
		sd->sliderHi->SetValue(v);

	SetPresetChanged();
	app->UpdatePreview();
}

void BodySlideFrame::OnSliderFilterChanged(wxCommandEvent& WXUNUSED(event)) {
	DoFilterSliders();
}

void BodySlideFrame::DoFilterSliders() {
	sliderScroll->Freeze();

	wxString filterStr = sliderFilter->GetValue();
	filterStr.MakeLower();

	wxArrayString filterStrings;

	// Split string for "or" filtering
	wxStringTokenizer tokenizer(filterStr, ",;");
	while (tokenizer.HasMoreTokens()) {
		wxString token = tokenizer.GetNextToken();
		token.Trim().Trim(false);
		filterStrings.Add(token);
	}

	std::set<std::string> matchedSliders;

	for (auto& sliderDisplay : sliderDisplays) {
		if (!sliderDisplay.second)
			continue;

		// Filter slider by display name or category
		wxString sliderStr = wxString::FromUTF8(sliderDisplay.first).MakeLower();
		wxString displayStr = wxString::FromUTF8(sliderDisplay.second->displayName).MakeLower();
		wxString categoryStr = wxString::FromUTF8(sliderDisplay.second->categoryName).MakeLower();

		// Check if category is disabled
		bool disabledCat = false;

		const std::string& categoryName = sliderDisplay.second->categoryName;
		if (!categoryName.empty()) {
			SliderCategoryUI* sc = GetSliderCategory(categoryName);
			if (sc && !sc->isEnabled)
				disabledCat = true;
		}

		bool show = filterStrings.empty();
		if (!show) {
			for (auto& fstr : filterStrings) {
				// Split string by space for "and" filtering
				bool matched = false;

				wxStringTokenizer andTokenizer(fstr, " ");
				while (andTokenizer.HasMoreTokens()) {
					wxString token = andTokenizer.GetNextToken();
					token.Trim().Trim(false);

					if (displayStr.Contains(token) || sliderStr.Contains(token) || (!categoryStr.empty() && categoryStr.Contains(token)))
						matched = true;
					else {
						matched = false;
						break;
					}
				}

				if (matched) {
					show = true;
					matchedSliders.insert(sliderDisplay.first);
					break;
				}
			}
		}

		if (disabledCat)
			show = false;

		if (show) {
			if (!sliderDisplay.second->isShown)
				sliderDisplay.second->Show();
		}
		else {
			if (sliderDisplay.second->isShown)
				sliderDisplay.second->Show(false);
		}
	}

	for (auto& sliderCategory : sliderCategories) {
		if (!sliderCategory.second)
			continue;

		bool showCat = false;

		if (!filterStrings.empty()) {
			for (auto& sliderName : sliderCategory.second->sliderNames) {
				if (matchedSliders.find(sliderName) != matchedSliders.end())
					showCat = true; // Show category if any slider in it was matched
			}
		}
		else
			showCat = true; // Without a filter, show all categories

		sliderCategory.second->Show(showCat);
	}

	sliderScroll->Thaw();
	sliderScroll->Layout();
	sliderScroll->FitInside();
}

void BodySlideFrame::OnCategoryCheckChanged(wxCommandEvent& event) {
	wxWindow* w = (wxWindow*)event.GetEventObject();
	if (!w)
		return;

	wxCheckBox* cb = (wxCheckBox*)event.GetEventObject();
	if (!cb)
		return;

	std::string categoryName = cb->GetName().ToUTF8().data();

	SliderCategoryUI* sc = GetSliderCategory(categoryName);
	if (sc) {
		sc->isEnabled = event.IsChecked();

		DoFilterSliders();

		int scrollPos = sliderScroll->GetScrollPos(wxOrientation::wxVERTICAL);
		sliderScroll->Scroll(0, scrollPos);
	}
}

void BodySlideFrame::OnCategoryTabButton(wxCommandEvent& event) {
	wxWindow* w = (wxWindow*)event.GetEventObject();
	if (!w)
		return;

	wxStateButton* tabButton = (wxStateButton*)event.GetEventObject();
	if (!tabButton)
		return;

	std::string categoryName = tabButton->GetName().ToUTF8().data();

	SliderCategoryUI* sc = GetSliderCategory(categoryName);
	if (sc) {
		if (!sc->isEnabled) {
			sc->check->SetValue(true);
			sc->isEnabled = true;
			DoFilterSliders();
		}

		int scroll_rate_y = 0;
		sliderScroll->GetScrollPixelsPerUnit(nullptr, &scroll_rate_y);

		wxPoint window_pos = sliderScroll->CalcUnscrolledPosition(sc->check->GetPosition());
		sliderScroll->Scroll(0, window_pos.y / scroll_rate_y);
	}
}

void BodySlideFrame::OnZapCheckChanged(wxCommandEvent& event) {
	wxWindow* w = (wxWindow*)event.GetEventObject();
	if (!w)
		return;

	wxString sn;
	bool isLo = true;
	if (!w->GetName().EndsWith("|ZLO", &sn)) {
		isLo = false;
		if (!w->GetName().EndsWith("|ZHI", &sn)) {
			event.Skip();
			return;
		}
	}

	std::string sliderName{sn.ToUTF8()};
	bool isToggle = app->IsToggleSlider(sn);
	wxLogMessage("%s '%s' %s.", isToggle ? "Toggle" : "Zap", sn, event.IsChecked() ? "checked" : "unchecked");

	SliderDisplay* slider = GetSliderDisplay(sliderName);
	if (slider) {
		bool checked = event.IsChecked();
		if (checked) {
			if (slider->oneSize) {
				app->SetSliderValue(sn, false, 1.0f);
			}
			else {
				app->SetSliderValue(sn, true, 1.0f);
				app->SetSliderValue(sn, false, 1.0f);
				if (isLo)
					slider->zapCheckHi->SetValue(true);
				else
					slider->zapCheckLo->SetValue(true);
			}
		}
		else {
			if (slider->oneSize) {
				app->SetSliderValue(sn, false, 0.0f);
			}
			else {
				app->SetSliderValue(sn, true, 0.0f);
				app->SetSliderValue(sn, false, 0.0f);
				if (isLo)
					slider->zapCheckHi->SetValue(false);
				else
					slider->zapCheckLo->SetValue(false);
			}
		}

		if (!isToggle)
			app->SetZapChoice(sliderName, checked);
	}

	app->SetSliderChanged(sn, isLo);

	// A toggle applies its slider and the sliders it sets, and zaps or keeps the zaps it sets
	if (isToggle) {
		app->UpdateToggledSliders();
		SetPresetChanged();

		wxBeginBusyCursor();
		if (app->ToggleSetsZaps(sn))
			app->RebuildPreviewMeshes();
		else
			app->UpdatePreview();
		wxEndBusyCursor();
		return;
	}

	std::vector<std::string> zapToggles = app->GetSliderZapToggles(sn);
	for (auto& toggle : zapToggles) {
		wxLogMessage("Zap '%s' toggled.", toggle);

		app->SetSliderValue(toggle, true, 1.0f - app->GetSliderValue(toggle, true));
		app->SetSliderValue(toggle, false, 1.0f - app->GetSliderValue(toggle, false));
		app->SetSliderChanged(toggle, false);

		slider = GetSliderDisplay(toggle);
		if (slider) {
			if (slider->zapCheckHi)
				slider->zapCheckHi->SetValue(!slider->zapCheckHi->GetValue());
			if (slider->zapCheckLo)
				slider->zapCheckLo->SetValue(!slider->zapCheckLo->GetValue());
		}
	}

	// Zaps can toggle toggles too
	if (!zapToggles.empty())
		app->UpdateToggledSliders();

	SetPresetChanged();

	wxBeginBusyCursor();
	if (app->IsUVSlider(sn))
		app->UpdatePreview();
	else
		app->RebuildPreviewMeshes();
	wxEndBusyCursor();
}

void BodySlideFrame::OnHighToLow(wxCommandEvent& WXUNUSED(event)) {
	app->CopySliderValues(false);
}

void BodySlideFrame::OnLowToHigh(wxCommandEvent& WXUNUSED(event)) {
	app->CopySliderValues(true);
}

void BodySlideFrame::OnConformAddOns(wxCommandEvent& event) {
	app->SetConformAddOnSliders(event.IsChecked());
}
