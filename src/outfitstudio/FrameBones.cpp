/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

// Outfit Studio window: the bone list, bones, skinning and bone weights

#include "OutfitStudioFrame.h"
#include "PosePanel.h"
#include "AnimationPlayer.h"
#include "PhysicsPreview.h"
#include "ShapeTreeItems.h"
#include "GLPanel.h"
#include "OutfitStudioApp.h"
#include "utils/ConfigurationManager.h"
#include "ui/wxStateButton.h"
#include "ui/outfitstudio/WeightCopyDialog.h"
#include "components/TweakBrush.h"
#include "components/UndoHistory.h"
#include "utils/TargetGame.h"
#include "mcp/OutfitStudioMcp.h"
#include "components/SliderPresets.h"
#include "files/HkxFile.h"
#include "files/SFMorphFile.h"
#include "files/GameDataStream.h"
#include "ui/outfitstudio/BadBonesDialog.h"
#include "ui/outfitstudio/AutomationDialog.h"
#include "utils/ProjectUtil.h"
#include "utils/GameUtil.h"
#include "utils/StringStuff.h"

#include <cmath>
#include <cstdlib>
#include <iterator>
#include <sstream>
#include <wx/textctrl.h>

using namespace nifly;

void OutfitStudioFrame::UpdateBoneCounts() {
	auto totalBoneCountLabel = reinterpret_cast<wxStaticText*>(FindWindowByName("totalBoneCountLabel"));
	totalBoneCountLabel->SetLabel(wxString::Format(_("Total Bones: %zu"), project->GetActiveBoneCount()));

	std::vector<std::string> boneNames;
	project->GetActiveBones(boneNames);

	size_t selectedBoneCount = 0;
	for (auto& s : selectedItems) {
		for (auto& bone : boneNames) {
			if (project->GetWorkAnim()->HasWeights(s->GetShape()->name.get(), bone)) {
				selectedBoneCount++;
			}
		}
	}

	auto selectedBoneCountLabel = reinterpret_cast<wxStaticText*>(FindWindowByName("selectedBoneCountLabel"));
	selectedBoneCountLabel->SetLabel(wxString::Format(_("Shape Selection Bones: %zu"), selectedBoneCount));
}

void OutfitStudioFrame::HighlightBoneNamesWithWeights() {
	wxTreeItemIdValue cookie;
	wxTreeItemId item = outfitBones->GetFirstChild(bonesRoot, cookie);
	while (item.IsOk()) {
		outfitBones->SetItemTextColour(item, wxColour(255, 255, 255));

		auto boneName = outfitBones->GetItemText(item).ToStdString();
		for (auto& i : selectedItems) {
			if (project->GetWorkAnim()->HasWeights(i->GetShape()->name.get(), boneName)) {
				outfitBones->SetItemTextColour(item, wxColour(0, 255, 0));
			}
		}

		UpdateBoneItemState(item, boneName);

		item = outfitBones->GetNextChild(bonesRoot, cookie);
	}
}

void OutfitStudioFrame::GetNormalizeBones(std::vector<std::string>* normBones, std::vector<std::string>* notNormBones) {
	ProjectOperations(*project, editor).GetNormalizeBones(normBones, notNormBones);
}

std::vector<std::string> OutfitStudioFrame::GetSelectedBones() {
	std::vector<std::string> activeBones;
	project->GetActiveBones(activeBones);

	std::vector<std::string> boneList;

	for (auto& selBone : editor.lastSelectedBones) {
		if (std::find(activeBones.begin(), activeBones.end(), selBone) != activeBones.end()) {
			boneList.push_back(selBone);
		}
	}

	return boneList;
}

void OutfitStudioFrame::CalcAutoXMirrorBone() {
	// Note that there is very similar code to this in OutfitProject::MatchSymmetricBoneNames.

	editor.autoXMirrorBone.clear();

	const size_t abLen = editor.activeBone.length();
	std::vector<std::string> bones;
	project->GetActiveBones(bones);

	int bestFlips = 0;
	for (const std::string& b : bones) {
		if (abLen != b.length())
			continue;

		int flips = 0;
		bool nomatch = false;
		for (size_t i = 0; i < abLen && !nomatch; ++i) {
			char abc = ToLower(editor.activeBone[i]);
			char bc = ToLower(b[i]);
			if (abc == 'l') {
				if (bc == 'r')
					++flips;
				else if (bc != abc)
					nomatch = true;
			}
			else if (abc == 'r') {
				if (bc == 'l')
					++flips;
				else if (bc != abc)
					nomatch = true;
			}
			else
				nomatch = bc != abc;
		}

		if (nomatch)
			continue;
		if (flips <= bestFlips)
			continue;

		bestFlips = flips;
		editor.autoXMirrorBone = b;
	}

	if (editor.autoXMirrorBone.empty())
		cXMirrorBone->SetString(1, "Auto: None");
	else
		cXMirrorBone->SetString(1, "Auto: " + editor.autoXMirrorBone);
}

std::string OutfitStudioFrame::GetXMirrorBone() {
	int xMChoice = cXMirrorBone->GetSelection();
	if (xMChoice == 0)
		return std::string();
	else if (xMChoice == 1)
		return editor.autoXMirrorBone;
	else
		return cXMirrorBone->GetString(xMChoice).ToStdString();
}

std::string OutfitStudioFrame::GetActiveBone() {
	return editor.activeBone;
}

void OutfitStudioFrame::UpdateAnimationGUI() {
	// Preserve x-mirror and pose bones
	int xMChoice = cXMirrorBone->GetSelection();
	std::string manualXMirrorBone;
	if (xMChoice >= 2)
		manualXMirrorBone = cXMirrorBone->GetString(xMChoice);

	std::string poseBone;
	int poseBoneSel = posePanel->GetBoneChoice()->GetSelection();
	if (poseBoneSel != wxNOT_FOUND)
		poseBone = posePanel->GetBoneChoice()->GetString(poseBoneSel).ToStdString();

	if (editor.lastSelectedBones.count(editor.activeBone) == 0)
		editor.activeBone.clear();

	UpdateBoneTree();

	cXMirrorBone->Freeze();
	cXMirrorBone->Clear();
	cXMirrorBone->AppendString("None");
	cXMirrorBone->AppendString("Auto");
	cXMirrorBone->SetSelection(xMChoice == 1 ? 1 : 0);

	posePanel->GetBoneChoice()->Freeze();
	posePanel->GetBoneChoice()->Clear();

	std::vector<std::string> activeBones;
	project->GetActiveBones(activeBones);

	wxArrayString activeBonesArr;
	for (auto& bone : activeBones)
		activeBonesArr.Add(wxString::FromUTF8(bone));

	cXMirrorBone->Append(activeBonesArr);
	posePanel->GetBoneChoice()->Append(activeBonesArr);

	// Re-fill mirror and pose bone lists
	for (auto& bone : activeBones) {
		if (xMChoice >= 2 && bone == manualXMirrorBone)
			cXMirrorBone->SetSelection(cXMirrorBone->GetCount() - 1);

		if (poseBone == bone)
			posePanel->GetBoneChoice()->SetSelection(posePanel->GetBoneChoice()->GetCount() - 1);
	}

	CalcAutoXMirrorBone();

	if (posePanel->GetBoneChoice()->GetSelection() == wxNOT_FOUND && posePanel->GetBoneChoice()->GetCount() > 0)
		posePanel->GetBoneChoice()->SetSelection(0);

	cXMirrorBone->Thaw();
	posePanel->GetBoneChoice()->Thaw();

	auto cPoseName = (wxComboBox*)FindWindowByName("cPoseName");
	cPoseName->Clear();

	std::string poseDataPath = ProjectUtil::GetProjectPath() + "/PoseData";
	poseDataCollection.LoadData(poseDataPath);

	// Additionally load community-supplied ScreenArcherMenu/SAF poses if the
	// target game ships them in a known location:
	//   Skyrim SE/VR: Data\SAM\Poses\*.yaml
	//   Fallout 4/VR: Data\F4SE\Plugins\SAF\Poses\*.json
	const TargetGame samGame = wxGetApp().targetGame;
	switch (samGame) {
	case SKYRIMSE:
	case SKYRIMVR:
	case FO4:
	case FO4VR:
		poseDataCollection.LoadGamePoses(GameUtil::GetGameDataPath(samGame).ToUTF8().data(), samGame == FO4 || samGame == FO4VR);
		break;
	default:
		break;
	}

	for (auto& poseData : poseDataCollection.poseData) {
		wxString poseName = wxString::FromUTF8(poseData.name);
		cPoseName->Append(poseName, &poseData);
	}

	// Dummy sentinel entry representing a clean/empty pose.
	cPoseName->Append("<New>", (void*)nullptr);
	cPoseName->SetStringSelection("<New>");

	// Reflect the read-only state of the initially selected pose (if any).
	posePanel->UpdatePoseButtonStates();

	RefreshGUIWeightColors();
	posePanel->PoseToGUI();
	physicsPreview->UpdateControlsVisibility();

	glView->UpdateNodes();
	glView->UpdateBones();
}

