/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

// Outfit Studio window: shape operations such as transforms, mirroring, geometry edits, symmetry and normals

#include "OutfitStudioFrame.h"
#include "SegmentPartitionEditor.h"
#include "OutfitStudioApp.h"
#include "FrameHelpers.h"
#include "ShapeTreeItems.h"
#include "GLPanel.h"
#include "utils/ConfigurationManager.h"
#include "ui/wxStateButton.h"
#include "components/UndoHistory.h"
#include "files/HkxFile.h"
#include "ui/outfitstudio/EditUV.h"
#include "ui/outfitstudio/ShapeProperties.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <iterator>
#include <limits>
#include <sstream>
#include <wx/textctrl.h>

using namespace nifly;

void OutfitStudioFrame::OnInvertUV(wxCommandEvent& event) {
	if (!ShapeSelectionCheck())
		return;

	bool invertX = (event.GetId() == XRCID("uvInvertX"));
	bool invertY = (event.GetId() == XRCID("uvInvertY"));

	for (auto& i : selectedItems) {
		project->GetWorkNif()->InvertUVsForShape(i->GetShape(), invertX, invertY);
	}

	RefreshGUIFromProj();
	SetPendingChanges();
}

void OutfitStudioFrame::OnMirrorShape(wxCommandEvent& WXUNUSED(event)) {
	CloseBrushSettings();

	if (!ShapeSelectionCheck())
		return;

	if (!CheckEditableState())
		return;

	wxDialog dlg;
	if (wxXmlResource::Get()->LoadDialog(&dlg, this, "dlgMirrorShape")) {
		bool previewMirror = false;

		auto updateMirrorPreview = [&]() {
			if (previewMirror) {
				UndoStateProject* curState = glView->GetUndoHistory()->GetBackState();
				if (curState) {
					glView->ApplyUndoState(curState, true, false);
					glView->GetUndoHistory()->PopState();
					previewMirror = false;
				}
			}

			bool mirrorAxisX = XRCCTRL(dlg, "mirrorAxisX", wxCheckBox)->IsChecked();
			bool mirrorAxisY = XRCCTRL(dlg, "mirrorAxisY", wxCheckBox)->IsChecked();
			bool mirrorAxisZ = XRCCTRL(dlg, "mirrorAxisZ", wxCheckBox)->IsChecked();

			wxCheckBox* swapBonesX = XRCCTRL(dlg, "swapBonesX", wxCheckBox);
			if (mirrorAxisX && !swapBonesX->IsEnabled())
				swapBonesX->Set3StateValue(wxCheckBoxState::wxCHK_CHECKED);

			swapBonesX->Enable(mirrorAxisX);

			if (mirrorAxisX || mirrorAxisY || mirrorAxisZ) {
				UndoStateProject* usp = glView->GetUndoHistory()->PushState();
				usp->undoType = UndoType::Mirror;
				usp->mirrorX = mirrorAxisX;
				usp->mirrorY = mirrorAxisY;
				usp->mirrorZ = mirrorAxisZ;
				usp->swapBonesX = swapBonesX->IsChecked();

				for (auto& sel : selectedItems) {
					NiShape* shape = sel->GetShape();

					UndoStateShape uss;
					uss.shapeName = shape->name.get();

					usp->usss.push_back(std::move(uss));
				}

				glView->ApplyUndoState(usp, false);

				previewMirror = true;

				if (glView->GetTransformMode())
					glView->ShowTransformTool();
			}
		};

		auto mirrorAxisChanged = [&](wxCommandEvent&) { updateMirrorPreview(); };

		XRCCTRL(dlg, "mirrorAxisX", wxCheckBox)->Bind(wxEVT_CHECKBOX, mirrorAxisChanged);
		XRCCTRL(dlg, "mirrorAxisY", wxCheckBox)->Bind(wxEVT_CHECKBOX, mirrorAxisChanged);
		XRCCTRL(dlg, "mirrorAxisZ", wxCheckBox)->Bind(wxEVT_CHECKBOX, mirrorAxisChanged);
		XRCCTRL(dlg, "swapBonesX", wxCheckBox)->Bind(wxEVT_CHECKBOX, mirrorAxisChanged);
		dlg.Bind(wxEVT_CHAR_HOOK, &OutfitStudioFrame::OnEnterClose, this);

		if (dlg.ShowModal() != wxID_OK) {
			if (previewMirror) {
				UndoStateProject* curState = glView->GetUndoHistory()->GetBackState();
				if (curState) {
					glView->ApplyUndoState(curState, true);
					glView->GetUndoHistory()->PopState();
				}
			}
		}

		UpdateUndoTools();
		SetPendingChanges();
	}
}

void OutfitStudioFrame::OnRenameShape(wxCommandEvent& WXUNUSED(event)) {
	CloseBrushSettings();

	if (!ShapeSelectionCheck())
		return;

	if (project->activeSet.IsAddOn()) {
		wxMessageBox(_("The shapes of an add-on belong to its base project, so they can't be renamed. Make the add-on a variant with a NIF file of its own first."),
					 _("Rename Shape"),
					 wxOK | wxICON_INFORMATION,
					 this);
		return;
	}

	std::string shapeName = activeItem->GetShape()->name.get();
	std::string newShapeName;
	do {
		std::string result{GetTextFromUser(_("Please enter a new unique name for the shape."), _("Rename Shape"), wxString::FromUTF8(shapeName), this).ToUTF8()};
		if (result.empty())
			return;

		newShapeName = std::move(result);
	} while (project->IsValidShape(newShapeName));

	if (!ConfirmRenameFromBase("", shapeName))
		return;

	wxLogMessage("Renaming shape '%s' to '%s'.", shapeName, newShapeName);
	project->RenameShape(activeItem->GetShape(), newShapeName);
	glView->RenameShape(shapeName, newShapeName);

	outfitShapes->SetItemText(activeItem->GetId(), wxString::FromUTF8(newShapeName));
	SetPendingChanges();
}

void OutfitStudioFrame::OnSetReference(wxCommandEvent& WXUNUSED(event)) {
	if (!ShapeSelectionCheck())
		return;

	auto shape = activeItem->GetShape();
	if (!project->IsBaseShape(shape))
		project->SetBaseShape(shape);
	else
		project->SetBaseShape(nullptr);

	RefreshGUIFromProj();
	SetPendingChanges();
}

// What differs between the Move, Scale, Rotate and Inflate Shape dialogs
struct ShapeTransformDialogSpec {
	const char* dialogName = nullptr;
	// Controls are named <prefix>SliderX/Y/Z and <prefix>TextX/Y/Z, and <prefix>Uniform
	const char* prefix = nullptr;
	// Slider positions per unit of the value and the format of the text fields
	float sliderScale = 1000.0f;
	const char* textFormat = "%0.5f";
	// The value that changes nothing, which the dialog starts with
	Vector3 identity;
	// A <prefix>Uniform check box makes one slider or text field set all three
	bool hasUniform = false;
	// An "origin" choice picks the origin (0) or the center of the selected shapes (1)
	bool hasOrigin = false;
	// Lower limit of each value component, if any
	float minValue = std::numeric_limits<float>::lowest();
	// Check boxes that change the preview
	std::vector<const char*> checkBoxes;
	// Fills the unmasked change of each vertex of a shape. Returns false to leave the shape out.
	std::function<bool(wxDialog& dlg, NiShape* shape, const std::vector<Vector3>& verts, const Vector3& value, const Vector3& origin, std::vector<Vector3>& diffs)> diffsForShape;
	// Runs on the undo state of each shape once its changes are in, if set
	std::function<void(NiShape* shape, const std::vector<Vector3>& verts, UndoStateShape& uss)> finishShape;
};

