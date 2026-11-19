/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

// Outfit Studio window: the shape list, shape selection and the meshes shown for the shapes

#include "OutfitStudioFrame.h"
#include "SegmentPartitionEditor.h"
#include "ui/outfitstudio/EditUV.h"
#include "OutfitStudioApp.h"
#include "GLPanel.h"
#include "ShapeTreeItems.h"
#include "ViewState.h"
#include "utils/ConfigurationManager.h"
#include "components/UndoHistory.h"

#include <cmath>
#include <cstdlib>
#include <iterator>
#include <sstream>

using namespace nifly;

void OutfitStudioFrame::UpdateActiveShape() {
	bool smoothSeamNormals = true;
	bool lockNormals = false;
	bool enableSmoothSeamsAngle = true;

	if (!activeItem) {
		if (glView->GetTransformMode())
			glView->ShowTransformTool(false);
		if (glView->GetVertexEdit())
			glView->ShowVertexEdit(false);

		segPartEditor->CreateSegmentTree();
		segPartEditor->CreatePartitionTree();
		outfitBones->UnselectAll();
		editor.lastSelectedBones.clear();

		menuBar->Enable(XRCID("btnSmoothSeams"), false);
		enableSmoothSeamsAngle = false;
		menuBar->Enable(XRCID("btnLockNormals"), false);
	}
	else {
		Mesh* m = glView->GetMesh(activeItem->GetShape()->name.get());
		if (m) {
			smoothSeamNormals = m->smoothSeamNormals;
			lockNormals = m->lockNormals;

			if (glView->GetTransformMode())
				glView->ShowTransformTool();
			if (glView->GetVertexEdit())
				glView->ShowVertexEdit();
		}
		else
			enableSmoothSeamsAngle = false;

		menuBar->Enable(XRCID("btnSmoothSeams"), m != nullptr);
		menuBar->Enable(XRCID("btnSmoothSeamsAngle"), m != nullptr);
		menuBar->Enable(XRCID("btnLockNormals"), m != nullptr);

		segPartEditor->CreateSegmentTree(activeItem->GetShape());
		segPartEditor->CreatePartitionTree(activeItem->GetShape());
	}

	if (!smoothSeamNormals)
		enableSmoothSeamsAngle = false;

	menuBar->Check(XRCID("btnSmoothSeams"), smoothSeamNormals);
	menuBar->Enable(XRCID("btnSmoothSeamsAngle"), enableSmoothSeamsAngle);
	menuBar->Check(XRCID("btnLockNormals"), lockNormals);

	if (autoFrameSelected)
		FrameSelected();
	else if (glView->rotationCenterMode == RotationCenterMode::MeshCenter)
		glView->gls.camRotOffset = glView->gls.GetActiveCenter();

	glView->UpdateBones();
	glView->Render();

	HighlightSliderData();
	HighlightBoneNamesWithWeights();
	UpdateBoneCounts();
}

void OutfitStudioFrame::UpdateVertexColors() {
	bool enableVertexColors = menuBar->IsChecked(XRCID("btnEnableVertexColors"));
	glView->SetColorsVisible(enableVertexColors);
	if (enableVertexColors)
		FillVertexColors();
	else if (colorSettings->IsShown())
		glView->ClearColors();
}

void OutfitStudioFrame::SelectShape(const std::string& shapeName) {
	if (activeItem && activeItem->GetShape()->name == shapeName)
		return;

	wxTreeItemId item;
	wxTreeItemId subitem;
	wxTreeItemIdValue cookie;
	wxTreeItemIdValue subcookie;
	item = outfitShapes->GetFirstChild(shapesRoot, cookie);
	while (item.IsOk()) {
		subitem = outfitShapes->GetFirstChild(item, subcookie);
		while (subitem.IsOk()) {
			if (outfitShapes->GetItemText(subitem) == shapeName) {
				outfitShapes->UnselectAll();
				outfitShapes->SelectItem(subitem);
				outfitShapes->EnsureVisible(subitem);
				return;
			}
			subitem = outfitShapes->GetNextChild(item, subcookie);
		}
		item = outfitShapes->GetNextSibling(item);
	}
}

