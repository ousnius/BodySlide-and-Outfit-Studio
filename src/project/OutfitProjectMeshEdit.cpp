/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

// OutfitProject: preparing mesh edits and their undo states

#include "OutfitProject.h"
#include "utils/StringStuff.h"
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

namespace {

int GetFirstSegmentPartID(const NifSegmentationInfo& inf) {
	for (const NifSegmentInfo& seg : inf.segs) {
		if (seg.partID >= 0)
			return seg.partID;
		for (const NifSubSegmentInfo& sub : seg.subs)
			if (sub.partID >= 0)
				return sub.partID;
	}

	return 0;
}

int FindSegmentIndexByPartID(const NifSegmentationInfo& inf, int partID) {
	for (size_t i = 0; i < inf.segs.size(); ++i)
		if (inf.segs[i].partID == partID)
			return static_cast<int>(i);

	return -1;
}

void RemapCopyGeoTriangles(UndoStateShape& uss, const std::vector<int>& mappedSourceParts) {
	const size_t triCount = std::min(uss.delTris.size(), mappedSourceParts.size());
	for (size_t ti = 0; ti < triCount; ++ti)
		uss.delTris[ti].partID = mappedSourceParts[ti];
}

void ReconcileCopyGeoSegments(NifFile& workNif, NiShape* source, NiShape* target, UndoStateShape& uss) {
	std::vector<int> sourceTriParts;
	NifSegmentationInfo sourceInf;
	bool sourceHasSegs = workNif.GetShapeSegments(source, sourceInf, sourceTriParts);

	std::vector<int> targetTriParts;
	NifSegmentationInfo targetInf;
	bool targetHasSegs = workNif.GetShapeSegments(target, targetInf, targetTriParts);
	if (!sourceHasSegs && !targetHasSegs)
		return;

	NifSegmentationInfo mergedInf = targetHasSegs ? targetInf : NifSegmentationInfo();
	if (mergedInf.ssfFile.empty() && sourceHasSegs)
		mergedInf.ssfFile = sourceInf.ssfFile;

	std::unordered_set<int> usedPartIDs;
	int maxPartID = -1;
	for (const NifSegmentInfo& seg : mergedInf.segs) {
		usedPartIDs.insert(seg.partID);
		maxPartID = std::max(maxPartID, seg.partID);
		for (const NifSubSegmentInfo& sub : seg.subs) {
			usedPartIDs.insert(sub.partID);
			maxPartID = std::max(maxPartID, sub.partID);
		}
	}

	auto allocPartID = [&]() {
		int id = std::max(0, maxPartID + 1);
		while (usedPartIDs.find(id) != usedPartIDs.end())
			++id;
		usedPartIDs.insert(id);
		maxPartID = std::max(maxPartID, id);
		return id;
	};

	std::unordered_map<int, int> sourceToMergedPartIDs;
	if (sourceHasSegs) {
		for (const NifSegmentInfo& sourceSeg : sourceInf.segs) {
			int mergedSegID = sourceSeg.partID;
			int mergedSegIndex = FindSegmentIndexByPartID(mergedInf, mergedSegID);
			if (mergedSegIndex < 0) {
				if (usedPartIDs.find(mergedSegID) != usedPartIDs.end())
					mergedSegID = allocPartID();
				else {
					usedPartIDs.insert(mergedSegID);
					maxPartID = std::max(maxPartID, mergedSegID);
				}

				NifSegmentInfo newSeg = sourceSeg;
				newSeg.partID = mergedSegID;
				newSeg.subs.clear();
				mergedInf.segs.push_back(newSeg);
				mergedSegIndex = static_cast<int>(mergedInf.segs.size()) - 1;
			}

			sourceToMergedPartIDs[sourceSeg.partID] = mergedSegID;

			NifSegmentInfo& mergedSeg = mergedInf.segs[mergedSegIndex];
			for (const NifSubSegmentInfo& sourceSub : sourceSeg.subs) {
				int mergedSubID = sourceSub.partID;
				bool foundSubInSegment = false;
				for (const NifSubSegmentInfo& mergedSub : mergedSeg.subs) {
					if (mergedSub.partID == mergedSubID) {
						foundSubInSegment = true;
						break;
					}
				}

				if (!foundSubInSegment) {
					if (usedPartIDs.find(mergedSubID) != usedPartIDs.end())
						mergedSubID = allocPartID();
					else {
						usedPartIDs.insert(mergedSubID);
						maxPartID = std::max(maxPartID, mergedSubID);
					}

					NifSubSegmentInfo newSub = sourceSub;
					newSub.partID = mergedSubID;
					mergedSeg.subs.push_back(newSub);
				}

				sourceToMergedPartIDs[sourceSub.partID] = mergedSubID;
			}
		}
	}

	std::vector<int> mappedSourceParts(source->GetNumTriangles(), GetFirstSegmentPartID(mergedInf));
	if (sourceHasSegs) {
		for (size_t ti = 0; ti < sourceTriParts.size() && ti < mappedSourceParts.size(); ++ti) {
			auto idIt = sourceToMergedPartIDs.find(sourceTriParts[ti]);
			if (idIt != sourceToMergedPartIDs.end())
				mappedSourceParts[ti] = idIt->second;
		}
	}

	RemapCopyGeoTriangles(uss, mappedSourceParts);

	uss.hasSegmentInfo = true;
	uss.segmentInfoBefore = targetHasSegs ? targetInf : NifSegmentationInfo();
	uss.segmentInfoAfter = mergedInf;
	uss.hasPartitionInfo = false;
}

void ReconcileCopyGeoPartitions(NifFile& workNif, NiShape* source, NiShape* target, UndoStateShape& uss) {
	std::vector<int> sourceTriParts;
	NiVector<BSDismemberSkinInstance::PartitionInfo> sourceInfo;
	bool sourceHasParts = workNif.GetShapePartitions(source, sourceInfo, sourceTriParts);

	std::vector<int> targetTriParts;
	NiVector<BSDismemberSkinInstance::PartitionInfo> targetInfo;
	bool targetHasParts = workNif.GetShapePartitions(target, targetInfo, targetTriParts);
	if (!sourceHasParts && !targetHasParts)
		return;

	NiVector<BSDismemberSkinInstance::PartitionInfo> mergedInfo = targetHasParts ? targetInfo : NiVector<BSDismemberSkinInstance::PartitionInfo>();
	std::unordered_map<uint16_t, int> slotToTargetIndex;
	for (int i = 0; i < static_cast<int>(mergedInfo.size()); ++i)
		slotToTargetIndex[mergedInfo[i].partID] = i;

	std::vector<int> sourceIndexToMergedIndex(sourceInfo.size(), 0);
	if (sourceHasParts) {
		for (size_t si = 0; si < sourceInfo.size(); ++si) {
			const uint16_t partID = sourceInfo[si].partID;
			auto targetIt = slotToTargetIndex.find(partID);
			if (targetIt != slotToTargetIndex.end()) {
				sourceIndexToMergedIndex[si] = targetIt->second;
				continue;
			}

			const int newIndex = static_cast<int>(mergedInfo.size());
			mergedInfo.push_back(sourceInfo[si]);
			sourceIndexToMergedIndex[si] = newIndex;
			slotToTargetIndex[partID] = newIndex;
		}
	}

	std::vector<int> mappedSourceParts(source->GetNumTriangles(), mergedInfo.empty() ? -1 : 0);
	if (sourceHasParts) {
		for (size_t ti = 0; ti < sourceTriParts.size() && ti < mappedSourceParts.size(); ++ti) {
			int sourcePartIndex = sourceTriParts[ti];
			if (sourcePartIndex >= 0 && sourcePartIndex < static_cast<int>(sourceIndexToMergedIndex.size()))
				mappedSourceParts[ti] = sourceIndexToMergedIndex[sourcePartIndex];
		}
	}

	RemapCopyGeoTriangles(uss, mappedSourceParts);

	uss.hasPartitionInfo = true;
	uss.partitionInfoBefore = targetHasParts ? targetInfo : NiVector<BSDismemberSkinInstance::PartitionInfo>();
	uss.partitionInfoAfter = mergedInfo;
	uss.hasSegmentInfo = false;
}

} // namespace

