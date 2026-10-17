/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "Mesh.h"
#include "KDMatcher.hpp"
#include "NifFile.hpp"

#include <wx/log.h>

using namespace nifly;

Mesh::Mesh() {}

Mesh::~Mesh() {
	if (releaseBuffers)
		releaseBuffers(*this);
}

std::unique_ptr<Mesh> Mesh::CreateFromShape(NifFile* nif, const std::string& shapeName) {
	auto shape = nif->FindBlockByName<NiShape>(shapeName);
	if (!shape)
		return nullptr;

	std::vector<Vector3> nifVerts;
	nif->GetVertsForShape(shape, nifVerts);

	std::vector<Triangle> nifTris;
	shape->GetTriangles(nifTris);

	const std::vector<Vector3>* nifNorms = nif->GetNormalsForShape(shape);
	const std::vector<Vector2>* nifUvs = nif->GetUvsForShape(shape);

	auto m = std::make_unique<Mesh>();

	if (!shape->IsSkinned()) {
		// Calculate transform from shape's CS to global CS
		MatTransform ttg = shape->GetTransformToParent();
		NiNode* parent = nif->GetParentNode(shape);
		while (parent) {
			ttg = parent->GetTransformToParent().ComposeTransforms(ttg);
			parent = nif->GetParentNode(parent);
		}

		m->SetXformMeshToModel(Mesh::xformNifToMesh.ComposeTransforms(ttg.ComposeTransforms(Mesh::xformMeshToNif)));
	}
	else {
		// Skinned meshes
		MatTransform xformGlobalToSkin;
		if (nif->CalcShapeTransformGlobalToSkin(shape, xformGlobalToSkin)) {
			m->SetXformModelToMesh(Mesh::xformNifToMesh.ComposeTransforms(xformGlobalToSkin.ComposeTransforms(Mesh::xformMeshToNif)));
		}
	}

	NiShader* shader = nif->GetShader(shape);
	m->hasShader = shader != nullptr;
	m->bHelperShape = !shader || (shape->flags & 1) != 0;
	if (shader) {
		m->doublesided = shader->IsDoubleSided();
		m->modelSpace = shader->IsModelSpace();
		m->emissive = shader->IsEmissive();
		m->specular = shader->HasSpecular();
		m->vertexColors = shader->HasVertexColors();
		m->vertexAlpha = shader->HasVertexAlpha();
		m->glowmap = shader->HasGlowmap();
		m->greyscaleColor = shader->HasGreyscaleColor();
		m->cubemap = shader->HasEnvironmentMapping();

		if (nif->GetHeader().GetVersion().Stream() < 130) {
			m->backlight = shader->HasBacklight();
			m->backlightMap = m->backlight; // Dedicated map pre-130
			m->rimlight = shader->HasRimlight();
			m->softlight = shader->HasSoftlight();
			m->prop.rimlightPower = shader->GetRimlightPower();
			m->prop.softlighting = shader->GetSoftlight();
		}
		else {
			m->backlight = (shader->GetBacklightPower() > 0.0);
			m->softlight = (shader->GetSubsurfaceRolloff() > 0.0);
			m->prop.subsurfaceRolloff = shader->GetSubsurfaceRolloff();
		}

		m->prop.uvOffset = shader->GetUVOffset();
		m->prop.uvScale = shader->GetUVScale();
		m->prop.specularColor = shader->GetSpecularColor();
		m->prop.specularStrength = shader->GetSpecularStrength();
		m->prop.shininess = shader->GetGlossiness();
		m->prop.envReflection = shader->GetEnvironmentMapScale();
		m->prop.backlightPower = shader->GetBacklightPower();
		m->prop.paletteScale = shader->GetGrayscaleToPaletteScale();
		m->prop.fresnelPower = shader->GetFresnelPower();

		Color4 emissiveColor = shader->GetEmissiveColor();
		m->prop.emissiveColor = Vector3(emissiveColor.r, emissiveColor.g, emissiveColor.b);
		m->prop.emissiveMultiple = shader->GetEmissiveMultiple();

		m->prop.alpha = shader->GetAlpha();

		// Everything that has to come off the lighting shader property itself rather than off the
		// NiShader interface: the tint colors of the shader types that have one, and the True PBR flag.
		if (!nif->GetHeader().GetVersion().IsSF()) {
			auto* bslsp = dynamic_cast<BSLightingShaderProperty*>(shader);
			if (bslsp) {
				// Community Shaders "True PBR", which a shape asks for with Shader Flags 2 bit 23 - the bit
				// NifSkope shows as "Unused 01". Only asked of Skyrim: Fallout 4 shares the block type and
				// spends that bit on something else, and its shapes have their own shader either way.
				// The multi layer parallax shader type counts alongside the default one, since True PBR
				// borrows it purely as somewhere to keep its coat, fuzz and glint parameters. Such a shape
				// still holds its RMAOS map in slot 5, and reading that as one is far closer than falling
				// back to vanilla, which would take the same map for an environment mask.
				if (nif->GetHeader().GetVersion().Stream() < 130) {
					const uint32_t bslspType = bslsp->GetShaderType();
					m->pbr = (bslsp->shaderFlags2 & SLSF2_UNUSED01) != 0
							 && (bslspType == BSLightingShaderPropertyShaderType::BSLSP_DEFAULT
								 || bslspType == BSLightingShaderPropertyShaderType::BSLSP_MULTILAYERPARALLAX);

					if (m->pbr)
						wxLogMessage("Shape '%s' was detected as a True PBR material.", shapeName);
				}

				// Face tint map (FaceGen) of the face tint shader type, always in texture slot 6
				m->faceTint = bslsp->GetShaderType() == BSLightingShaderPropertyShaderType::BSLSP_FACE;

				if (bslsp->GetShaderType() == BSLightingShaderPropertyShaderType::BSLSP_SKINTINT) {
					m->tintType = Mesh::TintType::Skin;
					m->prop.tintColor = bslsp->skinTintColor;
				}
				else if (bslsp->GetShaderType() == BSLightingShaderPropertyShaderType::BSLSP_HAIRTINT) {
					m->tintType = Mesh::TintType::Hair;
					m->prop.tintColor = bslsp->hairTintColor;
				}
			}
		}
	}

	NiMaterialProperty* material = nif->GetMaterialProperty(shape);
	if (material) {
		m->emissive = material->IsEmissive();

		m->prop.specularColor = material->GetSpecularColor();
		m->prop.shininess = material->GetGlossiness();

		Color4 emissiveColor = material->GetEmissiveColor();
		m->prop.emissiveColor = Vector3(emissiveColor.r, emissiveColor.g, emissiveColor.b);
		m->prop.emissiveMultiple = material->GetEmissiveMultiple();

		m->prop.alpha = material->GetAlpha();
	}

	NiStencilProperty* stencil = nif->GetStencilProperty(shape);
	if (stencil) {
		int drawMode = (stencil->flags & DRAW_MASK) >> DRAW_POS;

		switch (drawMode) {
			case DRAW_CW:
				m->doublesided = false;
				m->cullMode = GL_FRONT;
				break;
			case DRAW_BOTH:
				m->doublesided = true;
				m->cullMode = GL_BACK;
				break;
			case DRAW_CCW:
			default:
				m->doublesided = false;
				m->cullMode = GL_BACK;
				break;
		}
	}

	NiAlphaProperty* alphaProp = nif->GetAlphaProperty(shape);
	if (alphaProp) {
		m->alphaFlags = alphaProp->flags;
		m->alphaThreshold = alphaProp->threshold;
	}

	m->nVerts = nifVerts.size();
	m->nTris = nifTris.size();

	if (m->nVerts > 0) {
		m->verts = std::make_unique<Vector3[]>(m->nVerts);
		m->norms = std::make_unique<Vector3[]>(m->nVerts);
		m->tangents = std::make_unique<Vector3[]>(m->nVerts);
		m->bitangents = std::make_unique<Vector3[]>(m->nVerts);
		m->vcolors = std::make_unique<Vector3[]>(m->nVerts);
		m->valpha = std::make_unique<float[]>(m->nVerts);
		m->texcoord = std::make_unique<Vector2[]>(m->nVerts);
		m->mask = std::make_unique<float[]>(m->nVerts);
		m->weight = std::make_unique<float[]>(m->nVerts);
	}

	if (m->nTris > 0)
		m->tris = std::make_unique<Triangle[]>(m->nTris);

	m->shapeName = shapeName;

	// Load verts. NIF verts are scaled up by approx. 10 and rotated on the x axis (Z up, Y forward).
	// Scale down by 10 and rotate on x axis by flipping y and z components. To face the camera, this also mirrors
	// on X and Y (180 degree y axis rotation.)
	for (int i = 0; i < m->nVerts; i++)
		m->verts[i] = Mesh::TransformPosNifToMesh(nifVerts[i]);

	if (nifUvs && !nifUvs->empty()) {
		for (int i = 0; i < m->nVerts; i++) {
			m->texcoord[i].u = (*nifUvs)[i].u;
			m->texcoord[i].v = (*nifUvs)[i].v;
		}
		m->textured = true;
	}

	// A Starfield material can map a layer or blender with the second UV channel instead
	if (nif->GetHeader().GetVersion().IsSF()) {
		NiGeometryData* geomData = nif->GetGeometryData(shape);
		if (geomData && geomData->uvSets.size() > 1 && geomData->uvSets[1].size() == static_cast<size_t>(m->nVerts)) {
			m->texcoord2 = std::make_unique<Vector2[]>(m->nVerts);
			std::copy(geomData->uvSets[1].begin(), geomData->uvSets[1].end(), m->texcoord2.get());
		}
	}

	// Copy triangles
	for (int t = 0; t < m->nTris; t++)
		m->tris[t] = nifTris[t];

	if (!nifNorms || nifNorms->empty()) {
		// Calc weldVerts and normals
		m->SmoothNormals();
	}
	else {
		// Already have normals, just copy the data over.
		// Copy normals. Note, normals are transformed the same way the vertices are.
		for (int i = 0; i < m->nVerts; i++)
			m->norms[i] = Mesh::TransformDirNifToMesh((*nifNorms)[i]);

		for (auto& extraDataRef : shape->extraDataRefs) {
			auto integersExtraData = nif->GetHeader().GetBlock<NiIntegersExtraData>(extraDataRef);
			if (integersExtraData && integersExtraData->name == "LOCKEDNORM")
				for (auto& i : integersExtraData->integersData)
					m->lockedNormalIndices.insert(i);
		}

		// Virtually weld verts across UV seams
		m->CalcWeldVerts();
	}

	std::vector<Color4> vColors;
	if (nif->GetColorsForShape(shape, vColors)) {
		for (size_t v = 0; v < vColors.size(); v++) {
			m->vcolors[v].x = vColors.at(v).r;
			m->vcolors[v].y = vColors.at(v).g;
			m->vcolors[v].z = vColors.at(v).b;
			m->valpha[v] = vColors.at(v).a;
		}
	}

	m->CalcTangentSpace();
	m->CreateBVH();

	return m;
}

