/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

// Outfit Studio window: animation playback

#include "OutfitStudioFrame.h"
#include "FrameHelpers.h"
#include "OutfitStudioApp.h"
#include "utils/ConfigurationManager.h"
#include "ui/wxStateButton.h"
#include "physics/Controller.h"
#include "utils/TargetGame.h"
#include "components/SliderPresets.h"
#include "files/HkxFile.h"
#include "utils/GameUtil.h"

#include <cmath>
#include <cstdlib>
#include <iterator>
#include <sstream>
#include <wx/display.h>

using namespace nifly;

namespace {

// The playback timer only backs OnAnimIdle up for stretches where no idle events
// arrive. It cannot pace playback on its own: on MSW it is a WM_TIMER, which
// Windows rounds up to a multiple of the ~15.6 ms system clock tick, making 15 ms
// the shortest useful request and 64 Hz the hard ceiling - measured, that holds
// even with the clock resolution already raised.
constexpr int AnimPlaybackTimerIntervalMS = 15;

// Same marks as BodySlide's favorite outfits, presets and animations
constexpr const char* FavoriteStar = "\xE2\x98\x85";
constexpr const char* FavoriteStarIcon = "/res/images/FavoriteStar.png";
constexpr const char* FavoriteStarEmptyIcon = "/res/images/FavoriteStarEmpty.png";

} // namespace

AnimationData* OutfitStudioFrame::GetSelectedAnimation() {
	if (!cAnimationName)
		return nullptr;

	int sel = cAnimationName->GetSelection();
	if (sel == wxNOT_FOUND)
		return nullptr;

	// The "<None>" sentinel entry has null client data.
	return reinterpret_cast<AnimationData*>(cAnimationName->GetClientData(sel));
}

double OutfitStudioFrame::GetAnimPlaybackSpeed() {
	// Indices match the "animSpeed" choice contents in the XRC.
	static const double speeds[] = {0.25, 0.5, 1.0, 1.5, 2.0};
	constexpr int defaultSel = 2; // 1x

	int sel = animSpeedChoice ? animSpeedChoice->GetSelection() : defaultSel;
	if (sel < 0 || sel >= static_cast<int>(std::size(speeds)))
		sel = defaultSel;

	return speeds[sel];
}

bool OutfitStudioFrame::IsAnimationInterpolated() const {
	return animInterpolateCheck && animInterpolateCheck->GetValue();
}

void OutfitStudioFrame::ApplyAnimationFrame(double framePos, bool updatePoseGUI) {
	AnimationData* anim = GetSelectedAnimation();
	if (!anim || anim->framePoses.empty())
		return;

	const size_t numFrames = anim->framePoses.size();
	if (!(framePos >= 0.0) || framePos > double(numFrames))
		framePos = 0.0;

	animCurrentFrame = framePos;

	const size_t frame = std::min(size_t(framePos), numFrames - 1);
	const float blend = float(framePos - double(frame));

	if (IsAnimationInterpolated() && numFrames > 1 && blend > 0.0f) {
		// Playback loops, so the last frame blends back into the first one.
		const size_t nextFrame = (frame + 1) % numFrames;
		PoseData::Interpolate(anim->framePoses[frame], anim->framePoses[nextFrame], blend, animBlendPose);
		animBlendPose.ApplyToSkeleton();
	}
	else {
		anim->framePoses[frame].ApplyToSkeleton();
	}

	// Updating the pose sliders is skipped during playback; they are synced
	// once when playback pauses.
	if (updatePoseGUI)
		PoseToGUI();

	if (!project->bPose)
		ActivatePose(true);
	else
		ApplyPose();

	if (animFrameSlider && animFrameSlider->GetValue() != static_cast<int>(frame))
		animFrameSlider->SetValue(static_cast<int>(frame));

	if (animFrameText)
		animFrameText->SetLabel(wxString::Format("%lu / %lu", static_cast<unsigned long>(frame + 1), static_cast<unsigned long>(numFrames)));
}

