/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "McpSnapshots.h"
#include "OutfitStudioMcp.h"
#include "../components/MaskOps.h"
#include "../program/OutfitStudio.h"

#include <algorithm>
#include <cmath>
#include <set>

using nlohmann::json;
using namespace nifly;

namespace {

// FNV-1a over raw bytes, enough to tell whether data changed
class Hash {
public:
	void Add(const void* data, size_t size) {
		auto bytes = static_cast<const uint8_t*>(data);
		for (size_t i = 0; i < size; i++) {
			value ^= bytes[i];
			value *= 1099511628211ull;
		}
	}

	template<typename T>
	void Add(const T& item) {
		Add(&item, sizeof(T));
	}

	uint64_t value = 14695981039346656037ull;
};

// Rounded, so the float error of applying and undoing a change doesn't count as a change
int32_t Quantize(float value, float step) {
	return static_cast<int32_t>(std::lround(value / step));
}

json Pair(size_t before, size_t after) {
	return json::array({before, after});
}

} // namespace

McpProjectDigest McpDigest(OutfitStudioMcp& mcp) {
	OutfitProject& project = mcp.Project();
	ProjectOperations ops = mcp.Ops();
	NifFile* nif = project.GetWorkNif();
	AnimInfo* anim = project.GetWorkAnim();

	McpProjectDigest digest;
	for (size_t i = 0; i < project.SliderCount(); i++)
		digest.sliders.push_back(project.GetSliderName(i));
	if (NiShape* reference = project.GetBaseShape())
		digest.reference = reference->name.get();

	for (auto* shape : nif->GetShapes()) {
		const std::string name = shape->name.get();
		McpProjectDigest::Shape& d = digest.shapes[name];

		nif->GetVertsForShape(shape, d.positions);
		d.vertices = d.positions.size();

		std::vector<Triangle> tris;
		shape->GetTriangles(tris);
		d.triangles = tris.size();
		Hash triangleHash;
		for (auto& t : tris) {
			triangleHash.Add(t.p1);
			triangleHash.Add(t.p2);
			triangleHash.Add(t.p3);
		}
		d.triangleHash = triangleHash.value;

		std::vector<Vector2> uvs;
		nif->GetUvsForShape(shape, uvs);
		Hash uvHash;
		if (!uvs.empty())
			uvHash.Add(uvs.data(), uvs.size() * sizeof(Vector2));
		d.uvHash = uvHash.value;

		if (Mesh* m = project.GetHost().GetShapeMesh(name))
			d.masked = MaskOps::GetMasked(*m).size();

		auto bones = anim->shapeBones.find(name);
		if (bones != anim->shapeBones.end()) {
			for (auto& bone : bones->second) {
				// Sorted, so the hash doesn't depend on the order of the map
				std::vector<std::pair<uint16_t, float>> weights;
				if (auto ptr = anim->GetWeightsPtr(name, bone))
					weights.assign(ptr->begin(), ptr->end());
				std::sort(weights.begin(), weights.end());

				Hash hash;
				for (auto& w : weights) {
					hash.Add(w.first);
					hash.Add(Quantize(w.second, 1e-5f));
				}
				d.bones[bone] = hash.value;
			}
		}

		for (size_t i = 0; i < project.SliderCount(); i++) {
			if (project.SliderZap(i) || project.SliderUV(i))
				continue;

			std::vector<Vector3> deltas;
			if (!ops.GetSliderDeltas(shape, project.GetSliderName(i), deltas).ok)
				continue;

			McpProjectDigest::Slider s;
			Hash hash;
			for (size_t v = 0; v < deltas.size(); v++) {
				// The same threshold as the morph analysis
				const float length = deltas[v].length();
				if (length <= 1e-4f)
					continue;
				s.moved++;
				s.maxDelta = std::max(s.maxDelta, length);
				const uint32_t index = static_cast<uint32_t>(v);
				hash.Add(index);
				hash.Add(Quantize(deltas[v].x, 1e-4f));
				hash.Add(Quantize(deltas[v].y, 1e-4f));
				hash.Add(Quantize(deltas[v].z, 1e-4f));
			}
			s.hash = hash.value;
			if (s.moved)
				d.sliders[project.GetSliderName(i)] = s;
		}
	}

	return digest;
}