void OutfitStudioFrame::RunShapeTransformDialog(const ShapeTransformDialogSpec& spec) {
	CloseBrushSettings();

	if (!ShapeSelectionCheck())
		return;

	if (!CheckEditableState())
		return;

	wxDialog dlg;
	if (!wxXmlResource::Get()->LoadDialog(&dlg, this, spec.dialogName))
		return;

	const wxString prefix = spec.prefix;
	const char* axes[3] = {"X", "Y", "Z"};
	auto slider = [&](int axis) { return XRCCTRL(dlg, prefix + "Slider" + axes[axis], wxSlider); };
	auto text = [&](int axis) { return XRCCTRL(dlg, prefix + "Text" + axes[axis], wxTextCtrl); };
	auto textValue = [&](int axis) { return static_cast<float>(atof(text(axis)->GetValue().c_str())); };

	// Whether a preview is applied, which the next preview or cancelling undoes. Not the same as a value
	// other than the identity: mirroring changes the shape at any value.
	bool previewApplied = false;

	auto undoPreview = [&](bool render) {
		if (!previewApplied)
			return;
		previewApplied = false;

		UndoStateProject* curState = glView->GetUndoHistory()->GetBackState();
		if (curState) {
			glView->ApplyUndoState(curState, true, render);
			glView->GetUndoHistory()->PopState();
		}
	};

	auto updatePreview = [&]() {
		undoPreview(false);

		Vector3 value(textValue(0), textValue(1), textValue(2));
		value.x = std::max(value.x, spec.minValue);
		value.y = std::max(value.y, spec.minValue);
		value.z = std::max(value.z, spec.minValue);

		Vector3 origin;
		if (spec.hasOrigin && XRCCTRL(dlg, "origin", wxChoice)->GetCurrentSelection() == 1) {
			// Center of selected shape(s), respecting mask
			origin = glView->gls.GetActiveCenter();
			origin = Mesh::TransformPosMeshToNif(origin);
		}

		UndoStateProject* usp = glView->GetUndoHistory()->PushState();
		usp->undoType = UndoType::VertexPosition;

		std::unordered_map<uint16_t, float> mask;
		std::vector<Vector3> verts;
		std::vector<Vector3> diffs;
		for (auto& sel : selectedItems) {
			NiShape* shape = sel->GetShape();
			mask.clear();
			project->GetLiveVerts(shape, verts);
			glView->GetShapeMask(mask, shape->name.get());

			diffs.assign(verts.size(), Vector3());
			if (!spec.diffsForShape(dlg, shape, verts, value, origin, diffs))
				continue;

			UndoStateShape uss;
			uss.shapeName = shape->name.get();

			for (size_t i = 0; i < verts.size(); i++) {
				Vector3 diff = diffs[i];
				if (!mask.empty())
					diff *= 1.0f - mask[i];

				if (diff.IsZero(true))
					continue;

				uss.pointStartState[i] = Mesh::TransformPosNifToMesh(verts[i]);
				uss.pointEndState[i] = Mesh::TransformPosNifToMesh(verts[i] + diff);
			}

			if (spec.finishShape)
				spec.finishShape(shape, verts, uss);

			usp->usss.push_back(std::move(uss));
		}

		if (editor.bEditSlider) {
			usp->sliderName = editor.activeSlider;

			float sliderscale = project->SliderValue(editor.activeSlider);
			if (sliderscale == 0.0)
				sliderscale = 1.0;

			usp->sliderscale = sliderscale;
		}

		glView->ApplyUndoState(usp, false);

		previewApplied = true;

		if (glView->GetTransformMode())
			glView->ShowTransformTool();
	};

	auto uniform = [&]() { return spec.hasUniform && XRCCTRL(dlg, prefix + "Uniform", wxCheckBox)->IsChecked(); };

	auto sliderMoved = [&](wxCommandEvent& event) {
		float values[3];
		if (uniform()) {
			float uniformValue = ((wxSlider*)event.GetEventObject())->GetValue() / spec.sliderScale;
			for (int axis = 0; axis < 3; axis++) {
				values[axis] = uniformValue;
				slider(axis)->SetValue(uniformValue * spec.sliderScale);
			}
		}
		else {
			for (int axis = 0; axis < 3; axis++)
				values[axis] = slider(axis)->GetValue() / spec.sliderScale;
		}

		for (int axis = 0; axis < 3; axis++)
			text(axis)->ChangeValue(wxString::Format(spec.textFormat, values[axis]));

		updatePreview();
	};

	auto textChanged = [&](wxCommandEvent& event) {
		float values[3];
		if (uniform()) {
			float uniformValue = atof(((wxTextCtrl*)event.GetEventObject())->GetValue().c_str());
			for (int axis = 0; axis < 3; axis++) {
				if (textValue(axis) != uniformValue)
					text(axis)->ChangeValue(wxString::Format(spec.textFormat, uniformValue));
				values[axis] = uniformValue;
			}
		}
		else {
			for (int axis = 0; axis < 3; axis++)
				values[axis] = textValue(axis);
		}

		for (int axis = 0; axis < 3; axis++)
			slider(axis)->SetValue(values[axis] * spec.sliderScale);

		updatePreview();
	};

	auto previewChanged = [&](wxCommandEvent&) { updatePreview(); };

	for (int axis = 0; axis < 3; axis++) {
		slider(axis)->Bind(wxEVT_SLIDER, sliderMoved);
		text(axis)->Bind(wxEVT_TEXT, textChanged);
	}

	for (const char* checkBox : spec.checkBoxes)
		XRCCTRL(dlg, checkBox, wxCheckBox)->Bind(wxEVT_CHECKBOX, previewChanged);

	if (spec.hasOrigin)
		XRCCTRL(dlg, "origin", wxChoice)->Bind(wxEVT_CHOICE, previewChanged);

	dlg.Bind(wxEVT_CHAR_HOOK, &OutfitStudioFrame::OnEnterClose, this);

	if (dlg.ShowModal() != wxID_OK)
		undoPreview(true);

	UpdateUndoTools();
}

// Moves each vertex by the change of a matrix around the origin
static void MatrixDiffs(const Matrix4& xform, const std::vector<Vector3>& verts, std::vector<Vector3>& diffs) {
	for (size_t i = 0; i < verts.size(); i++)
		diffs[i] = xform * verts[i] - verts[i];
}

void OutfitStudioFrame::OnMoveShape(wxCommandEvent& WXUNUSED(event)) {
	ShapeTransformDialogSpec spec;
	spec.dialogName = "dlgMoveShape";
	spec.prefix = "ms";
	spec.checkBoxes = {"mirrorAxisX", "mirrorAxisY", "mirrorAxisZ"};
	spec.diffsForShape = [](wxDialog& dlg, NiShape*, const std::vector<Vector3>& verts, const Vector3& value, const Vector3&, std::vector<Vector3>& diffs) {
		bool mirrorAxisX = XRCCTRL(dlg, "mirrorAxisX", wxCheckBox)->IsChecked();
		bool mirrorAxisY = XRCCTRL(dlg, "mirrorAxisY", wxCheckBox)->IsChecked();
		bool mirrorAxisZ = XRCCTRL(dlg, "mirrorAxisZ", wxCheckBox)->IsChecked();

		for (size_t i = 0; i < verts.size(); i++) {
			Vector3 diff = value;
			if (mirrorAxisX && verts[i].x < 0.0f)
				diff.x = -diff.x;
			if (mirrorAxisY && verts[i].y < 0.0f)
				diff.y = -diff.y;
			if (mirrorAxisZ && verts[i].z < 0.0f)
				diff.z = -diff.z;
			diffs[i] = diff;
		}
		return true;
	};
	RunShapeTransformDialog(spec);
}