namespace {

wxTreeItemId FindShapeItem(wxTreeCtrl* tree, const wxTreeItemId& root, const std::string& shapeName) {
	if (!root.IsOk())
		return wxTreeItemId();

	wxTreeItemIdValue cookie;
	for (wxTreeItemId child = tree->GetFirstChild(root, cookie); child.IsOk(); child = tree->GetNextChild(root, cookie))
		if (tree->GetItemText(child).ToUTF8().data() == shapeName)
			return child;

	return wxTreeItemId();
}

} // namespace

bool OutfitStudioFrame::SelectShapes(const std::vector<std::string>& shapeNames) {
	std::vector<wxTreeItemId> items;
	for (auto& shapeName : shapeNames) {
		wxTreeItemId item = FindShapeItem(outfitShapes, outfitRoot, shapeName);
		if (!item.IsOk())
			return false;
		items.push_back(item);
	}

	outfitShapes->UnselectAll();

	// The item selected last becomes the active shape
	for (size_t i = items.size(); i > 1; i--)
		outfitShapes->SelectItem(items[i - 1]);

	if (!items.empty()) {
		outfitShapes->SelectItem(items[0]);
		outfitShapes->EnsureVisible(items[0]);
	}

	return true;
}

std::vector<std::string> OutfitStudioFrame::GetSelectedShapeNames() {
	std::vector<std::string> names;
	for (auto& item : selectedItems)
		names.push_back(item->GetShape()->name.get());
	return names;
}

bool OutfitStudioFrame::GetShapeDisplayMode(const std::string& shapeName, ShapeDisplayMode& outMode) {
	wxTreeItemId item = FindShapeItem(outfitShapes, outfitRoot, shapeName);
	if (!item.IsOk())
		return false;

	int state = outfitShapes->GetItemState(item);
	outMode = state == 1 ? ShapeDisplayMode::Hidden : (state == 2 ? ShapeDisplayMode::Ghost : ShapeDisplayMode::Visible);
	return true;
}

bool OutfitStudioFrame::SetShapeDisplayMode(const std::string& shapeName, ShapeDisplayMode mode) {
	wxTreeItemId item = FindShapeItem(outfitShapes, outfitRoot, shapeName);
	if (!item.IsOk())
		return false;

	glView->SetShapeGhostMode(shapeName, mode == ShapeDisplayMode::Ghost);
	glView->ShowShape(shapeName, mode != ShapeDisplayMode::Hidden);
	outfitShapes->SetItemState(item, static_cast<int>(mode));
	return true;
}

std::vector<std::string> OutfitStudioFrame::GetShapeList() {
	std::vector<std::string> shapes;

	if (!outfitRoot.IsOk())
		return shapes;

	wxTreeItemIdValue cookie;

	wxTreeItemId curItem = outfitShapes->GetFirstChild(outfitRoot, cookie);
	while (curItem.IsOk()) {
		wxString shapeName = outfitShapes->GetItemText(curItem);
		shapes.push_back(shapeName.ToStdString());
		curItem = outfitShapes->GetNextChild(outfitRoot, cookie);
	}

	return shapes;
}

void OutfitStudioFrame::UpdateShapeSource(NiShape* shape) {
	ProjectOperations(*project, editor).UpdateShapeFromMesh(shape);
}

void OutfitStudioFrame::ActiveShapesUpdated(UndoStateProject* usp, bool bIsUndo) {
	ApplyRefresh(ProjectOperations(*project, editor).ApplyShapeEdits(*usp, bIsUndo));
}

void OutfitStudioFrame::UpdateShapeReference(NiShape* shape, NiShape* newShape) {
	for (auto& i : selectedItems) {
		if (i->GetShape() == shape) {
			i->SetShape(newShape);
		}
	}

	if (project->IsBaseShape(shape))
		project->SetBaseShape(newShape, false);
}

std::vector<ShapeItemData*>& OutfitStudioFrame::GetSelectedItems() {
	return selectedItems;
}

void OutfitStudioFrame::ClearSelected(NiShape* shape) {
	if (activeItem && activeItem->GetShape() == shape)
		activeItem = nullptr;

	selectedItems.erase(std::remove_if(selectedItems.begin(), selectedItems.end(), [&](ShapeItemData* i) { return i->GetShape() == shape; }), selectedItems.end());
}