std::shared_ptr<AABBTree> Mesh::CreateBVH() {
	if (verts && tris && nTris > 0)
		bvh = std::make_shared<AABBTree>(verts.get(), tris.get(), nTris, 100, 2);
	else
		bvh.reset();

	return bvh;
}

bool Mesh::IsFacetVisible(int facet) const {
	if (subMeshesVisible.empty() || triSubMeshes.empty())
		return true;

	if (facet < 0 || static_cast<size_t>(facet) >= triSubMeshes.size())
		return true;

	int subMeshIndex = triSubMeshes[facet];
	if (subMeshIndex < 0 || static_cast<size_t>(subMeshIndex) >= subMeshesVisible.size())
		return true;

	return subMeshesVisible[subMeshIndex];
}

float Mesh::ComputeAvgEdgeLength() {
	if (avgEdgeLengthWS >= 0.0f)
		return avgEdgeLengthWS;

	if (!verts || !tris || nTris <= 0 || nVerts <= 0) {
		avgEdgeLengthWS = 0.0f;
		return avgEdgeLengthWS;
	}

	// Mesh-space scale of matModel for converting mesh-space edge length to world space.
	// Outfit Studio uses uniform scaling on shapes, so a single axis is representative.
	const float modelScale = std::sqrt(matModel[0][0] * matModel[0][0] + matModel[0][1] * matModel[0][1] + matModel[0][2] * matModel[0][2]);

	double sum = 0.0;
	uint64_t count = 0;
	for (int i = 0; i < nTris; ++i) {
		const Triangle& t = tris[i];
		if (t.p1 >= static_cast<uint32_t>(nVerts) || t.p2 >= static_cast<uint32_t>(nVerts) || t.p3 >= static_cast<uint32_t>(nVerts))
			continue;
		sum += (verts[t.p1] - verts[t.p2]).length();
		sum += (verts[t.p2] - verts[t.p3]).length();
		sum += (verts[t.p3] - verts[t.p1]).length();
		count += 3;
	}

	avgEdgeLengthWS = (count > 0) ? static_cast<float>(sum / static_cast<double>(count)) * modelScale : 0.0f;
	return avgEdgeLengthWS;
}