void OutfitStudioFrame::OnScaleShape(wxCommandEvent& WXUNUSED(event)) {
	ShapeTransformDialogSpec spec;
	spec.dialogName = "dlgScaleShape";
	spec.prefix = "ss";
	spec.identity = Vector3(1.0f, 1.0f, 1.0f);
	spec.hasUniform = true;
	spec.hasOrigin = true;
	spec.minValue = 0.01f;
	spec.diffsForShape = [](wxDialog&, NiShape*, const std::vector<Vector3>& verts, const Vector3& scale, const Vector3& origin, std::vector<Vector3>& diffs) {
		Matrix4 xform;
		xform.PushTranslate(origin);
		xform.PushScale(scale.x, scale.y, scale.z);
		xform.PushTranslate(origin * -1.0f);
		MatrixDiffs(xform, verts, diffs);
		return true;
	};
	RunShapeTransformDialog(spec);
}

void OutfitStudioFrame::OnRotateShape(wxCommandEvent& WXUNUSED(event)) {
	ShapeTransformDialogSpec spec;
	spec.dialogName = "dlgRotateShape";
	spec.prefix = "rs";
	spec.sliderScale = 100.0f;
	spec.textFormat = "%0.4f";
	spec.hasOrigin = true;
	spec.diffsForShape = [](wxDialog&, NiShape*, const std::vector<Vector3>& verts, const Vector3& angle, const Vector3& origin, std::vector<Vector3>& diffs) {
		Matrix4 xform;
		xform.PushTranslate(origin);
		xform.PushRotate(angle.x * DEG2RAD, Vector3(1.0f, 0.0f, 0.0f));
		xform.PushRotate(angle.y * DEG2RAD, Vector3(0.0f, 1.0f, 0.0f));
		xform.PushRotate(angle.z * DEG2RAD, Vector3(0.0f, 0.0f, 1.0f));
		xform.PushTranslate(origin * -1.0f);
		MatrixDiffs(xform, verts, diffs);
		return true;
	};
	RunShapeTransformDialog(spec);
}

void OutfitStudioFrame::OnInflateShape(wxCommandEvent& WXUNUSED(event)) {
	ShapeTransformDialogSpec spec;
	spec.dialogName = "dlgInflateShape";
	spec.prefix = "is";
	spec.hasUniform = true;
	spec.diffsForShape = [this](wxDialog&, NiShape* shape, const std::vector<Vector3>& verts, const Vector3& inflate, const Vector3&, std::vector<Vector3>& diffs) {
		Mesh* mesh = glView->GetMesh(shape->name.get());
		if (!mesh || !mesh->norms)
			return false;

		if (!mesh->bGotWeldVerts)
			mesh->CalcWeldVerts();

		for (size_t i = 0; i < verts.size(); i++)
			diffs[i] = Mesh::TransformDirMeshToNif(mesh->norms[i]).ComponentMultiply(inflate);
		return true;
	};
	// Welded vertices move together, to where the first of them went
	spec.finishShape = [this](NiShape* shape, const std::vector<Vector3>& verts, UndoStateShape& uss) {
		Mesh* mesh = glView->GetMesh(shape->name.get());
		for (size_t i = 0; i < verts.size(); i++) {
			auto pointEndStateIt = uss.pointEndState.find(i);
			if (pointEndStateIt != uss.pointEndState.end()) {
				mesh->DoForEachWeldedVertex(i, [&](int p) {
					auto pointEndStateWeldIt = uss.pointEndState.find(p);
					if (pointEndStateWeldIt != uss.pointEndState.end())
						pointEndStateWeldIt->second = pointEndStateIt->second;
				});
			}
		}
	};
	RunShapeTransformDialog(spec);
}

void OutfitStudioFrame::OnDeleteVerts(wxCommandEvent& WXUNUSED(event)) {
	if (!ShapeSelectionCheck())
		return;

	if (editor.bEditSlider) {
		wxMessageBox(_("You're currently editing slider data, please exit the slider's edit mode (pencil button) and try again."));
		return;
	}

	// Prepare the undo data and determine if any shapes are being deleted.
	UndoStateProject* usp = glView->GetUndoHistory()->PushState();
	usp->undoType = UndoType::Mesh;
	std::vector<NiShape*> delShapes;
	for (auto& i : selectedItems) {
		if (editUV && editUV->shape == i->GetShape())
			editUV->Close();

		std::unordered_map<uint16_t, float> mask;
		glView->GetShapeUnmasked(mask, i->GetShape()->name.get());
		UndoStateShape uss;
		uss.shapeName = i->GetShape()->name.get();
		if (project->PrepareDeleteVerts(i->GetShape(), mask, uss))
			delShapes.push_back(i->GetShape());
		else
			usp->usss.push_back(std::move(uss));
	}

	// Confirm deleting shapes; then delete them.
	if (!delShapes.empty()) {
		if (wxMessageBox(_("Are you sure you wish to delete parts of the selected shapes?"), _("Confirm Delete"), wxYES_NO) == wxNO)
			return;

		for (NiShape* shape : delShapes) {
			usp->deletedShapes.emplace_back();
			project->CaptureShapeDeleteState(shape, usp->deletedShapes.back());
			project->DeleteShape(shape);
		}
	}

	// Now do the vertex deletion
	std::unordered_map<std::string, std::vector<float>> maskStash = glView->StashMasks();
	for (auto& uss : usp->usss) {
		NiShape* shape = project->GetWorkNif()->FindBlockByName<NiShape>(uss.shapeName);
		if (!shape)
			continue;
		project->ApplyShapeMeshUndo(shape, maskStash[uss.shapeName], uss, false);
	}

	project->GetWorkAnim()->CleanupBones();

	RefreshGUIFromProj(false, false);
	SetPendingChanges();

	glView->UnstashMasks(maskStash);
	ApplySliders();
}