void OutfitProject::CollectVertexData(NiShape* shape, UndoStateShape& uss, const std::vector<uint16_t>& indices) {
	uss.delVerts.resize(indices.size());

	const std::vector<Vector3>* verts = workNif.GetVertsForShape(shape);
	const std::vector<Vector2>* uvs = workNif.GetUvsForShape(shape);
	const std::vector<Color4>* colors = workNif.GetColorsForShape(shape->name.get());
	const std::vector<Vector3>* normals = workNif.GetNormalsForShape(shape);
	const std::vector<Vector3>* tangents = workNif.GetTangentsForShape(shape);
	const std::vector<Vector3>* bitangents = workNif.GetBitangentsForShape(shape);
	const std::vector<float>* eyeData = workNif.GetEyeDataForShape(shape);
	AnimSkin& skin = workAnim.shapeSkinning[shape->name.get()];
	Mesh* m = host->GetShapeMesh(shape->name.get());
	const float* mask = m ? m->mask.get() : nullptr;

	uss.hadVertexColors = colors && !colors->empty();

	for (uint16_t di = 0; di < static_cast<uint16_t>(indices.size()); ++di) {
		UndoStateVertex& usv = uss.delVerts[di];
		uint16_t vi = indices[di];
		usv.index = vi;

		if (verts && verts->size() > vi)
			usv.pos = (*verts)[vi];
		if (uvs && uvs->size() > vi)
			usv.uv = (*uvs)[vi];
		if (colors && colors->size() > vi)
			usv.color = (*colors)[vi];
		if (normals && normals->size() > vi)
			usv.normal = (*normals)[vi];
		if (tangents && tangents->size() > vi)
			usv.tangent = (*tangents)[vi];
		if (bitangents && bitangents->size() > vi)
			usv.bitangent = (*bitangents)[vi];
		if (eyeData && eyeData->size() > vi)
			usv.eyeData = (*eyeData)[vi];
		if (mask)
			usv.mask = mask[vi];

		for (auto bnp : skin.boneNames) {
			AnimWeight& aw = skin.boneWeights[bnp.second];
			auto wit = aw.weights.find(vi);
			if (wit != aw.weights.end())
				usv.weights.emplace_back(UndoStateVertexBoneWeight{bnp.first, wit->second});
		}
	}

	// For diffs, it's more efficient to reverse the loop nesting.
	for (size_t si = 0; si < activeSet.size(); ++si) {
		std::unordered_map<uint16_t, Vector3>* diffSet = GetDiffSet(activeSet[si], shape);

		if (!diffSet)
			continue;

		for (UndoStateVertex& usv : uss.delVerts) {
			auto dit = diffSet->find(usv.index);
			if (dit == diffSet->end())
				continue;

			usv.diffs.push_back(UndoStateVertexSliderDiff{activeSet[si].name, dit->second});
		}
	}
}

void OutfitProject::CollectTriangleData(NiShape* shape, UndoStateShape& uss, const std::vector<uint32_t>& indices) {
	uss.delTris.resize(indices.size());

	std::vector<Triangle> tris;
	shape->GetTriangles(tris);

	std::vector<int> triParts;
	NifSegmentationInfo inf;
	if (!workNif.GetShapeSegments(shape, inf, triParts)) {
		NiVector<BSDismemberSkinInstance::PartitionInfo> partitionInfo;
		workNif.GetShapePartitions(shape, partitionInfo, triParts);
	}

	for (uint32_t di = 0; di < static_cast<uint32_t>(indices.size()); ++di) {
		UndoStateTriangle& ust = uss.delTris[di];
		uint32_t ti = indices[di];
		ust.index = ti;
		ust.t = tris[ti];
		if (ti < triParts.size())
			ust.partID = triParts[ti];
		else
			ust.partID = -1;
	}
}

bool OutfitProject::PrepareDeleteVerts(NiShape* shape, const std::unordered_map<uint16_t, float>& mask, UndoStateShape& uss) {
	uint16_t numVerts = shape->GetNumVertices();

	// Set flag for every vertex index in mask
	std::vector<bool> delVertFlags(numVerts, false);
	for (auto& m : mask)
		delVertFlags[m.first] = true;

	// Generate list of triangles to delete.  Also count how many
	// non-deleted triangles each vertex belongs to.
	std::vector<Triangle> tris;
	shape->GetTriangles(tris);

	std::vector<uint32_t> delTriInds;
	std::vector<uint32_t> vertTriCounts(numVerts, 0);

	for (uint32_t ti = 0; ti < static_cast<uint32_t>(tris.size()); ++ti) {
		if (delVertFlags[tris[ti].p1] || delVertFlags[tris[ti].p2] || delVertFlags[tris[ti].p3])
			delTriInds.push_back(ti);
		else {
			++vertTriCounts[tris[ti].p1];
			++vertTriCounts[tris[ti].p2];
			++vertTriCounts[tris[ti].p3];
		}
	}

	// If all triangles are deleted, then delete the whole shape.
	if (delTriInds.size() >= tris.size())
		return true;

	// Generate new list of vertices to delete: vertices that are not
	// used by any non-deleted triangle.
	std::vector<uint16_t> delVertInds;
	for (uint16_t vi = 0; vi < numVerts; ++vi)
		if (vertTriCounts[vi] <= 0)
			delVertInds.push_back(vi);

	// If all vertices are deleted, then delete the whole shape.
	if (delVertInds.size() >= numVerts)
		return true;

	// Now collect the vertex and triangle data.
	CollectVertexData(shape, uss, delVertInds);
	CollectTriangleData(shape, uss, delTriInds);

	return false;
}

