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
	proximity_radius = 10.0f;
	max_prox_points = 5.0f;
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
	m->verts = new Vertex[m->nVerts];

	m->nTris = objTris.size();
	m->tris = new Triangle[m->nTris];

	// Load verts. No transformation is done (in contrast to the very similar code in GLSurface).
	for (int i = 0; i < m->nVerts; i++)
		m->verts[i] = objVerts[i];

	Vector3 norm;
	// Load tris. Also sum face normals here.
	for (int j = 0; j < m->nTris; j++) {
		m->tris[j].p1 = objTris[j].p1;
		m->tris[j].p2 = objTris[j].p2;
		m->tris[j].p3 = objTris[j].p3;
		// calc normal
		m->tris[j].trinormal(m->verts, &norm);
		m->verts[m->tris[j].p1].nx += norm.x;
		m->verts[m->tris[j].p1].ny += norm.y;
		m->verts[m->tris[j].p1].nz += norm.z;
		m->verts[m->tris[j].p2].nx += norm.x;
		m->verts[m->tris[j].p2].ny += norm.y;
		m->verts[m->tris[j].p2].nz += norm.z;
		m->verts[m->tris[j].p3].nx += norm.x;
		m->verts[m->tris[j].p3].ny += norm.y;
		m->verts[m->tris[j].p3].nz += norm.z;
	}

	// Normalize all vertex normals to smooth them out.
	for (int i = 0; i < m->nVerts; i++) {
		Vector3* pn = (Vector3*)&m->verts[i].nx;
		pn->Normalize();
	}

	kd_matcher matcher(m->verts, m->nVerts);
	for (int i = 0; i < matcher.matches.size(); i++) {
		Vertex* a = matcher.matches[i].first;
		Vertex* b = matcher.matches[i].second;
		float dot = (a->nx * b->nx + a->ny * b->ny + a->nz * b->nz);
		if (dot < 1.57079633f) {
			a->nx = ((a->nx + b->nx) / 2.0f);
			a->ny = ((a->ny + b->ny) / 2.0f);
			a->nz = ((a->nz + b->nz) / 2.0f);
			b->nx = a->nx;
			b->ny = a->ny;
			b->nz = a->nz;
		}
	}
}

void Automorph::MeshFromNifShape(mesh* m, NifFile& ref, const string& shapeName) {
	vector<Vector3> nifVerts;
	vector<Triangle> nifTris;
	ref.GetVertsForShape(shapeName, nifVerts);
	ref.GetTrisForShape(shapeName, &nifTris);

	const vector<Vector3>* nifNorms = ref.GetNormalsForShape(shapeName);

	m->shapeName = shapeName;

	//float c = 0.4f + (meshes.size()*0.3f);
	//m->color = Vector3(c,c,c);

	m->nVerts = nifVerts.size();
	m->verts = new Vertex[m->nVerts];

	m->nTris = nifTris.size();
	m->tris = new Triangle[m->nTris];

	// Load verts. No transformation is done (in contrast to the very similar code in GLSurface).
	for (int i = 0; i < m->nVerts; i++) {
		m->verts[i] = (nifVerts)[i];

		//m->verts[i].x = (*nif_verts)[i].x / -10.0f;
		//m->verts[i].z = (*nif_verts)[i].y / 10.0f;
		//m->verts[i].y = (*nif_verts)[i].z / 10.0f;
	}

	Vector3 norm;
	if (!nifNorms) {
		// Load tris. Also sum face normals here.
		for (int j = 0; j < m->nTris; j++) {
			m->tris[j].p1 = nifTris[j].p1;
			m->tris[j].p2 = nifTris[j].p2;
			m->tris[j].p3 = nifTris[j].p3;
			m->tris[j].trinormal(m->verts, &norm);
			m->verts[m->tris[j].p1].nx += norm.x;
			m->verts[m->tris[j].p1].ny += norm.y;
			m->verts[m->tris[j].p1].nz += norm.z;
			m->verts[m->tris[j].p2].nx += norm.x;
			m->verts[m->tris[j].p2].ny += norm.y;
			m->verts[m->tris[j].p2].nz += norm.z;
			m->verts[m->tris[j].p3].nx += norm.x;
			m->verts[m->tris[j].p3].ny += norm.y;
			m->verts[m->tris[j].p3].nz += norm.z;
		}

		// Normalize all vertex normals to smooth them out.
		Vector3* pn;
		for (int i = 0; i < m->nVerts; i++) {
			pn = (Vector3*)&m->verts[i].nx;
			pn->Normalize();
		}

		kd_matcher matcher(m->verts, m->nVerts);
		for (int i = 0; i < matcher.matches.size(); i++) {
			Vertex* a = matcher.matches[i].first;
			Vertex* b = matcher.matches[i].second;
			float dot = (a->nx * b->nx + a->ny*b->ny + a->nz*b->nz);
			if (dot < 1.57079633f) {
				a->nx = ((a->nx + b->nx) / 2.0f);
				a->ny = ((a->ny + b->ny) / 2.0f);
				a->nz = ((a->nz + b->nz) / 2.0f);
				b->nx = a->nx;
				b->ny = a->ny;
				b->nz = a->nz;
			}
		}

	}
	else {
		// Already have normals, just copy the data over.
		for (int j = 0; j < m->nTris; j++)
			m->tris[j] = nifTris[j];

		// Copy normals. No transformation is done on the normals.
		for (int i = 0; i < m->nVerts; i++) {
			m->verts[i].nx = (*nifNorms)[i].x;
			m->verts[i].nz = (*nifNorms)[i].z;
			m->verts[i].ny = (*nifNorms)[i].y;
		}
	}
}