void OutfitStudioFrame::UpdateBoneItemState(const wxTreeItemId& item, const std::string& boneName) {
	bool badBone = false;
	if (activeItem) {
		auto* shape = activeItem->GetShape();
		if (shape && project->GetWorkNif()->IsValid() && project->GetWorkNif()->GetBlockID(shape) != nifly::NIF_NPOS)
			badBone = project->GetWorkAnim()->BoneHasInconsistentTransforms(shape->name.get(), boneName);
	}
	outfitBones->SetItemState(item, (editor.lastNormalizeBones.count(boneName) != 0 ? 1 : 0) + (badBone ? 2 : 0));
}

void OutfitStudioFrame::UpdateBoneTree() {
	bool saveRUI = recursingUI;
	recursingUI = true;

	outfitBones->Freeze();

	// Clear bone tree
	if (outfitBones->GetChildrenCount(bonesRoot) > 0)
		outfitBones->DeleteChildren(bonesRoot);

	wxString filterStr = bonesFilter->GetValue();
	filterStr.MakeLower();

	// Refill bone tree, re-setting normalize state and re-selecting bones
	std::vector<std::string> activeBones;
	project->GetActiveBones(activeBones);

	for (auto& bone : activeBones) {
		// Filter out bone by name
		wxString boneStr = wxString::FromUTF8(bone);
		if (!boneStr.Lower().Contains(filterStr))
			continue;

		wxTreeItemId item = outfitBones->AppendItem(bonesRoot, boneStr);
		UpdateBoneItemState(item, bone);

		if (editor.lastSelectedBones.count(bone) != 0) {
			outfitBones->SelectItem(item);
			if (editor.activeBone.empty())
				editor.activeBone = bone;
		}
	}

	outfitBones->Thaw();

	recursingUI = saveRUI;

	HighlightBoneNamesWithWeights();
	UpdateBoneCounts();
}

void OutfitStudioFrame::OnBonesFilterChanged(wxCommandEvent& WXUNUSED(event)) {
	UpdateBoneTree();
}

void OutfitStudioFrame::OnFixedWeight(wxCommandEvent& event) {
	bool checked = event.IsChecked();
	TB_Weight* weightBrush = dynamic_cast<TB_Weight*>(glView->GetActiveBrush());
	if (weightBrush)
		weightBrush->bFixedWeight = checked;
}

void OutfitStudioFrame::OnCBNormalizeWeights(wxCommandEvent& event) {
	bool checked = event.IsChecked();
	TB_Weight* weightBrush = dynamic_cast<TB_Weight*>(glView->GetActiveBrush());
	if (weightBrush)
		weightBrush->bNormalizeWeights = checked;
}

void OutfitStudioFrame::OnCBAccumulateSmooth(wxCommandEvent& event) {
	glView->SetToolOptionAccumulateSmoothWeight(event.IsChecked());
}

void OutfitStudioFrame::OnNormalizeWeights(wxCommandEvent& WXUNUSED(event)) {
	if (!ShapeSelectionCheck())
		return;

	std::vector<NiShape*> shapes;
	for (auto& sel : selectedItems)
		shapes.push_back(sel->GetShape());

	size_t changedVertices = 0;
	UndoStateProject* usp = glView->GetUndoHistory()->PushState();
	OpResult result = ProjectOperations(*project, editor).NormalizeWeights(shapes, *usp, &changedVertices);
	if (!result.ok || usp->usss.empty())
		glView->GetUndoHistory()->PopState();

	ApplyRefresh(result.refresh);
	RefreshGUIWeightColors();
	UpdateUndoTools();

	if (!result.ok)
		wxMessageBox(wxString::FromUTF8(result.error), _("Normalize Weights"), wxOK | wxICON_WARNING, this);
	else
		statusBar->SetStatusText(wxString::Format(_("Normalized the weights of %zu vertices."), changedVertices));
}

void OutfitStudioFrame::ToggleBoneState(wxTreeItemId item) {
	if (!item.IsOk())
		return;

	std::string boneName = outfitBones->GetItemText(item).ToStdString();
	if ((outfitBones->GetItemState(item) & 1) != 0)
		editor.lastNormalizeBones.erase(boneName);
	else
		editor.lastNormalizeBones.insert(boneName);
	UpdateBoneItemState(item, boneName);
}

void OutfitStudioFrame::OnBoneStateToggle(wxTreeEvent& event) {
	ToggleBoneState(event.GetItem());
	event.Skip();
}

void OutfitStudioFrame::RefreshGUIWeightColors() {
	std::vector<NiShape*> shapes;
	for (auto& s : selectedItems)
		shapes.push_back(s->GetShape());

	ProjectOperations(*project, editor).FillWeightColors(editor.activeBone, shapes);
	glView->Refresh();
}

void OutfitStudioFrame::OnBoneSelect(wxTreeEvent& event) {
	if (recursingUI)
		return;

	wxTreeItemId item = event.GetItem();
	if (!activeItem || !item.IsOk())
		return;

	wxArrayTreeItemIds selected;
	outfitBones->GetSelections(selected);

	editor.activeBone.clear();
	std::string selBone = outfitBones->GetItemText(item).ToStdString();

	if (!outfitBones->IsSelected(item)) {
		if (!selected.IsEmpty()) {
			std::string frontBone = outfitBones->GetItemText(selected.front()).ToStdString();
			editor.activeBone = frontBone;
		}
	}
	else {
		editor.activeBone = selBone;
		if (selected.GetCount() == 1)
			editor.lastSelectedBones.clear();
	}

	wxTreeItemIdValue cookie;
	wxTreeItemId itemTree = outfitBones->GetFirstChild(bonesRoot, cookie);
	while (itemTree.IsOk()) {
		std::string boneName = outfitBones->GetItemText(itemTree).ToStdString();
		if (outfitBones->IsSelected(itemTree))
			editor.lastSelectedBones.insert(boneName);
		else
			editor.lastSelectedBones.erase(boneName);

		itemTree = outfitBones->GetNextChild(bonesRoot, cookie);
	}

	// Sync with pose panel choice if the active bone exists there
	if (!editor.activeBone.empty() && posePanel->GetBoneChoice()) {
		wxString activeBoneStr = wxString::FromUTF8(editor.activeBone);
		for (unsigned int i = 0; i < posePanel->GetBoneChoice()->GetCount(); ++i) {
			if (posePanel->GetBoneChoice()->GetString(i) == activeBoneStr) {
				posePanel->GetBoneChoice()->SetSelection(i);
				posePanel->PoseToGUI();  // Update sliders and text values for the selected bone
				break;
			}
		}
	}

	glView->UpdateNodeColors();
	RefreshGUIWeightColors();
	CalcAutoXMirrorBone();

	if (glView->GetTransformMode())
		glView->ShowTransformTool();
}

