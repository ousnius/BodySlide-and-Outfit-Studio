/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

// OutfitProject: slider data and morphs in TRI, NIF, BSD, OBJ, FBX and Starfield morph files

#include "OutfitProject.h"
#include "files/FBXWrangler.h"
#include "files/GameDataStream.h"
#include "files/ObjFile.h"
#include "files/TriFile.h"
#include "files/SFMorphFile.h"
#include "utils/PlatformUtil.h"
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

bool OutfitProject::WriteMorphTRI(const std::string& triPath) {
	TriFile tri;
	std::string triFilePath = triPath;

	for (auto& shape : workNif.GetShapes()) {
		int shapeVertCount = GetVertexCount(shape);
		if (shapeVertCount <= 0)
			continue;

		std::string shapeName = shape->name.get();
		bool bIsOutfit = true;
		if (IsBaseShape(shape))
			bIsOutfit = false;

		for (size_t s = 0; s < activeSet.size(); s++) {
			if (!activeSet[s].bClamp && !activeSet[s].bZap) {
				MorphDataPtr morph = std::make_shared<MorphData>();
				morph->name = activeSet[s].name;

				if (activeSet[s].bUV) {
					morph->type = MORPHTYPE_UV;

					std::vector<Vector2> uvs;
					uvs.resize(shapeVertCount);

					std::string target = ShapeToTarget(shapeName);
					if (!bIsOutfit) {
						std::string dn = activeSet[s].TargetDataName(target);
						if (dn.empty())
							continue;

						baseDiffData.ApplyUVDiff(dn, target, 1.0f, &uvs);
					}
					else
						morpher.ApplyResultToUVs(morph->name, target, &uvs);

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

					std::string target = ShapeToTarget(shapeName);
					if (!bIsOutfit) {
						std::string dn = activeSet[s].TargetDataName(target);
						if (dn.empty())
							continue;

						baseDiffData.ApplyDiff(dn, target, 1.0f, &verts);
					}
					else
						morpher.ApplyResultToVerts(morph->name, target, &verts);

					int i = 0;
					for (auto& v : verts) {
						if (!v.IsZero(true))
							morph->offsets.emplace(i, v);
						i++;
					}
				}

				if (morph->offsets.size() > 0)
					tri.AddMorph(shapeName, morph);
			}
		}
	}

	if (!tri.Write(triFilePath))
		return false;

	return true;
}

bool OutfitProject::WriteHeadTRI(NiShape* shape, const std::string& triPath) {
	if (!shape)
		return false;

	TriHeadFile tri;
	std::string triFilePath = triPath;

	int shapeVertCount = GetVertexCount(shape);
	if (shapeVertCount <= 0)
		return false;

	std::string shapeName = shape->name.get();
	bool bIsOutfit = true;
	if (IsBaseShape(shape))
		bIsOutfit = false;

	std::vector<Triangle> tris;
	if (!shape->GetTriangles(tris))
		return false;

	const std::vector<Vector3>* verts = workNif.GetVertsForShape(shape);
	if (!verts)
		return false;

	tri.SetVertices(*verts);
	tri.SetTriangles(tris);

	const std::vector<Vector2>* uvs = workNif.GetUvsForShape(shape);
	if (uvs)
		tri.SetUV(*uvs);

	for (size_t s = 0; s < activeSet.size(); s++) {
		if (!activeSet[s].bClamp && !activeSet[s].bZap && !activeSet[s].bUV) {
			TriHeadMorph morph;
			morph.morphName = activeSet[s].name;

			std::vector<Vector3> morphVerts;
			morphVerts.resize(shapeVertCount);

			std::string target = ShapeToTarget(shapeName);
			if (!bIsOutfit) {
				std::string dn = activeSet[s].TargetDataName(target);
				if (dn.empty())
					continue;

				baseDiffData.ApplyDiff(dn, target, 1.0f, &morphVerts);
			}
			else
				morpher.ApplyResultToVerts(morph.morphName, target, &morphVerts);

			morph.vertices = morphVerts;
			tri.AddMorph(morph);
		}
	}

	if (!tri.Write(triFilePath))
		return false;

	return true;
}

