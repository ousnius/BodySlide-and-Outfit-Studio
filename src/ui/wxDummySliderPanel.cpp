/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "wxDummySliderPanel.h"

IMPLEMENT_DYNAMIC_CLASS(wxDummySliderPanel, wxWindow)

BEGIN_EVENT_TABLE(wxDummySliderPanel, wxWindow)
END_EVENT_TABLE()

wxDummySliderPanel::wxDummySliderPanel()
	: wxWindow() {}

wxDummySliderPanel::wxDummySliderPanel(wxWindow* parent, const wxString& name)
	: wxWindow() {
	Create(parent, name);
}

bool wxDummySliderPanel::Create(wxWindow* parent, const wxString& name) {
	if (isCreated) {
		SetLabel(name);
		m_isChecked = true;
		m_sliderValue = 0;
		m_sliderReadoutValue = "0%";
		m_isEditing = false;

		SetBackgroundColour(wxColour(64, 64, 64));
		Show();

		return true;
	}

	if (!wxWindow::Create(parent, wxID_ANY, wxDefaultPosition, wxSize(-1, 25), wxSIMPLE_BORDER | wxTAB_TRAVERSAL))
		return false;

	Hide();
	SetBackgroundColour(wxColour(64, 64, 64));
	SetMinSize(wxSize(-1, 25));
	SetMaxSize(wxSize(-1, 25));

	sizer = new wxBoxSizer(wxHORIZONTAL);
	SetSizer(sizer);

	isCreated = true;
	SetLabel(name);
	m_isChecked = true;
	m_sliderValue = 0;
	m_sliderReadoutValue = "0%";
	m_isEditing = false;

	return true;
}

void wxDummySliderPanel::AttachSubSliderPanel(wxSliderPanel* sliderPanel, wxSizer* sliderScrollSizer, int index) {
	if (!m_sliderPanel){
		m_sliderPanel = sliderPanel;
		if (m_sliderPanel->GetContainingSizer())
			sliderScrollSizer->Detach(m_sliderPanel);
		sliderScrollSizer->Insert(static_cast<size_t>(index), m_sliderPanel, 0, wxALL | wxEXPAND | wxFIXED_MINSIZE, 1);
	}

	if (!m_sliderPanel->IsShown())
		m_sliderPanel->Show();

	// set the state of the visible slider to what it should be
	m_sliderPanel->sliderCheck->Set3StateValue(m_isChecked ? wxCheckBoxState::wxCHK_CHECKED : wxCheckBoxState::wxCHK_UNCHECKED);
	m_sliderPanel->sliderReadout->ChangeValue(m_sliderReadoutValue);
	m_sliderPanel->slider->SetValue(m_sliderValue);
	m_sliderPanel->sliderCheck->Enable(!m_isEditing && m_isChecked);
	m_sliderPanel->btnSliderProp->Show(m_isEditing);
	m_sliderPanel->btnMinus->Show(m_isEditing);
	m_sliderPanel->btnPlus->Show(m_isEditing);

	Hide();
}

wxSliderPanel* wxDummySliderPanel::DetachSubSliderPanel(wxSizer* sliderScrollSizer, int index) {
	wxSliderPanel* sliderPanel = m_sliderPanel;

	if (sliderPanel) {
		sliderPanel->Hide();
		sliderScrollSizer->Detach(sliderPanel);
		m_sliderPanel = nullptr;
	}

	if (index >= 0) {
		if (!IsShown())
			Show();
	} else {
		if (IsShown())
			Hide();
	}

	return sliderPanel;
}

void wxDummySliderPanel::SetValue(int value) {
	m_sliderReadoutValue = wxString::Format("%d%%", value);
	m_sliderValue = value;

	if (!m_sliderPanel)
		return;

	m_sliderPanel->sliderReadout->ChangeValue(m_sliderReadoutValue);
	m_sliderPanel->slider->SetValue(m_sliderValue);
}

void wxDummySliderPanel::SetChecked(bool checked) {
	m_isChecked = checked;

	if (!m_sliderPanel)
		return;

	m_sliderPanel->sliderCheck->Set3StateValue(m_isChecked ? wxCheckBoxState::wxCHK_CHECKED : wxCheckBoxState::wxCHK_UNCHECKED);
}

void wxDummySliderPanel::SetEditing(bool editing) {
	m_isEditing = editing;

	if (!m_sliderPanel)
		return;

	m_sliderPanel->sliderCheck->Enable(!m_isEditing && m_isChecked);
	m_sliderPanel->btnSliderProp->Show(m_isEditing);
	m_sliderPanel->btnMinus->Show(m_isEditing);
	m_sliderPanel->btnPlus->Show(m_isEditing);

	m_sliderPanel->Layout();
}
void wxDummySliderPanel::SetName(const wxString& name) {
	SetLabel(name);

	if (!m_sliderPanel)
		return;

	m_sliderPanel->slider->SetName(name + "|slider");
	m_sliderPanel->sliderName->SetName(name + "|lbl");
	m_sliderPanel->btnSliderEdit->SetName(name + "|btn");
	m_sliderPanel->btnSliderProp->SetName(name + "|btnSliderProp");
	m_sliderPanel->btnMinus->SetName(name + "|btnMinus");
	m_sliderPanel->btnPlus->SetName(name + "|btnPlus");
	m_sliderPanel->sliderCheck->SetName(name + "|check");
	m_sliderPanel->sliderReadout->SetName(name + "|readout");
	m_sliderPanel->sliderName->SetLabel(name);
}

void wxDummySliderPanel::FocusSlider() {
	if (m_sliderPanel)
		m_sliderPanel->SetFocus();
}

wxDummySliderPanel* wxDummySliderPanelPool::Push() {
	if (pool.size() < MaxPoolSize) {
		auto entry = new wxDummySliderPanel();
		pool.push_back(entry);
		return entry;
	}

	return nullptr;
}

void wxDummySliderPanelPool::CreatePool(size_t poolSize, wxWindow* parent) {
	if (poolSize > MaxPoolSize)
		poolSize = MaxPoolSize;

	pool.resize(poolSize, nullptr);

	for (auto& p : pool) {
		if (!p)
			p = new wxDummySliderPanel();

		if (!p->IsCreated())
			p->Create(parent, "sliderPoolDummy");
	}
}

wxDummySliderPanel* wxDummySliderPanelPool::Get(size_t index) {
	if (pool.size() > index)
		return pool[index];

	return nullptr;
}

wxDummySliderPanel* wxDummySliderPanelPool::GetNext() {
	for (size_t i = 0; i < pool.size(); ++i) {
		wxDummySliderPanel* sliderPanel = pool[i];

		// Index of a slider panel that is invisible can be reused
		if (sliderPanel && !sliderPanel->IsInUse())
			return sliderPanel;
	}

	return Push();
}

void wxDummySliderPanelPool::Clear() {
	for (auto& p : pool)
		p->Destroy();

	pool.clear();
}
