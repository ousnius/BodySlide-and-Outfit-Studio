/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

// Outfit Studio window: the slider list, slider editing and slider operations

#include "OutfitStudioFrame.h"
#include "PhysicsPreview.h"
#include "FrameHelpers.h"
#include "components/UndoHistory.h"
#include "DropTargets.h"
#include "GLPanel.h"
#include "ShapeTreeItems.h"
#include "utils/ConfigurationManager.h"
#include "ui/wxStateButton.h"
#include "components/SliderPresets.h"
#include "files/HkxFile.h"
#include "ui/outfitstudio/PartitionTypeChoices.h"
#include "ui/outfitstudio/SliderDataDialog.h"

#include <cmath>
#include <cstdlib>
#include <iterator>
#include <sstream>
#include <wx/grid.h>
#include <wx/listctrl.h>
#include <wx/textctrl.h>

using namespace nifly;

void OutfitStudioFrame::CreateSetSliders() {
	wxSizer* rootSz = sliderScroll->GetSizer();

	int inc = 1;
	if (project->SliderCount()) {
		inc = 90 / project->SliderCount();
		StartProgress(_("Creating sliders..."));
	}

	UpdateProgress(0, _("Clearing old sliders..."));

	sliderScroll->Freeze();

	for (auto& sliderPanel : sliderPanels)
		HideSliderPanel(sliderPanel.second);

	sliderPanels.clear();
	sliderFilter->Clear();

	// A loaded project starts at zero, so a selected preset is off until it's turned on again.
	// Rebuilding the sliders for other reasons (an import, for one) keeps the values and the preset state.
	if (presetOff && presetQuick && presetQuick->GetSelection() != wxNOT_FOUND) {
		bool allZero = true;
		for (size_t i = 0; i < project->SliderCount() && allZero; i++) {
			if (project->SliderClamp(i))
				continue;
			if (editor.bEditSlider && project->GetSliderName(i) == editor.activeSlider)
				continue;
			if (project->SliderValue(i) != 0.0f)
				allZero = false;
		}

		if (allZero)
			presetOff->SetValue(true);
	}
	UpdateQuickPresetWeightVisibility();

	for (size_t i = 0; i < project->SliderCount(); i++) {
		UpdateProgress(inc, _("Loading slider: ") + project->GetSliderName(i));
		if (project->SliderClamp(i)) // clamp sliders are a special case, usually an incorrect scale
			continue;

		createSliderGUI(project->GetSliderName(i), sliderScroll, rootSz);
	}

	if (!sliderScroll->GetDropTarget())
		sliderScroll->SetDropTarget(new DnDSliderFile(this));

	sliderScroll->FitInside();
	sliderScroll->Thaw();

	DoFilterSliders();
	HighlightSliderData();

	EndProgress();
}

void OutfitStudioFrame::createSliderGUI(const std::string& name, wxScrolledWindow* wnd, wxSizer* rootSz) {
	wxString sliderName = wxString::FromUTF8(name);

	wxSliderPanel* sliderPanel = sliderPool.GetNext();
	if (sliderPanel) {
		int minValue = Config.GetIntValue("Input/SliderMinimum");
		int maxValue = Config.GetIntValue("Input/SliderMaximum");

		if (sliderPanel->Create(wnd, sliderName, minValue, maxValue, *bmpEditSlider, *bmpSliderSettings)) {
			// Hover enter
			sliderPanel->btnSliderEdit->Bind(wxEVT_ENTER_WINDOW, &OutfitStudioFrame::OnEnterHoverSlider, this);
			sliderPanel->sliderCheck->Bind(wxEVT_ENTER_WINDOW, &OutfitStudioFrame::OnEnterHoverSlider, this);
			sliderPanel->sliderName->Bind(wxEVT_ENTER_WINDOW, &OutfitStudioFrame::OnEnterHoverSlider, this);
			sliderPanel->slider->Bind(wxEVT_ENTER_WINDOW, &OutfitStudioFrame::OnEnterHoverSlider, this);
			sliderPanel->sliderReadout->Bind(wxEVT_ENTER_WINDOW, &OutfitStudioFrame::OnEnterHoverSlider, this);

			// Hover leave
			sliderPanel->btnSliderEdit->Bind(wxEVT_LEAVE_WINDOW, &OutfitStudioFrame::OnLeaveHoverSlider, this);
			sliderPanel->sliderCheck->Bind(wxEVT_LEAVE_WINDOW, &OutfitStudioFrame::OnLeaveHoverSlider, this);
			sliderPanel->sliderName->Bind(wxEVT_LEAVE_WINDOW, &OutfitStudioFrame::OnLeaveHoverSlider, this);
			sliderPanel->slider->Bind(wxEVT_LEAVE_WINDOW, &OutfitStudioFrame::OnLeaveHoverSlider, this);
			sliderPanel->sliderReadout->Bind(wxEVT_LEAVE_WINDOW, &OutfitStudioFrame::OnLeaveHoverSlider, this);

			sliderPanel->btnSliderEdit->Bind(wxEVT_BUTTON, &OutfitStudioFrame::OnClickSliderButton, this);
			sliderPanel->btnSliderProp->Bind(wxEVT_BUTTON, &OutfitStudioFrame::OnClickSliderButton, this);
			sliderPanel->btnMinus->Bind(wxEVT_BUTTON, &OutfitStudioFrame::OnClickSliderButton, this);
			sliderPanel->btnPlus->Bind(wxEVT_BUTTON, &OutfitStudioFrame::OnClickSliderButton, this);
			sliderPanel->sliderCheck->Bind(wxEVT_CHECKBOX, &OutfitStudioFrame::OnSliderCheckBox, this);
			sliderPanel->slider->Bind(wxEVT_SLIDER, &OutfitStudioFrame::OnSlider, this);
			sliderPanel->sliderReadout->Bind(wxEVT_TEXT, &OutfitStudioFrame::OnReadoutChange, this);

			if (sliderPanel->GetContainingSizer())
				rootSz->Detach(sliderPanel);

			rootSz->Add(sliderPanel, 0, wxALL | wxEXPAND | wxFIXED_MINSIZE, 1);

			if (!sliderPanel->IsShown())
				sliderPanel->Show();

			// Sliders for an add-on are only shown in BodySlide where it's installed
			const SliderData& sliderData = project->activeSet[name];
			if (sliderData.NeedsAddOn()) {
				sliderPanel->sliderName->SetToolTip(
					wxString::Format(_("Made for add-on '%s'. BodySlide only shows the slider where the add-on is installed."),
									 wxString::FromUTF8(sliderData.addOnName.empty() ? sliderData.addOnID : sliderData.addOnName)));
				sliderPanel->sliderName->SetForegroundColour(wxColour(120, 190, 255));
			}
			else {
				sliderPanel->sliderName->UnsetToolTip();
				sliderPanel->sliderName->SetForegroundColour(wxColour(255, 255, 255));
			}
			sliderPanel->sliderName->Refresh();

			sliderPanels[name] = sliderPanel;
			ShowSliderEffect(name);
		}
	}
}

