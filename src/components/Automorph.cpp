/*
BodySlide and Outfit Studio
Copyright (C) 2016  Caliente & ousnius
See the included LICENSE file
*/

#include "Automorph.h"

Automorph::Automorph() {
	morphRef = nullptr;
	refTree = nullptr;
	srcDiffData = nullptr;
	bEnableMask = true;
}

Automorph::~Automorph() {
	if (morphRef) {
		delete morphRef;
		morphRef = nullptr;
	}
	if (refTree) {
		delete refTree;
		refTree = nullptr;
	}
	ClearSourceShapes();
}

void Automorph::ClearSourceShapes() {
	for (auto &shapes : sourceShapes) {
		if (foreignShapes.find(shapes.first) != foreignShapes.end())
			continue;

		delete shapes.second;
	}
	sourceShapes.clear();
	foreignShapes.clear();
}

void Automorph::RenameResultDiffData(const string& shape, const string& oldName, const string& newName) {
	string setName = ResultDataName(shape, oldName);
	string newSetName = shape + newName;

	resultDiffData.RenameSet(setName, newSetName);
	targetSliderDataNames.erase(setName);
}

void Automorph::RenameShape(const string& shapeName, const string& newShapeName) {
	if (sourceShapes.find(shapeName) != sourceShapes.end()){
		sourceShapes[newShapeName] = sourceShapes[shapeName];
		sourceShapes.erase(shapeName);
	}
	if (foreignShapes.find(shapeName) != foreignShapes.end()){
		foreignShapes[newShapeName] = foreignShapes[shapeName];
		foreignShapes.erase(shapeName);
	}
	resultDiffData.DeepRename(shapeName, newShapeName);

	vector<string> newVals;
	vector<string> oldKeys;
	vector<string> newKeys;
	for (auto &tsdn : targetSliderDataNames) {
		string newDN = tsdn.second;
		string newKey = tsdn.first;
		bool found = false;
		size_t p = newDN.find(shapeName);
		if (p == 0) {
			newDN = newDN.substr(shapeName.length());
			newDN = newShapeName + newDN;
			found = true;
		}
		p = newKey.find(shapeName);
		if (p == 0) {
			newKey = newKey.substr(shapeName.length());
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

void Automorph::SetRef(NifFile& ref, const string& refShape) {
	if (morphRef)
		delete morphRef;

	morphRef = new mesh();
	MeshFromNifShape(morphRef, ref, refShape);

	if (refTree)
		delete refTree;

	refTree = new kd_tree(morphRef->verts, morphRef->nVerts);
}

void Automorph::LinkRefDiffData(DiffDataSets* diffData) {
	srcDiffData = diffData;
}

void Automorph::UnlinkRefDiffData() {
	srcDiffData = &__srcDiffData;
}

void Automorph::ApplyDiffToVerts(const string& sliderName, const string& shapeTargetName, vector<Vector3>* inOutResult, float strength) {
	srcDiffData->ApplyDiff(sliderName, shapeTargetName, strength, inOutResult);
}

void Automorph::ApplyResultToVerts(const string& sliderName, const string& shapeTargetName, vector<Vector3>* inOutResult, float strength) {
	string setname = ResultDataName(shapeTargetName, sliderName);

	resultDiffData.ApplyDiff(setname, shapeTargetName, strength, inOutResult);
}

void Automorph::ApplyResultToUVs(const string& sliderName, const string& shapeTargetName, vector<Vector2>* inOutResult, float strength) {
	string setname = ResultDataName(shapeTargetName, sliderName);

	resultDiffData.ApplyUVDiff(setname, shapeTargetName, strength, inOutResult);
}

void Automorph::SourceShapesFromNif(NifFile &baseNif) {
	ClearSourceShapes();
	vector<string> shapes;
	baseNif.GetShapeList(shapes);
	for (int i = 0; i < shapes.size(); i++) {
		mesh* m = new mesh();
		MeshFromNifShape(m, baseNif, shapes[i]);
		sourceShapes[shapes[i]] = m;
	}
}

void Automorph::SourceShapesFromObj(ObjFile &baseObj) {
	ClearSourceShapes();
	vector<string> shapes;
	baseObj.GetGroupList(shapes);
	for (int i = 0; i < shapes.size(); i++) {
		mesh* m = new mesh();
		MeshFromObjShape(m, baseObj, shapes[i]);
		sourceShapes[shapes[i]] = m;
	}
}

void Automorph::UpdateMeshFromNif(NifFile &baseNif, const string& shapeName) {
	if (sourceShapes.find(shapeName) == sourceShapes.end())
		return;

	mesh* m = sourceShapes[shapeName];
	vector<Vector3> upVerts;
	baseNif.GetVertsForShape(shapeName, upVerts);

	int numUpVerts = upVerts.size();
	if (m->nVerts != numUpVerts)
		return;

	for (int i = 0; i < m->nVerts; i++)
		m->verts[i] = upVerts[i];
}

void Automorph::CopyMeshMask(mesh* m, const string& shapeName) {
	mesh* dm = sourceShapes[shapeName];
	if (dm->nVerts != m->nVerts)
		return;

	if (!m->vcolors)
		return;

	if (!dm->vcolors)
		dm->vcolors = new Vector3[dm->nVerts];

	for (int i = 0; i < dm->nVerts; i++)
		dm->vcolors[i] = m->vcolors[i];
}

void Automorph::LinkSourceShapeMesh(mesh* m, const string& shapeName) {
	sourceShapes[shapeName] = m;
	foreignShapes[shapeName] = m;
}

void Automorph::MeshFromObjShape(mesh* m, ObjFile& ref, const string& shapeName) {
	m->shapeName = shapeName;
	vector<Vector3> objVerts;
	vector<Triangle> objTris;
	ref.CopyDataForGroup(shapeName, &objVerts, &objTris, nullptr);

	//float c = 0.4f + (meshes.size() * 0.3f);
	//m->color = Vector3(c, c, c);

	m->nVerts = objVerts.size();
	m->verts = new Vector3[m->nVerts];

	m->nTris = objTris.size();
	m->tris = new Triangle[m->nTris];

	// Load verts. No transformation is done (in contrast to the very similar code in GLSurface).
	for (int i = 0; i < m->nVerts; i++)
		m->verts[i] = objVerts[i];

	// Load triangles
	for (int j = 0; j < m->nTris; j++) {
		m->tris[j].p1 = objTris[j].p1;
		m->tris[j].p2 = objTris[j].p2;
		m->tris[j].p3 = objTris[j].p3;
	}
}

void Automorph::MeshFromNifShape(mesh* m, NifFile& ref, const string& shapeName) {
	vector<Vector3> nifVerts;
	vector<Triangle> nifTris;
	ref.GetVertsForShape(shapeName, nifVerts);
	ref.GetTrisForShape(shapeName, &nifTris);

	m->shapeName = shapeName;

	//float c = 0.4f + (meshes.size()*0.3f);
	//m->color = Vector3(c,c,c);

	m->nVerts = nifVerts.size();
	m->verts = new Vector3[m->nVerts];

	m->nTris = nifTris.size();
	m->tris = new Triangle[m->nTris];

	// Load verts. No transformation is done (in contrast to the very similar code in GLSurface).
	for (int i = 0; i < m->nVerts; i++)
		m->verts[i] = (nifVerts)[i];

	// Load triangles
	for (int j = 0; j < m->nTris; j++)
		m->tris[j] = nifTris[j];
}

void Automorph::ClearProximityCache() {
	prox_cache.clear();
}

void Automorph::BuildProximityCache(const string &shapeName, const float& proximityRadius) {
	mesh* m = sourceShapes[shapeName];
	int maxCount = 0;
	int minCount = 60000;

	for (int i = 0; i < m->nVerts; i++) {
		int resultCount;
		if (foreignShapes.find(shapeName) != foreignShapes.end()) {
			Vector3 vtmp(m->verts[i].x * -10.0f, m->verts[i].z * 10.0f, m->verts[i].y * 10.0f);
			resultCount = refTree->kd_nn(&vtmp, proximityRadius);
		}
		else
			resultCount = refTree->kd_nn(&m->verts[i], proximityRadius);

		if (resultCount < minCount)
			minCount = resultCount;
		if (resultCount > maxCount)
			maxCount = resultCount;

		vector<kd_query_result> indexResults;
		for (int id = 0; id < resultCount; id++)
			indexResults.push_back(refTree->queryResult[id]);

		prox_cache[i] = indexResults;
	}
}

void Automorph::GetRawResultDiff(const string& shapeName, const string& sliderName, unordered_map<ushort, Vector3>& outDiff) {
	string setName = ResultDataName(shapeName, sliderName);
	if (!resultDiffData.TargetMatch(setName, shapeName))
		return;

	outDiff.clear();

	unordered_map<ushort, Vector3>* set = resultDiffData.GetDiffSet(setName);
	for (auto &i : *set)
		outDiff[i.first] = i.second;
}

int  Automorph::GetResultDiffSize(const string& shapeName, const string& sliderName) {
	string setname = ResultDataName(shapeName, sliderName);
	if (!resultDiffData.TargetMatch(setname, shapeName))
		return 0;

	return resultDiffData.GetDiffSet(setname)->size();
}

void Automorph::ScaleResultDiff(const string& shapeName, const string& sliderName, float scaleValue) {
	string setName = ResultDataName(shapeName, sliderName);
	resultDiffData.ScaleDiff(setName, shapeName, scaleValue);
}

void Automorph::LoadResultDiffs(SliderSet &fromSet) {
	fromSet.LoadSetDiffData(resultDiffData);
	targetSliderDataNames.clear();
	for (int i = 0; i < fromSet.size(); i++)
		for (auto &df : fromSet[i].dataFiles)
			if (df.dataName != (df.targetName + fromSet[i].name))
				SetResultDataName(df.targetName, fromSet[i].name, df.dataName);
}

void Automorph::ClearResultSet(const string& sliderName) {
	resultDiffData.ClearSet(sliderName);
}

void Automorph::SaveResultDiff(const string& shapeName, const string& sliderName, const string& fileName) {
	string setName = ResultDataName(shapeName, sliderName);
	resultDiffData.SaveSet(setName, shapeName, fileName);
}

void Automorph::SetResultDiff(const string& shapeName, const string& sliderName, unordered_map<ushort, Vector3>& diff) {
	string setName = ResultDataName(shapeName, sliderName);

	if (!resultDiffData.TargetMatch(setName, shapeName))
		resultDiffData.AddEmptySet(setName, shapeName);

	for (auto &i : diff)
		resultDiffData.SumDiff(setName, shapeName, i.first, i.second);
}

void Automorph::UpdateResultDiff(const string& shapeName, const string& sliderName, unordered_map<ushort, Vector3>& diff) {
	string setName = ResultDataName(shapeName, sliderName);

	if (!resultDiffData.TargetMatch(setName, shapeName))
		resultDiffData.AddEmptySet(setName, shapeName);

	for (auto &i: diff) {
		Vector3 diffscale = Vector3(i.second.x * -10, i.second.z * 10, i.second.y * 10);
		resultDiffData.SumDiff(setName, shapeName, i.first, diffscale);
	}
}

void Automorph::UpdateRefDiff(const string& shapeName, const string& sliderName, unordered_map<ushort, Vector3>& diff) {
	string setName = ResultDataName(shapeName, sliderName);

	if (!srcDiffData->TargetMatch(setName, shapeName))
		srcDiffData->AddEmptySet(setName, shapeName);

	for (auto &i : diff) {
		Vector3 diffscale = Vector3(i.second.x * -10, i.second.z * 10, i.second.y * 10);
		srcDiffData->SumDiff(setName, shapeName, i.first, diffscale);
	}
}

void Automorph::EmptyResultDiff(const string& shapeName, const string& sliderName) {
	string setName = ResultDataName(shapeName, sliderName);
	resultDiffData.EmptySet(setName, shapeName);
}

void Automorph::ZeroVertDiff(const string& shapeName, const string& sliderName, vector<ushort>* vertSet, unordered_map<ushort, float>* mask) {
	string setName = ResultDataName(shapeName, sliderName);
	resultDiffData.ZeroVertDiff(setName, shapeName, vertSet, mask);
}

void Automorph::SetResultDataName(const string& shapeName, const string& sliderName, const string& dataName) {
	targetSliderDataNames[shapeName + sliderName] = dataName;
}

string Automorph::ResultDataName(const string& shapeName, const string& sliderName) {
	string search = shapeName + sliderName;
	auto f = targetSliderDataNames.find(search);
	if (f == targetSliderDataNames.end())
		return search;

	return f->second;
}

void Automorph::GenerateResultDiff(const string& shapeName, const string &sliderName, const string& refDataName, const int& maxResults) {
	unordered_map<ushort, Vector3>* diffData = srcDiffData->GetDiffSet(refDataName);
	if (!diffData)
		return;

	mesh* m = sourceShapes[shapeName];
	if (resultDiffData.TargetMatch(shapeName + sliderName, shapeName)) {
		if (m->vcolors)
			resultDiffData.ZeroVertDiff(shapeName + sliderName, m->vcolors);
		else
			resultDiffData.ClearSet(shapeName + sliderName);
	}

	resultDiffData.AddEmptySet(shapeName + sliderName, shapeName);

	for (int i = 0; i < m->nVerts; i++) {
		vector<kd_query_result>* vertProx = &prox_cache[i];
		int nValues = vertProx->size();
		if (nValues > maxResults)
			nValues = maxResults;

		int nearMoves = 0;
		double invDistTotal = 0.0;

		double weight;
		Vector3 totalMove;
		vector<double> invDist(nValues);
		vector<Vector3> effectVector(nValues);
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