void OutfitStudioFrame::LockShapeSelect() {
	selectionLocked = true;
	outfitShapes->Disable();
}

void OutfitStudioFrame::UnlockShapeSelect() {
	selectionLocked = false;
	outfitShapes->Enable();
}

void OutfitStudioFrame::MeshesFromProj(const bool reloadTextures) {
	glView->gls.DeleteOverlay("refineErrorEdges");

	for (auto& shape : project->GetWorkNif()->GetShapes())
		MeshFromProj(shape, reloadTextures);

	// Every mesh has been through SetMeshTextures by now, so whether any of them is a Complex
	// Material has been decided and the environment can be turned on for them.
	ApplyAutoHDRiBackground();

	if (glView->GetVertexEdit())
		glView->ShowVertexEdit();
}

void OutfitStudioFrame::MeshFromProj(NiShape* shape, const bool reloadTextures) {
	if (editUV)
		editUV->Close();

	// The mesh is also the editable state of a shape (mask, brushes, slider preview), so it exists
	// without OpenGL too. Buffers and textures follow once the view is initialized.
	glView->DeleteMesh(shape->name.get());
	glView->AddMeshFromNif(project->GetWorkNif(), shape->name.get());

	if (extInitialized) {
		MaterialFile matFile;
		bool hasMatFile = project->GetShapeMaterialFile(shape, matFile);
		glView->SetMeshTextures(shape->name.get(), project->GetShapeTextures(shape), hasMatFile, matFile, reloadTextures, project->GetShapeSFMaterial(shape));
	}

	UpdateMeshFromSet(shape);

	std::vector<std::string> selShapes;
	for (auto& i : selectedItems)
		selShapes.push_back(i->GetShape()->name.get());

	glView->SetActiveShapes(selShapes);

	if (activeItem)
		glView->SetSelectedShape(activeItem->GetShape()->name.get());
	else
		glView->SetSelectedShape("");
}

void OutfitStudioFrame::UpdateMeshFromSet(NiShape* shape) {
	bool disableNormalsCalc = OutfitStudioConfig.GetBoolValue("DisableNormalsCalc");

	std::string shapeName = shape->name.get();
	Mesh* m = glView->GetMesh(shapeName);
	if (m) {
		m->smoothSeamNormals = project->activeSet.GetSmoothSeamNormals(shapeName);
		m->smoothSeamNormalsAngle = project->activeSet.GetSmoothSeamNormalsAngle(shapeName);
		m->lockNormals = disableNormalsCalc || project->activeSet.GetLockNormals(shapeName);
	}
}

void OutfitStudioFrame::FillVertexColors() {
	std::vector<std::string> shapeNames = project->GetWorkNif()->GetShapeNames();

	for (auto& s : shapeNames) {
		Mesh* m = glView->GetMesh(s);
		if (!m)
			continue;

		m->ColorFill(Vector3(1.0f, 1.0f, 1.0f));
		m->AlphaFill(1.0f);

		const std::vector<Color4>* vcolors = project->GetWorkNif()->GetColorsForShape(s);
		if (vcolors) {
			for (size_t v = 0; v < vcolors->size(); v++) {
				m->vcolors[v].x = vcolors->at(v).r;
				m->vcolors[v].y = vcolors->at(v).g;
				m->vcolors[v].z = vcolors->at(v).b;
				m->valpha[v] = vcolors->at(v).a;
			}
		}

		m->QueueUpdate(Mesh::UpdateType::VertexColors);
		m->QueueUpdate(Mesh::UpdateType::VertexAlpha);

		if (m->overrideVertexColors) {
			m->overrideVertexColors = false;
			glView->gls.UpdateShaders(m);
		}
	}
}

bool OutfitStudioFrame::ShapeSelectionCheck() {
	if (!activeItem) {
		wxMessageBox(_("There is no shape selected!"), _("Error"));
		return false;
	}
	return true;
}

