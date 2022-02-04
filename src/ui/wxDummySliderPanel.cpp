/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "wxDummySliderPanel.h"

IMPLEMENT_DYNAMIC_CLASS(wxDummySliderPanel, wxWindow)

BEGIN_EVENT_TABLE(wxDummySliderPanel, wxWindow)
END_EVENT_TABLE()

wxDummySliderPanel::wxDummySliderPanel()
	: wxWindow(), m_sliderPanel(nullptr) {}

wxDummySliderPanel::wxDummySliderPanel(wxWindow* parent, const wxString& name)
	: wxWindow(), m_sliderPanel(nullptr) {
	Create(parent, name);
}

bool wxDummySliderPanel::Create(wxWindow* parent, const wxString& name) {
	SetLabel(name);

	if (isCreated) {
		SetBackgroundColour(wxColour(64, 64, 64));
		Show();
		m_isChecked = true;;
		m_sliderValue = 0;
		m_sliderReadoutValue = "0%";
		m_isEditing = false;
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
	return true;
}

void wxDummySliderPanel::AttachSubSliderPanel(wxSliderPanel* sliderPanel, wxSizer* sliderScrollSizer, size_t index) {
	if (m_sliderPanel != nullptr) {
		sliderScrollSizer->Detach(sliderPanel);
		m_sliderPanel->Hide();
	}
	m_sliderPanel = sliderPanel;

	if (sliderPanel->GetContainingSizer())
		sliderScrollSizer->Detach(sliderPanel);
	sliderScrollSizer->Insert(index, sliderPanel, 0, wxALL | wxEXPAND | wxFIXED_MINSIZE, 1);

	if (!sliderPanel->IsShown())
		sliderPanel->Show();

	// set the state of the visible slider to what it should be
	sliderPanel->sliderCheck->Set3StateValue(m_isChecked ? wxCheckBoxState::wxCHK_CHECKED : wxCheckBoxState::wxCHK_UNCHECKED);
	sliderPanel->sliderReadout->ChangeValue(m_sliderReadoutValue);
	sliderPanel->slider->SetValue(m_sliderValue);
	sliderPanel->sliderCheck->Enable(!m_isEditing && m_isChecked);
	sliderPanel->btnSliderProp->Show(m_isEditing);
	sliderPanel->btnMinus->Show(m_isEditing);
	sliderPanel->btnPlus->Show(m_isEditing);

	Hide();
}

wxSliderPanel* wxDummySliderPanel::DetachSubSliderPanel(wxSizer* sliderScrollSizer, size_t index) {
	wxSliderPanel* sliderPanel = m_sliderPanel;

	if (m_sliderPanel != nullptr) {
		sliderScrollSizer->Detach(m_sliderPanel);
		m_sliderPanel->Hide();
		m_sliderPanel = nullptr;
	}

	sliderScrollSizer->Detach(this);
	sliderScrollSizer->Insert(index, this, 0, wxALL | wxEXPAND | wxFIXED_MINSIZE, 1);

	Show();

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
		if (sliderPanel && !sliderPanel->IsShown())
			return sliderPanel;
	}

	return Push();
}

void wxDummySliderPanelPool::Clear() {
	for (auto& p : pool)
		p->Destroy();

	pool.clear();
}