void OutfitStudioFrame::OnSeparateVerts(wxCommandEvent& WXUNUSED(event)) {
	if (!ShapeSelectionCheck())
		return;

	if (editor.bEditSlider) {
		wxMessageBox(_("You're currently editing slider data, please exit the slider's edit mode (pencil button) and try again."));
		return;
	}

	std::unordered_map<uint16_t, float> masked;
	glView->GetShapeMask(masked, activeItem->GetShape()->name.get());
	if (masked.empty())
		return;

	std::string shapeName = activeItem->GetShape()->name.get();
	std::string newShapeName;
	do {
		std::string result{GetTextFromUser(_("Please enter a unique name for the new separated shape."), _("Separate Vertices..."), wxString::FromUTF8(shapeName), this).ToUTF8()};
		if (result.empty())
			return;

		newShapeName = std::move(result);
	} while (project->IsValidShape(newShapeName));

	if (editUV && editUV->shape == activeItem->GetShape())
		editUV->Close();

	auto newShape = project->DuplicateShape(activeItem->GetShape(), newShapeName);

	UndoStateProject* usp = glView->GetUndoHistory()->PushState();
	usp->undoType = UndoType::Mesh;
	usp->usss.resize(2);
	usp->usss[0].shapeName = shapeName;
	usp->usss[1].shapeName = newShapeName;

	std::unordered_map<uint16_t, float> unmasked = masked;
	glView->InvertMaskTris(unmasked, shapeName);

	project->PrepareDeleteVerts(activeItem->GetShape(), masked, usp->usss[0]);
	project->PrepareDeleteVerts(newShape, unmasked, usp->usss[1]);

	std::unordered_map<std::string, std::vector<float>> maskStash = glView->StashMasks();

	project->ApplyShapeMeshUndo(activeItem->GetShape(), maskStash[usp->usss[0].shapeName], usp->usss[0], false);
	project->ApplyShapeMeshUndo(newShape, maskStash[usp->usss[1].shapeName], usp->usss[1], false);

	project->SetTextures();
	RefreshGUIFromProj(false, false);
	SetPendingChanges();

	glView->UnstashMasks(maskStash);
	ApplySliders();
}

MergeCheckErrors OutfitStudioFrame::CheckCopyGeo(wxDialog& dlg) {
	wxStaticText* errors = XRCCTRL(dlg, "copyGeometryErrors", wxStaticText);
	wxChoice* sourceChoice = XRCCTRL(dlg, "sourceChoice", wxChoice);
	wxChoice* targetChoice = XRCCTRL(dlg, "targetChoice", wxChoice);

	std::string source = sourceChoice->GetString(sourceChoice->GetSelection()).ToStdString();
	std::string target = targetChoice->GetString(targetChoice->GetSelection()).ToStdString();

	MergeCheckErrors e;
	project->CheckMerge(source, target, e);
	XRCCTRL(dlg, "wxID_OK", wxButton)->Enable(e.canMerge);
	const bool hasWarnings = e.partitionsMismatch || e.segmentsMismatch || e.textureMismatch || e.transformsMismatch;

	if (e.canMerge && !hasWarnings) {
		errors->SetLabel(_("No errors found!"));
		dlg.SetSize(dlg.GetBestSize());
		return e;
	}

	wxString errorLines;
	if (e.shapesSame)
		errorLines << "\n- " << _("Target must be different from source.");
	if (e.tooManyVertices)
		errorLines << "\n- " << _("Resulting shape would have too many vertices.");
	if (e.tooManyTriangles)
		errorLines << "\n- " << _("Resulting shape would have too many triangles.");
	if (e.shaderMismatch)
		errorLines << "\n- " << _("Shaders do not match. Make sure both shapes either have or don't have a shader and their shader type matches.");
	if (e.alphaPropMismatch)
		errorLines << "\n- " << _("Alpha property mismatch. Make sure both shapes either have or don't have an alpha property and their flags + threshold match.");

	wxString warningLines;
	if (e.partitionsMismatch)
		warningLines << "\n- " << _("Partitions do not match. Merge will auto-reconcile matching slots and create missing partitions.");
	if (e.segmentsMismatch)
		warningLines << "\n- " << _("Segments do not match. Merge will auto-reconcile matching IDs and create missing segments/sub segments.");
	if (e.textureMismatch)
		warningLines << "\n- " << _("Base texture doesn't match. Merge will use texture paths from the target shape.");
	if (e.transformsMismatch)
		warningLines << "\n- " << _("Transforms do not match. Merge will apply the transforms of both shapes to their geometry and clear them, without moving the meshes.");

	wxString msg;
	if (!errorLines.empty())
		msg << _("Errors:") << errorLines;
	if (hasWarnings) {
		if (!msg.empty())
			msg << "\n";
		msg << _("Warnings:") << warningLines;
	}

	errors->SetLabel(msg);
	dlg.SetSize(dlg.GetBestSize());
	return e;
}

void OutfitStudioFrame::OnCopyGeo(wxCommandEvent& WXUNUSED(event)) {
	if (editor.bEditSlider) {
		wxMessageBox(_("You're currently editing slider data, please exit the slider's edit mode (pencil button) and try again."));
		return;
	}

	wxDialog dlg;
	if (!wxXmlResource::Get()->LoadDialog(&dlg, this, "dlgCopyGeometry"))
		return;

	wxChoice* sourceChoice = XRCCTRL(dlg, "sourceChoice", wxChoice);
	wxChoice* targetChoice = XRCCTRL(dlg, "targetChoice", wxChoice);
	if (!sourceChoice || !targetChoice)
		return;

	std::string sourceShapeName;
	if (activeItem)
		sourceShapeName = activeItem->GetShape()->name.get();

	std::vector<std::string> shapeList = GetShapeList();
	if (shapeList.size() < 2)
		return;

	CloseBrushSettings();

	for (const std::string& shape : shapeList) {
		sourceChoice->AppendString(shape);
		targetChoice->AppendString(shape);
		if (shape == sourceShapeName)
			sourceChoice->SetSelection(sourceChoice->GetCount() - 1);
	}

	if (sourceChoice->GetSelection() == wxNOT_FOUND)
		sourceChoice->SetSelection(0);
	if (sourceChoice->GetSelection() == 0)
		targetChoice->SetSelection(1);
	else
		targetChoice->SetSelection(0);

	MergeCheckErrors mergeErrors;
	sourceChoice->Bind(wxEVT_CHOICE, [this, &dlg, &mergeErrors](wxCommandEvent&) { mergeErrors = CheckCopyGeo(dlg); });
	targetChoice->Bind(wxEVT_CHOICE, [this, &dlg, &mergeErrors](wxCommandEvent&) { mergeErrors = CheckCopyGeo(dlg); });
	mergeErrors = CheckCopyGeo(dlg);
	dlg.CenterOnParent();

	if (dlg.ShowModal() != wxID_OK)
		return;

	sourceShapeName = sourceChoice->GetString(sourceChoice->GetSelection()).ToStdString();
	std::string targetShapeName = targetChoice->GetString(targetChoice->GetSelection()).ToStdString();

	NiShape* sourceShape = project->GetWorkNif()->FindBlockByName<NiShape>(sourceShapeName);
	if (!sourceShape)
		return;
	NiShape* targetShape = project->GetWorkNif()->FindBlockByName<NiShape>(targetShapeName);
	if (!targetShape)
		return;

	UndoStateProject* usp = glView->GetUndoHistory()->PushState();
	usp->undoType = UndoType::Mesh;
	usp->usss.resize(1);
	usp->usss[0].shapeName = targetShapeName;

	project->PrepareCopyGeo(sourceShape, targetShape, usp->usss[0]);

	std::unordered_map<std::string, std::vector<float>> maskStash = glView->StashMasks();

	project->ApplyShapeMeshUndo(targetShape, maskStash[usp->usss[0].shapeName], usp->usss[0], false);

	if (mergeErrors.textureMismatch)
		project->SetTextures(targetShape);

	if (XRCCTRL(dlg, "checkDeleteSource", wxCheckBox)->IsChecked())
		project->DeleteShape(sourceShape);

	RefreshGUIFromProj(false, false);
	SetPendingChanges();
	glView->UnstashMasks(maskStash);
	ApplySliders();
}

