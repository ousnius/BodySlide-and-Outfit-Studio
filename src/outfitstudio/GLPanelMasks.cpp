/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

// Viewport: masks and vertex colors of the meshes

#include "GLPanel.h"
#include "OutfitStudioFrame.h"
#include "utils/ConfigurationManager.h"
#include "components/UndoHistory.h"
#include "components/MaskOps.h"

#include <cmath>
#include <cstdlib>
#include <iterator>
#include <sstream>

using namespace nifly;

std::unordered_map<std::string, std::vector<float>> wxGLPanel::StashMasks() {
	std::unordered_map<std::string, std::vector<float>> stash;
	std::vector<Mesh*> meshes = gls.GetMeshes();
	for (Mesh* m : meshes) {
		if (m->bPrimitive || !m->mask)
			continue;
		std::vector<float>& mask = stash[m->shapeName];
		mask.resize(m->nVerts);
		std::copy(m->mask.get(), m->mask.get() + m->nVerts, mask.begin());
	}
	return stash;
}

void wxGLPanel::UnstashMasks(const std::unordered_map<std::string, std::vector<float>>& stash) {
	std::vector<Mesh*> meshes = gls.GetMeshes();
	for (Mesh* m : meshes) {
		if (m->bPrimitive)
			continue;

		auto stit = stash.find(m->shapeName);
		if (stit == stash.end())
			continue;
		const std::vector<float>& mask = stit->second;
		// assert(mask.size() == m->nVerts);
		if (static_cast<int>(mask.size()) != m->nVerts)
			continue;
		std::copy(mask.begin(), mask.end(), m->mask.get());
		m->QueueUpdate(Mesh::UpdateType::Mask);
	}
}

void wxGLPanel::ApplyMaskChanges(const std::function<std::unordered_map<int, float>(Mesh&)>& changesForMesh) {
	ApplyMaskChanges(gls.GetActiveMeshes(), changesForMesh);
}

void wxGLPanel::ApplyMaskChanges(const std::vector<Mesh*>& meshes, const std::function<std::unordered_map<int, float>(Mesh&)>& changesForMesh) {
	UndoStateProject* usp = GetUndoHistory()->PushState();
	usp->undoType = UndoType::Mask;

	for (auto& m : meshes) {
		usp->usss.emplace_back();
		UndoStateShape& uss = usp->usss.back();
		uss.shapeName = m->shapeName;

		for (auto& change : changesForMesh(*m)) {
			uss.pointStartState[change.first].x = m->mask[change.first];
			uss.pointEndState[change.first].x = change.second;
		}
	}

	ApplyUndoState(usp, false);

	if (!Config.GetBoolValue("Input/MaskHistory"))
		GetUndoHistory()->PopState();

	os->UpdateUndoTools();
}

void wxGLPanel::MaskLess() {
	ApplyMaskChanges([](Mesh& m) { return MaskOps::GrowShrinkChanges(m, false); });
}

void wxGLPanel::MaskMore() {
	ApplyMaskChanges([](Mesh& m) { return MaskOps::GrowShrinkChanges(m, true); });
}

void wxGLPanel::InvertMask() {
	ApplyMaskChanges([](Mesh& m) { return MaskOps::InvertChanges(m); });
}

void wxGLPanel::ClearMask() {
	ApplyMaskChanges([](Mesh& m) { return MaskOps::ClearChanges(m); });
}

void wxGLPanel::InvertMaskTris(std::unordered_map<uint16_t, float>& mask, const std::string& shapeName) {
	Mesh* m = gls.GetMesh(shapeName);
	if (!m)
		return;

	std::unordered_map<uint16_t, float> triMask;
	for (int t = 0; t < m->nTris; t++) {
		auto& tri = m->tris[t];
		if (mask.find(tri.p1) != mask.end() || mask.find(tri.p2) != mask.end() || mask.find(tri.p3) != mask.end()) {
			triMask.emplace(tri.p1, 1.0f);
			triMask.emplace(tri.p2, 1.0f);
			triMask.emplace(tri.p3, 1.0f);
		}
	}

	std::unordered_map<uint16_t, float> invertMask;
	for (int t = 0; t < m->nTris; t++) {
		auto& tri = m->tris[t];
		if (triMask.find(tri.p1) == triMask.end())
			invertMask.emplace(tri.p1, 1.0f);
		if (triMask.find(tri.p2) == triMask.end())
			invertMask.emplace(tri.p2, 1.0f);
		if (triMask.find(tri.p3) == triMask.end())
			invertMask.emplace(tri.p3, 1.0f);
	}

	mask = std::move(invertMask);
}

