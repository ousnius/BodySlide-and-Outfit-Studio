/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

// BodySlide: the morph files written next to built outfits (TRI, Starfield morphs, locked normals)

#include "BodySlideApp.h"
#include "files/TriFile.h"
#include "files/SFMorphFile.h"

#include <algorithm>
#include <unordered_map>
#include <utility>
#include <wx/filename.h>
#include <wx/wx.h>

using namespace nifly;

struct ContinuousRange {
	uint16_t index = 0;
	size_t length = 0;
};

static std::vector<ContinuousRange> FindContinuousRanges(const std::vector<uint16_t>& source) {
	std::vector<ContinuousRange> ranges;
	if (source.size() == 0) {
		return ranges;
	}

	size_t startIndex = 0;
	size_t endIndex = 1;
	int lastValue = (int)source[0]; // Cast to int to avoid potential uint16_t overflow in comparison below

	for (; endIndex < source.size(); ++endIndex) {
		int value = (int)source[endIndex];
		if (value != lastValue + 1) {
			ranges.emplace_back(ContinuousRange{ source[startIndex], endIndex - startIndex });
			startIndex = endIndex;
		}
		lastValue = value;
	}

	ranges.emplace_back(ContinuousRange{ source[startIndex], endIndex - startIndex });

	return ranges;
}

bool BodySlideApp::WriteMorphTRI(const std::string& triPath, SliderSet& sliderSet, NifFile& nif, std::unordered_map<std::string, std::vector<uint16_t>>& zapIndices) {
	DiffDataSets currentDiffs;
	sliderSet.LoadSetDiffData(currentDiffs);

	TriFile tri;
	std::string triFilePath = triPath + ".tri";

	for (auto targetShape = sliderSet.ShapesBegin(); targetShape != sliderSet.ShapesEnd(); ++targetShape) {
		auto shape = nif.FindBlockByName<NiShape>(targetShape->first);
		if (!shape)
			continue;

		const std::vector<uint16_t>& shapeZapIndices = zapIndices[targetShape->first];

		int shapeVertCount = shape->GetNumVertices();
		shapeVertCount += shapeZapIndices.size();

		if (shapeVertCount <= 0)
			continue;

		if (shapeZapIndices.size() > 0 && shapeZapIndices.back() >= shapeVertCount)
			continue;

		auto zapRanges = FindContinuousRanges(shapeZapIndices);

		for (size_t s = 0; s < sliderSet.size(); s++) {
			std::string dn = sliderSet[s].TargetDataName(targetShape->second.targetShape);
			std::string target = targetShape->second.targetShape;
			if (dn.empty())
				continue;

			if (!sliderSet[s].bClamp && !sliderSet[s].bZap) {
				MorphDataPtr morph = std::make_shared<MorphData>();
				morph->name = sliderSet[s].name;

				if (sliderSet[s].bUV) {
					morph->type = MORPHTYPE_UV;

					std::vector<Vector2> uvs;
					uvs.resize(shapeVertCount);

					currentDiffs.ApplyUVDiff(dn, target, 1.0f, &uvs);

					for (auto range = zapRanges.rbegin(); range != zapRanges.rend(); ++range) {
						const auto start = uvs.cbegin() + range->index;
						uvs.erase(start, start + range->length);
					}

					int i = 0;
					for (auto& uv : uvs) {
						Vector3 v(uv.u, uv.v, 0.0f);
						if (!v.IsZero(true))
							morph->offsets.emplace(i, v);
						i++;
					}
				}
				else {
					morph->type = MORPHTYPE_POSITION;

					std::vector<Vector3> verts;
					verts.resize(shapeVertCount);

					currentDiffs.ApplyDiff(dn, target, 1.0f, &verts);

					for (auto range = zapRanges.rbegin(); range != zapRanges.rend(); ++range) {
						const auto start = verts.cbegin() + range->index;
						verts.erase(start, start + range->length);
					}

					int i = 0;
					for (auto& v : verts) {
						if (!v.IsZero(true))
							morph->offsets.emplace(i, v);
						i++;
					}
				}

				if (morph->offsets.size() > 0)
					tri.AddMorph(targetShape->first, morph);
			}
		}
	}

	if (!tri.Write(triFilePath))
		return false;

	return true;
}

