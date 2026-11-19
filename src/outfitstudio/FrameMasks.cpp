/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

// Outfit Studio window: masks

#include "OutfitStudioFrame.h"
#include "SegmentPartitionEditor.h"
#include "OutfitStudioApp.h"
#include "GLPanel.h"
#include "ShapeTreeItems.h"
#include "utils/ConfigurationManager.h"
#include "components/UndoHistory.h"
#include "files/MaskFile.h"
#include "ui/ConfigDialogUtil.h"
#include "components/MaskTransfer.h"

#include <cmath>
#include <cstdlib>
#include <iterator>
#include <sstream>

using namespace nifly;

void OutfitStudioFrame::OnMaskVertexColor(wxCommandEvent& WXUNUSED(event)) {
	auto cpBrushColor = (wxColourPickerCtrl*)FindWindowById(XRCID("cpBrushColor"));
	if (!cpBrushColor)
		return;

	if (!ShapeSelectionCheck())
		return;

	wxColour color = cpBrushColor->GetColour();
	Vector3 brushColor;
	brushColor.x = color.Red() / 255.0f;
	brushColor.y = color.Green() / 255.0f;
	brushColor.z = color.Blue() / 255.0f;

	UndoStateProject* usp = glView->GetUndoHistory()->PushState();
	usp->undoType = UndoType::Mask;

	auto isSimilarColor = [](const Vector3& a,
							 const Vector3& b,
							 float angleEps = 1e-3f,	   // ~cos angle tolerance
							 float chromaThresh = 1e-4f) { // how much colorfulness we require
		if (a == b)
			return true;

		auto getChromaDir = [&](const Vector3& c, Vector3& outDir) {
			float minc = std::min({c.x, c.y, c.z});
			Vector3 chroma{c.x - minc, c.y - minc, c.z - minc}; // strip added white
			float l = chroma.length();
			if (l < chromaThresh)
				return false; // basically gray/white -> no reliable hue

			outDir = {chroma.x / l, chroma.y / l, chroma.z / l};
			return true;
		};

		Vector3 da, db;
		bool okA = getChromaDir(a, da);
		bool okB = getChromaDir(b, db);

		if (!okA && !okB) // both are essentially gray/white
			return false; // treat all near-white/gray as mismatch
		if (okA != okB)
			return false; // one has hue, the other doesn't

		return da.dot(db) > 1.0f - angleEps;
	};

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

			if (m->vcolors && isSimilarColor(m->vcolors[i], brushColor))
				uss.pointEndState[i].x = 1.0f;
			else
				uss.pointEndState[i].x = 0.0f;
		}
	}

	glView->ApplyUndoState(usp, false);

	if (!Config.GetBoolValue("Input/MaskHistory"))
		glView->GetUndoHistory()->PopState();

	UpdateUndoTools();
}

void OutfitStudioFrame::OnMaskLess(wxCommandEvent& WXUNUSED(event)) {
	if (!activeItem)
		return;

	if (glView->GetSegmentMode()) {
		if (segPartEditor->GrowShrinkSegmentPartitionSelection(false)) {
			segPartEditor->ShowSegment();
			segPartEditor->ShowPartition();
		}

		if (glView->GetTransformMode())
			glView->ShowTransformTool();
		else
			glView->Render();
		return;
	}

	glView->MaskLess();

	if (glView->GetTransformMode())
		glView->ShowTransformTool();
	else
		glView->Render();
}

void OutfitStudioFrame::OnMaskMore(wxCommandEvent& WXUNUSED(event)) {
	if (!activeItem)
		return;

	if (glView->GetSegmentMode()) {
		if (segPartEditor->GrowShrinkSegmentPartitionSelection(true)) {
			segPartEditor->ShowSegment();
			segPartEditor->ShowPartition();
		}

		if (glView->GetTransformMode())
			glView->ShowTransformTool();
		else
			glView->Render();
		return;
	}

	glView->MaskMore();

	if (glView->GetTransformMode())
		glView->ShowTransformTool();
	else
		glView->Render();
}