void OutfitStudioFrame::OnDupeShape(wxCommandEvent& WXUNUSED(event)) {
	std::string newName;
	wxTreeItemId subitem;
	if (activeItem) {
		if (!outfitRoot.IsOk()) {
			wxMessageBox(_("You can only copy shapes into an outfit, and there is no outfit in the current project. Load one first!"));
			return;
		}

		CloseBrushSettings();

		std::string shapeName = activeItem->GetShape()->name.get();
		do {
			std::string result{GetTextFromUser(_("Please enter a unique name for the duplicated shape."), _("Duplicate Shape"), wxString::FromUTF8(shapeName), this).ToUTF8()};
			if (result.empty())
				return;

			newName = std::move(result);
		} while (project->IsValidShape(newName));

		wxLogMessage("Duplicating shape '%s' as '%s'.", shapeName, newName);

		auto shape = project->DuplicateShape(activeItem->GetShape(), newName);
		if (shape) {
			glView->AddMeshFromNif(project->GetWorkNif(), newName);
			UpdateMeshFromSet(shape);
			project->SetTextures(shape);

			MaterialFile matFile;
			bool hasMatFile = project->GetShapeMaterialFile(shape, matFile);
			glView->SetMeshTextures(newName, project->GetShapeTextures(shape), hasMatFile, matFile, false, project->GetShapeSFMaterial(shape));

			subitem = outfitShapes->AppendItem(outfitRoot, wxString::FromUTF8(newName));
			outfitShapes->SetItemState(subitem, 0);
			outfitShapes->SetItemData(subitem, new ShapeItemData(shape));

			outfitShapes->UnselectAll();
			outfitShapes->SelectItem(subitem);
			SetPendingChanges();
		}
	}
}

void OutfitStudioFrame::OnRefineMesh(wxCommandEvent& WXUNUSED(event)) {
	if (!ShapeSelectionCheck())
		return;

	glView->gls.DeleteOverlay("refineErrorEdges");

	if (editor.bEditSlider) {
		wxMessageBox(_("You're currently editing slider data, please exit the slider's edit mode (pencil button) and try again."));
		return;
	}

	if (selectedItems.size() != 1) {
		wxMessageBox(_("There is more than one shape selected."), _("Error"));
		return;
	}

	// Determine unmasked vertices
	NiShape* shape = selectedItems[0]->GetShape();
	size_t nverts = shape->GetNumVertices();
	std::unordered_map<uint16_t, float> mask;
	glView->GetShapeUnmasked(mask, shape->name.get());
	std::vector<bool> pincs(nverts, false);
	for (auto& m : mask)
		pincs[m.first] = true;

	// Count unmasked edges
	std::vector<Triangle> tris;
	shape->GetTriangles(tris);
	size_t nedges = 0;
	for (Triangle& tri : tris) {
		int ntripts = pincs[tri.p1] + pincs[tri.p2] + pincs[tri.p3];
		if (ntripts == 3)
			nedges += 3;
		else if (ntripts == 2)
			nedges += 1;
	}

	// Determine maximum possible vertices and triangles for this Nif type
	auto workNif = project->GetWorkNif();
	if (!workNif)
		return;

	constexpr size_t maxVertIndex = std::numeric_limits<uint16_t>().max();
	size_t maxTriIndex = workNif->GetTriangleLimit();

	// Check if we'll overflow the vertex or triangle arrays.
	// Note that the actual number of vertices added could be as low as
	// nedges/2; nedges is the upper bound (if every edge is welded or
	// boundary).
	if (nverts + nedges > maxVertIndex) {
		wxMessageBox(_("The shape has reached the vertex count limit."), _("Error"), wxICON_ERROR);
		return;
	}

	if (shape->GetNumTriangles() + nedges > maxTriIndex) {
		wxMessageBox(_("The shape has reached the triangle count limit."), _("Error"), wxICON_ERROR);
		return;
	}

	// Prepare list of changes
	UndoStateShape uss;
	uss.shapeName = shape->name.get();
	Mesh* m = glView->GetMesh(shape->name.get());
	std::vector<Edge> badEdges;
	if (!project->PrepareRefineMesh(shape, uss, pincs, m->weldVerts, false, &badEdges)) {
		if (!badEdges.empty()) {
			glView->gls.AddVisEdges(m, badEdges, "refineErrorEdges");

			// Add bad edge vertices to the mask
			for (const auto& e : badEdges) {
				m->mask[e.p1] = 1.0f;
				m->mask[e.p2] = 1.0f;
			}
			m->QueueUpdate(Mesh::UpdateType::Mask);
		}

		wxMessageBox(_("Some edges have multiple triangles of the same orientation. They have been highlighted and masked. Correct the orientations before refining."), _("Error"), wxICON_ERROR);
		glView->Render();
		return;
	}

	// Push changes onto undo stack and execute.
	UndoStateProject* usp = glView->GetUndoHistory()->PushState();
	usp->undoType = UndoType::Mesh;
	usp->usss.push_back(std::move(uss));
	glView->ApplyUndoState(usp, false);

	UpdateUndoTools();
	SetPendingChanges();
}

void OutfitStudioFrame::OnDeleteShape(wxCommandEvent& WXUNUSED(event)) {
	if (currentTabButton == meshTabButton) {
		if (editor.bEditSlider) {
			wxMessageBox(_("Can't delete shape while in slider edit mode.  Use CTRL+Delete to delete sliders instead."), _("Error"));
			return;
		}

		if (!ShapeSelectionCheck())
			return;

		// Delete shape(s) when in meshes tab
		if (wxMessageBox(_("Are you sure you wish to delete the selected shapes?"), _("Confirm Delete"), wxYES_NO | wxICON_WARNING) == wxNO)
			return;

		UndoStateProject* usp = glView->GetUndoHistory()->PushState();
		usp->undoType = UndoType::ShapeDelete;

		std::vector<ShapeItemData> selected;
		for (auto& i : selectedItems)
			selected.push_back(*i);

		activeItem = nullptr;
		selectedItems.clear();

		for (auto& i : selected) {
			if (editUV && editUV->shape == i.GetShape())
				editUV->Close();

			std::string shapeName = i.GetShape()->name.get();
			wxLogMessage("Deleting shape '%s'.", shapeName);

			usp->deletedShapes.emplace_back();
			project->CaptureShapeDeleteState(i.GetShape(), usp->deletedShapes.back());

			project->DeleteShape(i.GetShape());
			wxTreeItemId item = i.GetId();
			outfitShapes->Delete(item);
		}

		SetPendingChanges();
		UpdateAnimationGUI();
		UpdateUndoTools();
		glView->Render();
	}
	else if (currentTabButton == boneTabButton) {
		// Delete bone(s) when in bones tab
		bool shiftDown = wxGetKeyState(WXK_SHIFT);
		bool ctrlDown = wxGetKeyState(WXK_CONTROL);
		if (shiftDown && ctrlDown) {
			if (wxMessageBox(_("Delete selected bones?  This action cannot be undone."), _("Confirm Delete"), wxYES_NO | wxICON_WARNING) == wxNO)
				return;

			wxCommandEvent evt;
			OnDeleteBone(evt);
		}
		else {
			if (!ShapeSelectionCheck())
				return;

			if (wxMessageBox(_("Delete bones from selected shape(s)?  This action cannot be undone."), _("Confirm Delete"), wxYES_NO | wxICON_WARNING) == wxNO)
				return;

			wxCommandEvent evt;
			OnDeleteBoneFromSelected(evt);
		}
	}
	else if (currentTabButton == partitionTabButton) {
		if (wxMessageBox(_("Delete partition?  This action cannot be undone."), _("Confirm Delete"), wxYES_NO | wxICON_WARNING) == wxNO)
			return;

		segPartEditor->DeleteSelectedPartition();
	}
	else if (currentTabButton == segmentTabButton) {
		if (wxMessageBox(_("Delete segment?  This action cannot be undone."), _("Confirm Delete"), wxYES_NO | wxICON_WARNING) == wxNO)
			return;

		segPartEditor->DeleteSelectedSegment();
	}
}