json McpDigestDiff(const McpProjectDigest& before, const McpProjectDigest& after, size_t limit) {
	json diff = json::object();
	bool changed = false;

	auto namesOnlyIn = [](const auto& a, const auto& b) {
		json list = json::array();
		for (auto& item : a)
			if (b.find(item.first) == b.end())
				list.push_back(item.first);
		return list;
	};

	json added = namesOnlyIn(after.shapes, before.shapes);
	json removed = namesOnlyIn(before.shapes, after.shapes);
	if (!added.empty())
		diff["shapesAdded"] = added;
	if (!removed.empty())
		diff["shapesRemoved"] = removed;
	changed = changed || !added.empty() || !removed.empty();

	std::set<std::string> slidersBefore(before.sliders.begin(), before.sliders.end());
	std::set<std::string> slidersAfter(after.sliders.begin(), after.sliders.end());
	json slidersAdded = json::array(), slidersRemoved = json::array();
	for (auto& s : slidersAfter)
		if (!slidersBefore.count(s))
			slidersAdded.push_back(s);
	for (auto& s : slidersBefore)
		if (!slidersAfter.count(s))
			slidersRemoved.push_back(s);
	if (!slidersAdded.empty())
		diff["slidersAdded"] = slidersAdded;
	if (!slidersRemoved.empty())
		diff["slidersRemoved"] = slidersRemoved;
	changed = changed || !slidersAdded.empty() || !slidersRemoved.empty();

	if (before.reference != after.reference) {
		diff["reference"] = {{"before", before.reference}, {"after", after.reference}};
		changed = true;
	}

	json shapes = json::object();
	for (auto& item : after.shapes) {
		auto old = before.shapes.find(item.first);
		if (old == before.shapes.end())
			continue;

		const McpProjectDigest::Shape& a = old->second;
		const McpProjectDigest::Shape& b = item.second;
		json entry = json::object();

		if (a.vertices != b.vertices)
			entry["vertices"] = Pair(a.vertices, b.vertices);
		if (a.triangles != b.triangles)
			entry["triangles"] = Pair(a.triangles, b.triangles);
		else if (a.triangleHash != b.triangleHash)
			entry["trianglesChanged"] = true;
		if (a.uvHash != b.uvHash)
			entry["uvsChanged"] = true;
		if (a.masked != b.masked)
			entry["maskedVertices"] = Pair(a.masked, b.masked);

		if (a.vertices == b.vertices) {
			size_t moved = 0;
			float largest = 0.0f;
			for (size_t v = 0; v < a.positions.size() && v < b.positions.size(); v++) {
				float distance = a.positions[v].DistanceTo(b.positions[v]);
				if (distance > 1e-5f) {
					moved++;
					largest = std::max(largest, distance);
				}
			}
			if (moved) {
				entry["movedVertices"] = moved;
				entry["largestMove"] = largest;
			}
		}

		json bonesAdded = namesOnlyIn(b.bones, a.bones);
		json bonesRemoved = namesOnlyIn(a.bones, b.bones);
		json weightsChanged = json::array();
		for (auto& bone : b.bones) {
			auto oldBone = a.bones.find(bone.first);
			if (oldBone != a.bones.end() && oldBone->second != bone.second)
				weightsChanged.push_back(bone.first);
		}
		if (!bonesAdded.empty())
			entry["bonesAdded"] = bonesAdded;
		if (!bonesRemoved.empty())
			entry["bonesRemoved"] = bonesRemoved;
		if (!weightsChanged.empty())
			entry["weightsChanged"] = weightsChanged;

		json sliders = json::array();
		size_t sliderChanges = 0;
		std::set<std::string> sliderNames;
		for (auto& s : a.sliders)
			sliderNames.insert(s.first);
		for (auto& s : b.sliders)
			sliderNames.insert(s.first);
		for (auto& name : sliderNames) {
			auto sa = a.sliders.find(name);
			auto sb = b.sliders.find(name);
			McpProjectDigest::Slider none;
			const McpProjectDigest::Slider& da = sa != a.sliders.end() ? sa->second : none;
			const McpProjectDigest::Slider& db = sb != b.sliders.end() ? sb->second : none;
			if (da.hash == db.hash && da.moved == db.moved)
				continue;

			sliderChanges++;
			if (sliders.size() < limit)
				sliders.push_back({{"slider", name}, {"moved", Pair(da.moved, db.moved)}, {"maxDelta", json::array({da.maxDelta, db.maxDelta})}});
		}
		if (sliderChanges) {
			entry["slidersChanged"] = sliderChanges;
			entry["sliders"] = sliders;
		}

		if (!entry.empty()) {
			shapes[item.first] = entry;
			changed = true;
		}
	}

	if (!shapes.empty())
		diff["shapes"] = shapes;
	diff["changed"] = changed;
	return diff;
}

