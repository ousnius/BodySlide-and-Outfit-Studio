/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

// BodySlide: posing, animating, skinning and simulating the preview

#include "BodySlideApp.h"
#include "ui/bodyslide/PreviewPanel.h"
#include "files/GameDataStream.h"
#include "utils/ProjectUtil.h"
#include "utils/GameUtil.h"

#include <algorithm>
#include <utility>
#include <wx/wx.h>

using namespace nifly;

void BodySlideApp::UpdatePreviewPhysicsAvailability(bool restart) {
	// Whatever led here replaced the previewed meshes, so the simulation - which
	// holds shapes of those meshes - has to go either way.
	EnablePreviewPhysics(false);

	previewPhysicsAvailable = false;
	for (auto& pp : projects) {
		if (Physics::HasPhysicsLinks(&pp->modNif, {})) {
			previewPhysicsAvailable = true;
			break;
		}
	}

	if (restart && previewPhysicsAvailable)
		EnablePreviewPhysics(true);

	if (preview) {
		preview->ShowPhysicsControls(previewPhysicsAvailable);
		preview->SetPhysicsChecked(previewPhysicsRunning);
	}
}

void BodySlideApp::EnablePreviewPhysics(bool enable) {
	if (enable == previewPhysicsRunning)
		return;

	if (!enable) {
		previewPhysicsRunning = false;
		previewPhysics.clear();
		UpdatePreviewPump();

		// Back to the plain morphed shape the sliders describe, or to the pose
		// without the simulated bones
		RefreshPreviewSkinning();
		return;
	}

	if (!IsPreviewRenderable() || projects.empty())
		return;

	// The simulation reads its kinematic input from the application skeleton
	if (!EnsurePreviewSkinning())
		return;

	for (size_t i = 0; i < projects.size() && i < previewAnims.size(); i++) {
		auto& pp = projects[i];

		// The mod's own folder is the fallback for physics XMLs that aren't
		// installed under the game data path
		const std::string& nifPath = pp->inputFileName;

		std::vector<std::string> warnings;
		auto controller = std::make_unique<Physics::Controller>();
		size_t systemCount = controller->BuildFromNif(
			&pp->modNif,
			previewAnims[i].get(),
			[&nifPath](const std::string& xmlPath) { return GameDataStream::OpenPhysicsXml(xmlPath, nifPath); },
			{},
			warnings);

		for (auto& warning : warnings)
			wxLogWarning("Physics: %s", warning);

		if (systemCount == 0)
			continue;

		// The systems are built from the base shapes of the NIF
		for (auto it = pp->sliderSet.ShapesBegin(); it != pp->sliderSet.ShapesEnd(); ++it) {
			auto morphedVerts = previewMorphedVerts.find(it->first);
			if (morphedVerts != previewMorphedVerts.end())
				controller->SetShapeVertices(it->first, morphedVerts->second);
		}

		controller->ResetDynamics();

		PreviewPhysicsProject entry;
		entry.controller = std::move(controller);
		entry.projectIdx = i;
		previewPhysics.push_back(std::move(entry));
	}

	if (previewPhysics.empty()) {
		wxLogWarning("No physics XMLs could be loaded for the previewed meshes.");
		return;
	}

	previewPhysicsRunning = true;
	previewClock.Reset();
	UpdatePreviewPump();

	// Fills the morphed vertex cache and shows the first simulated frame
	UpdatePreview();
}

bool BodySlideApp::EnsurePreviewSkeleton() {
	if (previewSkeletonPath == Config["Anim/DefaultSkeletonReference"] && AnimSkeleton::getInstance().GetActiveBoneCount() > 0)
		return true;

	// BodySlide has no use for the skeleton outside of the preview, so it's
	// only loaded once something there needs it
	wxString skeletonError;
	if (LoadDefaultSkeletonReference(&skeletonError) != 0) {
		wxMessageBox(skeletonError);
		wxLogError("The preview needs the reference skeleton, which could not be loaded.");
		return false;
	}

	previewSkeletonPath = Config["Anim/DefaultSkeletonReference"];
	return true;
}