void Mesh::BuildVertexAdjacency() {
	if (!tris)
		return;

	std::vector<std::set<int>> adjArray(nVerts);
	std::vector<int> p1ws, p2ws;

	auto AddWeldSetToAdjacenciesOfWeldSet = [&](int p1, int p2) {
		GetWeldSet(p1, p1ws);
		GetWeldSet(p2, p2ws);
		for (int p1w : p1ws)
		for (int p2w : p2ws) {
			adjArray[p1w].insert(p2w);
			adjArray[p2w].insert(p1w);
		}
	};
	
	for (int t = 0; t < nTris; t++) {
		auto& tri = tris[t];

		AddWeldSetToAdjacenciesOfWeldSet(tri.p1, tri.p2);
		AddWeldSetToAdjacenciesOfWeldSet(tri.p2, tri.p3);
		AddWeldSetToAdjacenciesOfWeldSet(tri.p3, tri.p1);
	}

	adjVerts = std::make_unique<std::vector<int>[]>(nVerts);
	auto av = adjVerts.get();

	for (int v = 0; v < nVerts; v++) {
		av[v].resize(adjArray[v].size());
		std::copy(adjArray[v].begin(), adjArray[v].end(), av[v].begin());
	}
}

void Mesh::MakeEdges() {
	if (!tris)
		return;
	if (edges)
		return;

	nEdges = nTris * 3;
	edges = std::make_unique<Edge[]>(nEdges);

	for (int i = 0; i < nEdges; i++) {
		// Find correct points for edge
		uint16_t* points = &tris[i / 3].p1;
		int pA = i % 3;
		int pB = i % 3 + 1;
		if (pB >= 3)
			pB = 0;

		// Create edge from points
		edges[i] = Edge(std::min(points[pA], points[pB]), std::max(points[pA], points[pB]));
	}
}