void OutfitStudioFrame::UpdateAnimationPlayerUI() {
	AnimationData* anim = GetSelectedAnimation();
	size_t numFrames = anim ? anim->GetNumFrames() : 0;

	if (animFavoriteButton) {
		const bool favorite = anim && poseDataCollection.IsFavoriteAnimation(*anim);
		animFavoriteButton->SetBitmap(wxBitmap(wxString::FromUTF8(Config["AppDir"] + (favorite ? FavoriteStarIcon : FavoriteStarEmptyIcon)), wxBITMAP_TYPE_PNG));
		animFavoriteButton->Enable(anim != nullptr);
	}

	if (animPlayPauseButton) {
		animPlayPauseButton->Enable(numFrames > 0);
		animPlayPauseButton->SetLabel(animPlaying ? _("Pause") : _("Play"));
	}

	const size_t currentFrame = numFrames > 0 ? std::min(static_cast<size_t>(animCurrentFrame), numFrames - 1) : 0;

	if (animFrameSlider) {
		animFrameSlider->Enable(numFrames > 1);
		animFrameSlider->SetRange(0, numFrames > 1 ? static_cast<int>(numFrames) - 1 : 1);
		animFrameSlider->SetValue(static_cast<int>(currentFrame));
	}

	if (animSpeedChoice)
		animSpeedChoice->Enable(numFrames > 0);

	if (animInterpolateCheck)
		animInterpolateCheck->Enable(numFrames > 1);

	if (animFrameText) {
		if (numFrames > 0)
			animFrameText->SetLabel(wxString::Format("%lu / %lu", static_cast<unsigned long>(currentFrame + 1), static_cast<unsigned long>(numFrames)));
		else
			animFrameText->SetLabel("0 / 0");
	}
}

int OutfitStudioFrame::GetAnimTargetFps() {
	int refresh = 0;

	const int displayIndex = wxDisplay::GetFromWindow(this);
	if (displayIndex != wxNOT_FOUND) {
		wxVideoMode mode = wxDisplay(static_cast<unsigned int>(displayIndex)).GetCurrentMode();
		refresh = mode.refresh;
	}

	if (refresh <= 0)
		refresh = 60;

	return std::min(refresh, 120);
}

wxLongLong OutfitStudioFrame::GetAnimFrameDueInMicro() {
	const wxLongLong period = 1000000 / std::max(animTargetFps, 1);
	const wxLongLong elapsed = animPlaybackWatch.TimeInMicro() - animLastDrawMicro;
	return elapsed >= period ? wxLongLong(0) : period - elapsed;
}

void OutfitStudioFrame::RestartAnimationClock() {
	animClockBaseFrame = animCurrentFrame;
	animPlaybackWatch.Start();
	animLastDrawMicro = 0;
}

void OutfitStudioFrame::PumpAnimationPlayback() {
	if (!animPlaying)
		return;

	AnimationData* anim = GetSelectedAnimation();
	if (!anim || anim->framePoses.empty()) {
		PauseAnimationPlayback();
		return;
	}

	// Ticks arrive from idle, the timer and mouse motion at wildly different
	// rates, so the drawing rate is decided here rather than by whatever woke us.
	if (GetAnimFrameDueInMicro() > 0)
		return;

	animLastDrawMicro = animPlaybackWatch.TimeInMicro();

	const size_t numFrames = anim->framePoses.size();
	const double frameDuration = anim->frameDuration > 0.0f ? anim->frameDuration : 1.0f / 30.0f;

	// Where the animation is due, from elapsed time rather than a tick count,
	// so a late or dropped tick shifts nothing but the smoothness.
	double framePos = animClockBaseFrame + (animPlaybackWatch.TimeInMicro().ToDouble() / 1000000.0) * GetAnimPlaybackSpeed() / frameDuration;
	framePos = std::fmod(framePos, double(numFrames));
	if (framePos < 0.0)
		framePos = 0.0;

	// Without interpolation only whole frames are ever shown, so skip the work
	// entirely until the position has actually crossed into the next one.
	if (!IsAnimationInterpolated() && size_t(framePos) == size_t(animCurrentFrame))
		return;

	ApplyAnimationFrame(framePos, false);
}

void OutfitStudioFrame::StartAnimationPlayback() {
	AnimationData* anim = GetSelectedAnimation();
	if (!anim || anim->framePoses.empty() || animPlaying)
		return;

	animPlaying = true;
	SetAnimationPlaybackLock(true);
	UpdateAnimationPlayerUI();

	// Resolved once here: the window does not move between displays mid-play.
	animTargetFps = GetAnimTargetFps();

	RestartAnimationClock();
	SetHighResolutionTimers(true);
	Bind(wxEVT_IDLE, &OutfitStudioFrame::OnAnimIdle, this);
	animPlaybackTimer.Start(AnimPlaybackTimerIntervalMS);

	// Physics switches from its own pump to stepping in lockstep with the
	// displayed animation frames (see ApplyPose)
	UpdatePhysicsState();
}

void OutfitStudioFrame::PauseAnimationPlayback() {
	if (!animPlaying)
		return;

	animPlaybackTimer.Stop();
	Unbind(wxEVT_IDLE, &OutfitStudioFrame::OnAnimIdle, this);
	SetHighResolutionTimers(false);
	animPlaying = false;
	SetAnimationPlaybackLock(false);

	// Land on a whole frame, so the pose left behind is the one the player
	// reports rather than a blend between two of them. Unconditional because
	// this is also the ApplyPose that rebuilds the mesh BVHs playback skipped.
	if (GetSelectedAnimation())
		ApplyAnimationFrame(std::floor(animCurrentFrame), false);
	else
		ApplyPose();

	// The frame stays applied, so a paused animation behaves like a pose. Sync
	// the pose sliders that were skipped while playing.
	PoseToGUI();
	UpdateAnimationPlayerUI();

	// Back to the physics pump if physics stays enabled with pose mode on
	UpdatePhysicsState();
}

