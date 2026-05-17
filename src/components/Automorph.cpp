/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "Automorph.h"
#include "Anim.h"

#include <algorithm>


using namespace nifly;

Automorph::Automorph() {}

Automorph::~Automorph() {
	ClearSourceShapes();
}

void Automorph::ClearSourceShapes() {
	for (auto& shapes : sourceShapes)
		delete shapes.second;

	sourceShapes.clear();
}

void Automorph::RenameResultDiffData(const std::string& shape, const std::string& oldName, const std::string& newName) {
	std::string setName = ResultDataName(shape, oldName);
	std::string newSetName = shape + newName;

	resultDiffData.RenameSet(setName, newSetName);
	targetSliderDataNames.erase(setName);
}

void Automorph::RenameShape(const std::string& oldShapeName, const std::string& newShapeName) {
	if (sourceShapes.find(oldShapeName) != sourceShapes.end()) {
		sourceShapes[newShapeName] = sourceShapes[oldShapeName];
		sourceShapes.erase(oldShapeName);
	}

	std::vector<std::string> oldKeys;
	for (auto& tsdn : targetSliderDataNames) {
		bool found = false;
		size_t p = tsdn.second.find(oldShapeName);
		if (p == 0)
			found = true;

		p = tsdn.first.find(oldShapeName);
		if (p == 0)
			found = true;

		if (found)
			oldKeys.push_back(tsdn.first);
	}

	for (size_t i = 0; i < oldKeys.size(); i++)
		targetSliderDataNames.erase(oldKeys[i]);
}

void Automorph::RenameDataTarget(const std::string& oldTarget, const std::string& newTarget) {
	resultDiffData.RenameDataTarget(oldTarget, newTarget);
}

void Automorph::RenameSet(const std::string& oldName, const std::string& newName) {
	resultDiffData.RenameSet(oldName, newName);
}

void Automorph::CopySet(const std::string& oldName, const std::string& newName, const std::string& newTargetName) {
	resultDiffData.CopySet(oldName, newName, newTargetName);
}

void Automorph::CopyShape(const std::string& srcShapeName, const std::string& destShapeName) {
	std::vector<std::string> newVals;
	std::vector<std::string> oldKeys;
	std::vector<std::string> newKeys;

	for (auto& tsdn : targetSliderDataNames) {
		std::string newDN = tsdn.second;
		std::string newKey = tsdn.first;

		bool found = false;
		size_t p = newDN.find(srcShapeName);
		if (p == 0) {
			newDN = newDN.substr(srcShapeName.length());
			newDN = destShapeName + newDN;
			found = true;
		}

		p = newKey.find(srcShapeName);
		if (p == 0) {
			newKey = newKey.substr(srcShapeName.length());
			newKey = destShapeName + newKey;
			found = true;
		}

		if (found) {
			oldKeys.push_back(tsdn.first);
			newKeys.push_back(newKey);
			newVals.push_back(newDN);
		}
	}

	for (size_t i = 0; i < oldKeys.size(); i++)
		targetSliderDataNames[newKeys[i]] = newVals[i];
}

std::string Automorph::GetDataTargetName(const std::string& targetName, const std::string& dataNameSuffix) {
	return resultDiffData.GetDataTargetName(targetName, dataNameSuffix);
}

void Automorph::SetRef(NifFile& ref, NiShape* refShape, const AnimInfo* workAnim) {
	morphRef = std::make_unique<Mesh>();
	MeshFromNifShape(morphRef.get(), ref, refShape, workAnim);

	refTree = std::make_unique<kd_tree<uint16_t>>(morphRef->verts.get(), static_cast<uint16_t>(morphRef->nVerts));
}

void Automorph::LinkRefDiffData(DiffDataSets* diffData) {
	srcDiffData = diffData;
}

void Automorph::UnlinkRefDiffData() {
	srcDiffData = &__srcDiffData;
}

bool Automorph::ApplyDiffToVerts(const std::string& sliderName, const std::string& shapeTargetName, std::vector<Vector3>* inOutResult, float strength) {
	return srcDiffData->ApplyDiff(sliderName, shapeTargetName, strength, inOutResult);
}

