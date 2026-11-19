/*
BodySlide and Outfit Studio
See the included LICENSE file
*/


#include "SegmentPartitionEditor.h"
#include "OutfitStudioFrame.h"
#include "FrameHelpers.h"
#include "ShapeTreeItems.h"
#include "GLPanel.h"
#include "OutfitStudioApp.h"
#include "ui/wxStateButton.h"
#include "utils/TargetGame.h"
#include "files/HkxFile.h"

#include <cmath>
#include <cstdlib>
#include <iterator>
#include <sstream>
#include <wx/textctrl.h>

using namespace nifly;

SegmentPartitionEditor::SegmentPartitionEditor(OutfitStudioFrame& frame)
	: frame(frame) {}

void SegmentPartitionEditor::Init() {
	segmentTree = (wxTreeCtrl*)frame.FindWindowByName("segmentTree");
	if (segmentTree) {
		wxImageList* segmentStateImages = new wxImageList(16, 16, false, 2);
		wxBitmap visImg(wxString::FromUTF8(Config["AppDir"]) + "/res/images/icoVisible.png", wxBITMAP_TYPE_PNG);
		wxBitmap invImg(wxString::FromUTF8(Config["AppDir"]) + "/res/images/icoInvisible.png", wxBITMAP_TYPE_PNG);

		if (visImg.IsOk())
			segmentStateImages->Add(visImg);
		if (invImg.IsOk())
			segmentStateImages->Add(invImg);

		segmentTree->AssignStateImageList(segmentStateImages);
		segmentRoot = segmentTree->AddRoot("Segments");
	}

	partitionTree = (wxTreeCtrl*)frame.FindWindowByName("partitionTree");
	if (partitionTree) {
		wxImageList* partitionStateImages = new wxImageList(16, 16, false, 2);
		wxBitmap visImg(wxString::FromUTF8(Config["AppDir"]) + "/res/images/icoVisible.png", wxBITMAP_TYPE_PNG);
		wxBitmap invImg(wxString::FromUTF8(Config["AppDir"]) + "/res/images/icoInvisible.png", wxBITMAP_TYPE_PNG);

		if (visImg.IsOk())
			partitionStateImages->Add(visImg);
		if (invImg.IsOk())
			partitionStateImages->Add(invImg);

		partitionTree->AssignStateImageList(partitionStateImages);
		partitionRoot = partitionTree->AddRoot("Partitions");
	}

	frame.Bind(wxEVT_MENU, &SegmentPartitionEditor::OnCopySegPart, this, XRCID("copySegPart"));
	frame.Bind(wxEVT_TREE_STATE_IMAGE_CLICK, &SegmentPartitionEditor::OnSegmentVisToggle, this, XRCID("segmentTree"));
	frame.Bind(wxEVT_TREE_SEL_CHANGED, &SegmentPartitionEditor::OnSegmentSelect, this, XRCID("segmentTree"));
	frame.Bind(wxEVT_TREE_ITEM_RIGHT_CLICK, &SegmentPartitionEditor::OnSegmentContext, this, XRCID("segmentTree"));
	frame.Bind(wxEVT_COMMAND_RIGHT_CLICK, &SegmentPartitionEditor::OnSegmentTreeContext, this, XRCID("segmentTree"));
	frame.Bind(wxEVT_MENU, &SegmentPartitionEditor::OnAddSegment, this, XRCID("addSegment"));
	frame.Bind(wxEVT_MENU, &SegmentPartitionEditor::OnAddSubSegment, this, XRCID("addSubSegment"));
	frame.Bind(wxEVT_MENU, &SegmentPartitionEditor::OnDeleteSegment, this, XRCID("deleteSegment"));
	frame.Bind(wxEVT_MENU, &SegmentPartitionEditor::OnDeleteSubSegment, this, XRCID("deleteSubSegment"));
	frame.Bind(wxEVT_CHOICE, &SegmentPartitionEditor::OnSegmentSlotChanged, this, XRCID("segmentSlot"));
	frame.Bind(wxEVT_CHOICE, &SegmentPartitionEditor::OnSegmentTypeChanged, this, XRCID("segmentType"));
	frame.Bind(wxEVT_BUTTON, &SegmentPartitionEditor::OnSegmentApply, this, XRCID("segmentApply"));
	frame.Bind(wxEVT_BUTTON, &SegmentPartitionEditor::OnSegmentReset, this, XRCID("segmentReset"));
	frame.Bind(wxEVT_BUTTON, &SegmentPartitionEditor::OnSegmentEditSSF, this, XRCID("segmentSSFEdit"));
	frame.Bind(wxEVT_TREE_STATE_IMAGE_CLICK, &SegmentPartitionEditor::OnPartitionVisToggle, this, XRCID("partitionTree"));
	frame.Bind(wxEVT_TREE_SEL_CHANGED, &SegmentPartitionEditor::OnPartitionSelect, this, XRCID("partitionTree"));
	frame.Bind(wxEVT_TREE_ITEM_RIGHT_CLICK, &SegmentPartitionEditor::OnPartitionContext, this, XRCID("partitionTree"));
	frame.Bind(wxEVT_COMMAND_RIGHT_CLICK, &SegmentPartitionEditor::OnPartitionTreeContext, this, XRCID("partitionTree"));
	frame.Bind(wxEVT_MENU, &SegmentPartitionEditor::OnAddPartition, this, XRCID("addPartition"));
	frame.Bind(wxEVT_MENU, &SegmentPartitionEditor::OnDeletePartition, this, XRCID("deletePartition"));
	frame.Bind(wxEVT_CHOICE, &SegmentPartitionEditor::OnPartitionTypeChanged, this, XRCID("partitionType"));
	frame.Bind(wxEVT_BUTTON, &SegmentPartitionEditor::OnPartitionApply, this, XRCID("partitionApply"));
	frame.Bind(wxEVT_BUTTON, &SegmentPartitionEditor::OnPartitionReset, this, XRCID("partitionReset"));
}

void SegmentPartitionEditor::DeleteSelectedSegment() {
	wxCommandEvent evt;
	OnDeleteSegment(evt);
}

void SegmentPartitionEditor::DeleteSelectedPartition() {
	wxCommandEvent evt;
	OnDeletePartition(evt);
}

void SegmentPartitionEditor::ClearActive() {
	activePartition.Unset();
	activeSegment.Unset();
}

void SegmentPartitionEditor::OnSegmentSelect(wxTreeEvent& event) {
	ShowSegment(event.GetItem());
}

void SegmentPartitionEditor::OnSegmentVisToggle(wxTreeEvent& event) {
	wxTreeItemId item = event.GetItem();
	if (!item.IsOk() || !segmentTree->GetItemParent(item).IsOk()) {
		event.Skip();
		return;
	}

	int newState = segmentTree->GetItemState(item) == 1 ? 0 : 1;
	segmentTree->SetItemState(item, newState);

	if (segmentTree->GetItemParent(item) == segmentRoot) {
		wxTreeItemIdValue cookie;
		wxTreeItemId child = segmentTree->GetFirstChild(item, cookie);
		while (child.IsOk()) {
			segmentTree->SetItemState(child, newState);
			child = segmentTree->GetNextChild(item, cookie);
		}
	}
	else if (newState == 0) {
		wxTreeItemId parent = segmentTree->GetItemParent(item);
		if (parent.IsOk() && parent != segmentRoot)
			segmentTree->SetItemState(parent, 0);
	}

	Mesh* m = nullptr;
	if (frame.activeItem && frame.activeItem->GetShape())
		m = frame.glView->GetMesh(frame.activeItem->GetShape()->name.get());

	if (m) {
		if (m->subMeshes.empty())
			SetSubMeshesForPartitions(m, triSParts);

		ApplySegmentVisibility(m);
		frame.glView->Render();
	}

	event.Skip();
}