std::string OutfitStudioFrame::NewSlider(const std::string& suggestedName, bool skipPrompt) {
	std::string baseName = "New Slider";
	if (!suggestedName.empty())
		baseName = suggestedName;

	int count = 1;
	std::string fillName = baseName;

	while (project->ValidSlider(fillName))
		fillName = wxString::Format("%s %d", baseName, ++count).ToUTF8();

	std::string sliderName;
	if (!skipPrompt) {
		do {
			sliderName = GetTextFromUser(_("Enter a name for the new slider:"), _("Create New Slider"), fillName, this).ToUTF8();
			if (sliderName.empty())
				return sliderName;
		} while (project->ValidSlider(sliderName));
	}
	else
		sliderName = fillName;

	wxLogMessage("Creating new slider '%s'.", sliderName);

	project->AddEmptySlider(sliderName);
	createSliderGUI(sliderName, sliderScroll, sliderScroll->GetSizer());

	sliderScroll->FitInside();
	SetPendingChanges();

	return sliderName;
}

void OutfitStudioFrame::SyncSliderPanels() {
	for (size_t i = 0; i < project->SliderCount(); i++) {
		auto it = sliderPanels.find(project->GetSliderName(i));
		if (it == sliderPanels.end() || !it->second)
			continue;

		int val = static_cast<int>(std::round(project->SliderValue(i) * 100.0f));
		it->second->sliderReadout->ChangeValue(wxString::Format("%d%%", val));
		it->second->slider->SetValue(val);
		it->second->sliderCheck->Set3StateValue(project->SliderShow(i) ? wxCheckBoxState::wxCHK_CHECKED : wxCheckBoxState::wxCHK_UNCHECKED);
	}
}

void OutfitStudioFrame::SetSliderValue(const size_t index, int val) {
	std::string name = project->GetSliderName(index);
	project->SliderValue(index) = val / 100.0f;
	auto it = sliderPanels.find(name);
	if (it != sliderPanels.end() && it->second) {
		it->second->sliderReadout->ChangeValue(wxString::Format("%d%%", val));
		it->second->slider->SetValue(val);
	}
}

void OutfitStudioFrame::SetSliderValue(const std::string& name, int val) {
	project->SliderValue(name) = val / 100.0f;
	auto it = sliderPanels.find(name);
	if (it != sliderPanels.end() && it->second) {
		it->second->sliderReadout->ChangeValue(wxString::Format("%d%%", val));
		it->second->slider->SetValue(val);
	}
}

void OutfitStudioFrame::UpdateLiveMesh(NiShape* shape, bool updateBVH, bool updateUVs) {
	// Only model space normals need the rotations, tangent space ones follow the vertex normals
	Mesh* m = glView->GetMesh(shape->name.get());
	const bool wantsRotations = m && m->modelSpace;

	std::vector<Vector3> verts;
	std::vector<Vector2> uvs;
	std::vector<Matrix3> poseRotations;
	project->GetLiveVerts(shape, verts, updateUVs ? &uvs : nullptr, wantsRotations ? &poseRotations : nullptr);
	glView->UpdateMeshVertices(shape->name.get(), &verts, updateBVH, true, false, updateUVs ? &uvs : nullptr);

	if (m) {
		if (poseRotations.empty())
			m->ClearPoseRotations();
		else
			m->SetPoseRotations(poseRotations);
	}
}

void OutfitStudioFrame::ApplySliders(bool recalcBVH) {
	for (auto& shape : project->GetWorkNif()->GetShapes())
		UpdateLiveMesh(shape, recalcBVH, true);

	physicsPreview->UpdateShapes();

	bool tMode = glView->GetTransformMode();

	if (tMode)
		glView->ShowTransformTool();

	if (!tMode)
		glView->Render();
}

void OutfitStudioFrame::ShowSliderEffect(const std::string& sliderName, bool show) {
	if (project->ValidSlider(sliderName)) {
		project->SliderShow(sliderName) = show;

		wxSliderPanel* sliderPanel = GetSliderPanel(sliderName);
		if (sliderPanel) {
			if (show)
				sliderPanel->sliderCheck->Set3StateValue(wxCheckBoxState::wxCHK_CHECKED);
			else
				sliderPanel->sliderCheck->Set3StateValue(wxCheckBoxState::wxCHK_UNCHECKED);
		}
	}
}

wxSliderPanel* OutfitStudioFrame::GetSliderPanel(const std::string& sliderName) {
	// Don't use operator[], it would insert a null panel for unknown names
	auto it = sliderPanels.find(sliderName);
	if (it != sliderPanels.end())
		return it->second;

	return nullptr;
}

void OutfitStudioFrame::HighlightSliderData() {
	for (auto& sliderPanel : sliderPanels) {
		sliderPanel.second->btnSliderEdit->SetBitmap(*bmpEditSlider);

		SliderData& sd = project->activeSet[sliderPanel.first];

		for (auto& i : selectedItems) {
			auto diff = project->GetDiffSet(sd, i->GetShape());
			if (diff && !diff->empty()) {
				sliderPanel.second->btnSliderEdit->SetBitmap(*bmpEditSliderGreen);
				break;
			}
		}
	}
}

void OutfitStudioFrame::HideSliderPanel(wxSliderPanel* sliderPanel) {
	if (!sliderPanel)
		return;

	// Hover enter
	sliderPanel->btnSliderEdit->Unbind(wxEVT_ENTER_WINDOW, &OutfitStudioFrame::OnEnterHoverSlider, this);
	sliderPanel->sliderCheck->Unbind(wxEVT_ENTER_WINDOW, &OutfitStudioFrame::OnEnterHoverSlider, this);
	sliderPanel->sliderName->Unbind(wxEVT_ENTER_WINDOW, &OutfitStudioFrame::OnEnterHoverSlider, this);
	sliderPanel->slider->Unbind(wxEVT_ENTER_WINDOW, &OutfitStudioFrame::OnEnterHoverSlider, this);
	sliderPanel->sliderReadout->Unbind(wxEVT_ENTER_WINDOW, &OutfitStudioFrame::OnEnterHoverSlider, this);

	// Hover leave
	sliderPanel->btnSliderEdit->Unbind(wxEVT_LEAVE_WINDOW, &OutfitStudioFrame::OnLeaveHoverSlider, this);
	sliderPanel->sliderCheck->Unbind(wxEVT_LEAVE_WINDOW, &OutfitStudioFrame::OnLeaveHoverSlider, this);
	sliderPanel->sliderName->Unbind(wxEVT_LEAVE_WINDOW, &OutfitStudioFrame::OnLeaveHoverSlider, this);
	sliderPanel->slider->Unbind(wxEVT_LEAVE_WINDOW, &OutfitStudioFrame::OnLeaveHoverSlider, this);
	sliderPanel->sliderReadout->Unbind(wxEVT_LEAVE_WINDOW, &OutfitStudioFrame::OnLeaveHoverSlider, this);

	sliderPanel->btnSliderEdit->Unbind(wxEVT_BUTTON, &OutfitStudioFrame::OnClickSliderButton, this);
	sliderPanel->btnSliderProp->Unbind(wxEVT_BUTTON, &OutfitStudioFrame::OnClickSliderButton, this);
	sliderPanel->btnMinus->Unbind(wxEVT_BUTTON, &OutfitStudioFrame::OnClickSliderButton, this);
	sliderPanel->btnPlus->Unbind(wxEVT_BUTTON, &OutfitStudioFrame::OnClickSliderButton, this);
	sliderPanel->sliderCheck->Unbind(wxEVT_CHECKBOX, &OutfitStudioFrame::OnSliderCheckBox, this);
	sliderPanel->slider->Unbind(wxEVT_SLIDER, &OutfitStudioFrame::OnSlider, this);
	sliderPanel->sliderReadout->Unbind(wxEVT_TEXT, &OutfitStudioFrame::OnReadoutChange, this);

	sliderPanel->Hide();
}

