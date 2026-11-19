/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "PhysicsPreview.h"
#include "PosePanel.h"
#include "AnimationPlayer.h"
#include "FrameHelpers.h"
#include "GLPanel.h"
#include "OutfitStudioFrame.h"
#include "physics/Controller.h"
#include "project/OutfitProject.h"

#include <cmath>
#include <wx/collpane.h>
#include <wx/wx.h>
#include <wx/xrc/xmlres.h>

using namespace nifly;

namespace {

// The sliders count in tenths of a NIF unit, so dragging one lands on a decimal
// rather than jumping a whole unit at a time. The field beside it holds the value
// that is actually used, and can be typed into for anything finer or further out
// than the slider reaches.
constexpr double physicsProbeSliderScale = 10.0;

// The probe position and size controls, in the same order
const char* const PhysicsProbeSliderNames[] = {"physicsProbeX", "physicsProbeY", "physicsProbeZ", "physicsProbeSize"};
const char* const PhysicsProbeTextNames[] = {"physicsProbeXText", "physicsProbeYText", "physicsProbeZText", "physicsProbeSizeText"};

} // namespace

PhysicsPreview::PhysicsPreview(OutfitStudioFrame& frame)
	: frame(frame)
	, timer(this) {
	Bind(wxEVT_TIMER, &PhysicsPreview::OnTimer, this);
}

PhysicsPreview::~PhysicsPreview() {
	timer.Stop();
}

void PhysicsPreview::Init() {
	cbPhysics = (wxCheckBox*)frame.FindWindowByName("cbPhysics");
	cbVis = (wxCheckBox*)frame.FindWindowByName("cbPhysicsVis");
	cbGrab = (wxCheckBox*)frame.FindWindowByName("cbPhysicsGrab");
	cbProbe = (wxCheckBox*)frame.FindWindowByName("cbPhysicsProbe");
	probePanel = (wxPanel*)frame.FindWindowByName("physicsProbePanel");
	probeX = (wxSlider*)frame.FindWindowByName("physicsProbeX");
	probeY = (wxSlider*)frame.FindWindowByName("physicsProbeY");
	probeZ = (wxSlider*)frame.FindWindowByName("physicsProbeZ");
	probeSize = (wxSlider*)frame.FindWindowByName("physicsProbeSize");
	probeXText = (wxTextCtrl*)frame.FindWindowByName("physicsProbeXText");
	probeYText = (wxTextCtrl*)frame.FindWindowByName("physicsProbeYText");
	probeZText = (wxTextCtrl*)frame.FindWindowByName("physicsProbeZText");
	probeSizeText = (wxTextCtrl*)frame.FindWindowByName("physicsProbeSizeText");
	windSlider = (wxSlider*)frame.FindWindowByName("physicsWindSlider");
	windDir = (wxChoice*)frame.FindWindowByName("physicsWindDir");

	frame.Bind(wxEVT_CHECKBOX, &PhysicsPreview::OnPhysicsCheckBox, this, XRCID("cbPhysics"));
	frame.Bind(wxEVT_CHECKBOX, &PhysicsPreview::OnVisCheckBox, this, XRCID("cbPhysicsVis"));
	frame.Bind(wxEVT_CHECKBOX, &PhysicsPreview::OnGrabCheckBox, this, XRCID("cbPhysicsGrab"));
	frame.Bind(wxEVT_CHECKBOX, &PhysicsPreview::OnProbeCheckBox, this, XRCID("cbPhysicsProbe"));
	for (auto& type : CommandScrollEventTypes()) {
		for (const char* name : PhysicsProbeSliderNames)
			frame.Bind(type, &PhysicsPreview::OnProbeAxisSlider, this, XRCID(name));
		frame.Bind(type, &PhysicsPreview::OnWindSlider, this, XRCID("physicsWindSlider"));
	}
	for (const char* name : PhysicsProbeTextNames)
		frame.Bind(wxEVT_TEXT, &PhysicsPreview::OnProbeAxisText, this, XRCID(name));
	frame.Bind(wxEVT_CHOICE, &PhysicsPreview::OnWindDir, this, XRCID("physicsWindDir"));
}

void PhysicsPreview::InjectCameraYaw(float deltaDegrees) {
	if (running)
		controller->InjectCameraYaw(deltaDegrees);
}

void PhysicsPreview::OnPhysicsCheckBox(wxCommandEvent& e) {
	// Physics only simulates while posing or playing; make checking the box
	// take effect immediately by enabling pose mode along with it.
	if (e.IsChecked() && !frame.project->bPose && !frame.animation->IsPlaying())
		frame.posePanel->ActivatePose(true);
	else
		UpdateState();
}