void McpSnapshotStore::Track(const OutfitProject* project) {
	uint64_t currentId = project ? project->GetInstanceId() : 0;
	if (currentId != projectId) {
		entries.clear();
		projectId = currentId;
	}
}

McpSnapshotStore::Entry* McpSnapshotStore::Find(const std::string& name) {
	for (auto& entry : entries)
		if (entry.name == name)
			return &entry;
	return nullptr;
}

void McpSnapshotStore::Save(OutfitStudioMcp& mcp, const std::string& name) {
	Track(&mcp.Project());
	if (name.rfind("auto-", 0) == 0)
		throw McpToolError("Names starting with 'auto-' are for the automatic snapshots.");

	Remove(name);
	size_t named = std::count_if(entries.begin(), entries.end(), [](const Entry& e) { return !e.automatic; });
	if (named >= MaxNamed)
		throw McpToolError("There are already " + std::to_string(MaxNamed) + " snapshots, delete one with snapshot_delete.");

	Entry entry;
	entry.name = name;
	entry.snapshot = mcp.Ops().CaptureSnapshot();
	entry.digest = McpDigest(mcp);
	entries.push_back(std::move(entry));
}

std::string McpSnapshotStore::SaveAutomatic(OutfitStudioMcp& mcp, const std::string& tool, const std::string& keep) {
	Track(&mcp.Project());

	size_t automatic = std::count_if(entries.begin(), entries.end(), [](const Entry& e) { return e.automatic; });
	if (automatic >= MaxAutomatic) {
		auto oldest = std::find_if(entries.begin(), entries.end(), [&](const Entry& e) { return e.automatic && e.name != keep; });
		if (oldest != entries.end())
			entries.erase(oldest);
	}

	Entry entry;
	entry.name = "auto-" + std::to_string(++automaticCount);
	entry.snapshot = mcp.Ops().CaptureSnapshot();
	entry.digest = McpDigest(mcp);
	entry.automatic = true;
	entry.tool = tool;
	entries.push_back(std::move(entry));
	return entries.back().name;
}

bool McpSnapshotStore::Remove(const std::string& name) {
	auto it = std::find_if(entries.begin(), entries.end(), [&](const Entry& e) { return e.name == name; });
	if (it == entries.end())
		return false;
	entries.erase(it);
	return true;
}

json McpSnapshotStore::Names() const {
	json names = json::array();
	for (auto& entry : entries)
		names.push_back(entry.name);
	return names;
}

json McpSnapshotStore::Automatic() const {
	json list = json::array();
	for (auto& entry : entries)
		if (entry.automatic)
			list.push_back({{"name", entry.name}, {"before", entry.tool}});
	return list;
}