void SegmentPartitionEditor::OnSegmentContext(wxTreeEvent& event) {
	if (!event.GetItem().IsOk())
		return;

	segmentTree->SelectItem(event.GetItem());

	wxMenu* menu = nullptr;
	SubSegmentItemData* subSegmentData = dynamic_cast<SubSegmentItemData*>(segmentTree->GetItemData(event.GetItem()));
	if (subSegmentData)
		menu = wxXmlResource::Get()->LoadMenu("menuSubSegmentContext");
	else
		menu = wxXmlResource::Get()->LoadMenu("menuSegmentContext");

	if (menu) {
		frame.PopupMenu(menu);
		delete menu;
	}
}

void SegmentPartitionEditor::OnSegmentTreeContext(wxCommandEvent& WXUNUSED(event)) {
	wxMenu* menu = wxXmlResource::Get()->LoadMenu("menuSegmentTreeContext");
	if (menu) {
		frame.PopupMenu(menu);
		delete menu;
	}
}

int SegmentPartitionEditor::CalcMaxSegPartID() {
	int maxid = -1;
	wxTreeItemIdValue cookie;
	wxTreeItemId child = segmentTree->GetFirstChild(segmentRoot, cookie);
	while (child.IsOk()) {
		SegmentItemData* segmentData = dynamic_cast<SegmentItemData*>(segmentTree->GetItemData(child));
		if (segmentData)
			maxid = std::max(maxid, segmentData->partID);
		wxTreeItemIdValue subCookie;
		wxTreeItemId subChild = segmentTree->GetFirstChild(child, subCookie);
		while (subChild.IsOk()) {
			SubSegmentItemData* subSegmentData = dynamic_cast<SubSegmentItemData*>(segmentTree->GetItemData(subChild));
			if (subSegmentData)
				maxid = std::max(maxid, subSegmentData->partID);
			subChild = segmentTree->GetNextChild(child, subCookie);
		}
		child = segmentTree->GetNextChild(segmentRoot, cookie);
	}
	return maxid;
}

void SegmentPartitionEditor::OnAddSegment(wxCommandEvent& WXUNUSED(event)) {
	int newPartID = CalcMaxSegPartID() + 1;
	wxTreeItemId newItem;
	if (!activeSegment.IsOk() || segmentTree->GetChildrenCount(segmentRoot) <= 0) {
		// The new segment is the only partition: assign all triangles.
		for (size_t i = 0; i < triSParts.size(); ++i)
			triSParts[i] = newPartID;

		newItem = segmentTree->AppendItem(segmentRoot, "Segment", -1, -1, new SegmentItemData(newPartID));
	}
	else
		newItem = segmentTree->InsertItem(segmentRoot, activeSegment, "Segment", -1, -1, new SegmentItemData(newPartID));

	if (newItem.IsOk()) {
		segmentTree->SetItemState(newItem, 0);
		segmentTree->UnselectAll();
		segmentTree->SelectItem(newItem);
	}

	UpdateSegmentNames();
	frame.segmentTabButton->SetPendingChanges();
}

void SegmentPartitionEditor::OnAddSubSegment(wxCommandEvent& WXUNUSED(event)) {
	int newPartID = CalcMaxSegPartID() + 1;
	wxTreeItemId newItem;
	wxTreeItemId parent = segmentTree->GetItemParent(activeSegment);
	if (parent == segmentRoot) {
		if (segmentTree->GetChildrenCount(activeSegment) <= 0) {
			// The new subsegment will be the only child: assign all of
			// the segment's triangles to it.
			SegmentItemData* segmentData = dynamic_cast<SegmentItemData*>(segmentTree->GetItemData(activeSegment));
			if (segmentData)
				for (size_t i = 0; i < triSParts.size(); ++i)
					if (triSParts[i] == segmentData->partID)
						triSParts[i] = newPartID;
		}

		newItem = segmentTree->PrependItem(activeSegment, "Sub Segment", -1, -1, new SubSegmentItemData(newPartID, 0, 0xFFFFFFFF));
	}
	else
		newItem = segmentTree->InsertItem(parent, activeSegment, "Sub Segment", -1, -1, new SubSegmentItemData(newPartID, 0, 0xFFFFFFFF));

	if (newItem.IsOk()) {
		segmentTree->SetItemState(newItem, 0);
		segmentTree->UnselectAll();
		segmentTree->SelectItem(newItem);
	}

	UpdateSegmentNames();
	frame.segmentTabButton->SetPendingChanges();
}

void SegmentPartitionEditor::OnDeleteSegment(wxCommandEvent& WXUNUSED(event)) {
	SegmentItemData* segmentData = dynamic_cast<SegmentItemData*>(segmentTree->GetItemData(activeSegment));
	if (segmentData) {
		// Collect list of partition IDs that will be disappearing.
		std::vector<bool> oldPartIDs(CalcMaxSegPartID() + 1, false);
		oldPartIDs[segmentData->partID] = true;
		wxTreeItemIdValue cookie;
		wxTreeItemId child = segmentTree->GetFirstChild(activeSegment, cookie);
		while (child.IsOk()) {
			SubSegmentItemData* childData = dynamic_cast<SubSegmentItemData*>(segmentTree->GetItemData(child));
			if (childData)
				oldPartIDs[childData->partID] = true;
			child = segmentTree->GetNextChild(activeSegment, cookie);
		}

		// Find a new partition to put triangles into.
		int newPartID = -1;
		wxTreeItemId sibling = segmentTree->GetPrevSibling(activeSegment);
		if (!sibling.IsOk())
			sibling = segmentTree->GetNextSibling(activeSegment);
		if (sibling.IsOk()) {
			SegmentItemData* siblingData = dynamic_cast<SegmentItemData*>(segmentTree->GetItemData(sibling));
			if (siblingData)
				newPartID = siblingData->partID;

			child = segmentTree->GetFirstChild(sibling, cookie);
			if (child.IsOk()) {
				SubSegmentItemData* childData = dynamic_cast<SubSegmentItemData*>(segmentTree->GetItemData(child));
				if (childData)
					newPartID = childData->partID;
			}
		}

		// Assign triangles from old partitions to new partition.
		for (size_t i = 0; i < triSParts.size(); ++i)
			if (triSParts[i] >= 0 && triSParts[i] < static_cast<int>(oldPartIDs.size()) && oldPartIDs[triSParts[i]])
				triSParts[i] = newPartID;

		segmentTree->UnselectAll();
		segmentTree->Delete(activeSegment);
		if (sibling.IsOk())
			segmentTree->SelectItem(sibling);
		else
			activeSegment.Unset();
	}

	UpdateSegmentNames();
	frame.segmentTabButton->SetPendingChanges();
}