bool Automorph::ApplyResultToVerts(const std::string& sliderName, const std::string& shapeTargetName, std::vector<Vector3>* inOutResult, float strength) {
	std::string setname = ResultDataName(shapeTargetName, sliderName);

	return resultDiffData.ApplyDiff(setname, shapeTargetName, strength, inOutResult);
}

bool Automorph::ApplyResultToUVs(const std::string& sliderName, const std::string& shapeTargetName, std::vector<Vector2>* inOutResult, float strength) {
	std::string setname = ResultDataName(shapeTargetName, sliderName);

	return resultDiffData.ApplyUVDiff(setname, shapeTargetName, strength, inOutResult);
}

void Automorph::SourceShapesFromNif(NifFile& baseNif, const AnimInfo* workAnim) {
	ClearSourceShapes();

	auto shapes = baseNif.GetShapes();
	for (auto& s : shapes) {
		Mesh* m = new Mesh();
		MeshFromNifShape(m, baseNif, s, workAnim);
		sourceShapes[s->name.get()] = m;
	}
}

void Automorph::UpdateMeshFromNif(NifFile& baseNif, const std::string& shapeName) {
	if (sourceShapes.find(shapeName) == sourceShapes.end())
		return;

	Mesh* m = sourceShapes[shapeName];
	auto shape = baseNif.FindBlockByName<NiShape>(shapeName);

	std::vector<Vector3> upVerts;
	baseNif.GetVertsForShape(shape, upVerts);

	int numUpVerts = upVerts.size();
	if (m->nVerts != numUpVerts)
		return;

	for (int i = 0; i < m->nVerts; i++)
		m->verts[i] = upVerts[i];
}

void Automorph::CopyMeshMask(Mesh* m, const std::string& shapeName) {
	Mesh* dm = sourceShapes[shapeName];
	if (dm->nVerts != m->nVerts)
		return;

	if (!m->mask)
		return;

	if (!dm->mask)
		dm->mask = std::make_unique<float[]>(dm->nVerts);

	for (int i = 0; i < dm->nVerts; i++)
		dm->mask[i] = m->mask[i];
}

void Automorph::MeshFromNifShape(Mesh* m, NifFile& ref, NiShape* shape, const AnimInfo* workAnim) {
	std::vector<Vector3> nifVerts;
	ref.GetVertsForShape(shape, nifVerts);
	std::vector<Triangle> nifTris;
	shape->GetTriangles(nifTris);

	m->shapeName = shape->name.get();

	m->SetXformMeshToModel(workAnim->GetTransformShapeToGlobal(shape));

	m->nVerts = nifVerts.size();
	m->verts = std::make_unique<Vector3[]>(m->nVerts);
	m->nTris = nifTris.size();
	if (m->nTris > 0) {
		m->tris = std::make_unique<Triangle[]>(m->nTris);
		std::copy(nifTris.begin(), nifTris.end(), m->tris.get());
	}

	// Load verts. No CS transformation is done (in contrast to the very similar code in GLSurface).
	for (int i = 0; i < m->nVerts; i++)
		m->verts[i] = m->TransformPosMeshToModel(nifVerts[i]);
}

void Automorph::DeleteVerts(const std::string& shapeName, const std::vector<uint16_t>& indices) {
	resultDiffData.DeleteVerts(shapeName, indices);
}

void Automorph::InsertVertexIndices(const std::string& target, const std::vector<uint16_t>& indices) {
	resultDiffData.InsertVertexIndices(target, indices);
}

void Automorph::ClearProximityCache() {
	prox_cache.clear();
}