void OutfitStudioFrame::OnBoneActivated(wxTreeEvent& event) {
	int hitFlags;
	outfitBones->HitTest(event.GetPoint(), hitFlags);

	if (hitFlags & wxTREE_HITTEST_ONITEMSTATEICON)
		return;

	wxCommandEvent evt;
	OnEditBone(evt);
}

void OutfitStudioFrame::OnBoneContext(wxTreeEvent& WXUNUSED(event)) {
	ShowBoneContextMenu();
}

void OutfitStudioFrame::ShowBoneContextMenu(const std::string& bone, wxWindow* at, const wxPoint& pos) {
	// A bone drawn only to connect the hierarchy can't be selected, but bones
	// can still be added as its children.
	const bool inProject = bone.empty() || FindBoneItem(bone).IsOk();

	wxMenu* menu = wxXmlResource::Get()->LoadMenu(inProject ? "menuBoneContext" : "menuBoneTreeContext");
	if (!menu)
		return;

	if (inProject) {
		if (!editor.activeBone.empty() && activeItem && project->GetWorkAnim()->BoneHasInconsistentTransforms(activeItem->GetShape()->name.get(), editor.activeBone)) {
			if (skeleton.GetBonePtr(editor.activeBone)->isStandardBone)
				menu->Enable(XRCID("setBoneNode"), false);
		}
		else
			menu->Destroy(XRCID("menuBadBone"));

		const std::vector<std::string> selBones = GetSelectedBoneNames();
		bool anyCustom = false;
		for (auto& b : selBones) {
			AnimBone* bPtr = skeleton.GetBonePtr(b);
			if (bPtr && !bPtr->isStandardBone)
				anyCustom = true;
		}

		menu->Enable(XRCID("duplicateBones"), !selBones.empty());
		menu->Enable(XRCID("duplicateBonesMirrored"), !selBones.empty());
		menu->Enable(XRCID("mirrorBonesX"), anyCustom);
	}
	else
		contextParentBone = bone;

	wxPoint menuPos = wxDefaultPosition;
	if (at && pos != wxDefaultPosition)
		menuPos = ScreenToClient(at->ClientToScreen(pos));

	// The menu commands run before PopupMenu returns.
	PopupMenu(menu, menuPos);
	delete menu;

	contextParentBone.clear();
}

wxTreeItemId OutfitStudioFrame::FindBoneItem(const std::string& bone) {
	if (!outfitBones || !bonesRoot.IsOk() || bone.empty())
		return wxTreeItemId();

	const wxString boneStr = wxString::FromUTF8(bone);

	wxTreeItemIdValue cookie;
	wxTreeItemId item = outfitBones->GetFirstChild(bonesRoot, cookie);
	while (item.IsOk()) {
		if (outfitBones->GetItemText(item) == boneStr)
			return item;
		item = outfitBones->GetNextChild(bonesRoot, cookie);
	}

	return wxTreeItemId();
}

bool OutfitStudioFrame::SelectBoneFromViewport(const std::string& bone, bool toggle, bool keepSelection) {
	wxTreeItemId item = FindBoneItem(bone);
	if (!item.IsOk())
		return false;

	recursingUI = true;
	if (toggle) {
		outfitBones->ToggleItemSelection(item);
	}
	else if (!keepSelection || !outfitBones->IsSelected(item)) {
		outfitBones->UnselectAll();
		outfitBones->SelectItem(item);
	}
	recursingUI = false;

	outfitBones->EnsureVisible(item);

	wxTreeEvent treeEvent(wxEVT_TREE_SEL_CHANGED, outfitBones, item);
	OnBoneSelect(treeEvent);
	return true;
}

void OutfitStudioFrame::OnBoneTreeContext(wxCommandEvent& WXUNUSED(event)) {
	wxMenu* menu = wxXmlResource::Get()->LoadMenu("menuBoneTreeContext");
	if (menu) {
		PopupMenu(menu);
		delete menu;
	}
}

void OutfitStudioFrame::OnSetBoneSkin(wxCommandEvent& WXUNUSED(event)) {
	std::string shape = activeItem->GetShape()->name.get();
	project->GetWorkAnim()->RecalcXFormSkinToBone(shape, editor.activeBone);

	glView->UpdateBones();
	HighlightBoneNamesWithWeights();
	ApplyPose();
	SetPendingChanges();
}

void OutfitStudioFrame::OnSetBoneNode(wxCommandEvent& WXUNUSED(event)) {
	std::string shape = activeItem->GetShape()->name.get();
	project->GetWorkAnim()->RecalcCustomBoneXFormsFromSkin(shape, editor.activeBone);

	glView->UpdateBones();
	HighlightBoneNamesWithWeights();
	ApplyPose();
	SetPendingChanges();
}

void OutfitStudioFrame::OnAddBone(wxCommandEvent& WXUNUSED(event)) {
	wxDialog dlg;
	if (!wxXmlResource::Get()->LoadDialog(&dlg, this, "dlgSkeletonBones"))
		return;

	CloseBrushSettings();

	dlg.SetSize(450, 470);
	dlg.CenterOnParent();

	wxTreeCtrl* boneTree = XRCCTRL(dlg, "boneTree", wxTreeCtrl);

	// Bones are added as children of the bone the menu was opened on, so the
	// dialog opens with that bone expanded to show its children.
	const std::string parentBone = contextParentBone.empty() ? editor.activeBone : contextParentBone;
	wxTreeItemId parentItem;

	std::function<void(wxTreeItemId, AnimBone*)> fAddBoneChildren = [&](wxTreeItemId treeParent, AnimBone* boneParent) {
		for (auto& cb : boneParent->children) {
			if (!cb->boneName.empty()) {
				auto newItem = boneTree->AppendItem(treeParent, cb->boneName);
				fAddBoneChildren(newItem, cb);
				if (cb->boneName == parentBone)
					parentItem = newItem;
			}
			else
				fAddBoneChildren(treeParent, cb);
		}
	};

	AnimBone* rb = skeleton.GetRootBonePtr();
	wxTreeItemId rt = boneTree->AddRoot(rb->boneName);
	fAddBoneChildren(rt, rb);

	// Nothing is preselected, so OK doesn't add the parent itself by accident
	if (parentItem.IsOk()) {
		boneTree->Expand(parentItem);
		wxTreeItemIdValue cookie;
		wxTreeItemId firstChild = boneTree->GetFirstChild(parentItem, cookie);
		boneTree->EnsureVisible(firstChild.IsOk() ? firstChild : parentItem);
	}

	if (dlg.ShowModal() == wxID_OK) {
		std::vector<std::string> activeBones;
		project->GetActiveBones(activeBones);
		std::set<std::string> projectBones(activeBones.begin(), activeBones.end());

		std::string lastAdded;
		wxArrayTreeItemIds sel;
		boneTree->GetSelections(sel);
		for (size_t i = 0; i < sel.size(); i++) {
			std::string bone = boneTree->GetItemText(sel[i]).ToStdString();
			if (projectBones.count(bone) != 0)
				continue;

			wxLogMessage("Adding bone '%s' to project.", bone);

			project->AddBoneRef(bone);
			projectBones.insert(bone);
			wxTreeItemId item = outfitBones->AppendItem(bonesRoot, bone);
			UpdateBoneItemState(item, bone);
			cXMirrorBone->AppendString(bone);
			posePanel->GetBoneChoice()->AppendString(bone);
			lastAdded = bone;
		}

		glView->UpdateBones();
		UpdateBoneCounts();
		SetPendingChanges();

		if (!lastAdded.empty())
			SelectBoneFromViewport(lastAdded);

		glView->Render();
	}
}

