/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "SliderDisplay.h"
#include "ui/wxStateButton.h"

#include <wx/wrapsizer.h>

SliderCategoryUI::SliderCategoryUI() {}

bool SliderCategoryUI::Create(wxScrolledWindow* scrollWindow, wxSizer* sliderLayout, wxSizer* categoryTabSizer, const std::string& name, const std::vector<std::string>& sliders, bool pEnabled, bool pOneSize) {
	categoryName = name;
	sliderNames = sliders;

	isEnabled = pEnabled;
	oneSize = pOneSize;

	if (isCreated) {
		check->SetValue(isEnabled);
		label->SetLabel(name);

		Show();
		return true;
	}

	if (!oneSize) {
		sliderLayout->AddSpacer(0);

		dummyPanel1 = new wxPanel(scrollWindow);
		dummyPanel1->SetBackgroundColour(wxColour(90, 90, 90));
		sliderLayout->Add(dummyPanel1, 0, wxTOP | wxBOTTOM | wxEXPAND, 10);
	}

	check = new wxCheckBox(scrollWindow, wxID_ANY, "");
	check->SetName(name);
	sliderLayout->Add(check, 0, wxRIGHT | wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL, 5);
	check->SetValue(isEnabled);

	label = new wxStaticText(scrollWindow, wxID_ANY, name);
	label->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, "Andalus"));
	label->SetForegroundColour(wxColour(200, 200, 200));
	sliderLayout->Add(label, 0, wxLEFT | wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL);

	if (!oneSize) {
		dummyPanel2 = new wxPanel(scrollWindow);
		dummyPanel2->SetBackgroundColour(wxColour(90, 90, 90));
		sliderLayout->Add(dummyPanel2, 0, wxTOP | wxBOTTOM | wxEXPAND, 10);
	}

	sliderLayout->AddSpacer(0);

	if (categoryTabSizer) {
		tabButton = new wxStateButton(categoryTabSizer->GetContainingWindow(), wxID_ANY, name, wxDefaultPosition, wxDefaultSize, 0L, wxDefaultValidator, name, true);
		categoryTabSizer->Add(tabButton, 0, 0, 0);
	}

	Show(false);
	isCreated = true;
	return true;
}

void SliderCategoryUI::Show(bool show) {
	if (dummyPanel1)
		dummyPanel1->Show(show && !oneSize);

	check->Show(show);
	label->Show(show);

	if (dummyPanel2)
		dummyPanel2->Show(show && !oneSize);

	if (tabButton) {
		auto wrapSizer = (wxWrapSizer*)tabButton->GetContainingSizer();
		wrapSizer->Show(tabButton, show);

		wxSize minSize = wrapSizer->CalcMin();
		wrapSizer->RepositionChildren(minSize);
	}

	isShown = show;
}

void SliderCategoryUI::Destroy() {
	if (dummyPanel1) {
		dummyPanel1->Destroy();
		dummyPanel1 = nullptr;
	}

	check->Destroy();
	check = nullptr;

	label->Destroy();
	label = nullptr;

	if (dummyPanel2) {
		dummyPanel2->Destroy();
		dummyPanel2 = nullptr;
	}

	if (tabButton) {
		tabButton->Destroy();
		tabButton = nullptr;
	}

	isShown = false;
}

SliderDisplay::SliderDisplay() {}