void PhysicsPreview::OnVisCheckBox(wxCommandEvent& e) {
	if (controller && running) {
		controller->UpdateDebugVis(frame.glView->gls, e.IsChecked());
		frame.glView->Render();
	}
}

void PhysicsPreview::OnGrabCheckBox(wxCommandEvent& e) {
	// Turning it off mid-drag is only possible with a lost mouse capture, which
	// releases the grab already; this is just to be sure nothing keeps pulling.
	if (!e.IsChecked() && frame.glView)
		frame.glView->EndPhysicsGrab();
}

void PhysicsPreview::OnProbeCheckBox(wxCommandEvent& WXUNUSED(e)) {
	UpdateProbeState();
}

void PhysicsPreview::OnProbeSlider(wxScrollEvent& e, wxTextCtrl* text) {
	if (!text)
		return;

	// "%g" rather than streaming the value in, which would spell 60 as 60.000000
	// and leave nothing of it visible in a field this narrow
	text->ChangeValue(wxString::Format("%g", e.GetPosition() / physicsProbeSliderScale));
	ApplyProbe();
}

void PhysicsPreview::OnProbeAxisSlider(wxScrollEvent& event) {
	wxTextCtrl* texts[] = {probeXText, probeYText, probeZText, probeSizeText};
	for (int i = 0; i < 4; i++) {
		if (event.GetId() == XRCID(PhysicsProbeSliderNames[i])) {
			OnProbeSlider(event, texts[i]);
			return;
		}
	}
}

void PhysicsPreview::OnProbeText(wxTextCtrl* text, wxSlider* slider) {
	if (!text || !slider)
		return;

	double val = 0.0;
	if (!text->GetValue().ToDouble(&val))
		return;

	// Only to keep the handle where the value is - the slider clamps to its own
	// range, and a typed value outside it is still the one that gets used.
	slider->SetValue(static_cast<int>(std::lround(val * physicsProbeSliderScale)));
	ApplyProbe();
}

void PhysicsPreview::OnProbeAxisText(wxCommandEvent& event) {
	wxTextCtrl* texts[] = {probeXText, probeYText, probeZText, probeSizeText};
	wxSlider* sliders[] = {probeX, probeY, probeZ, probeSize};
	for (int i = 0; i < 4; i++) {
		if (event.GetId() == XRCID(PhysicsProbeTextNames[i])) {
			OnProbeText(texts[i], sliders[i]);
			return;
		}
	}
}

void PhysicsPreview::UpdateProbeControl(bool enabled) {
	if (!cbProbe)
		return;

	if (!enabled)
		cbProbe->SetValue(false);

	cbProbe->Enable(enabled);
	UpdateProbeState();
}

// Brings the ball, its sliders and the simulation in line with the checkbox.
void PhysicsPreview::UpdateProbeState() {
	const bool enabled = running && cbProbe && cbProbe->IsChecked();

	// The controls are the only way to place the ball, so they are of no use
	// while there is no ball to place
	if (probePanel && probePanel->IsShown() != enabled) {
		probePanel->Show(enabled);

		// The panel sits inside a collapsible pane, and it is that pane's best
		// size the tool area lays out against, so it has to be recomputed
		// before the pane will make room for the sliders.
		if (frame.physicsPane) {
			frame.physicsPane->GetPane()->Layout();
			frame.physicsPane->InvalidateBestSize();
		}

		frame.UpdateToolScrollLayout();
	}

	if (enabled) {
		ApplyProbe();
		return;
	}

	if (controller)
		controller->ClearProbe();

	if (frame.glView)
		frame.glView->HidePhysicsProbe();
}

// Hands the ball's place and size to the simulation and to the mesh that draws it.
// The fields are read rather than the sliders, so a value typed finer than the
// slider steps or beyond the range it covers is the one that counts. Both are in
// NIF units, the space the simulation works in.
void PhysicsPreview::ApplyProbe() {
	if (!running || !probeXText || !probeYText || !probeZText || !probeSizeText)
		return;

	double x = 0.0, y = 0.0, z = 0.0, size = 0.0;
	if (!probeXText->GetValue().ToDouble(&x) || !probeYText->GetValue().ToDouble(&y) || !probeZText->GetValue().ToDouble(&z)
		|| !probeSizeText->GetValue().ToDouble(&size))
		return;

	// A ball with no radius collides with nothing, and there would be nothing to draw
	if (size <= 0.0)
		return;

	const Vector3 position(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));
	const float radius = static_cast<float>(size);

	controller->SetProbe(position, radius);

	if (frame.glView) {
		frame.glView->ShowPhysicsProbe(position, radius);
		frame.glView->Render();
	}
}