bool BodySlideApp::EnsurePreviewSkinning() {
	if (previewAnimsBuilt && previewLooseAnimsBuilt)
		return true;

	if (!EnsurePreviewSkeleton())
		return false;

	if (!previewAnimsBuilt) {
		previewAnims.clear();
		for (auto& pp : projects) {
			auto anim = std::make_unique<AnimInfo>();
			anim->LoadFromNif(&pp->modNif);
			previewAnims.push_back(std::move(anim));
		}

		previewReferenceAnim.reset();
		if (referenceNif) {
			previewReferenceAnim = std::make_unique<AnimInfo>();
			previewReferenceAnim->LoadFromNif(referenceNif.get());
		}

		previewAnimsBuilt = true;
	}

	if (!previewLooseAnimsBuilt) {
		previewLooseAnims.clear();
		for (auto& nif : previewLooseNifs) {
			auto anim = std::make_unique<AnimInfo>();
			anim->LoadFromNif(nif.get());
			previewLooseAnims.push_back(std::move(anim));
		}

		previewLooseAnimsBuilt = true;
	}

	// Loading the skinning may have added custom bones to the skeleton
	ApplyPreviewSkeletonPose();
	return true;
}

void BodySlideApp::SetPreviewLooseNifs(std::vector<std::unique_ptr<NifFile>> nifs) {
	previewLooseAnims.clear();
	previewLooseAnimsBuilt = false;
	previewLooseNifs = std::move(nifs);

	if (IsPreviewPosed() && EnsurePreviewSkinning())
		UpdatePreviewLooseMeshes();
}

void BodySlideApp::UpdatePreviewLooseMeshes() {
	if (!preview)
		return;

	const bool posed = IsPreviewPosed() && previewLooseAnimsBuilt;

	std::vector<Vector3> verts;
	std::vector<Matrix3> poseRotations;
	for (size_t i = 0; i < previewLooseNifs.size(); i++) {
		auto& nif = previewLooseNifs[i];
		for (auto shape : nif->GetShapes()) {
			if (!shape->IsSkinned() || !nif->GetVertsForShape(shape, verts))
				continue;

			auto rotations = PoseRotationsFor(shape->name.get(), &poseRotations);
			if (posed && i < previewLooseAnims.size())
				ApplySkinningToVerts(*previewLooseAnims[i], shape, nif->GetHeader().GetVersion().IsSF(), nullptr, verts, rotations);

			preview->UpdateMeshes(shape->name.get(), &verts, nullptr, &poseRotations);
		}
	}
}

void BodySlideApp::ReleasePreviewSkinning() {
	const bool physicsWasRunning = previewPhysicsRunning;

	// The simulation holds pointers into the skinning, so it goes first
	previewPhysicsRunning = false;
	previewPhysics.clear();

	previewAnims.clear();
	previewReferenceAnim.reset();
	previewAnimsBuilt = false;

	// Nothing holds on to these, but they go along in case the skeleton changes
	previewLooseAnims.clear();
	previewLooseAnimsBuilt = false;

	previewMorphedVerts.clear();
	previewReferenceMorphedVerts.clear();

	if (physicsWasRunning && preview)
		preview->SetPhysicsChecked(false);

	UpdatePreviewPump();
}

bool BodySlideApp::IsPreviewPosed() const {
	return previewAnimIndex >= 0 || previewPoseIndex != PreviewPoseNone;
}

bool BodySlideApp::IsPreviewSkinned() const {
	return previewPhysicsRunning || IsPreviewPosed();
}

AnimationData* BodySlideApp::GetPreviewAnimationData() {
	if (previewAnimIndex < 0 || static_cast<size_t>(previewAnimIndex) >= previewPoses.animationData.size())
		return nullptr;

	return &previewPoses.animationData[previewAnimIndex];
}