bool BodySlideApp::WriteSFMorphFile(const std::string& morphFolder, SliderSet& sliderSet, NifFile& nif, std::unordered_map<std::string, std::vector<uint16_t>>& zapIndices) {
	std::string targetShapeName = sliderSet.GetSFMorphTargetShape();
	if (targetShapeName.empty()) {
		wxLogMessage("No morph target shape designated, skipping morph.dat.");
		return false;
	}

	wxLogMessage("Writing Starfield morph.dat for shape '%s' to '%s'...", targetShapeName, morphFolder);

	DiffDataSets currentDiffs;
	sliderSet.LoadSetDiffData(currentDiffs);

	// Find the designated shape in the slider set
	std::string targetDataShape;
	for (auto it = sliderSet.ShapesBegin(); it != sliderSet.ShapesEnd(); ++it) {
		if (it->first == targetShapeName) {
			targetDataShape = it->second.targetShape;
			break;
		}
	}

	if (targetDataShape.empty()) {
		wxLogMessage("Morph target shape '%s' not found in slider set, skipping morph.dat.", targetShapeName);
		return false;
	}

	auto shape = nif.FindBlockByName<NiShape>(targetShapeName);
	if (!shape) {
		wxLogMessage("Shape '%s' not found in NIF, skipping morph.dat.", targetShapeName);
		return false;
	}

	const std::vector<uint16_t>& shapeZapIndices = zapIndices[targetShapeName];

	int shapeVertCount = shape->GetNumVertices();
	shapeVertCount += shapeZapIndices.size();

	if (shapeVertCount <= 0)
		return false;

	if (shapeZapIndices.size() > 0 && shapeZapIndices.back() >= shapeVertCount)
		return false;

	auto zapRanges = FindContinuousRanges(shapeZapIndices);

	SFMorphNormalSettings normalSettings;
	normalSettings.smoothSeamNormals = sliderSet.GetSmoothSeamNormals(targetShapeName);
	normalSettings.smoothSeamNormalsAngle = sliderSet.GetSmoothSeamNormalsAngle(targetShapeName);
	normalSettings.lockNormals = sliderSet.GetLockNormals(targetShapeName);
	const SFMorphShape morphShape = SFMorphShape::FromShape(nif, shape, normalSettings);

	SFMorphFile morphFile;
	morphFile.SetVertexCount(shapeVertCount - static_cast<int>(shapeZapIndices.size()));

	for (size_t s = 0; s < sliderSet.size(); s++) {
		std::string dn = sliderSet[s].TargetDataName(targetDataShape);
		if (dn.empty())
			continue;

		if (sliderSet[s].bClamp || sliderSet[s].bZap || sliderSet[s].bUV)
			continue;

		std::vector<Vector3> diffs;
		diffs.resize(shapeVertCount);

		currentDiffs.ApplyDiff(dn, targetDataShape, 1.0f, &diffs);

		for (auto range = zapRanges.rbegin(); range != zapRanges.rend(); ++range) {
			const auto start = diffs.cbegin() + range->index;
			diffs.erase(start, start + range->length);
		}

		std::unordered_map<uint16_t, Vector3> morphOffsets;
		int i = 0;
		for (auto& d : diffs) {
			if (!d.IsZero(true))
				morphOffsets.emplace(i, d);
			i++;
		}

		if (morphOffsets.empty())
			continue;

		if (morphFile.morphOffsetsCache.size() >= SFMaxShapeKeys) {
			wxLogWarning("Starfield morph.dat supports at most 128 morphs; '%s' and any remaining morphs were skipped.", sliderSet[s].name);
			break;
		}

		morphShape.AddMorph(morphFile, sliderSet[s].name, morphOffsets);
	}

	if (morphFile.morphOffsetsCache.empty()) {
		wxLogMessage("No morphs found for shape '%s', skipping morph.dat.", targetShapeName);
		return false;
	}

	wxLogMessage("Writing %zu morph(s) for shape '%s'...", morphFile.morphOffsetsCache.size(), targetShapeName);

	wxFileName::Mkdir(wxString::FromUTF8(morphFolder), wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);

	std::string shapeFilePath = morphFolder + PathSepStr + "morph.dat";

	morphFile.CacheToFileData();

	if (!morphFile.Write(shapeFilePath)) {
		wxLogError("Failed to write morph.dat file to '%s'!", shapeFilePath);
		wxMessageBox(wxString().Format(_("Failed to write morph.dat file to the following location\n\n%s"), shapeFilePath),
					 _("Unable to process"), wxOK | wxICON_ERROR);
		return false;
	}

	wxLogMessage("Successfully wrote morph.dat to '%s'.", shapeFilePath);
	return true;
}

