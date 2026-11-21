/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "physics/PumpClock.h"

#include "Object3d.hpp"

#include <memory>
#include <string>
#include <unordered_set>
#include <vector>
#include <wx/event.h>
#include <wx/timer.h>

class OutfitStudioFrame;
class wxCheckBox;
class wxChoice;
class wxIdleEvent;
class wxPanel;
class wxScrollEvent;
class wxSlider;
class wxTextCtrl;

namespace Physics {
class Controller;
struct GrabTarget;
struct PatchRequest;
struct SystemInfo;
} // namespace Physics

// The physics preview (HDT-SMP) of an Outfit Studio window, with the controls of its physics pane.
// The simulation runs while the physics checkbox is set and either pose mode is active or an animation
// is playing.
class PhysicsPreview : public wxEvtHandler {
public:
	explicit PhysicsPreview(OutfitStudioFrame& frame);
	~PhysicsPreview() override;

	// Finds the physics controls in the window and binds their events
	void Init();

	// Simulation built and active (either pump mode or animation lockstep)
	bool IsRunning() const { return running; }
	// Whether any loaded mesh references a physics XML. The physics pane only
	// exists on the bones tab, so showing it takes both this and the tab.
	bool IsAvailable() const { return available; }
	// The own idle and timer pump is bound (pose mode without animation playback)
	bool IsPumping() const { return pumpActive; }

	// Single authority over building/tearing down the simulation; call after
	// anything that changes one of those conditions or the loaded meshes.
	void UpdateState();
	// Hands the shapes as the sliders morph them to the running simulation.
	void UpdateShapes();
	// Shows the physics controls only while the loaded meshes reference a
	// physics XML, since there is nothing to simulate otherwise. Stops a running
	// simulation when the last of them goes away.
	void UpdateControlsVisibility();
	// Hard teardown for project unload/close: stops the pump and drops all
	// simulation state without touching the (possibly dying) project meshes.
	void Shutdown();

	// The per-frame physics tick while physics runs without animation
	// playback; while an animation plays, ApplyPose steps the simulation in
	// lockstep instead. Internally paced, so like AnimationPlayer::Pump it is
	// safe to call from any event source at any rate.
	void Pump();
	// The lockstep step of a displayed animation frame, by the wall-clock time
	// since the last step. Does nothing unless an animation plays.
	void StepWithAnimation();
	// Re-seats the simulated bodies on the current pose, for when the pose jumps
	void ResetDynamics();

	// The systems the simulation built while it runs, otherwise the ones the loaded meshes link
	std::vector<Physics::SystemInfo> ListSystems() const;
	// Writes one edited property of a physics XML into the running simulation, see Controller::PatchProperty.
	// False when it isn't running or the change needs a rebuild.
	bool PatchProperty(const Physics::PatchRequest& request);
	// Rebuilds the running simulation from the edited XMLs a moment from now. Every call restarts the wait, so dragging
	// a value through many settings rebuilds once at the end. Does nothing while physics isn't running: the next start
	// reads the edits anyway.
	void RequestRebuild();

	// Feeds a horizontal camera rotation into the physics preview so cloth
	// and hair react as if the character turned under a fixed camera. No-op
	// while physics is not simulating.
	void InjectCameraYaw(float deltaDegrees);

	// Whether the viewport should treat a left drag as a grab of the running
	// simulation rather than as whatever the active tool does.
	bool IsGrabEnabled() const;
	// The shapes the simulation moves; empty while it is not running.
	const std::unordered_set<std::string>& GetAffectedShapes() const;
	// Fits the BVHs of those shapes to where the simulation has moved them.
	// The pump leaves them behind on purpose - rebuilding a tree per frame
	// costs far more than it is worth for trees only picking uses - so
	// anything that does pick the simulated mesh has to catch them up first.
	void RefitBVH();

	// Grabbing the mesh with the cursor: drags the physics bones the grabbed
	// patch is skinned to, leaving what the mesh does on the way to the
	// constraints, collisions and gravity of its physics XML. Purely a
	// simulation input - none of it reaches the mesh data or the undo history.
	bool BeginGrab(const std::vector<Physics::GrabTarget>& targets);
	void UpdateGrab(const nifly::Vector3& offset);
	void EndGrab();

private:
	// Builds the systems from the XMLs of the edit session, logging what couldn't be done. Returns how many it built.
	size_t BuildSystems();
	// Rebuilds the running simulation, keeping the bones where they are and moving as they are, so a structural edit
	// doesn't drop the cloth back onto the pose
	void Rebuild();

	void StartPump();
	void StopPump();
	// Puts the grab checkbox in the state a (not) running simulation allows
	void UpdateGrabControl(bool enabled);
	// The same for the collision ball checkbox
	void UpdateProbeControl(bool enabled);
	// Shows or hides the ball, its controls and its collider to match the box
	void UpdateProbeState();
	// Pushes the ball's position and size into the simulation and the viewport
	void ApplyProbe();
	// Pushes the wind controls into the simulation; the controller is built
	// fresh every time physics starts, so it has no idea of either value.
	void ApplyWind();

	void OnPhysicsCheckBox(wxCommandEvent& event);
	void OnVisCheckBox(wxCommandEvent& event);
	void OnGrabCheckBox(wxCommandEvent& event);
	void OnProbeCheckBox(wxCommandEvent& event);
	// Each ball slider writes its value into the field beside it, and each field
	// puts the handle back where the value is; either way the field is what is read.
	void OnProbeSlider(wxScrollEvent& event, wxTextCtrl* text);
	void OnProbeAxisSlider(wxScrollEvent& event);
	void OnProbeText(wxTextCtrl* text, wxSlider* slider);
	void OnProbeAxisText(wxCommandEvent& event);
	void OnWindSlider(wxScrollEvent& event);
	void OnWindDir(wxCommandEvent& event);
	void OnTimer(wxTimerEvent& event);
	void OnRebuildTimer(wxTimerEvent& event);
	void OnIdle(wxIdleEvent& event);

	OutfitStudioFrame& frame;

	wxCheckBox* cbPhysics = nullptr;
	wxCheckBox* cbVis = nullptr;
	wxCheckBox* cbGrab = nullptr;
	wxCheckBox* cbProbe = nullptr;
	wxPanel* probePanel = nullptr;
	wxSlider* probeX = nullptr;
	wxSlider* probeY = nullptr;
	wxSlider* probeZ = nullptr;
	wxSlider* probeSize = nullptr;
	wxTextCtrl* probeXText = nullptr;
	wxTextCtrl* probeYText = nullptr;
	wxTextCtrl* probeZText = nullptr;
	wxTextCtrl* probeSizeText = nullptr;
	wxSlider* windSlider = nullptr;
	wxChoice* windDir = nullptr;

	std::unique_ptr<Physics::Controller> controller;
	bool available = false;
	wxTimer timer;
	// One shot, armed by RequestRebuild
	wxTimer rebuildTimer;
	bool running = false;
	bool pumpActive = false;
	Physics::PumpClock clock;
};