void OutfitStudioFrame::FillParentBoneChoice(wxDialog& dlg, const std::string& selBone) {
	wxChoice* cParentBone = XRCCTRL(dlg, "cParentBone", wxChoice);
	cParentBone->AppendString("(none)");

	std::set<std::string> boneSet;
	for (auto selItem : selectedItems) {
		const std::vector<std::string>& bones = project->GetWorkAnim()->shapeBones[selItem->GetShape()->name.get()];
		for (const std::string& b : bones)
			boneSet.insert(b);
	}

	for (auto& bone : boneSet) {
		cParentBone->AppendString(bone);
		if (bone == selBone)
			cParentBone->SetSelection(cParentBone->GetCount() - 1);
	}

	if (cParentBone->GetSelection() == wxNOT_FOUND) {
		if (selBone.empty()) {
			cParentBone->SetSelection(0);
		}
		else {
			cParentBone->AppendString(selBone);
			cParentBone->SetSelection(cParentBone->GetCount() - 1);
		}
	}
}

void OutfitStudioFrame::GetBoneDlgData(wxDialog& dlg, MatTransform& xform, std::string& parentBone, int& addCount) {
	xform.translation.x = atof(XRCCTRL(dlg, "textX", wxTextCtrl)->GetValue().c_str());
	xform.translation.y = atof(XRCCTRL(dlg, "textY", wxTextCtrl)->GetValue().c_str());
	xform.translation.z = atof(XRCCTRL(dlg, "textZ", wxTextCtrl)->GetValue().c_str());

	Vector3 rotvec;
	rotvec.x = atof(XRCCTRL(dlg, "textRX", wxTextCtrl)->GetValue().c_str());
	rotvec.y = atof(XRCCTRL(dlg, "textRY", wxTextCtrl)->GetValue().c_str());
	rotvec.z = atof(XRCCTRL(dlg, "textRZ", wxTextCtrl)->GetValue().c_str());
	xform.rotation = RotVecToMat(rotvec);

	wxChoice* cParentBone = XRCCTRL(dlg, "cParentBone", wxChoice);
	int pBChoice = cParentBone->GetSelection();
	if (pBChoice != wxNOT_FOUND)
		parentBone = cParentBone->GetString(pBChoice).ToStdString();

	if (parentBone == "(none)")
		parentBone = std::string();

	wxSpinCtrl* numAddCount = XRCCTRL(dlg, "numAddCount", wxSpinCtrl);
	if (numAddCount)
		addCount = numAddCount->GetValue();
}

void OutfitStudioFrame::OnAddCustomBone(wxCommandEvent& WXUNUSED(event)) {
	wxDialog dlg;
	if (!wxXmlResource::Get()->LoadDialog(&dlg, this, "dlgCustomBone"))
		return;

	CloseBrushSettings();

	dlg.Bind(wxEVT_CHAR_HOOK, &OutfitStudioFrame::OnEnterClose, this);
	FillParentBoneChoice(dlg, contextParentBone.empty() ? editor.activeBone : contextParentBone);

	if (dlg.ShowModal() != wxID_OK)
		return;

	wxString bone = XRCCTRL(dlg, "boneName", wxTextCtrl)->GetValue();
	if (bone.empty()) {
		wxMessageBox(_("No bone name was entered!"), _("Error"), wxICON_INFORMATION, this);
		return;
	}

	wxTreeItemIdValue cookie;
	wxTreeItemId item = outfitBones->GetFirstChild(bonesRoot, cookie);
	while (item.IsOk()) {
		if (outfitBones->GetItemText(item) == bone) {
			wxMessageBox(wxString::Format(_("Bone '%s' already exists in the project!"), bone), _("Error"), wxICON_INFORMATION, this);
			return;
		}
		item = outfitBones->GetNextChild(bonesRoot, cookie);
	}

	MatTransform xform;
	std::string parentBone;
	int addCount = 1;
	GetBoneDlgData(dlg, xform, parentBone, addCount);

	wxLogMessage("Adding custom bone '%s' to project (%d/%d).", bone, 1, addCount);
	project->AddCustomBoneRef(bone.ToStdString(), parentBone, xform);

	AddBoneItemToGUI(bone.ToStdString());

	wxString origBone = bone;
	int suffixNumber = 0;

	size_t firstDigit = origBone.find_first_of("0123456789");
	if (firstDigit != std::string::npos) {
		wxString strNumber = origBone.substr(origBone.find_first_of("0123456789"));
		suffixNumber = wxAtoi(strNumber);
	}

	parentBone = bone;

	for (int i = 1; i < addCount; i++) {
		if (firstDigit != std::string::npos)
			bone = origBone.substr(0, firstDigit) + std::to_string(++suffixNumber);
		else
			bone = origBone + std::to_string(++suffixNumber);

		wxTreeItemIdValue cookie2;
		item = outfitBones->GetFirstChild(bonesRoot, cookie2);
		while (item.IsOk()) {
			if (outfitBones->GetItemText(item) == bone) {
				wxMessageBox(wxString::Format(_("Bone '%s' already exists in the project!"), bone), _("Error"), wxICON_INFORMATION, this);
				break;
			}
			item = outfitBones->GetNextChild(bonesRoot, cookie2);
		}

		wxLogMessage("Adding custom bone '%s' to project (%d/%d).", bone, i, addCount);
		project->AddCustomBoneRef(bone.ToStdString(), parentBone, xform);

		AddBoneItemToGUI(bone.ToStdString());

		parentBone = bone;
	}

	glView->UpdateBones();
	UpdateBoneCounts();
	SetPendingChanges();

	// The new bone starts on its parent's position; selecting it puts the
	// transform tool on it, ready to be moved out.
	SelectBoneFromViewport(std::string(bone.ToUTF8()));

	glView->Render();
}