void SegmentPartitionEditor::OnDeleteSubSegment(wxCommandEvent& WXUNUSED(event)) {
	SubSegmentItemData* subSegmentData = dynamic_cast<SubSegmentItemData*>(segmentTree->GetItemData(activeSegment));
	wxTreeItemId newSelItem;
	if (subSegmentData) {
		int oldPartID = subSegmentData->partID, newPartID = -1;

		// Find a partition to assign triangles to.
		wxTreeItemId sibling = segmentTree->GetPrevSibling(activeSegment);
		if (!sibling.IsOk())
			sibling = segmentTree->GetNextSibling(activeSegment);

		if (sibling.IsOk()) {
			SubSegmentItemData* siblingData = dynamic_cast<SubSegmentItemData*>(segmentTree->GetItemData(sibling));
			if (siblingData)
				newPartID = siblingData->partID;
			newSelItem = sibling;
		}
		else {
			wxTreeItemId parent = segmentTree->GetItemParent(activeSegment);
			if (parent.IsOk()) {
				SegmentItemData* parentData = dynamic_cast<SegmentItemData*>(segmentTree->GetItemData(parent));
				if (parentData)
					newPartID = parentData->partID;
			}
			newSelItem = parent;
		}

		// Assign triangles to new partition.
		for (size_t i = 0; i < triSParts.size(); ++i)
			if (triSParts[i] == oldPartID)
				triSParts[i] = newPartID;

		segmentTree->UnselectAll();
		segmentTree->Delete(activeSegment);
		segmentTree->SelectItem(newSelItem);
	}

	UpdateSegmentNames();
	frame.segmentTabButton->SetPendingChanges();
}

void SegmentPartitionEditor::UpdateActiveSlotID() {
	wxChoice* segmentSlot = (wxChoice*)frame.FindWindowByName("segmentSlot");

	if (activeSegment.IsOk() && segmentTree->GetItemParent(activeSegment).IsOk()) {
		SubSegmentItemData* subSegmentData = dynamic_cast<SubSegmentItemData*>(segmentTree->GetItemData(activeSegment));
		if (subSegmentData) {
			subSegmentData->userSlotID = 0;

			if (segmentSlot->GetSelection() > 0) {
				wxString slotSel = segmentSlot->GetStringSelection().BeforeFirst(' ');
				if (slotSel.length() >= 1) {
					unsigned long slot = 0;
					slotSel.ToULong(&slot);

					if (slot > 0)
						subSegmentData->userSlotID = slot;
				}
			}

			UpdateSegmentNames();
			frame.segmentTabButton->SetPendingChanges();
		}
	}
}

void SegmentPartitionEditor::OnSegmentSlotChanged(wxCommandEvent& WXUNUSED(event)) {
	UpdateActiveSlotID();
}

void SegmentPartitionEditor::OnSegmentTypeChanged(wxCommandEvent& event) {
	wxChoice* segmentType = (wxChoice*)event.GetEventObject();

	if (activeSegment.IsOk() && segmentTree->GetItemParent(activeSegment).IsOk()) {
		SubSegmentItemData* subSegmentData = dynamic_cast<SubSegmentItemData*>(segmentTree->GetItemData(activeSegment));
		if (subSegmentData) {
			unsigned long type = 0xFFFFFFFF;

			wxString hashSel = segmentType->GetStringSelection();
			int hashPos = hashSel.First("0x");
			if (hashPos != wxNOT_FOUND)
				hashSel.Mid(hashPos).ToULong(&type, 16);

			subSegmentData->material = type;
			UpdateSegmentNames();
			frame.segmentTabButton->SetPendingChanges();
		}
	}
}

void SegmentPartitionEditor::OnSegmentApply(wxCommandEvent& WXUNUSED(event)) {
	ApplySegments();
}

void SegmentPartitionEditor::ApplySegments() {
	NifSegmentationInfo inf;

	wxTreeItemIdValue cookie;
	wxTreeItemId child = segmentTree->GetFirstChild(segmentRoot, cookie);

	while (child.IsOk()) {
		SegmentItemData* segmentData = dynamic_cast<SegmentItemData*>(segmentTree->GetItemData(child));
		if (segmentData) {
			inf.segs.emplace_back();
			NifSegmentInfo& seg = inf.segs.back();
			seg.partID = segmentData->partID;
			size_t childCount = segmentTree->GetChildrenCount(child);

			if (childCount > 0) {
				seg.subs.resize(childCount);
				int childInd = 0;

				wxTreeItemIdValue subCookie;
				wxTreeItemId subChild = segmentTree->GetFirstChild(child, subCookie);
				while (subChild.IsOk()) {
					SubSegmentItemData* subSegmentData = dynamic_cast<SubSegmentItemData*>(segmentTree->GetItemData(subChild));
					if (subSegmentData) {
						NifSubSegmentInfo& sub = seg.subs[childInd++];
						sub.partID = subSegmentData->partID;
						sub.userSlotID = subSegmentData->userSlotID;
						sub.material = subSegmentData->material;
						sub.extraData = subSegmentData->extraData;
					}

					subChild = segmentTree->GetNextChild(child, subCookie);
				}
			}
		}

		child = segmentTree->GetNextChild(segmentRoot, cookie);
	}

	wxTextCtrl* segmentSSF = (wxTextCtrl*)frame.FindWindowByName("segmentSSF");
	inf.ssfFile = segmentSSF->GetValue().ToStdString();

	frame.project->GetWorkNif()->SetShapeSegments(frame.activeItem->GetShape(), inf, triSParts);
	frame.MeshFromProj(frame.activeItem->GetShape());

	CreateSegmentTree(frame.activeItem->GetShape());
	frame.SetPendingChanges();
}

void SegmentPartitionEditor::OnSegmentReset(wxCommandEvent& WXUNUSED(event)) {
	ResetSegments();
}

void SegmentPartitionEditor::ResetSegments() {
	if (frame.activeItem)
		CreateSegmentTree(frame.activeItem->GetShape());
	else
		CreateSegmentTree(nullptr);
}

void SegmentPartitionEditor::OnSegmentEditSSF(wxCommandEvent& WXUNUSED(event)) {
	auto segmentSSF = (wxTextCtrl*)frame.FindWindowByName("segmentSSF");

	wxString result = GetTextFromUser(_("Please enter an SSF file path."), _("SSF File"), segmentSSF->GetValue(), &frame);
	if (result.empty())
		return;

	segmentSSF->ChangeValue(result);
	frame.segmentTabButton->SetPendingChanges();
}

void SegmentPartitionEditor::CreateSegmentTree(NiShape* shape) {
	if (segmentTree->GetChildrenCount(segmentRoot) > 0) {
		triSParts.clear(); // DeleteChildren calls OnSegmentSelect
		segmentTree->DeleteChildren(segmentRoot);
	}

	frame.segmentTabButton->SetPendingChanges(false);

	NifSegmentationInfo inf;
	if (frame.project->GetWorkNif()->GetShapeSegments(shape, inf, triSParts)) {
		for (size_t i = 0; i < inf.segs.size(); i++) {
			wxTreeItemId segID = segmentTree->AppendItem(segmentRoot, "Segment", -1, -1, new SegmentItemData(inf.segs[i].partID));
			if (segID.IsOk()) {
				segmentTree->SetItemState(segID, 0);
				for (size_t j = 0; j < inf.segs[i].subs.size(); j++) {
					NifSubSegmentInfo& sub = inf.segs[i].subs[j];
					wxTreeItemId subID = segmentTree->AppendItem(segID, "Sub Segment", -1, -1, new SubSegmentItemData(sub.partID, sub.userSlotID, sub.material, sub.extraData));
					segmentTree->SetItemState(subID, 0);
				}
			}
		}
	}

	wxTextCtrl* segmentSSF = (wxTextCtrl*)frame.FindWindowByName("segmentSSF");
	segmentSSF->ChangeValue(inf.ssfFile);

	UpdateSegmentNames();
	segmentTree->ExpandAll();

	wxTreeItemIdValue cookie;
	wxTreeItemId child = segmentTree->GetFirstChild(segmentRoot, cookie);
	if (child.IsOk())
		segmentTree->SelectItem(child);
}

