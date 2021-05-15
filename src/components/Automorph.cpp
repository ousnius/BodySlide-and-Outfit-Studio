/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "Automorph.h"
#include "Anim.h"

using namespace nifly;

Automorph::Automorph() {
}

Automorph::~Automorph() {
	ClearSourceShapes();
}

void Automorph::ClearSourceShapes() {
	for (auto &shapes : sourceShapes)
		delete shapes.second;

	sourceShapes.clear();
}

void Automorph::RenameResultDiffData(const std::string& shape, const std::string& oldName, const std::string& newName) {
	std::string setName = ResultDataName(shape, oldName);
	std::string newSetName = shape + newName;

	resultDiffData.RenameSet(setName, newSetName);
	targetSliderDataNames.erase(setName);
}

void Automorph::RenameShape(const std::string& oldShapeName, const std::string& oldTarget, const std::string& newShapeName) {
	if (sourceShapes.find(oldShapeName) != sourceShapes.end()) {
		sourceShapes[newShapeName] = sourceShapes[oldShapeName];
		sourceShapes.erase(oldShapeName);
	}

	resultDiffData.DeepRename(oldTarget, newShapeName);

	std::vector<std::string> oldKeys;
	for (auto &tsdn : targetSliderDataNames) {
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

void Automorph::CopyShape(const std::string& srcShapeName, const std::string& srcTarget, const std::string& destShapeName) {
	resultDiffData.DeepCopy(srcTarget, destShapeName);

	std::vector<std::string> newVals;
	std::vector<std::string> oldKeys;
	std::vector<std::string> newKeys;

	for (auto &tsdn : targetSliderDataNames) {
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

void Automorph::SetRef(NifFile& ref, NiShape* refShape, const AnimInfo *workAnim) {
	morphRef = std::make_unique<mesh>();
	MeshFromNifShape(morphRef.get(), ref, refShape, workAnim);

	refTree = std::make_unique<kd_tree>(morphRef->verts.get(), static_cast<uint16_t>(morphRef->nVerts));
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

void Automorph::SourceShapesFromNif(NifFile &baseNif, const AnimInfo *workAnim) {
	ClearSourceShapes();

	auto shapes = baseNif.GetShapes();
	for (auto &s : shapes) {
		mesh* m = new mesh();
		MeshFromNifShape(m, baseNif, s, workAnim);
		sourceShapes[s->name.get()] = m;
	}
}

void Automorph::UpdateMeshFromNif(NifFile &baseNif, const std::string& shapeName) {
	if (sourceShapes.find(shapeName) == sourceShapes.end())
		return;

	mesh* m = sourceShapes[shapeName];
	auto shape = baseNif.FindBlockByName<NiShape>(shapeName);

	std::vector<Vector3> upVerts;
	baseNif.GetVertsForShape(shape, upVerts);

	int numUpVerts = upVerts.size();
	if (m->nVerts != numUpVerts)
		return;

	for (int i = 0; i < m->nVerts; i++)
		m->verts[i] = upVerts[i];
}

void Automorph::CopyMeshMask(mesh* m, const std::string& shapeName) {
	mesh* dm = sourceShapes[shapeName];
	if (dm->nVerts != m->nVerts)
		return;

	if (!m->vcolors)
		return;

	if (!dm->vcolors)
		dm->vcolors = std::make_unique<Vector3[]>(dm->nVerts);

	for (int i = 0; i < dm->nVerts; i++)
		dm->vcolors[i] = m->vcolors[i];
}

void Automorph::MeshFromNifShape(mesh* m, NifFile& ref, NiShape* shape, const AnimInfo *workAnim) {
	std::vector<Vector3> nifVerts;
	ref.GetVertsForShape(shape, nifVerts);

	std::vector<Triangle> nifTris;
	shape->GetTriangles(nifTris);

	m->shapeName = shape->name.get();

	if (!shape->IsSkinned()) {
		// Calculate transform from shape's CS to global CS.
		MatTransform ttg = shape->GetTransformToParent();
		NiNode* parent = ref.GetParentNode(shape);
		while (parent) {
			ttg = parent->GetTransformToParent().ComposeTransforms(ttg);
			parent = ref.GetParentNode(parent);
		}

		// Convert ttg to a glm::mat4x4.
		auto matShape = glm::identity<glm::mat4x4>();
		matShape = glm::translate(matShape, glm::vec3(ttg.translation.x, ttg.translation.y, ttg.translation.z));
		float y, p, r;
		ttg.rotation.ToEulerAngles(y, p, r);
		matShape *= glm::yawPitchRoll(r, p, y);
		matShape = glm::scale(matShape, glm::vec3(ttg.scale, ttg.scale, ttg.scale));
		m->matModel = matShape;
	}
	else {
		// Not rendered by the game for skinned meshes
		// Keep to counter-offset bone transforms
		auto matSkin = glm::identity<glm::mat4x4>();
		const MatTransform &xFormSkin = workAnim->shapeSkinning.find(m->shapeName)->second.xformGlobalToSkin;
		float y, p, r;
		xFormSkin.rotation.ToEulerAngles(y, p, r);
		matSkin = glm::translate(matSkin, glm::vec3(xFormSkin.translation.x, xFormSkin.translation.y, xFormSkin.translation.z));
		matSkin *= glm::yawPitchRoll(r, p, y);
		matSkin = glm::scale(matSkin, glm::vec3(xFormSkin.scale, xFormSkin.scale, xFormSkin.scale));
		m->matModel = glm::inverse(matSkin);
	}


	//float c = 0.4f + (meshes.size()*0.3f);
	//m->color = Vector3(c,c,c);

	m->nVerts = nifVerts.size();
	m->verts = std::make_unique<Vector3[]>(m->nVerts);

	m->nTris = nifTris.size();
	m->tris = std::make_unique<Triangle[]>(m->nTris);

	// Load verts. No transformation is done (in contrast to the very similar code in GLSurface).
	for (int i = 0; i < m->nVerts; i++) {
		glm::vec4 vtmp = m->matModel * glm::vec4((nifVerts)[i].x, (nifVerts)[i].y, (nifVerts)[i].z, 1.0f);
		m->verts[i] = Vector3(vtmp.x, vtmp.y, vtmp.z);
	}

	// Load triangles
	for (int j = 0; j < m->nTris; j++)
		m->tris[j] = nifTris[j];
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

void Automorph::BuildProximityCache(const std::string& shapeName, const float proximityRadius) {
	if (sourceShapes.find(shapeName) == sourceShapes.end())
		return;

	mesh* m = sourceShapes[shapeName];
	uint16_t maxCount = 0;
	uint16_t minCount = std::numeric_limits<uint16_t>::max();

	for (int i = 0; i < m->nVerts; i++) {
		uint16_t resultCount = refTree->kd_nn(&m->verts[i], proximityRadius);
		if (resultCount < minCount)
			minCount = resultCount;
		if (resultCount > maxCount)
			maxCount = resultCount;

		std::vector<kd_query_result> indexResults;
		for (int id = 0; id < resultCount; id++)
			indexResults.push_back(refTree->queryResult[id]);

		prox_cache[i] = indexResults;
	}
}

void Automorph::GetRawResultDiff(const std::string& shapeName, const std::string& sliderName, std::unordered_map<uint16_t, Vector3>& outDiff) {
	std::string setName = ResultDataName(shapeName, sliderName);
	if (!resultDiffData.TargetMatch(setName, shapeName))
		return;

	outDiff.clear();

	std::unordered_map<uint16_t, Vector3>* set = resultDiffData.GetDiffSet(setName);
	for (auto &i : *set)
		outDiff[i.first] = i.second;
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
		for (auto &df : fromSet[i].dataFiles)
			if (df.dataName != (df.targetName + fromSet[i].name))
				SetResultDataName(df.targetName, fromSet[i].name, df.dataName);
}

void Automorph::MergeResultDiffs(SliderSet& fromSet, SliderSet& mergeSet, DiffDataSets& baseDiffData, const std::string& baseShape, const bool newDataLocal) {
	fromSet.Merge(mergeSet, resultDiffData, baseDiffData, baseShape, newDataLocal);
	for (size_t i = 0; i < fromSet.size(); i++)
		for (auto &df : fromSet[i].dataFiles)
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

void Automorph::SetResultDiff(const std::string& shapeName, const std::string& sliderName, std::unordered_map<uint16_t, Vector3>& diff) {
	std::string setName = ResultDataName(shapeName, sliderName);

	if (!resultDiffData.TargetMatch(setName, shapeName))
		resultDiffData.AddEmptySet(setName, shapeName);

	for (auto &i : diff)
		resultDiffData.UpdateDiff(setName, shapeName, i.first, i.second);
}

void Automorph::UpdateResultDiff(const std::string& shapeName, const std::string& sliderName, std::unordered_map<uint16_t, Vector3>& diff) {
	std::string setName = ResultDataName(shapeName, sliderName);

	if (!resultDiffData.TargetMatch(setName, shapeName))
		resultDiffData.AddEmptySet(setName, shapeName);

	for (auto &i: diff) {
		Vector3 diffscale = Vector3(i.second.x * -10, i.second.z * 10, i.second.y * 10);
		resultDiffData.SumDiff(setName, shapeName, i.first, diffscale);
	}
}

void Automorph::UpdateRefDiff(const std::string& shapeName, const std::string& sliderName, std::unordered_map<uint16_t, Vector3>& diff) {
	std::string setName = ResultDataName(shapeName, sliderName);

	if (!srcDiffData->TargetMatch(setName, shapeName))
		srcDiffData->AddEmptySet(setName, shapeName);

	for (auto &i : diff) {
		Vector3 diffscale = Vector3(i.second.x * -10, i.second.z * 10, i.second.y * 10);
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

void Automorph::GenerateResultDiff(const std::string& shapeName, const std::string &sliderName, const std::string& refDataName, int maxResults, bool noSqueeze, bool solidMode, bool axisX, bool axisY, bool axisZ) {
	if (sourceShapes.find(shapeName) == sourceShapes.end())
		return;

	std::unordered_map<uint16_t, Vector3>* diffData = srcDiffData->GetDiffSet(refDataName);
	if (!diffData)
		return;

	mesh* m = sourceShapes[shapeName];
	std::string dataName = shapeName + sliderName;

	if (resultDiffData.TargetMatch(dataName, shapeName)) {
		if (m->vcolors)
			resultDiffData.ZeroVertDiff(dataName, m->vcolors.get());
		else
			resultDiffData.ClearSet(dataName);
	}

	resultDiffData.AddEmptySet(dataName, shapeName);
	auto resultDiffSet = resultDiffData.GetDiffSet(dataName);

	std::vector<Vector3> totalMoveList;
	std::vector<float> invDist;
	std::vector<Vector3> effectVector;

	for (int i = 0; i < m->nVerts; i++) {
		std::vector<kd_query_result>* vertProx = &prox_cache[i];
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
				weight = (*vertProx)[j].distance;	// "weight" is just a placeholder here...
				if (weight == 0.0f)
					invDist[nearMoves] = 1000.0f;	// Exact match, choose big nearness weight.
				else
					invDist[nearMoves] = 1.0f / weight;

				invDistTotal += invDist[nearMoves];

				auto& effect = effectVector[nearMoves];
				if (axisX) {
					if (!noSqueeze ||
						(noSqueeze && ((diffItem->second.x > 0.0f && v->x > 0.0f) || (diffItem->second.x < 0.0f && v->x < 0.0f))))
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

		if (m->vcolors && bEnableMask)
			totalMove *= (1.0f - m->vcolors[i].x);

		if (totalMove.IsZero(true))
			continue;

		if (!solidMode) {
			(*resultDiffSet)[i] = totalMove;
		}
		else {
			totalMoveList.reserve(m->nVerts);
			totalMoveList.push_back(totalMove);
		}
	}

	if (solidMode && !totalMoveList.empty()) {
		Vector3 totalSolidMove;
		Vector3 totalSolidMoveNeg;

		for (const auto &mv : totalMoveList) {
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

			if (m->vcolors && bEnableMask)
				totalMove *= (1.0f - m->vcolors[i].x);

			if (totalMove.IsZero(true))
				continue;

			(*resultDiffSet)[i] = totalMove;
		}
	}
}