void OutfitStudioFrame::ToggleVisibility(wxTreeItemId firstItem) {
	bool vis = true;
	bool ghost = false;
	int state = 0;

	if (!firstItem.IsOk()) {
		if (!selectedItems.empty()) {
			firstItem = selectedItems.front()->GetId();
		}
	}

	if (firstItem.IsOk()) {
		state = outfitShapes->GetItemState(firstItem);
		switch (state) {
			case 0:
				vis = false;
				ghost = false;
				state = 1;
				break;
			case 1:
				vis = true;
				ghost = true;
				state = 2;
				break;
			default:
				vis = true;
				ghost = false;
				state = 0;
				break;
		}

		std::string shapeName{outfitShapes->GetItemText(firstItem).ToUTF8()};
		glView->SetShapeGhostMode(shapeName, ghost);
		glView->ShowShape(shapeName, vis);
		outfitShapes->SetItemState(firstItem, state);
	}

	if (selectedItems.size() > 1) {
		for (auto& i : selectedItems) {
			if (i->GetId().GetID() != firstItem.GetID()) {
				std::string shapeName{outfitShapes->GetItemText(i->GetId()).ToUTF8()};
				glView->SetShapeGhostMode(shapeName, ghost);
				glView->ShowShape(shapeName, vis);
				outfitShapes->SetItemState(i->GetId(), state);
			}
		}
	}

	glView->Render();
}

void OutfitStudioFrame::OnShapeVisToggle(wxTreeEvent& event) {
	ToggleVisibility(event.GetItem());
	event.Skip();
}

void OutfitStudioFrame::OnShapeSelect(wxTreeEvent& event) {
	wxTreeItemId item = event.GetItem();
	if (!item.IsOk()) {
		event.Veto();
		return;
	}

	if (selectionLocked) {
		event.Veto();
		return;
	}

	if (outfitShapes->GetItemParent(item).IsOk()) {
		if (outfitShapes->IsSelected(item))
			activeItem = (ShapeItemData*)outfitShapes->GetItemData(item);
		else
			activeItem = nullptr;
	}
	else {
		wxTreeItemIdValue cookie;
		wxTreeItemId subitem = outfitShapes->GetFirstChild(item, cookie);
		if (subitem.IsOk() && outfitShapes->IsSelected(subitem))
			activeItem = (ShapeItemData*)outfitShapes->GetItemData(subitem);
		else
			activeItem = nullptr;
	}

	selectedItems.clear();

	std::vector<std::string> shapeNames;
	wxArrayTreeItemIds selected;
	outfitShapes->GetSelections(selected);

	for (auto& i : selected) {
		if (outfitShapes->GetItemParent(i).IsOk()) {
			auto data = (ShapeItemData*)outfitShapes->GetItemData(i);
			if (data) {
				shapeNames.push_back(data->GetShape()->name.get());
				selectedItems.push_back(data);

				if (!activeItem)
					activeItem = data;
			}
		}
		else {
			wxTreeItemIdValue cookie;
			wxTreeItemId subitem = outfitShapes->GetFirstChild(i, cookie);
			if (subitem.IsOk()) {
				auto data = (ShapeItemData*)outfitShapes->GetItemData(subitem);
				if (data) {
					shapeNames.push_back(data->GetShape()->name.get());
					selectedItems.push_back(data);

					if (!activeItem)
						activeItem = data;
				}
			}
		}
	}

	glView->SetActiveShapes(shapeNames);

	if (activeItem)
		glView->SetSelectedShape(activeItem->GetShape()->name.get());
	else
		glView->SetSelectedShape("");

	UpdateActiveShape();
}

void OutfitStudioFrame::OnShapeActivated(wxTreeEvent& event) {
	int hitFlags;
	outfitShapes->HitTest(event.GetPoint(), hitFlags);

	if (hitFlags & wxTREE_HITTEST_ONITEMSTATEICON)
		return;

	wxCommandEvent evt;
	OnShapeProperties(evt);
}

void OutfitStudioFrame::OnCheckTreeSel(wxTreeEvent& event) {
	int outflags;
	wxPoint p;
	wxGetMousePosition(&p.x, &p.y);
	p = outfitShapes->ScreenToClient(p);

	int mask = wxTREE_HITTEST_ONITEMINDENT | wxTREE_HITTEST_ONITEMSTATEICON;
	outfitShapes->HitTest(p, outflags);
	if ((outflags & mask) != 0)
		event.Veto();
}