void Mesh::BuildEdgeList() {
	if (!edges)
		MakeEdges();

	vertEdges = std::make_unique<std::vector<int>[]>(nVerts);
	auto ve = vertEdges.get();
	for (int e = 0; e < nEdges; e++) {
		uint16_t i1 = edges[e].p1;
		uint16_t i2 = edges[e].p2;
		if (i1 >= nVerts || i2 >= nVerts)
			continue;

		ve[edges[e].p1].push_back(e);
		ve[edges[e].p2].push_back(e);
	}
}

void Mesh::QueueUpdate(const UpdateType& type) {
	queueUpdate[type] = true;
}

void Mesh::UpdateFromMaterialFile(const MaterialFile& matFile) {
	doublesided = matFile.twoSided;
	modelSpace = matFile.modelSpaceNormals;
	emissive = matFile.emitEnabled;
	specular = matFile.specularEnabled;
	backlight = matFile.backLighting;
	rimlight = matFile.rimLighting;
	softlight = matFile.subsurfaceLighting;
	glowmap = matFile.glowMap;
	greyscaleColor = matFile.grayscaleToPaletteColor;
	cubemap = matFile.environmentMapping;

	// Skin and hair tint of the material file override the ones of the shader property.
	// Material files have no skin tint color of their own, so the shader property color is kept for it.
	if (matFile.hair) {
		tintType = TintType::Hair;
		prop.tintColor = matFile.hairTintColor;
	}
	else if (matFile.skinTint) {
		if (tintType != TintType::Skin)
			prop.tintColor = Vector3(1.0f, 1.0f, 1.0f);

		tintType = TintType::Skin;
	}
	else {
		tintType = TintType::None;
	}

	prop.alpha = matFile.alpha;
	prop.uvOffset = matFile.uvOffset;
	prop.uvScale = matFile.uvScale;
	prop.specularColor = matFile.specularColor;
	prop.specularStrength = matFile.specularMult;
	prop.shininess = matFile.smoothness;
	prop.emissiveColor = matFile.emittanceColor;
	prop.emissiveMultiple = matFile.emittanceMult;
	prop.envReflection = matFile.environmentMappingMaskScale;
	prop.backlightPower = matFile.backLightPower;
	prop.rimlightPower = matFile.rimPower;
	prop.subsurfaceRolloff = matFile.subsurfaceLightingRolloff;
	prop.fresnelPower = matFile.fresnelPower;
	prop.paletteScale = matFile.grayscaleToPaletteScale;
}

