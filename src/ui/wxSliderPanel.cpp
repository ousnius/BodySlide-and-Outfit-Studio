/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "wxSliderPanel.h"

IMPLEMENT_DYNAMIC_CLASS(wxSliderPanel, wxWindow)

BEGIN_EVENT_TABLE(wxSliderPanel, wxWindow)
END_EVENT_TABLE()

wxSliderPanel::wxSliderPanel()
	: wxWindow() {}

wxSliderPanel::wxSliderPanel(wxWindow* parent, const wxString& name)
	: wxWindow() {
	Create(parent, name);
}

bool wxSliderPanel::Create(wxWindow* parent, const wxString& name) {
	SetLabel(name);

	if (isCreated) {
		SetBackgroundColour(wxColour(64, 64, 64));
		Show();
		return true;
	}

	if (!wxWindow::Create(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSIMPLE_BORDER | wxTAB_TRAVERSAL))
		return false;

	Hide();
	SetBackgroundColour(wxColour(64, 64, 64));

	//TODO: SliderPanel needs to match the side it would be if a subSliderPanelIsAttached
	SetMinSize(wxSize(-1, 40));
	SetMaxSize(wxSize(-1, 40));
	sizer = new wxBoxSizer(wxHORIZONTAL);

	SetSizer(sizer);

	isCreated = true;
	return true;
}

bool wxSliderPanel::AttachSubSliderPanel(wxSubSliderPanel* subSliderPanel) {
	if (this->subSliderPanel != nullptr)
		return false;

	this->subSliderPanel = subSliderPanel;

	//TODO: attach sub slider (update names from GeLabel())

	return true;
}

wxSubSliderPanel* wxSliderPanel::DetachSubSliderPanel() {
	if (this->subSliderPanel == nullptr)
		return nullptr;

	auto subSliderPanel = this->subSliderPanel;

	//TODO: detach sub slider

	return subSliderPanel;
}

void wxSliderPanel::SetValue(float value) {
	sliderReadoutValue = wxString::Format("%d%%", value);
	sliderValue = value;
}

void wxSliderPanel::SetChecked(bool checked) {
	isChecked = checked;
}

void wxSliderPanel::SetEditing(bool editing) {
	isEditing = editing;
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