void BodySlideApp::ApplyPreviewSkeletonPose() {
	AnimationData* anim = GetPreviewAnimationData();
	if (anim && !anim->framePoses.empty()) {
		const size_t numFrames = anim->framePoses.size();
		const size_t frame = std::min(static_cast<size_t>(previewAnimFrame), numFrames - 1);
		const float blend = static_cast<float>(previewAnimFrame - static_cast<double>(frame));

		if (previewAnimInterpolate && numFrames > 1 && blend > 0.0f) {
			// Playback loops, so the last frame blends back into the first one
			const size_t nextFrame = (frame + 1) % numFrames;
			PoseData::Interpolate(anim->framePoses[frame], anim->framePoses[nextFrame], blend, previewAnimBlendPose);
			previewAnimBlendPose.ApplyToSkeleton();
		}
		else {
			anim->framePoses[frame].ApplyToSkeleton();
		}
		return;
	}

	const int poseDataIndex = previewPoseIndex - PreviewPoseFirst;
	if (poseDataIndex >= 0 && static_cast<size_t>(poseDataIndex) < previewPoses.poseData.size()) {
		previewPoses.poseData[poseDataIndex].ApplyToSkeleton();
		return;
	}

	// An empty pose puts every bone back where the skeleton has it
	PoseData().ApplyToSkeleton();
}

std::vector<Matrix3>* BodySlideApp::PoseRotationsFor(const std::string& shapeName, std::vector<Matrix3>* outPoseRotations) {
	if (!outPoseRotations)
		return nullptr;

	outPoseRotations->clear();
	if (!preview || !preview->WantsPoseRotations(shapeName))
		return nullptr;

	return outPoseRotations;
}

void BodySlideApp::ApplyPreviewSkinning(size_t projectIdx, const std::string& shapeName, std::vector<Vector3>& verts, std::vector<Matrix3>* outPoseRotations) {
	outPoseRotations = PoseRotationsFor(shapeName, outPoseRotations);

	if (projectIdx >= projects.size() || projectIdx >= previewAnims.size())
		return;

	auto& modNif = projects[projectIdx]->modNif;
	auto shape = modNif.FindBlockByName<NiShape>(shapeName);
	if (!shape || !shape->IsSkinned())
		return;

	// The skinning indexes the vertices of the NIF it was loaded from
	if (verts.size() != shape->GetNumVertices())
		return;

	const AnimPoseOverrideMap* poseOverrides = nullptr;
	for (auto& physics : previewPhysics) {
		if (physics.projectIdx == projectIdx && physics.controller->AffectedShapes().count(shapeName) != 0) {
			poseOverrides = &physics.controller->PoseOverrides();
			break;
		}
	}

	// Without a pose only the simulated shapes move, everything else stays as
	// the sliders shape it
	if (!poseOverrides && !IsPreviewPosed())
		return;

	ApplySkinningToVerts(*previewAnims[projectIdx], shape, modNif.GetHeader().GetVersion().IsSF(), poseOverrides, verts, outPoseRotations);
}

void BodySlideApp::ApplyPreviewReferenceSkinning(std::vector<Vector3>& verts, std::vector<Matrix3>* outPoseRotations) {
	outPoseRotations = PoseRotationsFor(referenceShapeName, outPoseRotations);

	if (!referenceNif || !previewReferenceAnim || !IsPreviewPosed())
		return;

	auto shape = referenceNif->FindBlockByName<NiShape>(referenceShapeName);
	if (!shape || !shape->IsSkinned() || verts.size() != shape->GetNumVertices())
		return;

	ApplySkinningToVerts(*previewReferenceAnim, shape, referenceNif->GetHeader().GetVersion().IsSF(), nullptr, verts, outPoseRotations);
}