void Automorph::BuildProximityCache(const std::string& shapeName, float proximityRadius, const std::set<uint16_t>* maskIndices) {
	if (sourceShapes.find(shapeName) == sourceShapes.end())
		return;

	Mesh* m = sourceShapes[shapeName];
	uint16_t maxCount = 0;
	uint16_t minCount = std::numeric_limits<uint16_t>::max();

	for (int i = 0; i < m->nVerts; i++) {
		uint16_t resultCount = refTree->kd_nn(&m->verts[i], proximityRadius);
		if (resultCount < minCount)
			minCount = resultCount;
		if (resultCount > maxCount)
			maxCount = resultCount;

		std::vector<kd_query_result<uint16_t>> indexResults;
		for (uint16_t id = 0; id < resultCount; id++) {
			auto& result = refTree->queryResult[id];

			if (maskIndices) {
				if (maskIndices->find(result.vertex_index) != maskIndices->end())
					continue;
			}

			indexResults.push_back(std::move(result));
		}

		prox_cache[i] = std::move(indexResults);
	}
}

void Automorph::GetRawResultDiff(const std::string& shapeName, const std::string& sliderName, std::unordered_map<uint16_t, Vector3>& outDiff) {
	std::string setName = ResultDataName(shapeName, sliderName);
	if (!resultDiffData.TargetMatch(setName, shapeName))
		return;

	outDiff = *resultDiffData.GetDiffSet(setName);
}

int Automorph::GetResultDiffSize(const std::string& shapeName, const std::string& sliderName) {
	std::string setname = ResultDataName(shapeName, sliderName);
	if (!resultDiffData.TargetMatch(setname, shapeName))
		return 0;

	return resultDiffData.GetDiffSet(setname)->size();
}

std::unordered_map<uint16_t, Vector3>* Automorph::GetDiffSet(const std::string& targetDataName) {
	return resultDiffData.GetDiffSet(targetDataName);
}

void Automorph::ScaleResultDiff(const std::string& shapeName, const std::string& sliderName, float scaleValue) {
	std::string setName = ResultDataName(shapeName, sliderName);
	resultDiffData.ScaleDiff(setName, shapeName, scaleValue);
}

void Automorph::LoadResultDiffs(SliderSet& fromSet) {
	fromSet.LoadSetDiffData(resultDiffData);
	targetSliderDataNames.clear();
	for (size_t i = 0; i < fromSet.size(); i++)
		for (auto& df : fromSet[i].dataFiles)
			if (df.dataName != (df.targetName + fromSet[i].name))
				SetResultDataName(df.targetName, fromSet[i].name, df.dataName);
}

void Automorph::MergeResultDiffs(
	SliderSet& fromSet, SliderSet& mergeSet, DiffDataSets& baseDiffData, const std::string& baseShape, const bool newDataLocal, const bool appendNewSliders) {
	fromSet.Merge(mergeSet, resultDiffData, baseDiffData, baseShape, newDataLocal, appendNewSliders);
	for (size_t i = 0; i < fromSet.size(); i++)
		for (auto& df : fromSet[i].dataFiles)
			if (df.dataName != (df.targetName + fromSet[i].name))
				SetResultDataName(df.targetName, fromSet[i].name, df.dataName);
}

void Automorph::ClearResultSet(const std::string& sliderName) {
	resultDiffData.ClearSet(sliderName);
}

void Automorph::SaveResultDiff(const std::string& shapeName, const std::string& sliderName, const std::string& fileName) {
	std::string setName = ResultDataName(shapeName, sliderName);
	resultDiffData.SaveSet(setName, shapeName, fileName);
}

void Automorph::AddEmptySet(const std::string& shapeName, const std::string& sliderName) {
	std::string setName = ResultDataName(shapeName, sliderName);

	if (!resultDiffData.TargetMatch(setName, shapeName))
		resultDiffData.AddEmptySet(setName, shapeName);
}

void Automorph::SetResultDiff(const std::string& shapeName, const std::string& sliderName, const TargetDataDiffs& diff) {
	std::string setName = ResultDataName(shapeName, sliderName);

	if (!resultDiffData.TargetMatch(setName, shapeName))
		resultDiffData.AddEmptySet(setName, shapeName);

	for (auto& i : diff)
		resultDiffData.UpdateDiff(setName, shapeName, i.first, i.second);
}