bool Mesh::HasTintColor() const {
	if (tintType == TintType::None)
		return false;

	return prop.tintColor.x != 0.0f || prop.tintColor.y != 0.0f || prop.tintColor.z != 0.0f;
}

bool Mesh::HasAlphaBlend() {
	bool alphaBlend = alphaFlags & 1;
	if (prop.alpha < 1.0f)
		alphaBlend = true;

	return alphaBlend;
}

void Mesh::ScaleVertices(const Vector3& center, const float& factor) {
	for (int i = 0; i < nVerts; i++)
		verts[i] = center + (verts[i] - center) * factor;

	CreateBVH();
	queueUpdate[UpdateType::Position] = true;
}

void Mesh::GetAdjacentPoints(int querypoint, std::unordered_set<int>& outPoints) {
	if (querypoint >= nVerts)
		return;

	if (!adjVerts)
		BuildVertexAdjacency();

	if (!adjVerts)
		return;

	auto av = adjVerts.get();
	const auto& avPoint = av[querypoint];

	outPoints.insert(avPoint.begin(), avPoint.end());
}

int Mesh::GetAdjacentPoints(int querypoint, int outPoints[], int maxPoints) {
	if (querypoint >= nVerts)
		return 0;

	if (!adjVerts)
		BuildVertexAdjacency();

	if (!adjVerts)
		return 0;

	auto av = adjVerts.get();
	const auto& avPoint = av[querypoint];

	int n = 0;
	for (auto& p : avPoint) {
		if (n == maxPoints)
			break;

		outPoints[n++] = p;
	}

	return n;
	/* TODO: sort by distance */
}

int Mesh::GetAdjacentUnvisitedPoints(int querypoint, int outPoints[], int maxPoints, bool* visPoint) const {
	int n = 0;
	for (int p : adjVerts[querypoint])
		if (n + 1 < maxPoints && !visPoint[p]) {
			outPoints[n++] = p;
			visPoint[p] = true;
		}
	return n;
	/* TODO: sort by distance */
}

int Mesh::FindAdjacentBalancedPairs(int pt, int pairs[]) const {
	// If adjPts contains any welded points, this algorithm should pick
	// out just one balanced pair from among the welds.

	const std::vector<int>& adjPts = adjVerts[pt];
	int c = std::min(static_cast<int>(adjPts.size()), MaxAdjacentPoints);
	if (c == 0)
		return 0;

	// Calculate direction from pt to each adjPts
	Vector3 pDirs[MaxAdjacentPoints];
	for (int i = 0; i < c; ++i) {
		Vector3 pDir = verts[adjPts[i]] - verts[pt];
		pDir.Normalize();
		pDirs[i] = pDir;
	}

	// For each point in adjPts, find which of the other neighboring points
	// has the lowest direction dot product with it.  Ideally, we want -1
	// for each matching pair.
	int matchInd[MaxAdjacentPoints];
	float matchDot[MaxAdjacentPoints];
	for (int i = 0; i < c; ++i) {
		int bestInd = i;
		float bestDot = 1;
		for (int j = 0; j < c; ++j) {
			float dot = pDirs[j].dot(pDirs[i]);
			if (dot < bestDot) {
				bestDot = dot;
				bestInd = j;
			}
		}
		matchInd[i] = bestInd;
		matchDot[i] = bestDot;
	}

	// Repeatedly pick out the best pairs.  If a point's best match is to
	// a point that's already been paired, we won't match it to anything.
	bool donePt[MaxAdjacentPoints];
	for (int i = 0; i < c; ++i)
		donePt[i] = false;
	int pairInd = 0;
	bool gotOne = false;
	do {
		gotOne = false;
		float bestDot = 1;
		int bestInd = 0;
		for (int i = 0; i < c; ++i)
			// Avoid pairing a point with itself: require matchInd[i] != i
			if (!donePt[i] && matchInd[i] != i && !donePt[matchInd[i]] && matchDot[i] < bestDot) {
				bestDot = matchDot[i];
				bestInd = i;
				gotOne = true;
			}
		if (gotOne) {
			pairs[pairInd++] = adjPts[bestInd];
			pairs[pairInd++] = adjPts[matchInd[bestInd]];
			donePt[bestInd] = true;
			donePt[matchInd[bestInd]] = true;
		}
	} while (gotOne);

	return pairInd;
}