void PhysicsPreview::UpdateGrabControl(bool enabled) {
	if (!cbGrab)
		return;

	if (!enabled) {
		cbGrab->SetValue(false);
		if (frame.glView)
			frame.glView->EndPhysicsGrab();
	}

	cbGrab->Enable(enabled);
}

bool PhysicsPreview::IsGrabEnabled() const {
	return running && cbGrab && cbGrab->IsChecked();
}

const std::unordered_set<std::string>& PhysicsPreview::GetAffectedShapes() const {
	static const std::unordered_set<std::string> none;
	return running ? controller->AffectedShapes() : none;
}

void PhysicsPreview::RefitBVH() {
	if (!running)
		return;

	for (const auto& shapeName : controller->AffectedShapes()) {
		Mesh* m = frame.glView->GetMesh(shapeName);
		if (m && m->bvh)
			m->bvh->Refit();
	}
}

bool PhysicsPreview::BeginGrab(const std::vector<Physics::GrabTarget>& targets) {
	return running && controller->BeginGrab(targets);
}

void PhysicsPreview::UpdateGrab(const Vector3& offset) {
	if (running)
		controller->UpdateGrab(offset);
}

void PhysicsPreview::EndGrab() {
	if (controller)
		controller->EndGrab();
}

void PhysicsPreview::OnWindSlider(wxScrollEvent& WXUNUSED(e)) {
	ApplyWind();
}

void PhysicsPreview::OnWindDir(wxCommandEvent& WXUNUSED(e)) {
	ApplyWind();
}

void PhysicsPreview::ApplyWind() {
	if (!controller)
		return;

	// The choice control lists Physics::WindDirectionNames() in order
	if (windDir)
		controller->SetWindDirection(Physics::WindDirectionFromIndex(windDir->GetSelection()));

	if (windSlider)
		controller->SetWindStrength(windSlider->GetValue() / 100.0f);
}

void PhysicsPreview::UpdateControlsVisibility() {
	if (!cbPhysics || !frame.physicsPane)
		return;

	OutfitProject* project = frame.project;
	const bool nowAvailable = project && Physics::HasPhysicsLinks(project->GetWorkNif(), project->shapePhysicsFiles);
	if (nowAvailable == available)
		return;

	available = nowAvailable;

	// Deleting the last shape with physics while it simulates has to stop it,
	// and UpdateState only ever starts physics for a checked box
	if (!available)
		cbPhysics->SetValue(false);

	// The pane belongs to the bones tab; leaving it shows nothing regardless.
	frame.physicsPane->Show(available && frame.currentTabButton == frame.boneTabButton);

	if (!available)
		UpdateState();

	frame.UpdateToolScrollLayout();
}

void PhysicsPreview::Shutdown() {
	StopPump();
	running = false;

	if (frame.project)
		frame.project->physicsPose = nullptr;

	if (controller) {
		if (frame.glView)
			controller->UpdateDebugVis(frame.glView->gls, false);
		controller->Clear();
	}

	if (cbPhysics)
		cbPhysics->SetValue(false);

	if (cbVis) {
		cbVis->SetValue(false);
		cbVis->Enable(false);
	}

	UpdateGrabControl(false);
	UpdateProbeControl(false);
}

