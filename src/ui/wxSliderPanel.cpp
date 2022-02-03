/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "wxSliderPanel.h"

IMPLEMENT_DYNAMIC_CLASS(wxSliderPanel, wxWindow)

BEGIN_EVENT_TABLE(wxSliderPanel, wxWindow)
END_EVENT_TABLE()

wxSliderPanel::wxSliderPanel()
	: wxWindow(), m_subSliderPanel(nullptr) {}

wxSliderPanel::wxSliderPanel(wxWindow* parent, const wxString& name)
	: wxWindow(), m_subSliderPanel(nullptr) {
	Create(parent, name);
}

bool wxSliderPanel::Create(wxWindow* parent, const wxString& name) {
	SetLabel(name);

	if (isCreated) {
		SetBackgroundColour(wxColour(64, 64, 64));
		Show();
		isChecked = true;;
		sliderValue = 0;
		sliderReadoutValue = "0%";
		isEditing = false;
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

void wxSliderPanel::AttachSubSliderPanel(wxSubSliderPanel* subSliderPanel, size_t index, const wxBitmap& bmpEdit, const wxBitmap& bmpSettings) {
	auto sliderScrollSizer = GetParent()->GetSizer();
	if (m_subSliderPanel != nullptr) {
		sliderScrollSizer->Detach(subSliderPanel);
		m_subSliderPanel->Hide();
	}
	m_subSliderPanel = subSliderPanel;
	m_subSliderPanel->Create(GetParent(), GetLabel(), bmpEdit, bmpSettings);

	if (subSliderPanel->GetContainingSizer())
		sliderScrollSizer->Detach(subSliderPanel);
	sliderScrollSizer->Insert(index, subSliderPanel, 0, wxALL | wxEXPAND | wxFIXED_MINSIZE, 1);

	if (!subSliderPanel->IsShown())
		subSliderPanel->Show();

	// set the state of the visible slider to what it should be
	subSliderPanel->sliderCheck->Set3StateValue(isChecked ? wxCheckBoxState::wxCHK_CHECKED : wxCheckBoxState::wxCHK_UNCHECKED);
	subSliderPanel->sliderReadout->ChangeValue(sliderReadoutValue);
	subSliderPanel->slider->SetValue(sliderValue);
	subSliderPanel->sliderCheck->Enable(!isEditing && isChecked);
	subSliderPanel->btnSliderProp->Show(isEditing);
	subSliderPanel->btnMinus->Show(isEditing);
	subSliderPanel->btnPlus->Show(isEditing);

	Hide();
}

void wxSliderPanel::DetachSubSliderPanel(size_t index) {
	auto sliderScrollSizer = GetParent()->GetSizer();
	if (m_subSliderPanel != nullptr) {
		sliderScrollSizer->Detach(m_subSliderPanel);
		m_subSliderPanel->Hide();
		m_subSliderPanel = nullptr;
	}

	sliderScrollSizer->Detach(this);
	sliderScrollSizer->Insert(index, this, 0, wxALL | wxEXPAND | wxFIXED_MINSIZE, 1);

	Show();
}

void wxSliderPanel::SetValue(int value) {
	sliderReadoutValue = wxString::Format("%d%%", value);
	sliderValue = value;
}

void wxSliderPanel::SetChecked(bool checked) {
	isChecked = checked;
}

void wxSliderPanel::SetEditing(bool editing) {
	isEditing = editing;
}
void wxSliderPanel::SetName(const wxString& name) {
	SetLabel(name);
}

void wxSliderPanel::FocusSlider() {
	if (m_subSliderPanel)
		m_subSliderPanel->SetFocus();
}

wxSliderPanel* wxSliderPanelPool::Push() {
	if (pool.size() < MaxPoolSize) {
		auto entry = new wxSliderPanel();
		pool.push_back(entry);
		return entry;
	}

	return nullptr;
}

void wxSliderPanelPool::CreatePool(size_t poolSize, wxWindow* parent) {
	if (poolSize > MaxPoolSize)
		poolSize = MaxPoolSize;

	pool.resize(poolSize, nullptr);

	for (auto& p : pool) {
		if (!p)
			p = new wxSliderPanel();

		if (!p->IsCreated())
			p->Create(parent, "sliderPoolDummy");
	}
}

wxSliderPanel* wxSliderPanelPool::Get(size_t index) {
	if (pool.size() > index)
		return pool[index];

	return nullptr;
}

wxSliderPanel* wxSliderPanelPool::GetNext() {
	for (size_t i = 0; i < pool.size(); ++i) {
		wxSliderPanel* sliderPanel = pool[i];

		// Index of a slider panel that is invisible can be reused
		if (sliderPanel && !sliderPanel->IsShown())
			return sliderPanel;
	}

	return Push();
}

void wxSliderPanelPool::Clear() {
	for (auto& p : pool)
		p->Destroy();

	pool.clear();
}
