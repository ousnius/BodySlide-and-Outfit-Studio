/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "Automorph.h"

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

	std::vector<std::string> newVals;
	std::vector<std::string> oldKeys;
	std::vector<std::string> newKeys;
	for (auto &tsdn : targetSliderDataNames) {
		std::string newDN = tsdn.second;
		std::string newKey = tsdn.first;
		bool found = false;
		size_t p = newDN.find(oldShapeName);
		if (p == 0) {
			newDN = newDN.substr(oldShapeName.length());
			newDN = newShapeName + newDN;
			found = true;
		}
		p = newKey.find(oldShapeName);
		if (p == 0) {
			newKey = newKey.substr(oldShapeName.length());
			newKey = newShapeName + newKey;
			found = true;
		}
		if (found) {
			oldKeys.push_back(tsdn.first);
			newKeys.push_back(newKey);
			newVals.push_back(newDN);
		}
	}
	for (int i = 0; i < oldKeys.size(); i++) {
		targetSliderDataNames.erase(oldKeys[i]);
		targetSliderDataNames[newKeys[i]] = newVals[i];
	}
}

void Automorph::SetRef(NifFile& ref, NiShape* refShape) {
	morphRef = std::make_unique<mesh>();
	MeshFromNifShape(morphRef.get(), ref, refShape);

	refTree = std::make_unique<kd_tree>(morphRef->verts.get(), morphRef->nVerts);
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

void Automorph::SourceShapesFromNif(NifFile &baseNif) {
	ClearSourceShapes();

	auto shapes = baseNif.GetShapes();
	for (auto &s : shapes) {
		mesh* m = new mesh();
		MeshFromNifShape(m, baseNif, s);
		sourceShapes[s->GetName()] = m;
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

void Automorph::MeshFromNifShape(mesh* m, NifFile& ref, NiShape* shape) {
	std::vector<Vector3> nifVerts;
	ref.GetVertsForShape(shape, nifVerts);

	std::vector<Triangle> nifTris;
	shape->GetTriangles(nifTris);

	m->shapeName = shape->GetName();

	float y, p, r;
	auto matParents = glm::identity<glm::mat4x4>();
	auto matShape = glm::identity<glm::mat4x4>();
	auto matSkin = glm::identity<glm::mat4x4>();

	if (!shape->IsSkinned()) {
		NiNode* parent = ref.GetParentNode(shape);
		while (parent) {
			parent->transform.ToEulerDegrees(y, p, r);
			matParents = glm::translate(matParents, glm::vec3(parent->transform.translation.x, parent->transform.translation.y, parent->transform.translation.z));
			matParents *= glm::yawPitchRoll(r * DEG2RAD, p * DEG2RAD, y * DEG2RAD);
			matParents = glm::scale(matParents, glm::vec3(parent->transform.scale, parent->transform.scale, parent->transform.scale));
			parent = ref.GetParentNode(parent);
		}

		matShape = glm::translate(matShape, glm::vec3(shape->transform.translation.x, shape->transform.translation.y, shape->transform.translation.z));
		shape->transform.ToEulerDegrees(y, p, r);
		matShape *= glm::yawPitchRoll(r * DEG2RAD, p * DEG2RAD, y * DEG2RAD);
		matShape = glm::scale(matShape, glm::vec3(shape->transform.scale, shape->transform.scale, shape->transform.scale));
	}

	m->matModel = matParents * matShape * glm::inverse(matSkin);

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

void Automorph::DeleteVerts(const std::string& shapeName, const std::vector<ushort>& indices) {
	resultDiffData.DeleteVerts(shapeName, indices);
}

void Automorph::ClearProximityCache() {
	prox_cache.clear();
}

void Automorph::BuildProximityCache(const std::string& shapeName, const float proximityRadius) {
	mesh* m = sourceShapes[shapeName];
	int maxCount = 0;
	int minCount = 60000;

	for (int i = 0; i < m->nVerts; i++) {
		int resultCount = refTree->kd_nn(&m->verts[i], proximityRadius);
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

void Automorph::GetRawResultDiff(const std::string& shapeName, const std::string& sliderName, std::unordered_map<ushort, Vector3>& outDiff) {
	std::string setName = ResultDataName(shapeName, sliderName);
	if (!resultDiffData.TargetMatch(setName, shapeName))
		return;

	outDiff.clear();

	std::unordered_map<ushort, Vector3>* set = resultDiffData.GetDiffSet(setName);
	for (auto &i : *set)
		outDiff[i.first] = i.second;
}

int Automorph::GetResultDiffSize(const std::string& shapeName, const std::string& sliderName) {
	std::string setname = ResultDataName(shapeName, sliderName);
	if (!resultDiffData.TargetMatch(setname, shapeName))
		return 0;

	return resultDiffData.GetDiffSet(setname)->size();
}

void Automorph::ScaleResultDiff(const std::string& shapeName, const std::string& sliderName, float scaleValue) {
	std::string setName = ResultDataName(shapeName, sliderName);
	resultDiffData.ScaleDiff(setName, shapeName, scaleValue);
}

void Automorph::LoadResultDiffs(SliderSet& fromSet) {
	fromSet.LoadSetDiffData(resultDiffData);
	targetSliderDataNames.clear();
	for (int i = 0; i < fromSet.size(); i++)
		for (auto &df : fromSet[i].dataFiles)
			if (df.dataName != (df.targetName + fromSet[i].name))
				SetResultDataName(df.targetName, fromSet[i].name, df.dataName);
}

void Automorph::MergeResultDiffs(SliderSet& fromSet, SliderSet& mergeSet, DiffDataSets& baseDiffData, const std::string& baseShape) {
	fromSet.Merge(mergeSet, resultDiffData, baseDiffData, baseShape);
	for (int i = 0; i < fromSet.size(); i++)
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

void Automorph::SetResultDiff(const std::string& shapeName, const std::string& sliderName, std::unordered_map<ushort, Vector3>& diff) {
	std::string setName = ResultDataName(shapeName, sliderName);

	if (!resultDiffData.TargetMatch(setName, shapeName))
		resultDiffData.AddEmptySet(setName, shapeName);

	for (auto &i : diff)
		resultDiffData.SumDiff(setName, shapeName, i.first, i.second);
}

void Automorph::UpdateResultDiff(const std::string& shapeName, const std::string& sliderName, std::unordered_map<ushort, Vector3>& diff) {
	std::string setName = ResultDataName(shapeName, sliderName);

	if (!resultDiffData.TargetMatch(setName, shapeName))
		resultDiffData.AddEmptySet(setName, shapeName);

	for (auto &i: diff) {
		Vector3 diffscale = Vector3(i.second.x * -10, i.second.z * 10, i.second.y * 10);
		resultDiffData.SumDiff(setName, shapeName, i.first, diffscale);
	}
}

void Automorph::UpdateRefDiff(const std::string& shapeName, const std::string& sliderName, std::unordered_map<ushort, Vector3>& diff) {
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

void Automorph::ZeroVertDiff(const std::string& shapeName, const std::string& sliderName, std::vector<ushort>* vertSet, std::unordered_map<ushort, float>* mask) {
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

void Automorph::GenerateResultDiff(const std::string& shapeName, const std::string &sliderName, const std::string& refDataName, const int maxResults) {
	std::unordered_map<ushort, Vector3>* diffData = srcDiffData->GetDiffSet(refDataName);
	if (!diffData)
		return;

	mesh* m = sourceShapes[shapeName];
	if (resultDiffData.TargetMatch(shapeName + sliderName, shapeName)) {
		if (m->vcolors)
			resultDiffData.ZeroVertDiff(shapeName + sliderName, m->vcolors.get());
		else
			resultDiffData.ClearSet(shapeName + sliderName);
	}

	resultDiffData.AddEmptySet(shapeName + sliderName, shapeName);

	for (int i = 0; i < m->nVerts; i++) {
		std::vector<kd_query_result>* vertProx = &prox_cache[i];
		int nValues = vertProx->size();
		if (nValues > maxResults)
			nValues = maxResults;

		int nearMoves = 0;
		double invDistTotal = 0.0;

		double weight;
		Vector3 totalMove;
		std::vector<double> invDist(nValues);
		std::vector<Vector3> effectVector(nValues);
		for (int j = 0; j < nValues; j++) {
			ushort vi = (*vertProx)[j].vertex_index;
			auto diffItem = diffData->find(vi);
			if (diffItem != diffData->end()) {
				weight = (*vertProx)[j].distance;	// "weight" is just a placeholder here...
				if (weight == 0.0)
					invDist[nearMoves] = 1000.0;	// Exact match, choose big nearness weight.
				else
					invDist[nearMoves] = 1.0 / weight;

				invDistTotal += invDist[nearMoves];
				effectVector[nearMoves] = diffItem->second;
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
			totalMove += (effectVector[j] * (float)weight);
		}

		if (m->vcolors && bEnableMask)
			totalMove = totalMove * (1.0f - m->vcolors[i].x);

		if (totalMove.DistanceTo(Vector3(0.0f, 0.0f, 0.0f)) < EPSILON)
			continue;

		resultDiffData.UpdateDiff(shapeName + sliderName, shapeName, i, totalMove);
	}
}
