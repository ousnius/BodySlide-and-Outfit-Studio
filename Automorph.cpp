#include "Automorph.h"

Automorph::Automorph(void)
{
	morphRef = NULL;
	refTree = NULL;
	srcDiffData = NULL;
	bEnableMask = true;
	proximity_radius = 10.0f;
	max_prox_points = 5.0f;
}

Automorph::~Automorph(void)
{
	if (morphRef) {
		delete morphRef;
		morphRef = 0;
	}
	if (refTree) {
		delete refTree;
		refTree = 0;
	}
	ClearSourceShapes();
}

Automorph::Automorph(NifFile &ref, const string& refShape) {
	morphRef = NULL;
	refTree = NULL;
	srcDiffData = NULL;
	bEnableMask = true;
	proximity_radius = 10.0f;
	max_prox_points = 5.0f;
	SetRef(ref, refShape);
}

void Automorph::ClearSourceShapes() {
	map <string,mesh*>::iterator shapes;
	for (shapes = sourceShapes.begin(); shapes != sourceShapes.end(); ++shapes) {
		if (foreignShapes.find(shapes->first) != foreignShapes.end()) {
			continue;
		}
		delete shapes->second;
	}
	sourceShapes.clear();
	foreignShapes.clear();
}

void Automorph::RenameResultDiffData(const string& shape, const string& oldName, const string& newName) {
	string setname = ResultDataName(shape, oldName);
	string newsetname = shape + newName;

	resultDiffData.RenameSet(setname, newsetname);
	targetSliderDataNames.erase(setname);
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

	string newDN;
	string newKey;
	vector<string> newvals;
	vector<string> oldkeys;
	vector<string> newkeys;
	bool found;
	for (auto &tsdn: targetSliderDataNames) {
		newDN = tsdn.second;
		newKey = tsdn.first;
		found = false;
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
			oldkeys.push_back(tsdn.first);
			newkeys.push_back(newKey);
			newvals.push_back(newDN);
		}
	}
	for (int i = 0; i < oldkeys.size(); i++) {
		targetSliderDataNames.erase(oldkeys[i]);
		targetSliderDataNames[newkeys[i]] = newvals[i];
	}
}

void Automorph::SetRef(NifFile& ref, const string& refShape) {
	if (morphRef)
		delete morphRef;

	morphRef = new mesh();
	MeshFromNifShape(morphRef, ref, refShape);

	if (refTree) {
		delete refTree;
	}

	refTree = new kd_tree(morphRef->verts, morphRef->nVerts);
}

int Automorph::InitRefDiffData(const string &srcFileName, const string &dataSetName, const string &baseDataPath) {
	SliderSetFile srcSliderSetFile;
	SliderSet srcSet;
	
	srcSliderSetFile.Open(srcFileName);
	
	if (srcSliderSetFile.GetSet(dataSetName, srcSet))
		return 1;
	
	srcSet.SetBaseDataPath(baseDataPath);
	
	srcSet.LoadSetDiffData(__srcDiffData);
	srcDiffData = &__srcDiffData;

	return 0;
}

void Automorph::LinkRefDiffData(DiffDataSets* diffData) {
	srcDiffData = diffData;
}

void Automorph::UnlinkRefDiffData() {
	srcDiffData = &__srcDiffData;
}

void Automorph::ApplyDiffToVerts(const string& sliderName, const string& shapeTargetName, vector<vector3>* inOutResult, float strength) {
	srcDiffData->ApplyDiff(sliderName, shapeTargetName, strength, inOutResult);
}

void Automorph::ApplyResultToVerts(const string& sliderName, const string& shapeTargetName, vector<vector3>* inOutResult, float strength) {
	string setname = ResultDataName(shapeTargetName, sliderName);
	
	resultDiffData.ApplyDiff(setname, shapeTargetName, strength, inOutResult);
}

void Automorph::SourceShapesFromNif(NifFile &baseNif) {
	mesh* m;
	vector<string> shapes;
	ClearSourceShapes();
	baseNif.GetShapeList(shapes);
	for (int i = 0; i < shapes.size(); i++) {
		m = new mesh();
		MeshFromNifShape(m, baseNif,shapes[i]);
		sourceShapes[shapes[i]] = m;
	}	
}

void Automorph::SourceShapesFromObj(ObjFile &baseObj) {
	mesh* m;
	vector<string> shapes;
	ClearSourceShapes();
	baseObj.GetGroupList(shapes);
	for (int i = 0; i < shapes.size(); i++) {
		m = new mesh();
		MeshFromObjShape(m,baseObj,shapes[i]);
		sourceShapes[shapes[i]] = m;
	}	
}