void OutfitProject::ApplyShapeMeshUndo(NiShape* shape, std::vector<float>& mask, UndoStateShape& uss, bool bUndo) {
	if (uss.hasClothSources)
		clothData.SetShapeSources(shape->name.get(), bUndo ? uss.clothSourcesBefore : uss.clothSourcesAfter);

	const std::vector<UndoStateVertex>& delVerts = bUndo ? uss.addVerts : uss.delVerts;
	const std::vector<UndoStateVertex>& addVerts = bUndo ? uss.delVerts : uss.addVerts;
	const std::vector<UndoStateTriangle>& delTris = bUndo ? uss.addTris : uss.delTris;
	const std::vector<UndoStateTriangle>& addTris = bUndo ? uss.delTris : uss.addTris;

	// Gather data
	std::vector<Triangle> tris;
	shape->GetTriangles(tris);

	// The LOD triangle lists of a Starfield shape index the same vertices, so they follow the edit as well
	auto* meshData = dynamic_cast<BSGeometryMeshData*>(shape->GetGeomData());
	std::vector<std::vector<Triangle>> lods;
	if (meshData) {
		if (!bUndo && !uss.hasLodsBefore) {
			uss.hasLodsBefore = true;
			uss.lodsBefore = meshData->lods;
		}

		lods = meshData->lods;
	}

	std::vector<int> triParts;
	NifSegmentationInfo inf;
	bool gotsegs = workNif.GetShapeSegments(shape, inf, triParts);
	NiVector<BSDismemberSkinInstance::PartitionInfo> partitionInfo;
	bool gotparts = false;
	if (!gotsegs)
		gotparts = workNif.GetShapePartitions(shape, partitionInfo, triParts);

	if (uss.hasSegmentInfo) {
		gotsegs = true;
		gotparts = false;
		inf = bUndo ? uss.segmentInfoBefore : uss.segmentInfoAfter;
		if (triParts.size() != tris.size())
			triParts.resize(tris.size(), -1);
	}
	else if (uss.hasPartitionInfo && !gotsegs) {
		gotparts = true;
		partitionInfo = bUndo ? uss.partitionInfoBefore : uss.partitionInfoAfter;
		if (triParts.size() != tris.size())
			triParts.resize(tris.size(), -1);
	}

	std::vector<Vector3> verts;
	workNif.GetVertsForShape(shape, verts);

	const std::vector<Vector2>* uvsp = workNif.GetUvsForShape(shape);
	const std::vector<Color4>* colorsp = workNif.GetColorsForShape(shape->name.get());
	const std::vector<Vector3>* normalsp = workNif.GetNormalsForShape(shape);
	const std::vector<Vector3>* tangentsp = workNif.GetTangentsForShape(shape);
	const std::vector<Vector3>* bitangentsp = workNif.GetBitangentsForShape(shape);
	const std::vector<float>* eyeDatap = workNif.GetEyeDataForShape(shape);

	if (uvsp && uvsp->empty())
		uvsp = nullptr;
	if (colorsp && colorsp->empty())
		colorsp = nullptr;
	if (normalsp && normalsp->empty())
		normalsp = nullptr;
	if (tangentsp && tangentsp->empty())
		tangentsp = nullptr;
	if (bitangentsp && bitangentsp->empty())
		bitangentsp = nullptr;
	if (eyeDatap && eyeDatap->empty())
		eyeDatap = nullptr;

	std::vector<Vector2> uvs;
	std::vector<Color4> colors;
	std::vector<Vector3> normals;
	std::vector<Vector3> tangents;
	std::vector<Vector3> bitangents;
	std::vector<float> eyeData;
	if (uvsp)
		uvs = *uvsp;
	if (colorsp)
		colors = *colorsp;
	if (normalsp)
		normals = *normalsp;
	if (tangentsp)
		tangents = *tangentsp;
	if (bitangentsp)
		bitangents = *bitangentsp;
	if (eyeDatap)
		eyeData = *eyeDatap;
	if (mask.size() != verts.size())
		mask.resize(verts.size());

	AnimSkin& skin = workAnim.shapeSkinning[shape->name.get()];
	std::string target = ShapeToTarget(shape->name.get());

	if (!delTris.empty()) {
		// Delete triangles
		std::vector<uint32_t> delTriInds(delTris.size());
		for (uint32_t di = 0; di < static_cast<uint32_t>(delTris.size()); ++di)
			delTriInds[di] = delTris[di].index;

		EraseVectorIndices(tris, delTriInds);

		if (gotsegs || gotparts)
			EraseVectorIndices(triParts, delTriInds);
	}

	bool makeLocal = false;

	if (!delVerts.empty()) {
		// Delete vertices...
		std::vector<uint16_t> delVertInds(delVerts.size());
		for (uint16_t di = 0; di < static_cast<uint16_t>(delVerts.size()); ++di)
			delVertInds[di] = delVerts[di].index;

		std::vector<int> vertCollapseMap = GenerateIndexCollapseMap(delVertInds, verts.size());

		// ...from triangles
		ApplyMapToTriangles(tris, vertCollapseMap);
		for (auto& lod : lods)
			ApplyMapToTriangles(lod, vertCollapseMap);

		// ...from workAnim
		workAnim.DeleteVertsForShape(shape->name.get(), delVertInds);

		// ...from diff data
		if (IsBaseShape(shape))
			baseDiffData.DeleteVerts(target, delVertInds);
		else
			morpher.DeleteVerts(target, delVertInds);

		// ...from nif arrays
		EraseVectorIndices(verts, delVertInds);
		EraseVectorIndices(mask, delVertInds);
		if (uvsp)
			EraseVectorIndices(uvs, delVertInds);
		if (colorsp)
			EraseVectorIndices(colors, delVertInds);
		if (normalsp)
			EraseVectorIndices(normals, delVertInds);
		if (tangentsp)
			EraseVectorIndices(tangents, delVertInds);
		if (bitangentsp)
			EraseVectorIndices(bitangents, delVertInds);
		if (eyeDatap)
			EraseVectorIndices(eyeData, delVertInds);

		makeLocal = true;
	}

	if (!addVerts.empty()) {
		// Insert new vertex indices...
		std::vector<uint16_t> insVertInds(addVerts.size());
		for (uint16_t di = 0; di < static_cast<uint16_t>(addVerts.size()); ++di)
			insVertInds[di] = addVerts[di].index;

		std::vector<int> vertExpandMap = GenerateIndexExpandMap(insVertInds, verts.size());

		// ...into triangles
		ApplyMapToTriangles(tris, vertExpandMap);
		for (auto& lod : lods)
			ApplyMapToTriangles(lod, vertExpandMap);

		// ...into workAnim
		skin.InsertVertexIndices(insVertInds);

		// ...into diff data
		if (IsBaseShape(shape))
			baseDiffData.InsertVertexIndices(target, insVertInds);
		else
			morpher.InsertVertexIndices(target, insVertInds);

		// ...into nif arrays
		InsertVectorIndices(verts, insVertInds);
		InsertVectorIndices(mask, insVertInds);
		if (uvsp)
			InsertVectorIndices(uvs, insVertInds);
		if (colorsp)
			InsertVectorIndices(colors, insVertInds);
		if (normalsp)
			InsertVectorIndices(normals, insVertInds);
		if (tangentsp)
			InsertVectorIndices(tangents, insVertInds);
		if (bitangentsp)
			InsertVectorIndices(bitangents, insVertInds);
		if (eyeDatap)
			InsertVectorIndices(eyeData, insVertInds);

		// Store vertex data...
		for (const UndoStateVertex& usv : addVerts) {
			// ...in nif arrays
			verts[usv.index] = usv.pos;
			mask[usv.index] = usv.mask;

			// UV
			if (uvsp && uvs.size() > usv.index)
				uvs[usv.index] = usv.uv;

			// Vertex Color
			if (colorsp && colors.size() > usv.index) {
				if (uss.hadVertexColors) {
					colors[usv.index] = usv.color;
				}
				else {
					colors[usv.index].r = 1.0f;
					colors[usv.index].g = 1.0f;
					colors[usv.index].b = 1.0f;
					colors[usv.index].a = 1.0f;
				}
			}

			// Normal
			if (normalsp && normals.size() > usv.index)
				normals[usv.index] = usv.normal;

			// Tangent
			if (tangentsp && tangents.size() > usv.index)
				tangents[usv.index] = usv.tangent;

			// Bitangent
			if (bitangentsp && bitangents.size() > usv.index)
				bitangents[usv.index] = usv.bitangent;

			// Eye Data
			if (eyeDatap && eyeData.size() > usv.index)
				eyeData[usv.index] = usv.eyeData;

			// ...in workAnim
			for (const auto& usvbw : usv.weights) {
				auto bnit = skin.boneNames.find(usvbw.boneName);
				if (bnit == skin.boneNames.end()) {
					workAnim.AddShapeBone(shape->name.get(), usvbw.boneName);
					bnit = skin.boneNames.find(usvbw.boneName);
				}
				skin.boneWeights[bnit->second].weights[usv.index] = usvbw.w;
			}

			// ...in diff data
			for (const UndoStateVertexSliderDiff& diff : usv.diffs) {
				std::string targetDataName = activeSet[diff.sliderName].TargetDataName(target);
				if (targetDataName.empty()) {
					targetDataName = target + diff.sliderName;

					if (IsBaseShape(shape))
						baseDiffData.AddEmptySet(targetDataName, target);
					else
						morpher.AddEmptySet(target, diff.sliderName);
				}

				std::unordered_map<uint16_t, Vector3>* diffSet;
				if (IsBaseShape(shape))
					diffSet = baseDiffData.GetDiffSet(targetDataName);
				else
					diffSet = morpher.GetDiffSet(targetDataName);

				if (!diffSet) // should be impossible
					continue;

				(*diffSet)[usv.index] = diff.diff;
			}
		}

		makeLocal = true;
	}

	if (makeLocal) {
		for (size_t i = 0; i < activeSet.size(); i++) {
			std::string targetData = activeSet[i].TargetDataName(target);

			if (!targetData.empty())
				activeSet[i].SetLocalData(targetData);
		}
	}

	if (!addTris.empty()) {
		// Insert new triangle indices
		std::vector<uint32_t> insTriInds(addTris.size());
		for (uint32_t di = 0; di < static_cast<uint32_t>(addTris.size()); ++di)
			insTriInds[di] = addTris[di].index;

		InsertVectorIndices(tris, insTriInds);

		if (gotsegs || gotparts)
			InsertVectorIndices(triParts, insTriInds);

		// Store triangle data
		for (const UndoStateTriangle& ust : addTris) {
			tris[ust.index] = ust.t;
			if (gotsegs || gotparts)
				triParts[ust.index] = ust.partID;
		}
	}

	// Put data back in nif
	workNif.SetVertsForShape(shape, verts);
	if (uvsp)
		workNif.SetUvsForShape(shape, uvs);
	if (colorsp)
		workNif.SetColorsForShape(shape, colors);
	if (normalsp)
		workNif.SetNormalsForShape(shape, normals);
	if (tangentsp)
		workNif.SetTangentsForShape(shape, tangents);
	if (bitangentsp)
		workNif.SetBitangentsForShape(shape, bitangents);
	if (eyeDatap)
		workNif.SetEyeDataForShape(shape, eyeData);

	shape->SetTriangles(tris);

	if (meshData) {
		// Undo brings back the LOD triangles that the edit dropped
		meshData->lods = bUndo && uss.hasLodsBefore ? uss.lodsBefore : lods;
		meshData->nLODS = static_cast<uint32_t>(meshData->lods.size());
	}

	if (gotsegs)
		workNif.SetShapeSegments(shape, inf, triParts);

	if (gotparts) {
		workNif.SetShapePartitions(shape, partitionInfo, triParts);
		workNif.RemoveEmptyPartitions(shape);
	}

	// Note that we do not restore the nif's vertex bone weights.
	// That should happen when the file is saved.
}