void OutfitStudioFrame::ResetAnimationList() {
	PauseAnimationPlayback();
	animCurrentFrame = 0;

	poseDataCollection.animationData.clear();

	// Read again every time, since BodySlide may have changed them
	const TargetGame targetGame = wxGetApp().targetGame;
	if (GetPoseHkxFormat(targetGame))
		poseDataCollection.LoadFavoriteAnimations(GameUtil::TargetGames[targetGame].ToStdString());

	if (cAnimationName) {
		cAnimationName->Clear();
		PopulateAnimationList();
	}

	UpdateAnimationPlayerUI();
}

void OutfitStudioFrame::PopulateAnimationList() {
	if (!cAnimationName)
		return;

	AnimationData* selected = GetSelectedAnimation();

	cAnimationName->Freeze();
	cAnimationName->Clear();
	cAnimationName->Append("<None>", (void*)nullptr);
	cAnimationName->SetSelection(0);

	for (auto& anim : poseDataCollection.animationData) {
		wxString label = wxString::FromUTF8(anim.name);
		if (poseDataCollection.IsFavoriteAnimation(anim))
			label = wxString::FromUTF8(FavoriteStar) + " " + label;

		const int idx = cAnimationName->Append(label, &anim);
		if (&anim == selected)
			cAnimationName->SetSelection(idx);
	}

	cAnimationName->Thaw();
}

void OutfitStudioFrame::SetAnimationPlaybackLock(bool locked) {
	const bool enable = !locked;

	if (menuBar) {
		for (size_t i = 0; i < menuBar->GetMenuCount(); ++i)
			menuBar->EnableTop(i, enable);
	}

	// Disabling a container disables its children without discarding their own
	// enabled state, so whatever the current tab, tool and pose selection had
	// decided (per-tool toolbar entries, the save/delete pose buttons, Pose to
	// Mesh) comes back unchanged on unlock.
	const std::initializer_list<wxWindow*> lockedWindows = {toolBarH,
															toolBarV,
															meshTabButton,
															boneTabButton,
															colorsTabButton,
															segmentTabButton,
															partitionTabButton,
															lightsTabButton,
															outfitShapes,
															outfitBones,
															sliderScroll,
															sliderFilter,
															bonesFilter,
															masksPane,
															notesPane,
															posePane,
															physicsPane,
															FindWindow(XRCID("cbFixedWeight")),
															FindWindow(XRCID("cbNormalizeWeights")),
															// The player itself stays usable; only the
															// controls that would swap the animation out
															// from under it are locked.
															cAnimationName,
															FindWindow(XRCID("importAnimationFile"))};

	for (wxWindow* w : lockedWindows) {
		if (w)
			w->Enable(enable);
	}

	// Swallow every menu, toolbar and accelerator command while playing.
	// Dynamically bound handlers run before the static event table, so this
	// blocks all of them in one place.
	if (locked)
		Bind(wxEVT_MENU, &OutfitStudioFrame::OnBlockedCommandDuringPlayback, this);
	else
		Unbind(wxEVT_MENU, &OutfitStudioFrame::OnBlockedCommandDuringPlayback, this);
}

void OutfitStudioFrame::OnBlockedCommandDuringPlayback(wxCommandEvent& WXUNUSED(event)) {
	// Intentionally empty: not calling Skip() drops the command.
}

void OutfitStudioFrame::OnSelectAnimation(wxCommandEvent& WXUNUSED(event)) {
	PauseAnimationPlayback();
	animCurrentFrame = 0;

	AnimationData* anim = GetSelectedAnimation();
	if (anim && !anim->IsLoaded()) {
		// Favorites are only read once they're picked
		wxString loadError;
		bool loaded = false;
		{
			wxBusyCursor busy;
			loaded = PoseDataCollection::LoadAnimationFrames(*anim, loadError);
		}

		if (!loaded) {
			wxMessageBox(loadError, _("Load Animation File"), wxOK | wxICON_ERROR, this);
			cAnimationName->SetSelection(0); // "<None>"
			anim = nullptr;
		}
	}

	if (anim) {
		// The animation takes over the skeleton pose; deselect any pose in
		// the pose list without firing its handler.
		if (auto cPoseName = (wxComboBox*)FindWindowByName("cPoseName"))
			cPoseName->SetStringSelection("<New>");

		ApplyAnimationFrame(0);
	}
	else {
		// "<None>" sentinel: reset all bones like the "<New>" pose entry.
		ResetAllPoseBones();
		PoseToGUI();
		ApplyPose();
	}

	UpdateAnimationPlayerUI();
	UpdatePoseButtonStates();
}