NiShape* OutfitProject::ImportHeadTRI(const std::string& triPath, const std::string& shapeName, bool withSliders, std::vector<std::string>* newSliders) {
	TriHeadFile tri;
	if (!tri.Read(triPath))
		return nullptr;

	std::string baseName = shapeName;
	if (baseName.empty())
		baseName = wxFileName(wxString::FromUTF8(triPath)).GetName().ToUTF8().data();

	std::string uniqueName = baseName;
	for (int i = 2; IsValidShape(uniqueName); i++)
		uniqueName = baseName + std::to_string(i);

	auto verts = tri.GetVertices();
	auto tris = tri.GetTriangles();
	auto uvs = tri.GetUV();

	auto shape = CreateNifShapeFromData(uniqueName, &verts, &tris, &uvs);
	if (!shape)
		return nullptr;

	if (!withSliders)
		return shape;

	auto morphs = tri.GetMorphs();
	for (auto& morph : morphs) {
		std::unordered_map<uint16_t, Vector3> diff;
		diff.reserve(morph.vertices.size());

		for (size_t i = 0; i < morph.vertices.size(); i++)
			if (!morph.vertices[i].IsZero(true))
				diff.emplace(static_cast<uint16_t>(i), morph.vertices[i]);

		if (diff.empty())
			continue;

		if (!ValidSlider(morph.morphName)) {
			AddEmptySlider(morph.morphName);

			if (newSliders)
				newSliders->push_back(morph.morphName);
		}

		SetSliderFromDiff(morph.morphName, shape, diff);
	}

	return shape;
}

bool OutfitProject::WriteSFMorphs(nifly::NiShape* shape, const std::string& morphPath) {
	SFMorphFile morphFile;
	std::string morphPathFilePath = morphPath;

	int shapeVertCount = GetVertexCount(shape);
	if (shapeVertCount <= 0)
		return false;

	std::string shapeName = shape->name.get();
	bool bIsOutfit = true;
	if (IsBaseShape(shape))
		bIsOutfit = false;

	morphFile.SetVertexCount(shapeVertCount);

	SFMorphNormalSettings normalSettings;
	normalSettings.smoothSeamNormals = activeSet.GetSmoothSeamNormals(shapeName);
	normalSettings.smoothSeamNormalsAngle = activeSet.GetSmoothSeamNormalsAngle(shapeName);
	normalSettings.lockNormals = activeSet.GetLockNormals(shapeName);
	const SFMorphShape morphShape = SFMorphShape::FromShape(workNif, shape, normalSettings);

	for (size_t s = 0; s < activeSet.size(); s++) {
		// morph.dat has no UV morphs
		if (!activeSet[s].bClamp && !activeSet[s].bZap && !activeSet[s].bUV) {
			std::string morphName = activeSet[s].name;
			std::unordered_map<uint16_t, Vector3> morphOffsets;

			std::vector<Vector3> diffs;
			diffs.resize(shapeVertCount);

			std::string target = ShapeToTarget(shapeName);
			if (!bIsOutfit) {
				std::string dn = activeSet[s].TargetDataName(target);
				if (dn.empty())
					continue;

				baseDiffData.ApplyDiff(dn, target, 1.0f, &diffs);
			}
			else
				morpher.ApplyResultToVerts(morphName, target, &diffs);

			int i = 0;
			for (auto& d : diffs) {
				if (!d.IsZero(true))
					morphOffsets.emplace(i, d);
				i++;
			}

			// TODO: Allow storing custom normals for a slider?
			if (morphOffsets.size() > 0)
				morphShape.AddMorph(morphFile, morphName, morphOffsets);
		}
	}

	morphFile.CacheToFileData();

	if (!morphFile.Write(morphPathFilePath))
		return false;

	return true;
}

int OutfitProject::SaveSliderNIF(const std::string& sliderName, NiShape* shape, const std::string& fileName) {
	if (!shape)
		return 1;

	std::string target = ShapeToTarget(shape->name.get());

	std::vector<Vector3> outVerts;
	std::vector<Vector2> outUVs;

	if (!workNif.GetVertsForShape(shape, outVerts))
		return 2;

	workNif.GetUvsForShape(shape, outUVs);

	GetSliderDiff(shape, sliderName, outVerts);
	GetSliderDiffUV(shape, sliderName, outUVs);

	NifFile nif;
	nif.CopyFrom(workNif);

	auto sliderShape = nif.FindBlockByName<NiShape>(shape->name.get());
	if (sliderShape) {
		for (auto& s : nif.GetShapes()) {
			if (s != sliderShape)
				nif.DeleteShape(s);
		}

		nif.SetVertsForShape(sliderShape, outVerts);
		nif.SetUvsForShape(sliderShape, outUVs);

		nif.DeleteUnreferencedNodes();
	}

	// Slider NIFs in ShapeData always use internal geometry
	ForceInternalGeometry(nif);

	std::fstream file;
	PlatformUtil::OpenFileStream(file, fileName, std::ios::out | std::ios::binary);

	if (nif.Save(file) != 0)
		return 3;

	return 0;
}