bool OutfitStudioFrame::ShowVertexAsym(Mesh* m, const SymmetricVertices& symverts, const VertexAsymmetries& asyms, VertexAsymmetryTasks& tasks, const std::vector<bool>& selVerts, bool trize) {
	// This dialog has two versions, one for OnMaskSymVert and one for
	// OnSymVert.  trize tells us which version to do.

	CloseBrushSettings();

	int nSliders = static_cast<int>(asyms.sliders.size());
	int nBones = static_cast<int>(asyms.bones.size());
	int nUnmatched = static_cast<int>(symverts.unmatched.size());

	VertexAsymmetryStats stats;
	CalcVertexAsymmetryStats(symverts, asyms, selVerts, stats);

	// Initialize the portion of tasks that doesn't get initialized by
	// member initialization.
	tasks.doSliders.resize(nSliders, false);
	tasks.doBones.resize(nBones, false);

	wxDialog dlg;
	if (!wxXmlResource::Get()->LoadDialog(&dlg, this, "dlgVertexAsym"))
		return false;

	// Fill in static labels
	XRCCTRL(dlg, "currentUnmaskedText", wxStaticText)->SetLabel(wxString() << stats.unmaskedCount);
	XRCCTRL(dlg, "unmatchedVerticesText", wxStaticText)->SetLabel(wxString() << nUnmatched);
	XRCCTRL(dlg, "posAvgText", wxStaticText)->SetLabel(wxString() << stats.posAvg);
	XRCCTRL(dlg, "positionText", wxStaticText)->SetLabel(wxString() << stats.posCount);
	XRCCTRL(dlg, "anySliderLabel", wxStaticText)->SetLabel(wxString() << nSliders << _(" sliders"));
	XRCCTRL(dlg, "anySliderText", wxStaticText)->SetLabel(wxString() << stats.anySliderCount);
	XRCCTRL(dlg, "anyBoneLabel", wxStaticText)->SetLabel(wxString() << nBones << _(" bones"));
	XRCCTRL(dlg, "anyBoneText", wxStaticText)->SetLabel(wxString() << stats.anyBoneCount);

	// The controls we need later
	wxCheckBox* cbUnmatched = XRCCTRL(dlg, "checkUnmatched", wxCheckBox);
	wxCheckBox* cbPosition = XRCCTRL(dlg, "checkPosition", wxCheckBox);
	wxCheckBox* cbAnySlider = XRCCTRL(dlg, "checkAnySlider", wxCheckBox);
	wxCheckBox* cbAnyBone = XRCCTRL(dlg, "checkAnyBone", wxCheckBox);
	std::vector<wxCheckBox*> cbSliders(nSliders, nullptr);
	std::vector<wxCheckBox*> cbBones;
	wxScrolledWindow* asymScroll = XRCCTRL(dlg, "asymScroll", wxScrolledWindow);
	wxStaticText* stillUnmaskedText = XRCCTRL(dlg, "stillUnmaskedText", wxStaticText);

	// UpdateStillUnmasked: calculates value for the "still unmasked"
	// wxStaticText.
	auto UpdateStillUnmasked = [&]() {
		int stillUnmasked = 0;
		if (tasks.doUnmatched)
			stillUnmasked += nUnmatched;
		std::vector<bool> doVert = CalcVertexListForAsymmetryTasks(symverts, asyms, tasks, m->nVerts);
		for (int vi = 0; vi < m->nVerts; ++vi)
			if (selVerts[vi] && doVert[vi])
				++stillUnmasked;
		stillUnmaskedText->SetLabel(wxString() << stillUnmasked);
	};

	// UpdateChecks: update the individual slider check boxes, the any-slider
	// check box, and the any-bone check box.  Also calls UpdateStillUnmasked.
	auto UpdateChecks = [&]() {
		int sliderCount = 0;
		for (int sai = 0; sai < nSliders; ++sai) {
			cbSliders[sai]->SetValue(tasks.doSliders[sai]);
			sliderCount += tasks.doSliders[sai];
		}
		cbAnySlider->Set3StateValue(sliderCount == nSliders ? wxCHK_CHECKED : sliderCount == 0 ? wxCHK_UNCHECKED : wxCHK_UNDETERMINED);

		int boneCount = 0, totalBones = 0;
		for (int bai = 0; bai < nBones; ++bai) {
			if (!stats.boneCounts[bai])
				continue;
			++totalBones;
			boneCount += tasks.doBones[bai];
		}
		cbAnyBone->Set3StateValue(boneCount == totalBones ? wxCHK_CHECKED : boneCount == 0 ? wxCHK_UNCHECKED : wxCHK_UNDETERMINED);

		UpdateStillUnmasked();
	};

	// Checkbox event handling for the checkboxes that are not in either
	// of the collapsible panes.
	cbUnmatched->Bind(wxEVT_CHECKBOX, [&](wxCommandEvent& e) {
		tasks.doUnmatched = e.IsChecked();
		UpdateStillUnmasked();
	});
	cbPosition->Bind(wxEVT_CHECKBOX, [&](wxCommandEvent& e) {
		tasks.doPos = e.IsChecked();
		UpdateStillUnmasked();
	});
	cbAnySlider->Bind(wxEVT_CHECKBOX, [&](wxCommandEvent& e) {
		bool newval = e.IsChecked();
		for (int sai = 0; sai < nSliders; ++sai)
			tasks.doSliders[sai] = newval;
		UpdateChecks();
	});
	cbAnyBone->Bind(wxEVT_CHECKBOX, [&](wxCommandEvent& e) {
		bool newval = e.IsChecked();
		for (int bai = 0; bai < nBones; ++bai)
			if (stats.boneCounts[bai])
				tasks.doBones[bai] = newval;
		for (size_t bci = 0; bci < cbBones.size(); ++bci)
			cbBones[bci]->SetValue(newval);
		UpdateChecks();
	});

	// Fill in Sliders collapsible pane
	wxCollapsiblePane* slidersCollapse = XRCCTRL(dlg, "slidersCollapse", wxCollapsiblePane);
	wxWindow* slidersPane = slidersCollapse->GetPane();
	wxFlexGridSizer* slidersSz = new wxFlexGridSizer(3);
	slidersSz->AddGrowableCol(0, 1);
	for (int sai = 0; sai < nSliders; ++sai) {
		const auto& sa = asyms.sliders[sai];
		wxCheckBox* cb = new wxCheckBox(slidersPane, wxID_ANY, sa.sliderName);
		slidersSz->Add(cb, 0, wxLEFT|wxRIGHT, 5);
		cbSliders[sai] = cb;
		slidersSz->Add(new wxStaticText(slidersPane, wxID_ANY, wxString() << stats.sliderAvgs[sai]), 0, wxRIGHT|wxALIGN_RIGHT, 20);
		slidersSz->Add(new wxStaticText(slidersPane, wxID_ANY, wxString() << stats.sliderCounts[sai]), 0, wxLEFT|wxRIGHT|wxALIGN_RIGHT, 5);
		cb->Bind(wxEVT_CHECKBOX, [&, sai](wxCommandEvent& e) {
			tasks.doSliders[sai] = e.IsChecked();
			UpdateChecks();
		});
	}
	slidersPane->SetSizerAndFit(slidersSz);
	slidersCollapse->Bind(wxEVT_COLLAPSIBLEPANE_CHANGED, [&asymScroll](wxCollapsiblePaneEvent&) { asymScroll->FitInside(); });

	// Fill in Bones collapsible pane
	wxCollapsiblePane* bonesCollapse = XRCCTRL(dlg, "bonesCollapse", wxCollapsiblePane);
	wxWindow* bonesPane = bonesCollapse->GetPane();
	wxFlexGridSizer* bonesSz = new wxFlexGridSizer(3);
	bonesSz->AddGrowableCol(0, 1);
	for (int bai = 0; bai < nBones; ++bai) {
		const auto& ba = asyms.bones[bai];
		// If a bone has no asymmetries but its mirror bone does, it's still
		// listed in asyms.bones.  But that doesn't mean we need to show it
		// to the user.
		if (!stats.boneCounts[bai])
			continue;

		wxCheckBox* cb = new wxCheckBox(bonesPane, wxID_ANY, ba.boneName);
		bonesSz->Add(cb, 0, wxLEFT|wxRIGHT, 5);
		cbBones.push_back(cb);
		bonesSz->Add(new wxStaticText(bonesPane, wxID_ANY, wxString() << stats.boneAvgs[bai]), 0, wxRIGHT|wxALIGN_RIGHT, 20);
		bonesSz->Add(new wxStaticText(bonesPane, wxID_ANY, wxString() << stats.boneCounts[bai]), 0, wxLEFT|wxRIGHT|wxALIGN_RIGHT, 5);
		cb->Bind(wxEVT_CHECKBOX, [&, bai](wxCommandEvent& e) {
			tasks.doBones[bai] = e.IsChecked();
			UpdateChecks();
		});
	}
	bonesPane->SetSizerAndFit(bonesSz);
	bonesCollapse->Bind(wxEVT_COLLAPSIBLEPANE_CHANGED, [&asymScroll](wxCollapsiblePaneEvent&) { asymScroll->FitInside(); });

	// Disable irrelevant checkboxes
	auto DisableCheck = [](wxCheckBox* cb) {
		cb->SetValue(false);
		cb->Disable();
	};
	if (!nUnmatched)
		DisableCheck(cbUnmatched);
	if (!stats.posCount)
		DisableCheck(cbPosition);
	if (!stats.anySliderCount)
		DisableCheck(cbAnySlider);
	if (!stats.anyBoneCount)
		DisableCheck(cbAnyBone);

	// Customize the dialog for OnSymVert
	if (trize) {
		dlg.SetTitle(_("Symmetrize Vertices"));
		cbUnmatched->Hide();
		auto desc = _("Eliminates the selected asymmetries from unmasked vertices by adjusting vertex data to be consistent with mirror vertices.");
		if (nBones)
			desc += _("  (Hint: To choose which non-selected bones are adjusted during weight normalization, unlock them in the bones list in the Bones tab.)");
		wxStaticText* descST = XRCCTRL(dlg, "maskSymmetricVerticesDescription", wxStaticText);
		descST->SetLabel(desc);
		descST->Wrap(600);
		XRCCTRL(dlg, "stillUnmaskedLabel", wxStaticText)->SetLabel(_("Vertices that will be symmetrized:"));
		XRCCTRL(dlg, "wxID_OK", wxButton)->SetLabel(_("&Symmetrize"));
	}

	UpdateStillUnmasked();
	dlg.SetSize(dlg.GetBestSize());
	dlg.CenterOnParent();

	if (dlg.ShowModal() != wxID_OK)
		return false;

	return true;
}