void Automorph::BuildProximityCache(const string &shapeName) {
	mesh* m = sourceShapes[shapeName];
	totalCount = 0;
	maxCount = 0;
	minCount = 60000;
	proximity_radius = 10.0f;
	for (int i = 0; i < m->nVerts; i++) {
		Vertex* v = &m->verts[i];

		int resultCount;
		if (foreignShapes.find(shapeName) != foreignShapes.end()) {
			Vertex vtmp(v->x * -10.0f, v->z * 10.0f, v->y * 10.0f);
			resultCount = refTree->kd_nn(&vtmp, proximity_radius);
		}
		else
			resultCount = refTree->kd_nn(v, proximity_radius);

		if (resultCount < minCount)
			minCount = resultCount;
		if (resultCount > maxCount)
			maxCount = resultCount;

		totalCount += resultCount;
		vector<kd_query_result> indexResults;
		for (int j = 0; j < resultCount; j++)
			indexResults.push_back(refTree->queryResult[j] /*.vertex_index*/);
		prox_cache[i] = indexResults;
	}
	avgCount = (float)totalCount / (float)m->nVerts;
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

void Automorph::GenerateResultDiff(const string& shapeName, const string &sliderName, const string& refDataName) {
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

	ushort index = 0xFFFF;
	for (int i = 0; i < m->nVerts; i++) {
		index++;
		vector<kd_query_result>* vertProx = &prox_cache[i];
		int nValues = vertProx->size();
		if (nValues > 10)
			nValues = 10;
		int nearMoves = 0;
		double invDistTotal = 0.0f;

		double weight;
		double invDist[40];
		Vector3 effectVector[40];
		Vector3 totalMove;
		for (int j = 0; j < nValues; j++) {
			ushort vi = (*vertProx)[j].vertex_index;
			auto diffItem = diffData->find(vi);
			if (diffItem != diffData->end()) {
				weight = (*vertProx)[j].distance;	// "weight" is just a placeholder here...
				if (weight == 0)
					invDist[nearMoves] = 1000;		// Exact match, choose big nearness weight.
				else
					invDist[nearMoves] = 1 / weight;
				invDistTotal += invDist[nearMoves];
				effectVector[nearMoves] = diffItem->second;
				nearMoves++;
			}
			else if (j == 0) {						// Closest proximity vert has zero movement.
				//	nearmoves=0;
				//	break;
			}

		}
		if (nearMoves == 0)
			continue;

		totalMove.x = 0;
		totalMove.y = 0;
		totalMove.z = 0;
		for (int j = 0; j < nearMoves; j++) {
			weight = invDist[j] / invDistTotal;
			totalMove += (effectVector[j] * (float)weight);
			//totalmove.x += ( weight ) *  effectVector[j].x;
			//totalmove.y += ( weight ) *  effectVector[j].y;
			//totalmove.z += ( weight ) *  effectVector[j].z;
		}

		if (m->vcolors && bEnableMask)
			totalMove = totalMove * (1.0f - m->vcolors[i].x);
		if (totalMove.DistanceTo(Vector3(0.0f, 0.0f, 0.0f)) < EPSILON)
			continue;

		resultDiffData.UpdateDiff(shapeName + sliderName, shapeName, index, totalMove);
	}
}
