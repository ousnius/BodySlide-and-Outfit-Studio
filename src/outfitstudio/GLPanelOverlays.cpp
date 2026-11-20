/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

// Viewport: overlays - hover highlight, nodes, bones, floor and vertex edit points

#include "GLPanel.h"
#include "AnimationPlayer.h"
#include "OutfitStudioFrame.h"
#include "ShapeTreeItems.h"

#include <cmath>
#include <cstdlib>
#include <iterator>
#include <sstream>

using namespace nifly;

static const char* kHoverHighlightOverlayName = "_shapehoverhilite";

void wxGLPanel::SetHoverHighlight(const std::string& shapeName) {
	if (shapeName == hoverHighlightName)
		return;

	// Drop any existing overlay first.
	if (!hoverHighlightName.empty()) {
		gls.DeleteOverlay(kHoverHighlightOverlayName);
		hoverHighlightName.clear();
	}

	if (shapeName.empty())
		return;

	Mesh* src = gls.GetMesh(shapeName);
	if (!src || src->nVerts <= 0 || src->nTris <= 0 || !src->verts || !src->tris)
		return;

	// Need an active GL context to create buffers for the overlay mesh.
	if (!gls.SetContext())
		return;

	auto* hov = new Mesh();
	hov->nVerts = src->nVerts;
	hov->nTris = src->nTris;
	hov->verts = std::make_unique<nifly::Vector3[]>(hov->nVerts);
	hov->norms = std::make_unique<nifly::Vector3[]>(hov->nVerts);
	hov->tris = std::make_unique<nifly::Triangle[]>(hov->nTris);

	for (int i = 0; i < hov->nVerts; i++) {
		hov->verts[i] = src->verts[i];
		if (src->norms)
			hov->norms[i] = src->norms[i];
	}
	for (int t = 0; t < hov->nTris; t++)
		hov->tris[t] = src->tris[t];

	// Inherit the source's model-space transform so the overlay sits
	// exactly on top of the original shape (skinned shapes have a
	// non-identity matModel).
	hov->matModel = src->matModel;
	hov->xformMeshToModel = src->xformMeshToModel;
	hov->xformModelToMesh = src->xformModelToMesh;

	hov->shapeName = kHoverHighlightOverlayName;
	hov->color = nifly::Vector3(0.25f, 1.0f, 0.25f); // Light green
	hov->prop.alpha = 0.5f;                           // Semi-transparent blend
	hov->material = gls.GetPrimitiveMaterial();       // Unlit, solid tint
	hov->rendermode = Mesh::RenderMode::UnlitSolid;
	hov->overlayLayer = 100;                          // Draw on top of other overlays
	hov->doublesided = true;
	hov->CreateBuffers();

	gls.AddOverlay(hov);
	hoverHighlightName = shapeName;
	Render();
}

void wxGLPanel::ClearHoverHighlight() {
	if (hoverHighlightName.empty())
		return;

	gls.DeleteOverlay(kHoverHighlightOverlayName);
	hoverHighlightName.clear();
	Render();
}

void wxGLPanel::ShowNodes(bool show) {
	nodesMode = show;

	for (auto& m : nodesPoints)
		m->bVisible = show;

	for (auto& m : nodesLines)
		m->bVisible = show;

	if (transformMode)
		ShowTransformTool();

	gls.RenderOneFrame();
}