void OutfitStudioFrame::EnterSliderEdit(const std::string& sliderName) {
	std::string sliderNameEdit = sliderName;

	if (sliderNameEdit.empty()) {
		if (editor.lastActiveSlider.empty())
			sliderNameEdit = project->GetSliderName(0);
		else
			sliderNameEdit = editor.lastActiveSlider;
	}

	if (sliderNameEdit.empty())
		return;

	if (editor.bEditSlider) {
		wxSliderPanel* sliderPanel = GetSliderPanel(editor.activeSlider);
		if (sliderPanel) {
			sliderPanel->sliderCheck->Enable(true);
			sliderPanel->btnSliderProp->Hide();
			sliderPanel->btnMinus->Hide();
			sliderPanel->btnPlus->Hide();
			sliderPanel->Layout();
		}
	}

	wxSliderPanel* sliderPanel = GetSliderPanel(sliderNameEdit);
	if (!sliderPanel)
		return;

	editor.activeSlider = sliderNameEdit;
	editor.lastActiveSlider = editor.activeSlider;
	editor.bEditSlider = true;

	sliderPanel->slider->SetValue(100);
	SetSliderValue(editor.activeSlider, 100);
	ShowSliderEffect(editor.activeSlider, true);

	sliderPanel->sliderCheck->Enable(false);
	sliderPanel->btnSliderProp->Show();
	sliderPanel->btnMinus->Show();
	sliderPanel->btnPlus->Show();
	sliderPanel->Layout();
	MenuEnterSliderEdit();

	HighlightSlider(editor.activeSlider);
	ApplySliders();
}

void OutfitStudioFrame::ExitSliderEdit() {
	if (!editor.activeSlider.empty()) {
		wxSliderPanel* sliderPanel = GetSliderPanel(editor.activeSlider);
		if (sliderPanel) {
			sliderPanel->sliderCheck->Enable(true);
			sliderPanel->slider->SetValue(0);
			SetSliderValue(editor.activeSlider, 0);
			ShowSliderEffect(editor.activeSlider, true);
			sliderPanel->btnSliderProp->Hide();
			sliderPanel->btnMinus->Hide();
			sliderPanel->btnPlus->Hide();
			sliderPanel->Layout();
		}

		editor.activeSlider.clear();
	}

	editor.bEditSlider = false;
	MenuExitSliderEdit();

	HighlightSlider(editor.activeSlider);
	ApplySliders();
}

void OutfitStudioFrame::MenuEnterSliderEdit() {
	menuBar->Enable(XRCID("menuImportSlider"), true);
	menuBar->Enable(XRCID("menuExportSlider"), true);
	menuBar->Enable(XRCID("sliderClone"), true);
	menuBar->Enable(XRCID("sliderNegate"), true);
	menuBar->Enable(XRCID("sliderMask"), true);
	menuBar->Enable(XRCID("sliderProperties"), true);
}

void OutfitStudioFrame::MenuExitSliderEdit() {
	menuBar->Enable(XRCID("menuImportSlider"), false);
	menuBar->Enable(XRCID("menuExportSlider"), false);
	menuBar->Enable(XRCID("sliderClone"), false);
	menuBar->Enable(XRCID("sliderNegate"), false);
	menuBar->Enable(XRCID("sliderMask"), false);
	menuBar->Enable(XRCID("sliderProperties"), false);
}

void OutfitStudioFrame::ScrollToActiveSlider() {
	if (!editor.activeSlider.empty()) {
		for (auto& sliderPanel : sliderPanels) {
			if (sliderPanel.first == editor.activeSlider) {
				ScrollWindowIntoView(sliderScroll, sliderPanel.second);
			}
		}
	}
}

bool OutfitStudioFrame::ConfirmSliderDataLocalForEdit(NiShape* shape, const std::string& sliderName) {
	std::vector<NiShape*> shapes;
	if (shape)
		shapes.push_back(shape);

	std::vector<std::string> sliderNames;
	if (!sliderName.empty())
		sliderNames.push_back(sliderName);

	return ConfirmSliderDataLocalForEdit(shapes, sliderNames);
}

bool OutfitStudioFrame::ConfirmSliderDataLocalForEdit(const std::vector<NiShape*>& shapes, const std::vector<std::string>& sliderNames) {
	if (!project || shapes.empty() || sliderNames.empty())
		return true;

	std::vector<std::pair<NiShape*, std::string>> externalData;
	std::vector<std::string> labels;
	bool inheritedData = false;

	for (auto* shape : shapes) {
		if (!shape)
			continue;

		std::string shapeName = shape->name.get();
		for (const auto& sliderName : sliderNames) {
			if (sliderName.empty() || !project->SliderDataIsExternal(sliderName, shape))
				continue;

			if (!project->RenameDisconnectsFromBase(sliderName, shapeName).empty())
				inheritedData = true;

			externalData.emplace_back(shape, sliderName);
			std::string label = shapeName + " / " + sliderName;
			if (std::find(labels.begin(), labels.end(), label) == labels.end())
				labels.push_back(label);
		}
	}

	if (externalData.empty())
		return true;

	wxString message = inheritedData ?
		_("The slider data you are about to edit comes from the base project. Editing it saves a copy with this project, which overrides the base project's data. Continue?") :
		_("The slider data you are about to edit is external. Editing it will make the affected slider data local to this project. Continue and make it local?");
	if (!labels.empty()) {
		message += "\n\n";
		message += _("Affected slider data:");

		size_t maxLabels = std::min<size_t>(labels.size(), 8);
		for (size_t i = 0; i < maxLabels; i++) {
			message += "\n  ";
			message += wxString::FromUTF8(labels[i]);
		}

		if (labels.size() > maxLabels)
			message += "\n  ...";
	}

	int response = wxMessageBox(message, _("External Slider Data"), wxYES_NO | wxICON_WARNING, this);
	if (response != wxYES)
		return false;

	for (auto& data : externalData)
		project->EnsureSliderDataLocal(data.second, data.first);

	SetPendingChanges();
	HighlightSliderData();
	return true;
}

void OutfitStudioFrame::OnSelectSliders(wxCommandEvent& event) {
	bool checked = event.IsChecked();
	for (auto& sliderPanel : sliderPanels)
		ShowSliderEffect(sliderPanel.first, checked);

	ApplySliders();
}

void OutfitStudioFrame::OnSliderFilterChanged(wxCommandEvent& WXUNUSED(event)) {
	DoFilterSliders();
}

void OutfitStudioFrame::DoFilterSliders() {
	wxString filterStr = sliderFilter->GetValue();
	filterStr.MakeLower();

	for (auto& sliderPanel : sliderPanels) {
		if (!sliderPanel.second)
			continue;

		// Filter slider by name
		wxString sliderStr = wxString::FromUTF8(sliderPanel.first);
		if (sliderStr.Lower().Contains(filterStr)) {
			if (!sliderPanel.second->IsShown())
				sliderPanel.second->Show();
		}
		else {
			if (sliderPanel.second->IsShown())
				sliderPanel.second->Hide();
		}
	}

	sliderScroll->FitInside();
}