bool SliderDisplay::Create(wxScrolledWindow* scrollWindow,
						   wxSizer* sliderLayout,
						   const std::string& name,
						   const std::string& display,
						   const std::string& category,
						   int minValue,
						   int maxValue,
						   bool pIsZap,
						   bool pOneSize) {
	isZap = pIsZap;
	oneSize = pOneSize;

	sliderName = name;
	displayName = display;
	categoryName = category;

	wxString nameStr = wxString::FromUTF8(name);
	wxString displayNameStr = wxString::FromUTF8(display);

	if (isCreated) {
		// The pooled slider was held by a toggle for another slider before
		SetForcedBy(wxEmptyString);

		if (lblSliderLo->GetLabel() != displayNameStr)
			lblSliderLo->SetLabel(displayNameStr);

		lblSliderLo->Show(!oneSize);

		if (!oneSize)
			sliderLayout->Add(lblSliderLo, 0, wxLEFT | wxALIGN_CENTER, 5);

		zapCheckLo->SetName(nameStr + "|ZLO");
		zapCheckLo->Show(!oneSize && isZap);

		if (!oneSize && isZap) {
			sliderLayout->AddStretchSpacer();
			sliderLayout->Add(zapCheckLo, 0, wxALIGN_LEFT, 0);
		}

		sliderLo->SetName(nameStr + "|LO");
		sliderLo->SetMin(minValue);
		sliderLo->SetMax(maxValue);
		sliderLo->Show(!oneSize && !isZap);

		if (!oneSize && !isZap)
			sliderLayout->Add(sliderLo, 1, wxEXPAND, 0);

		sliderReadoutLo->SetName(nameStr + "|RLO");
		sliderReadoutLo->Show(!oneSize && !isZap);

		if (!oneSize && !isZap)
			sliderLayout->Add(sliderReadoutLo, 0, wxALL | wxALIGN_CENTER, 0);

		if (lblSliderHi->GetLabel() != displayNameStr)
			lblSliderHi->SetLabel(displayNameStr);

		sliderLayout->Add(lblSliderHi, 0, wxLEFT | wxALIGN_CENTER, 5);

		zapCheckHi->SetName(nameStr + "|ZHI");
		zapCheckHi->Show(isZap);

		if (isZap) {
			sliderLayout->AddStretchSpacer();
			sliderLayout->Add(zapCheckHi, 0, wxALIGN_LEFT, 0);
		}

		sliderHi->SetName(nameStr + "|HI");
		sliderHi->SetMin(minValue);
		sliderHi->SetMax(maxValue);
		sliderHi->Show(!isZap);

		if (!isZap)
			sliderLayout->Add(sliderHi, 1, wxEXPAND, 0);

		sliderReadoutHi->SetName(nameStr + "|RHI");
		sliderReadoutHi->Show(!isZap);

		if (!isZap)
			sliderLayout->Add(sliderReadoutHi, 0, wxRIGHT | wxALIGN_CENTER, 10);

		Show();
		return true;
	}

	lblSliderLo = new wxStaticText(scrollWindow, wxID_ANY, display, wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER_HORIZONTAL);
	lblSliderLo->SetForegroundColour(wxColour(200, 200, 200));
	lblSliderLo->Show(!oneSize);

	if (!oneSize)
		sliderLayout->Add(lblSliderLo, 0, wxLEFT | wxALIGN_CENTER, 5);

	zapCheckLo = new wxCheckBox(scrollWindow, wxID_ANY, " ", wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, nameStr + "|ZLO");
	zapCheckLo->Show(!oneSize && isZap);

	if (!oneSize && isZap) {
		sliderLayout->AddStretchSpacer();
		sliderLayout->Add(zapCheckLo, 0, wxALIGN_LEFT, 0);
	}

	sliderLo = new wxSlider(scrollWindow, wxID_ANY, 0, minValue, maxValue, wxDefaultPosition, wxDefaultSize, wxSL_BOTTOM | wxSL_HORIZONTAL);
	sliderLo->SetTickFreq(5);
	sliderLo->SetName(nameStr + "|LO");
	sliderLo->Show(!oneSize && !isZap);

	if (!oneSize && !isZap)
		sliderLayout->Add(sliderLo, 1, wxEXPAND, 0);

	sliderReadoutLo = new wxTextCtrl(scrollWindow, wxID_ANY, "0%", wxDefaultPosition, wxSize(scrollWindow->FromDIP(50), -1), wxTE_CENTRE | wxNO_BORDER | wxTE_PROCESS_ENTER);
	sliderReadoutLo->Show(!oneSize && !isZap);
	sliderReadoutLo->SetName(nameStr + "|RLO");

	if (!oneSize && !isZap)
		sliderLayout->Add(sliderReadoutLo, 0, wxALL | wxALIGN_CENTER, 0);

	lblSliderHi = new wxStaticText(scrollWindow, wxID_ANY, display, wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER_HORIZONTAL);
	lblSliderHi->SetForegroundColour(wxColour(200, 200, 200));
	sliderLayout->Add(lblSliderHi, 0, wxLEFT | wxALIGN_CENTER, 5);

	zapCheckHi = new wxCheckBox(scrollWindow, wxID_ANY, " ", wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, nameStr + "|ZHI");
	zapCheckHi->Show(isZap);

	if (isZap) {
		sliderLayout->AddStretchSpacer();
		sliderLayout->Add(zapCheckHi, 0, wxALIGN_LEFT, 0);
	}

	sliderHi = new wxSlider(scrollWindow, wxID_ANY, 0, minValue, maxValue, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL);
	sliderHi->SetTickFreq(5);
	sliderHi->SetName(nameStr + "|HI");
	sliderHi->Show(!isZap);

	if (!isZap)
		sliderLayout->Add(sliderHi, 1, wxEXPAND, 0);

	sliderReadoutHi = new wxTextCtrl(scrollWindow, wxID_ANY, "0%", wxDefaultPosition, wxSize(scrollWindow->FromDIP(50), -1), wxTE_CENTRE | wxNO_BORDER | wxTE_PROCESS_ENTER);
	sliderReadoutHi->Show(!isZap);
	sliderReadoutHi->SetName(nameStr + "|RHI");

	if (!isZap)
		sliderLayout->Add(sliderReadoutHi, 0, wxRIGHT | wxALIGN_CENTER, 10);

	Show(false);
	isCreated = true;
	return true;
}