void wxGLPanel::UpdateNodes() {
	for (auto& m : nodesPoints)
		gls.DeleteOverlay(m);

	for (auto& m : nodesLines)
		gls.DeleteOverlay(m);

	nodesPoints.clear();
	nodesLines.clear();

	auto workNif = os->project->GetWorkNif();

	std::function<void(NiNode*, NiNode*, const Vector3&, const Vector3&)> addChildNodes =
		[&](NiNode* node, NiNode* parent, const Vector3& rootPosition, const Vector3& parentPosition) {
			MatTransform ttg = node->GetTransformToParent();
			NiNode* parentNode = parent;
			while (parentNode) {
				ttg = parentNode->GetTransformToParent().ComposeTransforms(ttg);
				parentNode = workNif->GetParentNode(parentNode);
			}

			Vector3 position = ttg.ApplyTransform(rootPosition);

			std::string nodeName = node->name.get();
			if (!nodeName.empty()) {
				Vector3 renderPosition = Mesh::TransformPosNifToMesh(position);

				auto pointMesh = gls.AddVisPoint(renderPosition, "P_" + nodeName);
				if (pointMesh) {
					pointMesh->bVisible = nodesMode;
					nodesPoints.push_back(pointMesh);
				}

				if (parent) {
					Vector3 renderParentPosition = Mesh::TransformPosNifToMesh(parentPosition);

					auto lineMesh = gls.AddVisSeg(renderParentPosition, renderPosition, "L_" + nodeName);
					if (lineMesh) {
						lineMesh->bVisible = nodesMode;
						nodesLines.push_back(lineMesh);
					}
				}
			}

			for (auto& child : node->childRefs) {
				auto childNode = workNif->GetHeader().GetBlock<NiNode>(child);
				if (childNode)
					addChildNodes(childNode, node, rootPosition, position);
			}
		};

	auto rootNode = workNif->GetRootNode();
	if (rootNode)
		addChildNodes(rootNode, nullptr, rootNode->transform.translation, rootNode->transform.translation);

	UpdateNodeColors();
}

void wxGLPanel::ShowBones(bool show) {
	bonesMode = show;

	if (!show && !hoverBone.empty()) {
		hoverBone.clear();
		UpdateNodeColors();
	}

	for (auto& m : bonesPoints)
		m->bVisible = show;

	for (auto& m : bonesLines)
		m->bVisible = show;

	if (transformMode)
		ShowTransformTool();

	gls.RenderOneFrame();
}

void wxGLPanel::UpdateBones() {
	for (auto& m : bonesPoints)
		gls.DeleteOverlay(m);

	for (auto& m : bonesLines)
		gls.DeleteOverlay(m);

	bonesPoints.clear();
	bonesLines.clear();
	connectorBones.clear();

	auto workAnim = os->project->GetWorkAnim();
	bool isSF = os->project->GetWorkNif()->GetHeader().GetVersion().IsSF();

	std::function<bool(AnimBone*)> addChildBones = [&](AnimBone* parent) {
		bool anyBoneInSelection = false;

		for (auto& cb : parent->children) {
			bool childBonesInSelection = addChildBones(cb);

			if (!cb->boneName.empty()) {
				bool boneInSelection = false;
				for (auto& si : os->GetSelectedItems()) {
					if (workAnim->GetShapeBoneIndex(si->GetShape()->name.get(), cb->boneName) != -1) {
						boneInSelection = true;
						break;
					}
				}

				if (boneInSelection || childBonesInSelection) {
					if (!boneInSelection)
						connectorBones.insert(cb->boneName);

					const MatTransform& toGlobal = os->project->bPose ? cb->xformPoseToGlobal : cb->xformToGlobal;
					Vector3 position = toGlobal.ApplyTransform(Vector3());
					const MatTransform& parentToGlobal = os->project->bPose ? parent->xformPoseToGlobal : parent->xformToGlobal;
					Vector3 parentPosition = parentToGlobal.ApplyTransform(Vector3());

					if (isSF) {
						position *= sfHavokScale;
						parentPosition *= sfHavokScale;
					}

					bool matchesParent = position.IsNearlyEqualTo(parentPosition);

					Vector3 renderPosition = Mesh::TransformPosNifToMesh(position);

					auto pointMesh = gls.AddVisPoint(renderPosition, "BP_" + cb->boneName);
					if (pointMesh) {
						pointMesh->bVisible = bonesMode;
						bonesPoints.push_back(pointMesh);
					}

					Vector3 renderParentPosition = Mesh::TransformPosNifToMesh(parentPosition);

					if (!matchesParent) {
						auto lineMesh = gls.AddVisSeg(renderParentPosition, renderPosition, "BL_" + cb->boneName);
						if (lineMesh) {
							lineMesh->bVisible = bonesMode;
							bonesLines.push_back(lineMesh);
						}
					}

					anyBoneInSelection = true;
				}
			}
		}

		return anyBoneInSelection;
	};

	AnimBone* rb = os->skeleton.GetRootBonePtr();
	if (rb)
		addChildBones(rb);

	UpdateNodeColors();
}

