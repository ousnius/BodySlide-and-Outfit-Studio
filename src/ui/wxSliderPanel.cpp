/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "wxSliderPanel.h"

IMPLEMENT_DYNAMIC_CLASS(wxSliderPanel, wxPanel)
 
BEGIN_EVENT_TABLE(wxSliderPanel, wxPanel)
END_EVENT_TABLE()

wxSliderPanel::wxSliderPanel() : wxPanel() {
}

wxSliderPanel::wxSliderPanel(wxWindow* parent, const wxString& name, const wxBitmap& bmpEdit, const wxBitmap& bmpSettings) : wxPanel() {
	Create(parent, name, bmpEdit, bmpSettings);
}

bool wxSliderPanel::Create(wxWindow* parent, const wxString& name, const wxBitmap& bmpEdit, const wxBitmap& bmpSettings) {
	if (isCreated) {
		btnSliderEdit->SetName(name + "|btn");
		btnSliderProp->SetName(name + "|btnSliderProp");
		btnMinus->SetName(name + "|btnMinus");
		btnPlus->SetName(name + "|btnPlus");
		sliderCheck->SetName(name + "|check");
		sliderName->SetName(name + "|lbl");
		slider->SetName(name + "|slider");
		sliderReadout->SetName(name + "|readout");

		sliderName->SetLabel(name);

		sliderCheck->Enable(true);
		slider->SetValue(0);
		sliderReadout->ChangeValue("0%");
		sliderCheck->Set3StateValue(wxCheckBoxState::wxCHK_CHECKED);

		btnSliderProp->Hide();
		btnMinus->Hide();
		btnPlus->Hide();

		editing = false;
		SetBackgroundColour(wxColour(64, 64, 64));

		Show();
		return true;
	}

	if (!wxPanelBase::Create(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSIMPLE_BORDER | wxTAB_TRAVERSAL))
		return false;

	Hide();

	SetBackgroundColour(wxColour(64, 64, 64));
	SetMinSize(wxSize(-1, 25));
	SetMaxSize(wxSize(-1, 25));

	sizer = new wxBoxSizer(wxHORIZONTAL);

	btnSliderEdit = new wxBitmapButton();
	btnSliderEdit->Create(this, wxID_ANY, bmpEdit, wxDefaultPosition, wxSize(22, 22), wxBU_AUTODRAW, wxDefaultValidator, name + "|btn");
	btnSliderEdit->SetToolTip(_("Turn on edit mode for this slider."));
	sizer->Add(btnSliderEdit, 0, wxALIGN_CENTER_VERTICAL | wxALL);

	btnSliderProp = new wxBitmapButton();
	btnSliderProp->Hide();
	btnSliderProp->Create(this, wxID_ANY, bmpSettings, wxDefaultPosition, wxSize(22, 22), wxBU_AUTODRAW, wxDefaultValidator, name + "|btnSliderProp");
	btnSliderProp->SetToolTip(_("Display and edit the active slider's properties."));
	sizer->Add(btnSliderProp, 0, wxALIGN_CENTER_VERTICAL | wxALL);

	btnMinus = new wxButton();
	btnMinus->Hide();
	btnMinus->Create(this, wxID_ANY, "-", wxDefaultPosition, wxSize(18, 18), 0, wxDefaultValidator, name + "|btnMinus");
	btnMinus->SetToolTip(_("Weaken slider data by 1%."));
	btnMinus->SetForegroundColour(wxTransparentColour);
	sizer->Add(btnMinus, 0, wxALIGN_CENTER_VERTICAL | wxALL);

	btnPlus = new wxButton();
	btnPlus->Hide();
	btnPlus->Create(this, wxID_ANY, "+", wxDefaultPosition, wxSize(18, 18), 0, wxDefaultValidator, name + "|btnPlus");
	btnPlus->SetToolTip(_("Strengthen slider data by 1%."));
	btnPlus->SetForegroundColour(wxTransparentColour);
	sizer->Add(btnPlus, 0, wxALIGN_CENTER_VERTICAL | wxALL);

	sliderCheck = new wxCheckBox();
	sliderCheck->Create(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, name + "|check");
	sliderCheck->SetForegroundColour(wxColour(255, 255, 255));
	sizer->Add(sliderCheck, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);

	sliderName = new wxStaticText();
	sliderName->Create(this, wxID_ANY, name, wxDefaultPosition, wxDefaultSize, 0, name + "|lbl");
	sliderName->SetForegroundColour(wxColour(255, 255, 255));
	sizer->Add(sliderName, 0, wxALIGN_CENTER_VERTICAL | wxALL);

	slider = new wxSlider();
	slider->Create(this, wxID_ANY, 0, 0, 100, wxDefaultPosition, wxSize(-1, -1), wxSL_HORIZONTAL, wxDefaultValidator, name + "|slider");
	slider->SetMinSize(wxSize(-1, 20));
	slider->SetMaxSize(wxSize(-1, 20));

	sizer->Add(slider, 1, wxLEFT | wxRIGHT | wxEXPAND, 5);

	sliderReadout = new wxTextCtrl();
	sliderReadout->Create(this, wxID_ANY, "0%", wxDefaultPosition, wxSize(40, -1), wxTE_RIGHT | wxTE_PROCESS_ENTER | wxSIMPLE_BORDER, wxDefaultValidator, name + "|readout");
	sliderReadout->SetMaxLength(0);
	sliderReadout->SetForegroundColour(wxColour(255, 255, 255));
	sliderReadout->SetBackgroundColour(wxColour(48, 48, 48));
	sliderReadout->SetMinSize(wxSize(40, 20));
	sliderReadout->SetMaxSize(wxSize(40, 20));

	sizer->Add(sliderReadout, 0, wxALIGN_CENTER_VERTICAL | wxALL, 2);

	SetSizer(sizer);
	sizer->Fit(this);

	isCreated = true;
	return true;
}


void wxSliderPanelPool::IncreaseSize(size_t poolSize) {
	if (poolSize <= MaxPoolSize && pool.size() < poolSize) {
		pool.resize(poolSize, nullptr);

		for (auto& p : pool) {
			if (!p) {
				p = new wxSliderPanel();
			}
		}
	}
}

void wxSliderPanelPool::CreatePool(wxWindow* parent, const wxBitmap& bmpEdit, const wxBitmap& bmpSettings) {
	for (auto& p : pool) {
		if (!p->IsCreated()) {
			p->Create(parent, "sliderPoolDummy", bmpEdit, bmpSettings);
		}
	}
}

wxSliderPanel* wxSliderPanelPool::Get(size_t index) {
	if (pool.size() > index)
		return pool[index];

	return nullptr;
}

void wxSliderPanelPool::Clear() {
	for (auto& p : pool)
		p->Destroy();

	pool.clear();
}
