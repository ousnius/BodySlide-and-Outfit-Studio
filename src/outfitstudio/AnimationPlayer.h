/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "components/PoseData.h"

#include <wx/event.h>
#include <wx/longlong.h>
#include <wx/stopwatch.h>
#include <wx/timer.h>

class OutfitStudioFrame;
class wxButton;
class wxCheckBox;
class wxChoice;
class wxComboBox;
class wxIdleEvent;
class wxScrollEvent;
class wxSlider;
class wxStaticText;

// Plays HKX animations on the skeleton of an Outfit Studio window, with the player controls of its pose pane.
// Every frame of a loaded animation is stored as a regular PoseData and applied through the same code path as
// poses, so a paused animation behaves exactly like an applied pose.
class AnimationPlayer : public wxEvtHandler {
public:
	explicit AnimationPlayer(OutfitStudioFrame& frame);
	~AnimationPlayer() override;

	// Finds the player controls in the window and binds their events
	void Init();

	// True while an animation is being played back; all editing is locked
	bool IsPlaying() const { return playing; }
	// The frame position, fractional while interpolating
	double GetCurrentFrame() const { return currentFrame; }

	AnimationData* GetSelected();
	// Applies the pose at the given frame position, blending the two frames
	// around a fractional one when interpolation is on.
	void ApplyFrame(double framePos, bool updatePoseGUI = true);
	void Start();
	void Pause();
	// One playback tick, safe to call from any event source at any rate
	void Pump();
	// Selects no animation anymore, for when a pose takes over the skeleton
	void Deselect();
	// Empties the animation list down to the favorites, which are listed but
	// only read once selected
	void ResetList();
	void UpdateUI();

private:
	double GetPlaybackSpeed();
	bool IsInterpolated() const;
	// Microseconds until the next frame should be drawn, 0 when it is due.
	wxLongLong GetFrameDueInMicro();
	// Restarts the playback clock at the current frame. Needed whenever the
	// position or the speed changes while playing, since the frame is derived
	// from the time elapsed since the clock was last started.
	void RestartClock();
	// Refills the animation drop-down from the animation entries, keeping the
	// selection
	void PopulateList();

	void OnSelect(wxCommandEvent& event);
	void OnLoadHkx(wxCommandEvent& event);
	void OnFavorite(wxCommandEvent& event);
	void OnPlayPause(wxCommandEvent& event);
	void OnFrameSlider(wxScrollEvent& event);
	void OnSpeedChanged(wxCommandEvent& event);
	void OnInterpolateChanged(wxCommandEvent& event);
	void OnTimer(wxTimerEvent& event);
	// Bound only while an animation plays; this is what drives the frame rate,
	// the timer being a fallback.
	void OnIdle(wxIdleEvent& event);

	OutfitStudioFrame& frame;

	wxComboBox* animationName = nullptr;
	wxButton* favoriteButton = nullptr;
	wxButton* playPauseButton = nullptr;
	wxSlider* frameSlider = nullptr;
	wxStaticText* frameText = nullptr;
	wxChoice* speedChoice = nullptr;
	wxCheckBox* interpolateCheck = nullptr;

	wxTimer timer;
	bool playing = false;
	// Frame position, fractional while interpolating.
	double currentFrame = 0.0;
	// Runs while playing; together with clockBaseFrame it tells which frame
	// is due, independent of how often ticks actually arrive.
	wxStopWatch watch;
	double clockBaseFrame = 0.0;
	// Frame pacing: the watch reading at which the last frame was drawn, and
	// the rate being aimed for, resolved once per playback.
	wxLongLong lastDrawMicro = 0;
	int targetFps = 60;
	// Scratch pose for interpolated frames, kept around so that playback does
	// not reallocate the bone list every tick.
	PoseData blendPose;
};