void PhysicsPreview::UpdateState() {
	OutfitProject* project = frame.project;
	const bool playing = frame.animation->IsPlaying();
	const bool desired = cbPhysics && cbPhysics->IsChecked() && project && (project->bPose || playing);

	if (desired && !running) {
		if (!controller)
			controller = std::make_unique<Physics::Controller>();

		std::vector<std::string> warnings;
		OutfitProject* proj = project;
		size_t systemCount = controller->BuildFromNif(
			project->GetWorkNif(),
			project->GetWorkAnim(),
			[proj](const std::string& xmlPath) { return proj->GetPhysicsXmlStream(xmlPath); },
			project->shapePhysicsFiles,
			warnings);

		for (auto& warning : warnings)
			wxLogWarning("Physics: %s", warning);

		if (systemCount == 0) {
			frame.statusBar->SetStatusText(_("No physics XMLs found in loaded meshes"));
			controller->Clear();

			// Nothing is simulating, so the box must not claim otherwise
			cbPhysics->SetValue(false);
			return;
		}

		frame.statusBar->SetStatusText(wxString::Format(_("Physics active: %zu system(s)"), systemCount));
		project->physicsPose = &controller->PoseOverrides();
		running = true;
		UpdateShapes();
		controller->ResetDynamics();

		// The step clock must be valid for the lockstep path too, where
		// StartPump never runs
		clock.Reset(GetTargetFps(&frame));

		// The controller is fresh; re-apply the current wind settings
		ApplyWind();

		if (cbVis)
			cbVis->Enable();

		UpdateGrabControl(true);
		UpdateProbeControl(true);

		if (!playing)
			StartPump();
	}
	else if (!desired && running) {
		// Drop the overrides before the pose is applied again, or the frame
		// that is supposed to restore the user pose still shows the simulated
		// one.
		running = false;
		project->physicsPose = nullptr;
		controller->UpdateDebugVis(frame.glView->gls, false);
		controller->Clear();
		StopPump();

		if (cbVis) {
			cbVis->SetValue(false);
			cbVis->Enable(false);
		}

		UpdateGrabControl(false);
		UpdateProbeControl(false);

		// Restore the clean user pose without the physics overrides. Also
		// catches the BVH up with what is displayed: the pump skipped the
		// rebuild on every tick.
		frame.ApplyPose();
	}
	else if (running) {
		// Animation playback took over or ended: while playing, ApplyPose
		// steps the simulation in lockstep instead of the pump.
		if (playing && pumpActive)
			StopPump();
		else if (!playing && !pumpActive)
			StartPump();
	}

	// A checked box always means "simulating". Anything that stopped physics -
	// leaving the bones tab, turning off pose mode, ending playback - clears
	// it instead of leaving a checked box that does nothing.
	if (cbPhysics && !running)
		cbPhysics->SetValue(false);
}

void PhysicsPreview::UpdateShapes() {
	OutfitProject* project = frame.project;
	if (!running || !controller || !project)
		return;

	// The systems are built from the base shapes of the NIF, so they would
	// otherwise keep colliding with the shapes the sliders morphed away from
	std::vector<Vector3> verts;
	for (auto& shape : project->GetWorkNif()->GetShapes()) {
		project->GetMorphedVerts(shape, verts);
		controller->SetShapeVertices(shape->name.get(), verts);
	}
}

void PhysicsPreview::StepWithAnimation() {
	if (running && frame.animation->IsPlaying())
		controller->Step(clock.TakeElapsed());
}

void PhysicsPreview::ResetDynamics() {
	if (running)
		controller->ResetDynamics();
}

void PhysicsPreview::StartPump() {
	if (pumpActive)
		return;

	pumpActive = true;
	clock.Reset(GetTargetFps(&frame));
	SetHighResolutionTimers(true);
	frame.Bind(wxEVT_IDLE, &PhysicsPreview::OnIdle, this);
	timer.Start(Physics::PumpTimerIntervalMS);
}

void PhysicsPreview::StopPump() {
	if (!pumpActive)
		return;

	timer.Stop();
	frame.Unbind(wxEVT_IDLE, &PhysicsPreview::OnIdle, this);
	SetHighResolutionTimers(false);
	pumpActive = false;
}

void PhysicsPreview::Pump() {
	if (!pumpActive || !running)
		return;

	float dtSeconds = 0.0f;
	if (!clock.StepDue(dtSeconds))
		return;

	controller->Step(dtSeconds);

	OutfitProject* project = frame.project;
	wxGLPanel* glView = frame.glView;
	const auto& affectedShapes = controller->AffectedShapes();
	for (auto& shape : project->GetWorkNif()->GetShapes()) {
		if (affectedShapes.count(shape->name.get()) == 0)
			continue;

		std::vector<Vector3> verts;
		project->GetLiveVerts(shape, verts);

		// BVH rebuilds only serve picking and brushes; way too expensive per
		// frame. StopPump does one full ApplyPose to catch up.
		glView->UpdateMeshVertices(shape->name.get(), &verts, false, true, false);
	}

	if (cbVis && cbVis->IsChecked())
		controller->UpdateDebugVis(glView->gls, true);

	glView->Render();
}

void PhysicsPreview::OnTimer(wxTimerEvent& WXUNUSED(event)) {
	Pump();
}

void PhysicsPreview::OnIdle(wxIdleEvent& event) {
	if (!pumpActive)
		return;

	if (clock.UntilDue() > 1500)
		wxMilliSleep(1);
	else
		Pump();

	event.RequestMore();
}