nifly::Vector3 wxGLPanel::CreateColorRamp(const float value) {
	float r;
	float g;
	float b;

	if (value <= 0.0f) {
		r = g = b = 1.0;
	}
	else if (value <= 0.25) {
		r = 0.0;
		b = 1.0;
		g = value / 0.25;
	}
	else if (value <= 0.5) {
		r = 0.0;
		g = 1.0;
		b = 1.0 + (-1.0) * (value - 0.25) / 0.25;
	}
	else if (value <= 0.75) {
		r = (value - 0.5) / 0.25;
		g = 1.0;
		b = 0.0;
	}
	else {
		r = 1.0;
		g = 1.0 + (-1.0) * (value - 0.75) / 0.25;
		b = 0.0;
	}

	return nifly::Vector3(r, g, b);
}

void wxGLPanel::ClearMasks() {
	for (auto& m : gls.GetMeshes())
		if (!m->bPrimitive)
			m->MaskFill(0.0f);
}

void wxGLPanel::ClearActiveMask() {
	for (auto& m : gls.GetActiveMeshes())
		m->MaskFill(0.0f);
}

void wxGLPanel::ClearColors() {
	for (auto& m : gls.GetMeshes())
		if (!m->bPrimitive)
			m->ColorFill(nifly::Vector3());
}

void wxGLPanel::ClearActiveColors() {
	for (auto& m : gls.GetActiveMeshes())
		m->ColorFill(nifly::Vector3());
}

void wxGLPanel::GetActiveMask(std::unordered_map<uint16_t, float>& mask) {
	if (gls.GetActiveMeshes().empty())
		return;

	Mesh* m = gls.GetActiveMeshes().back();

	for (int i = 0; i < m->nVerts; i++) {
		if (m->mask[i] != 0.0f)
			mask[i] = m->mask[i];
	}
}

void wxGLPanel::GetActiveUnmasked(std::unordered_map<uint16_t, float>& mask) {
	if (gls.GetActiveMeshes().empty())
		return;

	Mesh* m = gls.GetActiveMeshes().back();

	for (int i = 0; i < m->nVerts; i++)
		if (m->mask[i] == 0.0f)
			mask[i] = m->mask[i];
}

void wxGLPanel::GetShapeMask(std::unordered_map<uint16_t, float>& mask, const std::string& shapeName) {
	Mesh* m = gls.GetMesh(shapeName);
	if (!m)
		return;

	for (int i = 0; i < m->nVerts; i++) {
		if (m->mask[i] != 0.0f)
			mask[i] = m->mask[i];
	}
}

void wxGLPanel::GetShapeUnmasked(std::unordered_map<uint16_t, float>& mask, const std::string& shapeName) {
	Mesh* m = gls.GetMesh(shapeName);
	if (!m)
		return;

	for (int i = 0; i < m->nVerts; i++)
		if (m->mask[i] == 0.0f)
			mask[i] = m->mask[i];
}

void wxGLPanel::SetShapeMask(std::unordered_map<uint16_t, float>& mask, const std::string& shapeName) {
	Mesh* m = gls.GetMesh(shapeName);
	if (!m)
		return;

	for (int i = 0; i < m->nVerts; i++) {
		if (mask.find(i) != mask.end())
			m->mask[i] = mask[i];
		else
			m->mask[i] = 0.0f;
	}

	m->QueueUpdate(Mesh::UpdateType::Mask);
}