void Automorph::UpdateResultDiff(const std::string& shapeName, const std::string& sliderName, const TargetDataDiffs& diff) {
	std::string setName = ResultDataName(shapeName, sliderName);

	if (!resultDiffData.TargetMatch(setName, shapeName))
		resultDiffData.AddEmptySet(setName, shapeName);

	for (auto& i : diff) {
		Vector3 diffscale = Mesh::TransformDiffMeshToNif(i.second);
		resultDiffData.SumDiff(setName, shapeName, i.first, diffscale);
	}
}

void Automorph::UpdateRefDiff(const std::string& shapeName, const std::string& sliderName, const TargetDataDiffs& diff) {
	std::string setName = ResultDataName(shapeName, sliderName);

	if (!srcDiffData->TargetMatch(setName, shapeName))
		srcDiffData->AddEmptySet(setName, shapeName);

	for (auto& i : diff) {
		Vector3 diffscale = Mesh::TransformDiffMeshToNif(i.second);
		srcDiffData->SumDiff(setName, shapeName, i.first, diffscale);
	}
}

void Automorph::EmptyResultDiff(const std::string& shapeName, const std::string& sliderName) {
	std::string setName = ResultDataName(shapeName, sliderName);
	resultDiffData.EmptySet(setName, shapeName);
}

void Automorph::ZeroVertDiff(const std::string& shapeName, const std::string& sliderName, std::vector<uint16_t>* vertSet, std::unordered_map<uint16_t, float>* mask) {
	std::string setName = ResultDataName(shapeName, sliderName);
	resultDiffData.ZeroVertDiff(setName, shapeName, vertSet, mask);
}

void Automorph::SetResultDataName(const std::string& shapeName, const std::string& sliderName, const std::string& dataName) {
	targetSliderDataNames[shapeName + sliderName] = dataName;
}

std::string Automorph::ResultDataName(const std::string& shapeName, const std::string& sliderName) {
	std::string search = shapeName + sliderName;
	auto f = targetSliderDataNames.find(search);
	if (f == targetSliderDataNames.end())
		return search;

	return f->second;
}

