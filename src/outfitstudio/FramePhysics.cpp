/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

// Outfit Studio window: the physics preview

#include "OutfitStudioFrame.h"
#include "AnimationPlayer.h"
#include "FrameHelpers.h"
#include "GLPanel.h"
#include "ui/wxStateButton.h"
#include "physics/Controller.h"
#include "components/SliderPresets.h"
#include "files/HkxFile.h"

#include <cmath>
#include <cstdlib>
#include <iterator>
#include <sstream>
#include <wx/textctrl.h>

using namespace nifly;

void OutfitStudioFrame::InjectPhysicsCameraYaw(float deltaDegrees) {
	if (physicsRunning)
		physics->InjectCameraYaw(deltaDegrees);
}

void OutfitStudioFrame::OnPhysicsCheckBox(wxCommandEvent& e) {
	// Physics only simulates while posing or playing; make checking the box
	// take effect immediately by enabling pose mode along with it.
	if (e.IsChecked() && !project->bPose && !animation->IsPlaying())
		ActivatePose(true);
	else
		UpdatePhysicsState();
}

void OutfitStudioFrame::OnPhysicsVisCheckBox(wxCommandEvent& e) {
	if (physics && physicsRunning) {
		physics->UpdateDebugVis(glView->gls, e.IsChecked());
		glView->Render();
	}
}

void OutfitStudioFrame::OnPhysicsGrabCheckBox(wxCommandEvent& e) {
	// Turning it off mid-drag is only possible with a lost mouse capture, which
	// releases the grab already; this is just to be sure nothing keeps pulling.
	if (!e.IsChecked() && glView)
		glView->EndPhysicsGrab();
}

void OutfitStudioFrame::OnPhysicsProbeCheckBox(wxCommandEvent& WXUNUSED(e)) {
	UpdatePhysicsProbeState();
}

// The sliders count in tenths of a NIF unit, so dragging one lands on a decimal
// rather than jumping a whole unit at a time. The field beside it holds the value
// that is actually used, and can be typed into for anything finer or further out
// than the slider reaches.
static constexpr double physicsProbeSliderScale = 10.0;

void OutfitStudioFrame::OnPhysicsProbeSlider(wxScrollEvent& e, wxTextCtrl* text) {
	if (!text)
		return;

	// "%g" rather than streaming the value in, which would spell 60 as 60.000000
	// and leave nothing of it visible in a field this narrow
	text->ChangeValue(wxString::Format("%g", e.GetPosition() / physicsProbeSliderScale));
	ApplyPhysicsProbe();
}

// The probe position and size controls, in the same order
static const char* const PhysicsProbeSliderNames[] = {"physicsProbeX", "physicsProbeY", "physicsProbeZ", "physicsProbeSize"};
static const char* const PhysicsProbeTextNames[] = {"physicsProbeXText", "physicsProbeYText", "physicsProbeZText", "physicsProbeSizeText"};

void OutfitStudioFrame::OnPhysicsProbeAxisSlider(wxScrollEvent& event) {
	wxTextCtrl* texts[] = {physicsProbeXText, physicsProbeYText, physicsProbeZText, physicsProbeSizeText};
	for (int i = 0; i < 4; i++) {
		if (event.GetId() == XRCID(PhysicsProbeSliderNames[i])) {
			OnPhysicsProbeSlider(event, texts[i]);
			return;
		}
	}
}

void OutfitStudioFrame::OnPhysicsProbeText(wxTextCtrl* text, wxSlider* slider) {
	if (!text || !slider)
		return;

	double val = 0.0;
	if (!text->GetValue().ToDouble(&val))
		return;

	// Only to keep the handle where the value is - the slider clamps to its own
	// range, and a typed value outside it is still the one that gets used.
	slider->SetValue(static_cast<int>(std::lround(val * physicsProbeSliderScale)));
	ApplyPhysicsProbe();
}