void BodySlideApp::ReskinPreview(bool allShapes) {
	if (!IsPreviewRenderable() || !previewAnimsBuilt)
		return;

	std::vector<Vector3> verts;
	std::vector<Matrix3> poseRotations;
	if (allShapes) {
		for (size_t pi = 0; pi < projects.size(); pi++) {
			auto& pp = projects[pi];
			for (auto it = pp->sliderSet.ShapesBegin(); it != pp->sliderSet.ShapesEnd(); ++it) {
				auto morphedVerts = previewMorphedVerts.find(it->first);
				if (morphedVerts == previewMorphedVerts.end())
					continue;

				verts = morphedVerts->second;
				ApplyPreviewSkinning(pi, it->first, verts, &poseRotations);
				preview->UpdateMeshes(it->first, &verts, nullptr, &poseRotations);
			}
		}

		if (!previewReferenceMorphedVerts.empty()) {
			verts = previewReferenceMorphedVerts;
			ApplyPreviewReferenceSkinning(verts, &poseRotations);
			preview->UpdateMeshes(referenceShapeName, &verts, nullptr, &poseRotations);
		}

		UpdatePreviewLooseMeshes();
	}
	else {
		// Only the shapes the simulation actually drives have to be skinned again
		for (auto& physics : previewPhysics) {
			for (auto& shapeName : physics.controller->AffectedShapes()) {
				auto morphedVerts = previewMorphedVerts.find(shapeName);
				if (morphedVerts == previewMorphedVerts.end())
					continue;

				verts = morphedVerts->second;
				ApplyPreviewSkinning(physics.projectIdx, shapeName, verts, &poseRotations);
				preview->UpdateMeshes(shapeName, &verts, nullptr, &poseRotations);
			}
		}
	}

	preview->Render();
}

void BodySlideApp::RestoreUnskinnedPreview() {
	if (IsPreviewRenderable()) {
		for (auto& morphedVerts : previewMorphedVerts)
			preview->UpdateMeshes(morphedVerts.first, &morphedVerts.second);

		if (!previewReferenceMorphedVerts.empty())
			preview->UpdateMeshes(referenceShapeName, &previewReferenceMorphedVerts);

		UpdatePreviewLooseMeshes();
		preview->Render();
	}

	previewMorphedVerts.clear();
	previewReferenceMorphedVerts.clear();
}

void BodySlideApp::RefreshPreviewSkinning() {
	if (!IsPreviewSkinned()) {
		RestoreUnskinnedPreview();
		return;
	}

	// A preview of loose NIFs has no project, so no sliders to run either
	if (projects.empty()) {
		if (IsPreviewRenderable() && EnsurePreviewSkinning()) {
			UpdatePreviewLooseMeshes();
			preview->Render();
		}
		return;
	}

	// Nothing was skinned so far, so the sliders have to run once to fill the
	// morphed vertex cache
	if (!previewAnimsBuilt || previewMorphedVerts.empty()) {
		UpdatePreview();
		return;
	}

	ReskinPreview(true);
}

void BodySlideApp::UpdatePreviewPump() {
	if (preview)
		preview->SetPumpActive(IsPreviewPumping());
}

void BodySlideApp::PumpPreview() {
	if (!IsPreviewPumping())
		return;

	if (!IsPreviewRenderable()) {
		previewAnimPlaying = false;
		EnablePreviewPhysics(false);
		UpdatePreviewPump();
		return;
	}

	float dtSeconds = 0.0f;
	if (!previewClock.StepDue(dtSeconds))
		return;

	bool skeletonMoved = false;
	if (previewAnimPlaying) {
		AnimationData* anim = GetPreviewAnimationData();
		if (anim && !anim->framePoses.empty()) {
			const double numFrames = static_cast<double>(anim->framePoses.size());
			const double frameDuration = anim->frameDuration > 0.0f ? anim->frameDuration : 1.0 / 30.0;
			const double prevFrame = previewAnimFrame;

			previewAnimFrame = std::fmod(previewAnimFrame + dtSeconds * previewAnimSpeed / frameDuration, numFrames);
			if (!(previewAnimFrame >= 0.0))
				previewAnimFrame = 0.0;

			// Without interpolation only whole frames are ever shown, so nothing
			// changes until the position has crossed into the next one
			if (previewAnimInterpolate || static_cast<size_t>(previewAnimFrame) != static_cast<size_t>(prevFrame)) {
				ApplyPreviewSkeletonPose();
				skeletonMoved = true;
				preview->SyncAnimationControls();
			}
		}
	}

	for (auto& physics : previewPhysics)
		physics.controller->Step(dtSeconds);

	if (skeletonMoved || !previewPhysics.empty())
		ReskinPreview(skeletonMoved);
}