void SegmentPartitionEditor::ApplySegmentVisibility(Mesh* m) {
	if (!m)
		return;

	if (m->subMeshesVisible.size() != m->subMeshes.size())
		m->subMeshesVisible.assign(m->subMeshes.size(), true);

	if (!segmentTree || !segmentRoot.IsOk())
		return;

	const auto setSubMeshVisible = [&](int partID, bool visible) {
		if (partID >= 0 && static_cast<size_t>(partID) < m->subMeshesVisible.size())
			m->subMeshesVisible[partID] = visible;
	};

	wxTreeItemIdValue cookie;
	wxTreeItemId child = segmentTree->GetFirstChild(segmentRoot, cookie);
	while (child.IsOk()) {
		SegmentItemData* segmentData = dynamic_cast<SegmentItemData*>(segmentTree->GetItemData(child));
		bool segmentVisible = segmentTree->GetItemState(child) != 1;
		if (segmentData)
			setSubMeshVisible(segmentData->partID, segmentVisible);

		wxTreeItemIdValue subCookie;
		wxTreeItemId subChild = segmentTree->GetFirstChild(child, subCookie);
		while (subChild.IsOk()) {
			SubSegmentItemData* subSegmentData = dynamic_cast<SubSegmentItemData*>(segmentTree->GetItemData(subChild));
			if (subSegmentData)
				setSubMeshVisible(subSegmentData->partID, segmentVisible && segmentTree->GetItemState(subChild) != 1);

			subChild = segmentTree->GetNextChild(child, subCookie);
		}

		child = segmentTree->GetNextChild(segmentRoot, cookie);
	}
}

void SegmentPartitionEditor::ResetSegmentVisibility() {
	if (segmentTree && segmentRoot.IsOk()) {
		wxTreeItemIdValue cookie;
		wxTreeItemId child = segmentTree->GetFirstChild(segmentRoot, cookie);
		while (child.IsOk()) {
			segmentTree->SetItemState(child, 0);

			wxTreeItemIdValue subCookie;
			wxTreeItemId subChild = segmentTree->GetFirstChild(child, subCookie);
			while (subChild.IsOk()) {
				segmentTree->SetItemState(subChild, 0);
				subChild = segmentTree->GetNextChild(child, subCookie);
			}

			child = segmentTree->GetNextChild(segmentRoot, cookie);
		}
	}

	if (frame.glView) {
		for (auto& m : frame.glView->gls.GetMeshes()) {
			if (m)
				m->subMeshesVisible.assign(m->subMeshes.size(), true);
		}
	}
}

bool SegmentPartitionEditor::PaintSegmentPartitionTriangles(Mesh* hitMesh, int hitTri, const Vector3& hitPointModel, float radiusModel) {
	if (!hitMesh || !frame.activeItem || !frame.glView->GetSegmentMode())
		return false;

	auto shape = frame.activeItem->GetShape();
	if (!shape || shape->name.get() != hitMesh->shapeName)
		return false;

	std::vector<Triangle> tris;
	shape->GetTriangles(tris);
	if (tris.empty() || hitTri < 0 || static_cast<size_t>(hitTri) >= tris.size())
		return false;

	std::vector<bool> paintTris(tris.size(), false);
	paintTris[hitTri] = true;

	float radiusMesh = hitMesh->TransformDistModelToMesh(radiusModel);
	if (radiusMesh > 0.0f) {
		Vector3 hitPointMesh = hitMesh->TransformPosModelToMesh(hitPointModel);
		for (size_t ti = 0; ti < tris.size(); ++ti) {
			const Triangle& t = tris[ti];
			Vector3 centroid = (hitMesh->verts[t.p1] + hitMesh->verts[t.p2] + hitMesh->verts[t.p3]) / 3.0f;
			if (centroid.DistanceTo(hitPointMesh) <= radiusMesh)
				paintTris[ti] = true;
		}
	}

	bool changed = false;

	if (triSParts.size() == tris.size() && activeSegment.IsOk() && segmentTree->GetItemParent(activeSegment).IsOk()) {
		std::vector<bool> selPartIDs(CalcMaxSegPartID() + 1, false);
		SubSegmentItemData* subSegmentData = dynamic_cast<SubSegmentItemData*>(segmentTree->GetItemData(activeSegment));
		if (subSegmentData) {
			for (size_t ti = 0; ti < tris.size(); ++ti) {
				if (paintTris[ti] && triSParts[ti] != subSegmentData->partID) {
					triSParts[ti] = subSegmentData->partID;
					changed = true;
				}
			}
		}
		else {
			SegmentItemData* segmentData = dynamic_cast<SegmentItemData*>(segmentTree->GetItemData(activeSegment));
			if (segmentData) {
				selPartIDs[segmentData->partID] = true;
				int destPartID = segmentData->partID;
				wxTreeItemIdValue subCookie;
				wxTreeItemId child = segmentTree->GetFirstChild(activeSegment, subCookie);
				while (child.IsOk()) {
					SubSegmentItemData* childData = dynamic_cast<SubSegmentItemData*>(segmentTree->GetItemData(child));
					if (childData) {
						selPartIDs[childData->partID] = true;
						destPartID = childData->partID;
					}
					child = segmentTree->GetNextChild(activeSegment, subCookie);
				}

				for (size_t ti = 0; ti < tris.size(); ++ti) {
					if (!paintTris[ti])
						continue;
					if (triSParts[ti] >= 0 && triSParts[ti] < static_cast<int>(selPartIDs.size()) && selPartIDs[triSParts[ti]])
						continue;
					if (triSParts[ti] != destPartID) {
						triSParts[ti] = destPartID;
						changed = true;
					}
				}
			}
		}
	}

	if (triParts.size() == tris.size() && activePartition.IsOk() && partitionTree->GetItemParent(activePartition).IsOk()) {
		PartitionItemData* partitionData = dynamic_cast<PartitionItemData*>(partitionTree->GetItemData(activePartition));
		if (partitionData) {
			for (size_t ti = 0; ti < tris.size(); ++ti) {
				if (paintTris[ti] && triParts[ti] != partitionData->index) {
					triParts[ti] = partitionData->index;
					changed = true;
				}
			}
		}
	}

	return changed;
}