bool OutfitProject::PrepareCollapseVertex(NiShape* shape, UndoStateShape& uss, const std::vector<uint16_t>& indices) {
	// Get triangle data
	uint16_t numVerts = shape->GetNumVertices();

	std::vector<Triangle> tris;
	shape->GetTriangles(tris);

	std::vector<int> triParts;
	NifSegmentationInfo inf;

	if (!workNif.GetShapeSegments(shape, inf, triParts)) {
		NiVector<BSDismemberSkinInstance::PartitionInfo> partitionInfo;
		workNif.GetShapePartitions(shape, partitionInfo, triParts);
	}

	// Note that indices is already in sorted order.
	std::vector<uint16_t> nverts;
	std::vector<uint32_t> ntris;
	auto addneighbor = [&nverts](const uint16_t vi) {
		for (const auto evi : nverts)
			if (vi == evi)
				return;

		nverts.push_back(vi);
	};

	std::vector<uint32_t> nrtris; // non-replaced triangles

	for (const auto vi : indices) {
		ntris.clear();
		nverts.clear();

		// Collect lists of neighboring triangles and vertices.
		for (uint32_t ti = 0; ti < static_cast<uint32_t>(tris.size()); ++ti) {
			const Triangle& t = tris[ti];
			if (!t.HasVertex(vi))
				continue;

			if (ntris.size() >= 3)
				return false;

			ntris.push_back(ti);

			// Order of vertex collection is important, or we'll end up
			// with a backwards triangle.
			if (t.p1 == vi) {
				addneighbor(t.p2);
				addneighbor(t.p3);
			}
			else if (t.p2 == vi) {
				addneighbor(t.p3);
				addneighbor(t.p1);
			}
			else {
				addneighbor(t.p1);
				addneighbor(t.p2);
			}

			if (nverts.size() > 3)
				return false;
		}

		// Make sure no neighboring vertices are welded to this vertex.
		for (uint16_t nvi = 0; nvi < static_cast<uint16_t>(nverts.size()); ++nvi)
			for (uint16_t ii = 0; ii < static_cast<uint16_t>(indices.size()); ++ii)
				if (indices[ii] == nverts[nvi])
					return false;

		// Put triangles to delete in uss.delTris.
		for (uint32_t vti = 0; vti < static_cast<uint32_t>(ntris.size()); ++vti) {
			uint32_t ti = ntris[vti];
			uss.delTris.push_back(UndoStateTriangle{ti, tris[ti], ti < triParts.size() ? triParts[ti] : -1});
		}

		uint32_t pti = NIF_NPOS;
		if (nverts.size() == 3) {
			// Determine preferred triangle to replace.
			pti = 0;

			if (!triParts.empty() && ntris.size() == 3 && triParts[ntris[0]] != triParts[ntris[1]] && triParts[ntris[1]] == triParts[ntris[2]]) {
				pti = 1;
			}

			// Put new triangle in uss.
			uint32_t ti = ntris[pti];
			uss.addTris.push_back(UndoStateTriangle{ti, Triangle(nverts[0], nverts[1], nverts[2]), ti < triParts.size() ? triParts[ti] : -1});
		}

		// Collect list of non-replaced triangles for triangle renumbering.
		for (uint32_t vti = 0; vti < static_cast<uint32_t>(ntris.size()); ++vti)
			if (vti != pti)
				nrtris.push_back(ntris[vti]);
	}

	// We've loaded all the triangle data; now load the vertex data.
	CollectVertexData(shape, uss, indices);

	// Renumber vertex and triangle indices in uss.addTris.
	std::vector<int> vCollapse = GenerateIndexCollapseMap(indices, numVerts);

	std::sort(nrtris.begin(), nrtris.end());
	std::vector<int> tCollapse = GenerateIndexCollapseMap(nrtris, tris.size());

	for (UndoStateTriangle& ust : uss.addTris) {
		ust.t.p1 = vCollapse[ust.t.p1];
		ust.t.p2 = vCollapse[ust.t.p2];
		ust.t.p3 = vCollapse[ust.t.p3];
		ust.index = tCollapse[ust.index];
	}

	// Sort uss.addTris and uss.delTris, since they weren't added in any
	// particular order.
	std::sort(uss.addTris.begin(), uss.addTris.end());
	std::sort(uss.delTris.begin(), uss.delTris.end());

	return true;
}

static uint16_t TriangleOppositeVertex(const Triangle& t, const uint16_t p1) {
	if (t.p1 == p1)
		return t.p3;
	else if (t.p2 == p1)
		return t.p1;
	else
		return t.p2;
}

bool OutfitProject::PrepareFlipEdge(NiShape* shape, UndoStateShape& uss, const Edge& edge) {
	// Get triangle data
	std::vector<Triangle> tris;
	shape->GetTriangles(tris);

	std::vector<int> triParts;
	NifSegmentationInfo inf;
	if (!workNif.GetShapeSegments(shape, inf, triParts)) {
		NiVector<BSDismemberSkinInstance::PartitionInfo> partitionInfo;
		workNif.GetShapePartitions(shape, partitionInfo, triParts);
	}

	// Find the two neighboring triangles
	bool t1found = false;
	bool t2found = false;
	uint32_t t1 = 0;
	uint32_t t2 = 0;
	const Edge redge(edge.p2, edge.p1);

	for (uint32_t ti = 0; ti < static_cast<uint32_t>(tris.size()); ++ti) {
		if (tris[ti].HasOrientedEdge(edge)) {
			if (t1found)
				return false;

			t1 = ti;
			t1found = true;
		}

		if (tris[ti].HasOrientedEdge(redge)) {
			if (t2found)
				return false;

			t2 = ti;
			t2found = true;
		}
	}

	if (!t1found || !t2found)
		return false;

	// Find the non-edge vertex for each neighboring triangle.
	uint16_t nev1 = TriangleOppositeVertex(tris[t1], edge.p1);
	uint16_t nev2 = TriangleOppositeVertex(tris[t2], edge.p2);

	// Put data into uss.
	int tp1 = t1 < triParts.size() ? triParts[t1] : -1;
	int tp2 = t2 < triParts.size() ? triParts[t2] : -1;
	uss.delTris.push_back(UndoStateTriangle{t1, tris[t1], tp1});
	uss.delTris.push_back(UndoStateTriangle{t2, tris[t2], tp2});
	uss.addTris.push_back(UndoStateTriangle{t1, Triangle(edge.p1, nev2, nev1), tp1});
	uss.addTris.push_back(UndoStateTriangle{t2, Triangle(edge.p2, nev1, nev2), tp2});

	// Sort delTris and addTris by index.
	if (t2 < t1) {
		std::swap(uss.delTris[0], uss.delTris[1]);
		std::swap(uss.addTris[0], uss.addTris[1]);
	}

	return true;
}