void SliderDisplay::SetMarker(SliderMarker marker, const wxString& tip) {
	wxColour colour(200, 200, 200);
	if (marker == SliderMarker::AddOn)
		colour = wxColour(120, 190, 255);
	else if (marker == SliderMarker::AutoConformed)
		colour = wxColour(255, 190, 90);

	markerTip = tip;
	for (wxWindow* window : std::initializer_list<wxWindow*>{lblSliderLo, lblSliderHi, sliderLo, sliderHi, zapCheckLo, zapCheckHi}) {
		if (!window)
			continue;

		if (tip.empty())
			window->UnsetToolTip();
		else
			window->SetToolTip(tip);
	}

	for (wxStaticText* label : {lblSliderLo, lblSliderHi}) {
		if (label && label->GetForegroundColour() != colour) {
			label->SetForegroundColour(colour);
			label->Refresh();
		}
	}
}

void SliderDisplay::SetForcedBy(const wxString& toggleName) {
	bool forced = !toggleName.empty();
	if (!forced && !isForced)
		return;

	isForced = forced;

	wxString tip = markerTip;
	if (forced) {
		if (!tip.empty())
			tip += "\n";
		tip += wxString::Format(_("Set by toggle '%s'."), toggleName);
	}

	for (wxWindow* window : std::initializer_list<wxWindow*>{sliderLo, sliderHi, sliderReadoutLo, sliderReadoutHi, zapCheckLo, zapCheckHi}) {
		if (!window)
			continue;

		window->Enable(!forced);
		if (tip.empty())
			window->UnsetToolTip();
		else
			window->SetToolTip(tip);
	}

	for (wxWindow* window : std::initializer_list<wxWindow*>{lblSliderLo, lblSliderHi}) {
		if (!window)
			continue;

		if (tip.empty())
			window->UnsetToolTip();
		else
			window->SetToolTip(tip);
	}
}

void SliderDisplay::Show(bool show) {
	lblSliderLo->Show(show && !oneSize);
	zapCheckLo->Show(show && !oneSize && isZap);
	sliderLo->Show(show && !oneSize && !isZap);
	sliderReadoutLo->Show(show && !oneSize && !isZap);
	lblSliderHi->Show(show);
	zapCheckHi->Show(show && isZap);
	sliderHi->Show(show && !isZap);
	sliderReadoutHi->Show(show && !isZap);
	isShown = show;
}

SliderDisplay* SliderDisplayPool::Push() {
	if (pool.size() < MaxPoolSize) {
		auto entry = new SliderDisplay();
		pool.push_back(entry);
		return entry;
	}

	return nullptr;
}

void SliderDisplayPool::CreatePool(size_t poolSize, wxScrolledWindow* scrollWindow, wxSizer* sliderLayout) {
	if (poolSize > MaxPoolSize)
		poolSize = MaxPoolSize;

	pool.resize(poolSize, nullptr);

	for (auto& p : pool) {
		if (!p)
			p = new SliderDisplay();

		if (!p->IsCreated())
			p->Create(scrollWindow, sliderLayout, "sliderPoolDummy", "", "", 0, 100, false);
	}
}

SliderDisplay* SliderDisplayPool::Get(size_t index) {
	if (pool.size() > index)
		return pool[index];

	return nullptr;
}

SliderDisplay* SliderDisplayPool::GetNext() {
	for (size_t i = 0; i < pool.size(); ++i) {
		SliderDisplay* entry = pool[i];

		// Index of a slider that is invisible can be reused
		if (entry && !entry->isShown)
			return entry;
	}

	return Push();
}

void SliderDisplayPool::Clear() {
	for (auto& p : pool)
		delete p;

	pool.clear();
}