bool SegmentPartitionEditor::GrowShrinkSegmentPartitionSelection(bool grow) {
	if (!frame.activeItem || !frame.glView->GetSegmentMode())
		return false;

	auto shape = frame.activeItem->GetShape();
	if (!shape)
		return false;

	std::vector<Triangle> tris;
	shape->GetTriangles(tris);
	if (tris.empty())
		return false;

	std::vector<std::vector<int>> triNeighbors(tris.size());
	std::unordered_map<uint64_t, int> edgeToTri;
	edgeToTri.reserve(tris.size() * 3);

	auto makeEdgeKey = [](int a, int b) -> uint64_t {
		uint32_t va = static_cast<uint32_t>(std::min(a, b));
		uint32_t vb = static_cast<uint32_t>(std::max(a, b));
		return (static_cast<uint64_t>(va) << 32) | vb;
	};

	auto connectEdge = [&](int triIndex, int v1, int v2) {
		uint64_t edgeKey = makeEdgeKey(v1, v2);
		auto it = edgeToTri.find(edgeKey);
		if (it == edgeToTri.end()) {
			edgeToTri.emplace(edgeKey, triIndex);
		}
		else {
			int otherTri = it->second;
			if (otherTri != triIndex) {
				triNeighbors[triIndex].push_back(otherTri);
				triNeighbors[otherTri].push_back(triIndex);
			}
		}
	};

	for (size_t ti = 0; ti < tris.size(); ++ti) {
		const Triangle& t = tris[ti];
		int triIndex = static_cast<int>(ti);
		connectEdge(triIndex, t.p1, t.p2);
		connectEdge(triIndex, t.p2, t.p3);
		connectEdge(triIndex, t.p3, t.p1);
	}

	bool changed = false;

	if (triSParts.size() == tris.size() && activeSegment.IsOk() && segmentTree->GetItemParent(activeSegment).IsOk()) {
		std::vector<bool> selPartIDs(CalcMaxSegPartID() + 1, false);
		int destPartID = -1;

		SubSegmentItemData* subSegmentData = dynamic_cast<SubSegmentItemData*>(segmentTree->GetItemData(activeSegment));
		if (subSegmentData) {
			destPartID = subSegmentData->partID;
			selPartIDs[destPartID] = true;
		}
		else {
			SegmentItemData* segmentData = dynamic_cast<SegmentItemData*>(segmentTree->GetItemData(activeSegment));
			if (segmentData) {
				selPartIDs[segmentData->partID] = true;
				destPartID = segmentData->partID;
				wxTreeItemIdValue subCookie;
				wxTreeItemId child = segmentTree->GetFirstChild(activeSegment, subCookie);
				while (child.IsOk()) {
					SubSegmentItemData* childData = dynamic_cast<SubSegmentItemData*>(segmentTree->GetItemData(child));
					if (childData) {
						selPartIDs[childData->partID] = true;
						destPartID = childData->partID;
					}
					child = segmentTree->GetNextChild(activeSegment, subCookie);
				}
			}
		}

		if (destPartID >= 0) {
			std::vector<bool> triSelected(tris.size(), false);
			for (size_t ti = 0; ti < tris.size(); ++ti) {
				int partID = triSParts[ti];
				if (partID >= 0 && partID < static_cast<int>(selPartIDs.size()) && selPartIDs[partID])
					triSelected[ti] = true;
			}

			if (grow) {
				for (size_t ti = 0; ti < tris.size(); ++ti) {
					if (triSelected[ti])
						continue;

					bool touchesSelection = false;
					for (int nTri : triNeighbors[ti]) {
						if (triSelected[nTri]) {
							touchesSelection = true;
							break;
						}
					}

					if (touchesSelection && triSParts[ti] != destPartID) {
						triSParts[ti] = destPartID;
						changed = true;
					}
				}
			}
			else {
				for (size_t ti = 0; ti < tris.size(); ++ti) {
					if (!triSelected[ti])
						continue;

					std::unordered_map<int, int> neighborPartCounts;
					bool boundary = false;
					for (int nTri : triNeighbors[ti]) {
						if (!triSelected[nTri]) {
							boundary = true;
							int nPartID = triSParts[nTri];
							if (nPartID >= 0)
								neighborPartCounts[nPartID]++;
						}
					}

					if (!boundary || neighborPartCounts.empty())
						continue;

					int bestPartID = triSParts[ti];
					int bestCount = -1;
					for (const auto& kv : neighborPartCounts) {
						if (kv.second > bestCount) {
							bestPartID = kv.first;
							bestCount = kv.second;
						}
					}

					if (bestPartID != triSParts[ti]) {
						triSParts[ti] = bestPartID;
						changed = true;
					}
				}
			}
		}
	}

	if (triParts.size() == tris.size() && activePartition.IsOk() && partitionTree->GetItemParent(activePartition).IsOk()) {
		PartitionItemData* partitionData = dynamic_cast<PartitionItemData*>(partitionTree->GetItemData(activePartition));
		if (partitionData) {
			const int targetIndex = partitionData->index;
			std::vector<bool> triSelected(tris.size(), false);
			for (size_t ti = 0; ti < tris.size(); ++ti)
				triSelected[ti] = triParts[ti] == targetIndex;

			if (grow) {
				for (size_t ti = 0; ti < tris.size(); ++ti) {
					if (triSelected[ti])
						continue;

					bool touchesSelection = false;
					for (int nTri : triNeighbors[ti]) {
						if (triSelected[nTri]) {
							touchesSelection = true;
							break;
						}
					}

					if (touchesSelection && triParts[ti] != targetIndex) {
						triParts[ti] = targetIndex;
						changed = true;
					}
				}
			}
			else {
				for (size_t ti = 0; ti < tris.size(); ++ti) {
					if (!triSelected[ti])
						continue;

					std::unordered_map<int, int> neighborPartCounts;
					bool boundary = false;
					for (int nTri : triNeighbors[ti]) {
						if (!triSelected[nTri]) {
							boundary = true;
							int nPartID = triParts[nTri];
							if (nPartID >= 0)
								neighborPartCounts[nPartID]++;
						}
					}

					if (!boundary || neighborPartCounts.empty())
						continue;

					int bestPartID = triParts[ti];
					int bestCount = -1;
					for (const auto& kv : neighborPartCounts) {
						if (kv.second > bestCount) {
							bestPartID = kv.first;
							bestCount = kv.second;
						}
					}

					if (bestPartID != triParts[ti]) {
						triParts[ti] = bestPartID;
						changed = true;
					}
				}
			}
		}
	}

	if (changed && frame.currentTabButton)
		frame.currentTabButton->SetPendingChanges();

	return changed;
}

void SegmentPartitionEditor::ShowSegment(const wxTreeItemId& item) {
	if (!frame.activeItem || !frame.glView->GetSegmentMode())
		return;

	wxChoice* segmentType = (wxChoice*)frame.FindWindowByName("segmentType");
	segmentType->Disable();
	segmentType->SetSelection(0);

	wxChoice* segmentSlot = (wxChoice*)frame.FindWindowByName("segmentSlot");
	segmentSlot->Disable();
	segmentSlot->SetSelection(0);

	if (item.IsOk())
		activeSegment = item;

	if (!activeSegment.IsOk() || !segmentTree->GetItemParent(activeSegment).IsOk())
		return;

	// Get all triangles of the active shape
	std::vector<Triangle> tris;
	if (frame.activeItem->GetShape())
		frame.activeItem->GetShape()->GetTriangles(tris);
	if (triSParts.size() != tris.size())
		return;

	// selPartIDs will be true for selected item and its children.
	std::vector<bool> selPartIDs(CalcMaxSegPartID() + 1, false);

	SubSegmentItemData* subSegmentData = dynamic_cast<SubSegmentItemData*>(segmentTree->GetItemData(activeSegment));
	if (subSegmentData) {
		// Active segment is a subsegment
		selPartIDs[subSegmentData->partID] = true;

		if (subSegmentData->material != 0xFFFFFFFF) {
			bool typeFound = false;
			auto typeHash = wxString::Format("0x%08x", subSegmentData->material);
			for (uint32_t i = 0; i < segmentType->GetCount(); i++) {
				auto typeString = segmentType->GetString(i);
				if (typeString.Contains(typeHash)) {
					segmentType->SetSelection((int)i);
					typeFound = true;
					break;
				}
			}

			if (!typeFound)
				segmentType->SetSelection(segmentType->Append(typeHash));
		}

		segmentType->Enable();

		for (uint32_t i = 0; i < segmentSlot->GetCount(); i++) {
			uint32_t userSlotID = subSegmentData->userSlotID;

			// Find matching slot in choice
			wxString slotPrefix = wxString::Format("%d - ", userSlotID);
			auto slotString = segmentSlot->GetString(i);
			if (slotString.StartsWith(slotPrefix)) {
				segmentSlot->SetSelection((int)i);
				break;
			}
		}

		segmentSlot->Enable();
	}
	else {
		SegmentItemData* segmentData = dynamic_cast<SegmentItemData*>(segmentTree->GetItemData(activeSegment));
		if (segmentData) {
			// Active segment is a normal segment
			// Collect list of partition IDs for segment and children.
			selPartIDs[segmentData->partID] = true;
			wxTreeItemIdValue subCookie;
			wxTreeItemId child = segmentTree->GetFirstChild(activeSegment, subCookie);
			while (child.IsOk()) {
				SubSegmentItemData* childData = dynamic_cast<SubSegmentItemData*>(segmentTree->GetItemData(child));
				if (childData)
					selPartIDs[childData->partID] = true;
				child = segmentTree->GetNextChild(activeSegment, subCookie);
			}
		}
	}

	// Display segmentation colors depending on what is selected
	Mesh* m = frame.glView->GetMesh(frame.activeItem->GetShape()->name.get());
	if (m) {
		SetSubMeshesForPartitions(m, triSParts);
		ApplySegmentVisibility(m);

		// Set colors for segments
		int nsm = m->subMeshes.size();
		m->subMeshesColor.resize(nsm);
		for (int pi = 0; pi < nsm; ++pi) {
			if (selPartIDs[pi]) {
				m->subMeshesColor[pi].x = 1.0f;
				m->subMeshesColor[pi].y = 0.0f;
				m->subMeshesColor[pi].z = 0.0f;
			}
			else {
				float colorValue = (pi + 1.0f) / (nsm + 1);
				m->subMeshesColor[pi] = frame.glView->CreateColorRamp(colorValue);
			}
		}
	}

	frame.glView->Render();
}