void OutfitStudioFrame::OnMaskSymVert(wxCommandEvent& WXUNUSED(event)) {
	if (!ShapeSelectionCheck())
		return;

	NiShape* s = activeItem->GetShape();
	std::string shapeName = s->name.get();
	Mesh* m = glView->gls.GetMesh(shapeName);
	if (!m)
		return;

	std::vector<bool> selVerts(m->nVerts);
	for (int i = 0; i < m->nVerts; ++i)
		selVerts[i] = m->mask[i] == 0.0f;

	SymmetricVertices r;
	project->MatchSymmetricVertices(s, m->weldVerts, r);

	VertexAsymmetries a;
	project->FindVertexAsymmetries(s, r, m->weldVerts, a);

	VertexAsymmetryTasks tasks;
	if (!ShowVertexAsym(m, r, a, tasks, selVerts, false))
		return;

	UndoStateProject* usp = glView->GetUndoHistory()->PushState();
	usp->undoType = UndoType::Mask;
	usp->usss.resize(1);
	UndoStateShape& uss = usp->usss[0];
	uss.shapeName = shapeName;

	// Mask vertices based on selected tasks.
	std::vector<bool> doVert = CalcVertexListForAsymmetryTasks(r, a, tasks, m->nVerts);
	AddWeldedToVertexList(m->weldVerts, doVert);
	for (int i = 0; i < m->nVerts; ++i) {
		if (selVerts[i] && doVert[i])
			continue;
		uss.pointStartState[i].x = m->mask[i];
		uss.pointEndState[i].x = 1.0f;
	};

	glView->ApplyUndoState(usp, false);

	if (!Config.GetBoolValue("Input/MaskHistory"))
		glView->GetUndoHistory()->PopState();

	UpdateUndoTools();
}

void OutfitStudioFrame::OnSymVert(wxCommandEvent& WXUNUSED(event)) {
	if (!ShapeSelectionCheck())
		return;

	NiShape* s = activeItem->GetShape();
	std::string shapeName = s->name.get();
	Mesh* m = glView->gls.GetMesh(shapeName);
	if (!m)
		return;

	std::vector<bool> selVerts(m->nVerts);
	for (int i = 0; i < m->nVerts; ++i)
		selVerts[i] = m->mask[i] == 0.0f;

	SymmetricVertices r;
	project->MatchSymmetricVertices(s, m->weldVerts, r);

	VertexAsymmetries a;
	project->FindVertexAsymmetries(s, r, m->weldVerts, a);

	VertexAsymmetryTasks tasks;
	if (!ShowVertexAsym(m, r, a, tasks, selVerts, true))
		return;

	std::vector<std::string> normBones, notNormBones;
	GetNormalizeBones(&normBones, &notNormBones);

	UndoStateProject* usp = glView->GetUndoHistory()->PushState();
	usp->undoType = UndoType::Mesh;
	usp->usss.resize(1);
	UndoStateShape& uss = usp->usss[0];
	uss.shapeName = shapeName;

	project->PrepareSymmetrizeVertices(s, uss, r, a, tasks, m->weldVerts, selVerts, normBones, notNormBones);

	glView->ApplyUndoState(usp, false);
	UpdateUndoTools();
}

