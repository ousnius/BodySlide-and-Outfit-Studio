/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

// OutfitProject: importing and exporting NIF, OBJ and FBX files

#include "OutfitProject.h"
#include "ProjectNifHelpers.h"
#include "files/FBXWrangler.h"
#include "files/GameDataStream.h"
#include "files/ObjFile.h"
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

namespace {

// Uses the given answer to an import question, or asks the host with "yes" as the default.
bool AskImportChoice(IProjectHost& host, const std::optional<bool>& choice, const wxString& msg, const wxString& title) {
	if (choice)
		return *choice;

	return host.Ask(msg, title, false, HostAnswer::Yes) == HostAnswer::Yes;
}

} // namespace

int OutfitProject::ImportNIF(const std::string& fileName, bool clear, const std::string& inOutfitName, std::map<std::string, std::string>* renamedShapes) {
	std::fstream fileStream;
	if (!fileName.empty())
		PlatformUtil::OpenFileStream(fileStream, fileName, std::ios::in | std::ios::binary);

	return ImportNIF(fileStream, fileName, clear, inOutfitName, renamedShapes);
}

int OutfitProject::ImportNIF(std::istream& fileStream, const std::string& fileName, bool clear, const std::string& inOutfitName, std::map<std::string, std::string>* renamedShapes) {
	if (clear)
		ClearOutfit();

	if (fileName.empty()) {
		wxLogMessage("No outfit selected.");
		return 0;
	}

	auto targetGame = (TargetGame)Config.GetIntValue("TargetGame");

	if (!inOutfitName.empty())
		outfitName = inOutfitName;
	else if (outfitName.empty())
		outfitName = "New Outfit";

	wxFileName file(fileName);
	if (mBaseFile.empty())
		mBaseFile = file.GetFullName();

	if (clear) {
		mGameFile = file.GetName();
		mGamePath = file.GetPath();

		int pos = mGamePath.Lower().Find("meshes");
		if (pos != wxNOT_FOUND)
			mGamePath = mGamePath.Mid(pos);
		else
			mGamePath.Clear();

		if (targetGame == SKYRIM || targetGame == SKYRIMSE || targetGame == SKYRIMVR) {
			wxString fileRest;
			if (mGameFile.EndsWith("_0", &fileRest) || mGameFile.EndsWith("_1", &fileRest))
				mGameFile = fileRest;
		}
	}

	NifFile nif;
	int error = nif.Load(fileStream);
	if (error) {
		if (error == 2) {
			wxString errorText = wxString::Format(_("NIF version not supported!\n\nFile: %s\n%s"), fileName, nif.GetHeader().GetVersion().GetVersionInfo());

			wxLogError(errorText);
			host->ShowMessage(HostMessageLevel::Error, errorText, _("NIF Error"));
			return 4;
		}

		wxLogError("Could not load NIF file '%s'!", fileName);
		host->ShowMessage(HostMessageLevel::Error, wxString::Format(_("Could not load NIF file '%s'!"), fileName), _("NIF Error"));
		return 1;
	}

	ValidateNIF(nif, fileName);

	nif.SetNodeName(0, "Scene Root");
	nif.RenameDuplicateShapes();

	if (baseShape) {
		std::string baseShapeName = baseShape->name.get();
		auto bshape = nif.FindBlockByName<NiShape>(baseShapeName);
		if (nif.RenameShape(bshape, baseShapeName + "_outfit")) {
			if (renamedShapes)
				(*renamedShapes)[baseShapeName] = baseShapeName + "_outfit";
		}
	}

	std::vector<std::string> shapes = workNif.GetShapeNames();
	auto nifShapes = nif.GetShapes();

	for (auto& s : nifShapes) {
		std::vector<std::string> uniqueShapes = nif.GetShapeNames();
		uniqueShapes.insert(uniqueShapes.end(), shapes.begin(), shapes.end());

		std::string shapeName = s->name.get();
		std::string newName = shapeName;
		int uniqueCount = 0;
		for (;;) {
			auto foundShape = find(uniqueShapes.begin(), uniqueShapes.end(), newName);
			if (foundShape != uniqueShapes.end()) {
				uniqueShapes.erase(foundShape);
				uniqueCount++;
				if (uniqueCount > 1)
					newName = shapeName + wxString::Format("_%d", uniqueCount).ToStdString();
			}
			else {
				if (uniqueCount > 1) {
					if (nif.RenameShape(s, newName)) {
						if (renamedShapes)
							(*renamedShapes)[shapeName] = newName;
					}
				}
				break;
			}
		}
	}

	clothData.Capture(nif, fileName, nif.GetShapes(), IsSkeletonBone);

	// Record physics XML links of all shapes before they are merged in
	CaptureRootPhysicsData(nif);
	CapturePhysicsFiles(nif, nif.GetShapes());

	if (workNif.IsValid()) {
		for (auto& s : nif.GetShapes()) {
			std::string shapeName = s->name.get();
			auto clonedShape = workNif.CloneShape(s, shapeName, &nif);
			workAnim.LoadFromNif(&workNif, clonedShape);
		}

		// CloneShape leaves the root behind, so bring its extra data over
		MergeRootExtraData(nif);
	}
	else {
		workNif.CopyFrom(nif);
		workAnim.LoadFromNif(&workNif);
	}

	return 0;
}

