/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

// OutfitProject: sliders, their properties and their diffs

#include "OutfitProject.h"
#include "NifUtil.hpp"

#include <algorithm>
#include <atomic>
#include <cfloat>
#include <filesystem>
#include <limits>
#include <regex>
#include <sstream>
#include <wx/log.h>

using namespace nifly;

bool OutfitProject::ValidSlider(const size_t index) {
	if (index < activeSet.size())
		return true;

	return false;
}

bool OutfitProject::ValidSlider(const std::string& sliderName) {
	return activeSet.SliderExists(sliderName);
}

bool OutfitProject::AllSlidersZero() {
	for (size_t i = 0; i < activeSet.size(); i++)
		if (activeSet[i].curValue != 0.0f)
			return false;
	return true;
}

size_t OutfitProject::SliderCount() {
	return activeSet.size();
}

void OutfitProject::GetSliderList(std::vector<std::string>& sliderNames) {
	for (size_t i = 0; i < activeSet.size(); i++)
		sliderNames.push_back(activeSet[i].name);
}

std::string OutfitProject::GetSliderName(const size_t index) {
	if (!ValidSlider(index))
		return std::string();

	return activeSet[index].name;
}

void OutfitProject::NewBaseDiffSet(const std::string& dataName, const std::string& target) {
	// The reference's data sets outlive the projects that made them, so one of the same name from an earlier
	// project can still be there. A new slider starts without it, or its data would be summed onto the old.
	baseDiffData.AddEmptySet(dataName, target);
	baseDiffData.EmptySet(dataName, target);
}

void OutfitProject::AddEmptySlider(const std::string& newName) {
	size_t sliderID = activeSet.CreateSlider(newName);

	if (baseShape) {
		std::string baseShapeName = baseShape->name.get();
		std::string target = SliderDataTargetForShape(baseShape);
		std::string shapeSlider = target + newName;
		activeSet[sliderID].AddDataFile(target, shapeSlider, shapeSlider);
		activeSet.AddShapeTarget(baseShapeName, target);
		NewBaseDiffSet(shapeSlider, target);
	}
}

void OutfitProject::AddZapSlider(const std::string& newName, std::unordered_map<uint16_t, float>& verts, NiShape* shape) {
	if (!shape)
		return;

	std::unordered_map<uint16_t, Vector3> diffData;
	Vector3 moveVec(0.0f, 1.0f, 0.0f);
	for (auto& v : verts)
		diffData[v.first] = moveVec;

	std::string target = SliderDataTargetForShape(shape);
	std::string shapeSlider = target + newName;

	size_t sliderID = 0;
	if (!SliderIndexFromName(newName, sliderID))
		sliderID = activeSet.CreateSlider(newName);

	activeSet[sliderID].bZap = true;
	activeSet[sliderID].defBigValue = 0.0f;
	activeSet[sliderID].defSmallValue = 0.0f;

	if (IsBaseShape(shape)) {
		activeSet[sliderID].AddDataFile(target, shapeSlider, shapeSlider);
		activeSet.AddShapeTarget(shape->name.get(), target);
		NewBaseDiffSet(shapeSlider, target);
		for (auto& i : diffData)
			baseDiffData.SumDiff(shapeSlider, target, i.first, i.second);
	}
	else
		morpher.SetResultDiff(target, newName, diffData);
}

std::vector<bool> OutfitProject::GetZappedVertices(NiShape* shape, const std::string& sliderName) {
	std::vector<bool> zapped;
	if (!shape)
		return zapped;

	const size_t count = shape->GetNumVertices();
	std::vector<Vector3> diffs(count);
	GetSliderDiff(shape, sliderName, diffs);

	bool any = false;
	for (auto& d : diffs)
		any = any || !d.IsZero(true);

	if (!any && IsBaseShape(shape)) {
		diffs.assign(count, Vector3());
		morpher.ApplyResultToVerts(sliderName, ShapeToTarget(shape->name.get()), &diffs);
	}

	zapped.resize(count);
	for (size_t v = 0; v < count; v++)
		zapped[v] = !diffs[v].IsZero(true);
	return zapped;
}