void wxGLPanel::UpdateNodeColors() {
	std::string activeBone = os->GetActiveBone();

	// The active bone (whose weights are shown and edited) is red, other selected bones are pink.
	// Selected nodes and bones are drawn on top.
	for (auto& m : nodesPoints) {
		auto nodeName = m->shapeName.substr(2, m->shapeName.length() - 2);
		if (nodeName == activeBone) {
			m->color = Vector3(1.0f, 0.0f, 0.0f);
			m->overlayLayer = OverlayLayer::NodeSelection;
		}
		else if (os->IsBoneSelected(nodeName)) {
			m->color = Vector3(1.0f, 0.35f, 0.75f);
			m->overlayLayer = OverlayLayer::NodeSelection;
		}
		else {
			m->color = Vector3(0.0f, 1.0f, 0.0f);
			m->overlayLayer = OverlayLayer::Default;
		}
	}

	for (auto& m : nodesLines) {
		auto nodeName = m->shapeName.substr(2, m->shapeName.length() - 2);
		if (nodeName == activeBone) {
			m->color = Vector3(0.7f, 0.0f, 0.0f);
			m->overlayLayer = OverlayLayer::NodeSelection;
		}
		else if (os->IsBoneSelected(nodeName)) {
			m->color = Vector3(0.7f, 0.25f, 0.53f);
			m->overlayLayer = OverlayLayer::NodeSelection;
		}
		else {
			m->color = Vector3(0.0f, 0.7f, 0.0f);
			m->overlayLayer = OverlayLayer::Default;
		}
	}

	// Hovered bones are yellow, or orange when also active, and drawn on top.
	// Connector bones are grey unless selected.
	for (auto& m : bonesPoints) {
		auto nodeName = m->shapeName.substr(3, m->shapeName.length() - 3);
		const bool active = nodeName == activeBone;
		const bool selected = !active && os->IsBoneSelected(nodeName);
		if (!hoverBone.empty() && nodeName == hoverBone) {
			m->color = active ? Vector3(1.0f, 0.5f, 0.0f) : Vector3(1.0f, 1.0f, 0.0f);
			m->overlayLayer = OverlayLayer::NodeSelection;
		}
		else if (active) {
			m->color = Vector3(1.0f, 0.0f, 0.0f);
			m->overlayLayer = OverlayLayer::NodeSelection;
		}
		else if (selected) {
			m->color = Vector3(1.0f, 0.35f, 0.75f);
			m->overlayLayer = OverlayLayer::NodeSelection;
		}
		else if (connectorBones.count(nodeName) != 0) {
			m->color = Vector3(0.4f, 0.4f, 0.4f);
			m->overlayLayer = OverlayLayer::Default;
		}
		else {
			m->color = Vector3(0.0f, 1.0f, 0.0f);
			m->overlayLayer = OverlayLayer::Default;
		}
	}

	for (auto& m : bonesLines) {
		auto nodeName = m->shapeName.substr(3, m->shapeName.length() - 3);
		const bool active = nodeName == activeBone;
		const bool selected = !active && os->IsBoneSelected(nodeName);
		if (!hoverBone.empty() && nodeName == hoverBone) {
			m->color = active ? Vector3(0.9f, 0.45f, 0.0f) : Vector3(0.9f, 0.9f, 0.0f);
			m->overlayLayer = OverlayLayer::NodeSelection;
		}
		else if (active) {
			m->color = Vector3(0.7f, 0.0f, 0.0f);
			m->overlayLayer = OverlayLayer::NodeSelection;
		}
		else if (selected) {
			m->color = Vector3(0.7f, 0.25f, 0.53f);
			m->overlayLayer = OverlayLayer::NodeSelection;
		}
		else if (connectorBones.count(nodeName) != 0) {
			m->color = Vector3(0.35f, 0.35f, 0.35f);
			m->overlayLayer = OverlayLayer::Default;
		}
		else {
			m->color = Vector3(0.0f, 0.7f, 0.0f);
			m->overlayLayer = OverlayLayer::Default;
		}
	}
}

void wxGLPanel::SetHoverBone(const std::string& bone) {
	if (bone == hoverBone)
		return;

	hoverBone = bone;
	UpdateNodeColors();
}