void OutfitStudioFrame::OnEditBone(wxCommandEvent& WXUNUSED(event)) {
	AnimBone* bPtr = skeleton.GetBonePtr(editor.activeBone);
	if (!bPtr)
		return;

	wxDialog dlg;
	if (!wxXmlResource::Get()->LoadDialog(&dlg, this, "dlgCustomBone"))
		return;

	CloseBrushSettings();

	dlg.Bind(wxEVT_CHAR_HOOK, &OutfitStudioFrame::OnEnterClose, this);

	if (bPtr->parent)
		FillParentBoneChoice(dlg, bPtr->parent->boneName);
	else
		FillParentBoneChoice(dlg);

	wxTextCtrl* boneNameTC = XRCCTRL(dlg, "boneName", wxTextCtrl);
	boneNameTC->SetValue(bPtr->boneName);
	boneNameTC->Disable();

	Vector3 rotvec = RotMatToVec(bPtr->xformToParent.rotation);
	XRCCTRL(dlg, "textX", wxTextCtrl)->SetValue(wxString() << bPtr->xformToParent.translation.x);
	XRCCTRL(dlg, "textY", wxTextCtrl)->SetValue(wxString() << bPtr->xformToParent.translation.y);
	XRCCTRL(dlg, "textZ", wxTextCtrl)->SetValue(wxString() << bPtr->xformToParent.translation.z);
	XRCCTRL(dlg, "textRX", wxTextCtrl)->SetValue(wxString() << rotvec.x);
	XRCCTRL(dlg, "textRY", wxTextCtrl)->SetValue(wxString() << rotvec.y);
	XRCCTRL(dlg, "textRZ", wxTextCtrl)->SetValue(wxString() << rotvec.z);

	XRCCTRL(dlg, "lbAddCount", wxStaticText)->Hide();
	XRCCTRL(dlg, "numAddCount", wxSpinCtrl)->Hide();

	if (bPtr->isStandardBone) {
		dlg.SetLabel(_("View Standard Bone"));
		XRCCTRL(dlg, "textX", wxTextCtrl)->Disable();
		XRCCTRL(dlg, "textY", wxTextCtrl)->Disable();
		XRCCTRL(dlg, "textZ", wxTextCtrl)->Disable();
		XRCCTRL(dlg, "textRX", wxTextCtrl)->Disable();
		XRCCTRL(dlg, "textRY", wxTextCtrl)->Disable();
		XRCCTRL(dlg, "textRZ", wxTextCtrl)->Disable();
		XRCCTRL(dlg, "cParentBone", wxChoice)->Disable();
		XRCCTRL(dlg, "wxID_OK", wxButton)->Disable();
	}
	else {
		dlg.SetLabel(_("Edit Custom Bone"));
	}

	if (dlg.ShowModal() != wxID_OK)
		return;

	MatTransform xform;
	std::string parentBone;
	int addCount = 1;
	GetBoneDlgData(dlg, xform, parentBone, addCount);

	project->ModifyCustomBone(bPtr, parentBone, xform);
	glView->UpdateBones();
	ApplyPose();
	SetPendingChanges();
}

wxTreeItemId OutfitStudioFrame::AddBoneItemToGUI(const std::string& bone) {
	wxTreeItemId item = outfitBones->AppendItem(bonesRoot, bone);
	UpdateBoneItemState(item, bone);

	cXMirrorBone->AppendString(bone);
	posePanel->GetBoneChoice()->AppendString(bone);
	return item;
}

std::vector<std::string> OutfitStudioFrame::GetSelectedBoneNames() {
	std::vector<std::string> bones;

	wxTreeItemIdValue cookie;
	wxTreeItemId item = outfitBones->GetFirstChild(bonesRoot, cookie);
	while (item.IsOk()) {
		if (outfitBones->IsSelected(item))
			bones.push_back(outfitBones->GetItemText(item).ToStdString());
		item = outfitBones->GetNextChild(bonesRoot, cookie);
	}

	if (bones.empty() && !editor.activeBone.empty())
		bones.push_back(editor.activeBone);

	return bones;
}

void OutfitStudioFrame::DuplicateSelectedBones(bool mirror) {
	const std::vector<std::string> bones = GetSelectedBoneNames();
	if (bones.empty()) {
		wxMessageBox(_("No bones are selected."), _("Error"), wxICON_INFORMATION, this);
		return;
	}

	wxDialog dlg;
	if (!wxXmlResource::Get()->LoadDialog(&dlg, this, "dlgDuplicateBones"))
		return;

	CloseBrushSettings();

	wxTextCtrl* textFind = XRCCTRL(dlg, "textFind", wxTextCtrl);
	wxTextCtrl* textReplace = XRCCTRL(dlg, "textReplace", wxTextCtrl);
	wxTextCtrl* textSuffix = XRCCTRL(dlg, "textSuffix", wxTextCtrl);
	wxCheckBox* chkMirrorX = XRCCTRL(dlg, "chkMirrorX", wxCheckBox);
	wxListBox* listPreview = XRCCTRL(dlg, "listPreview", wxListBox);
	wxStaticText* lbConflict = XRCCTRL(dlg, "lbConflict", wxStaticText);
	wxButton* btnOK = XRCCTRL(dlg, "wxID_OK", wxButton);

	chkMirrorX->SetValue(mirror);
	textSuffix->ChangeValue(wxString(project->NextFreeDuplicateSuffix(bones, "", "")));

	// Parent choices: what the chain hangs from now, no parent, or any bone of the project. Each entry's
	// parent, or none to keep the originals' parents when they differ.
	wxChoice* cDupParent = XRCCTRL(dlg, "cDupParent", wxChoice);
	std::vector<std::optional<std::string>> parentChoices;
	const std::set<std::string> chainParents = project->DuplicateChainParents(bones);
	const bool oneParent = chainParents.size() == 1;
	if (!oneParent) {
		cDupParent->Append(_("(original parents)"));
		parentChoices.push_back(std::nullopt);
	}
	cDupParent->Append(_("(none)"));
	parentChoices.push_back(std::string());

	std::vector<std::string> projectBones;
	project->GetActiveBones(projectBones);
	if (oneParent && !chainParents.begin()->empty() && std::find(projectBones.begin(), projectBones.end(), *chainParents.begin()) == projectBones.end())
		projectBones.push_back(*chainParents.begin());
	std::sort(projectBones.begin(), projectBones.end());
	for (auto& bone : projectBones) {
		cDupParent->Append(wxString(bone));
		parentChoices.push_back(bone);
	}

	cDupParent->SetSelection(0);
	if (oneParent) {
		for (size_t i = 0; i < parentChoices.size(); ++i)
			if (parentChoices[i] == *chainParents.begin())
				cDupParent->SetSelection(static_cast<int>(i));
	}

	auto conflictText = [](OutfitProject::DuplicateBoneConflict conflict, const std::string& newName) {
		switch (conflict) {
			case OutfitProject::DuplicateBoneConflict::EmptyName: return wxString(_("A new bone name is empty."));
			case OutfitProject::DuplicateBoneConflict::StandardBone:
				return wxString::Format(_("'%s' is a bone of the reference skeleton."), wxString(newName));
			case OutfitProject::DuplicateBoneConflict::Original:
				return wxString::Format(_("'%s' is one of the bones to duplicate."), wxString(newName));
			case OutfitProject::DuplicateBoneConflict::InProject:
				return wxString::Format(_("Bone '%s' already exists in the project!"), wxString(newName));
			case OutfitProject::DuplicateBoneConflict::ParentBone:
				return wxString::Format(_("Bone '%s' already exists in the skeleton as the parent of other bones."), wxString(newName));
			case OutfitProject::DuplicateBoneConflict::Repeated:
				return wxString::Format(_("More than one bone would be named '%s'."), wxString(newName));
			default: return wxString();
		}
	};

	auto updatePreview = [&]() {
		const std::vector<std::string> newNames = OutfitProject::DuplicateBoneNames(bones,
																					 textFind->GetValue().ToStdString(),
																					 textReplace->GetValue().ToStdString(),
																					 textSuffix->GetValue().ToStdString());
		const std::vector<OutfitProject::DuplicateBoneConflict> conflicts = project->DuplicateBoneConflicts(bones, newNames);

		wxArrayString lines;
		wxString firstConflict;
		for (size_t i = 0; i < bones.size(); ++i) {
			wxString line = wxString(bones[i]) + "  ->  " + wxString(newNames[i]);
			if (conflicts[i] != OutfitProject::DuplicateBoneConflict::None) {
				line += "  (!)";
				if (firstConflict.empty())
					firstConflict = conflictText(conflicts[i], newNames[i]);
			}
			lines.Add(line);
		}

		listPreview->Set(lines);
		lbConflict->SetLabel(firstConflict);
		lbConflict->Wrap(listPreview->GetSize().GetWidth());
		btnOK->Enable(firstConflict.empty());
		dlg.Layout();
	};

	// Until it's edited, the suffix follows the replacement: none when the replaced names are free
	bool suffixEdited = false;
	auto replaceChanged = [&](wxCommandEvent&) {
		if (!suffixEdited)
			textSuffix->ChangeValue(
				wxString(project->NextFreeDuplicateSuffix(bones, textFind->GetValue().ToStdString(), textReplace->GetValue().ToStdString())));
		updatePreview();
	};

	textFind->Bind(wxEVT_TEXT, replaceChanged);
	textReplace->Bind(wxEVT_TEXT, replaceChanged);
	textSuffix->Bind(wxEVT_TEXT, [&](wxCommandEvent&) {
		suffixEdited = true;
		updatePreview();
	});
	dlg.Bind(wxEVT_CHAR_HOOK, [&](wxKeyEvent& event) {
		if (event.GetKeyCode() == WXK_RETURN || event.GetKeyCode() == WXK_NUMPAD_ENTER) {
			if (btnOK->IsEnabled())
				dlg.EndModal(wxID_OK);
		}
		else
			event.Skip();
	});

	updatePreview();
	dlg.Fit();
	dlg.CenterOnParent();

	if (dlg.ShowModal() != wxID_OK)
		return;

	const std::string find = textFind->GetValue().ToStdString();
	const std::string replace = textReplace->GetValue().ToStdString();
	const std::string suffix = textSuffix->GetValue().ToStdString();
	const bool mirrorNew = chkMirrorX->GetValue();

	// The chain's own parent needs no change
	std::optional<std::string> newParent;
	const int parentSel = cDupParent->GetSelection();
	if (parentSel != wxNOT_FOUND && static_cast<size_t>(parentSel) < parentChoices.size())
		newParent = parentChoices[parentSel];
	if (newParent && oneParent && *newParent == *chainParents.begin())
		newParent.reset();

	std::vector<std::string> newNames;
	std::string error;
	if (!project->DuplicateBones(bones, find, replace, suffix, newParent, newNames, error)) {
		wxMessageBox(wxString(error), _("Error"), wxICON_INFORMATION, this);
		return;
	}

	std::vector<wxTreeItemId> newItems;
	for (auto& newName : newNames)
		newItems.push_back(AddBoneItemToGUI(newName));

	// Not undoable, like adding the bones: undoing would only leave unmirrored new bones
	if (mirrorNew)
		project->MirrorBonesX(newNames);

	glView->UpdateBones();
	UpdateBoneCounts();
	ApplyPose();
	SetPendingChanges();

	// The copies are selected, ready to be mirrored or moved
	recursingUI = true;
	outfitBones->UnselectAll();
	for (auto& item : newItems)
		outfitBones->SelectItem(item, true);
	recursingUI = false;

	if (!newItems.empty()) {
		outfitBones->EnsureVisible(newItems.front());
		wxTreeEvent treeEvent(wxEVT_TREE_SEL_CHANGED, outfitBones, newItems.front());
		OnBoneSelect(treeEvent);
	}

	glView->Render();
}

