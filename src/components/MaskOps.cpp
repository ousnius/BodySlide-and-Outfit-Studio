/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "MaskOps.h"

#include <unordered_set>

namespace MaskOps {

std::unordered_map<int, float> GrowShrinkChanges(Mesh& m, bool grow) {
	std::unordered_map<int, float> changes;
	if (!m.mask)
		return changes;

	const float to = grow ? 1.0f : 0.0f;

	for (int i = 0; i < m.nVerts; i++) {
		if (grow ? m.mask[i] == 0.0f : m.mask[i] > 0.0f)
			continue;

		m.DoForEachWeldedVertex(i, [&](int wvi) {
			if (m.mask[wvi] != to)
				changes[wvi] = to;
		});
	}

	if (!changes.empty())
		return changes;

	std::unordered_set<int> adjacentPoints;
	for (int i = 0; i < m.nVerts; i++)
		if (grow ? m.mask[i] > 0.0f : m.mask[i] == 0.0f)
			m.GetAdjacentPoints(i, adjacentPoints);

	for (int adj : adjacentPoints)
		if (m.mask[adj] != to)
			changes[adj] = to;

	return changes;
}

std::unordered_map<int, float> InvertChanges(const Mesh& m) {
	std::unordered_map<int, float> changes;
	if (!m.mask)
		return changes;

	for (int i = 0; i < m.nVerts; i++)
		changes[i] = 1.0f - m.mask[i];

	return changes;
}

std::unordered_map<int, float> ClearChanges(const Mesh& m) {
	std::unordered_map<int, float> changes;
	if (!m.mask)
		return changes;

	for (int i = 0; i < m.nVerts; i++)
		changes[i] = 0.0f;

	return changes;
}

void ApplyChanges(Mesh& m, const std::unordered_map<int, float>& changes) {
	if (!m.mask)
		return;

	for (auto& change : changes)
		if (change.first >= 0 && change.first < m.nVerts)
			m.mask[change.first] = change.second;

	m.QueueUpdate(Mesh::UpdateType::Mask);
}

std::unordered_map<uint16_t, float> GetMasked(const Mesh& m) {
	std::unordered_map<uint16_t, float> masked;
	if (!m.mask)
		return masked;

	for (int i = 0; i < m.nVerts; i++)
		if (m.mask[i] != 0.0f)
			masked[static_cast<uint16_t>(i)] = m.mask[i];

	return masked;
}

std::vector<float> GetMask(const Mesh& m) {
	if (!m.mask)
		return std::vector<float>(m.nVerts, 0.0f);

	return std::vector<float>(m.mask.get(), m.mask.get() + m.nVerts);
}

bool SetMask(Mesh& m, const std::vector<float>& mask) {
	if (!m.mask || static_cast<int>(mask.size()) != m.nVerts)
		return false;

	std::copy(mask.begin(), mask.end(), m.mask.get());
	m.QueueUpdate(Mesh::UpdateType::Mask);
	return true;
}

} // namespace MaskOps