std::string wxGLPanel::PickBone(int x, int y, bool includeConnectors) {
	if (!bonesMode || !os || os->animation->IsPlaying())
		return std::string();

	const float pointRadius = FromDIP(8);
	const float lineRadius = FromDIP(4);
	// Joints closer together on screen than this count as overlapping.
	const float tieTolerance = FromDIP(1);
	const std::string activeBone = os->GetActiveBone();
	const Vector3 cursor(static_cast<float>(x), static_cast<float>(y), 0.0f);

	// Overlapping joints, like a new bone still on its parent, prefer the
	// active bone and then the one nearest to the camera.
	Mesh* best = nullptr;
	float bestDist = 0.0f;
	float bestDepth = 0.0f;
	bool bestActive = false;

	// A skipped connector joint under the cursor also hides the lines starting
	// there, so it doesn't pick the bone below it instead.
	bool connectorJointHit = false;

	auto consider = [&](Mesh* m, float dist, float depth) {
		if (!includeConnectors && connectorBones.count(m->shapeName.substr(3)) != 0) {
			if (m->nVerts == 1)
				connectorJointHit = true;
			return;
		}

		const bool active = m->shapeName.compare(3, std::string::npos, activeBone) == 0;
		bool better = !best || dist < bestDist - tieTolerance;
		if (!better && std::fabs(dist - bestDist) <= tieTolerance) {
			if (active != bestActive)
				better = active;
			else
				better = depth < bestDepth;
		}

		if (better) {
			best = m;
			bestDist = dist;
			bestDepth = depth;
			bestActive = active;
		}
	};

	for (auto& m : bonesPoints) {
		if (!m->bVisible || m->nVerts < 1)
			continue;

		float sx, sy, depth;
		if (!gls.ProjectPointToViewport(m->verts[0], sx, sy, &depth))
			continue;

		const float dist = Vector3(sx, sy, 0.0f).DistanceTo(cursor);
		if (dist <= pointRadius)
			consider(m, dist, depth);
	}

	// Segments belong to the bone at their end, away from the parent.
	if (!best && !connectorJointHit) {
		for (auto& m : bonesLines) {
			if (!m->bVisible || m->nVerts < 2)
				continue;

			float sx0, sy0, depth0, sx1, sy1, depth1;
			if (!gls.ProjectPointToViewport(m->verts[0], sx0, sy0, &depth0) || !gls.ProjectPointToViewport(m->verts[1], sx1, sy1, &depth1))
				continue;

			const Vector3 a(sx0, sy0, 0.0f);
			const Vector3 ab = Vector3(sx1, sy1, 0.0f) - a;
			const float lenSq = ab.dot(ab);
			float t = lenSq > 0.0f ? (cursor - a).dot(ab) / lenSq : 0.0f;
			t = std::clamp(t, 0.0f, 1.0f);

			const float dist = (a + ab * t).DistanceTo(cursor);
			if (dist <= lineRadius)
				consider(m, dist, depth0 + (depth1 - depth0) * t);
		}
	}

	if (!best)
		return std::string();

	return best->shapeName.substr(3);
}

void wxGLPanel::ShowFloor(bool show) {
	floorMode = show;

	for (auto& m : floorMeshes)
		m->bVisible = show;

	gls.RenderOneFrame();
}

void wxGLPanel::UpdateFloor() {
	for (auto& m : floorMeshes)
		gls.DeleteMesh(m);

	floorMeshes = gls.AddFloor();

	for (auto& m : floorMeshes)
		m->bVisible = floorMode;
}

void wxGLPanel::ShowVertexEdit(bool show) {
	for (auto& m : gls.GetMeshes())
		if (m)
			m->bShowPoints = false;

	if (show) {
		if (os->activeItem) {
			Mesh* m = GetMesh(os->activeItem->GetShape()->name.get());
			if (m) {
				m->bShowPoints = true;
				m->ComputeAvgEdgeLength();
				m->QueueUpdate(Mesh::UpdateType::Mask);
			}
		}
	}
}

void wxGLPanel::SetVertexEdit(bool on) {
	vertexEdit = on;
	ShowVertexEdit(on);
	Render();
}