void SegmentPartitionEditor::UpdateSegmentNames() {
	auto segmentType = (wxChoice*)frame.FindWindowByName("segmentType");
	auto segmentSlot = (wxChoice*)frame.FindWindowByName("segmentSlot");

	int segmentIndex = 0;
	wxTreeItemIdValue cookie;
	wxTreeItemId child = segmentTree->GetFirstChild(segmentRoot, cookie);

	while (child.IsOk()) {
		segmentTree->SetItemText(child, wxString::Format("Segment #%d", segmentIndex));

		int subSegmentIndex = 0;
		wxTreeItemIdValue subCookie;
		wxTreeItemId subChild = segmentTree->GetFirstChild(child, subCookie);

		while (subChild.IsOk()) {
			std::string subSegmentName = "Default";
			std::string subSegmentSlot = "";

			auto subSegmentData = dynamic_cast<SubSegmentItemData*>(segmentTree->GetItemData(subChild));
			if (subSegmentData) {
				if (subSegmentData->material != 0xFFFFFFFF) {
					bool typeFound = false;
					auto typeHash = wxString::Format("0x%08x", subSegmentData->material);
					for (uint32_t i = 0; i < segmentType->GetCount(); i++) {
						auto typeString = segmentType->GetString(i);
						if (typeString.Contains(typeHash)) {
							subSegmentName = typeString.BeforeLast('|');
							typeFound = true;
							break;
						}
					}

					if (!typeFound)
						subSegmentName = typeHash;
				}

				uint32_t userSlotID = subSegmentData->userSlotID;
				if (userSlotID >= 30) {
					wxString slotPrefix = wxString::Format("%d - ", userSlotID);

					// Find matching slot in choice
					for (uint32_t i = 0; i < segmentSlot->GetCount(); i++) {
						auto slotString = segmentSlot->GetString(i);
						if (slotString.StartsWith(slotPrefix)) {
							subSegmentSlot = wxString::Format("(Slot %s)", slotString);
							break;
						}
					}
				}
			}

			segmentTree->SetItemText(subChild, wxString::Format("#%d: %s%s", subSegmentIndex, subSegmentName, subSegmentSlot));

			subChild = segmentTree->GetNextChild(child, subCookie);
			subSegmentIndex++;
		}

		child = segmentTree->GetNextChild(segmentRoot, cookie);
		segmentIndex++;
	}
}

void SegmentPartitionEditor::OnPartitionSelect(wxTreeEvent& event) {
	ShowPartition(event.GetItem());
}

void SegmentPartitionEditor::OnPartitionVisToggle(wxTreeEvent& event) {
	wxTreeItemId item = event.GetItem();
	if (!item.IsOk() || !partitionTree->GetItemParent(item).IsOk()) {
		event.Skip();
		return;
	}

	partitionTree->SetItemState(item, partitionTree->GetItemState(item) == 1 ? 0 : 1);

	Mesh* m = nullptr;
	if (frame.activeItem && frame.activeItem->GetShape())
		m = frame.glView->GetMesh(frame.activeItem->GetShape()->name.get());

	if (m) {
		if (m->subMeshes.empty())
			SetSubMeshesForPartitions(m, triParts);

		ApplyPartitionVisibility(m);
		frame.glView->Render();
	}

	event.Skip();
}

void SegmentPartitionEditor::OnPartitionContext(wxTreeEvent& event) {
	if (!event.GetItem().IsOk())
		return;

	partitionTree->SelectItem(event.GetItem());

	wxMenu* menu = nullptr;
	PartitionItemData* partitionData = dynamic_cast<PartitionItemData*>(partitionTree->GetItemData(event.GetItem()));
	if (partitionData)
		menu = wxXmlResource::Get()->LoadMenu("menuPartitionContext");

	if (menu) {
		frame.PopupMenu(menu);
		delete menu;
	}
}

void SegmentPartitionEditor::OnPartitionTreeContext(wxCommandEvent& WXUNUSED(event)) {
	wxMenu* menu = wxXmlResource::Get()->LoadMenu("menuPartitionTreeContext");
	if (menu) {
		frame.PopupMenu(menu);
		delete menu;
	}
}

void SegmentPartitionEditor::OnAddPartition(wxCommandEvent& WXUNUSED(event)) {
	bool isSkyrim = wxGetApp().targetGame == SKYRIM || wxGetApp().targetGame == SKYRIMSE || wxGetApp().targetGame == SKYRIMVR;

	// Find an unused partition index
	std::set<int> partInds;
	wxTreeItemIdValue cookie;
	wxTreeItemId child = partitionTree->GetFirstChild(partitionRoot, cookie);
	while (child.IsOk()) {
		PartitionItemData* partitionData = dynamic_cast<PartitionItemData*>(partitionTree->GetItemData(child));
		if (partitionData)
			partInds.insert(partitionData->index);
		child = partitionTree->GetNextChild(partitionRoot, cookie);
	}

	int partInd = 0;
	while (partInds.count(partInd) != 0)
		++partInd;

	// Create partition item
	wxTreeItemId newItem;
	if (!activePartition.IsOk() || partitionTree->GetChildrenCount(partitionRoot) <= 0) {
		auto shape = frame.activeItem->GetShape();
		if (shape && shape->GetNumVertices() > 0) {
			newItem = partitionTree->AppendItem(partitionRoot, "Partition", -1, -1, new PartitionItemData(partInd, isSkyrim ? 32 : 0));
		}

		for (int& pi : triParts)
			pi = partInd;
	}
	else
		newItem = partitionTree->InsertItem(partitionRoot, activePartition, "Partition", -1, -1, new PartitionItemData(partInd, isSkyrim ? 32 : 0));

	if (newItem.IsOk()) {
		partitionTree->SetItemState(newItem, 0);
		partitionTree->UnselectAll();
		partitionTree->SelectItem(newItem);
	}

	UpdatePartitionNames();
	frame.partitionTabButton->SetPendingChanges();
}