void OutfitStudioFrame::OnClearMask(wxCommandEvent& WXUNUSED(event)) {
	if (!activeItem)
		return;

	glView->gls.DeleteOverlay("refineErrorEdges");
	glView->ClearMask();

	if (glView->GetTransformMode())
		glView->ShowTransformTool();
	else
		glView->Render();
}

void OutfitStudioFrame::OnInvertMask(wxCommandEvent& WXUNUSED(event)) {
	if (!activeItem)
		return;

	glView->InvertMask();

	if (glView->GetTransformMode())
		glView->ShowTransformTool();
	else
		glView->Render();
}

void OutfitStudioFrame::OnSelectMask(wxCommandEvent& WXUNUSED(event)) {
	auto cMaskName = (wxComboBox*)FindWindowByName("cMaskName");
	int maskSel = cMaskName->GetSelection();
	if (maskSel != wxNOT_FOUND) {
		auto maskData = (std::map<std::string, std::unordered_map<uint16_t, float>>*)cMaskName->GetClientData(maskSel);
		for (auto mask : (*maskData)) {
			glView->SetShapeMask(mask.second, mask.first);
		}
	}

	glView->Render();
}

void OutfitStudioFrame::OnSaveMask(wxCommandEvent& WXUNUSED(event)) {
	auto cMaskName = (wxComboBox*)FindWindowByName("cMaskName");

	wxString maskName = cMaskName->GetValue();
	if (maskName.empty())
		return;

	auto maskData = new std::map<std::string, std::unordered_map<uint16_t, float>>();

	std::vector<std::string> shapes = GetShapeList();
	for (auto& s : shapes) {
		std::unordered_map<uint16_t, float> mask;
		glView->GetShapeMask(mask, s);
		(*maskData)[s] = std::move(mask);
	}

	int existingSel = cMaskName->FindString(maskName);
	if (existingSel != wxNOT_FOUND) {
		cMaskName->SetClientData(existingSel, maskData);
		cMaskName->SetSelection(existingSel);
	}
	else {
		int maskSel = cMaskName->Append(maskName, maskData);
		cMaskName->SetSelection(maskSel);
	}
}

void OutfitStudioFrame::OnDeleteMask(wxCommandEvent& WXUNUSED(event)) {
	auto cMaskName = (wxComboBox*)FindWindowByName("cMaskName");
	int maskSel = cMaskName->GetSelection();
	if (maskSel != wxNOT_FOUND) {
		cMaskName->Delete(maskSel);
		cMaskName->SetValue("");
	}
}