void Automorph::SmoothResultDiff(const std::string& shapeName, const std::string& sliderName, const std::string& resultTargetName, int iterations, float strength) {
	if (iterations <= 0 || strength <= 0.0f)
		return;

	auto shapeItem = sourceShapes.find(shapeName);
	if (shapeItem == sourceShapes.end())
		return;

	Mesh* m = shapeItem->second;
	if (!m || m->nVerts <= 0 || m->nTris <= 0 || !m->tris)
		return;

	std::string resultTarget = resultTargetName.empty() ? shapeName : resultTargetName;
	std::string dataName = ResultDataName(resultTarget, sliderName);
	if (!resultDiffData.TargetMatch(dataName, resultTarget))
		return;

	auto resultDiffSet = resultDiffData.GetDiffSet(dataName);
	if (!resultDiffSet || resultDiffSet->empty())
		return;

	if (!m->bGotWeldVerts)
		m->CalcWeldVerts();
	if (!m->adjVerts)
		m->BuildVertexAdjacency();
	if (!m->adjVerts)
		return;

	strength = std::clamp(strength, 0.0f, 1.0f);
	std::vector<Vector3> current(m->nVerts);
	std::vector<Vector3> next(m->nVerts);
	std::vector<bool> hasValue(m->nVerts, false);
	std::vector<bool> isCandidate(m->nVerts, false);
	std::vector<int> candidates;
	candidates.reserve(resultDiffSet->size() * 4);

	auto isLockedByMask = [&](int index) {
		return m->mask && bEnableMask && m->mask[index] >= 1.0f;
	};

	auto addCandidate = [&](int index) {
		if (index < 0 || index >= m->nVerts || isCandidate[index])
			return;
		isCandidate[index] = true;
		candidates.push_back(index);
	};

	auto addCandidateWithWelds = [&](int index) {
		if (index < 0 || index >= m->nVerts)
			return;

		std::vector<int> weldSet;
		m->GetWeldSet(index, weldSet);
		for (int weldIndex : weldSet)
			addCandidate(weldIndex);
	};

	for (auto& diff : *resultDiffSet) {
		int index = diff.first;
		if (index < 0 || index >= m->nVerts)
			continue;

		current[index] = diff.second;
		hasValue[index] = true;
		addCandidateWithWelds(index);
		for (int adjIndex : m->adjVerts[index])
			addCandidateWithWelds(adjIndex);
	}

	auto syncWelds = [&](std::vector<Vector3>& values, std::vector<bool>* valueFlags) {
		for (int index : candidates) {
			if (m->LeastWeldedVertexIndex(index) != index)
				continue;

			std::vector<int> weldSet;
			m->GetWeldSet(index, weldSet);
			if (weldSet.size() <= 1)
				continue;

			Vector3 total;
			int count = 0;
			for (int weldIndex : weldSet) {
				if (weldIndex < 0 || weldIndex >= m->nVerts || isLockedByMask(weldIndex))
					continue;
				if (valueFlags && !(*valueFlags)[weldIndex])
					continue;

				total += values[weldIndex];
				count++;
			}

			if (count == 0)
				continue;

			Vector3 average = total / static_cast<float>(count);
			for (int weldIndex : weldSet) {
				if (weldIndex < 0 || weldIndex >= m->nVerts || isLockedByMask(weldIndex))
					continue;

				values[weldIndex] = average;
				if (valueFlags)
					(*valueFlags)[weldIndex] = true;
			}
		}
	};

	syncWelds(current, &hasValue);

	for (int iteration = 0; iteration < iterations; iteration++) {
		next = current;
		for (int index : candidates) {
			if (isLockedByMask(index))
				continue;

			const auto& adjVerts = m->adjVerts[index];
			if (adjVerts.empty())
				continue;

			Vector3 total;
			int count = 0;
			for (int adjIndex : adjVerts) {
				if (adjIndex < 0 || adjIndex >= m->nVerts || isLockedByMask(adjIndex))
					continue;

				total += current[adjIndex];
				count++;
			}

			if (count == 0)
				continue;

			float vertexStrength = strength;
			if (m->mask && bEnableMask)
				vertexStrength *= (1.0f - m->mask[index]);

			Vector3 average = total / static_cast<float>(count);
			next[index] = (current[index] * (1.0f - vertexStrength)) + (average * vertexStrength);
		}

		syncWelds(next, nullptr);
		current.swap(next);
	}

	resultDiffSet->clear();
	for (int index : candidates) {
		if (current[index].IsZero(true))
			continue;

		(*resultDiffSet)[static_cast<uint16_t>(index)] = current[index];
	}
}