/*
PrepareRefineMesh takes a list of vertices, and optionally a filter that
picks which of the edges between them to split (see MeshRefine).
Parameter "noCurveOffset" disables the smoothing/curving of the new vertex position within split edges.
*/
bool OutfitProject::PrepareRefineMesh(NiShape* shape,
									  UndoStateShape& uss,
									  std::vector<bool>& pincs,
									  const Mesh::WeldVertsType& weldVerts,
									  const bool noCurveOffset,
									  std::vector<Edge>* badEdges,
									  const std::function<bool(const Edge&)>& splitEdge) {
	// Get vertex coordinates and triangle data
	size_t nverts = pincs.size();
	const std::vector<Vector3>* verts = workNif.GetVertsForShape(shape);
	if (!verts || verts->size() != nverts)
		return false;	// Shouldn't be possible

	std::vector<Triangle> tris;
	shape->GetTriangles(tris);

	std::vector<int> triParts;
	NifSegmentationInfo inf;
	if (!workNif.GetShapeSegments(shape, inf, triParts)) {
		NiVector<BSDismemberSkinInstance::PartitionInfo> partitionInfo;
		workNif.GetShapePartitions(shape, partitionInfo, triParts);
	}

	// Ensure welded vertices are included in pincs
	for (size_t vi = 0; vi < nverts; ++vi) {
		if (!pincs[vi])
			continue;
		Mesh::DoForEachWeldedVertex(weldVerts, vi, [&](int p){pincs[p] = true;});
	}

	// Get data for all the vertices (in a secondary data-collection
	// UndoStateShape).
	UndoStateShape duss;
	std::vector<uint16_t> vertexInds, dussInds(nverts);
	for (size_t vi = 0; vi < nverts; ++vi)
		if (pincs[vi]) {
			dussInds[vi] = vertexInds.size();
			vertexInds.push_back(vi);
		}

	CollectVertexData(shape, duss, vertexInds);

	// Calculate the normal at each point by averaging triangle normals.
	std::vector<Vector3> nsums(nverts);
	for (uint16_t vi : vertexInds) {
		Vector3 nsum;
		for (const Triangle& t : tris)
			if (t.HasVertex(vi)) {
				Vector3 tn = t.trinormal(*verts);
				tn.Normalize();
				nsum += tn;
			}

		nsums[vi] = nsum;
	}
	std::vector<Vector3> normals(nverts);
	for (uint16_t vi : vertexInds) {
		Vector3 normal(nsums[vi]);
		Mesh::DoForEachWeldedVertex(weldVerts, vi, [&](int p){normal += nsums[p];});
		normal.Normalize();
		normals[vi] = normal;
	}

	// Search through the list of triangles for edges to include.
	// The boolean in singleEdges indicates whether the edge has been
	// split yet (in the following big loop).
	std::unordered_map<Edge, bool> singleEdges;
	bool hasBadEdges = false;
	for (size_t ti = 0; ti < tris.size(); ++ti) {
		if (tris[ti].p1 >= nverts || tris[ti].p2 >= nverts ||
			tris[ti].p3 >= nverts)
			return false;	// Out-of-range indices; shouldn't be possible.
		for (int tei = 0; tei < 3; ++tei) {
			Edge e = tris[ti].GetEdge(tei);
			if (!pincs[e.p1] || !pincs[e.p2])
				continue;
			if (splitEdge && !splitEdge(e))
				continue;
			if (singleEdges.find(e) != singleEdges.end()) {
				// Multiple triangles for this edge
				if (badEdges)
					badEdges->push_back(e);
				hasBadEdges = true;
				continue;
			}
			singleEdges[e] = false;
		}
	}

	if (hasBadEdges)
		return false;

	uint16_t newvi = nverts;

	// emls contains the data that will be needed for updating the
	// triangulation.  We fill it in in the big edge loop, sort it, and
	// then update the triangles (far below).
	struct EdgeWithMidAndLen: public Edge {
		uint16_t mid;
		float len;

		bool operator<(const EdgeWithMidAndLen &e2) {
			return e2.len < len;
		}
	};
	std::vector<EdgeWithMidAndLen> emls;

	// begin big loop through the edges
	std::vector<uint16_t> p1s, p2s;
	for (auto& e : singleEdges) {
		if (e.second) continue;
		e.second = true;

		Edge edge(e.first), redge;
		bool hasre = false;

		// Collect lists of welded vertices for the edge's two points.
		Mesh::GetWeldSet(weldVerts, edge.p1, p1s);
		Mesh::GetWeldSet(weldVerts, edge.p2, p2s);

		// Search for edge and reverse-edge matches in singleEdges
		bool edgeError = false;
		for (const auto p1 : p1s) {
			for (const auto p2 : p2s) {
				auto seit = singleEdges.find(Edge(p1, p2));
				if (seit != singleEdges.end() && !seit->second) {
					if (badEdges) {
						badEdges->push_back(Edge(p1, p2));
						hasBadEdges = true;
						edgeError = true;
						continue;
					}
					return false;	// Multiple triangles for this edge
				}
				seit = singleEdges.find(Edge(p2, p1));
				if (seit == singleEdges.end())
					continue;
				if (seit->second) {
					if (badEdges) {
						badEdges->push_back(Edge(p2, p1));
						hasBadEdges = true;
						edgeError = true;
						continue;
					}
					return false;	// Shouldn't be possible
				}
				seit->second = true;
				if (hasre) {
					if (badEdges) {
						badEdges->push_back(Edge(p2, p1));
						hasBadEdges = true;
						edgeError = true;
						continue;
					}
					return false;	// Multiple reverse edges for this edge
				}
				hasre = true;
				redge = Edge(p2, p1);
			}
		}

		if (edgeError)
			continue;

		// Find the edge and redge vertex data
		const UndoStateVertex& p1d = duss.delVerts[dussInds[edge.p1]];
		const UndoStateVertex& p2d = duss.delVerts[dussInds[edge.p2]];
		const UndoStateVertex& rp1d = duss.delVerts[dussInds[redge.p1]];
		const UndoStateVertex& rp2d = duss.delVerts[dussInds[redge.p2]];
		const Vector3& p1 = p1d.pos;
		const Vector3& p2 = p2d.pos;
		const Vector3& np1 = normals[edge.p1];
		const Vector3& np2 = normals[edge.p2];

		bool welded = hasre && (redge.p1 != edge.p2 || redge.p2 != edge.p1);

		// Create entry for new vertex and (if welded) new welded vertex
		uss.addVerts.emplace_back();
		uss.addVerts.back().index = newvi++;
		if (welded) {
			uss.addVerts.emplace_back();
			uss.addVerts.back().index = newvi++;
		}
		UndoStateVertex& usv = uss.addVerts[uss.addVerts.size() - (welded ? 2 : 1)];
		UndoStateVertex& rusv = uss.addVerts.back();

		/* Now comes the hard part: determining a good location for the
		new vertex.  Clearly it should lie somewhere on the plane bisecting
		the segment between p1 and p2.  If we wanted to be lazy, we could
		just pick the midpoint.  But that would require the user to adjust
		the position every time.  Better would be if we could fit a circle
		through p1, p2, and the new point so that all three have normals
		perpendicular to the circle.  But there's no guarantee that that's
		possible: the old normals could have different angles to the edge;
		and they could even be non-coplanar.  So we need to average the
		angles for the two normals somehow.  There are several stages
		where this could be done; I choose to do it early. */

		// Starting position of new point: midpoint between p1 and p2
		usv.pos = (p1 + p2) * 0.5f;

		Vector3 u12 = p2 - p1;	   // unit vector from p1 to p2 (along the edge)
		float elen = u12.length(); // edge length
		u12.Normalize();

		usv.normal = np1 + np2;

		if (!noCurveOffset) {
			// Working normal for new point: average of np1 and np2, made
			// perpendicular to u12.  (If you want something fancy for usv.normal,
			// calculate it later.  This calculation needs to be done this
			// way for the circle fitter.)
			usv.normal -= u12 * u12.dot(usv.normal);
		}

		usv.normal.Normalize();

		float curveOffsetFactor = 1.0f;
		if (!noCurveOffset) {
			// Now, the angle between npi and usv.normal, in the plane of
			// npi and u12 (since usv.normal isn't necessarily in that plane)
			// would be asin(u12.dot(npi)).  We want to average this for np1
			// and np2 and carefully preserve the sign.
			float angle = asin(u12.dot(np2 - np1) * 0.5);

			// Now, "angle" is the desired circle angle between the new point and
			// either p1 or p2.  It's positive for convex, negative for concave.
			// To figure out how far off of the edge we need to go, we need to
			// take the trigonometric tangent of the correct angle.  It turns out
			// the correct angle is the angle we just calculated divided by 2.
			curveOffsetFactor = tan(angle * 0.5);

			// Now apply the offset to the new point.  (curveOffsetFactor is
			// positive for convex, negative for concave, and will be no larger
			// than 1.)
			usv.pos += usv.normal * (curveOffsetFactor * elen * 0.5f);
		}

		// Calculate uv, color, tangent, bitangent, and eyeData by averaging.
		// We need to make sure normal, tangent, and bitangent are
		// perpendicular.
		usv.uv = (p1d.uv + p2d.uv) * 0.5;
		usv.color.r = (p1d.color.r + p2d.color.r) * 0.5f;
		usv.color.g = (p1d.color.g + p2d.color.g) * 0.5f;
		usv.color.b = (p1d.color.b + p2d.color.b) * 0.5f;
		usv.color.a = (p1d.color.a + p2d.color.a) * 0.5f;
		usv.eyeData = (p1d.eyeData + p2d.eyeData) * 0.5f;
		usv.mask = std::min(p1d.mask, p2d.mask);
		usv.tangent = (p1d.tangent + p2d.tangent) * 0.5f;
		usv.bitangent = (p1d.bitangent + p2d.bitangent) * 0.5f;
		usv.tangent -= usv.normal * usv.normal.dot(usv.tangent);
		usv.tangent.Normalize();
		usv.bitangent -= usv.normal * usv.normal.dot(usv.bitangent);
		usv.bitangent -= usv.tangent * usv.tangent.dot(usv.bitangent);
		usv.bitangent.Normalize();

		// Calculate weights by averaging.  Note that the resulting weights
		// will add up to 1 if each source's weights do.
		for (int si = 0; si < 2; ++si) {
			const std::vector<UndoStateVertexBoneWeight>& swts = si ? p2d.weights : p1d.weights;
			for (const UndoStateVertexBoneWeight& sw : swts) {
				bool found = false;
				for (size_t dwi = 0; dwi < usv.weights.size() && !found; ++dwi) {
					if (usv.weights[dwi].boneName == sw.boneName) {
						found = true;
						usv.weights[dwi].w += sw.w;
					}
				}

				if (!found)
					usv.weights.push_back(sw);
			}
		}

		for (auto& dw : usv.weights)
			dw.w *= 0.5;

		if (welded) {
			// Welded vertex gets exactly the same data except uv and uv-diffs.
			uint16_t rvi = rusv.index;
			rusv = usv;
			rusv.index = rvi;
			rusv.uv = (rp1d.uv + rp2d.uv) * 0.5;
		}

		// Unfortunately, we can't just calculate diffs by averaging.  p1
		// and p2 may have moved farther apart, which would require greater
		// curve offset.

		// diffpairs: key is sliderName.
		std::unordered_map<std::string, std::pair<Vector3, Vector3>> diffpairs;
		for (auto& sd : p1d.diffs)
			diffpairs[sd.sliderName].first = sd.diff;
		for (auto& sd : p2d.diffs)
			diffpairs[sd.sliderName].second = sd.diff;

		for (auto& dp : diffpairs) {
			// First, just average the diffs.
			Vector3 diff = (dp.second.first + dp.second.second) * 0.5f;
			const SliderData& sd = activeSet[dp.first];
			if (!noCurveOffset && !sd.bUV && !sd.bClamp && !sd.bZap) {
				// Calculate the distance between the moved p1 and p2.
				float delen = (dp.second.second + p2 - dp.second.first - p1).length();
				// Apply more curve offset (for delen > elen)
				// or less (for delen < elen).
				diff += usv.normal * (curveOffsetFactor * (delen - elen) * 0.5f);
			}

			usv.diffs.push_back(UndoStateVertexSliderDiff{dp.first, diff});
			if (welded && !sd.bUV)
				rusv.diffs.push_back(UndoStateVertexSliderDiff{dp.first, diff});
		}

		if (welded) {
			// Repeat the diff calculation, but only for UV diffs.
			std::unordered_map<std::string, std::pair<Vector3, Vector3>> rdiffpairs;
			for (auto& sd : rp1d.diffs)
				rdiffpairs[sd.sliderName].first = sd.diff;
			for (auto& sd : rp2d.diffs)
				rdiffpairs[sd.sliderName].second = sd.diff;

			for (auto& dp : rdiffpairs) {
				const SliderData& sd = activeSet[dp.first];
				if (!sd.bUV)
					continue;

				Vector3 diff = (dp.second.first + dp.second.second) * 0.5f;
				rusv.diffs.push_back(UndoStateVertexSliderDiff{dp.first, diff});
			}
		}

		// Now usv and rusv are finished.  Add the edge and reverse edge to
		// the list of edges that need triangle updates.
		emls.emplace_back(EdgeWithMidAndLen{edge, usv.index, elen});
		if (hasre)
			emls.emplace_back(EdgeWithMidAndLen{redge, rusv.index, elen});
	}
	// end big loop through the edges

	if (hasBadEdges)
		return false;

	// Sort the edges from longest to shortest so that we'll split the
	// triangles for the longest edges first.
	std::sort(emls.begin(), emls.end());

	for (auto& eml : emls) {
		/* We know (because we've already checked) that there is
		exactly one triangle in the old triangle list with the oriented
		edge eml.  However, it might have already been split into two
		or more triangles.  So we have to look in the new triangle
		list and then the old.  We also don't know ahead of time
		how many times an original triangle will be split in two,
		so we can't know the new triangle numbering until the end.
		So we assign each triangle of the pair created the same index
		as the original, for now, and we'll update those indices later,
		after the new-triangle list has been sorted. */
		bool found = false;

		// Loop through the new triangles
		for (auto& ust : uss.addTris) {
			if (!ust.t.HasOrientedEdge(eml))
				continue;
			found = true;
			uint16_t ovi = TriangleOppositeVertex(ust.t, eml.p1);
			ust.t.p1 = eml.p1;
			ust.t.p2 = eml.mid;
			ust.t.p3 = ovi;
			UndoStateTriangle newust = ust; // copies index and partID too
			newust.t.p1 = eml.mid;
			newust.t.p2 = eml.p2;
			uss.addTris.push_back(newust);
			break;
		}
		if (found)
			continue;

		// Loop through the old triangles
		for (size_t ti = 0; ti < tris.size(); ++ti) {
			if (!tris[ti].HasOrientedEdge(eml))
				continue;
			found = true;
			uint16_t ovi = TriangleOppositeVertex(tris[ti], eml.p1);
			int tp = ti < triParts.size() ? triParts[ti] : -1;
			uint32_t sti = static_cast<uint32_t>(ti);
			uss.delTris.push_back(UndoStateTriangle{sti, tris[ti], tp});
			uss.addTris.push_back(UndoStateTriangle{sti, Triangle(eml.p1, eml.mid, ovi), tp});
			uss.addTris.push_back(UndoStateTriangle{sti, Triangle(eml.mid, eml.p2, ovi), tp});
			break;
		}
		// assert(found)
	}

	// Sort delTris and addTris by index.
	std::sort(uss.delTris.begin(), uss.delTris.end());
	std::sort(uss.addTris.begin(), uss.addTris.end());

	// Update the triangle indices in addTris so indices are not duplicated
	uint32_t tioffset = 0, lastindex = 0;
	for (size_t ati = 0; ati < uss.addTris.size(); ++ati) {
		uint32_t& ti = uss.addTris[ati].index;
		if (ati != 0 && ti == lastindex)
			++tioffset;
		lastindex = ti;
		ti += tioffset;
	}

	return true;
}