int Mesh::FindOpposingPoint(int p1, int p2, float maxdot) const {
	const std::vector<int>& adjPts = adjVerts[p1];
	int c = static_cast<int>(adjPts.size());
    if (c == 0)
        return -1;

    Vector3 p2dir = verts[p2] - verts[p1];
    p2dir.Normalize();

    float bestdot = maxdot;
    int bestpt = -1;
    for (int i = 0; i < c; ++i) {
        Vector3 dir = verts[adjPts[i]] - verts[p1];
        dir.Normalize();
        float dot = dir.dot(p2dir);
        if (dot < bestdot) {
            bestdot = dot;
            bestpt = adjPts[i];
        }
    }

    return bestpt;
}

void Mesh::CalcWeldVerts() {
	weldVerts.clear();

	SortingMatcher matcher(verts.get(), static_cast<uint16_t>(nVerts));
	for (const auto& matchset : matcher.matches) {
		for (size_t j = 0; j < matchset.size(); ++j) {
			std::vector<int>& wv = weldVerts[matchset[j]];
			for (size_t k = 0; k < matchset.size(); ++k) {
				if (j != k)
					wv.push_back(matchset[k]);
			}
		}
	}
	bGotWeldVerts = true;
}

void Mesh::SmoothNormals(const std::unordered_set<int>& vertices) {
	if (lockNormals || !norms)
		return;

	// Copy old normals into a working temporary
	std::vector<Vector3> tnorms(norms.get(), norms.get() + nVerts);

	// Zero old normals
	bool noVertices = vertices.empty();
	for (int i = 0; i < nVerts; i++) {
		if (!noVertices && vertices.count(i) == 0)
			continue;

		if (lockedNormalIndices.count(i) != 0)
			continue;

		tnorms[i].Zero();
	}

	// Face normals
	for (int t = 0; t < nTris; t++) {
		Triangle& tri = tris[t];
		bool bn1 = (noVertices || vertices.count(tri.p1) != 0);
		bool bn2 = (noVertices || vertices.count(tri.p2) != 0);
		bool bn3 = (noVertices || vertices.count(tri.p3) != 0);

		// None of the three normals should change
		if (!bn1 && !bn2 && !bn3)
			continue;

		Vector3 tn = tri.trinormal(verts.get());

		if (bn1 && lockedNormalIndices.count(tri.p1) == 0)
			tnorms[tri.p1] += tn;

		if (bn2 && lockedNormalIndices.count(tri.p2) == 0)
			tnorms[tri.p2] += tn;

		if (bn3 && lockedNormalIndices.count(tri.p3) == 0)
			tnorms[tri.p3] += tn;
	}

	for (int i = 0; i < nVerts; i++) {
		if (lockedNormalIndices.count(i) != 0)
			continue;

		tnorms[i].Normalize();
	}

	// Smooth welded vertex normals
	if (smoothSeamNormals) {
		if (!bGotWeldVerts)
			CalcWeldVerts();

		float smoothThresh = smoothSeamNormalsAngle * DEG2RAD;
		std::vector<std::pair<int, Vector3>> seamNorms;

		for (auto& wvp : weldVerts) {
			auto& key = wvp.first;
			if (!noVertices && vertices.count(key) == 0)
				continue;

			if (lockedNormalIndices.count(key) != 0)
				continue;

			const Vector3& n = tnorms[key];
			Vector3 sn = n;
			auto& value = wvp.second;
			for (int wvi : value)
				if (n.angle(tnorms[wvi]) < smoothThresh)
					sn += tnorms[wvi];

			sn.Normalize();
			seamNorms.emplace_back(key, sn);
		}

		for (auto& snp : seamNorms)
			tnorms[snp.first] = snp.second;
	}

	// Copy temporary back into the main.
	std::copy(tnorms.begin(), tnorms.end(), norms.get());

	queueUpdate[UpdateType::Normals] = true;
	CalcTangentSpace();
}