void Automorph::UpdateMeshFromNif(NifFile &baseNif, const string& shapeName) {
	if (sourceShapes.find(shapeName) == sourceShapes.end()) 
		return;
	mesh* m = sourceShapes[shapeName];
	vector<vec3> upverts;
	baseNif.GetVertsForShape(shapeName, upverts);
	
	if (m->nVerts != upverts.size())
		return;

	for (int i = 0; i < m->nVerts; i++) {
		m->verts[i] = upverts[i];
	}
}

void Automorph::CopyMeshMask(mesh* m, const string& shapeName) {
	mesh* dm = sourceShapes[shapeName];
	if (dm->nVerts != m->nVerts)
		return;

	if (!m->vcolors)
		return;

	if (!dm->vcolors)
		dm->vcolors = new vec3[dm->nVerts];

	for (int i = 0; i < dm->nVerts; i++) {
		dm->vcolors[i] = m->vcolors[i];
	}
}	

void Automorph::LinkSourceShapeMesh(mesh* m, const string& shapeName) {
	sourceShapes[shapeName] = m;
	foreignShapes[shapeName] = m;
}

void Automorph::MeshFromObjShape(mesh *m, ObjFile &ref, const string& shapeName) {
	m->shapeName = shapeName;
	vector<vec3> obj_verts;
	vector<tri> obj_tris; 
	ref.CopyDataForGroup(shapeName, &obj_verts, &obj_tris, NULL);
	int i, j;

	//float c = 0.4f + (meshes.size()*0.3f);
	//m->color = vec3(c,c,c);

	m->nVerts =  obj_verts.size();
	m->verts = new vtx[m->nVerts];

	m->nTris = obj_tris.size();
	m->tris = new tri[m->nTris];

	// load verts. No transformation is done (in contrast to the very similar code in GLSurface)
	for(i =0 ;i< m->nVerts; i++ ){
		m->verts[i] = obj_verts[i];
	}

	vec3 norm;
	// load tris.  Also sum face normals here
	for (j = 0; j < m->nTris; j++) {
		m->tris[j].p1 = obj_tris[j].p1;
		m->tris[j].p2 = obj_tris[j].p2;
		m->tris[j].p3 = obj_tris[j].p3;
		// calc normal
		m->tris[j].trinormal(m->verts,&norm);
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
	// normalize all vertex normals to smooth them out.
	vec3* pn;
	for (i = 0; i < m->nVerts; i++) {
		pn = (vec3*)&m->verts[i].nx;
		pn->Normalize();
	}
	
	kd_matcher matcher(m->verts, m->nVerts);
	for (i = 0; i < matcher.matches.size(); i++) {
		vtx* a = matcher.matches[i].first;
		vtx* b = matcher.matches[i].second;
		float dot = (a->nx * b->nx + a->ny*b->ny + a->nz*b->nz);
		if (dot < 1.57079633f) {
			a->nx=((a->nx + b->nx) /2.0f);
			a->ny=((a->ny + b->ny) /2.0f);
			a->nz=((a->nz + b->nz) /2.0f);
			b->nx = a->nx;
			b->ny = a->ny;
			b->nz = a->nz;
		}
	}
}

void Automorph::MeshFromNifShape(mesh *m, NifFile &ref, const string& shapeName) {
	int i, j;
	vector<vec3> nif_verts;
	ref.GetVertsForShape(shapeName, nif_verts);
	//const vector<vec3>* nif_verts = ref.GetVertsForShape(shapeName);
	const vector<triangle>* nif_tris = ref.GetTrisForShape(shapeName);
	vector<triangle> localtris;
	if(nif_tris == NULL) {  // perhaps a tristrip
		bool found = ref.GetTrisForTriStripShape(shapeName, &localtris);
		if(found) 
			nif_tris = &localtris;
	}

	const vector<vec3>* nif_norms = ref.GetNormalsForShape(shapeName);

	m->shapeName = shapeName;

	//float c = 0.4f + (meshes.size()*0.3f);
	//m->color = vec3(c,c,c);

	m->nVerts =  nif_verts.size();
	m->verts = new vtx[m->nVerts];

	m->nTris = nif_tris->size();
	m->tris = new tri[m->nTris];

	// load verts. No transformation is done (in contrast to the very similar code in GLSurface)
	for(i =0 ;i< m->nVerts; i++ ){
		m->verts[i] = (nif_verts)[i];
		
//		m->verts[i].x = (*nif_verts)[i].x / -10.0f;
//		m->verts[i].z = (*nif_verts)[i].y / 10.0f;
//		m->verts[i].y = (*nif_verts)[i].z / 10.0f;
	}

	vec3 norm;
	if (!nif_norms) {
		// load tris.  Also sum face normals here
		for (j = 0; j < m->nTris; j++) {
			m->tris[j].p1 = (*nif_tris)[j].p1;
			m->tris[j].p2 = (*nif_tris)[j].p2;
			m->tris[j].p3 = (*nif_tris)[j].p3;
			// calc normal
			m->tris[j].trinormal(m->verts,&norm);
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
		// normalize all vertex normals to smooth them out.
		vec3* pn;
		for (i = 0; i < m->nVerts; i++) {
			pn = (vec3*)&m->verts[i].nx;
			pn->Normalize();
		}
		
		kd_matcher matcher(m->verts, m->nVerts);
		for (i = 0; i < matcher.matches.size(); i++) {
			vtx* a = matcher.matches[i].first;
			vtx* b = matcher.matches[i].second;
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

	} else {
		// alread have normals, just copy the data over.
		for (j = 0; j < m->nTris; j++) {
			m->tris[j] = (*nif_tris)[j];
		}	
		// copy normals.  No transformation is done on the normals.
		for (i = 0; i < m->nVerts; i++) {
			m->verts[i].nx = (*nif_norms)[i].x;			
			m->verts[i].nz = (*nif_norms)[i].z;			
			m->verts[i].ny = (*nif_norms)[i].y;
		}
	}
}

void Automorph::BuildProximityCache(const string &shapeName) {
	mesh* m = sourceShapes[shapeName];
	vtx* v;
	int resultCount;
	totalCount = 0;
	maxCount = 0;
	minCount = 60000;
	//vector<int> index_results;
	vector<kd_query_result> index_results;
	proximity_radius = 10.0f;
	for (int i = 0; i < m->nVerts; i++) {
		v = &m->verts[i];

		if (foreignShapes.find(shapeName) != foreignShapes.end()) {
			vtx vtmp(v->x * -10.0f,
					 v->z * 10.0f, 
					 v->y * 10.0f);		
			resultCount = refTree->kd_nn(&vtmp, proximity_radius);
		} else {			
			resultCount = refTree->kd_nn(v, proximity_radius);
		}

		if (resultCount < minCount) minCount = resultCount;
		if (resultCount > maxCount) maxCount = resultCount;		
		totalCount += resultCount;
		index_results.clear();
//		if (resultCount > 5) resultCount = 5;
		for (int j = 0; j < resultCount; j++) {
			index_results.push_back(refTree->queryResult[j]/*.vertex_index*/);
		}
		prox_cache[i] = index_results;
	}
	avgCount = (float)totalCount / (float)m->nVerts;
}

void Automorph::GetRawResultDiff(const string& shapeName, const string& sliderName, unordered_map<int,vec3>& outDiff) {
	string setname = ResultDataName(shapeName, sliderName);
	vec3 diffscale;
	if (!resultDiffData.TargetMatch(setname, shapeName)) {
		return;
	}
	
	outDiff.clear();
	
	unordered_map<int, vec3>* set = resultDiffData.GetDiffSet(setname);
	for (auto i: (*set)) {	
		outDiff[i.first] = i.second;
	}
}

int  Automorph::GetResultDiffSize(const string& shapeName, const string& sliderName) {
	string setname = ResultDataName(shapeName, sliderName);
	vec3 diffscale;
	if (!resultDiffData.TargetMatch(setname, shapeName)) {
		return 0;
	}
	return resultDiffData.GetDiffSet(setname)->size();
}

void Automorph::ScaleResultDiff(const string& shapeName,const string& sliderName, float scaleValue) {
	string setname = ResultDataName(shapeName, sliderName);
	resultDiffData.ScaleDiff(setname, shapeName, scaleValue);
}

void Automorph::LoadResultDiffs(SliderSet &fromSet) {
	fromSet.LoadSetDiffData(resultDiffData);
	targetSliderDataNames.clear();
	for (int i = 0; i < fromSet.size(); i++) {
		for (auto df: fromSet[i].dataFiles) {
			if (df.dataName != (df.targetName + fromSet[i].Name)) {
				SetResultDataName(df.targetName, fromSet[i].Name, df.dataName);
			}
		}
	}
}

void Automorph::ClearResultSet(const string& sliderName) {
	resultDiffData.ClearSet(sliderName);
}

void Automorph::SaveResultDiff(const string& shapeName, const string& sliderName, const string& fileName) {
	string setname = ResultDataName(shapeName, sliderName);
	resultDiffData.SaveSet(setname, shapeName, fileName);
}

void Automorph::SetResultDiff(const string& shapeName, const string& sliderName, unordered_map<int,vec3>& diff) {
	string setname = ResultDataName(shapeName, sliderName);
	vec3 diffscale;
	if (!resultDiffData.TargetMatch(setname, shapeName)) {
		resultDiffData.AddEmptySet(setname, shapeName);
	}
	for (auto i: diff) {	
		resultDiffData.SumDiff(setname, shapeName, i.first, i.second);
	}
}

void Automorph::UpdateResultDiff(const string& shapeName, const string& sliderName, unordered_map<int,vec3>& diff) {
	string setname = ResultDataName(shapeName, sliderName);
	vec3 diffscale;
	if (!resultDiffData.TargetMatch(setname, shapeName)) {
		resultDiffData.AddEmptySet(setname, shapeName);
	}
	for (auto i: diff) {	
		diffscale = vec3(i.second.x * -10, i.second.z * 10, i.second.y * 10);
		resultDiffData.SumDiff(setname, shapeName, i.first, diffscale);
	}
}

void Automorph::UpdateRefDiff(const string& shapeName, const string& sliderName, unordered_map<int,vec3>& diff) {
	string setname = ResultDataName(shapeName, sliderName);
	vec3 diffscale;
	if (!srcDiffData->TargetMatch(setname, shapeName)) {
		srcDiffData->AddEmptySet(setname, shapeName);
	}
	for (auto i: diff) {	
		diffscale = vec3(i.second.x * -10, i.second.z * 10, i.second.y * 10);
		srcDiffData->SumDiff(setname, shapeName, i.first, diffscale);
	}
}

void Automorph::EmptyResultDiff(const string& shapeName, const string& sliderName) {
	string setname = ResultDataName(shapeName, sliderName);
	resultDiffData.EmptySet(setname, shapeName);
}

void Automorph::ZeroVertDiff(const string& shapeName, const string& sliderName, vector<int>* vertSet, unordered_map<int,float>* mask) {
	string setname = ResultDataName(shapeName, sliderName);
	resultDiffData.ZeroVertDiff(setname, shapeName, vertSet, mask);
}

void Automorph::SetResultDataName(const string& shapeName, const string& sliderName, const string& dataName) {
	targetSliderDataNames[shapeName + sliderName] = dataName;
}

string Automorph::ResultDataName(const string& shapeName, const string& sliderName) {
	string srch = shapeName + sliderName;
	auto f = targetSliderDataNames.find(srch);
	if (f == targetSliderDataNames.end()) {
		return srch;
	} 
	return f->second;
}

void Automorph::GenerateResultDiff(const string& shapeName, const string &sliderName, const string& refDataName) {
	MAPTYPE<int, vector3>* diffData = srcDiffData->GetDiffSet(refDataName);
	if (diffData == NULL) {
		return;
	}

	MAPTYPE<int, vector3>::iterator diffItem;

	mesh* m = sourceShapes[shapeName];
	vector<kd_query_result>* vertProx;
	int vi;
	double weight;
	double invDist[40];
	vector3 effectVector[40];
	vector3 totalmove;
	int nValues;
	int nearmoves;
	double invDistTotal;
	
	if (resultDiffData.TargetMatch(shapeName + sliderName, shapeName)) {
		if (m->vcolors) {
			resultDiffData.ZeroVertDiff(shapeName + sliderName, shapeName, m->vcolors);
		} else {
			resultDiffData.ClearSet(shapeName + sliderName);
		}
	}
	
	resultDiffData.AddEmptySet(shapeName + sliderName, shapeName);

	for (int i = 0; i < m->nVerts; i++) {
		vertProx = &prox_cache[i];
		nValues = vertProx->size();
		if (nValues > 10)
			nValues = 10;
		//if (i == 814) 
		//	DebugBreak();
		nearmoves = 0;
		invDistTotal = 0.0f;

		for (int j = 0; j < nValues; j++) {
			vi = (*vertProx)[j].vertex_index;
			diffItem = diffData->find(vi);
			if (diffItem != diffData->end()) {
				weight = (*vertProx)[j].distance;  // "weight" is just a placeholder here...
				if (weight == 0) {
					invDist[nearmoves] = 1000;		// exact match, choose big nearness weight.
				} else {
				 	invDist[nearmoves] = 1 / weight;				
				}
				invDistTotal += invDist[nearmoves];
				effectVector[nearmoves] = diffItem->second;
				nearmoves++;
			}	else if (j == 0) {					// closest proximity vert has zero movement
			//	nearmoves=0;
			//	break;
			}

		}
		if (nearmoves == 0) 
			continue;

		totalmove.x = 0;
		totalmove.y = 0;
		totalmove.z = 0;
		for (int j = 0; j < nearmoves; j++) {
			weight = invDist[j] / invDistTotal;
			totalmove += (effectVector[j] * (float)weight);
			/*
			totalmove.x += ( weight ) *  effectVector[j].x;
			totalmove.y += ( weight ) *  effectVector[j].y;
			totalmove.z += ( weight ) *  effectVector[j].z;*/
		}
		if (m->vcolors && bEnableMask) {
			totalmove = totalmove * (1.0f - m->vcolors[i].x);
		}
		if (totalmove.DistanceTo(vec3(0.0f, 0.0f, 0.0f)) < EPSILON)
			continue;

		resultDiffData.UpdateDiff(shapeName + sliderName, shapeName, i, totalmove);
	}
}