void OutfitStudioFrame::OnDuplicateBones(wxCommandEvent& WXUNUSED(event)) {
	DuplicateSelectedBones(false);
}

void OutfitStudioFrame::OnDuplicateBonesMirrored(wxCommandEvent& WXUNUSED(event)) {
	DuplicateSelectedBones(true);
}

void OutfitStudioFrame::OnMirrorBonesX(wxCommandEvent& WXUNUSED(event)) {
	const std::vector<std::string> bones = GetSelectedBoneNames();
	if (bones.empty())
		return;

	std::vector<UndoStateBoneXform> xforms = project->MirrorBonesX(bones);
	if (xforms.empty()) {
		wxMessageBox(_("Only custom bones can be mirrored."), _("Error"), wxICON_INFORMATION, this);
		return;
	}

	UndoStateProject* usp = glView->GetUndoHistory()->PushState();
	usp->undoType = UndoType::BoneEdit;
	usp->boneXforms = std::move(xforms);

	glView->UpdateBones();
	ApplyPose();
	RefreshBoneTreeBadBoneIcons();
	UpdateUndoTools();
	SetPendingChanges();

	if (glView->GetTransformMode())
		glView->ShowTransformTool();
	else
		glView->Render();
}

void OutfitStudioFrame::OnDeleteBone(wxCommandEvent& WXUNUSED(event)) {
	wxArrayTreeItemIds selItems;
	outfitBones->GetSelections(selItems);

	// Deleted together, so a parent deleted with its child bones doesn't keep
	// its node for them
	std::vector<std::string> deleteBones;
	for (size_t i = 0; i < selItems.size(); i++)
		deleteBones.push_back(outfitBones->GetItemText(selItems[i]).ToStdString());

	project->DeleteBones(deleteBones);

	for (size_t i = 0; i < selItems.size(); i++) {
		wxString boneText = outfitBones->GetItemText(selItems[i]);
		wxLogMessage("Deleting bone '%s' from project.", boneText);

		std::string bone = boneText.ToStdString();
		editor.activeBone.clear();

		outfitBones->Delete(selItems[i]);
		editor.lastSelectedBones.erase(bone);
		editor.lastNormalizeBones.erase(bone);

		int idxMirrorBone = cXMirrorBone->FindString(boneText);
		if (idxMirrorBone != wxNOT_FOUND)
			cXMirrorBone->Delete(idxMirrorBone);

		int idxPoseBone = posePanel->GetBoneChoice()->FindString(boneText);
		if (idxPoseBone != wxNOT_FOUND)
			posePanel->GetBoneChoice()->Delete(idxPoseBone);
	}

	if (cXMirrorBone->GetStringSelection().IsEmpty())
		cXMirrorBone->SetSelection(0);

	if (posePanel->GetBoneChoice()->GetStringSelection().IsEmpty())
		posePanel->GetBoneChoice()->SetSelection(wxNOT_FOUND);

	glView->UpdateBones();
	ReselectBone();
	CalcAutoXMirrorBone();
	glView->GetUndoHistory()->ClearHistory();
	UpdateBoneCounts();
	UpdateUndoTools();
	SetPendingChanges();
}

void OutfitStudioFrame::OnDeleteBoneFromSelected(wxCommandEvent& WXUNUSED(event)) {
	wxArrayTreeItemIds selItems;
	outfitBones->GetSelections(selItems);
	for (size_t i = 0; i < selItems.size(); i++) {
		std::string bone = outfitBones->GetItemText(selItems[i]).ToStdString();
		wxLogMessage("Deleting weights of bone '%s' from selected shapes.", bone);

		for (auto& s : selectedItems)
			project->GetWorkAnim()->RemoveShapeBone(s->GetShape()->name.get(), bone);
	}

	glView->UpdateBones();
	ReselectBone();
	glView->GetUndoHistory()->ClearHistory();
	HighlightBoneNamesWithWeights();
	UpdateBoneCounts();
	UpdateUndoTools();
	SetPendingChanges();
}