void OutfitStudioFrame::OnPhysicsProbeAxisText(wxCommandEvent& event) {
	wxTextCtrl* texts[] = {physicsProbeXText, physicsProbeYText, physicsProbeZText, physicsProbeSizeText};
	wxSlider* sliders[] = {physicsProbeX, physicsProbeY, physicsProbeZ, physicsProbeSize};
	for (int i = 0; i < 4; i++) {
		if (event.GetId() == XRCID(PhysicsProbeTextNames[i])) {
			OnPhysicsProbeText(texts[i], sliders[i]);
			return;
		}
	}
}

void OutfitStudioFrame::UpdatePhysicsProbeControl(bool enabled) {
	if (!cbPhysicsProbe)
		return;

	if (!enabled)
		cbPhysicsProbe->SetValue(false);

	cbPhysicsProbe->Enable(enabled);
	UpdatePhysicsProbeState();
}

// Brings the ball, its sliders and the simulation in line with the checkbox.
void OutfitStudioFrame::UpdatePhysicsProbeState() {
	const bool enabled = physicsRunning && cbPhysicsProbe && cbPhysicsProbe->IsChecked();

	// The controls are the only way to place the ball, so they are of no use
	// while there is no ball to place
	if (physicsProbePanel && physicsProbePanel->IsShown() != enabled) {
		physicsProbePanel->Show(enabled);

		// The panel sits inside a collapsible pane, and it is that pane's best
		// size the tool area lays out against, so it has to be recomputed
		// before the pane will make room for the sliders.
		if (physicsPane) {
			physicsPane->GetPane()->Layout();
			physicsPane->InvalidateBestSize();
		}

		UpdateToolScrollLayout();
	}

	if (enabled) {
		ApplyPhysicsProbe();
		return;
	}

	if (physics)
		physics->ClearProbe();

	if (glView)
		glView->HidePhysicsProbe();
}

// Hands the ball's place and size to the simulation and to the mesh that draws it.
// The fields are read rather than the sliders, so a value typed finer than the
// slider steps or beyond the range it covers is the one that counts. Both are in
// NIF units, the space the simulation works in.
void OutfitStudioFrame::ApplyPhysicsProbe() {
	if (!physicsRunning || !physicsProbeXText || !physicsProbeYText || !physicsProbeZText || !physicsProbeSizeText)
		return;

	double x = 0.0, y = 0.0, z = 0.0, size = 0.0;
	if (!physicsProbeXText->GetValue().ToDouble(&x) || !physicsProbeYText->GetValue().ToDouble(&y) || !physicsProbeZText->GetValue().ToDouble(&z)
		|| !physicsProbeSizeText->GetValue().ToDouble(&size))
		return;

	// A ball with no radius collides with nothing, and there would be nothing to draw
	if (size <= 0.0)
		return;

	const Vector3 position(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));
	const float radius = static_cast<float>(size);

	physics->SetProbe(position, radius);

	if (glView) {
		glView->ShowPhysicsProbe(position, radius);
		glView->Render();
	}
}

void OutfitStudioFrame::UpdatePhysicsGrabControl(bool enabled) {
	if (!cbPhysicsGrab)
		return;

	if (!enabled) {
		cbPhysicsGrab->SetValue(false);
		if (glView)
			glView->EndPhysicsGrab();
	}

	cbPhysicsGrab->Enable(enabled);
}

bool OutfitStudioFrame::IsPhysicsGrabEnabled() const {
	return physicsRunning && cbPhysicsGrab && cbPhysicsGrab->IsChecked();
}

const std::unordered_set<std::string>& OutfitStudioFrame::GetPhysicsAffectedShapes() const {
	static const std::unordered_set<std::string> none;
	return physicsRunning ? physics->AffectedShapes() : none;
}

void OutfitStudioFrame::RefitPhysicsBVH() {
	if (!physicsRunning)
		return;

	for (const auto& shapeName : physics->AffectedShapes()) {
		Mesh* m = glView->GetMesh(shapeName);
		if (m && m->bvh)
			m->bvh->Refit();
	}
}

bool OutfitStudioFrame::BeginPhysicsGrab(const std::vector<Physics::GrabTarget>& targets) {
	return physicsRunning && physics->BeginGrab(targets);
}