int OutfitProject::ExportNIF(const std::string& fileName, const std::vector<Mesh*>& modMeshes, bool withRef, std::optional<bool> useInternalGeom) {
	workAnim.CleanupBones();
	host->OnAnimationChanged();

	NifFile clone(workNif);
	ChoosePhysicsData(clone);

	std::vector<Vector3> liveVerts;
	std::vector<Vector3> liveNorms;
	for (auto& m : modMeshes) {
		auto shape = clone.FindBlockByName<NiShape>(m->shapeName);
		if (shape) {
			liveVerts.clear();
			liveNorms.clear();

			for (int i = 0; i < m->nVerts; i++) {
				liveVerts.push_back(Mesh::TransformPosMeshToNif(m->verts[i]));
				liveNorms.push_back(Mesh::TransformDirMeshToNif(m->norms[i]));
			}

			clone.SetVertsForShape(shape, liveVerts);

			if (clone.GetHeader().GetVersion().IsSK() || clone.GetHeader().GetVersion().IsSSE()) {
				NiShader* shader = clone.GetShader(shape);
				if (shader && shader->IsModelSpace())
					continue;
			}

			clone.SetNormalsForShape(shape, liveNorms);
			clone.CalcTangentsForShape(shape);
		}
	}

	if (!withRef && baseShape) {
		std::string baseShapeName = baseShape->name.get();
		auto bshape = clone.FindBlockByName<NiShape>(baseShapeName);
		clone.DeleteShape(bshape);
		WriteAnimToNif(clone, baseShapeName);
	}
	else
		WriteAnimToNif(clone);

	ChooseClothData(clone);

	for (auto& s : clone.GetShapes())
		clone.UpdateSkinPartitions(s);

	clone.SetShapeOrder(GetShapeOrder());
	clone.GetHeader().SetExportInfo("Exported using Outfit Studio.");

	GenerateStarfieldMeshlets(clone);
	ConfigureInternalGeometry(clone, fileName, useInternalGeom);

	std::fstream file;
	PlatformUtil::OpenFileStream(file, fileName, std::ios::out | std::ios::binary);

	int result = clone.Save(file);
	if (result == 0)
		SaveExternalMeshes(clone, fileName);

	return result;
}

bool OutfitProject::HasBonesPerVertexCount(NiShape* s) {
	return s && workNif.GetShapeMaxBonesPerVertex(s) > 4;
}

uint8_t OutfitProject::GetNeededBonesPerVertex(NiShape* s) {
	if (!HasBonesPerVertexCount(s))
		return 4;

	return std::clamp<uint8_t>(workAnim.GetMostBonesPerVertex(s->name.get()), 1, workNif.GetShapeMaxBonesPerVertex(s));
}