void OutfitStudioFrame::OnSliderCheckBox(wxCommandEvent& event) {
	wxCheckBox* box = (wxCheckBox*)event.GetEventObject();
	if (!box)
		return;

	bool checked = event.IsChecked();

	std::string sliderName = box->GetName().BeforeLast('|').ToStdString();
	ShowSliderEffect(sliderName, checked);

	bool shiftDown = wxGetKeyState(WXK_SHIFT);

	if (!lastCheckedSlider.empty() && shiftDown) {
		wxSliderPanel* sliderPanel = GetSliderPanel(sliderName);
		wxSliderPanel* lastSliderPanel = GetSliderPanel(lastCheckedSlider);

		if (sliderPanel && lastSliderPanel) {
			const size_t sliderIndex = sliderPool.FindIndex(sliderPanel);
			const size_t lastSliderIndex = sliderPool.FindIndex(lastSliderPanel);
			const size_t invalidSliderIndex = static_cast<size_t>(-1);

			if (sliderIndex != invalidSliderIndex && lastSliderIndex != invalidSliderIndex && sliderIndex != lastSliderIndex) {
				size_t startIndex, endIndex;
				if (sliderIndex > lastSliderIndex) {
					startIndex = lastSliderIndex;
					endIndex = sliderIndex;
				}
				else {
					startIndex = sliderIndex;
					endIndex = lastSliderIndex;
				}

				for (size_t i = startIndex; i < endIndex; i++) {
					wxSliderPanel* sliderPanelIndex = sliderPool.Get(i);
					if (sliderPanelIndex && sliderPanelIndex->IsCreated()) {
						std::string sliderNameIndex = sliderPanelIndex->sliderCheck->GetName().BeforeLast('|').ToStdString();
						ShowSliderEffect(sliderNameIndex, checked);
					}
				}
			}
		}
	}

	ApplySliders();

	if (!shiftDown)
		lastCheckedSlider = sliderName;
}

void OutfitStudioFrame::OnClickSliderButton(wxCommandEvent& event) {
	wxWindow* btn = (wxWindow*)event.GetEventObject();
	if (!btn)
		return;

	wxString buttonName = btn->GetName();
	std::string clickedName{buttonName.BeforeLast('|').ToUTF8()};
	if (clickedName.empty()) {
		event.Skip();
		return;
	}

	wxString buttonNameId = buttonName.AfterLast('|');

	float scale = 0.0f;
	if (buttonNameId == "btnMinus")
		scale = 0.99f;
	else if (buttonNameId == "btnPlus")
		scale = 1.01f;

	if (scale != 0.0f) {
		std::vector<NiShape*> shapes;
		for (auto& i : selectedItems)
			shapes.push_back(i->GetShape());

		if (!ConfirmSliderDataLocalForEdit(shapes, std::vector<std::string>{editor.activeSlider}))
			return;

		for (auto& i : selectedItems) {
			auto shape = i->GetShape();
			std::vector<Vector3> verts;
			project->ScaleMorphResult(shape, editor.activeSlider, scale);
			project->GetLiveVerts(shape, verts);
			glView->UpdateMeshVertices(shape->name.get(), &verts);
		}
		return;
	}

	if (buttonNameId == "btnSliderProp") {
		ShowSliderProperties(clickedName);
		return;
	}

	if (editor.activeSlider != clickedName)
		EnterSliderEdit(clickedName);
	else
		ExitSliderEdit();
}

void OutfitStudioFrame::OnEnterHoverSlider(wxMouseEvent& event) {
	event.Skip();

	if (!event.ControlDown())
		return;

	if (currentTabButton != meshTabButton)
		return;

	wxWindow* winObj = (wxWindow*)event.GetEventObject();
	if (!winObj)
		return;

	wxString objName = winObj->GetName();
	std::string sliderName{objName.BeforeLast('|').ToUTF8()};
	if (sliderName.empty())
		return;

	SliderData& sd = project->activeSet[sliderName];

	for (auto& shape : project->GetWorkNif()->GetShapes()) {
		Mesh* m = glView->GetMesh(shape->name.get());
		if (m && m->weight) {
			// Clear color
			m->WeightFill(0.0f);

			auto diff = project->GetDiffSet(sd, shape);
			if (!diff)
				continue;

			// Get largest diff (length)
			float maxLength = 0.0f;
			for (auto& d : *diff) {
				if (d.second.length() > maxLength)
					maxLength = d.second.length();
			}

			// Apply color gradient to all vertices with diffs
			for (auto& d : *diff) {
				if (m->nVerts > d.first) {
					float gradient = d.second.length() / maxLength;
					m->weight[d.first] = gradient;
				}
			}

			m->QueueUpdate(Mesh::UpdateType::Weight);
		}
	}

	glView->SetWeightVisible();
	glView->Render();
}

void OutfitStudioFrame::OnLeaveHoverSlider(wxMouseEvent& event) {
	event.Skip();

	if (currentTabButton != meshTabButton)
		return;

	for (auto& shape : project->GetWorkNif()->GetShapes()) {
		Mesh* m = glView->GetMesh(shape->name.get());
		if (m && m->weight) {
			// Clear color
			m->WeightFill(0.0f);
		}
	}

	glView->SetWeightVisible(false);
	glView->Render();
}

void OutfitStudioFrame::OnReadoutChange(wxCommandEvent& event) {
	wxTextCtrl* w = (wxTextCtrl*)event.GetEventObject();
	event.Skip();
	if (!w)
		return;

	wxString sn = w->GetName();
	if (!sn.EndsWith("|readout", &sn))
		return;

	std::string sliderName{sn.ToUTF8()};

	double v;
	wxString val = w->GetValue();
	val.Replace("%", "");
	if (!val.ToDouble(&v))
		return;

	wxSliderPanel* sliderPanel = GetSliderPanel(sliderName);
	if (!sliderPanel)
		return;

	sliderPanel->slider->SetValue(v);

	project->SliderValue(sliderName) = v / 100.0f;

	ApplySliders();
}

void OutfitStudioFrame::ScrollWindowIntoView(wxScrolledWindow* scrolled, wxWindow* window) {
	// "window" must be an immediate child of "scrolled"
	int scrollRateY = 0;
	scrolled->GetScrollPixelsPerUnit(nullptr, &scrollRateY);

	wxPoint window_pos = scrolled->CalcUnscrolledPosition(window->GetPosition());
	scrolled->Scroll(0, window_pos.y / scrollRateY);
}

void OutfitStudioFrame::HighlightSlider(const std::string& name) {
	for (auto& d : sliderPanels) {
		if (d.first == name) {
			d.second->editing = true;
			d.second->SetBackgroundColour(wxColour(125, 77, 138));
		}
		else {
			d.second->editing = false;
			d.second->SetBackgroundColour(wxColour(64, 64, 64));
		}
	}
	sliderScroll->Refresh();
}

void OutfitStudioFrame::ZeroSliders() {
	ApplyRefresh(ProjectOperations(*project, editor).ZeroSliders());
}

void OutfitStudioFrame::OnSlider(wxCommandEvent& event) {
	wxSlider* s = ((wxSlider*)event.GetEventObject());
	if (!s)
		return;

	wxString sliderName = s->GetName();
	std::string sn{sliderName.BeforeLast('|').ToUTF8()};
	if (sn.empty())
		return;

	SetSliderValue(sn, event.GetInt());

	ApplySliders();
}