void Mesh::FacetNormals() {
	if (lockNormals)
		return;

	// Zero old normals
	for (int i = 0; i < nVerts; i++) {
		if (lockedNormalIndices.find(i) != lockedNormalIndices.end())
			continue;

		Vector3& pn = norms[i];
		pn.Zero();
	}

	for (int t = 0; t < nTris; t++) {
		auto& tri = tris[t];
		if (tri.p1 >= nVerts || tri.p2 >= nVerts || tri.p3 >= nVerts)
			continue;

		Vector3 tn = tri.trinormal(verts.get());

		if (lockedNormalIndices.find(tri.p1) == lockedNormalIndices.end()) {
			Vector3& pn1 = norms[tri.p1];
			pn1 += tn;
		}

		if (lockedNormalIndices.find(tri.p2) == lockedNormalIndices.end()) {
			Vector3& pn2 = norms[tri.p2];
			pn2 += tn;
		}

		if (lockedNormalIndices.find(tri.p3) == lockedNormalIndices.end()) {
			Vector3& pn3 = norms[tri.p3];
			pn3 += tn;
		}
	}

	for (int i = 0; i < nVerts; i++) {
		if (lockedNormalIndices.find(i) != lockedNormalIndices.end())
			continue;

		Vector3& pn = norms[i];
		pn.Normalize();
	}

	queueUpdate[UpdateType::Normals] = true;
	CalcTangentSpace();
}

void Mesh::ColorFill(const Vector3& vcolor) {
	if (!vcolors)
		return;

	for (int i = 0; i < nVerts; i++)
		vcolors[i] = vcolor;

	queueUpdate[UpdateType::VertexColors] = true;
}

void Mesh::AlphaFill(float alpha) {
	if (!valpha)
		return;

	for (int i = 0; i < nVerts; i++)
		valpha[i] = alpha;

	queueUpdate[UpdateType::VertexAlpha] = true;
}

void Mesh::MaskFill(float maskValue) {
	if (!mask)
		return;

	for (int i = 0; i < nVerts; i++)
		mask[i] = maskValue;

	queueUpdate[UpdateType::Mask] = true;
}

void Mesh::WeightFill(float weightValue) {
	if (!weight)
		return;

	for (int i = 0; i < nVerts; i++)
		weight[i] = weightValue;

	queueUpdate[UpdateType::Weight] = true;
}

void Mesh::ColorChannelFill(int channel, float value) {
	if (!vcolors)
		return;

	for (int i = 0; i < nVerts; i++) {
		if (channel == 0)
			vcolors[i].x = value;
		else if (channel == 1)
			vcolors[i].y = value;
		else if (channel == 2)
			vcolors[i].z = value;
	}

	queueUpdate[UpdateType::VertexColors] = true;
}

void Mesh::CalcTangentSpace() {
	if (!norms || !texcoord)
		return;

	for (int i = 0; i < nTris; i++) {
		int i1 = tris[i].p1;
		int i2 = tris[i].p2;
		int i3 = tris[i].p3;

		Vector3 v1 = verts[i1];
		Vector3 v2 = verts[i2];
		Vector3 v3 = verts[i3];

		Vector2 w1 = texcoord[i1];
		Vector2 w2 = texcoord[i2];
		Vector2 w3 = texcoord[i3];

		float x1 = v2.x - v1.x;
		float x2 = v3.x - v1.x;
		float y1 = v2.y - v1.y;
		float y2 = v3.y - v1.y;
		float z1 = v2.z - v1.z;
		float z2 = v3.z - v1.z;

		float s1 = w2.u - w1.u;
		float s2 = w3.u - w1.u;
		float t1 = w2.v - w1.v;
		float t2 = w3.v - w1.v;

		float r = (s1 * t2 - s2 * t1);
		r = (r >= 0.0f ? +1.0f : -1.0f);

		Vector3 sdir = Vector3((t2 * x1 - t1 * x2) * r, (t2 * y1 - t1 * y2) * r, (t2 * z1 - t1 * z2) * r);
		Vector3 tdir = Vector3((s1 * x2 - s2 * x1) * r, (s1 * y2 - s2 * y1) * r, (s1 * z2 - s2 * z1) * r);

		sdir.Normalize();
		tdir.Normalize();

		tangents[i1] += tdir;
		tangents[i2] += tdir;
		tangents[i3] += tdir;

		bitangents[i1] += sdir;
		bitangents[i2] += sdir;
		bitangents[i3] += sdir;
	}

	for (int i = 0; i < nVerts; i++) {
		if (tangents[i].IsZero() || bitangents[i].IsZero()) {
			tangents[i].x = norms[i].y;
			tangents[i].y = norms[i].z;
			tangents[i].z = norms[i].x;
			bitangents[i] = norms[i].cross(tangents[i]);
		}
		else {
			tangents[i].Normalize();
			tangents[i] = (tangents[i] - norms[i] * norms[i].dot(tangents[i]));
			tangents[i].Normalize();

			bitangents[i].Normalize();

			bitangents[i] = (bitangents[i] - norms[i] * norms[i].dot(bitangents[i]));
			bitangents[i] = (bitangents[i] - tangents[i] * tangents[i].dot(bitangents[i]));

			bitangents[i].Normalize();
		}
	}

	queueUpdate[UpdateType::Tangents] = true;
	queueUpdate[UpdateType::Bitangents] = true;
}