uint8_t OutfitProject::GetBonesPerVertex(NiShape* s) {
	if (uint8_t locked = GetLockedBonesPerVertex(s))
		return locked;

	return GetNeededBonesPerVertex(s);
}

uint8_t OutfitProject::GetBonesPerVertexLimit(NiShape* s) {
	if (!HasBonesPerVertexCount(s))
		return 4;

	if (uint8_t locked = GetLockedBonesPerVertex(s))
		return locked;

	return workNif.GetShapeMaxBonesPerVertex(s);
}

uint8_t OutfitProject::GetLockedBonesPerVertex(NiShape* s) {
	if (!HasBonesPerVertexCount(s))
		return 0;

	return std::min(activeSet.GetBonesPerVertex(s->name.get()), workNif.GetShapeMaxBonesPerVertex(s));
}

void OutfitProject::SetLockedBonesPerVertex(NiShape* s, uint8_t bonesPerVertex) {
	if (!HasBonesPerVertexCount(s))
		return;

	activeSet.SetBonesPerVertex(s->name.get(), std::min(bonesPerVertex, workNif.GetShapeMaxBonesPerVertex(s)));
}

void OutfitProject::ChooseClothData(NifFile& nif) {
	// A NIF holds a single cloth block. Every loaded block is offered, with the cloth that fits the exported shapes
	// preselected. That includes the reference's cloth once an outfit has been given its physics bones, even when the
	// reference isn't exported. Unattended runs and a cancelled prompt get the preselection.
	auto choices = clothData.GetExportChoices(nif, IsSkeletonBone);

	std::string source;
	if (choices.preselected)
		source = choices.sources[*choices.preselected];

	if (!choices.sources.empty()) {
		wxArrayString names;
		for (const auto& name : choices.sources)
			names.Add(wxString::FromUTF8(name));

		// The last entry writes no cloth
		names.Add(_("None"));

		int sel = static_cast<int>(choices.preselected.value_or(choices.sources.size()));
		if (host->ChooseOne(_("There was cloth physics data loaded at some point (BSClothExtraData). Please choose the origin to use in the output."),
							_("Choose cloth data"),
							names,
							sel)
			&& sel >= 0 && sel < static_cast<int>(names.GetCount()))
			source = static_cast<size_t>(sel) < choices.sources.size() ? choices.sources[sel] : std::string();
	}

	clothData.Write(nif, source);
}

void OutfitProject::ChoosePhysicsData(NifFile& nif) {
	if (rootPhysicsData.empty())
		return;

	auto root = nif.GetRootNode();
	if (!root)
		return;

	size_t chosen = 0;
	if (rootPhysicsData.size() > 1 && host->IsInteractive()) {
		wxArrayString physicsFileNames;
		for (auto& physicsData : rootPhysicsData)
			physicsFileNames.Add(wxString::FromUTF8(physicsData->stringData.get()));

		int sel = 0;
		if (!host->ChooseOne(_("There was HDT-SMP physics data loaded at some point. Only one physics file can be linked to the root node, so please choose the one to use in the output."),
							 _("Choose physics data"),
							 physicsFileNames,
							 sel))
			return;

		if (sel == wxNOT_FOUND)
			return;

		chosen = static_cast<size_t>(sel);
	}

	// Drop the links the work NIF inherited from the file it was created from,
	// so the choice is the only one left. Deleting a block clears the refs
	// pointing at it and shifts every higher block index down, so the ids have
	// to be resolved one at a time.
	for (auto* existingData : GetRootPhysicsExtraData(nif))
		nif.GetHeader().DeleteBlock(nif.GetBlockID(existingData));

	auto clonedData = rootPhysicsData[chosen]->Clone();
	auto clonedBlock = clonedData.get();
	nif.AssignExtraData(root, std::move(clonedData));

	RemapExtraDataStrings(nif, clonedBlock);
}