bool OutfitProject::IsVertexOnBoundary(NiShape* shape, int vi) {
	std::vector<Triangle> tris;
	shape->GetTriangles(tris);

	// Count how many times each neighboring vertex is in a triangle with
	// this vertex
	std::unordered_map<uint16_t, int> vcounts;
	for (const Triangle &t : tris) {
		if (!t.HasVertex(vi))
			continue;

		if (t.p1 != vi) vcounts[t.p1]++;
		if (t.p2 != vi) vcounts[t.p2]++;
		if (t.p3 != vi) vcounts[t.p3]++;
	}

	// If any vertex count is not even, the corresponding edge is on the
	// boundary.  (Note that if any vc is greater than 2, the surface is
	// messed up.)
	for (auto& vc : vcounts)
		if (vc.second % 2 != 0)
			return true;

	return false;
}

bool OutfitProject::PointsHaveDifferingWeightsOrDiffs(NiShape* shape1, int p1, NiShape* shape2, int p2) {
	// Check for position differences
	const AnimSkin& skin1 = shape1->IsSkinned() ? workAnim.shapeSkinning[shape1->name.get()] : AnimSkin();
	const AnimSkin& skin2 = shape2->IsSkinned() ? workAnim.shapeSkinning[shape2->name.get()] : AnimSkin();
	MatTransform skin1ToGlobal = workAnim.GetTransformShapeToGlobal(shape1);
	MatTransform skin2ToGlobal = workAnim.GetTransformShapeToGlobal(shape2);
	const std::vector<Vector3>* verts1 = workNif.GetVertsForShape(shape1);
	const std::vector<Vector3>* verts2 = workNif.GetVertsForShape(shape2);
	if (verts1 && verts2 && (*verts1)[p1] != (*verts2)[p2])
		return true;

	// Check for bone weight differences
	for (auto bnp : skin1.boneNames) {
		const AnimWeight& aw1 = skin1.boneWeights.at(bnp.second);
		auto wit1 = aw1.weights.find(p1);
		if (wit1 == aw1.weights.end() || wit1->second == 0.0f)
			continue;
		auto bnit = skin2.boneNames.find(bnp.first);
		if (bnit == skin2.boneNames.end())
			return true;
		const AnimWeight& aw2 = skin2.boneWeights.at(bnit->second);
		auto wit2 = aw2.weights.find(p2);
		if (wit2 == aw2.weights.end() || wit2->second != wit1->second)
			return true;
	}
	for (auto bnp : skin2.boneNames) {
		const AnimWeight& aw2 = skin2.boneWeights.at(bnp.second);
		auto wit2 = aw2.weights.find(p2);
		if (wit2 == aw2.weights.end() || wit2->second == 0.0f)
			continue;
		auto bnit = skin1.boneNames.find(bnp.first);
		if (bnit == skin1.boneNames.end())
			return true;
		const AnimWeight& aw1 = skin1.boneWeights.at(bnit->second);
		auto wit1 = aw1.weights.find(p1);
		if (wit1 == aw1.weights.end() || wit1->second != wit2->second)
			return true;
	}

	// Check for position slider differences.  We skip uv, clamp, and zap.
	for (size_t si = 0; si < activeSet.size(); ++si) {
		if (activeSet[si].bUV || activeSet[si].bClamp || activeSet[si].bZap)
			continue;

		std::unordered_map<uint16_t, Vector3>* diffSet1 = GetDiffSet(activeSet[si], shape1);
		std::unordered_map<uint16_t, Vector3>* diffSet2 = GetDiffSet(activeSet[si], shape2);

		if (!diffSet1 && !diffSet2)
			continue;

		Vector3 diff1, diff2;
		if (diffSet1) {
			auto dit1 = diffSet1->find(p1);
			if (dit1 != diffSet1->end())
				diff1 = skin1ToGlobal.ApplyTransformToDiff(dit1->second);
		}
		if (diffSet2) {
			auto dit2 = diffSet2->find(p2);
			if (dit2 != diffSet2->end())
				diff2 = skin2ToGlobal.ApplyTransformToDiff(dit2->second);
		}
		if (diff1 != diff2)
			return true;
	}

	return false;
}