void OutfitProject::SetZappedVertices(NiShape* shape, const std::string& sliderName, const std::vector<bool>& zapped) {
	if (!shape || !ValidSlider(sliderName))
		return;

	std::string target = SliderDataTargetForShape(shape);
	std::string dataName = EnsureSliderDataLocal(sliderName, shape);
	if (dataName.empty())
		return;

	// Zap data only marks vertices, the direction of the delta doesn't matter
	std::unordered_map<uint16_t, Vector3> diffData;
	for (size_t v = 0; v < zapped.size(); v++)
		if (zapped[v])
			diffData[static_cast<uint16_t>(v)] = Vector3(0.0f, 1.0f, 0.0f);

	if (IsBaseShape(shape)) {
		activeSet.AddShapeTarget(shape->name.get(), target);
		NewBaseDiffSet(dataName, target);
		for (auto& d : diffData)
			baseDiffData.SumDiff(dataName, target, d.first, d.second);
	}
	else
		morpher.SetResultDiff(target, sliderName, diffData);
}

void OutfitProject::AddCombinedSlider(const std::string& newName) {
	std::vector<Vector3> verts;
	std::unordered_map<uint16_t, Vector3> diffData;

	for (auto& s : workNif.GetShapes()) {
		if (IsBaseShape(s))
			continue;

		diffData.clear();
		GetLiveVerts(s, verts);
		workNif.CalcShapeDiff(s, &verts, diffData);
		std::string target = SliderDataTargetForShape(s);
		morpher.SetResultDiff(target, newName, diffData);
	}

	size_t sliderID = activeSet.CreateSlider(newName);
	if (baseShape) {
		std::string baseShapeName = baseShape->name.get();
		std::string target = SliderDataTargetForShape(baseShape);
		std::string shapeSlider = target + newName;
		activeSet[sliderID].AddDataFile(target, shapeSlider, shapeSlider);
		NewBaseDiffSet(shapeSlider, target);

		GetLiveVerts(baseShape, verts);
		workNif.CalcShapeDiff(baseShape, &verts, diffData);

		for (auto& i : diffData)
			baseDiffData.SumDiff(shapeSlider, target, i.first, i.second);
	}
}

NiShape* OutfitProject::CreateNifShapeFromData(
	const std::string& shapeName, const std::vector<Vector3>* v, const std::vector<Triangle>* t, const std::vector<Vector2>* uv, const std::vector<Vector3>* norms) {
	auto targetGame = (TargetGame)Config.GetIntValue("TargetGame");

	if (!workNif.IsValid()) {
		NiVersion version;

		switch (targetGame) {
			case OB: version = NiVersion::getOB(); break;
			case FO3:
			case FONV: version = NiVersion::getFO3(); break;
			case SKYRIM: version = NiVersion::getSK(); break;
			case FO4:
			case FO4VR: version = NiVersion::getFO4(); break;
			case SKYRIMSE:
			case SKYRIMVR: version = NiVersion::getSSE(); break;
			case FO76: version = NiVersion::getFO76(); break;
			case SF: version = NiVersion::getSF(); break;
		}

		workNif.Create(version);
	}

	auto shapeResult = workNif.CreateShapeFromData(shapeName, v, t, uv, norms);
	if (shapeResult)
		SetTextures(shapeResult);

	return shapeResult;
}

bool OutfitProject::SliderClamp(const size_t index) {
	if (!ValidSlider(index))
		return false;

	return activeSet[index].bClamp;
}

bool OutfitProject::SliderZap(const size_t index) {
	if (!ValidSlider(index))
		return false;

	return activeSet[index].bZap;
}

bool OutfitProject::SliderUV(const size_t index) {
	if (!ValidSlider(index))
		return false;

	return activeSet[index].bUV;
}

bool OutfitProject::SliderNoAutoConform(const size_t index) {
	if (!ValidSlider(index))
		return false;

	return activeSet[index].bNoAutoConform;
}