void BodySlideApp::EnsurePreviewPosesLoaded() {
	if (previewPosesLoaded)
		return;

	previewPosesLoaded = true;
	previewPoses.LoadData(ProjectUtil::GetProjectPath() + "/PoseData");

	switch (targetGame) {
		case SKYRIMSE:
		case SKYRIMVR:
		case FO4:
		case FO4VR:
			previewPoses.LoadGamePoses(GameUtil::GetGameDataPath(targetGame).ToUTF8().data(), targetGame == FO4 || targetGame == FO4VR);
			break;
		default: break;
	}

	// Listed right away, but only read once selected
	if (CanLoadPreviewAnimations())
		previewPoses.LoadFavoriteAnimations(GetAnimationFavoritesGame());
}

std::string BodySlideApp::GetAnimationFavoritesGame() const {
	return GameUtil::TargetGames[targetGame].ToStdString();
}

std::vector<std::string> BodySlideApp::GetPreviewPoseNames() {
	EnsurePreviewPosesLoaded();

	std::vector<std::string> names;
	names.reserve(previewPoses.poseData.size());
	for (auto& pose : previewPoses.poseData)
		names.push_back(pose.name);

	return names;
}

void BodySlideApp::SetPreviewPose(int poseIndex) {
	if (poseIndex < PreviewPoseNone || poseIndex >= PreviewPoseFirst + static_cast<int>(previewPoses.poseData.size()))
		poseIndex = PreviewPoseNone;

	if (poseIndex == previewPoseIndex)
		return;

	previewPoseIndex = poseIndex;

	// An animation decides the pose for as long as it's selected
	if (previewAnimIndex >= 0)
		return;

	JumpPreviewSkeleton();
}

void BodySlideApp::JumpPreviewSkeleton() {
	ApplyPreviewSkeletonPose();

	// The bones teleport, so re-seat the simulated bodies on them instead of
	// letting them read the jump as a huge velocity
	for (auto& physics : previewPhysics)
		physics.controller->ResetDynamics();

	RefreshPreviewSkinning();
}

void BodySlideApp::ResetPreviewPoses() {
	previewAnimPlaying = false;
	ReleasePreviewSkinning();

	previewPoses = PoseDataCollection();
	previewPosesLoaded = false;
	previewPoseIndex = PreviewPoseNone;
	previewAnimIndex = -1;
	previewAnimFrame = 0.0;

	// The next game may use a different skeleton
	previewSkeletonPath.clear();

	if (preview)
		preview->RefreshPoseControls();

	UpdatePreview();
}

bool BodySlideApp::CanLoadPreviewAnimations() const {
	switch (targetGame) {
		case SKYRIM:
		case SKYRIMSE:
		case SKYRIMVR:
		case FO4:
		case FO4VR: return true;
		default: return false;
	}
}

bool BodySlideApp::LoadPreviewAnimation(const std::string& filePath, std::string& errorOut) {
	EnsurePreviewPosesLoaded();

	// A file that's listed already, as a favorite or loaded before, is selected
	// instead of being added again
	const size_t prevCount = previewPoses.animationData.size();
	const AnimationData* anim = previewPoses.AddAnimationFile(filePath);

	for (size_t i = 0; i < previewPoses.animationData.size(); i++) {
		if (&previewPoses.animationData[i] != anim)
			continue;

		if (SetPreviewAnimation(static_cast<int>(i), &errorOut))
			return true;

		// A file that can't be read isn't worth listing. New entries go last,
		// so dropping it doesn't move the selected one.
		if (previewPoses.animationData.size() > prevCount)
			previewPoses.animationData.pop_back();

		return false;
	}

	return false;
}