void OutfitProject::PrepareMergeVertex(NiShape* shape, UndoStateShape& uss, int selVert, int targVert) {
	// Get triangle data
	uint16_t numVerts = shape->GetNumVertices();

	std::vector<Triangle> tris;
	shape->GetTriangles(tris);

	std::vector<int> triParts;
	NifSegmentationInfo inf;

	if (!workNif.GetShapeSegments(shape, inf, triParts)) {
		NiVector<BSDismemberSkinInstance::PartitionInfo> partitionInfo;
		workNif.GetShapePartitions(shape, partitionInfo, triParts);
	}

	// Delete vertex
	std::vector<uint16_t> indices(1, selVert);
	CollectVertexData(shape, uss, indices);
	std::vector<int> vCollapse = GenerateIndexCollapseMap(indices, numVerts);

	// Replace triangles
	for (uint32_t ti = 0; ti < tris.size(); ++ti) {
		const Triangle& t = tris[ti];
		if (!t.HasVertex(selVert))
			continue;

		// Replace selVert with targVert
		Triangle newt = t;
		if (newt.p1 == selVert)
			newt.p1 = targVert;
		if (newt.p2 == selVert)
			newt.p2 = targVert;
		if (newt.p3 == selVert)
			newt.p3 = targVert;

		// Get partition number
		int part = -1;
		if (ti < triParts.size())
			part = triParts[ti];

		// Collapse vertex indices
		newt.p1 = vCollapse[newt.p1];
		newt.p2 = vCollapse[newt.p2];
		newt.p3 = vCollapse[newt.p3];

		uss.delTris.push_back(UndoStateTriangle{ti, t, part});
		uss.addTris.push_back(UndoStateTriangle{ti, newt, part});
	}
}

void OutfitProject::PrepareWeldVertex(NiShape* shape, UndoStateShape& uss, int selVert, int targVert, NiShape* targShape) {
	// Get triangle data
	std::vector<Triangle> tris;
	shape->GetTriangles(tris);

	std::vector<int> triParts;
	NifSegmentationInfo inf;

	if (!workNif.GetShapeSegments(shape, inf, triParts)) {
		NiVector<BSDismemberSkinInstance::PartitionInfo> partitionInfo;
		workNif.GetShapePartitions(shape, partitionInfo, triParts);
	}

	// Delete and restore all triangles that use selVert
	for (uint32_t ti = 0; ti < tris.size(); ++ti) {
		const Triangle& t = tris[ti];
		if (!t.HasVertex(selVert))
			continue;

		// Get partition number
		int part = -1;
		if (ti < triParts.size())
			part = triParts[ti];

		uss.delTris.push_back(UndoStateTriangle{ti, t, part});
		uss.addTris.push_back(UndoStateTriangle{ti, t, part});
	}

	// Get data for the target vertex (in a secondary data-collection
	// UndoStateShape).
	UndoStateShape duss;
	std::vector<uint16_t> targInds(1, targVert);
	CollectVertexData(targShape, duss, targInds);
	UndoStateVertex &tusv = duss.delVerts[0];

	// Calculate transform from target's skin coordinates to selected's.
	MatTransform skx = workAnim.GetTransformGlobalToShape(shape).ComposeTransforms(workAnim.GetTransformShapeToGlobal(targShape));

	// Delete vertex
	std::vector<uint16_t> selInds(1, selVert);
	CollectVertexData(shape, uss, selInds);
	UndoStateVertex &susv = uss.delVerts.back();

	// Build replacement vertex from a mix of selected and target vertex data
	uss.addVerts.emplace_back();
	UndoStateVertex& usv = uss.addVerts.back();
	usv.index = selVert;
	usv.pos = skx.ApplyTransform(tusv.pos);
	usv.uv = susv.uv;
	usv.color = tusv.color;
	usv.normal = skx.ApplyTransformToDir(tusv.normal);
	usv.tangent = skx.ApplyTransformToDir(tusv.tangent);
	usv.bitangent = skx.ApplyTransformToDir(tusv.bitangent);
	usv.eyeData = tusv.eyeData;
	usv.mask = tusv.mask;
	usv.weights = std::move(tusv.weights);

	// UV, clamp, and zap diffs come from selected vertex; position diffs
	// come from target
	for (const auto& diff : susv.diffs) {
		const SliderData& sd = activeSet[diff.sliderName];
		if (sd.bUV || sd.bClamp || sd.bZap)
			usv.diffs.push_back(diff);
	}
	for (const auto& diff : tusv.diffs) {
		const SliderData& sd = activeSet[diff.sliderName];
		if (!sd.bUV && !sd.bClamp && !sd.bZap) {
			Vector3 d = skx.ApplyTransformToDiff(diff.diff);
			usv.diffs.push_back(UndoStateVertexSliderDiff{diff.sliderName, d});
		}
	}
}