wxArrayString OutfitProject::SliderZapToggles(const size_t index) {
	wxArrayString toggles;
	if (ValidSlider(index))
		for (auto& toggle : activeSet[index].zapToggles)
			toggles.Add(wxString::FromUTF8(toggle));

	return toggles;
}

bool OutfitProject::SliderToggle(const size_t index) {
	if (!ValidSlider(index))
		return false;

	return activeSet[index].bToggle && !activeSet[index].bZap;
}

float OutfitProject::SliderToggleValue(const size_t index) {
	if (!ValidSlider(index))
		return 100.0f;

	return activeSet[index].toggleValue;
}

std::vector<ToggleTarget> OutfitProject::SliderToggleSliders(const size_t index) {
	if (!ValidSlider(index))
		return {};

	return activeSet[index].toggleSliders;
}

bool OutfitProject::SliderInvert(const size_t index) {
	if (!ValidSlider(index))
		return false;

	return activeSet[index].bInvert;
}

bool OutfitProject::SliderHidden(const size_t index) {
	if (!ValidSlider(index))
		return false;

	return activeSet[index].bHidden;
}

namespace {
// Changing a property of an inherited slider makes its properties the project's own
template<typename T>
void SetSliderProperty(SliderData& slider, T& property, const T& value) {
	if (property == value)
		return;

	property = value;
	slider.bInherited = false;
}
} // namespace

void OutfitProject::SetSliderZap(const size_t index, const bool zap) {
	if (!ValidSlider(index))
		return;

	SetSliderProperty(activeSet[index], activeSet[index].bZap, zap);
}

void OutfitProject::SetSliderZapToggles(const size_t index, const wxArrayString& toggles) {
	if (!ValidSlider(index))
		return;

	std::vector<std::string> zapToggles;
	for (auto& s : toggles)
		zapToggles.push_back(s.ToUTF8().data());

	SetSliderProperty(activeSet[index], activeSet[index].zapToggles, zapToggles);
}

void OutfitProject::SetSliderToggle(const size_t index, const bool toggle) {
	if (!ValidSlider(index))
		return;

	SetSliderProperty(activeSet[index], activeSet[index].bToggle, toggle);
}

void OutfitProject::SetSliderToggleValue(const size_t index, const float value) {
	if (!ValidSlider(index))
		return;

	SetSliderProperty(activeSet[index], activeSet[index].toggleValue, value);
}

void OutfitProject::SetSliderToggleSliders(const size_t index, const std::vector<ToggleTarget>& targets) {
	if (!ValidSlider(index))
		return;

	// A toggle can't set itself, other toggles or clamps
	std::vector<ToggleTarget> toggleSliders;
	for (auto& target : targets) {
		size_t targetIndex = 0;
		if (target.slider == activeSet[index].name || !SliderIndexFromName(target.slider, targetIndex) || SliderToggle(targetIndex) || SliderClamp(targetIndex))
			continue;

		auto existing = std::find_if(toggleSliders.begin(), toggleSliders.end(), [&target](const ToggleTarget& t) { return t.slider == target.slider; });
		if (existing != toggleSliders.end())
			existing->value = target.value;
		else
			toggleSliders.push_back(target);
	}

	SetSliderProperty(activeSet[index], activeSet[index].toggleSliders, toggleSliders);
}

void OutfitProject::SetSliderInvert(const size_t index, const bool inv) {
	if (!ValidSlider(index))
		return;

	SetSliderProperty(activeSet[index], activeSet[index].bInvert, inv);
}

void OutfitProject::SetSliderUV(const size_t index, const bool uv) {
	if (!ValidSlider(index))
		return;

	SetSliderProperty(activeSet[index], activeSet[index].bUV, uv);
}

void OutfitProject::SetSliderNoAutoConform(const size_t index, const bool noAutoConform) {
	if (!ValidSlider(index))
		return;

	SetSliderProperty(activeSet[index], activeSet[index].bNoAutoConform, noAutoConform);
}

void OutfitProject::SetSliderHidden(const size_t index, const bool hidden) {
	if (!ValidSlider(index))
		return;

	SetSliderProperty(activeSet[index], activeSet[index].bHidden, hidden);
}