bool OutfitStudioFrame::HasUnweightedCheck() {
	std::vector<std::string> unweighted;
	if (project->HasUnweighted(&unweighted)) {
		std::string shapesJoin = JoinStrings(unweighted, "; ");
		wxLogWarning(wxString::Format("Unweighted vertices found on shapes: %s", shapesJoin));

		HostAnswer answer = projectHost.Ask(wxString::Format("%s\n \n%s",
												  _("The following shapes have unweighted vertices, which can cause issues. The affected vertices have been put under a mask. Do "
													"you want to save anyway?"),
												  shapesJoin),
											   _("Unweighted Vertices"),
											   false,
											   HostAnswer::Yes);
		if (answer != HostAnswer::Yes)
			return true;
	}

	return false;
}

void OutfitStudioFrame::ReselectBone() {
	wxArrayTreeItemIds selItems;
	outfitBones->GetSelections(selItems);
	if (!selItems.empty()) {
		wxTreeEvent treeEvent(wxEVT_TREE_SEL_CHANGED, outfitBones, selItems.front());
		OnBoneSelect(treeEvent);
	}
}

void OutfitStudioFrame::CalcCopySkinTransOption(WeightCopyOptions& options) {
	// This function calculates whether the "copy global-to-skin transform
	// from base shape" checkbox should be shown and what the default value
	// for the "transform-geometry" checkbox should be

	NifFile* nif = project->GetWorkNif();
	NiShape* baseShape = project->GetBaseShape();
	if (!baseShape)
		return;

	AnimInfo& workAnim = *project->GetWorkAnim();

	MatTransform globalToBaseShape = workAnim.GetTransformGlobalToShape(baseShape);

	// Check if any shape's CS is different from the base shape's
	for (size_t i = 0; i < selectedItems.size(); i++) {
		NiShape* shape = selectedItems[i]->GetShape();
		if (shape == baseShape)
			continue;

		MatTransform globalToShape = workAnim.GetTransformGlobalToShape(shape);
		if (!globalToShape.IsNearlyEqualTo(globalToBaseShape)) {
			options.showSkinTransOption = true;
			break;
		}
	}

	if (!options.showSkinTransOption)
		// They're all the same, so hide the option
		return;

	options.doSkinTransCopy = true;

	// As a first step in calculating a good default for the transform-geometry option,
	// find the average vertex position of the base shape in its own skin coordinates
	Vector3 baseAvg;

	const std::vector<Vector3>& baseVerts = *nif->GetVertsForShape(baseShape);
	for (size_t i = 0; i < baseVerts.size(); ++i)
		baseAvg += baseVerts[i];

	if (baseVerts.size())
		baseAvg /= static_cast<uint32_t>(baseVerts.size());

	// Now check if any shape would be better aligned by changing its global-to-skin transform
	for (size_t i = 0; i < selectedItems.size(); i++) {
		NiShape* shape = selectedItems[i]->GetShape();
		if (shape == baseShape)
			continue;

		MatTransform globalToShape = workAnim.GetTransformGlobalToShape(shape);
		if (globalToShape.IsNearlyEqualTo(globalToBaseShape))
			continue;

		const std::vector<Vector3>& verts = *nif->GetVertsForShape(shape);
		if (verts.empty())
			continue;

		// Calculate old average and new average.
		MatTransform shapeToGlobal = globalToShape.InverseTransform();
		MatTransform shapeToBaseShape = globalToBaseShape.ComposeTransforms(shapeToGlobal);

		Vector3 oldAvg, newAvg;
		for (size_t j = 0; j < verts.size(); ++j) {
			oldAvg += shapeToBaseShape.ApplyTransform(verts[j]);
			newAvg += verts[j];
		}

		oldAvg /= static_cast<uint32_t>(verts.size());
		newAvg /= static_cast<uint32_t>(verts.size());

		// Check whether old or new is closer to the base shape.
		// If new is farther away, then transforming the geometry would be a good idea
		if (newAvg.DistanceTo(baseAvg) > oldAvg.DistanceTo(baseAvg)) {
			options.doTransformGeo = true;
			break;
		}
	}
}

void OutfitStudioFrame::OnCopyBoneWeight(wxCommandEvent& WXUNUSED(event)) {
	CopyWeightsToSelectedShapes(false);
}

void OutfitStudioFrame::OnCopySelectedWeight(wxCommandEvent& WXUNUSED(event)) {
	CopyWeightsToSelectedShapes(true);
}

void OutfitStudioFrame::CopyWeightsToSelectedShapes(bool selectedBonesOnly) {
	if (!ShapeSelectionCheck())
		return;

	if (!project->GetBaseShape()) {
		wxMessageBox(_("There is no reference shape!"), _("Error"));
		return;
	}

	// Bones selected in the bone list are checked in the dialog, all bones otherwise
	std::vector<std::string> preselectedBones;
	if (selectedBonesOnly) {
		preselectedBones = GetSelectedBones();
		if (preselectedBones.empty()) {
			wxMessageBox(_("There are no bones selected!"), _("Error"));
			return;
		}
	}

	std::vector<NiShape*> selectedShapes;
	for (auto& s : selectedItems) {
		if (auto shape = s->GetShape(); !project->IsBaseShape(shape))
			selectedShapes.push_back(s->GetShape());
		else
			wxMessageBox(_("Sorry, you can't copy weights from the reference shape to itself. Skipping this shape."), _("Can't copy weights"), wxICON_WARNING);
	}
	CopyBoneWeightForShapes(selectedShapes, false, preselectedBones);
}

int OutfitStudioFrame::CopyBoneWeightForShapes(std::vector<NiShape*> shapes, bool silent, const std::vector<std::string>& preselectedBones) {
	if (shapes.empty())
		return 0;

	CloseBrushSettings();

	WeightCopyOptions options;
	options.preselectedBones = preselectedBones;
	CalcCopySkinTransOption(options);

	WeightCopyDialog dlg(this, project, glView, poseDataCollection, editor.lastNormalizeBones, shapes, options, silent);
	if (dlg.GetResult()) {
		StartProgress(_("Copying bone weights..."));

		BoneWeightCopyOptions copyOptions;
		copyOptions.proximityRadius = options.proximityRadius;
		copyOptions.maxResults = options.maxResults;
		copyOptions.selectedBones = options.selectedBones;
		copyOptions.copySkinTransform = options.doSkinTransCopy;
		copyOptions.transformGeometry = options.doTransformGeo;

		UndoStateProject* usp = glView->GetUndoHistory()->PushState();
		OpResult result = ProjectOperations(*project, editor).CopyBoneWeights(shapes, copyOptions, *usp);
		if (!result.ok || usp->usss.empty())
			glView->GetUndoHistory()->PopState();

		ApplyRefresh(result.refresh);

		if (!result.ok)
			wxMessageBox(wxString::FromUTF8(result.error), _("Copy Bone Weights"), wxOK | wxICON_WARNING, this);
		UpdateUndoTools();

		EndProgress();
	}

	return 0;
}

