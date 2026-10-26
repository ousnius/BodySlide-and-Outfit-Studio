/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "../components/TweakBrush.h"
#include "ProjectOperations.h"

#include <memory>
#include <vector>

// Runs one brush stroke on shape meshes and applies its changes to the project. Where the
// stroke goes comes from the caller as pick info, so it works without mouse or keyboard input.
class BrushStrokeController {
public:
	BrushStrokeController(OutfitProject& project, EditorState& editor);

	// Makes sure the shapes are skinned and have the active bone, before a weight stroke
	void PrepareWeightStroke(const std::vector<nifly::NiShape*>& shapes);

	// Starts a stroke and records it as an undo step. Without applyFirstDab, the brush
	// isn't applied at the start position (move brush, segment painting).
	void Begin(const std::vector<Mesh*>& meshes, TweakBrush* brush, TweakPickInfo pick, bool applyFirstDab = true);

	// Continues the stroke. Weight strokes apply their weights to the project as they go.
	RefreshFlags Update(TweakPickInfo pick);

	// Ends the stroke and applies its changes to the project
	RefreshFlags End();

	bool IsActive() const { return stroke != nullptr; }
	TweakBrush* GetBrush() const { return brush; }

	// Pick info for a point on a mesh surface. Origin and normal are in mesh space, view
	// points from the surface towards the viewer.
	static TweakPickInfo PickFromMeshHit(Mesh* mesh, const nifly::Vector3& meshOrigin, const nifly::Vector3& meshNormal, const nifly::Vector3& view);

	// Pick info at a vertex of a mesh, viewed along its normal
	static TweakPickInfo PickAtVertex(Mesh* mesh, int vertex);

	// Smooths all unmasked vertices of the meshes like the smooth brush, as one stroke and undo step.
	// The mask scales the change, so a soft mask border fades it out. keepBorders keeps open mesh edges
	// in place.
	RefreshFlags SmoothMeshes(const std::vector<Mesh*>& meshes, int iterations, float strength, bool keepBorders = true);

	// Vertex moves for the follower meshes after a position edit of the edited meshes: each follower
	// vertex within radius (model units) of their surface moves as far as the surface under it moved
	// across, so parts lying on them (trim, zippers) stay on the surface without sliding along with
	// its vertices. The followers' masks scale their moves.
	static std::vector<UndoStateShape> FollowEdit(const UndoStateProject& edit, const std::vector<Mesh*>& editedMeshes,
												  const std::vector<Mesh*>& followers, float radius);

private:
	OutfitProject& project;
	EditorState& editor;
	TweakBrush* brush = nullptr;
	std::unique_ptr<TweakStroke> stroke;
};