void SegmentPartitionEditor::OnDeletePartition(wxCommandEvent& WXUNUSED(event)) {
	PartitionItemData* partitionData = dynamic_cast<PartitionItemData*>(partitionTree->GetItemData(activePartition));
	if (partitionData) {
		wxTreeItemId sibling = partitionTree->GetPrevSibling(activePartition);
		if (!sibling.IsOk())
			sibling = partitionTree->GetNextSibling(activePartition);

		int newIndex = -1;
		if (sibling.IsOk()) {
			PartitionItemData* siblingData = dynamic_cast<PartitionItemData*>(partitionTree->GetItemData(sibling));
			if (siblingData)
				newIndex = siblingData->index;
		}
		for (size_t i = 0; i < triParts.size(); ++i)
			if (triParts[i] == partitionData->index)
				triParts[i] = newIndex;

		if (sibling.IsOk()) {
			partitionTree->UnselectAll();
			partitionTree->Delete(activePartition);
			partitionTree->SelectItem(sibling);

			ShowPartition(sibling);
		}
		else {
			partitionTree->Delete(activePartition);
			activePartition.Unset();
		}
	}

	UpdatePartitionNames();
	frame.partitionTabButton->SetPendingChanges();
}

void SegmentPartitionEditor::OnPartitionTypeChanged(wxCommandEvent& event) {
	wxChoice* partitionType = (wxChoice*)event.GetEventObject();

	if (activePartition.IsOk() && partitionTree->GetItemParent(activePartition).IsOk()) {
		PartitionItemData* partitionData = dynamic_cast<PartitionItemData*>(partitionTree->GetItemData(activePartition));
		if (partitionData) {
			unsigned long type = 0;
			partitionType->GetStringSelection().ToULong(&type);
			partitionData->type = type;
		}
	}

	UpdatePartitionNames();
	frame.partitionTabButton->SetPendingChanges();
}

void SegmentPartitionEditor::OnPartitionApply(wxCommandEvent& WXUNUSED(event)) {
	ApplyPartitions();
}

void SegmentPartitionEditor::ApplyPartitions() {
	auto shape = frame.activeItem->GetShape();
	if (!shape)
		return;

	NiVector<BSDismemberSkinInstance::PartitionInfo> partitionInfo;
	std::vector<bool> delPartFlags;

	wxTreeItemIdValue cookie;
	wxTreeItemId child = partitionTree->GetFirstChild(partitionRoot, cookie);

	while (child.IsOk()) {
		PartitionItemData* partitionData = dynamic_cast<PartitionItemData*>(partitionTree->GetItemData(child));
		if (partitionData) {
			const int index = partitionData->index;
			if (index >= static_cast<int64_t>(partitionInfo.size())) {
				partitionInfo.resize(index + 1);
				delPartFlags.resize(index + 1, true);
			}
			partitionInfo[index].flags = PF_EDITOR_VISIBLE;
			partitionInfo[index].partID = partitionData->type;
			delPartFlags[index] = false;
		}

		child = partitionTree->GetNextChild(partitionRoot, cookie);
	}

	std::vector<uint32_t> delPartInds;
	for (uint32_t pi = 0; pi < delPartFlags.size(); ++pi)
		if (delPartFlags[pi])
			delPartInds.push_back(pi);

	frame.project->GetWorkNif()->SetShapePartitions(shape, partitionInfo, triParts);
	if (!delPartInds.empty())
		frame.project->GetWorkNif()->DeletePartitions(shape, delPartInds);

	CreatePartitionTree(shape);
	frame.SetPendingChanges();
}

void SegmentPartitionEditor::OnPartitionReset(wxCommandEvent& WXUNUSED(event)) {
	ResetPartitions();
}

void SegmentPartitionEditor::ResetPartitions() {
	if (frame.activeItem)
		CreatePartitionTree(frame.activeItem->GetShape());
	else
		CreatePartitionTree(nullptr);
}

void SegmentPartitionEditor::RefreshActivePartitionTree() {
	if (frame.activeItem)
		CreatePartitionTree(frame.activeItem->GetShape());
	else
		CreatePartitionTree(nullptr);
}

void SegmentPartitionEditor::RefreshActiveSegmentTree() {
	if (frame.activeItem)
		CreateSegmentTree(frame.activeItem->GetShape());
	else
		CreateSegmentTree(nullptr);
}

void SegmentPartitionEditor::CreatePartitionTree(NiShape* shape) {
	if (partitionTree->GetChildrenCount(partitionRoot) > 0) {
		triParts.clear(); // DeleteChildren calls OnPartitionSelect
		partitionTree->DeleteChildren(partitionRoot);
	}

	frame.partitionTabButton->SetPendingChanges(false);

	NiVector<BSDismemberSkinInstance::PartitionInfo> partitionInfo;
	if (frame.project->GetWorkNif()->GetShapePartitions(shape, partitionInfo, triParts)) {
		for (uint32_t i = 0; i < partitionInfo.size(); i++) {
			wxTreeItemId item = partitionTree->AppendItem(partitionRoot, "Partition", -1, -1, new PartitionItemData(static_cast<int>(i), partitionInfo[i].partID));
			partitionTree->SetItemState(item, 0);
		}
	}

	UpdatePartitionNames();
	partitionTree->ExpandAll();

	wxTreeItemIdValue cookie;
	wxTreeItemId child = partitionTree->GetFirstChild(partitionRoot, cookie);
	if (child.IsOk())
		partitionTree->SelectItem(child);
}

void SegmentPartitionEditor::ApplyPartitionVisibility(Mesh* m) {
	if (!m)
		return;

	if (m->subMeshesVisible.size() != m->subMeshes.size())
		m->subMeshesVisible.assign(m->subMeshes.size(), true);

	if (!partitionTree || !partitionRoot.IsOk())
		return;

	wxTreeItemIdValue cookie;
	wxTreeItemId child = partitionTree->GetFirstChild(partitionRoot, cookie);
	while (child.IsOk()) {
		PartitionItemData* partitionData = dynamic_cast<PartitionItemData*>(partitionTree->GetItemData(child));
		if (partitionData && partitionData->index >= 0 && static_cast<size_t>(partitionData->index) < m->subMeshesVisible.size())
			m->subMeshesVisible[partitionData->index] = partitionTree->GetItemState(child) != 1;

		child = partitionTree->GetNextChild(partitionRoot, cookie);
	}
}

void SegmentPartitionEditor::ResetPartitionVisibility() {
	if (partitionTree && partitionRoot.IsOk()) {
		wxTreeItemIdValue cookie;
		wxTreeItemId child = partitionTree->GetFirstChild(partitionRoot, cookie);
		while (child.IsOk()) {
			partitionTree->SetItemState(child, 0);
			child = partitionTree->GetNextChild(partitionRoot, cookie);
		}
	}

	if (frame.glView) {
		for (auto& m : frame.glView->gls.GetMeshes()) {
			if (m)
				m->subMeshesVisible.assign(m->subMeshes.size(), true);
		}
	}
}