int OutfitProject::ExportShapeNIF(const std::string& fileName, const std::vector<std::string>& exportShapes, std::optional<bool> useInternalGeom) {
	if (exportShapes.empty())
		return 1;

	if (!workNif.IsValid())
		return 2;

	workAnim.CleanupBones();

	NifFile clone(workNif);
	ChoosePhysicsData(clone);

	clone.SetShapeOrder(GetShapeOrder());

	for (auto& s : clone.GetShapes())
		if (find(exportShapes.begin(), exportShapes.end(), s->name.get()) == exportShapes.end())
			clone.DeleteShape(s);

	WriteAnimToNif(clone);

	ChooseClothData(clone);

	for (auto& s : clone.GetShapes())
		clone.UpdateSkinPartitions(s);

	clone.GetHeader().SetExportInfo("Exported using Outfit Studio.");

	GenerateStarfieldMeshlets(clone);
	ConfigureInternalGeometry(clone, fileName, useInternalGeom);

	std::fstream file;
	PlatformUtil::OpenFileStream(file, fileName, std::ios::out | std::ios::binary);

	int result = clone.Save(file);
	if (result == 0)
		SaveExternalMeshes(clone, fileName);

	return result;
}

void OutfitProject::ForceInternalGeometry(NifFile& nif) {
	if (!nif.GetHeader().GetVersion().IsSF())
		return;

	for (auto& s : nif.GetShapes()) {
		auto bsgeo = dynamic_cast<BSGeometry*>(s);
		if (bsgeo)
			bsgeo->SetInternalGeomData(true);
	}
}

void OutfitProject::GenerateStarfieldMeshlets(NifFile& nif) {
	if (!nif.GetHeader().GetVersion().IsSF()) {
		return;
	}

	// Generate meshlets for any BSGeometry that lacks them (freshly authored shapes, or shapes whose meshlets were dropped by an edit that changed the triangle list).
	std::vector<NiShape*> toConvert;
	for (auto& s : nif.GetShapes()) {
		auto bsgeo = dynamic_cast<BSGeometry*>(s);
        if (!bsgeo || bsgeo->HasMeshlets()) {
            continue;
		}
		bsgeo->GenerateMeshlets();
	}
}