int OutfitProject::SaveSliderBSD(const std::string& sliderName, NiShape* shape, const std::string& fileName) {
	std::string target = ShapeToTarget(shape->name.get());

	if (IsBaseShape(shape)) {
		std::string sliderData = activeSet[sliderName].TargetDataName(target);
		baseDiffData.SaveSet(sliderData, target, fileName);
	}
	else
		morpher.SaveResultDiff(target, sliderName, fileName);

	return 0;
}

int OutfitProject::SaveSliderOBJ(const std::string& sliderName, NiShape* shape, const std::string& fileName, const bool onlyDiff) {
	if (!shape)
		return 1;

	std::vector<Triangle> tris;
	shape->GetTriangles(tris);

	std::string target = ShapeToTarget(shape->name.get());
	const std::vector<Vector3>* verts = workNif.GetVertsForShape(shape);
	if (!verts)
		return 2;

	const std::vector<Vector2>* uvs = workNif.GetUvsForShape(shape);
	const std::vector<Vector3> norms;

	std::vector<Vector3> outVerts = *verts;

	if (IsBaseShape(shape)) {
		std::string sliderData = activeSet[sliderName].TargetDataName(target);
		bool foundDiff = baseDiffData.ApplyDiff(sliderData, target, 1.0f, &outVerts);
		if (onlyDiff && !foundDiff)
			return 0;
	}
	else {
		bool foundDiff = morpher.ApplyResultToVerts(sliderName, target, &outVerts);
		if (onlyDiff && !foundDiff)
			return 0;
	}

	ObjFile obj;
	obj.SetScale(Vector3(0.1f, 0.1f, 0.1f));
	obj.AddGroup(shape->name.get(), outVerts, tris, uvs ? *uvs : std::vector<Vector2>(), norms);
	if (obj.Save(fileName))
		return 3;

	return 0;
}

bool OutfitProject::SetSliderFromNIF(const std::string& sliderName, NiShape* shape, const std::string& fileName) {
	std::string target = SliderDataTargetForShape(shape);

	std::fstream file;
	PlatformUtil::OpenFileStream(file, fileName, std::ios::in | std::ios::binary);

	NifFile nif;
	if (nif.Load(file) != 0)
		return false;

	for (const auto& meshPath : GameDataStream::LoadExternalGeometry(nif, Config["GameDataPath"], fileName))
		wxLogWarning("Unable to locate external mesh data '%s' of nif file: %s", meshPath, fileName);

	// File needs at least one shape
	auto shapeNames = nif.GetShapeNames();
	if (shapeNames.empty())
		return false;

	// Use first shape or shape with matching name
	std::string srcShapeName = shapeNames.front();
	if (std::find(shapeNames.begin(), shapeNames.end(), shape->name.get()) != shapeNames.end())
		srcShapeName = shape->name.get();

	std::vector<Vector3> verts;
	std::vector<Vector2> uvs;

	auto srcShape = nif.FindBlockByName<NiShape>(srcShapeName);
	if (!srcShape)
		return false;

	if (!nif.GetVertsForShape(srcShape, verts))
		return false;

	nif.GetUvsForShape(srcShape, uvs);

	std::unordered_map<uint16_t, Vector3> diff;
	if (activeSet[sliderName].bUV) {
		if (workNif.CalcUVDiff(shape, &uvs, diff))
			return false;
	}
	else {
		if (workNif.CalcShapeDiff(shape, &verts, diff))
			return false;
	}

	std::string sliderData = EnsureSliderDataLocal(sliderName, shape);
	if (sliderData.empty())
		return false;

	if (IsBaseShape(shape)) {
		baseDiffData.LoadSet(sliderData, target, diff);
	}
	else
		morpher.SetResultDiff(target, sliderName, diff);

	return true;
}