void BodySlideApp::SetTriData(NifFile& nif, const std::string& triPath, bool toRoot) {
	auto& hdr = nif.GetHeader();

	// Get rid of every BODYTRI block in the file, no matter where it's attached
	std::vector<uint32_t> obsoleteIds;

	for (uint32_t id = 0; id < hdr.GetNumBlocks(); id++) {
		auto stringExtraData = hdr.GetBlock<NiStringExtraData>(id);
		if (stringExtraData && stringExtraData->name.get() == "BODYTRI")
			obsoleteIds.push_back(id);
	}

	// Delete the highest block ids first to keep the remaining ones valid
	for (auto it = obsoleteIds.rbegin(); it != obsoleteIds.rend(); ++it)
		hdr.DeleteBlock(*it);

	// Fallout reads the path from the root node, the other games from a shape
	NiAVObject* target = nullptr;

	if (toRoot) {
		target = nif.GetRootNode();
	}
	else {
		for (auto& shape : nif.GetShapes()) {
			if (shape->GetNumVertices() > 0) {
				target = shape;
				break;
			}
		}
	}

	if (!target)
		return;

	auto triExtraData = std::make_unique<NiStringExtraData>();
	triExtraData->name.get() = "BODYTRI";
	triExtraData->stringData.get() = triPath;
	nif.AssignExtraData(target, std::move(triExtraData));
}

void BodySlideApp::SetLockedNormalsData(NifFile& nif, SliderSet& sliderSet) {
	auto& hdr = nif.GetHeader();

	for (auto it = sliderSet.ShapesBegin(); it != sliderSet.ShapesEnd(); ++it) {
		if (!it->second.lockNormals)
			continue;

		auto shape = nif.FindBlockByName<NiShape>(it->first);
		if (!shape)
			continue;

		uint32_t numVerts = shape->GetNumVertices();
		if (numVerts == 0)
			continue;

		// Reuse an existing LOCKEDNORM block of the shape
		NiIntegersExtraData* lockedNormalsData = nullptr;
		for (auto& extraDataRef : shape->extraDataRefs) {
			auto integersExtraData = hdr.GetBlock<NiIntegersExtraData>(extraDataRef);
			if (integersExtraData && integersExtraData->name == "LOCKEDNORM") {
				lockedNormalsData = integersExtraData;
				break;
			}
		}

		if (!lockedNormalsData) {
			auto newExtraData = std::make_unique<NiIntegersExtraData>();
			newExtraData->name.get() = "LOCKEDNORM";
			lockedNormalsData = newExtraData.get();
			nif.AssignExtraData(shape, std::move(newExtraData));
		}

		lockedNormalsData->integersData.resize(numVerts);
		for (uint32_t i = 0; i < numVerts; i++)
			lockedNormalsData->integersData[i] = i;
	}
}