void OutfitProject::SetSliderDefault(const size_t index, const int val, const bool isHi) {
	if (!ValidSlider(index))
		return;

	if (!isHi)
		SetSliderProperty(activeSet[index], activeSet[index].defSmallValue, static_cast<float>(val));
	else
		SetSliderProperty(activeSet[index], activeSet[index].defBigValue, static_cast<float>(val));
}

void OutfitProject::SetSliderName(const size_t index, const std::string& newName) {
	if (!ValidSlider(index))
		return;

	std::string oldName = activeSet[index].name;
	bool fromBase = activeSet.HasBaseProject() && activeSet.BaseHasSlider(oldName);
	for (auto& s : workNif.GetShapes()) {
		std::string shapeName = s->name.get();
		std::string oldDT = shapeName + oldName;
		std::string newDT = shapeName + newName;

		if (IsBaseShape(s))
			baseDiffData.RenameSet(oldDT, newDT);
		else
			morpher.RenameResultDiffData(shapeName, oldName, newName);

		activeSet[index].RenameData(oldDT, newDT);
		activeSet[index].SetLocalData(newDT);
	}

	// Under another name, the slider of the base project is hidden and its data becomes the project's own
	if (fromBase) {
		activeSet.AddRemovedSlider(oldName);
		activeSet[index].bInherited = false;
		for (auto& dataFile : activeSet[index].dataFiles)
			DetachSliderData(dataFile);
	}

	activeSet.RemoveRemovedSlider(newName);
	activeSet[index].name = newName;

	// Zaps and toggles keep toggling the slider under its new name
	for (size_t i = 0; i < activeSet.size(); i++) {
		SliderData& other = activeSet[i];
		bool refersToSlider = std::find(other.zapToggles.begin(), other.zapToggles.end(), oldName) != other.zapToggles.end()
							  || std::any_of(other.toggleSliders.begin(), other.toggleSliders.end(), [&oldName](const ToggleTarget& t) { return t.slider == oldName; });
		if (!refersToSlider)
			continue;

		other.RenameToggledSlider(oldName, newName);
		other.bInherited = false;
	}
}

float& OutfitProject::SliderValue(const size_t index) {
	return activeSet[index].curValue;
}

float& OutfitProject::SliderValue(const std::string& name) {
	return activeSet[name].curValue;
}

float OutfitProject::SliderDefault(const size_t index, const bool hi) {
	if (hi)
		return activeSet[index].defBigValue;

	return activeSet[index].defSmallValue;
}

bool& OutfitProject::SliderShow(const size_t index) {
	return activeSet[index].bShow;
}

bool& OutfitProject::SliderShow(const std::string& sliderName) {
	return activeSet[sliderName].bShow;
}

bool OutfitProject::SliderIndexFromName(const std::string& sliderName, size_t& index) {
	for (size_t i = 0; i < activeSet.size(); i++) {
		if (activeSet[i].name == sliderName) {
			index = i;
			return true;
		}
	}

	return false;
}

void OutfitProject::CloneSlider(const std::string& sliderName, const std::string& cloneName) {
	size_t cloneIndex = activeSet.CloneSlider(sliderName, cloneName);
	if (cloneIndex == static_cast<size_t>(-1))
		return;

	for (auto& shape : workNif.GetShapes()) {
		std::string shapeName = shape->name.get();
		std::string target = ShapeToTarget(shape->name.get());
		std::string oldDT = target + sliderName;
		std::string newDT = target + cloneName;

		// Adjust all data names to cloned slider name and make it local
		activeSet[cloneIndex].RenameData(oldDT, newDT);
		activeSet[cloneIndex].SetLocalData(newDT);
		if (DiffInfo* dataFile = activeSet[cloneIndex].GetDataFile(newDT))
			UnlinkSliderData(*dataFile);

		std::string data = activeSet[sliderName].TargetDataName(target);

		if (IsBaseShape(shape)) {
			activeSet[cloneIndex].AddDataFile(target, newDT, newDT);
			activeSet.AddShapeTarget(shape->name.get(), target);
			NewBaseDiffSet(newDT, target);

			auto diffData = baseDiffData.GetDiffSet(oldDT);
			if (diffData) {
				for (auto& i : *diffData)
					baseDiffData.SumDiff(newDT, target, i.first, i.second);
			}
		}
		else {
			auto diffData = morpher.GetDiffSet(oldDT);
			if (diffData)
				morpher.SetResultDiff(target, cloneName, *diffData);
		}
	}
}