void OutfitStudioFrame::OnShapeContext(wxTreeEvent& event) {
	wxTreeItemId item = event.GetItem();
	if (outfitShapes->GetItemParent(item).IsOk()) {
		outfitShapes->SelectItem(item);

		wxMenu* menu = wxXmlResource::Get()->LoadMenu("menuMeshContext");
		if (menu) {
			PopupMenu(menu);
			delete menu;
		}
	}
}

void OutfitStudioFrame::OnShapeTreeMotion(wxMouseEvent& event) {
	event.Skip();

	if (!outfitShapes || !glView)
		return;

	const wxPoint mousePos = event.GetPosition();
	int flags = 0;
	wxTreeItemId item = outfitShapes->HitTest(mousePos, flags);

	// The project's node explains what it's an add-on or variant of
	wxString tip;
	if (item.IsOk() && item == outfitRoot && (flags & wxTREE_HITTEST_ONITEMLABEL) != 0)
		tip = ProjectNodeTooltip();

	wxString currentTip = outfitShapes->GetToolTipText();
	if (tip != currentTip) {
		if (tip.empty())
			outfitShapes->UnsetToolTip();
		else
			outfitShapes->SetToolTip(tip);
	}

	if (!Config.GetBoolValue("Input/ShapeHoverHighlight")) {
		glView->ClearHoverHighlight();
		return;
	}

	bool onItemLabelText = false;
	if ((flags & wxTREE_HITTEST_ONITEMLABEL) != 0 && item.IsOk() && outfitShapes->GetItemParent(item).IsOk()) {
		wxRect labelRect;
		onItemLabelText = outfitShapes->GetBoundingRect(item, labelRect, true) && labelRect.Contains(mousePos);
	}

	if (onItemLabelText) {
		auto* data = dynamic_cast<ShapeItemData*>(outfitShapes->GetItemData(item));
		if (data && data->GetShape())
			glView->SetHoverHighlight(data->GetShape()->name.get());
		else
			glView->ClearHoverHighlight();
	}
	else {
		glView->ClearHoverHighlight();
	}
}

void OutfitStudioFrame::OnShapeTreeLeave(wxMouseEvent& event) {
	event.Skip();

	if (glView)
		glView->ClearHoverHighlight();
}

void OutfitStudioFrame::OnShapeDrag(wxTreeEvent& event) {
	if (activeItem) {
		wxPoint p;
		wxGetMousePosition(&p.x, &p.y);
		p = outfitShapes->ScreenToClient(p);

		int outflags;
		outfitShapes->HitTest(p, outflags);

		int mask = wxTREE_HITTEST_ONITEMINDENT | wxTREE_HITTEST_ONITEMSTATEICON;
		if ((outflags & mask) == 0) {
			outfitShapes->SetCursor(wxCURSOR_HAND);
			event.Allow();
		}
		else
			event.Veto();
	}
}

void OutfitStudioFrame::OnShapeDrop(wxTreeEvent& event) {
	outfitShapes->SetCursor(wxNullCursor);
	wxTreeItemId dropItem = event.GetItem();

	if (!dropItem.IsOk() || !activeItem)
		return;

	// Make first child
	if (dropItem == outfitRoot)
		dropItem = 0;

	// Duplicate item
	wxTreeItemId movedItem = outfitShapes->InsertItem(outfitRoot, dropItem, activeItem->GetShape()->name.get());
	if (!movedItem.IsOk())
		return;

	// Set data
	auto dropData = new ShapeItemData(activeItem->GetShape());
	outfitShapes->SetItemState(movedItem, 0);
	outfitShapes->SetItemData(movedItem, dropData);
	if (project->IsBaseShape(dropData->GetShape())) {
		outfitShapes->SetItemBold(movedItem);
		outfitShapes->SetItemTextColour(movedItem, wxColour(0, 255, 0));
	}

	// Delete old item
	outfitShapes->Delete(activeItem->GetId());

	// Select new item
	outfitShapes->UnselectAll();
	outfitShapes->SelectItem(movedItem);

	// Keep NIF shape order in sync with the tree order so future GUI refreshes preserve user reordering.
	project->GetWorkNif()->SetShapeOrder(GetShapeList());
}