void OutfitStudioFrame::OnClearSlider(wxCommandEvent& WXUNUSED(event)) {
	if (!ShapeSelectionCheck())
		return;

	int result;
	if (selectedItems.size() > 1) {
		wxString prompt = _("Are you sure you wish to clear the unmasked slider data for the selected shapes?  This action cannot be undone.");
		result = wxMessageBox(prompt, _("Confirm data erase"), wxYES_NO | wxICON_WARNING, this);
	}
	else {
		wxString prompt = wxString::Format(_("Are you sure you wish to clear the unmasked slider data for the shape '%s'?  This action cannot be undone."),
										   activeItem->GetShape()->name.get());
		result = wxMessageBox(prompt, _("Confirm data erase"), wxYES_NO | wxICON_WARNING, this);
	}

	if (result != wxYES)
		return;

	std::vector<NiShape*> shapes;
	for (auto& i : selectedItems)
		shapes.push_back(i->GetShape());

	std::vector<std::string> sliderNames;
	if (!editor.bEditSlider) {
		for (auto& sliderPanel : sliderPanels)
			if (sliderPanel.second->sliderCheck->Get3StateValue() == wxCheckBoxState::wxCHK_CHECKED)
				sliderNames.push_back(sliderPanel.first);
	}
	else
		sliderNames.push_back(editor.activeSlider);

	if (!ConfirmSliderDataLocalForEdit(shapes, sliderNames))
		return;

	auto clearSlider = [&](const std::string& sliderName) {
		std::unordered_map<uint16_t, float> mask;
		for (auto& i : selectedItems) {
			mask.clear();
			glView->GetShapeMask(mask, i->GetShape()->name.get());
			if (mask.size() > 0)
				project->ClearUnmaskedDiff(i->GetShape(), sliderName, &mask);
			else
				project->ClearSlider(i->GetShape(), sliderName);
		}
	};

	if (!editor.bEditSlider) {
		wxLogMessage("Clearing slider data of the checked sliders for the selected shapes.");
		for (auto& sliderPanel : sliderPanels)
			if (sliderPanel.second->sliderCheck->Get3StateValue() == wxCheckBoxState::wxCHK_CHECKED)
				clearSlider(sliderPanel.first);
	}
	else {
		wxLogMessage("Clearing slider data of '%s' for the selected shapes.", editor.activeSlider);
		clearSlider(editor.activeSlider);
	}

	SetPendingChanges();
	ApplySliders();
	HighlightSliderData();
}

void OutfitStudioFrame::OnNewSlider(wxCommandEvent& WXUNUSED(event)) {
	NewSlider();
}

void OutfitStudioFrame::OnNewZapSlider(wxCommandEvent& WXUNUSED(event)) {
	if (!ShapeSelectionCheck())
		return;

	std::string baseName = "New Zap";

	int count = 1;
	std::string fillName = baseName;

	while (project->ValidSlider(fillName))
		fillName = wxString::Format("%s %d", baseName, ++count).ToUTF8();

	std::string sliderName;
	do {
		sliderName = GetTextFromUser(_("Enter a name for the new zap:"), _("Create New Zap"), fillName, this).ToUTF8();
		if (sliderName.empty())
			return;
	} while (project->ValidSlider(sliderName));

	wxLogMessage("Creating new zap '%s'.", sliderName);

	std::unordered_map<uint16_t, float> unmasked;
	for (auto& i : selectedItems) {
		unmasked.clear();
		glView->GetShapeUnmasked(unmasked, i->GetShape()->name.get());
		project->AddZapSlider(sliderName, unmasked, i->GetShape());
	}

	createSliderGUI(sliderName, sliderScroll, sliderScroll->GetSizer());

	sliderScroll->FitInside();
	SetPendingChanges();
}

void OutfitStudioFrame::OnNewCombinedSlider(wxCommandEvent& WXUNUSED(event)) {
	std::string baseName = "New Slider";

	int count = 1;
	std::string fillName = baseName;

	while (project->ValidSlider(fillName))
		fillName = wxString::Format("%s %d", baseName, ++count).ToUTF8();

	std::string sliderName;
	do {
		sliderName = GetTextFromUser(_("Enter a name for the new slider:"), _("Create New Slider"), fillName, this).ToUTF8();
		if (sliderName.empty())
			return;
	} while (project->ValidSlider(sliderName));

	wxLogMessage("Creating new combined slider '%s'.", sliderName);

	createSliderGUI(sliderName, sliderScroll, sliderScroll->GetSizer());

	project->AddCombinedSlider(sliderName);
	sliderScroll->FitInside();
	SetPendingChanges();
}

void OutfitStudioFrame::OnSliderClone(wxCommandEvent& WXUNUSED(event)) {
	if (!editor.bEditSlider) {
		wxMessageBox(_("There is no slider in edit mode to clone!"), _("Error"));
		return;
	}

	wxLogMessage("Cloning slider '%s'.", editor.activeSlider);

	std::string baseName = "Cloned Slider";

	int count = 1;
	std::string fillName = baseName;

	while (project->ValidSlider(fillName))
		fillName = wxString::Format("%s %d", baseName, ++count).ToUTF8();

	std::string sliderName;
	do {
		sliderName = GetTextFromUser(_("Enter a name for the cloned slider:"), _("Clone Slider"), fillName, this).ToUTF8();
		if (sliderName.empty())
			return;
	} while (project->ValidSlider(sliderName));

	wxLogMessage("Creating cloned slider '%s'.", sliderName);

	project->CloneSlider(editor.activeSlider, sliderName);
	createSliderGUI(sliderName, sliderScroll, sliderScroll->GetSizer());

	sliderScroll->FitInside();
	SetPendingChanges();
	HighlightSliderData();
}

void OutfitStudioFrame::OnSliderNegate(wxCommandEvent& WXUNUSED(event)) {
	if (!ShapeSelectionCheck())
		return;

	if (!editor.bEditSlider) {
		wxMessageBox(_("There is no slider in edit mode to negate!"), _("Error"));
		return;
	}

	std::vector<NiShape*> shapes;
	for (auto& i : selectedItems)
		shapes.push_back(i->GetShape());

	if (!ConfirmSliderDataLocalForEdit(shapes, std::vector<std::string>{editor.activeSlider}))
		return;

	wxLogMessage("Negating slider '%s' for the selected shapes.", editor.activeSlider);
	for (auto& i : selectedItems)
		project->NegateSlider(editor.activeSlider, i->GetShape());

	SetPendingChanges();
}