std::vector<std::string> BodySlideApp::GetPreviewAnimationNames() {
	EnsurePreviewPosesLoaded();

	std::vector<std::string> names;
	names.reserve(previewPoses.animationData.size());
	for (auto& anim : previewPoses.animationData)
		names.push_back(anim.name);

	return names;
}

bool BodySlideApp::IsPreviewAnimationFavorite(int animIndex) const {
	if (animIndex < 0 || static_cast<size_t>(animIndex) >= previewPoses.animationData.size())
		return false;

	return previewPoses.IsFavoriteAnimation(previewPoses.animationData[animIndex]);
}

void BodySlideApp::SetPreviewAnimationFavorite(int animIndex, bool favorite) {
	if (animIndex < 0 || static_cast<size_t>(animIndex) >= previewPoses.animationData.size())
		return;

	previewPoses.SetFavoriteAnimation(previewPoses.animationData[animIndex], GetAnimationFavoritesGame(), favorite);
}

bool BodySlideApp::SetPreviewAnimation(int animIndex, std::string* errorOut) {
	if (animIndex < 0 || static_cast<size_t>(animIndex) >= previewPoses.animationData.size())
		animIndex = -1;

	// Favorites are only read once they're picked
	if (animIndex >= 0) {
		wxString error;
		if (!PoseDataCollection::LoadAnimationFrames(previewPoses.animationData[animIndex], error)) {
			if (errorOut)
				*errorOut = error.ToUTF8().data();
			return false;
		}
	}

	previewAnimIndex = animIndex;
	previewAnimFrame = 0.0;

	if (animIndex < 0)
		previewAnimPlaying = false;

	JumpPreviewSkeleton();
	UpdatePreviewPump();
	return true;
}

void BodySlideApp::SetPreviewAnimationPlaying(bool playing) {
	AnimationData* anim = GetPreviewAnimationData();
	if (!anim || anim->framePoses.empty())
		playing = false;

	if (playing == previewAnimPlaying)
		return;

	previewAnimPlaying = playing;

	if (playing) {
		previewClock.Reset();
	}
	else {
		// Land on a whole frame, so the pose left behind is the one the frame
		// slider reports rather than a blend between two of them
		previewAnimFrame = std::floor(previewAnimFrame);
		ApplyPreviewSkeletonPose();
		RefreshPreviewSkinning();
	}

	UpdatePreviewPump();
}

size_t BodySlideApp::GetPreviewAnimationFrameCount() {
	AnimationData* anim = GetPreviewAnimationData();
	return anim ? anim->GetNumFrames() : 0;
}

void BodySlideApp::SeekPreviewAnimation(int frame) {
	const size_t numFrames = GetPreviewAnimationFrameCount();
	if (numFrames == 0)
		return;

	previewAnimFrame = static_cast<double>(std::clamp(frame, 0, static_cast<int>(numFrames) - 1));
	JumpPreviewSkeleton();
}

void BodySlideApp::SetPreviewAnimationInterpolate(bool interpolate) {
	previewAnimInterpolate = interpolate;
}

void BodySlideApp::InjectPreviewCameraYaw(float deltaDegrees) {
	for (auto& physics : previewPhysics)
		physics.controller->InjectCameraYaw(deltaDegrees);
}

void BodySlideApp::SetPreviewWind(int directionIndex, int strengthPercent) {
	for (auto& physics : previewPhysics) {
		physics.controller->SetWindDirection(Physics::WindDirectionFromIndex(directionIndex));
		physics.controller->SetWindStrength(strengthPercent / 100.0f);
	}
}