void OutfitStudioFrame::OnExportMask(wxCommandEvent& WXUNUSED(event)) {
	auto cMaskName = (wxComboBox*)FindWindowByName("cMaskName");
	if (cMaskName->GetCount() == 0) {
		wxMessageBox(_("No masks to export."), _("Export Masks"), wxICON_INFORMATION);
		return;
	}

	wxFileDialog saveDialog(this, _("Export Masks"), wxEmptyString, "masks.xml",
		"XML Files (*.xml)|*.xml", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

	if (saveDialog.ShowModal() == wxID_CANCEL)
		return;

	std::string filePath = saveDialog.GetPath().ToUTF8().data();

	MaskFile maskFile;
	for (unsigned int i = 0; i < cMaskName->GetCount(); i++) {
		auto maskData = (std::map<std::string, std::unordered_map<uint16_t, float>>*)cMaskName->GetClientData(i);
		if (!maskData)
			continue;

		MaskEntry entry;
		entry.name = cMaskName->GetString(i).ToUTF8().data();

		std::map<std::string, int> vertexCounts;
		for (auto& [shapeName, mask] : *maskData) {
			Mesh* m = glView->GetMesh(shapeName);
			if (m)
				vertexCounts[shapeName] = m->nVerts;
		}

		entry.SetFromMaskData(*maskData, vertexCounts);
		maskFile.GetEntries().push_back(std::move(entry));
	}

	int err = maskFile.Save(filePath);
	if (err)
		wxMessageBox(_("Failed to save mask file."), _("Export Masks"), wxICON_ERROR);
}

void OutfitStudioFrame::OnImportMask(wxCommandEvent& WXUNUSED(event)) {
	wxFileDialog openDialog(this, _("Import Masks"), wxEmptyString, wxEmptyString,
		"XML Files (*.xml)|*.xml", wxFD_OPEN | wxFD_FILE_MUST_EXIST);

	if (openDialog.ShowModal() == wxID_CANCEL)
		return;

	std::string filePath = openDialog.GetPath().ToUTF8().data();

	MaskFile maskFile;
	int err = maskFile.Load(filePath);
	if (err) {
		wxMessageBox(_("Failed to load mask file."), _("Import Masks"), wxICON_ERROR);
		return;
	}

	auto cMaskName = (wxComboBox*)FindWindowByName("cMaskName");

	for (const auto& entry : maskFile.GetEntries()) {
		wxString maskName = entry.name.empty() ? _("Imported Mask") : wxString::FromUTF8(entry.name);
		auto maskData = new std::map<std::string, std::unordered_map<uint16_t, float>>(entry.ToMaskData());

		int existing = cMaskName->FindString(maskName);
		if (existing != wxNOT_FOUND) {
			cMaskName->SetClientData(existing, maskData);
		}
		else {
			cMaskName->Append(maskName, maskData);
		}
	}

	// Select and apply the first imported entry
	if (!maskFile.GetEntries().empty()) {
		wxString firstName = wxString::FromUTF8(maskFile.GetEntries().front().name);
		int sel = cMaskName->FindString(firstName);
		if (sel != wxNOT_FOUND) {
			cMaskName->SetSelection(sel);
			auto maskData = (std::map<std::string, std::unordered_map<uint16_t, float>>*)cMaskName->GetClientData(sel);
			if (maskData) {
				for (auto& [shapeName, mask] : *maskData)
					glView->SetShapeMask(mask, shapeName);
			}
		}
	}

	glView->Render();
}

void OutfitStudioFrame::OnCopyMask(wxCommandEvent& WXUNUSED(event)) {
	std::vector<std::string> shapeList = GetShapeList();
	if (shapeList.size() < 2) {
		wxMessageBox(_("Copying a mask needs at least two shapes."), _("Copy Mask"), wxICON_INFORMATION);
		return;
	}

	wxDialog dlg;
	if (!wxXmlResource::Get()->LoadDialog(&dlg, this, "dlgCopyMask"))
		return;

	wxChoice* sourceChoice = XRCCTRL(dlg, "sourceChoice", wxChoice);
	wxCheckListBox* targetList = XRCCTRL(dlg, "targetList", wxCheckListBox);
	if (!sourceChoice || !targetList)
		return;

	CloseBrushSettings();

	std::unordered_set<std::string> selectedShapes;
	for (auto& s : selectedItems)
		selectedShapes.insert(s->GetShape()->name.get());

	auto hasMask = [this](const std::string& shapeName) {
		std::unordered_map<uint16_t, float> mask;
		glView->GetShapeMask(mask, shapeName);
		return !mask.empty();
	};

	// Prefer a masked shape that isn't selected as the source, the reference shape first
	std::string sourceShapeName;
	NiShape* baseShape = project->GetBaseShape();
	if (baseShape && !selectedShapes.count(baseShape->name.get()) && hasMask(baseShape->name.get()))
		sourceShapeName = baseShape->name.get();

	for (int pass = 0; pass < 2 && sourceShapeName.empty(); pass++) {
		for (const std::string& shape : shapeList) {
			if ((pass == 1 || !selectedShapes.count(shape)) && hasMask(shape)) {
				sourceShapeName = shape;
				break;
			}
		}
	}

	for (const std::string& shape : shapeList) {
		sourceChoice->Append(wxString::FromUTF8(shape));
		targetList->Append(wxString::FromUTF8(shape));
		if (shape == sourceShapeName)
			sourceChoice->SetSelection(sourceChoice->GetCount() - 1);
	}

	if (sourceChoice->GetSelection() == wxNOT_FOUND)
		sourceChoice->SetSelection(0);

	for (unsigned int i = 0; i < shapeList.size(); i++)
		targetList->Check(i, selectedShapes.count(shapeList[i]) && static_cast<int>(i) != sourceChoice->GetSelection());

	// The source can't also be a target
	sourceChoice->Bind(wxEVT_CHOICE, [sourceChoice, targetList](wxCommandEvent&) { targetList->Check(sourceChoice->GetSelection(), false); });
	targetList->Bind(wxEVT_CHECKLISTBOX, [sourceChoice, targetList](wxCommandEvent& evt) {
		if (evt.GetInt() == sourceChoice->GetSelection())
			targetList->Check(evt.GetInt(), false);
	});

	ConfigDialogUtil::LoadDialogTextFloat(OutfitStudioConfig, dlg, "CopyMask", "maxDistance");
	ConfigDialogUtil::LoadDialogCheckBox(OutfitStudioConfig, dlg, "CopyMask", "addToMask");

	dlg.SetSize(dlg.GetBestSize());
	if (dlg.ShowModal() != wxID_OK)
		return;

	float maxDistance = ConfigDialogUtil::SetFloatFromDialogTextControl(OutfitStudioConfig, dlg, "CopyMask", "maxDistance");
	bool addToMask = ConfigDialogUtil::SetBoolFromDialogCheckbox(OutfitStudioConfig, dlg, "CopyMask", "addToMask");

	sourceShapeName = shapeList[sourceChoice->GetSelection()];
	Mesh* source = glView->GetMesh(sourceShapeName);
	if (!source || !source->mask)
		return;

	std::vector<Vector3> sourceVerts(source->verts.get(), source->verts.get() + source->nVerts);
	std::vector<Triangle> sourceTris(source->tris.get(), source->tris.get() + source->nTris);
	std::vector<float> sourceMask(source->mask.get(), source->mask.get() + source->nVerts);

	UndoStateProject* usp = glView->GetUndoHistory()->PushState();
	usp->undoType = UndoType::Mask;

	for (unsigned int i = 0; i < shapeList.size(); i++) {
		if (!targetList->IsChecked(i) || shapeList[i] == sourceShapeName)
			continue;

		Mesh* target = glView->GetMesh(shapeList[i]);
		if (!target || !target->mask)
			continue;

		std::vector<Vector3> targetVerts(target->verts.get(), target->verts.get() + target->nVerts);
		std::vector<float> copiedMask;
		std::vector<bool> matched;
		MaskTransfer::CopyMask(sourceVerts, sourceTris, sourceMask, targetVerts, maxDistance, copiedMask, matched);

		usp->usss.emplace_back();
		UndoStateShape& uss = usp->usss.back();
		uss.shapeName = target->shapeName;

		int nMatched = 0;
		for (int vi = 0; vi < target->nVerts; vi++) {
			if (matched[vi])
				nMatched++;

			uss.pointStartState[vi].x = target->mask[vi];
			uss.pointEndState[vi].x = addToMask ? std::max(target->mask[vi], copiedMask[vi]) : copiedMask[vi];
		}

		wxLogMessage("Copied mask from '%s' to '%s' (%d of %d vertices within distance %g).", sourceShapeName, target->shapeName, nMatched, target->nVerts, maxDistance);
	}

	if (usp->usss.empty()) {
		glView->GetUndoHistory()->PopState();
		return;
	}

	glView->ApplyUndoState(usp, false);

	if (!Config.GetBoolValue("Input/MaskHistory"))
		glView->GetUndoHistory()->PopState();

	UpdateUndoTools();
}