void OutfitStudioFrame::OnSliderRigidify(wxCommandEvent& WXUNUSED(event)) {
	if (!ShapeSelectionCheck())
		return;

	// The slider in edit mode, or else the checked sliders that have a morph
	std::vector<std::string> sliderNames;
	if (editor.bEditSlider)
		sliderNames.push_back(editor.activeSlider);
	else {
		for (auto& sliderPanel : sliderPanels) {
			if (sliderPanel.second->sliderCheck->Get3StateValue() != wxCheckBoxState::wxCHK_CHECKED)
				continue;
			size_t sliderIndex = 0;
			if (project->SliderIndexFromName(sliderPanel.first, sliderIndex) && !project->SliderZap(sliderIndex) && !project->SliderUV(sliderIndex))
				sliderNames.push_back(sliderPanel.first);
		}
	}

	if (sliderNames.empty()) {
		wxMessageBox(_("Check the sliders to rigidify, or enter edit mode for one. Zap and UV sliders have no morph to rigidify."), _("Error"));
		return;
	}

	std::vector<NiShape*> shapes;
	for (auto& i : selectedItems)
		if (!project->IsBaseShape(i->GetShape()))
			shapes.push_back(i->GetShape());

	if (shapes.empty()) {
		wxMessageBox(_("Select the outfit shapes to rigidify. The reference shape isn't changed."), _("Error"));
		return;
	}

	if (!ConfirmSliderDataLocalForEdit(shapes, sliderNames))
		return;

	wxArrayString choices;
	choices.Add(_("Each part moves and turns, keeping its size"));
	choices.Add(_("Each part moves, turns and scales evenly"));
	choices.Add(_("All unmasked vertices of a shape move as one part"));

	wxString message = sliderNames.size() == 1
		? wxString::Format(_("The unmasked parts of the selected shapes will follow the slider '%s' without bending, e.g. buckles, gems "
							 "and armor plates. Mask everything else first. Masked vertices keep their morph, partly masked ones blend."),
						   wxString::FromUTF8(sliderNames.front()))
		: wxString::Format(_("The unmasked parts of the selected shapes will follow the %zu checked sliders without bending, e.g. buckles, gems "
							 "and armor plates. Mask everything else first. Masked vertices keep their morph, partly masked ones blend."),
						   sliderNames.size());

	int choice = wxGetSingleChoiceIndex(message,
										_("Rigidify Parts"),
										choices,
										0,
										this);
	if (choice < 0)
		return;

	RigidifyOptions options;
	options.allowScale = choice == 1;
	options.perPart = choice != 2;

	wxBusyCursor busy;
	ProjectOperations ops(*project, editor);
	RefreshFlags refresh = RefreshFlags::None;
	OpResult result;
	for (auto& sliderName : sliderNames) {
		wxLogMessage("Rigidifying slider '%s' for the selected shapes.", sliderName);

		// An undo step per slider, an undo state holds the changes of one slider
		UndoStateProject* usp = glView->GetUndoHistory()->PushState();
		size_t parts = 0;
		result = ops.RigidifySlider(shapes, sliderName, options, *usp, &parts);
		if (!result.ok || usp->usss.empty())
			glView->GetUndoHistory()->PopState();

		refresh |= result.refresh;
		if (!result.ok)
			break;

		wxLogMessage("Rigidified %d parts.", static_cast<int>(parts));
	}

	ApplyRefresh(refresh);
	UpdateUndoTools();

	if (!result.ok)
		wxMessageBox(wxString::FromUTF8(result.error), _("Rigidify Parts"), wxOK | wxICON_WARNING, this);
}

void OutfitStudioFrame::OnMaskAffected(wxCommandEvent& WXUNUSED(event)) {
	if (!ShapeSelectionCheck())
		return;

	if (!editor.bEditSlider) {
		wxMessageBox(_("There is no slider in edit mode to create a mask from!"), _("Error"));
		return;
	}

	wxLogMessage("Creating mask for affected vertices of the slider '%s'.", editor.activeSlider);
	for (auto& i : selectedItems)
		project->MaskAffected(editor.activeSlider, i->GetShape());
}

void OutfitStudioFrame::OnDeleteSlider(wxCommandEvent& WXUNUSED(event)) {
	wxString prompt = _("Are you sure you wish to delete the selected slider(s)?");
	if (project->activeSet.HasBaseProject())
		prompt += "\n\n" + _("Sliders of the base project are only hidden in this project and stay in the base project.");
	int result = wxMessageBox(prompt, _("Confirm slider delete"), wxYES_NO | wxICON_WARNING, this);
	if (result != wxYES)
		return;

	DeleteSliders();
}

void OutfitStudioFrame::DeleteSliders(bool keepSliders, bool keepZaps) {
	if (keepSliders && keepZaps)
		return;

	auto deleteSlider = [&](const std::string& sliderName) {
		wxSliderPanel* sliderPanel = GetSliderPanel(sliderName);
		if (sliderPanel) {
			sliderPanel->slider->SetValue(0);
			sliderPanel->slider->SetFocus();
			HideSliderPanel(sliderPanel);
		}

		SetSliderValue(sliderName, 0);
		ShowSliderEffect(sliderName, true);

		sliderScroll->FitInside();
		project->DeleteSlider(sliderName);
	};

	if (!editor.bEditSlider) {
		for (auto it = sliderPanels.begin(); it != sliderPanels.end();) {
			if (it->second->sliderCheck->Get3StateValue() == wxCheckBoxState::wxCHK_CHECKED) {
				if ((keepZaps && project->activeSet[it->first].bZap) || (keepSliders && !project->activeSet[it->first].bZap)) {
					++it;
					continue;
				}
				deleteSlider(it->first);
				it = sliderPanels.erase(it);
			}
			else
				++it;
		}
	}
	else {
		wxLogMessage("Deleting slider '%s'.", editor.activeSlider);

		deleteSlider(editor.activeSlider);
		sliderPanels.erase(editor.activeSlider);

		MenuExitSliderEdit();
		editor.activeSlider.clear();
		editor.lastActiveSlider.clear();
		editor.bEditSlider = false;
	}

	SetPendingChanges();
	ApplySliders();
}