void OutfitProject::ConfigureInternalGeometry(NifFile& nif, const std::string& nifFileName, std::optional<bool> useInternalGeom) {
	if (!nif.GetHeader().GetVersion().IsSF())
		return;

	bool hasBSGeo = false;
	for (auto& s : nif.GetShapes()) {
		if (s->HasType<BSGeometry>()) {
			hasBSGeo = true;
			break;
		}
	}
	if (!hasBSGeo)
		return;

	// Derive geometry folder name from the NIF filename (without extension)
	wxFileName nifPath(wxString::FromUTF8(nifFileName));
	std::string folderName = nifPath.GetName().ToUTF8().data();

	for (auto& s : nif.GetShapes()) {
		auto bsgeo = dynamic_cast<BSGeometry*>(s);
		if (!bsgeo)
			continue;

		// Use provided choice, or keep the shape's current state
		bool useInternal = useInternalGeom.value_or(bsgeo->HasInternalGeomData());

		bsgeo->SetInternalGeomData(useInternal);

		if (!useInternal) {
			// Ensure external mesh paths are set
			for (uint8_t i = 0; i < bsgeo->MeshCount(); i++) {
				auto mesh = bsgeo->SelectMesh(i);
				if (mesh && mesh->meshName.get().empty()) {
					// Generate mesh path: NifName\ShapeName_Index
					std::string shapeName = s->name.get();
					auto sanitize = [](std::string& str) {
						for (char& c : str) {
							if (c == ' ' || c == '/' || c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|')
								c = '_';
						}
					};
					std::string folder = folderName;
					sanitize(folder);
					sanitize(shapeName);
					mesh->meshName.get() = folder + "\\" + shapeName + "_" + std::to_string(i);
				}
				bsgeo->ReleaseMesh();
			}
		}
	}
}

bool OutfitProject::SaveExternalMeshes(NifFile& nif, const std::string& nifFileName) {
	if (!nif.GetHeader().GetVersion().IsSF())
		return true;

	wxFileName nifPath(wxString::FromUTF8(nifFileName));
	wxString nifDir = nifPath.GetPath();

	for (auto& s : nif.GetShapes()) {
		auto meshPaths = nif.GetExternalGeometryPathRefs(s);
		uint8_t meshIndex = 0;

		for (auto& meshPathRef : meshPaths) {
			std::string meshPath = meshPathRef.get();
			if (meshPath.empty()) {
				meshIndex++;
				continue;
			}

			// Build full output path: geometries/{meshPath}.mesh in the Data root (sibling to Meshes/)
			// Walk up to the directory that contains "meshes" (case-insensitive)
			wxFileName walker(nifDir + wxFileName::GetPathSeparator());
			bool foundMeshes = false;
			while (walker.GetDirCount() > 0) {
				wxString lastDir = walker.GetDirs().Last();
				if (lastDir.CmpNoCase("meshes") == 0) {
					walker.RemoveLastDir();
					foundMeshes = true;
					break;
				}
				walker.RemoveLastDir();
			}
			// If not inside a game data layout, place geometries/ next to the NIF
			wxString rootDir = foundMeshes ? walker.GetPath() : nifDir;
			wxString normalizedMeshPath = wxString::FromUTF8(meshPath);
			normalizedMeshPath.Replace("\\", wxString(wxFileName::GetPathSeparator()));
			normalizedMeshPath.Replace("/", wxString(wxFileName::GetPathSeparator()));
			if (normalizedMeshPath.Length() < 5 || normalizedMeshPath.Right(5).CmpNoCase(".mesh") != 0)
				normalizedMeshPath += ".mesh";
			wxFileName meshFullPath(rootDir + wxFileName::GetPathSeparator() + "geometries"
				+ wxFileName::GetPathSeparator() + normalizedMeshPath);

			// Ensure the directory exists
			wxFileName::Mkdir(meshFullPath.GetPath(), wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);

			std::string meshFileStr = meshFullPath.GetFullPath().ToUTF8().data();

			std::fstream meshFile;
			PlatformUtil::OpenFileStream(meshFile, meshFileStr, std::ios::out | std::ios::binary);
			if (meshFile.fail()) {
				wxLogError("Failed to save external mesh file '%s'.", meshFileStr.c_str());
				meshIndex++;
				continue;
			}

			nif.SaveExternalShapeData(s, meshFile, meshIndex);
			meshIndex++;
		}
	}

	return true;
}

int OutfitProject::ImportOBJ(const std::string& fileName, const ObjImportOptions& options, const std::string& shapeName, NiShape* mergeShape, const ImportChoices& choices) {
	// Set reference NIF in case nothing was loaded yet
	if (!workAnim.GetRefNif())
		workAnim.SetRefNif(&workNif);

	ObjFile obj;
	obj.SetScale(Vector3(10.0f, 10.0f, 10.0f));

	if (!shapeName.empty())
		outfitName = shapeName;
	else if (outfitName.empty())
		outfitName = "New Outfit";

	size_t vertexLimit = workNif.GetVertexLimit();
	size_t triLimit = workNif.GetTriangleLimit();

	if (obj.LoadForNif(fileName, options)) {
		wxLogError("Could not load OBJ file '%s'!", fileName);
		host->ShowMessage(HostMessageLevel::Error, wxString::Format(_("Could not load OBJ file '%s'!"), fileName), _("OBJ Error"));
		return 1;
	}

	bool copyBaseSkinTrans = false;
	if (choices.copyReferenceSkinTransform) {
		copyBaseSkinTrans = baseShape && *choices.copyReferenceSkinTransform;
	}
	else if (baseShape && !workAnim.GetTransformGlobalToShape(baseShape).IsNearlyEqualTo(MatTransform())) {
		HostAnswer res = host->Ask(_("The reference shape has a skin coordinate system that is different from the global coordinate system.  Would you like to copy the reference's "
												"global-to-skin transform to the imported shapes?"),
									_("Copy skin coordinates"), true, HostAnswer::No);
		if (res == HostAnswer::Cancel)
			return 1;
		if (res == HostAnswer::Yes)
			copyBaseSkinTrans = true;
	}

	std::vector<std::string> groupNames = obj.GetGroupList();
	for (const auto& group : groupNames) {
		std::vector<Vector3> v;
		std::vector<Triangle> t;
		std::vector<Vector2> uv;
		std::vector<Vector3> n;
		if (!obj.CopyDataForGroup(group, &v, &t, &uv, &n)) {
			wxLogError("Could not copy data from OBJ file '%s'!", fileName);
			host->ShowMessage(HostMessageLevel::Error, wxString::Format(_("Could not copy data from OBJ file '%s'!"), fileName), _("OBJ Error"));
			return 3;
		}

		size_t vertCount = v.size();
		size_t triCount = t.size();

		// Skip zero size groups.
		if (vertCount == 0)
			continue;

		std::string useShapeName = group;

		if (mergeShape) {
			std::vector<Vector3> shapeVerts;
			workNif.GetVertsForShape(mergeShape, shapeVerts);
			if (shapeVerts.size() == vertCount) {
				if (AskImportChoice(*host,
									choices.updateMatchingShape,
									_("The vertex count of the selected .obj file matches the currently selected outfit shape.  Do you wish to update the current shape?  "
									"(click No to create a new shape)"),
									_("Merge or New"))) {
					if (AskImportChoice(*host, choices.updatePositions, _("Update Vertex Positions?"), _("Vertex Position Update")))
						workNif.SetVertsForShape(mergeShape, v);

					if (uv.size() == vertCount) {
						if (AskImportChoice(*host, choices.updateUVs, _("Update Texture Coordinates?"), _("UV Update")))
							workNif.SetUvsForShape(mergeShape, uv);
					}

					if (n.size() == vertCount) {
						if (AskImportChoice(*host, choices.updateNormals, _("Update Normals?"), _("Normals Update"))) {
							workNif.SetNormalsForShape(mergeShape, n);
							workNif.CalcTangentsForShape(mergeShape);
						}
					}

					return 101;
				}
			}
			if (choices.newShapeName) {
				useShapeName = *choices.newShapeName;
			}
			else {
				wxString newShapeName = wxString::FromUTF8(useShapeName);
				if (!host->GetText(_("Please specify a name for the new shape"), _("New Shape Name"), newShapeName))
					return 100;
				useShapeName = newShapeName.ToUTF8();
			}

			if (useShapeName.empty())
				return 100;
		}

		auto newShape = CreateNifShapeFromData(useShapeName, &v, &t, &uv, &n);
		if (newShape) {
			uint16_t vertCountNew = newShape->GetNumVertices();
			uint32_t triCountNew = newShape->GetNumTriangles();

			if (vertCountNew < vertCount || triCountNew < triCount) {
				host->ShowMessage(HostMessageLevel::Warning, wxString::Format(_("The vertex or triangle limit for '%s' was exceeded.\nRemaining data was dropped.\n\nVertices (current/max): %zu/%zu\nTriangles "
												"(current/max): %zu/%zu"),
											  useShapeName,
											  vertCount,
											  vertexLimit,
											  triCount,
											  triLimit),
							 _("OBJ Error"));
			}

			if (copyBaseSkinTrans)
				workAnim.SetTransformGlobalToShape(newShape, workAnim.GetTransformGlobalToShape(baseShape));
		}
	}

	return 0;
}

int OutfitProject::ExportOBJ(const std::string& fileName, const std::vector<NiShape*>& shapes, bool transToGlobal, const Vector3& scale, const Vector3& offset) {
	ObjFile obj;
	obj.SetScale(scale);
	obj.SetOffset(offset);

	for (auto& shape : shapes) {
		if (!shape)
			return 1;

		std::vector<Triangle> tris;
		if (!shape->GetTriangles(tris))
			return 2;

		const std::vector<Vector3>* verts = workNif.GetVertsForShape(shape);
		if (!verts)
			return 3;

		const std::vector<Vector2>* uvs = workNif.GetUvsForShape(shape);
		const std::vector<Vector3>* norms = workNif.GetNormalsForShape(shape);

		std::vector<Vector3> gVerts, gNorms;
		if (transToGlobal) {
			MatTransform toGlobal = workAnim.GetTransformShapeToGlobal(shape);
			gVerts.resize(verts->size());

			for (size_t i = 0; i < gVerts.size(); ++i)
				gVerts[i] = toGlobal.ApplyTransform((*verts)[i]);

			verts = &gVerts;
			if (norms) {
				gNorms.resize(norms->size());

				for (size_t i = 0; i < gNorms.size(); ++i)
					gNorms[i] = toGlobal.ApplyTransformToDir((*norms)[i]);

				norms = &gNorms;
			}
		}

		obj.AddGroup(shape->name.get(), *verts, tris, uvs ? *uvs : std::vector<Vector2>(), norms ? *norms : std::vector<Vector3>());
	}

	if (obj.Save(fileName))
		return 4;

	return 0;
}

#ifdef USE_FBXSDK
int OutfitProject::ImportFBX(const std::string& fileName, const FBXImportOptions& options, const std::string& shapeName, NiShape* mergeShape, const ImportChoices& choices) {
	// Set reference NIF in case nothing was loaded yet
	if (!workAnim.GetRefNif())
		workAnim.SetRefNif(&workNif);

	FBXWrangler fbxw;
	std::string nonRefBones;
	size_t vertexLimit = workNif.GetVertexLimit();
	size_t triLimit = workNif.GetTriangleLimit();

	bool result = fbxw.ImportScene(fileName, options);
	if (!result)
		return 2;

	bool copyBaseSkinTrans = false;
	if (choices.copyReferenceSkinTransform) {
		copyBaseSkinTrans = baseShape && *choices.copyReferenceSkinTransform;
	}
	else if (baseShape && !workAnim.GetTransformGlobalToShape(baseShape).IsNearlyEqualTo(MatTransform())) {
		HostAnswer res = host->Ask(_("The reference shape has a skin coordinate system that is different from the global coordinate system.  Would you like to copy the reference's "
												"global-to-skin transform to the imported shapes?"),
									_("Copy skin coordinates"), true, HostAnswer::No);
		if (res == HostAnswer::Cancel)
			return 1;
		if (res == HostAnswer::Yes)
			copyBaseSkinTrans = true;
	}

	if (!shapeName.empty())
		outfitName = shapeName;
	else if (outfitName.empty())
		outfitName = "New Outfit";

	std::vector<std::string> shapes;
	fbxw.GetShapeNames(shapes);
	for (auto& s : shapes) {
		FBXShape* fbxShape = fbxw.GetShape(s);
		std::string useShapeName = s;

		size_t vertCount = fbxShape->verts.size();
		size_t triCount = fbxShape->tris.size();

		if (mergeShape) {
			if (mergeShape->GetNumVertices() == vertCount) {
				if (AskImportChoice(*host,
									choices.updateMatchingShape,
									_("The vertex count of the selected .fbx file matches the currently selected outfit shape.  Do you wish to update the current shape?  "
									"(click No to create a new shape)"),
									_("Merge or New"))) {
					if (AskImportChoice(*host, choices.updatePositions, _("Update Vertex Positions?"), _("Vertex Position Update")))
						workNif.SetVertsForShape(mergeShape, fbxShape->verts);

					if (fbxShape->uvs.size() == vertCount) {
						if (AskImportChoice(*host, choices.updateUVs, _("Update Texture Coordinates?"), _("UV Update")))
							workNif.SetUvsForShape(mergeShape, fbxShape->uvs);
					}

					if (fbxShape->normals.size() == vertCount) {
						if (AskImportChoice(*host, choices.updateNormals, _("Update Normals?"), _("Normals Update"))) {
							workNif.SetNormalsForShape(mergeShape, fbxShape->normals);
							workNif.CalcTangentsForShape(mergeShape);
						}
					}

					if (AskImportChoice(*host, choices.updateWeights, _("Update Animation Weighting?"), _("Animation Weight Update")))
						for (auto& bn : fbxShape->boneNames)
							workAnim.SetWeights(mergeShape->name.get(), bn, fbxShape->boneSkin[bn].GetWeights());

					return 101;
				}
			}

			if (choices.newShapeName) {
				useShapeName = *choices.newShapeName;
			}
			else {
				wxString newShapeName = wxString::FromUTF8(useShapeName);
				if (!host->GetText(_("Please specify a name for the new shape"), _("New Shape Name"), newShapeName))
					return 100;
				useShapeName = newShapeName.ToUTF8();
			}

			if (useShapeName.empty())
				return 100;
		}

		auto newShape = CreateNifShapeFromData(useShapeName, &fbxShape->verts, &fbxShape->tris, &fbxShape->uvs, &fbxShape->normals);
		if (!newShape)
			continue;

		CreateSkinning(newShape);

		uint16_t vertCountNew = newShape->GetNumVertices();
		uint32_t triCountNew = newShape->GetNumTriangles();

		if (vertCountNew < vertCount || triCountNew < triCount) {
			host->ShowMessage(HostMessageLevel::Warning, wxString::Format(_("The vertex or triangle limit for '%s' was exceeded.\nRemaining data was dropped.\n\nVertices (current/max): %zu/%zu\nTriangles "
											"(current/max): %zu/%zu"),
										  useShapeName,
										  vertCount,
										  vertexLimit,
										  triCount,
										  triLimit),
						 _("OBJ Error"));
		}

		if (copyBaseSkinTrans)
			workAnim.SetTransformGlobalToShape(newShape, workAnim.GetTransformGlobalToShape(baseShape));

		for (auto& bn : fbxShape->boneNames) {
			if (!AnimSkeleton::getInstance().GetBonePtr(bn)) {
				// Not found in reference skeleton, use default values
				AnimSkeleton::getInstance().AddCustomBone(bn);
				//AnimBone& cstm = AnimSkeleton::getInstance().AddCustomBone(bn);
				// TODO: call SetParentBone (FbxNode::GetParent?)
				// TODO: call SetTransformBoneToParent (FbxNode::LclTranslation and LclRotation?)
				nonRefBones += bn + "\n";
			}

			workAnim.AddShapeBone(useShapeName, bn);
			workAnim.SetWeights(useShapeName, bn, fbxShape->boneSkin[bn].GetWeights());
		}

		if (!nonRefBones.empty())
			wxLogMessage("Bones in shape '%s' not found in reference skeleton:\n%s", useShapeName, nonRefBones);
	}

	return 0;
}

int OutfitProject::ExportFBX(const std::string& fileName, const std::vector<NiShape*>& shapes, bool transToGlobal) {
	FBXWrangler fbxw;
	fbxw.AddSkeleton(&AnimSkeleton::getInstance().refSkeletonNif);

	for (auto& s : shapes) {
		fbxw.AddNif(&workNif, &workAnim, transToGlobal, s);
		fbxw.AddSkinning(&workAnim, s);
	}

	return fbxw.ExportScene(fileName);
}
#endif

std::unique_ptr<std::istream> OutfitProject::GetPhysicsXmlStream(const std::string& xmlPath) {
	return GameDataStream::OpenPhysicsXml(xmlPath, activeSet.GetInputFileName());
}

std::vector<std::pair<std::string, PhysicsXmlFile>> OutfitProject::ReadPhysicsXmls(const std::string& shapeName, std::vector<std::string>* outMissing) {
	std::vector<std::pair<std::string, PhysicsXmlFile>> files;
	auto links = shapePhysicsFiles.find(shapeName);
	if (links == shapePhysicsFiles.end())
		return files;

	for (auto& xmlPath : links->second) {
		PhysicsXmlFile file;
		auto stream = GetPhysicsXmlStream(xmlPath);
		if (stream && file.Load(*stream))
			files.emplace_back(xmlPath, std::move(file));
		else if (outMissing)
			outMissing->push_back(xmlPath);
	}
	return files;
}
