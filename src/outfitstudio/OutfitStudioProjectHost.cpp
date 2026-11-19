/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "OutfitStudioProjectHost.h"
#include "OutfitStudioFrame.h"
#include "FrameHelpers.h"
#include "GLPanel.h"

#include <cmath>
#include <cstdlib>
#include <iterator>
#include <sstream>

using namespace nifly;

void OutfitStudioProjectHost::StartProgress(const wxString& msg) {
	frame->StartProgress(msg);
}

void OutfitStudioProjectHost::StartSubProgress(int min, int max) {
	frame->StartSubProgress(min, max);
}

void OutfitStudioProjectHost::UpdateProgress(int val, const wxString& msg) {
	frame->UpdateProgress(val, msg);
}

void OutfitStudioProjectHost::EndProgress(const wxString& msg, bool forceEmpty) {
	frame->EndProgress(msg, forceEmpty);
}

void OutfitStudioProjectHost::ShowMessage(HostMessageLevel level, const wxString& msg, const wxString& title) {
	if (!interactive) {
		NonInteractiveHost::ShowMessage(level, msg, title);
		return;
	}

	long icon = wxICON_INFORMATION;
	if (level == HostMessageLevel::Error)
		icon = wxICON_ERROR;
	else if (level == HostMessageLevel::Warning)
		icon = wxICON_WARNING;

	wxMessageBox(msg, title, wxOK | icon, frame);
}

HostAnswer OutfitStudioProjectHost::Ask(const wxString& msg, const wxString& title, bool withCancel, HostAnswer defaultAnswer) {
	if (!interactive)
		return NonInteractiveHost::Ask(msg, title, withCancel, defaultAnswer);

	long style = wxYES_NO | wxICON_QUESTION;
	if (withCancel)
		style |= wxCANCEL;

	int res = wxMessageBox(msg, title, style, frame);
	if (res == wxYES)
		return HostAnswer::Yes;
	if (res == wxCANCEL)
		return HostAnswer::Cancel;
	return HostAnswer::No;
}

bool OutfitStudioProjectHost::ChooseMany(const wxString& msg, const wxString& title, const wxArrayString& choices, wxArrayInt& selections) {
	if (!interactive)
		return NonInteractiveHost::ChooseMany(msg, title, choices, selections);

	wxMultiChoiceDialog choiceDialog(frame, msg, title, choices);
	choiceDialog.SetSelections(selections);
	if (choiceDialog.ShowModal() == wxID_CANCEL)
		return false;

	selections = choiceDialog.GetSelections();
	return true;
}

bool OutfitStudioProjectHost::ChooseOne(const wxString& msg, const wxString& title, const wxArrayString& choices, int& selection) {
	if (!interactive)
		return NonInteractiveHost::ChooseOne(msg, title, choices, selection);

	wxSingleChoiceDialog choiceDialog(frame, msg, title, choices);
	if (selection >= 0 && selection < static_cast<int>(choices.GetCount()))
		choiceDialog.SetSelection(selection);
	if (choiceDialog.ShowModal() == wxID_CANCEL)
		return false;

	selection = choiceDialog.GetSelection();
	return true;
}

bool OutfitStudioProjectHost::GetText(const wxString& msg, const wxString& title, wxString& value) {
	if (!interactive)
		return NonInteractiveHost::GetText(msg, title, value);

	value = GetTextFromUser(msg, title, value, frame);
	return !value.empty();
}

Mesh* OutfitStudioProjectHost::GetShapeMesh(const std::string& shapeName) {
	if (!frame->glView)
		return nullptr;

	return frame->glView->GetMesh(shapeName);
}

void OutfitStudioProjectHost::UpdateShapeMesh(const std::string& shapeName) {
	auto shape = frame->project->GetWorkNif()->FindBlockByName<NiShape>(shapeName);
	if (!shape || !frame->glView)
		return;

	std::vector<Vector3> verts;
	frame->project->GetLiveVerts(shape, verts);
	frame->glView->UpdateMeshVertices(shapeName, &verts, true, true, false);
}

void OutfitStudioProjectHost::OnAnimationChanged() {
	frame->UpdateAnimationGUI();
}

void OutfitStudioProjectHost::OnPartitionsChanged() {
	frame->ShowPartition();
}

void OutfitStudioProjectHost::OnShapeDeleting(NiShape* shape) {
	frame->glView->DeleteMesh(shape->name.get());
	frame->ClearSelected(shape);
}

void OutfitStudioProjectHost::DeleteSlidersForMerge(bool keepSliders, bool keepZaps) {
	frame->DeleteSliders(keepSliders, keepZaps);
}