void OutfitStudioFrame::UpdatePhysicsGrab(const Vector3& offset) {
	if (physicsRunning)
		physics->UpdateGrab(offset);
}

void OutfitStudioFrame::EndPhysicsGrab() {
	if (physics)
		physics->EndGrab();
}

void OutfitStudioFrame::OnPhysicsWindSlider(wxScrollEvent& WXUNUSED(e)) {
	ApplyPhysicsWind();
}

void OutfitStudioFrame::OnPhysicsWindDir(wxCommandEvent& WXUNUSED(e)) {
	ApplyPhysicsWind();
}

void OutfitStudioFrame::ApplyPhysicsWind() {
	if (!physics)
		return;

	// The choice control lists Physics::WindDirectionNames() in order
	if (physicsWindDir)
		physics->SetWindDirection(Physics::WindDirectionFromIndex(physicsWindDir->GetSelection()));

	if (physicsWindSlider)
		physics->SetWindStrength(physicsWindSlider->GetValue() / 100.0f);
}

void OutfitStudioFrame::UpdatePhysicsControlsVisibility() {
	if (!cbPhysics || !physicsPane)
		return;

	const bool available = project && Physics::HasPhysicsLinks(project->GetWorkNif(), project->shapePhysicsFiles);
	if (available == physicsAvailable)
		return;

	physicsAvailable = available;

	// Deleting the last shape with physics while it simulates has to stop it,
	// and UpdatePhysicsState only ever starts physics for a checked box
	if (!available)
		cbPhysics->SetValue(false);

	// The pane belongs to the bones tab; leaving it shows nothing regardless.
	physicsPane->Show(available && currentTabButton == boneTabButton);

	if (!available)
		UpdatePhysicsState();

	UpdateToolScrollLayout();
}

void OutfitStudioFrame::ShutdownPhysics() {
	StopPhysicsPump();
	physicsRunning = false;

	if (project)
		project->physicsPose = nullptr;

	if (physics) {
		if (glView)
			physics->UpdateDebugVis(glView->gls, false);
		physics->Clear();
	}

	if (cbPhysics)
		cbPhysics->SetValue(false);

	if (cbPhysicsVis) {
		cbPhysicsVis->SetValue(false);
		cbPhysicsVis->Enable(false);
	}

	UpdatePhysicsGrabControl(false);
	UpdatePhysicsProbeControl(false);
}

void OutfitStudioFrame::UpdatePhysicsState() {
	const bool desired = cbPhysics && cbPhysics->IsChecked() && project && (project->bPose || animation->IsPlaying());

	if (desired && !physicsRunning) {
		if (!physics)
			physics = std::make_unique<Physics::Controller>();

		std::vector<std::string> warnings;
		OutfitProject* proj = project;
		size_t systemCount = physics->BuildFromNif(
			project->GetWorkNif(),
			project->GetWorkAnim(),
			[proj](const std::string& xmlPath) { return proj->GetPhysicsXmlStream(xmlPath); },
			project->shapePhysicsFiles,
			warnings);

		for (auto& warning : warnings)
			wxLogWarning("Physics: %s", warning);

		if (systemCount == 0) {
			statusBar->SetStatusText(_("No physics XMLs found in loaded meshes"));
			physics->Clear();

			// Nothing is simulating, so the box must not claim otherwise
			cbPhysics->SetValue(false);
			return;
		}

		statusBar->SetStatusText(wxString::Format(_("Physics active: %zu system(s)"), systemCount));
		project->physicsPose = &physics->PoseOverrides();
		physicsRunning = true;
		UpdatePhysicsShapes();
		physics->ResetDynamics();

		// The step clock must be valid for the lockstep path too, where
		// StartPhysicsPump never runs
		physicsClock.Reset(GetTargetFps(this));

		// The controller is fresh; re-apply the current wind settings
		ApplyPhysicsWind();

		if (cbPhysicsVis)
			cbPhysicsVis->Enable();

		UpdatePhysicsGrabControl(true);
		UpdatePhysicsProbeControl(true);

		if (!animation->IsPlaying())
			StartPhysicsPump();
	}
	else if (!desired && physicsRunning) {
		// Drop the overrides before the pose is applied again, or the frame
		// that is supposed to restore the user pose still shows the simulated
		// one.
		physicsRunning = false;
		project->physicsPose = nullptr;
		physics->UpdateDebugVis(glView->gls, false);
		physics->Clear();
		StopPhysicsPump();

		if (cbPhysicsVis) {
			cbPhysicsVis->SetValue(false);
			cbPhysicsVis->Enable(false);
		}

		UpdatePhysicsGrabControl(false);
		UpdatePhysicsProbeControl(false);

		// Restore the clean user pose without the physics overrides. Also
		// catches the BVH up with what is displayed: the pump skipped the
		// rebuild on every tick.
		ApplyPose();
	}
	else if (physicsRunning) {
		// Animation playback took over or ended: while playing, ApplyPose
		// steps the simulation in lockstep instead of the pump.
		if (animation->IsPlaying() && physicsPumpActive)
			StopPhysicsPump();
		else if (!animation->IsPlaying() && !physicsPumpActive)
			StartPhysicsPump();
	}

	// A checked box always means "simulating". Anything that stopped physics -
	// leaving the bones tab, turning off pose mode, ending playback - clears
	// it instead of leaving a checked box that does nothing.
	if (cbPhysics && !physicsRunning)
		cbPhysics->SetValue(false);
}

