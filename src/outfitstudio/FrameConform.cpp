/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

// Outfit Studio window: conforming sliders and fixing clipping

#include "OutfitStudioFrame.h"
#include "ShapeTreeItems.h"
#include "GLPanel.h"
#include "components/UndoHistory.h"
#include "components/MaskOps.h"
#include "files/HkxFile.h"
#include "components/ClippingFixer.h"

#include <cmath>
#include <cstdlib>
#include <iterator>
#include <sstream>
#include <wx/textctrl.h>

using namespace nifly;

bool OutfitStudioFrame::ShowClippingFixStrength(float& outStrength) {
	int strengthPct = 50;

	wxDialog dlg(this, wxID_ANY, _("Fix Clipping"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE);
	auto* sizer = new wxBoxSizer(wxVERTICAL);

	auto* label = new wxStaticText(&dlg, wxID_ANY, wxString::Format(_("Strength: %d"), strengthPct));
	auto* slider = new wxSlider(&dlg, wxID_ANY, strengthPct, 1, 100);

	slider->Bind(wxEVT_SLIDER, [&](wxCommandEvent&) {
		strengthPct = slider->GetValue();
		label->SetLabel(wxString::Format(_("Strength: %d"), strengthPct));
	});

	sizer->Add(label, 0, wxALL, 10);
	sizer->Add(slider, 0, wxEXPAND | wxLEFT | wxRIGHT, 10);
	sizer->Add(dlg.CreateStdDialogButtonSizer(wxOK | wxCANCEL), 0, wxALL | wxEXPAND, 10);
	dlg.SetSizerAndFit(sizer);
	dlg.CenterOnParent();

	if (dlg.ShowModal() != wxID_OK)
		return false;

	outStrength = strengthPct / 100.0f;
	return true;
}

void OutfitStudioFrame::FixClippingForShape(const std::vector<Vector3>& bodyVerts,
											const std::vector<Triangle>& bodyTris,
											NiShape* shape,
											const std::vector<Vector3>& outfitVerts,
											const ClippingFixOptions& options,
											UndoStateProject* usp,
											const std::vector<float>& fixWeights) {
	if (!ClippingFixer::IsEligibleForFix(*project->GetWorkNif(), shape))
		return;

	std::vector<Triangle> outfitTris;
	shape->GetTriangles(outfitTris);
	if (outfitTris.empty())
		return;

	std::vector<Vector3> fixedVerts = outfitVerts;
	ClippingFixer::FixClipping(bodyVerts, bodyTris, fixedVerts, outfitTris, options);

	UndoStateShape uss;
	uss.shapeName = shape->name.get();

	for (size_t i = 0; i < outfitVerts.size(); i++) {
		// Masked vertices keep their place, a soft mask border blends into the fix
		Vector3 diff = fixedVerts[i] - outfitVerts[i];
		if (i < fixWeights.size())
			diff *= fixWeights[i];

		if (diff.IsZero(true))
			continue;

		uss.pointStartState[i] = Mesh::TransformPosNifToMesh(outfitVerts[i]);
		uss.pointEndState[i] = Mesh::TransformPosNifToMesh(outfitVerts[i] + diff);
	}

	if (!uss.pointStartState.empty())
		usp->usss.push_back(std::move(uss));
}

void OutfitStudioFrame::OnSliderFixClipping(wxCommandEvent& WXUNUSED(event)) {
	if (!editor.bEditSlider) {
		wxMessageBox(_("You must be in slider edit mode to fix clipping for a slider."), _("Fix Clipping"), wxICON_WARNING);
		return;
	}

	NiShape* refShape = project->GetBaseShape();
	if (!refShape) {
		wxMessageBox(_("No reference shape set."), _("Fix Clipping"), wxICON_WARNING);
		return;
	}

	float strength = 0.0f;
	if (!ShowClippingFixStrength(strength))
		return;

	ClippingFixOptions options;
	options.strength = strength;
	options.fixPokeThrough = true;

	UndoStateProject* usp = glView->GetUndoHistory()->PushState();
	usp->undoType = UndoType::VertexPosition;

	float sliderscale = project->SliderValue(editor.activeSlider);
	if (sliderscale == 0.0)
		sliderscale = 1.0;

	for (auto& sel : selectedItems) {
		NiShape* shape = sel->GetShape();
		if (project->IsBaseShape(shape))
			continue;

		if (!ClippingFixer::IsEligibleForFix(*project->GetWorkNif(), shape))
			continue;

		// Only process shapes that have a diff for the active slider
		size_t sliderIndex = 0;
		if (!project->SliderIndexFromName(editor.activeSlider, sliderIndex))
			continue;

		TargetDataDiffs* diffSet = project->GetDiffSet(project->activeSet[sliderIndex], shape);
		if (!diffSet || diffSet->empty())
			continue;

		// Masked vertices keep their place, a soft mask border blends into the fix
		std::vector<float> fixWeights = MaskOps::FixWeights(glView->GetMesh(shape->name.get()));
		std::unordered_set<uint16_t> allowed;
		for (size_t i = 0; i < fixWeights.size(); i++)
			if (fixWeights[i] > 0.0f)
				allowed.insert(static_cast<uint16_t>(i));

		if (!fixWeights.empty() && allowed.empty())
			continue;

		TargetDataDiffs morphDiffs;
		project->CalcSliderClippingCorrection(shape, editor.activeSlider, options.strength, morphDiffs, fixWeights.empty() ? nullptr : &allowed);
		for (auto& diff : morphDiffs)
			if (diff.first < fixWeights.size())
				diff.second *= fixWeights[diff.first];

		if (morphDiffs.empty())
			continue;

		std::vector<Vector3> outfitVerts;
		project->GetLiveVerts(shape, outfitVerts);

		UndoStateShape uss;
		uss.shapeName = shape->name.get();

		for (auto& diffEntry : morphDiffs) {
			uint16_t vertIndex = diffEntry.first;
			if (vertIndex >= outfitVerts.size())
				continue;

			Vector3 start = Mesh::TransformPosNifToMesh(outfitVerts[vertIndex]);
			uss.pointStartState[vertIndex] = start;
			uss.pointEndState[vertIndex] = start + diffEntry.second * sliderscale;
		}

		if (!uss.pointStartState.empty())
			usp->usss.push_back(std::move(uss));
	}

	if (usp->usss.empty()) {
		glView->GetUndoHistory()->PopState();
		return;
	}

	usp->sliderName = editor.activeSlider;
	usp->sliderscale = sliderscale;

	glView->ApplyUndoState(usp, false);

	if (glView->GetTransformMode())
		glView->ShowTransformTool();

	UpdateUndoTools();
	HighlightSliderData();
}

void OutfitStudioFrame::OnSliderConform(wxCommandEvent& WXUNUSED(event)) {
	if (!project->GetBaseShape())
		return;

	std::vector<NiShape*> shapes;
	for (auto& i : selectedItems)
		shapes.push_back(i->GetShape());

	ConformShapes(shapes);
}

void OutfitStudioFrame::OnSliderConformAll(wxCommandEvent& WXUNUSED(event)) {
	if (!project->GetBaseShape())
		return;

	auto shapes = project->GetWorkNif()->GetShapes();
	ConformShapes(shapes);
}

int OutfitStudioFrame::ConformShapes(std::vector<NiShape*> shapes, bool silent) {
	if (shapes.empty())
		return 0;

	StartProgress(_("Conforming shapes..."));

	ConformOptions options;
	if (ShowConform(options, silent)) {
		wxLogMessage("Conforming shapes...");

		// Collect shown slider names before zeroing
		for (size_t i = 0; i < project->SliderCount(); i++)
			if (project->SliderShow(i))
				options.sliderNames.push_back(project->GetSliderName(i));

		std::vector<NiShape*> targetShapes;
		for (auto* shape : shapes)
			if (shape && !project->IsBaseShape(shape))
				targetShapes.push_back(shape);

		std::vector<std::string> conformSliderNames;
		project->GetConformSliderNames(options, conformSliderNames);
		if (!ConfirmSliderDataLocalForEdit(targetShapes, conformSliderNames)) {
			EndProgress();
			HighlightSliderData();
			return 0;
		}

		OpResult result = ProjectOperations(*project, editor).ConformSliders(shapes, options);
		ApplyRefresh(result.refresh);

		if (statusBar)
			statusBar->SetStatusText(_("All shapes conformed."));

		wxLogMessage("All shapes conformed.");
	}

	EndProgress();
	HighlightSliderData();
	return 0;
}

bool OutfitStudioFrame::ShowConform(ConformOptions& options, bool silent) {
	CloseBrushSettings();

	wxDialog dlg;
	if (wxXmlResource::Get()->LoadDialog(&dlg, this, "dlgConforming")) {
		XRCCTRL(dlg, "proximityRadiusSlider", wxSlider)->Bind(wxEVT_SLIDER, [&dlg](wxCommandEvent&) {
			float changed = XRCCTRL(dlg, "proximityRadiusSlider", wxSlider)->GetValue() / 1000.0f;
			XRCCTRL(dlg, "proximityRadiusText", wxTextCtrl)->ChangeValue(wxString::Format("%0.5f", changed));
		});

		XRCCTRL(dlg, "proximityRadiusText", wxTextCtrl)->Bind(wxEVT_TEXT, [&dlg](wxCommandEvent&) {
			float changed = atof(XRCCTRL(dlg, "proximityRadiusText", wxTextCtrl)->GetValue().c_str());
			XRCCTRL(dlg, "proximityRadiusSlider", wxSlider)->SetValue(changed * 1000);
		});

		XRCCTRL(dlg, "maxResultsSlider", wxSlider)->Bind(wxEVT_SLIDER, [&dlg](wxCommandEvent&) {
			int changed = XRCCTRL(dlg, "maxResultsSlider", wxSlider)->GetValue();
			XRCCTRL(dlg, "maxResultsText", wxTextCtrl)->ChangeValue(wxString::Format("%d", changed));
		});

		XRCCTRL(dlg, "noTargetLimit", wxCheckBox)->Bind(wxEVT_CHECKBOX, [&dlg](wxCommandEvent&) {
			bool noTargetLimit = XRCCTRL(dlg, "noTargetLimit", wxCheckBox)->IsChecked();
			XRCCTRL(dlg, "maxResultsText", wxTextCtrl)->Enable(!noTargetLimit);
			XRCCTRL(dlg, "maxResultsSlider", wxSlider)->Enable(!noTargetLimit);

			if (noTargetLimit) {
				XRCCTRL(dlg, "proximityRadiusText", wxTextCtrl)->ChangeValue("5.00000");
				XRCCTRL(dlg, "proximityRadiusSlider", wxSlider)->SetValue(5000);
			}
		});

		auto updateSmoothingState = [&dlg]() {
			bool smoothResults = XRCCTRL(dlg, "smoothResults", wxCheckBox)->IsChecked();
			XRCCTRL(dlg, "smoothIterationsText", wxTextCtrl)->Enable(smoothResults);
			XRCCTRL(dlg, "smoothStrengthText", wxTextCtrl)->Enable(smoothResults);
		};

		XRCCTRL(dlg, "smoothResults", wxCheckBox)->Bind(wxEVT_CHECKBOX, [&updateSmoothingState](wxCommandEvent&) {
			updateSmoothingState();
		});
		updateSmoothingState();

		auto updateFixClippingState = [&dlg]() {
			bool fixClipping = XRCCTRL(dlg, "fixClipping", wxCheckBox)->IsChecked();
			XRCCTRL(dlg, "fixClippingStrengthText", wxTextCtrl)->Enable(fixClipping);
		};

		XRCCTRL(dlg, "fixClipping", wxCheckBox)->Bind(wxEVT_CHECKBOX, [&updateFixClippingState](wxCommandEvent&) {
			updateFixClippingState();
		});
		updateFixClippingState();

		XRCCTRL(dlg, "presetDefault", wxButton)->Bind(wxEVT_BUTTON, [&dlg](wxCommandEvent&) {
			XRCCTRL(dlg, "noTargetLimit", wxCheckBox)->SetValue(false);
			XRCCTRL(dlg, "smoothResults", wxCheckBox)->SetValue(false);
			XRCCTRL(dlg, "smoothIterationsText", wxTextCtrl)->Disable();
			XRCCTRL(dlg, "smoothStrengthText", wxTextCtrl)->Disable();
			XRCCTRL(dlg, "fixClipping", wxCheckBox)->SetValue(false);
			XRCCTRL(dlg, "fixClippingStrengthText", wxTextCtrl)->ChangeValue("50");
			XRCCTRL(dlg, "fixClippingStrengthText", wxTextCtrl)->Disable();
			XRCCTRL(dlg, "noSqueeze", wxCheckBox)->SetValue(false);
			XRCCTRL(dlg, "solidMode", wxCheckBox)->SetValue(false);
			XRCCTRL(dlg, "proximityRadiusText", wxTextCtrl)->ChangeValue("10.00000");
			XRCCTRL(dlg, "proximityRadiusSlider", wxSlider)->SetValue(10000);
			XRCCTRL(dlg, "maxResultsText", wxTextCtrl)->Enable();
			XRCCTRL(dlg, "maxResultsSlider", wxSlider)->Enable();
			XRCCTRL(dlg, "maxResultsText", wxTextCtrl)->ChangeValue("10");
			XRCCTRL(dlg, "maxResultsSlider", wxSlider)->SetValue(10);
		});

		XRCCTRL(dlg, "presetEvenMovement", wxButton)->Bind(wxEVT_BUTTON, [&dlg](wxCommandEvent&) {
			XRCCTRL(dlg, "noTargetLimit", wxCheckBox)->SetValue(true);
			XRCCTRL(dlg, "smoothResults", wxCheckBox)->SetValue(false);
			XRCCTRL(dlg, "smoothIterationsText", wxTextCtrl)->Disable();
			XRCCTRL(dlg, "smoothStrengthText", wxTextCtrl)->Disable();
			XRCCTRL(dlg, "fixClipping", wxCheckBox)->SetValue(false);
			XRCCTRL(dlg, "fixClippingStrengthText", wxTextCtrl)->ChangeValue("50");
			XRCCTRL(dlg, "fixClippingStrengthText", wxTextCtrl)->Disable();
			XRCCTRL(dlg, "noSqueeze", wxCheckBox)->SetValue(true);
			XRCCTRL(dlg, "solidMode", wxCheckBox)->SetValue(false);
			XRCCTRL(dlg, "maxResultsText", wxTextCtrl)->Disable();
			XRCCTRL(dlg, "maxResultsSlider", wxSlider)->Disable();
			XRCCTRL(dlg, "proximityRadiusText", wxTextCtrl)->ChangeValue("5.00000");
			XRCCTRL(dlg, "proximityRadiusSlider", wxSlider)->SetValue(5000);
		});

		XRCCTRL(dlg, "presetSmoothClipping", wxButton)->Bind(wxEVT_BUTTON, [&dlg](wxCommandEvent&) {
			XRCCTRL(dlg, "noTargetLimit", wxCheckBox)->SetValue(true);
			XRCCTRL(dlg, "smoothResults", wxCheckBox)->SetValue(true);
			XRCCTRL(dlg, "smoothIterationsText", wxTextCtrl)->Enable();
			XRCCTRL(dlg, "smoothStrengthText", wxTextCtrl)->Enable();
			XRCCTRL(dlg, "fixClipping", wxCheckBox)->SetValue(true);
			XRCCTRL(dlg, "fixClippingStrengthText", wxTextCtrl)->ChangeValue("50");
			XRCCTRL(dlg, "fixClippingStrengthText", wxTextCtrl)->Enable();
			XRCCTRL(dlg, "noSqueeze", wxCheckBox)->SetValue(true);
			XRCCTRL(dlg, "solidMode", wxCheckBox)->SetValue(false);
			XRCCTRL(dlg, "maxResultsText", wxTextCtrl)->Disable();
			XRCCTRL(dlg, "maxResultsSlider", wxSlider)->Disable();
			XRCCTRL(dlg, "proximityRadiusText", wxTextCtrl)->ChangeValue("5.00000");
			XRCCTRL(dlg, "proximityRadiusSlider", wxSlider)->SetValue(5000);
		});

		XRCCTRL(dlg, "presetSolidObject", wxButton)->Bind(wxEVT_BUTTON, [&dlg](wxCommandEvent&) {
			XRCCTRL(dlg, "noTargetLimit", wxCheckBox)->SetValue(true);
			XRCCTRL(dlg, "smoothResults", wxCheckBox)->SetValue(false);
			XRCCTRL(dlg, "smoothIterationsText", wxTextCtrl)->Disable();
			XRCCTRL(dlg, "smoothStrengthText", wxTextCtrl)->Disable();
			XRCCTRL(dlg, "fixClipping", wxCheckBox)->SetValue(false);
			XRCCTRL(dlg, "fixClippingStrengthText", wxTextCtrl)->ChangeValue("50");
			XRCCTRL(dlg, "fixClippingStrengthText", wxTextCtrl)->Disable();
			XRCCTRL(dlg, "noSqueeze", wxCheckBox)->SetValue(false);
			XRCCTRL(dlg, "solidMode", wxCheckBox)->SetValue(true);
			XRCCTRL(dlg, "maxResultsText", wxTextCtrl)->Disable();
			XRCCTRL(dlg, "maxResultsSlider", wxSlider)->Disable();
			XRCCTRL(dlg, "proximityRadiusText", wxTextCtrl)->ChangeValue("10.00000");
			XRCCTRL(dlg, "proximityRadiusSlider", wxSlider)->SetValue(10000);
		});

		dlg.Bind(wxEVT_CHAR_HOOK, &OutfitStudioFrame::OnEnterClose, this);

		if (silent || dlg.ShowModal() == wxID_OK) {
			options.proximityRadius = atof(XRCCTRL(dlg, "proximityRadiusText", wxTextCtrl)->GetValue().c_str());

			bool noTargetLimit = XRCCTRL(dlg, "noTargetLimit", wxCheckBox)->IsChecked();
			if (!noTargetLimit)
				options.maxResults = atol(XRCCTRL(dlg, "maxResultsText", wxTextCtrl)->GetValue().c_str());
			else
				options.maxResults = std::numeric_limits<int>::max();

			options.smoothResultDeltas = XRCCTRL(dlg, "smoothResults", wxCheckBox)->IsChecked();
			options.smoothIterations = atol(XRCCTRL(dlg, "smoothIterationsText", wxTextCtrl)->GetValue().c_str());
			options.smoothStrength = atof(XRCCTRL(dlg, "smoothStrengthText", wxTextCtrl)->GetValue().c_str());
			options.noSqueeze = XRCCTRL(dlg, "noSqueeze", wxCheckBox)->IsChecked();
			options.solidMode = XRCCTRL(dlg, "solidMode", wxCheckBox)->IsChecked();
			options.axisX = XRCCTRL(dlg, "axisX", wxCheckBox)->IsChecked();
			options.axisY = XRCCTRL(dlg, "axisY", wxCheckBox)->IsChecked();
			options.axisZ = XRCCTRL(dlg, "axisZ", wxCheckBox)->IsChecked();
			options.fixClipping = XRCCTRL(dlg, "fixClipping", wxCheckBox)->IsChecked();
			options.fixClippingStrength = std::max(0.0f, std::min(1.0f, static_cast<float>(atof(XRCCTRL(dlg, "fixClippingStrengthText", wxTextCtrl)->GetValue().c_str()) / 100.0f)));
			return true;
		}
	}

	return false;
}

void OutfitStudioFrame::OnFixClippingShape(wxCommandEvent& event) {
	if (editor.bEditSlider) {
		OnSliderFixClipping(event);
		return;
	}

	CloseBrushSettings();

	if (!ShapeSelectionCheck())
		return;

	if (!CheckEditableState())
		return;

	NiShape* refShape = project->GetBaseShape();
	if (!refShape) {
		wxMessageBox(_("No reference shape set."), _("Fix Clipping"), wxICON_WARNING);
		return;
	}

	float strength = 0.0f;
	if (!ShowClippingFixStrength(strength))
		return;

	ClippingFixOptions options;
	options.strength = strength;
	options.fixPokeThrough = true;

	// Get reference shape geometry (unmorphed)
	std::vector<Vector3> bodyVerts;
	std::vector<Triangle> bodyTris;
	project->GetWorkNif()->GetVertsForShape(refShape, bodyVerts);
	refShape->GetTriangles(bodyTris);

	UndoStateProject* usp = glView->GetUndoHistory()->PushState();
	usp->undoType = UndoType::VertexPosition;

	for (auto& sel : selectedItems) {
		NiShape* shape = sel->GetShape();
		if (project->IsBaseShape(shape))
			continue;

		if (!ClippingFixer::IsEligibleForFix(*project->GetWorkNif(), shape))
			continue;

		std::vector<Vector3> outfitVerts;
		project->GetWorkNif()->GetVertsForShape(shape, outfitVerts);

		FixClippingForShape(bodyVerts, bodyTris, shape, outfitVerts, options, usp, MaskOps::FixWeights(glView->GetMesh(shape->name.get())));
	}

	if (usp->usss.empty()) {
		glView->GetUndoHistory()->PopState();
		return;
	}

	glView->ApplyUndoState(usp, false);

	if (glView->GetTransformMode())
		glView->ShowTransformTool();

	UpdateUndoTools();
}