void OutfitStudioFrame::OnTransferSelectedWeight(wxCommandEvent& WXUNUSED(event)) {
	if (!ShapeSelectionCheck())
		return;

	auto baseShape = project->GetBaseShape();
	if (!baseShape) {
		wxMessageBox(_("There is no reference shape!"), _("Error"));
		return;
	}

	if (project->IsBaseShape(activeItem->GetShape())) {
		wxMessageBox(_("Sorry, you can't copy weights from the reference shape to itself."), _("Error"));
		return;
	}

	int baseVertCount = project->GetVertexCount(baseShape);
	int workVertCount = project->GetVertexCount(activeItem->GetShape());
	if (baseVertCount != workVertCount) {
		wxMessageBox(_("The vertex count of the reference and chosen shape is not the same!"), _("Error"));
		return;
	}

	std::vector<std::string> selectedBones = GetSelectedBones();
	if (selectedBones.size() < 1)
		return;

	std::string bonesString;
	for (std::string& boneName : selectedBones)
		bonesString += "'" + boneName + "' ";

	wxLogMessage("Transferring selected bone weights to '%s' for %s...", activeItem->GetShape()->name.get(), bonesString);
	StartProgress(_("Transferring bone weights..."));

	std::unordered_map<uint16_t, float> mask;
	glView->GetActiveMask(mask);
	project->TransferSelectedWeights(activeItem->GetShape(), &mask, &selectedBones);

	UpdateAnimationGUI();

	EndProgress();
	SetPendingChanges();
}

void OutfitStudioFrame::OnMaskWeighted(wxCommandEvent& WXUNUSED(event)) {
	if (!ShapeSelectionCheck())
		return;

	UndoStateProject* usp = glView->GetUndoHistory()->PushState();
	usp->undoType = UndoType::Mask;

	for (auto& selItem : selectedItems) {
		std::string shapeName = selItem->GetShape()->name.get();
		Mesh* m = glView->GetMesh(shapeName);
		if (!m)
			continue;

		usp->usss.emplace_back();
		UndoStateShape& uss = usp->usss.back();
		uss.shapeName = m->shapeName;

		for (int i = 0; i < m->nVerts; i++) {
			uss.pointStartState[i].x = m->mask[i];
			uss.pointEndState[i].x = 0.0f;
		}

		auto& bones = project->GetWorkAnim()->shapeBones;
		if (bones.find(shapeName) != bones.end()) {
			for (auto& b : bones[shapeName]) {
				auto weights = project->GetWorkAnim()->GetWeightsPtr(shapeName, b);
				if (weights) {
					for (auto& bw : *weights)
						if (bw.second > 0.0f)
							uss.pointEndState[bw.first].x = 1.0f;
				}
			}
		}
	}

	glView->ApplyUndoState(usp, false);

	if (!Config.GetBoolValue("Input/MaskHistory"))
		glView->GetUndoHistory()->PopState();

	UpdateUndoTools();
}

void OutfitStudioFrame::OnMaskBoneWeighted(wxCommandEvent& WXUNUSED(event)) {
	if (!ShapeSelectionCheck())
		return;

	UndoStateProject* usp = glView->GetUndoHistory()->PushState();
	usp->undoType = UndoType::Mask;

	for (auto& selItem : selectedItems) {
		std::string shapeName = selItem->GetShape()->name.get();
		Mesh* m = glView->GetMesh(shapeName);
		if (!m || !m->mask)
			continue;

		usp->usss.emplace_back();
		UndoStateShape& uss = usp->usss.back();
		uss.shapeName = m->shapeName;

		for (int i = 0; i < m->nVerts; i++) {
			uss.pointStartState[i].x = m->mask[i];
			uss.pointEndState[i].x = 0.0f;
		}

		for (auto& b : GetSelectedBones()) {
			auto weights = project->GetWorkAnim()->GetWeightsPtr(shapeName, b);
			if (weights) {
				for (auto& bw : *weights)
					if (bw.second > 0.0f)
						uss.pointEndState[bw.first].x = 1.0f;
			}
		}
	}

	glView->ApplyUndoState(usp, false);

	if (!Config.GetBoolValue("Input/MaskHistory"))
		glView->GetUndoHistory()->PopState();

	UpdateUndoTools();
}

void OutfitStudioFrame::OnCheckBadBones(wxCommandEvent& WXUNUSED(event)) {
	BadBonesReport report = project->FindBadBones();
	if (!report.found) {
		wxMessageBox(_("No Bad Bones Found."), _("No Bad Bones"), wxOK, this);
		return;
	}

	if (!ShowBadBonesDialog(this, report, *project->GetWorkAnim()))
		return;

	project->FixBadBones(report);

	// FixBadBones updates AnimBone transforms in-place but leaves the
	// NIF nodes untouched; push the fixed transforms out so every view
	// (node overlays, bad-bone indicator, gizmo center) is consistent.
	if (auto* nif = project->GetWorkNif()) {
		if (auto* workAnim = project->GetWorkAnim())
			workAnim->WriteNodesToNif(nif);
	}

	glView->UpdateBones();
	glView->UpdateNodes();
	RefreshBoneTreeBadBoneIcons();
	HighlightBoneNamesWithWeights();
	ApplyPose();
	SetPendingChanges();
}

void OutfitStudioFrame::UpdateBoneTransformToolEnabled() {
	// The transform tool is generally enabled on every tab except bones (and
	// colors), but on the bones tab we want it enabled when the user has
	// turned on bones-mode or nodes-mode, so they can drag bones/nodes.
	if (!currentTabButton || !boneTabButton || currentTabButton->GetId() != boneTabButton->GetId())
		return;

	const bool enable = glView->GetBonesMode() || glView->GetNodesMode();
	menuBar->Enable(XRCID("btnTransform"), enable);
	toolBarV->EnableTool(XRCID("btnTransform"), enable);

	if (!enable) {
		// Hiding bones and nodes only suspends the transform tool; it comes back
		// with them. The button is unchecked meanwhile, as nothing is shown.
		if (glView->GetTransformMode()) {
			boneTransformSuspended = true;
			glView->SetTransformMode(false);
		}
		menuBar->Check(XRCID("btnTransform"), false);
		toolBarV->ToggleTool(XRCID("btnTransform"), false);
	}
	else if (boneTransformSuspended) {
		boneTransformSuspended = false;
		if (!glView->GetTransformMode()) {
			glView->SetTransformMode(true);
			menuBar->Check(XRCID("btnTransform"), true);
			toolBarV->ToggleTool(XRCID("btnTransform"), true);
		}
	}
}

void OutfitStudioFrame::RefreshBoneTreeBadBoneIcons() {
	// Iterate the bone list and refresh each item's state so that the
	// bad-bones icon reflects any inconsistent transforms introduced by the
	// last edit.
	if (!outfitBones || !bonesRoot.IsOk())
		return;

	wxTreeItemIdValue cookie;
	wxTreeItemId item = outfitBones->GetFirstChild(bonesRoot, cookie);
	while (item.IsOk()) {
		UpdateBoneItemState(item, outfitBones->GetItemText(item).ToStdString());
		item = outfitBones->GetNextChild(bonesRoot, cookie);
	}
}

void OutfitStudioFrame::ApplyPose() {
	// While an animation plays there is no separate physics pump; the
	// simulation advances in lockstep with each displayed frame, using the
	// wall-clock time since the last step.
	physicsPreview->StepWithAnimation();

	// Asking for a BVH update queues the shape with wxGLPanel::OnIdle, which
	// rebuilds a whole AABB tree over every triangle of it - far too expensive
	// to pay once per displayed animation frame. The tree only serves picking
	// and brush hit tests, both locked out during playback, so the rebuild is
	// deferred to the single ApplyPose that AnimationPlayer::Pause does.
	const bool updateBVH = !animation->IsPlaying() && !physicsPreview->IsPumping();

	for (auto& shape : project->GetWorkNif()->GetShapes())
		UpdateLiveMesh(shape, updateBVH);

	// Bone overlays cost one GL mesh created and destroyed per bone, not worth
	// paying for every frame while they are not being drawn.
	if (!animation->IsPlaying() || glView->GetBonesMode() || glView->GetNodesMode())
		glView->UpdateBones();

	glView->Render();
}