void SegmentPartitionEditor::ShowPartition(const wxTreeItemId& item) {
	if (!frame.activeItem || !frame.glView->GetSegmentMode())
		return;

	auto shape = frame.activeItem->GetShape();
	if (!shape)
		return;

	wxChoice* partitionType = (wxChoice*)frame.FindWindowByName("partitionType");
	partitionType->Disable();
	partitionType->SetSelection(0);
	wxArrayString partitionStrings = partitionType->GetStrings();

	if (item.IsOk())
		activePartition = item;

	if (!activePartition.IsOk() || !partitionTree->GetItemParent(activePartition).IsOk())
		return;

	// Get all triangles of the active shape
	std::vector<Triangle> allTris;
	shape->GetTriangles(allTris);
	if (allTris.size() != triParts.size())
		return;

	PartitionItemData* partitionData = dynamic_cast<PartitionItemData*>(partitionTree->GetItemData(activePartition));
	if (partitionData) {
		for (auto& s : partitionStrings) {
			if (s.StartsWith(wxString::Format("%d", partitionData->type))) {
				// Show correct data in UI
				partitionType->Enable();
				partitionType->SetStringSelection(s);
			}
		}
	}

	// Display partition colors depending on what is selected
	Mesh* m = frame.glView->GetMesh(frame.activeItem->GetShape()->name.get());
	if (m) {
		SetSubMeshesForPartitions(m, triParts);
		ApplyPartitionVisibility(m);

		// Set colors for non-selected partitions
		int nsm = m->subMeshes.size();
		m->subMeshesColor.resize(nsm);

		for (int pi = 0; pi < nsm; ++pi) {
			float colorValue = (pi + 1.0f) / (nsm + 1);
			m->subMeshesColor[pi] = frame.glView->CreateColorRamp(colorValue);
		}

		// Set color for selected partition
		if (partitionData) {
			if (nsm > partitionData->index) {
				m->subMeshesColor[partitionData->index].x = 1.0f;
				m->subMeshesColor[partitionData->index].y = 0.0f;
				m->subMeshesColor[partitionData->index].z = 0.0f;
			}
		}
	}

	frame.glView->Render();
}

void SegmentPartitionEditor::UpdatePartitionNames() {
	wxChoice* partitionType = (wxChoice*)frame.FindWindowByName("partitionType");
	wxArrayString partitionStrings = partitionType->GetStrings();

	wxTreeItemIdValue cookie;
	wxTreeItemId child = partitionTree->GetFirstChild(partitionRoot, cookie);

	while (child.IsOk()) {
		PartitionItemData* partitionData = dynamic_cast<PartitionItemData*>(partitionTree->GetItemData(child));
		if (partitionData) {
			bool found = false;
			for (auto& s : partitionStrings) {
				if (s.StartsWith(wxString::Format("%d", partitionData->type))) {
					partitionTree->SetItemText(child, s);
					found = true;
					break;
				}
			}

			if (!found)
				partitionTree->SetItemText(child, wxString::Format("%d Unknown", partitionData->type));
		}

		child = partitionTree->GetNextChild(partitionRoot, cookie);
	}
}

void SegmentPartitionEditor::SetSubMeshesForPartitions(Mesh* m, const std::vector<int>& tp) {
	uint32_t nTris = static_cast<uint32_t>(tp.size());

	// Sort triangles (via triInds) by partition number, negative partition
	// numbers at the end.
	std::vector<uint32_t> triInds(nTris);
	for (uint32_t ti = 0; ti < nTris; ++ti)
		triInds[ti] = ti;

	std::stable_sort(triInds.begin(), triInds.end(), [&tp](int i, int j) { return tp[j] < 0 || tp[i] < tp[j]; });

	// Re-order triangles
	for (uint32_t ti = 0; ti < nTris; ++ti)
		m->renderTris[ti] = m->tris[triInds[ti]];

	// Find first triangle of each sub-mesh.
	m->subMeshes.clear();
	m->subMeshesColor.clear();
	m->subMeshesVisible.clear();
	m->triSubMeshes.assign(nTris, -1);
	int negativeSubMeshIndex = -1;

	for (uint32_t ti = 0; ti < nTris; ++ti) {
		while (tp[triInds[ti]] >= static_cast<int>(m->subMeshes.size()))
			m->subMeshes.emplace_back(ti, 0);

		if (tp[triInds[ti]] < 0) {
			negativeSubMeshIndex = static_cast<int>(m->subMeshes.size());
			m->subMeshes.emplace_back(ti, 0);
			break;
		}
	}

	// Calculate size of each sub-mesh.
	m->subMeshes.emplace_back(nTris, 0);
	for (size_t si = 0; si + 1 < m->subMeshes.size(); ++si)
		m->subMeshes[si].second = m->subMeshes[si + 1].first - m->subMeshes[si].first;

	m->subMeshes.pop_back();
	m->subMeshesVisible.assign(m->subMeshes.size(), true);

	for (uint32_t ti = 0; ti < nTris; ++ti) {
		int subMeshIndex = tp[ti] >= 0 ? tp[ti] : negativeSubMeshIndex;
		if (subMeshIndex >= 0 && static_cast<size_t>(subMeshIndex) < m->subMeshes.size())
			m->triSubMeshes[ti] = subMeshIndex;
	}

	m->QueueUpdate(Mesh::UpdateType::Indices);
}

void SegmentPartitionEditor::SetNoSubMeshes(Mesh* m) {
	if (!m)
		return;

	m->subMeshes.clear();
	m->subMeshesColor.clear();
	m->subMeshesVisible.clear();
	m->triSubMeshes.clear();

	for (int ti = 0; ti < m->nTris; ++ti)
		m->renderTris[ti] = m->tris[ti];

	m->QueueUpdate(Mesh::UpdateType::Indices);
}

void SegmentPartitionEditor::SetNoSubMeshes() {
	if (!frame.activeItem)
		return;

	SetNoSubMeshes(frame.glView->GetMesh(frame.activeItem->GetShape()->name.get()));
}

void SegmentPartitionEditor::OnCopySegPart(wxCommandEvent& WXUNUSED(event)) {
	if (!frame.ShapeSelectionCheck())
		return;

	if (!frame.project->GetBaseShape()) {
		wxMessageBox(_("There is no reference shape!"), _("Error"));
		return;
	}

	std::vector<NiShape*> selectedShapes;
	for (auto& s : frame.selectedItems) {
		if (auto shape = s->GetShape(); !frame.project->IsBaseShape(shape))
			selectedShapes.push_back(s->GetShape());
		else
			wxMessageBox(_("Sorry, you can't copy partitions/segments from the reference shape to itself. Skipping this shape."), _("Can't copy segments/partitions"), wxICON_WARNING);
	}
	if (selectedShapes.empty())
		return;

	if (wxMessageBox(_("Triangles will be assigned to the partition/segment of the nearest triangle in the reference.  Existing partitions/segments are cleared.  This action can't be undone."), _("Copy Partitions/Segments"), wxOK | wxCANCEL | wxICON_INFORMATION | wxOK_DEFAULT) != wxOK)
		return;

	CopySegPartForShapes(selectedShapes);
}

int SegmentPartitionEditor::CopySegPartForShapes(std::vector<NiShape*> shapes, bool silent) {
	if (shapes.empty())
		return 0;

	int failshapes = 0;

	frame.StartProgress(_("Copying segments/partitions..."));

	const int inc = 100 / shapes.size() - 1;

	for (size_t i = 0; i < shapes.size(); i++) {
		NiShape* shape = shapes[i];
		wxLogMessage("Copying segments/partitions to '%s'...", shape->name.get());
		frame.StartSubProgress(i * inc, i * inc + inc);

		int failcount = frame.project->CopySegPart(shape);
		if (failcount && !silent)
			wxMessageBox(wxString::Format(_("The partitions/segments could not be copied for '%s' because %d triangles could not be matched."), shape->name.get(), failcount), _("Error"));
		if (failcount)
			++failshapes;

		if (!failcount) {
			frame.MeshFromProj(shape);

			if (shape == frame.activeItem->GetShape()) {
				CreateSegmentTree(shape);
				CreatePartitionTree(shape);
			}
		}

		frame.EndProgress();
	}

	frame.SetPendingChanges();
	frame.UpdateUndoTools();

	frame.EndProgress();

	return failshapes;
}