void OutfitStudioFrame::UpdatePhysicsShapes() {
	if (!physicsRunning || !physics || !project)
		return;

	// The systems are built from the base shapes of the NIF, so they would
	// otherwise keep colliding with the shapes the sliders morphed away from
	std::vector<Vector3> verts;
	for (auto& shape : project->GetWorkNif()->GetShapes()) {
		project->GetMorphedVerts(shape, verts);
		physics->SetShapeVertices(shape->name.get(), verts);
	}
}

void OutfitStudioFrame::StartPhysicsPump() {
	if (physicsPumpActive)
		return;

	physicsPumpActive = true;
	physicsClock.Reset(GetTargetFps(this));
	SetHighResolutionTimers(true);
	Bind(wxEVT_IDLE, &OutfitStudioFrame::OnPhysicsIdle, this);
	physicsTimer.Start(Physics::PumpTimerIntervalMS);
}

void OutfitStudioFrame::StopPhysicsPump() {
	if (!physicsPumpActive)
		return;

	physicsTimer.Stop();
	Unbind(wxEVT_IDLE, &OutfitStudioFrame::OnPhysicsIdle, this);
	SetHighResolutionTimers(false);
	physicsPumpActive = false;
}

void OutfitStudioFrame::PumpPhysics() {
	if (!physicsPumpActive || !physicsRunning)
		return;

	float dtSeconds = 0.0f;
	if (!physicsClock.StepDue(dtSeconds))
		return;

	physics->Step(dtSeconds);

	const auto& affectedShapes = physics->AffectedShapes();
	for (auto& shape : project->GetWorkNif()->GetShapes()) {
		if (affectedShapes.count(shape->name.get()) == 0)
			continue;

		std::vector<Vector3> verts;
		project->GetLiveVerts(shape, verts);

		// BVH rebuilds only serve picking and brushes; way too expensive per
		// frame. StopPhysicsPump does one full ApplyPose to catch up.
		glView->UpdateMeshVertices(shape->name.get(), &verts, false, true, false);
	}

	if (cbPhysicsVis && cbPhysicsVis->IsChecked())
		physics->UpdateDebugVis(glView->gls, true);

	glView->Render();
}

void OutfitStudioFrame::OnPhysicsTimer(wxTimerEvent& WXUNUSED(event)) {
	PumpPhysics();
}

void OutfitStudioFrame::OnPhysicsIdle(wxIdleEvent& event) {
	if (!physicsPumpActive)
		return;

	if (physicsClock.UntilDue() > 1500)
		wxMilliSleep(1);
	else
		PumpPhysics();

	event.RequestMore();
}