void OutfitProject::SetSliderFromBSD(const std::string& sliderName, NiShape* shape, const std::string& fileName) {
	std::string target = SliderDataTargetForShape(shape);
	std::string sliderData = EnsureSliderDataLocal(sliderName, shape);
	if (sliderData.empty())
		return;

	if (IsBaseShape(shape)) {
		baseDiffData.LoadSet(sliderData, target, fileName);
	}
	else {
		DiffDataSets tmpSet;
		tmpSet.LoadSet(sliderName, target, fileName);
		std::unordered_map<uint16_t, Vector3>* diff = tmpSet.GetDiffSet(sliderName);
		morpher.SetResultDiff(target, sliderName, (*diff));
	}
}

bool OutfitProject::SetSliderFromOBJ(const std::string& sliderName, NiShape* shape, const std::string& fileName) {
	std::string target = SliderDataTargetForShape(shape);

	ObjImportOptions options;
	options.NoFaces = true;

	ObjFile obj;
	obj.LoadForNif(fileName, options);

	// File needs at least one group
	std::vector<std::string> groupNames = obj.GetGroupList();
	if (groupNames.empty())
		return false;

	// Use first shape or shape with matching name
	std::string sourceShape = groupNames.front();
	if (std::find(groupNames.begin(), groupNames.end(), shape->name.get()) != groupNames.end())
		sourceShape = shape->name.get();

	std::vector<Vector3> objVerts;
	std::vector<Vector2> objUVs;
	if (!obj.CopyDataForGroup(sourceShape, &objVerts, nullptr, &objUVs, nullptr))
		return false;

	std::unordered_map<uint16_t, Vector3> diff;
	if (activeSet[sliderName].bUV) {
		if (workNif.CalcUVDiff(shape, &objUVs, diff))
			return false;
	}
	else {
		if (workNif.CalcShapeDiff(shape, &objVerts, diff, 10.0f))
			return false;
	}

	std::string sliderData = EnsureSliderDataLocal(sliderName, shape);
	if (sliderData.empty())
		return false;

	if (IsBaseShape(shape)) {
		baseDiffData.LoadSet(sliderData, target, diff);
	}
	else
		morpher.SetResultDiff(target, sliderName, diff);

	return true;
}

#ifdef USE_FBXSDK
bool OutfitProject::SetSliderFromFBX(const std::string& sliderName, NiShape* shape, const std::string& fileName) {
	std::string target = SliderDataTargetForShape(shape);

	FBXWrangler fbxw;
	bool result = fbxw.ImportScene(fileName);
	if (!result)
		return 1;

	std::vector<std::string> shapes;
	fbxw.GetShapeNames(shapes);
	bool found = false;
	for (auto& s : shapes)
		if (s == shape->name.get())
			found = true;

	if (!found)
		return false;

	FBXShape* fbxShape = fbxw.GetShape(shape->name.get());

	std::unordered_map<uint16_t, Vector3> diff;
	if (IsBaseShape(shape)) {
		if (workNif.CalcShapeDiff(shape, &fbxShape->verts, diff, 1.0f))
			return false;

		std::string sliderData = EnsureSliderDataLocal(sliderName, shape);
		if (sliderData.empty())
			return false;

		baseDiffData.LoadSet(sliderData, target, diff);
	}
	else {
		if (workNif.CalcShapeDiff(shape, &fbxShape->verts, diff, 1.0f))
			return false;

		std::string sliderData = EnsureSliderDataLocal(sliderName, shape);
		if (sliderData.empty())
			return false;

		morpher.SetResultDiff(target, sliderName, diff);
	}

	return true;
}
#endif

void OutfitProject::SetSliderFromDiff(const std::string& sliderName, NiShape* shape, const TargetDataDiffs& diff) {
	std::string target = SliderDataTargetForShape(shape);
	std::string sliderData = EnsureSliderDataLocal(sliderName, shape);
	if (sliderData.empty())
		return;

	if (IsBaseShape(shape)) {
		baseDiffData.LoadSet(sliderData, target, diff);
	}
	else {
		morpher.EmptyResultDiff(target, sliderName);
		morpher.SetResultDiff(target, sliderName, diff);
	}
}