void Mesh::ConnectedPointsInSphere(const Vector3& center, float sqradius, int startTri, std::vector<bool>& pointvisit, int outPoints[], int& nOutPoints) {
	// Add the triangle's three vertices to the queue, if they're within the
	// radius.
	for (int tvi = 0; tvi < 3; ++tvi) {
		auto p = tris[startTri][tvi];
		if (!pointvisit[p] && verts[p].DistanceSquaredTo(center) <= sqradius) {
			outPoints[nOutPoints++] = p;
			pointvisit[p] = true;
		}
	}

	// Loop through points in the queue
	for (int adjCursor = 0; adjCursor < nOutPoints; ++adjCursor) {
		int p = outPoints[adjCursor];

		// Add welds of p to the queue
		DoForEachWeldedVertex(p, [&](int wvi){
			if (!pointvisit[wvi]) {
				pointvisit[wvi] = true;
				outPoints[nOutPoints++] = wvi;
			}
		});

		// Add adjacent points of p to the queue, if they're within the radius
		for (int ei : vertEdges[p]) {
			int op = edges[ei].p1 == p ? edges[ei].p2 : edges[ei].p1;
			if (!pointvisit[op] && verts[op].DistanceSquaredTo(center) <= sqradius) {
				pointvisit[op] = true;
				outPoints[nOutPoints++] = op;
			}
		}
	}
}

void Mesh::ConnectedPointsInTwoSpheres(const Vector3& center1, const Vector3& center2, float sqradius, int startTri1, int startTri2, std::vector<bool>& pointvisit, int outPoints[], int& nOutPoints) {
	// Add each triangle's three vertices to the queue, if they're within the
	// radius.
	for (int triInd = 0; triInd < 2; ++triInd) {
		int startTri = triInd ? startTri2 : startTri1;
		for (int tvi = 0; tvi < 3; ++tvi) {
			auto p = tris[startTri][tvi];
			if (!pointvisit[p] &&
				(verts[p].DistanceSquaredTo(center1) <= sqradius ||
				verts[p].DistanceSquaredTo(center2) <= sqradius)) {
				outPoints[nOutPoints++] = p;
				pointvisit[p] = true;
			}
		}
	}

	// Loop through points in the queue
	for (int adjCursor = 0; adjCursor < nOutPoints; ++adjCursor) {
		int p = outPoints[adjCursor];

		// Add welds of p to the queue
		DoForEachWeldedVertex(p, [&](int wvi){
			if (!pointvisit[wvi]) {
				pointvisit[wvi] = true;
				outPoints[nOutPoints++] = wvi;
			}
		});

		// Add adjacent points of p to the queue, if they're within the radius
		for (int ei : vertEdges[p]) {
			int op = edges[ei].p1 == p ? edges[ei].p2 : edges[ei].p1;
			if (!pointvisit[op] &&
				(verts[op].DistanceSquaredTo(center1) <= sqradius ||
				verts[op].DistanceSquaredTo(center2) <= sqradius)) {
				pointvisit[op] = true;
				outPoints[nOutPoints++] = op;
			}
		}
	}
}