void Automorph::GenerateResultDiff(
	const std::string& shapeName,
	const std::string& sliderName,
	const std::string& refDataName,
	bool transformResults,
	int maxResults,
	bool noSqueeze,
	bool solidMode,
	bool axisX,
	bool axisY,
	bool axisZ,
	const std::string& resultTargetName) {
	if (sourceShapes.find(shapeName) == sourceShapes.end())
		return;

	std::unordered_map<uint16_t, Vector3>* diffData = srcDiffData->GetDiffSet(refDataName);
	if (!diffData)
		return;

	Mesh* m = sourceShapes[shapeName];
	std::string resultTarget = resultTargetName.empty() ? shapeName : resultTargetName;
	std::string dataName = ResultDataName(resultTarget, sliderName);

	MatTransform transform;
	if (transformResults) {
		transform = m->xformModelToMesh.ComposeTransforms(morphRef->xformMeshToModel);
		transformResults = !transform.IsNearlyEqualTo(MatTransform());
	}

	if (resultDiffData.TargetMatch(dataName, resultTarget)) {
		if (m->mask)
			resultDiffData.ZeroVertDiff(dataName, m->nVerts, m->mask.get());
		else
			resultDiffData.ClearSet(dataName);
	}

	resultDiffData.AddEmptySet(dataName, resultTarget);
	auto resultDiffSet = resultDiffData.GetDiffSet(dataName);

	std::vector<Vector3> totalMoveList;
	std::vector<float> invDist;
	std::vector<Vector3> effectVector;

	for (int i = 0; i < m->nVerts; i++) {
		std::vector<kd_query_result<uint16_t>>* vertProx = &prox_cache[i];
		int nValues = vertProx->size();
		if (nValues > maxResults)
			nValues = maxResults;

		int nearMoves = 0;
		float invDistTotal = 0.0;

		float weight;
		Vector3 totalMove;

		invDist.assign(nValues, 0.0f);
		effectVector.assign(nValues, Vector3());

		for (int j = 0; j < nValues; j++) {
			uint16_t vi = (*vertProx)[j].vertex_index;
			const Vector3* v = (*vertProx)[j].v;
			auto diffItem = diffData->find(vi);
			if (diffItem != diffData->end()) {
				weight = (*vertProx)[j].distance; // "weight" is just a placeholder here...
				if (weight == 0.0f)
					invDist[nearMoves] = 1000.0f; // Exact match, choose big nearness weight.
				else
					invDist[nearMoves] = 1.0f / weight;

				invDistTotal += invDist[nearMoves];

				auto& effect = effectVector[nearMoves];
				if (axisX) {
					if (!noSqueeze || (noSqueeze && ((diffItem->second.x > 0.0f && v->x > 0.0f) || (diffItem->second.x < 0.0f && v->x < 0.0f))))
						effect.x = diffItem->second.x;
				}
				if (axisY)
					effect.y = diffItem->second.y;
				if (axisZ)
					effect.z = diffItem->second.z;

				nearMoves++;
			}
			else if (j == 0) {
				// Closest proximity vert has zero movement
				nearMoves = 0;
				break;
			}
		}

		if (nearMoves == 0)
			continue;

		totalMove.Zero();
		for (int j = 0; j < nearMoves; j++) {
			weight = invDist[j] / invDistTotal;
			totalMove += (effectVector[j] * weight);
		}

		if (m->mask && bEnableMask)
			totalMove *= (1.0f - m->mask[i]);

		if (totalMove.IsZero(true))
			continue;

		if (!solidMode) {
			if (transformResults)
				totalMove = transform.ApplyTransformToDiff(totalMove);
			(*resultDiffSet)[i] += totalMove;
		}
		else {
			totalMoveList.reserve(m->nVerts);
			totalMoveList.push_back(totalMove);
		}
	}

	if (solidMode && !totalMoveList.empty()) {
		Vector3 totalSolidMove;
		Vector3 totalSolidMoveNeg;

		for (const auto& mv : totalMoveList) {
			if (mv.x >= 0.0f) {
				if (mv.x > totalSolidMove.x)
					totalSolidMove.x = mv.x;
			}
			else {
				if (mv.x < totalSolidMoveNeg.x)
					totalSolidMoveNeg.x = mv.x;
			}

			if (mv.y >= 0.0f) {
				if (mv.y > totalSolidMove.y)
					totalSolidMove.y = mv.y;
			}
			else {
				if (mv.y < totalSolidMoveNeg.y)
					totalSolidMoveNeg.y = mv.y;
			}

			if (mv.z >= 0.0f) {
				if (mv.z > totalSolidMove.z)
					totalSolidMove.z = mv.z;
			}
			else {
				if (mv.z < totalSolidMoveNeg.z)
					totalSolidMoveNeg.z = mv.z;
			}
		}

		totalSolidMove += totalSolidMoveNeg;

		for (int i = 0; i < m->nVerts; i++) {
			Vector3 totalMove = totalSolidMove;

			if (m->mask && bEnableMask)
				totalMove *= (1.0f - m->mask[i]);

			if (totalMove.IsZero(true))
				continue;

			if (transformResults)
				totalMove = transform.ApplyTransformToDiff(totalMove);
			(*resultDiffSet)[i] += totalMove;
		}
	}
}