void OutfitStudioFrame::OnLoadHkxAnimation(wxCommandEvent& WXUNUSED(event)) {
	wxFileDialog loadDlg(this, _("Select animation file"), wxEmptyString, wxEmptyString, "HKX animation files (*.hkx)|*.hkx", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
	if (loadDlg.ShowModal() == wxID_CANCEL)
		return;

	std::string skeletonHkxPath;
	if (!GetReferenceSkeletonHkxPath(skeletonHkxPath, _("Load Animation File")))
		return;

	// A file that's listed already, as a favorite or loaded before, is selected
	// instead of being added again
	const size_t prevCount = poseDataCollection.animationData.size();
	AnimationData* anim = poseDataCollection.AddAnimationFile(std::string(loadDlg.GetPath().ToUTF8().data()));

	wxString loadError;
	bool loaded = false;
	{
		wxBusyCursor busy;
		loaded = PoseDataCollection::LoadAnimationFrames(*anim, loadError);
	}

	if (!loaded) {
		// A file that can't be read isn't worth listing. New entries go last,
		// so dropping it doesn't move the selected one.
		if (poseDataCollection.animationData.size() > prevCount)
			poseDataCollection.animationData.pop_back();

		wxMessageBox(loadError, _("Load Animation File"), wxOK | wxICON_ERROR, this);
		return;
	}

	if (!cAnimationName)
		return;

	PopulateAnimationList();
	for (unsigned int i = 0; i < cAnimationName->GetCount(); i++) {
		if (cAnimationName->GetClientData(i) == anim) {
			cAnimationName->SetSelection(i);
			break;
		}
	}

	wxCommandEvent dummy;
	OnSelectAnimation(dummy);

	if (statusBar)
		statusBar->SetStatusText(_("Animation file loaded."), 0);
}

void OutfitStudioFrame::OnAnimationFavorite(wxCommandEvent& WXUNUSED(event)) {
	AnimationData* anim = GetSelectedAnimation();
	if (!anim)
		return;

	poseDataCollection.SetFavoriteAnimation(*anim, GameUtil::TargetGames[wxGetApp().targetGame].ToStdString(), !poseDataCollection.IsFavoriteAnimation(*anim));

	PopulateAnimationList();
	UpdateAnimationPlayerUI();
}

void OutfitStudioFrame::OnAnimPlayPause(wxCommandEvent& WXUNUSED(event)) {
	if (animPlaying)
		PauseAnimationPlayback();
	else
		StartAnimationPlayback();
}

void OutfitStudioFrame::OnAnimFrameSlider(wxScrollEvent& event) {
	if (!GetSelectedAnimation())
		return;

	// Seeking teleports the pose; re-seat the physics bodies on it instead of
	// letting them interpret the jump as a huge velocity.
	if (physicsRunning)
		physics->ResetDynamics();

	ApplyAnimationFrame(std::max(event.GetPosition(), 0), !animPlaying);

	// Seeking moves the playhead, so playback has to continue from there.
	if (animPlaying)
		RestartAnimationClock();
}

void OutfitStudioFrame::OnAnimSpeedChanged(wxCommandEvent& WXUNUSED(event)) {
	// The frame position is derived from the elapsed time and the speed, so the
	// clock has to restart at the current frame for the new speed to take over
	// from there rather than jump.
	if (animPlaying)
		RestartAnimationClock();
}

void OutfitStudioFrame::OnAnimInterpolateChanged(wxCommandEvent& WXUNUSED(event)) {
	AnimationData* anim = GetSelectedAnimation();
	if (!anim)
		return;

	// While playing, the next tick picks the new mode up on its own.
	if (!animPlaying)
		ApplyAnimationFrame(animCurrentFrame);
}

void OutfitStudioFrame::OnAnimPlaybackTimer(wxTimerEvent& WXUNUSED(event)) {
	PumpAnimationPlayback();
}

void OutfitStudioFrame::OnAnimIdle(wxIdleEvent& event) {
	if (!animPlaying)
		return;

	// Idle events asked for with RequestMore come back as fast as the event loop
	// can spin, so hand the CPU back while the next frame is still a way off
	// instead of burning a core on the wait. A millisecond at a time stays well
	// inside one frame even at the 120 fps cap, where frames are 8.3 ms apart.
	if (GetAnimFrameDueInMicro() > 1500)
		wxMilliSleep(1);
	else
		PumpAnimationPlayback();

	event.RequestMore();
}