void OutfitStudioFrame::OnMaskSymTri(wxCommandEvent& WXUNUSED(event)) {
	if (selectedItems.empty())
		return;

	UndoStateProject* usp = glView->GetUndoHistory()->PushState();
	usp->undoType = UndoType::Mask;

	for (auto& selItem : selectedItems) {
		NiShape* shape = selItem->GetShape();
		if (!shape)
			continue;
		std::string shapeName = shape->name.get();

		Mesh* m = glView->gls.GetMesh(shapeName);
		if (!m)
			return;

		usp->usss.emplace_back();
		UndoStateShape& uss = usp->usss.back();
		uss.shapeName = shapeName;

		std::vector<bool> amask = project->CalculateAsymmetricTriangleVertexMask(shape, m->weldVerts);

		// Mask vertices that are _not_ in amask.
		for (int vi = 0; vi < m->nVerts; ++vi)
			if (m->mask[vi] != 1.0f && !amask[vi]) {
				uss.pointStartState[vi].x = m->mask[vi];
				uss.pointEndState[vi].x = 1.0f;
			}
	}

	glView->ApplyUndoState(usp, false);

	if (!Config.GetBoolValue("Input/MaskHistory"))
		glView->GetUndoHistory()->PopState();

	UpdateUndoTools();
}

void OutfitStudioFrame::OnResetTransforms(wxCommandEvent& WXUNUSED(event)) {
	project->ResetTransforms();
	RefreshGUIFromProj();
	SetPendingChanges();
}

void OutfitStudioFrame::OnDeleteUnreferencedNodes(wxCommandEvent& WXUNUSED(event)) {
	int deletionCount = 0;
	auto workNif = project->GetWorkNif();
	if (workNif)
		workNif->DeleteUnreferencedNodes(&deletionCount);

	if (deletionCount > 0)
		SetPendingChanges();

	wxString msg = wxString::Format(_("%d unreferenced nodes were deleted."), deletionCount);
	wxMessageBox(msg, _("Delete Unreferenced Nodes"));
}

void OutfitStudioFrame::OnRemoveSkinning(wxCommandEvent& WXUNUSED(event)) {
	project->RemoveSkinning();
	RefreshGUIFromProj();
	SetPendingChanges();
}

void OutfitStudioFrame::OnShapeProperties(wxCommandEvent& WXUNUSED(event)) {
	CloseBrushSettings();

	if (!ShapeSelectionCheck())
		return;

	std::vector<NiShape*> selectedShapes;
	for (auto& s : selectedItems)
		selectedShapes.push_back(s->GetShape());

	auto shape = activeItem->GetShape();
	if (shape) {
		ShapeProperties prop(this, project->GetWorkNif(), selectedShapes);
		prop.ShowModal();
	}
}

void OutfitStudioFrame::OnRecalcNormals(wxCommandEvent& WXUNUSED(event)) {
	for (auto& s : selectedItems)
		glView->RecalcNormals(s->GetShape()->name.get(), true);

	glView->Render();
}

void OutfitStudioFrame::OnDisableNormalsCalc(wxCommandEvent& event) {
	OutfitStudioConfig.SetBoolValue("DisableNormalsCalc", event.IsChecked());
}

void OutfitStudioFrame::OnSmoothNormalSeams(wxCommandEvent& event) {
	bool enable = event.IsChecked();
	glView->SetNormalSeamSmoothMode(enable);

	for (auto& s : selectedItems)
		project->activeSet.SetSmoothSeamNormals(s->GetShape()->name.get(), enable);

	menuBar->Enable(XRCID("btnSmoothSeamsAngle"), enable);

	glView->Render();
}

void OutfitStudioFrame::OnSmoothSeamsAngle(wxCommandEvent& WXUNUSED(event)) {
	if (!ShapeSelectionCheck())
		return;

	std::vector<Mesh*> activeMeshes = glView->gls.GetActiveMeshes();
	if (activeMeshes.empty())
		return;

	std::vector<float> oldMeshAngles;
	oldMeshAngles.resize(activeMeshes.size());

	for (size_t i = 0; i < activeMeshes.size(); i++)
		oldMeshAngles[i] = activeMeshes[i]->smoothSeamNormalsAngle;

	wxDialog dlg;
	if (wxXmlResource::Get()->LoadDialog(&dlg, this, "dlgSmoothSeams")) {
		auto updatePreview = [&]() {
			float angle = atof(XRCCTRL(dlg, "angleText", wxTextCtrl)->GetValue().c_str());

			for (size_t i = 0; i < activeMeshes.size(); i++) {
				Mesh* m = activeMeshes[i];
				m->smoothSeamNormalsAngle = angle;
				m->SmoothNormals();
			}

			glView->Render();
		};

		auto sliderMoved = [&](wxCommandEvent&) {
			float angle = XRCCTRL(dlg, "angleSlider", wxSlider)->GetValue() / 100.0f;
			XRCCTRL(dlg, "angleText", wxTextCtrl)->ChangeValue(wxString::Format("%0.2f", angle));

			updatePreview();
		};

		auto textChanged = [&](wxCommandEvent&) {
			float angle = atof(XRCCTRL(dlg, "angleText", wxTextCtrl)->GetValue().c_str());
			XRCCTRL(dlg, "angleSlider", wxSlider)->SetValue(angle * 100);

			float angleLimited = XRCCTRL(dlg, "angleSlider", wxSlider)->GetValue() / 100.0f;
			if (angle != angleLimited)
				XRCCTRL(dlg, "angleText", wxTextCtrl)->ChangeValue(wxString::Format("%0.2f", angleLimited));

			updatePreview();
		};

		XRCCTRL(dlg, "angleSlider", wxSlider)->Bind(wxEVT_SLIDER, sliderMoved);
		XRCCTRL(dlg, "angleText", wxTextCtrl)->Bind(wxEVT_TEXT, textChanged);
		dlg.Bind(wxEVT_CHAR_HOOK, &OutfitStudioFrame::OnEnterClose, this);

		// Set old value of first mesh
		XRCCTRL(dlg, "angleText", wxTextCtrl)->SetValue(wxString::Format("%0.2f", oldMeshAngles.front()));

		if (dlg.ShowModal() != wxID_OK) {
			for (size_t i = 0; i < activeMeshes.size(); i++) {
				Mesh* m = activeMeshes[i];
				m->smoothSeamNormalsAngle = oldMeshAngles[i];
				m->SmoothNormals();
			}

			glView->Render();
			return;
		}

		float angle = atof(XRCCTRL(dlg, "angleText", wxTextCtrl)->GetValue().c_str());

		for (auto& s : selectedItems)
			project->activeSet.SetSmoothSeamNormalsAngle(s->GetShape()->name.get(), angle);
	}
}

void OutfitStudioFrame::OnLockNormals(wxCommandEvent& event) {
	bool enable = event.IsChecked();
	glView->SetLockNormalsMode(enable);

	for (auto& s : selectedItems)
		project->activeSet.SetLockNormals(s->GetShape()->name.get(), enable);
}

void OutfitStudioFrame::OnEditUV(wxCommandEvent& WXUNUSED(event)) {
	if (editUV)
		return;

	if (!ShapeSelectionCheck())
		return;

	auto shape = activeItem->GetShape();
	Mesh* m = glView->GetMesh(shape->name.get());
	if (shape && m) {
		if (editor.bEditSlider && !ConfirmSliderDataLocalForEdit(shape, editor.activeSlider))
			return;

		editUV = new EditUV(this, project->GetWorkNif(), shape, m, editor.activeSlider);

		editUV->Bind(wxEVT_CLOSE_WINDOW, [&](wxCloseEvent& event) {
			editUV = nullptr;
			event.Skip();
		});

		editUV->CenterOnParent();
		editUV->Show();
	}
}
