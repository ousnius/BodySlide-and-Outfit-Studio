/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "AnimationPlayer.h"
#include "PosePanel.h"
#include "PhysicsPreview.h"
#include "FrameHelpers.h"
#include "OutfitStudioApp.h"
#include "OutfitStudioFrame.h"
#include "project/OutfitProject.h"
#include "utils/ConfigurationManager.h"
#include "utils/GameUtil.h"
#include "utils/TargetGame.h"

#include <cmath>
#include <iterator>
#include <wx/wx.h>
#include <wx/xrc/xmlres.h>

using namespace nifly;

namespace {

// The playback timer only backs OnIdle up for stretches where no idle events
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

AnimationPlayer::AnimationPlayer(OutfitStudioFrame& frame)
	: frame(frame)
	, timer(this) {
	Bind(wxEVT_TIMER, &AnimationPlayer::OnTimer, this);
}

AnimationPlayer::~AnimationPlayer() {
	timer.Stop();
}

void AnimationPlayer::Init() {
	animationName = (wxComboBox*)frame.FindWindowByName("cAnimationName");
	favoriteButton = (wxButton*)frame.FindWindowByName("animFavorite");
	playPauseButton = (wxButton*)frame.FindWindowByName("animPlayPause");
	frameSlider = (wxSlider*)frame.FindWindowByName("animFrameSlider");
	frameText = dynamic_cast<wxStaticText*>(frame.FindWindowByName("animFrameText"));
	speedChoice = (wxChoice*)frame.FindWindowByName("animSpeed");
	interpolateCheck = (wxCheckBox*)frame.FindWindowByName("animInterpolate");

	frame.Bind(wxEVT_COMBOBOX, &AnimationPlayer::OnSelect, this, XRCID("cAnimationName"));
	frame.Bind(wxEVT_BUTTON, &AnimationPlayer::OnLoadHkx, this, XRCID("importAnimationFile"));
	frame.Bind(wxEVT_BUTTON, &AnimationPlayer::OnFavorite, this, XRCID("animFavorite"));
	frame.Bind(wxEVT_BUTTON, &AnimationPlayer::OnPlayPause, this, XRCID("animPlayPause"));
	for (auto& type : CommandScrollEventTypes())
		frame.Bind(type, &AnimationPlayer::OnFrameSlider, this, XRCID("animFrameSlider"));
	frame.Bind(wxEVT_CHOICE, &AnimationPlayer::OnSpeedChanged, this, XRCID("animSpeed"));
	frame.Bind(wxEVT_CHECKBOX, &AnimationPlayer::OnInterpolateChanged, this, XRCID("animInterpolate"));
}

AnimationData* AnimationPlayer::GetSelected() {
	if (!animationName)
		return nullptr;

	int sel = animationName->GetSelection();
	if (sel == wxNOT_FOUND)
		return nullptr;

	// The "<None>" sentinel entry has null client data.
	return reinterpret_cast<AnimationData*>(animationName->GetClientData(sel));
}

double AnimationPlayer::GetPlaybackSpeed() {
	// Indices match the "animSpeed" choice contents in the XRC.
	static const double speeds[] = {0.25, 0.5, 1.0, 1.5, 2.0};
	constexpr int defaultSel = 2; // 1x

	int sel = speedChoice ? speedChoice->GetSelection() : defaultSel;
	if (sel < 0 || sel >= static_cast<int>(std::size(speeds)))
		sel = defaultSel;

	return speeds[sel];
}

bool AnimationPlayer::IsInterpolated() const {
	return interpolateCheck && interpolateCheck->GetValue();
}

void AnimationPlayer::ApplyFrame(double framePos, bool updatePoseGUI) {
	AnimationData* anim = GetSelected();
	if (!anim || anim->framePoses.empty())
		return;

	const size_t numFrames = anim->framePoses.size();
	if (!(framePos >= 0.0) || framePos > double(numFrames))
		framePos = 0.0;

	currentFrame = framePos;

	const size_t frameIndex = std::min(size_t(framePos), numFrames - 1);
	const float blend = float(framePos - double(frameIndex));

	if (IsInterpolated() && numFrames > 1 && blend > 0.0f) {
		// Playback loops, so the last frame blends back into the first one.
		const size_t nextFrame = (frameIndex + 1) % numFrames;
		PoseData::Interpolate(anim->framePoses[frameIndex], anim->framePoses[nextFrame], blend, blendPose);
		blendPose.ApplyToSkeleton();
	}
	else {
		anim->framePoses[frameIndex].ApplyToSkeleton();
	}

	// Updating the pose sliders is skipped during playback; they are synced
	// once when playback pauses.
	if (updatePoseGUI)
		frame.posePanel->PoseToGUI();

	if (!frame.project->bPose)
		frame.posePanel->ActivatePose(true);
	else
		frame.ApplyPose();

	if (frameSlider && frameSlider->GetValue() != static_cast<int>(frameIndex))
		frameSlider->SetValue(static_cast<int>(frameIndex));

	if (frameText)
		frameText->SetLabel(wxString::Format("%lu / %lu", static_cast<unsigned long>(frameIndex + 1), static_cast<unsigned long>(numFrames)));
}

void AnimationPlayer::UpdateUI() {
	AnimationData* anim = GetSelected();
	size_t numFrames = anim ? anim->GetNumFrames() : 0;

	if (favoriteButton) {
		const bool favorite = anim && frame.poseDataCollection.IsFavoriteAnimation(*anim);
		favoriteButton->SetBitmap(wxBitmap(wxString::FromUTF8(Config["AppDir"] + (favorite ? FavoriteStarIcon : FavoriteStarEmptyIcon)), wxBITMAP_TYPE_PNG));
		favoriteButton->Enable(anim != nullptr);
	}

	if (playPauseButton) {
		playPauseButton->Enable(numFrames > 0);
		playPauseButton->SetLabel(playing ? _("Pause") : _("Play"));
	}

	const size_t shownFrame = numFrames > 0 ? std::min(static_cast<size_t>(currentFrame), numFrames - 1) : 0;

	if (frameSlider) {
		frameSlider->Enable(numFrames > 1);
		frameSlider->SetRange(0, numFrames > 1 ? static_cast<int>(numFrames) - 1 : 1);
		frameSlider->SetValue(static_cast<int>(shownFrame));
	}

	if (speedChoice)
		speedChoice->Enable(numFrames > 0);

	if (interpolateCheck)
		interpolateCheck->Enable(numFrames > 1);

	if (frameText) {
		if (numFrames > 0)
			frameText->SetLabel(wxString::Format("%lu / %lu", static_cast<unsigned long>(shownFrame + 1), static_cast<unsigned long>(numFrames)));
		else
			frameText->SetLabel("0 / 0");
	}
}

wxLongLong AnimationPlayer::GetFrameDueInMicro() {
	const wxLongLong period = 1000000 / std::max(targetFps, 1);
	const wxLongLong elapsed = watch.TimeInMicro() - lastDrawMicro;
	return elapsed >= period ? wxLongLong(0) : period - elapsed;
}

void AnimationPlayer::RestartClock() {
	clockBaseFrame = currentFrame;
	watch.Start();
	lastDrawMicro = 0;
}

void AnimationPlayer::Pump() {
	if (!playing)
		return;

	AnimationData* anim = GetSelected();
	if (!anim || anim->framePoses.empty()) {
		Pause();
		return;
	}

	// Ticks arrive from idle, the timer and mouse motion at wildly different
	// rates, so the drawing rate is decided here rather than by whatever woke us.
	if (GetFrameDueInMicro() > 0)
		return;

	lastDrawMicro = watch.TimeInMicro();

	const size_t numFrames = anim->framePoses.size();
	const double frameDuration = anim->frameDuration > 0.0f ? anim->frameDuration : 1.0f / 30.0f;

	// Where the animation is due, from elapsed time rather than a tick count,
	// so a late or dropped tick shifts nothing but the smoothness.
	double framePos = clockBaseFrame + (watch.TimeInMicro().ToDouble() / 1000000.0) * GetPlaybackSpeed() / frameDuration;
	framePos = std::fmod(framePos, double(numFrames));
	if (framePos < 0.0)
		framePos = 0.0;

	// Without interpolation only whole frames are ever shown, so skip the work
	// entirely until the position has actually crossed into the next one.
	if (!IsInterpolated() && size_t(framePos) == size_t(currentFrame))
		return;

	ApplyFrame(framePos, false);
}

void AnimationPlayer::Start() {
	AnimationData* anim = GetSelected();
	if (!anim || anim->framePoses.empty() || playing)
		return;

	playing = true;
	frame.SetAnimationPlaybackLock(true);
	UpdateUI();

	// Resolved once here: the window does not move between displays mid-play.
	targetFps = GetTargetFps(&frame);

	RestartClock();
	SetHighResolutionTimers(true);
	frame.Bind(wxEVT_IDLE, &AnimationPlayer::OnIdle, this);
	timer.Start(AnimPlaybackTimerIntervalMS);

	// Physics switches from its own pump to stepping in lockstep with the
	// displayed animation frames (see ApplyPose)
	frame.physicsPreview->UpdateState();
}

void AnimationPlayer::Pause() {
	if (!playing)
		return;

	timer.Stop();
	frame.Unbind(wxEVT_IDLE, &AnimationPlayer::OnIdle, this);
	SetHighResolutionTimers(false);
	playing = false;
	frame.SetAnimationPlaybackLock(false);

	// Land on a whole frame, so the pose left behind is the one the player
	// reports rather than a blend between two of them. Unconditional because
	// this is also the ApplyPose that rebuilds the mesh BVHs playback skipped.
	if (GetSelected())
		ApplyFrame(std::floor(currentFrame), false);
	else
		frame.ApplyPose();

	// The frame stays applied, so a paused animation behaves like a pose. Sync
	// the pose sliders that were skipped while playing.
	frame.posePanel->PoseToGUI();
	UpdateUI();

	// Back to the physics pump if physics stays enabled with pose mode on
	frame.physicsPreview->UpdateState();
}

void AnimationPlayer::Deselect() {
	if (!GetSelected())
		return;

	Pause();
	currentFrame = 0;
	animationName->SetSelection(0); // "<None>"
	UpdateUI();
}

void AnimationPlayer::ResetList() {
	Pause();
	currentFrame = 0;

	frame.poseDataCollection.animationData.clear();

	// Read again every time, since BodySlide may have changed them
	const TargetGame targetGame = wxGetApp().targetGame;
	if (GetPoseHkxFormat(targetGame))
		frame.poseDataCollection.LoadFavoriteAnimations(GameUtil::TargetGames[targetGame].ToStdString());

	if (animationName) {
		animationName->Clear();
		PopulateList();
	}

	UpdateUI();
}

void AnimationPlayer::PopulateList() {
	if (!animationName)
		return;

	AnimationData* selected = GetSelected();

	animationName->Freeze();
	animationName->Clear();
	animationName->Append("<None>", (void*)nullptr);
	animationName->SetSelection(0);

	for (auto& anim : frame.poseDataCollection.animationData) {
		wxString label = wxString::FromUTF8(anim.name);
		if (frame.poseDataCollection.IsFavoriteAnimation(anim))
			label = wxString::FromUTF8(FavoriteStar) + " " + label;

		const int idx = animationName->Append(label, &anim);
		if (&anim == selected)
			animationName->SetSelection(idx);
	}

	animationName->Thaw();
}

void AnimationPlayer::OnSelect(wxCommandEvent& WXUNUSED(event)) {
	Pause();
	currentFrame = 0;

	AnimationData* anim = GetSelected();
	if (anim && !anim->IsLoaded()) {
		// Favorites are only read once they're picked
		wxString loadError;
		bool loaded = false;
		{
			wxBusyCursor busy;
			loaded = PoseDataCollection::LoadAnimationFrames(*anim, loadError);
		}

		if (!loaded) {
			wxMessageBox(loadError, _("Load Animation File"), wxOK | wxICON_ERROR, &frame);
			animationName->SetSelection(0); // "<None>"
			anim = nullptr;
		}
	}

	if (anim) {
		// The animation takes over the skeleton pose; deselect any pose in
		// the pose list without firing its handler.
		if (auto cPoseName = (wxComboBox*)frame.FindWindowByName("cPoseName"))
			cPoseName->SetStringSelection("<New>");

		ApplyFrame(0);
	}
	else {
		// "<None>" sentinel: reset all bones like the "<New>" pose entry.
		frame.posePanel->ResetAllPoseBones();
		frame.posePanel->PoseToGUI();
		frame.ApplyPose();
	}

	UpdateUI();
	frame.posePanel->UpdatePoseButtonStates();
}

void AnimationPlayer::OnLoadHkx(wxCommandEvent& WXUNUSED(event)) {
	wxFileDialog loadDlg(&frame, _("Select animation file"), wxEmptyString, wxEmptyString, "HKX animation files (*.hkx)|*.hkx", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
	if (loadDlg.ShowModal() == wxID_CANCEL)
		return;

	std::string skeletonHkxPath;
	if (!frame.posePanel->GetReferenceSkeletonHkxPath(skeletonHkxPath, _("Load Animation File")))
		return;

	// A file that's listed already, as a favorite or loaded before, is selected
	// instead of being added again
	auto& animations = frame.poseDataCollection.animationData;
	const size_t prevCount = animations.size();
	AnimationData* anim = frame.poseDataCollection.AddAnimationFile(std::string(loadDlg.GetPath().ToUTF8().data()));

	wxString loadError;
	bool loaded = false;
	{
		wxBusyCursor busy;
		loaded = PoseDataCollection::LoadAnimationFrames(*anim, loadError);
	}

	if (!loaded) {
		// A file that can't be read isn't worth listing. New entries go last,
		// so dropping it doesn't move the selected one.
		if (animations.size() > prevCount)
			animations.pop_back();

		wxMessageBox(loadError, _("Load Animation File"), wxOK | wxICON_ERROR, &frame);
		return;
	}

	if (!animationName)
		return;

	PopulateList();
	for (unsigned int i = 0; i < animationName->GetCount(); i++) {
		if (animationName->GetClientData(i) == anim) {
			animationName->SetSelection(i);
			break;
		}
	}

	wxCommandEvent dummy;
	OnSelect(dummy);

	if (frame.statusBar)
		frame.statusBar->SetStatusText(_("Animation file loaded."), 0);
}

void AnimationPlayer::OnFavorite(wxCommandEvent& WXUNUSED(event)) {
	AnimationData* anim = GetSelected();
	if (!anim)
		return;

	auto& poses = frame.poseDataCollection;
	poses.SetFavoriteAnimation(*anim, GameUtil::TargetGames[wxGetApp().targetGame].ToStdString(), !poses.IsFavoriteAnimation(*anim));

	PopulateList();
	UpdateUI();
}

void AnimationPlayer::OnPlayPause(wxCommandEvent& WXUNUSED(event)) {
	if (playing)
		Pause();
	else
		Start();
}

void AnimationPlayer::OnFrameSlider(wxScrollEvent& event) {
	if (!GetSelected())
		return;

	// Seeking teleports the pose; re-seat the physics bodies on it instead of
	// letting them interpret the jump as a huge velocity.
	frame.physicsPreview->ResetDynamics();

	ApplyFrame(std::max(event.GetPosition(), 0), !playing);

	// Seeking moves the playhead, so playback has to continue from there.
	if (playing)
		RestartClock();
}

void AnimationPlayer::OnSpeedChanged(wxCommandEvent& WXUNUSED(event)) {
	// The frame position is derived from the elapsed time and the speed, so the
	// clock has to restart at the current frame for the new speed to take over
	// from there rather than jump.
	if (playing)
		RestartClock();
}

void AnimationPlayer::OnInterpolateChanged(wxCommandEvent& WXUNUSED(event)) {
	AnimationData* anim = GetSelected();
	if (!anim)
		return;

	// While playing, the next tick picks the new mode up on its own.
	if (!playing)
		ApplyFrame(currentFrame);
}

void AnimationPlayer::OnTimer(wxTimerEvent& WXUNUSED(event)) {
	Pump();
}

void AnimationPlayer::OnIdle(wxIdleEvent& event) {
	if (!playing)
		return;

	// Idle events asked for with RequestMore come back as fast as the event loop
	// can spin, so hand the CPU back while the next frame is still a way off
	// instead of burning a core on the wait. A millisecond at a time stays well
	// inside one frame even at the 120 fps cap, where frames are 8.3 ms apart.
	if (GetFrameDueInMicro() > 1500)
		wxMilliSleep(1);
	else
		Pump();

	event.RequestMore();
}