void OutfitProject::CheckMerge(const std::string& sourceName, const std::string& targetName, MergeCheckErrors& e) {
	if (sourceName == targetName) {
		e.shapesSame = true;
		return;
	}

	NiShape* source = workNif.FindBlockByName<NiShape>(sourceName);
	if (!source)
		return;
	NiShape* target = workNif.FindBlockByName<NiShape>(targetName);
	if (!target)
		return;

	constexpr size_t maxVertIndex = std::numeric_limits<uint16_t>().max();
	size_t maxTriIndex = std::numeric_limits<uint16_t>().max();
	if (workNif.GetHeader().GetVersion().IsFO4() || workNif.GetHeader().GetVersion().IsFO76())
		maxTriIndex = std::numeric_limits<uint32_t>().max();

	size_t snVerts = source->GetNumVertices();
	size_t tnVerts = target->GetNumVertices();
	if (snVerts + tnVerts > maxVertIndex)
		e.tooManyVertices = true;

	size_t snTris = source->GetNumTriangles();
	size_t tnTris = target->GetNumTriangles();
	if (snTris + tnTris > maxTriIndex)
		e.tooManyTriangles = true;

	std::vector<int> triParts;
	NifSegmentationInfo sinf, tinf;
	bool gotssegs = workNif.GetShapeSegments(source, sinf, triParts);
	bool gottsegs = workNif.GetShapeSegments(target, tinf, triParts);
	if (gotssegs != gottsegs) {
		// Shape with segments and the other without
		e.segmentsMismatch = true;
	}
	else if (gotssegs) {
		// Both shapes have segments
		if (sinf.ssfFile != tinf.ssfFile) {
			// Segment definition file path differs
			e.segmentsMismatch = true;
		}

		if (sinf.segs.size() != tinf.segs.size()) {
			// Shapes have different amount of segments
			e.segmentsMismatch = true;
		}

		for (size_t si = 0; !e.segmentsMismatch && si < sinf.segs.size(); ++si) {
			if (sinf.segs[si].subs.size() != tinf.segs[si].subs.size()) {
				// Shapes have different amount of sub segments
				e.segmentsMismatch = true;
			}

			for (size_t ssi = 0; !e.segmentsMismatch && ssi < sinf.segs[si].subs.size(); ++ssi) {
				const NifSubSegmentInfo& sssinf = sinf.segs[si].subs[ssi];
				const NifSubSegmentInfo& tssinf = tinf.segs[si].subs[ssi];
				if (sssinf.userSlotID != tssinf.userSlotID || sssinf.material != tssinf.material) {
					// Sub segment information differs
					e.segmentsMismatch = true;
				}
			}
		}
	}

	if (workNif.GetHeader().GetVersion().File() == NiFileVersion::V20_2_0_7) {
		NiVector<BSDismemberSkinInstance::PartitionInfo> spinf, tpinf;
		bool gotspar = workNif.GetShapePartitions(source, spinf, triParts);
		bool gottpar = workNif.GetShapePartitions(target, tpinf, triParts);
		if (gotspar != gottpar) {
			// Shape with partitions and the other without
			e.partitionsMismatch = true;
		}
		else if (gotspar) {
			// Both shapes have partitions
			if (spinf.size() != tpinf.size()) {
				// Shapes have different amount of partitions
				e.partitionsMismatch = true;
			}

			for (uint32_t pi = 0; !e.partitionsMismatch && pi < spinf.size(); ++pi) {
				if (spinf[pi].partID != tpinf[pi].partID) {
					// Partition slot differs
					e.partitionsMismatch = true;
				}
			}
		}
	}

	auto sShader = workNif.GetShader(source);
	auto tShader = workNif.GetShader(target);
	if ((sShader != nullptr) != (tShader != nullptr)) {
		// Shape with shader and the other without
		e.shaderMismatch = true;
	}
	else if (sShader) {
		// Both shapes have a shader
		if (sShader->GetShaderType() != tShader->GetShaderType()) {
			// Shader type differs
			e.shaderMismatch = true;
		}
		else if (workNif.GetHeader().GetVersion().IsFO4() || workNif.GetHeader().GetVersion().IsFO76()) {
			if (!StringsEqualInsens(sShader->name.get().c_str(), tShader->name.get().c_str())
				|| !StringsEqualInsens(sShader->GetWetMaterialName().c_str(), tShader->GetWetMaterialName().c_str())) {
				// Material file paths differ
				e.shaderMismatch = true;
			}
		}

		std::string sTexBase, tTexBase;
		workNif.GetTextureSlot(source, sTexBase);
		workNif.GetTextureSlot(target, tTexBase);

		if (!StringsEqualInsens(sTexBase.c_str(), tTexBase.c_str())) {
			// Base texture path differs (and possibly UV layout)
			e.textureMismatch = true;
		}
	}

	auto sAlphaProp = workNif.GetAlphaProperty(source);
	auto tAlphaProp = workNif.GetAlphaProperty(target);
	if ((sAlphaProp != nullptr) != (tAlphaProp != nullptr)) {
		// Shape with alpha property and the other without
		e.alphaPropMismatch = true;
	}
	else if (sAlphaProp) {
		// Both shapes have an alpha property
		if (sAlphaProp->flags != tAlphaProp->flags || sAlphaProp->threshold != tAlphaProp->threshold) {
			// Flags or threshold differs
			e.alphaPropMismatch = true;
		}
	}

	if (!workAnim.GetTransformShapeToGlobal(source).IsNearlyEqualTo(workAnim.GetTransformShapeToGlobal(target))) {
		// Coordinate systems of the shapes differ, so the merge has to apply
		// the transforms to the geometry of both shapes first
		e.transformsMismatch = true;
	}

	e.canMerge = !e.tooManyVertices && !e.tooManyTriangles && !e.shaderMismatch && !e.alphaPropMismatch;
}

void OutfitProject::PrepareCopyGeo(NiShape* source, NiShape* target, UndoStateShape& uss) {
	if (!source || !target)
		return;

	// The vertices are collected in the source shape's coordinates and appended to
	// the target shape, so both shapes need to share a coordinate system. If they
	// don't, apply the transforms to the geometry of both shapes and clear them,
	// which leaves the meshes where they are in global coordinates.
	if (!workAnim.GetTransformShapeToGlobal(source).IsNearlyEqualTo(workAnim.GetTransformShapeToGlobal(target))) {
		ApplyShapeTransformToGeometry(source);
		ApplyShapeTransformToGeometry(target);
	}

	uss.hasClothSources = true;
	uss.clothSourcesBefore = clothData.GetShapeSources(target->name.get());
	uss.clothSourcesAfter = uss.clothSourcesBefore;
	for (const auto& sourceName : clothData.GetShapeSources(source->name.get())) {
		if (std::find(uss.clothSourcesAfter.begin(), uss.clothSourcesAfter.end(), sourceName) == uss.clothSourcesAfter.end())
			uss.clothSourcesAfter.push_back(sourceName);
	}

	uint16_t snVerts = source->GetNumVertices();
	uint16_t tnVerts = target->GetNumVertices();
	uint32_t snTris = source->GetNumTriangles();
	uint32_t tnTris = target->GetNumTriangles();

	std::vector<uint16_t> vinds(snVerts);
	std::vector<uint32_t> tinds(snTris);
	for (uint16_t i = 0; i < snVerts; ++i)
		vinds[i] = i;
	for (uint32_t i = 0; i < snTris; ++i)
		tinds[i] = i;

	CollectVertexData(source, uss, vinds);
	CollectTriangleData(source, uss, tinds);

	std::vector<int> sourceTriParts;
	NifSegmentationInfo sourceSegInfo;
	if (workNif.GetShapeSegments(source, sourceSegInfo, sourceTriParts)) {
		ReconcileCopyGeoSegments(workNif, source, target, uss);
	}
	else if (workNif.GetHeader().GetVersion().File() == NiFileVersion::V20_2_0_7) {
		ReconcileCopyGeoPartitions(workNif, source, target, uss);
	}

	for (uint16_t vi = 0; vi < snVerts; ++vi)
		uss.delVerts[vi].index += tnVerts;

	for (uint32_t ti = 0; ti < snTris; ++ti) {
		uss.delTris[ti].index += tnTris;
		uss.delTris[ti].t.p1 += tnVerts;
		uss.delTris[ti].t.p2 += tnVerts;
		uss.delTris[ti].t.p3 += tnVerts;
	}

	uss.delVerts.swap(uss.addVerts);
	uss.delTris.swap(uss.addTris);
}