void OutfitStudioFrame::ShowSliderProperties(const std::string& sliderName) {
	size_t curSlider = 0;
	if (!project->SliderIndexFromName(sliderName, curSlider))
		return;

	CloseBrushSettings();

	wxDialog dlg;
	if (wxXmlResource::Get()->LoadDialog(&dlg, this, "dlgSliderProp")) {
		dlg.SetSize(dlg.FromDIP(wxSize(760, 560)));
		dlg.SetMinSize(dlg.FromDIP(wxSize(500, 480)));
		dlg.CenterOnParent();

		wxTextCtrl* edSliderName = XRCCTRL(dlg, "edSliderName", wxTextCtrl);
		wxStaticText* lbValLo = XRCCTRL(dlg, "lbValLo", wxStaticText);
		wxStaticText* lbValHi = XRCCTRL(dlg, "lbValHi", wxStaticText);
		wxTextCtrl* edValLo = XRCCTRL(dlg, "edValLo", wxTextCtrl);
		wxTextCtrl* edValHi = XRCCTRL(dlg, "edValHi", wxTextCtrl);
		wxCheckBox* cbValZapped = XRCCTRL(dlg, "cbValZapped", wxCheckBox);
		wxCheckBox* chkHidden = XRCCTRL(dlg, "chkHidden", wxCheckBox);
		wxCheckBox* chkInvert = XRCCTRL(dlg, "chkInvert", wxCheckBox);
		wxCheckBox* chkZap = XRCCTRL(dlg, "chkZap", wxCheckBox);
		wxCheckBox* chkUV = XRCCTRL(dlg, "chkUV", wxCheckBox);
		wxCheckBox* chkToggle = XRCCTRL(dlg, "chkToggle", wxCheckBox);
		wxCheckBox* chkNoAutoConform = XRCCTRL(dlg, "chkNoAutoConform", wxCheckBox);
		wxStaticText* lbToggleValue = XRCCTRL(dlg, "lbToggleValue", wxStaticText);
		wxTextCtrl* edToggleValue = XRCCTRL(dlg, "edToggleValue", wxTextCtrl);
		wxStaticText* zapToggleLabel = XRCCTRL(dlg, "zapToggleLabel", wxStaticText);
		wxCheckListBox* zapToggleList = XRCCTRL(dlg, "zapToggleList", wxCheckListBox);
		wxStaticText* toggleSliderLabel = XRCCTRL(dlg, "toggleSliderLabel", wxStaticText);
		wxGrid* toggleSliderGrid = XRCCTRL(dlg, "toggleSliderGrid", wxGrid);
		wxListCtrl* sliderDataList = XRCCTRL(dlg, "sliderDataList", wxListCtrl);
		wxButton* btnSliderDataLocal = XRCCTRL(dlg, "btnSliderDataLocal", wxButton);
		wxButton* btnSliderDataExternal = XRCCTRL(dlg, "btnSliderDataExternal", wxButton);
		wxButton* btnSliderDataFolders = XRCCTRL(dlg, "btnSliderDataFolders", wxButton);
		wxButton* btnSliderDataRevert = XRCCTRL(dlg, "btnSliderDataRevert", wxButton);
		btnSliderDataRevert->Show(project->activeSet.HasBaseProject());

		std::vector<SliderDataLocation> sliderDataRows;
		auto updateSliderDataButtons = [&]() {
			std::vector<size_t> selectedRows;
			size_t selectionCount = SliderDataList::GetSelectedRows(sliderDataList, sliderDataRows, selectedRows);
			bool allLocal = SliderDataList::AllHaveSource(sliderDataRows, selectedRows, true);
			bool allExternal = SliderDataList::AllHaveSource(sliderDataRows, selectedRows, false);
			bool anyInherited = SliderDataList::AnyInherited(sliderDataRows, selectedRows);
			btnSliderDataLocal->Enable(selectionCount > 0 && allExternal);
			btnSliderDataExternal->Enable(selectionCount > 0 && allLocal);
			btnSliderDataFolders->Enable(selectionCount > 0 && allExternal && !anyInherited);
			btnSliderDataRevert->Enable(SliderDataList::AllOverridden(sliderDataRows, selectedRows));
		};

		auto refreshSliderData = [&]() {
			project->GetSliderDataLocations(sliderDataRows, sliderName);
			SliderDataList::Populate(sliderDataList, sliderDataRows, nullptr, false);
			updateSliderDataButtons();
		};

		SliderDataList::Configure(sliderDataList, false);

		long loVal = (int)(project->SliderDefault(curSlider, false));
		long hiVal = (int)(project->SliderDefault(curSlider, true));

		edSliderName->SetValue(wxString::FromUTF8(editor.activeSlider));
		edValLo->SetValue(wxString::Format("%d", loVal));
		edValHi->SetValue(wxString::Format("%d", hiVal));

		if (project->SliderHidden(curSlider))
			chkHidden->SetValue(true);

		if (project->SliderInvert(curSlider))
			chkInvert->SetValue(true);

		if (project->SliderUV(curSlider))
			chkUV->SetValue(true);

		if (!project->mGenWeights)
			lbValHi->SetLabel(_("Default"));

		chkZap->SetValue(project->SliderZap(curSlider));
		chkToggle->SetValue(project->SliderToggle(curSlider));

		// BodySlide only conforms sliders of add-ons to outfits
		chkNoAutoConform->SetValue(project->SliderNoAutoConform(curSlider));
		chkNoAutoConform->Show(project->activeSet.IsAddOn());

		edToggleValue->SetValue(wxString::Format("%g", project->SliderToggleValue(curSlider)));

		for (size_t i = 0; i < project->SliderCount(); i++)
			if (i != curSlider && (project->SliderZap(i) || project->SliderHidden(i)))
				zapToggleList->Append(wxString::FromUTF8(project->GetSliderName(i)));

		for (auto& s : project->SliderZapToggles(curSlider)) {
			int stringId = zapToggleList->FindString(s, true);
			if (stringId != wxNOT_FOUND)
				zapToggleList->Check(stringId);
		}

		// Sliders and zaps the toggle can set, with whether it sets them and the value
		enum { ToggleColSet, ToggleColSlider, ToggleColValue };
		toggleSliderGrid->CreateGrid(0, 3);
		toggleSliderGrid->SetRowLabelSize(0);
		toggleSliderGrid->SetColLabelValue(ToggleColSet, _("Set"));
		toggleSliderGrid->SetColLabelValue(ToggleColSlider, _("Slider"));
		toggleSliderGrid->SetColLabelValue(ToggleColValue, _("Value"));
		toggleSliderGrid->EnableDragRowSize(false);
		toggleSliderGrid->EnableDragGridSize(false);

		auto* setAttr = new wxGridCellAttr();
		setAttr->SetRenderer(new wxGridCellBoolRenderer());
		setAttr->SetAlignment(wxALIGN_CENTRE, wxALIGN_CENTRE);
		setAttr->SetReadOnly();
		toggleSliderGrid->SetColAttr(ToggleColSet, setAttr);

		auto* nameAttr = new wxGridCellAttr();
		nameAttr->SetReadOnly();
		toggleSliderGrid->SetColAttr(ToggleColSlider, nameAttr);

		auto* valueAttr = new wxGridCellAttr();
		valueAttr->SetAlignment(wxALIGN_RIGHT, wxALIGN_CENTRE);
		toggleSliderGrid->SetColAttr(ToggleColValue, valueAttr);

		std::vector<ToggleTarget> toggleSliders = project->SliderToggleSliders(curSlider);
		for (size_t i = 0; i < project->SliderCount(); i++) {
			if (i == curSlider || project->SliderToggle(i) || project->SliderClamp(i))
				continue;

			std::string name = project->GetSliderName(i);
			auto target = std::find_if(toggleSliders.begin(), toggleSliders.end(), [&name](const ToggleTarget& t) { return t.slider == name; });

			int row = toggleSliderGrid->GetNumberRows();
			toggleSliderGrid->AppendRows(1);
			toggleSliderGrid->SetCellValue(row, ToggleColSet, target != toggleSliders.end() ? "1" : "");
			toggleSliderGrid->SetCellValue(row, ToggleColSlider, wxString::FromUTF8(name));
			toggleSliderGrid->SetCellValue(row, ToggleColValue, wxString::Format("%g", target != toggleSliders.end() ? target->value : 100.0f));
		}

		toggleSliderGrid->SetColSize(ToggleColSet, dlg.FromDIP(40));
		toggleSliderGrid->SetColSize(ToggleColSlider, dlg.FromDIP(300));
		toggleSliderGrid->SetColSize(ToggleColValue, dlg.FromDIP(80));

		// One click ticks a slider, and changing its value ticks it too
		toggleSliderGrid->Bind(wxEVT_GRID_CELL_LEFT_CLICK, [&](wxGridEvent& event) {
			if (event.GetCol() != ToggleColSet) {
				event.Skip();
				return;
			}

			int row = event.GetRow();
			bool set = toggleSliderGrid->GetCellValue(row, ToggleColSet) == "1";
			toggleSliderGrid->SetCellValue(row, ToggleColSet, set ? "" : "1");
			toggleSliderGrid->SetGridCursor(row, ToggleColSet);
		});
		toggleSliderGrid->Bind(wxEVT_GRID_CELL_CHANGED, [&](wxGridEvent& event) {
			if (event.GetCol() == ToggleColValue)
				toggleSliderGrid->SetCellValue(event.GetRow(), ToggleColSet, "1");
			event.Skip();
		});

		auto updateSliderMode = [&]() {
			bool zap = chkZap->IsChecked();
			bool toggle = chkToggle->IsChecked();
			bool check = zap || toggle;

			lbValLo->Show(!check && project->mGenWeights);
			edValLo->Show(!check && project->mGenWeights);
			lbValHi->Show(!check);
			edValHi->Show(!check);
			cbValZapped->SetLabel(toggle ? _("Ticked") : _("Zapped"));
			cbValZapped->Show(check);
			lbToggleValue->Show(toggle);
			edToggleValue->Show(toggle);

			zapToggleLabel->Show(!toggle);
			zapToggleList->Show(!toggle);
			zapToggleList->Enable(zap);
			toggleSliderLabel->Show(toggle);
			toggleSliderGrid->Show(toggle);

			dlg.Layout();
		};
		updateSliderMode();

		if (hiVal > 0 || loVal > 0)
			cbValZapped->Set3StateValue(wxCheckBoxState::wxCHK_CHECKED);
		else
			cbValZapped->Set3StateValue(wxCheckBoxState::wxCHK_UNCHECKED);

		refreshSliderData();
		sliderDataList->Bind(wxEVT_LIST_ITEM_SELECTED, [&](wxListEvent&) { updateSliderDataButtons(); });
		sliderDataList->Bind(wxEVT_LIST_ITEM_DESELECTED, [&](wxListEvent&) { updateSliderDataButtons(); });
		SliderDataList::BindSelectAll(&dlg, sliderDataList, updateSliderDataButtons);
		btnSliderDataLocal->Bind(wxEVT_BUTTON, [&](wxCommandEvent&) {
			std::vector<size_t> selectedRows;
			SliderDataList::GetSelectedRows(sliderDataList, sliderDataRows, selectedRows);
			if (SliderDataList::MakeLocal(&dlg, project, sliderDataRows, selectedRows)) {
				SetPendingChanges();
				refreshSliderData();
			}
		});
		btnSliderDataExternal->Bind(wxEVT_BUTTON, [&](wxCommandEvent&) {
			std::vector<size_t> selectedRows;
			SliderDataList::GetSelectedRows(sliderDataList, sliderDataRows, selectedRows);
			if (SliderDataList::EditFolders(&dlg, project, sliderDataRows, selectedRows)) {
				SetPendingChanges();
				refreshSliderData();
			}
		});
		btnSliderDataFolders->Bind(wxEVT_BUTTON, [&](wxCommandEvent&) {
			std::vector<size_t> selectedRows;
			SliderDataList::GetSelectedRows(sliderDataList, sliderDataRows, selectedRows);
			if (SliderDataList::EditFolders(&dlg, project, sliderDataRows, selectedRows)) {
				SetPendingChanges();
				refreshSliderData();
			}
		});
		btnSliderDataRevert->Bind(wxEVT_BUTTON, [&](wxCommandEvent&) {
			std::vector<size_t> selectedRows;
			SliderDataList::GetSelectedRows(sliderDataList, sliderDataRows, selectedRows);
			if (SliderDataList::RevertToBase(&dlg, project, sliderDataRows, selectedRows)) {
				SetPendingChanges();
				ApplySliders();
				refreshSliderData();
			}
		});

		// A slider is either a zap or a toggle
		chkZap->Bind(wxEVT_CHECKBOX, [&](wxCommandEvent& event) {
			if (event.IsChecked())
				chkToggle->SetValue(false);
			updateSliderMode();
		});
		chkToggle->Bind(wxEVT_CHECKBOX, [&](wxCommandEvent& event) {
			if (event.IsChecked())
				chkZap->SetValue(false);
			updateSliderMode();
		});

		XRCCTRL(dlg, "wxID_CANCEL", wxButton)->SetFocus();

		if (dlg.ShowModal() == wxID_OK) {
			toggleSliderGrid->SaveEditControlValue();

			// Zaps and toggles are on or off by default
			if (chkZap->IsChecked() || chkToggle->IsChecked()) {
				if (cbValZapped->IsChecked()) {
					loVal = 100;
					hiVal = 100;
				}
				else {
					loVal = 0;
					hiVal = 0;
				}
			}
			else {
				edValLo->GetValue().ToLong(&loVal);
				edValHi->GetValue().ToLong(&hiVal);
			}

			if (chkZap->IsChecked()) {
				wxArrayString zapToggles;
				wxArrayInt toggled;
				zapToggleList->GetCheckedItems(toggled);
				for (auto& i : toggled)
					zapToggles.Add(zapToggleList->GetString(i));

				project->SetSliderZapToggles(curSlider, zapToggles);
			}

			project->SetSliderToggle(curSlider, chkToggle->IsChecked());
			if (chkToggle->IsChecked()) {
				double toggleValue = 100.0;
				if (edToggleValue->GetValue().ToDouble(&toggleValue))
					project->SetSliderToggleValue(curSlider, static_cast<float>(toggleValue));

				std::vector<ToggleTarget> targets;
				for (int row = 0; row < toggleSliderGrid->GetNumberRows(); row++) {
					if (toggleSliderGrid->GetCellValue(row, ToggleColSet) != "1")
						continue;

					ToggleTarget target;
					target.slider = toggleSliderGrid->GetCellValue(row, ToggleColSlider).ToUTF8().data();

					double value = 100.0;
					if (toggleSliderGrid->GetCellValue(row, ToggleColValue).ToDouble(&value))
						target.value = static_cast<float>(value);

					targets.push_back(std::move(target));
				}

				project->SetSliderToggleSliders(curSlider, targets);
			}

			project->SetSliderZap(curSlider, chkZap->GetValue());
			project->SetSliderInvert(curSlider, chkInvert->GetValue());
			project->SetSliderHidden(curSlider, chkHidden->GetValue());
			project->SetSliderUV(curSlider, chkUV->GetValue());
			if (project->activeSet.IsAddOn())
				project->SetSliderNoAutoConform(curSlider, chkNoAutoConform->GetValue());
			project->SetSliderDefault(curSlider, loVal, false);
			project->SetSliderDefault(curSlider, hiVal, true);

			std::string newSliderName{edSliderName->GetValue().ToUTF8()};
			if (sliderName != newSliderName && !project->ValidSlider(newSliderName) && ConfirmRenameFromBase(sliderName, "")) {
				project->SetSliderName(curSlider, newSliderName);
				wxSliderPanel* d = GetSliderPanel(sliderName);
				sliderPanels.erase(sliderName);

				if (d) {
					sliderPanels[newSliderName] = d;

					wxString sn = wxString::FromUTF8(newSliderName);
					d->slider->SetName(sn + "|slider");
					d->sliderName->SetName(sn + "|lbl");
					d->btnSliderEdit->SetName(sn + "|btn");
					d->btnSliderProp->SetName(sn + "|btnSliderProp");
					d->btnMinus->SetName(sn + "|btnMinus");
					d->btnPlus->SetName(sn + "|btnPlus");
					d->sliderCheck->SetName(sn + "|check");
					d->sliderReadout->SetName(sn + "|readout");
					d->sliderName->SetLabel(sn);
				}

				if (sliderName == editor.activeSlider)
					editor.activeSlider = std::move(newSliderName);
			}

			SetPendingChanges();
		}
	}
}

void OutfitStudioFrame::OnSliderProperties(wxCommandEvent& WXUNUSED(event)) {
	if (!editor.bEditSlider) {
		wxMessageBox(_("There is no slider in edit mode to show properties for!"), _("Error"));
		return;
	}

	ShowSliderProperties(editor.activeSlider);
}

void OutfitStudioFrame::OnSliderDataLocations(wxCommandEvent& WXUNUSED(event)) {
	SliderDataLocationsDialog dlg(this, project);
	dlg.ShowModal();
}