void OutfitProject::NegateSlider(const std::string& sliderName, NiShape* shape) {
	std::string target = SliderDataTargetForShape(shape);
	std::string sliderData = EnsureSliderDataLocal(sliderName, shape);
	if (sliderData.empty())
		return;

	if (IsBaseShape(shape)) {
		baseDiffData.ScaleDiff(sliderData, target, -1.0f);
	}
	else
		morpher.ScaleResultDiff(target, sliderName, -1.0f);
}

void OutfitProject::MaskAffected(const std::string& sliderName, NiShape* shape) {
	Mesh* m = host->GetShapeMesh(shape->name.get());
	if (!m)
		return;

	m->MaskFill(0.0f);

	if (IsBaseShape(shape)) {
		std::vector<uint16_t> outIndices;
		std::string target = ShapeToTarget(shape->name.get());

		std::string sliderData = activeSet[sliderName].TargetDataName(target);
		baseDiffData.GetDiffIndices(sliderData, target, outIndices);

		for (auto& i : outIndices) {
			if (m->nVerts > i)
				m->mask[i] = 1.0f;
		}
	}
	else {
		std::unordered_map<uint16_t, Vector3> outDiff;
		std::string target = SliderDataTargetForShape(shape);
		morpher.GetRawResultDiff(target, sliderName, outDiff);

		for (auto& i : outDiff) {
			if (m->nVerts > i.first)
				m->mask[i.first] = 1.0f;
		}
	}

	m->QueueUpdate(Mesh::UpdateType::Mask);
}

void OutfitProject::GetSliderDiff(NiShape* shape, const std::string& sliderName, std::vector<Vector3>& outVerts) {
	size_t sliderIndex = 0;
	if (!SliderIndexFromName(sliderName, sliderIndex))
		return;

	std::string target = ShapeToTarget(shape->name.get());
	if (IsBaseShape(shape)) {
		std::string targetData = activeSet.ShapeToDataName(sliderIndex, shape->name.get());
		if (targetData.empty())
			return;

		if (!activeSet[sliderIndex].bUV)
			baseDiffData.ApplyDiff(targetData, target, 1.0f, &outVerts);
	}
	else {
		if (!activeSet[sliderIndex].bUV)
			morpher.ApplyResultToVerts(activeSet[sliderIndex].name, target, &outVerts);
	}
}

void OutfitProject::GetSliderDiffUV(NiShape* shape, const std::string& sliderName, std::vector<Vector2>& outUVs) {
	size_t sliderIndex = 0;
	if (!SliderIndexFromName(sliderName, sliderIndex))
		return;

	std::string target = ShapeToTarget(shape->name.get());
	if (IsBaseShape(shape)) {
		std::string targetData = activeSet.ShapeToDataName(sliderIndex, shape->name.get());
		if (targetData.empty())
			return;

		if (activeSet[sliderIndex].bUV)
			baseDiffData.ApplyUVDiff(targetData, target, 1.0f, &outUVs);
	}
	else {
		if (activeSet[sliderIndex].bUV)
			morpher.ApplyResultToUVs(activeSet[sliderIndex].name, target, &outUVs);
	}
}

void OutfitProject::UpdateMorphResult(NiShape* shape, const std::string& sliderName, const TargetDataDiffs& vertUpdates) {
	// Morph results are stored in two different places depending on whether it's an outfit or the base shape.
	// The outfit morphs are stored in the automorpher, whereas the base shape diff info is stored in directly in basediffdata.

	std::string target = SliderDataTargetForShape(shape);
	std::string dataName = vertUpdates.empty() ? activeSet[sliderName].TargetDataName(target) : EnsureSliderDataLocal(sliderName, shape);
	if (dataName.empty())
		return;

	if (IsBaseShape(shape)) {
		for (auto& i : vertUpdates) {
			Vector3 diffscale = Mesh::TransformDiffMeshToNif(i.second);
			baseDiffData.SumDiff(dataName, target, i.first, diffscale);
		}
	}
	else
		morpher.UpdateResultDiff(target, sliderName, vertUpdates);
}

void OutfitProject::ScaleMorphResult(NiShape* shape, const std::string& sliderName, float scaleValue) {
	std::string target = SliderDataTargetForShape(shape);
	if (target.empty())
		return;

	std::string sliderData = EnsureSliderDataLocal(sliderName, shape);
	if (sliderData.empty())
		return;

	if (IsBaseShape(shape)) {
		baseDiffData.ScaleDiff(sliderData, target, scaleValue);
	}
	else
		morpher.ScaleResultDiff(target, sliderName, scaleValue);
}

bool OutfitProject::SmoothSliderDiff(NiShape* shape, const std::string& sliderName, int iterations, float strength) {
	if (!shape || IsBaseShape(shape) || !ValidSlider(sliderName))
		return false;

	if (baseShape)
		InitConform();
	else
		morpher.SourceShapesFromNif(workNif, &workAnim);

	std::string shapeName = shape->name.get();
	if (Mesh* m = host->GetShapeMesh(shapeName))
		morpher.CopyMeshMask(m, shapeName);

	std::string target = SliderDataTargetForShape(shape);
	if (morpher.GetResultDiffSize(target, sliderName) == 0) {
		morpher.UnlinkRefDiffData();
		return false;
	}

	morpher.SmoothResultDiff(shapeName, sliderName, target, iterations, strength);
	morpher.UnlinkRefDiffData();
	EnsureSliderDataLocal(sliderName, shape);
	return true;
}

void OutfitProject::ClearWorkSliders() {
	morpher.ClearResultDiff();
}

void OutfitProject::ClearSlider(NiShape* shape, const std::string& sliderName) {
	std::string target = SliderDataTargetForShape(shape);
	std::string data = EnsureSliderDataLocal(sliderName, shape);
	if (data.empty())
		return;

	if (IsBaseShape(shape)) {
		baseDiffData.EmptySet(data, target);
	}
	else
		morpher.EmptyResultDiff(target, sliderName);
}

void OutfitProject::ClearUnmaskedDiff(NiShape* shape, const std::string& sliderName, std::unordered_map<uint16_t, float>* mask) {
	std::string target = SliderDataTargetForShape(shape);
	std::string data = EnsureSliderDataLocal(sliderName, shape);
	if (data.empty())
		return;

	if (IsBaseShape(shape)) {
		baseDiffData.ZeroVertDiff(data, target, nullptr, mask);
	}
	else
		morpher.ZeroVertDiff(target, sliderName, nullptr, mask);
}

void OutfitProject::DeleteSlider(const std::string& sliderName) {
	for (auto& s : workNif.GetShapes()) {
		std::string target = ShapeToTarget(s->name.get());
		std::string data = activeSet[sliderName].TargetDataName(target);

		if (IsBaseShape(s))
			baseDiffData.ClearSet(data);
		else
			morpher.ClearResultSet(data);
	}

	// The base project still has the slider, the project only leaves it out
	if (activeSet.HasBaseProject() && activeSet.BaseHasSlider(sliderName))
		activeSet.AddRemovedSlider(sliderName);

	activeSet.DeleteSlider(sliderName);
}
TargetDataDiffs* OutfitProject::GetDiffSet(SliderData& sliderData, NiShape* shape) {
	std::string target = ShapeToTarget(shape->name.get());
	std::string targetDataName = sliderData.TargetDataName(target);
	if (targetDataName.empty())
		targetDataName = target + sliderData.name;

	if (IsBaseShape(shape))
		return baseDiffData.GetDiffSet(targetDataName);
	else
		return morpher.GetDiffSet(targetDataName);
}
