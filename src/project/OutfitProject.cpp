/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "OutfitProject.h"
#include "utils/ConfigurationManager.h"
#include "ProjectNifHelpers.h"
#include "components/AddOnSliders.h"
#include "components/ClippingFixer.h"
#include "components/MaskOps.h"
#include "components/SliderDataFileUtil.h"
#include "components/SliderSetIndex.h"
#include "components/WeightNorm.h"
#include "files/FBXWrangler.h"
#include "files/GameDataStream.h"
#include "files/ObjFile.h"
#include "files/TriFile.h"
#include "files/SFMorphFile.h"
#include "utils/PlatformUtil.h"
#include "utils/StringStuff.h"
#include "utils/ProjectUtil.h"
#include "NifUtil.hpp"

#include "FSEngine/FSEngine.h"
#include "FSEngine/FSManager.h"

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

class ArchiveMaterialLoader {
public:
	static bool ReadFile(const std::string& filePath, wxMemoryBuffer& data) {
		for (FSArchiveFile* archive : FSManager::archiveList()) {
			if (!archive || !archive->hasFile(filePath))
				continue;

			wxMemoryBuffer outData;
			archive->fileContents(filePath, outData);
			if (!outData.IsEmpty()) {
				data = std::move(outData);
				return true;
			}
		}

		return false;
	}
};

} // namespace

OutfitProject::OutfitProject(IProjectHost* inHost, bool loadSkeleton) {
	static std::atomic<uint64_t> lastInstanceId{0};
	instanceId = ++lastInstanceId;

	host = inHost;
	if (!host) {
		ownHost = std::make_unique<NonInteractiveHost>();
		host = ownHost.get();
	}

	workAnim.SetRefNif(&workNif);

	if (loadSkeleton) {
		wxString skeletonError;
		if (LoadDefaultSkeletonReference(&skeletonError) != 0)
			host->ShowMessage(HostMessageLevel::Error, skeletonError, _("Error"));
	}

	auto targetGame = (TargetGame)Config.GetIntValue("TargetGame");
	if (targetGame == SKYRIM || targetGame == SKYRIMSE || targetGame == SKYRIMVR)
		mGenWeights = true;
}

std::unique_ptr<OutfitProject::Snapshot> OutfitProject::CaptureSnapshot() {
	auto snapshot = std::make_unique<Snapshot>();
	snapshot->workNif = workNif;
	snapshot->workAnim = workAnim;
	snapshot->baseDiffData = baseDiffData;
	morpher.SaveResults(snapshot->morphResults, snapshot->morphDataNames);
	snapshot->activeSet = activeSet;
	snapshot->outfitName = outfitName;
	snapshot->baseShapeName = baseShape ? baseShape->name.get() : "";
	snapshot->refProjectFile = mRefProjectFile;
	snapshot->refProjectName = mRefProjectName;
	snapshot->refShapeName = mRefShapeName;
	snapshot->shapeRefSources = shapeRefSources;
	snapshot->shapeTextures = shapeTextures;
	snapshot->shapeMaterialFiles = shapeMaterialFiles;
	snapshot->shapeSFMaterials = shapeSFMaterials;
	snapshot->shapePhysicsFiles = shapePhysicsFiles;
	snapshot->clothSources = clothData.GetAllShapeSources();
	return snapshot;
}

void OutfitProject::RestoreSnapshot(const Snapshot& snapshot) {
	morpher.UnlinkRefDiffData();
	morpher.ClearProximityCache();

	workNif = snapshot.workNif;
	workAnim = snapshot.workAnim;
	workAnim.SetRefNif(&workNif);
	baseDiffData = snapshot.baseDiffData;
	morpher.RestoreResults(snapshot.morphResults, snapshot.morphDataNames);
	activeSet = snapshot.activeSet;
	outfitName = snapshot.outfitName;
	baseShape = snapshot.baseShapeName.empty() ? nullptr : workNif.FindBlockByName<NiShape>(snapshot.baseShapeName);
	mRefProjectFile = snapshot.refProjectFile;
	mRefProjectName = snapshot.refProjectName;
	mRefShapeName = snapshot.refShapeName;
	shapeRefSources = snapshot.shapeRefSources;
	shapeTextures = snapshot.shapeTextures;
	shapeMaterialFiles = snapshot.shapeMaterialFiles;
	shapeSFMaterials = snapshot.shapeSFMaterials;
	shapePhysicsFiles = snapshot.shapePhysicsFiles;
	clothData.SetAllShapeSources(snapshot.clothSources);
}

void OutfitProject::UpdateProgress(int val, const wxString& msg) {
	host->UpdateProgress(val, msg);
}

OutfitProject::~OutfitProject() {}

void OutfitProject::SetBaseShape(NiShape* shape, const bool moveData) {
	if (baseShape && baseShape != shape && (!mRefProjectFile.empty() || !mRefProjectName.empty()))
		SetShapeReferenceSource(baseShape->name.get(), mRefProjectFile, mRefProjectName);

	if (moveData) {
		if (baseShape != shape) {
			// Copy data from base shape to regular shape
			if (baseShape) {
				std::string shapeName = baseShape->name.get();
				std::string srcTarget = ShapeToTarget(shapeName);

				// Data in another data folder, like a body's, is found by the names in its file, and add-ons of the body
				// know the shape by its target. The shape keeps both, and with them its data folders.
				const bool keepTarget = activeSet.TargetHasExternalData(srcTarget);
				const std::string newTarget = keepTarget ? srcTarget : shapeName;

				for (size_t i = 0; i < activeSet.size(); i++) {
					auto& sd = activeSet[i];
					std::string srcTargetData = sd.TargetDataName(srcTarget);

					// The outfit data keeps the reference's data name, like loading the project names it
					if (keepTarget && !srcTargetData.empty() && srcTargetData != newTarget + sd.name)
						morpher.SetResultDataName(newTarget, sd.name, srcTargetData);

					auto diff = baseDiffData.GetDiffSet(srcTargetData);
					if (diff)
						morpher.SetResultDiff(newTarget, sd.name, *diff);

					baseDiffData.ClearSet(srcTargetData);
					if (keepTarget)
						continue;

					activeSet[i].RenameTarget(srcTarget, shapeName, sd.name);

					// The data is now the shape's under its new name, which a link into the reference's file can't find
					std::string targetData = sd.TargetDataName(shapeName);
					if (!targetData.empty()) {
						sd.SetLocalData(targetData);
						if (DiffInfo* dataFile = sd.GetDataFile(targetData))
							UnlinkSliderData(*dataFile);
					}
				}

				activeSet.AddShapeTarget(shapeName, newTarget);
				if (!keepTarget)
					activeSet.Retarget(srcTarget, shapeName);
			}

			// Copy data from regular shape to base shape
			if (shape) {
				std::string shapeName = shape->name.get();
				std::string target = SliderDataTargetForShape(shape);

				for (size_t i = 0; i < activeSet.size(); i++) {
					std::string sliderName = activeSet[i].name;
					std::string targetData = activeSet[i].TargetDataName(target);
					if (targetData.empty()) {
						targetData = target + sliderName;
						activeSet[i].AddDataFile(target, target + sliderName, target + sliderName);
					}

					std::unordered_map<uint16_t, Vector3> diff;
					morpher.GetRawResultDiff(target, sliderName, diff);
					morpher.ClearResultSet(targetData);

					baseDiffData.LoadSet(targetData, target, diff);
				}
			}
		}
	}

	baseShape = shape;

	if (shape) {
		std::string refProjectFile;
		std::string refProjectName;
		if (GetShapeReferenceSource(shape->name.get(), refProjectFile, refProjectName)) {
			mRefProjectFile = refProjectFile;
			mRefProjectName = refProjectName;
		}
		else {
			mRefProjectFile.clear();
			mRefProjectName.clear();
		}

		if (ShapeSliderDataIsLocalOnly(shape->name.get())) {
			mRefProjectFile.clear();
			mRefProjectName.clear();
			SetShapeReferenceSource(shape->name.get(), "", "");
		}
		else
			SetShapeReferenceSource(shape->name.get(), mRefProjectFile, mRefProjectName);

		mRefShapeName = shape->name.get();
	}
	else {
		mRefProjectFile.clear();
		mRefProjectName.clear();
		mRefShapeName.clear();
	}
}


std::string OutfitProject::SliderSetName() {
	return activeSet.GetName();
}

std::string OutfitProject::SliderSetFileName() {
	return activeSet.GetInputFileName();
}

std::string OutfitProject::OutfitName() {
	return outfitName;
}

void OutfitProject::ReplaceForbidden(std::string& str, const char& replacer) {
	const std::string forbiddenChars = "\\/:*?\"<>|";

	std::transform(str.begin(), str.end(), str.begin(), [&forbiddenChars, &replacer](char c) { return forbiddenChars.find(c) != std::string::npos ? replacer : c; });
}

int OutfitProject::GetVertexCount(NiShape* shape) {
	if (workNif.IsValid()) {
		if (shape)
			return shape->GetNumVertices();
	}

	return 0;
}

void OutfitProject::GetLiveVerts(NiShape* shape, std::vector<Vector3>& outVerts, std::vector<Vector2>* outUVs, std::vector<Matrix3>* outPoseRotations) {
	GetMorphedVerts(shape, outVerts, outUVs);

	if (outPoseRotations)
		outPoseRotations->clear();

	if (bPose) {
		bool isSF = workNif.GetHeader().GetVersion().IsSF();
		ApplySkinningToVerts(workAnim, shape, isSF, physicsPose, outVerts, outPoseRotations);
	}
}

void OutfitProject::GetMorphedVerts(NiShape* shape, std::vector<Vector3>& outVerts, std::vector<Vector2>* outUVs) {
	workNif.GetVertsForShape(shape, outVerts);
	if (outUVs)
		workNif.GetUvsForShape(shape, *outUVs);

	std::string target = ShapeToTarget(shape->name.get());
	if (IsBaseShape(shape)) {
		for (size_t i = 0; i < activeSet.size(); i++) {
			if (activeSet[i].bShow && activeSet[i].curValue != 0.0f) {
				std::string targetData = activeSet.ShapeToDataName(i, shape->name.get());
				if (targetData.empty())
					continue;

				if (activeSet[i].bUV) {
					if (outUVs)
						baseDiffData.ApplyUVDiff(targetData, target, activeSet[i].curValue, outUVs);
				}
				else
					baseDiffData.ApplyDiff(targetData, target, activeSet[i].curValue, &outVerts);
			}
		}
	}
	else {
		for (size_t i = 0; i < activeSet.size(); i++) {
			if (activeSet[i].bShow && activeSet[i].curValue != 0.0f) {
				if (activeSet[i].bUV) {
					if (outUVs)
						morpher.ApplyResultToUVs(activeSet[i].name, target, outUVs, activeSet[i].curValue);
				}
				else
					morpher.ApplyResultToVerts(activeSet[i].name, target, &outVerts, activeSet[i].curValue);
			}
		}
	}
}

const std::string& OutfitProject::ShapeToTarget(const std::string& shapeName) {
	for (auto it = activeSet.ShapesBegin(); it != activeSet.ShapesEnd(); ++it)
		if (it->first == shapeName)
			return it->second.targetShape;

	return shapeName;
}

const std::string& OutfitProject::TargetToShape(const std::string& targetName) {
	for (auto it = activeSet.ShapesBegin(); it != activeSet.ShapesEnd(); ++it)
		if (it->second.targetShape == targetName)
			return it->first;

	return targetName;
}

std::vector<std::string> OutfitProject::GetShapeTextures(NiShape* shape) {
	std::string shapeName = shape->name.get();

	if (shapeTextures.find(shapeName) != shapeTextures.end())
		return shapeTextures[shapeName];

	return std::vector<std::string>();
}

bool OutfitProject::GetShapeMaterialFile(NiShape* shape, MaterialFile& outMatFile) {
	std::string shapeName = shape->name.get();

	if (shapeMaterialFiles.find(shapeName) != shapeMaterialFiles.end()) {
		outMatFile = shapeMaterialFiles[shapeName];
		return true;
	}

	return false;
}

std::shared_ptr<const SFLayeredMaterial> OutfitProject::GetShapeSFMaterial(NiShape* shape) {
	auto material = shapeSFMaterials.find(shape->name.get());
	if (material != shapeSFMaterials.end())
		return material->second;

	return nullptr;
}

void OutfitProject::SetTextures() {
	for (auto& s : workNif.GetShapes())
		SetTextures(s);
}

void OutfitProject::SetTextures(const std::vector<std::string>& textureFiles) {
	for (auto& s : workNif.GetShapes())
		SetTextures(s, textureFiles);
}

void OutfitProject::SetTextures(NiShape* shape, const std::vector<std::string>& textureFiles) {
	if (!shape)
		return;

	std::string shapeName = shape->name.get();
	if (shapeName.empty())
		return;

	if (textureFiles.empty()) {
		std::string texturesDir = Config["GameDataPath"];
		bool hasMat = false;
		bool hasSFMat = false;
		std::string matFile;

		const uint8_t MAX_TEXTURE_PATHS = 10;
		std::vector<std::string> texFiles(MAX_TEXTURE_PATHS);

		NiShader* shader = workNif.GetShader(shape);
		if (shader) {
			// Find material file
			if (workNif.GetHeader().GetVersion().IsFO4() || workNif.GetHeader().GetVersion().IsFO76()) {
				matFile = shader->name.get();
				if (!matFile.empty())
					hasMat = true;
			}
			else if (workNif.GetHeader().GetVersion().IsSF()) {
				matFile = shader->name.get();
				if (!matFile.empty()) {
					hasMat = true;
					hasSFMat = true;
				}
			}
		}

		shapeMaterialFiles.erase(shapeName);
		shapeSFMaterials.erase(shapeName);

		MaterialFile mat(MaterialFile::BGSM);
		if (hasMat) {
			if (hasSFMat) {
				SFLayeredMaterial sfMat;
				if (sfMaterialResolver.Resolve(matFile, texturesDir, sfMat)) {
					texFiles = sfMat.GetPrimaryTextureFiles(MAX_TEXTURE_PATHS);

					sfMat.ResolveTexturePaths([](const std::string& texFile) { return GameDataStream::FindAssetPath(texFile); });
					shapeSFMaterials[shapeName] = std::make_shared<const SFLayeredMaterial>(std::move(sfMat));
				}
				else if (shader) {
					for (int i = 0; i < MAX_TEXTURE_PATHS; i++)
						workNif.GetTextureSlot(shape, texFiles[i], i);
				}
			}
			else {
				matFile = SFMaterialResolver::NormalizeMaterialPath(matFile, "");

				// Attempt to read loose material file
				mat = MaterialFile(GameDataStream::FindAssetPath(matFile));

				if (mat.Failed()) {
					// Search for material file in archives
					wxMemoryBuffer data;
					if (ArchiveMaterialLoader::ReadFile(matFile, data)) {
						std::string content(static_cast<const char*>(data.GetData()), data.GetDataLen());
						std::istringstream contentStream(content, std::istringstream::binary);

						mat = MaterialFile(contentStream);
					}
				}

				if (!mat.Failed()) {
					if (mat.signature == MaterialFile::BGSM) {
						texFiles[0] = mat.diffuseTexture.c_str();
						texFiles[1] = mat.normalTexture.c_str();
						texFiles[2] = mat.glowTexture.c_str();
						texFiles[3] = mat.greyscaleTexture.c_str();
						texFiles[4] = mat.envmapTexture.c_str();
						texFiles[7] = mat.smoothSpecTexture.c_str();
					}
					else if (mat.signature == MaterialFile::BGEM) {
						texFiles[0] = mat.baseTexture.c_str();
						texFiles[1] = mat.fxNormalTexture.c_str();
						texFiles[3] = mat.grayscaleTexture.c_str();
						texFiles[4] = mat.fxEnvmapTexture.c_str();
						texFiles[5] = mat.envmapMaskTexture.c_str();
					}

					shapeMaterialFiles[shapeName] = std::move(mat);
				}
				else if (shader) {
					for (int i = 0; i < MAX_TEXTURE_PATHS; i++)
						workNif.GetTextureSlot(shape, texFiles[i], i);
				}
			}
		}
		else if (shader) {
			for (int i = 0; i < MAX_TEXTURE_PATHS; i++)
				workNif.GetTextureSlot(shape, texFiles[i], i);
		}

		for (int i = 0; i < MAX_TEXTURE_PATHS; i++) {
			if (!texFiles[i].empty()) {
				texFiles[i] = GameDataStream::FindAssetPath(SFMaterialResolver::NormalizeTexturePath(texFiles[i]));
			}
		}

		shapeTextures[shapeName] = texFiles;
	}
	else {
		shapeTextures[shapeName] = textureFiles;

		// Textures picked by hand replace the ones the first layer of a Starfield material renders
		// with, and leave the rest of the material as it was
		if (workNif.GetHeader().GetVersion().IsSF()) {
			SFLayeredMaterial sfMat;
			auto existing = shapeSFMaterials.find(shapeName);
			if (existing != shapeSFMaterials.end() && existing->second)
				sfMat = *existing->second;

			if (sfMat.layers.empty())
				sfMat.layers.emplace_back();

			auto& layerFiles = sfMat.layers[0].textureSet.files;
			for (size_t i = 0; i < std::min<size_t>(textureFiles.size(), 8); i++)
				layerFiles[i] = textureFiles[i];

			shapeSFMaterials[shapeName] = std::make_shared<const SFLayeredMaterial>(std::move(sfMat));
		}
	}
}

bool OutfitProject::IsValidShape(const std::string& shapeName) {
	for (auto& s : workNif.GetShapeNames())
		if (s == shapeName)
			return true;

	return false;
}

void OutfitProject::RefreshMorphShape(NiShape* shape) {
	morpher.UpdateMeshFromNif(workNif, shape->name.get());
}

Vector3 OutfitProject::InversePoseDiff(int vertIndex, const Vector3& diffNif, AnimSkin& animSkin, const MatTransform& globalToSkin) {
	Matrix3 blendedMat(0, 0, 0, 0, 0, 0, 0, 0, 0);
	float totalWeight = 0;

	for (auto& boneNameIt : animSkin.boneNames) {
		AnimBone* animB = GetSkeleton().GetBonePtr(boneNameIt.first);
		if (!animB)
			continue;

		AnimWeight& animW = animSkin.boneWeights[boneNameIt.second];
		auto wIt = animW.weights.find(vertIndex);
		if (wIt == animW.weights.end())
			continue;

		float w = wIt->second;
		MatTransform xform = globalToSkin.ComposeTransforms(animB->xformPoseToGlobal.ComposeTransforms(animW.xformSkinToBone));

		for (int r = 0; r < 3; r++)
			for (int c = 0; c < 3; c++)
				blendedMat[r][c] += w * xform.rotation[r][c] * xform.scale;

		totalWeight += w;
	}

	if (totalWeight >= EPSILON) {
		Matrix3 invMat;
		if (blendedMat.Invert(&invMat))
			return invMat * diffNif;
	}

	return diffNif;
}

void OutfitProject::UpdateShapeFromMesh(NiShape* shape, const Mesh* m) {
	if (bPose) {
		// When posed, m->verts are in posed mesh space. We need to compute
		// the delta between current posed verts and original posed positions,
		// then un-pose those deltas and apply them to the NIF rest positions.
		std::vector<Vector3> restVerts;
		workNif.GetVertsForShape(shape, restVerts);

		std::vector<Vector3> posedVerts;
		GetLiveVerts(shape, posedVerts);

		AnimSkin& animSkin = workAnim.shapeSkinning[shape->name.get()];
		MatTransform globalToSkin = workAnim.GetTransformGlobalToShape(shape);

		for (int i = 0; i < m->nVerts; i++) {
			Vector3 meshVertNif = Mesh::TransformPosMeshToNif(m->verts[i]);
			Vector3 deltaPosed = meshVertNif - posedVerts[i];

			if (deltaPosed.IsZero())
				continue;

			restVerts[i] += InversePoseDiff(i, deltaPosed, animSkin, globalToSkin);
		}

		workNif.SetVertsForShape(shape, restVerts);
	}
	else {
		std::vector<Vector3> liveVerts(m->nVerts);

		for (int i = 0; i < m->nVerts; i++)
			liveVerts[i] = Mesh::TransformPosMeshToNif(m->verts[i]);

		workNif.SetVertsForShape(shape, liveVerts);
	}
}

void OutfitProject::UndoPoseDiffs(NiShape* shape, std::unordered_map<uint16_t, Vector3>& diffs) {
	if (!bPose)
		return;

	AnimSkin& animSkin = workAnim.shapeSkinning[shape->name.get()];
	MatTransform globalToSkin = workAnim.GetTransformGlobalToShape(shape);

	for (auto& d : diffs) {
		Vector3 diffNif = Mesh::TransformDiffMeshToNif(d.second);
		diffNif = InversePoseDiff(d.first, diffNif, animSkin, globalToSkin);
		d.second = Mesh::TransformDiffNifToMesh(diffNif);
	}
}

void OutfitProject::ComputeUndoRestDiffs(NiShape* shape, UndoStateShape& uss) {
	if (!uss.restDiffs.empty())
		return;

	AnimSkin& animSkin = workAnim.shapeSkinning[shape->name.get()];
	MatTransform globalToSkin = workAnim.GetTransformGlobalToShape(shape);

	for (auto& ps : uss.pointStartState) {
		auto pe = uss.pointEndState.find(ps.first);
		if (pe == uss.pointEndState.end())
			continue;

		Vector3 diffMesh = pe->second - ps.second;
		Vector3 diffNif = Mesh::TransformDiffMeshToNif(diffMesh);
		if (bPose)
			diffNif = InversePoseDiff(ps.first, diffNif, animSkin, globalToSkin);
		uss.restDiffs[ps.first] = diffNif;
	}
}

void OutfitProject::MoveVertex(NiShape* shape, const Vector3& pos, const int& id) {
	workNif.MoveVertex(shape, pos, id);
}

void OutfitProject::OffsetShape(NiShape* shape, const Vector3& xlate, std::unordered_map<uint16_t, float>* mask) {
	workNif.OffsetShape(shape, xlate, mask);
}

void OutfitProject::ScaleShape(NiShape* shape, const Vector3& scale, std::unordered_map<uint16_t, float>* mask) {
	workNif.ScaleShape(shape, scale, mask);
}

void OutfitProject::RotateShape(NiShape* shape, const Vector3& angle, std::unordered_map<uint16_t, float>* mask) {
	workNif.RotateShape(shape, angle, mask);
}

void OutfitProject::ApplyTransformToShapeGeometry(NiShape* shape, const MatTransform& t) {
	if (!shape)
		return;

	// Vertices
	const std::vector<Vector3>* oldVerts = workNif.GetVertsForShape(shape);
	if (!oldVerts || oldVerts->empty())
		return;

	size_t nVerts = oldVerts->size();
	std::vector<Vector3> verts(nVerts);
	for (size_t i = 0; i < nVerts; ++i)
		verts[i] = t.ApplyTransform((*oldVerts)[i]);

	workNif.SetVertsForShape(shape, verts);

	// Position diffs
	for (size_t si = 0; si < activeSet.size(); ++si) {
		SliderData& sd = activeSet[si];
		if (sd.bUV || sd.bClamp || sd.bZap)
			continue;

		std::unordered_map<uint16_t, Vector3>* diffSet = GetDiffSet(sd,  shape);
		if (!diffSet)
			continue;

		for (auto& diffp : *diffSet)
			diffp.second = t.ApplyTransformToDiff(diffp.second);
	}

	// Normals
	if (t.rotation.IsNearlyEqualTo(Matrix3()))
		return;

	const std::vector<Vector3>* oldNorms = workNif.GetNormalsForShape(shape);
	if (!oldNorms || oldNorms->size() != nVerts)
		return;

	std::vector<Vector3> norms(nVerts);
	for (size_t i = 0; i < nVerts; ++i)
		norms[i] = t.ApplyTransformToDir((*oldNorms)[i]);

	workNif.SetNormalsForShape(shape, norms);
}

bool OutfitProject::ApplyShapeTransformToGeometry(NiShape* shape) {
	if (!shape)
		return false;

	MatTransform oldShapeToGlobal = workAnim.GetTransformShapeToGlobal(shape);
	MatTransform newShapeToGlobal;
	if (oldShapeToGlobal.IsNearlyEqualTo(newShapeToGlobal))
		return false;

	// Same as the shape properties dialog with the option to recalculate the
	// geometry's coordinates: the vertices are moved by the old transform, so
	// they stay in place once the transform is cleared.
	ApplyTransformToShapeGeometry(shape, newShapeToGlobal.InverseTransform().ComposeTransforms(oldShapeToGlobal));
	workAnim.SetTransformShapeToGlobal(shape, newShapeToGlobal);
	return true;
}

int OutfitProject::CopySegPart(NiShape* shape) {
	// Gather baseShape's triangles and vertices
	std::vector<Triangle> tris;
	baseShape->GetTriangles(tris);
	std::vector<Vector3> verts;
	workNif.GetVertsForShape(baseShape, verts);

	// Transform baseShape vertices to nif global coordinates
	MatTransform xformToGlobal = workAnim.GetTransformShapeToGlobal(baseShape);
	for (Vector3& v : verts)
		v = xformToGlobal.ApplyTransform(v);

	// Calculate center of each triangle (times 3)
	std::vector<Vector3> tricenters(tris.size());
	for (size_t ti = 0; ti < tris.size(); ++ti)
		tricenters[ti] = verts[tris[ti].p1] + verts[tris[ti].p2] + verts[tris[ti].p3];

	// Build proximity cache for triangle centers of baseShape.
	// Note that kd_tree keeps pointers into tricenters.
	kd_tree<uint32_t> refTree(&tricenters[0], tricenters.size());

	// Get baseShape's segment/partition data
	std::vector<int> bstriParts;
	NifSegmentationInfo inf;
	bool gotsegs = workNif.GetShapeSegments(baseShape, inf, bstriParts);
	NiVector<BSDismemberSkinInstance::PartitionInfo> partitionInfo;
	bool gotparts = false;
	if (!gotsegs)
		gotparts = workNif.GetShapePartitions(baseShape, partitionInfo, bstriParts);

	// Gather shape's triangles and vertices
	shape->GetTriangles(tris);
	workNif.GetVertsForShape(shape, verts);

	// Transform shape's vertices to nif global coordinates
	xformToGlobal = workAnim.GetTransformShapeToGlobal(shape);
	for (Vector3& v : verts)
		v = xformToGlobal.ApplyTransform(v);

	// Calculate new partition/segment for each triangle
	std::vector<int> triParts(tris.size());
	int failcount = 0;
	for (size_t ti = 0; ti < tris.size(); ++ti) {
		// Calculate center of triangle (times 3)
		Vector3 tricenter = verts[tris[ti].p1] + verts[tris[ti].p2] + verts[tris[ti].p3];

		// Find closest triangle center in proximity cache.
		uint32_t resultcount = refTree.kd_nn(&tricenter, 0);
		if (resultcount < 1) {
			++failcount;
			triParts[ti] = -1;
			continue;
		}

		// Look up closest triangle and copy its partition/segment ID.
		size_t bti = refTree.queryResult[0].vertex_index;
		triParts[ti] = bstriParts[bti];
		if (triParts[ti] == -1)
			++failcount;
	}

	// Refuse to continue if we have any triangles without segments/partitions.
	if (failcount)
		return failcount;

	// Store new information in NIF.
	if (gotsegs)
		workNif.SetShapeSegments(shape, inf, triParts);

	if (gotparts) {
		workNif.SetShapePartitions(shape, partitionInfo, triParts);
		workNif.RemoveEmptyPartitions(shape);
	}

	return 0;
}

NiShape* OutfitProject::DuplicateShape(NiShape* sourceShape, const std::string& destShapeName) {
	if (!sourceShape)
		return nullptr;

	workAnim.CloneShape(&workNif, sourceShape, destShapeName);

	auto newShape = workNif.CloneShape(sourceShape, destShapeName);
	if (newShape) {
		clothData.SetShapeSources(destShapeName, clothData.GetShapeSources(sourceShape->name.get()));

		// The copy is saved with the same bones per vertex
		if (uint8_t locked = GetLockedBonesPerVertex(sourceShape))
			SetLockedBonesPerVertex(newShape, locked);
	}

	std::string shapeName = sourceShape->name.get();
	std::string srcTarget = ShapeToTarget(shapeName);

	if (IsBaseShape(sourceShape)) {
		for (size_t i = 0; i < activeSet.size(); i++) {
			auto& sd = activeSet[i];
			std::string srcTargetData = sd.TargetDataName(srcTarget);
			if (srcTargetData.empty())
				srcTargetData = baseDiffData.GetDataTargetName(srcTarget, sd.name);

			auto diff = baseDiffData.GetDiffSet(srcTargetData);
			if (diff)
				morpher.SetResultDiff(destShapeName, sd.name, *diff);
		}
	}
	else {
		morpher.CopyShape(shapeName, destShapeName);

		for (size_t i = 0; i < activeSet.size(); i++) {
			auto& sd = activeSet[i];
			std::string oldDataName = sd.TargetDataName(srcTarget);
			if (oldDataName.empty())
				oldDataName = morpher.GetDataTargetName(srcTarget, sd.name);

			if (!oldDataName.empty()) {
				std::string newDataName = "";

				if (oldDataName == srcTarget + sd.name)
					newDataName = destShapeName + sd.name;
				else if (StringStartsWith(oldDataName, srcTarget))
					newDataName = destShapeName + oldDataName.substr(srcTarget.length());
				else if (StringEndsWith(oldDataName, sd.name))
					newDataName = destShapeName + sd.name;

				if (!newDataName.empty())
					morpher.CopySet(oldDataName, newDataName, destShapeName);
			}
		}
	}

	return newShape;
}

void OutfitProject::DeleteShape(NiShape* shape) {
	if (!shape)
		return;

	std::string shapeName = shape->name.get();
	workAnim.ClearShape(shapeName);
	host->OnShapeDeleting(shape);
	shapeRefSources.erase(shapeName);
	shapeTextures.erase(shapeName);
	shapeMaterialFiles.erase(shapeName);
	shapeSFMaterials.erase(shapeName);
	shapePhysicsFiles.erase(shapeName);
	clothData.RemoveShape(shapeName);

	if (IsBaseShape(shape)) {
		morpher.UnlinkRefDiffData();
		baseShape = nullptr;
		mRefProjectFile.clear();
		mRefProjectName.clear();
		mRefShapeName.clear();
	}

	workNif.DeleteShape(shape);
}

void OutfitProject::CapturePhysicsFiles(NifFile& nif, const std::vector<NiShape*>& shapes) {
	// The game only reads the physics link from the model's root node, so that
	// is where it usually sits, but extra data further down the hierarchy is
	// honored here as well.
	auto collect = [&nif](NiObjectNET* obj, std::vector<std::string>& outFiles) {
		for (auto& extraDataRef : obj->extraDataRefs) {
			auto stringExtraData = nif.GetHeader().GetBlock<NiStringExtraData>(extraDataRef);
			if (!stringExtraData || !IsPhysicsExtraData(stringExtraData))
				continue;

			const std::string& xmlPath = stringExtraData->stringData.get();
			if (!xmlPath.empty() && std::find(outFiles.begin(), outFiles.end(), xmlPath) == outFiles.end())
				outFiles.push_back(xmlPath);
		}
	};

	for (auto& shape : shapes) {
		if (!shape)
			continue;

		std::vector<std::string> physicsFiles;
		collect(shape, physicsFiles);

		for (NiNode* node = nif.GetParentNode(shape); node; node = nif.GetParentNode(node))
			collect(node, physicsFiles);

		// Always overwrite: re-importing a file must not keep links of a shape
		// of the same name that no longer has any.
		std::string shapeName = shape->name.get();
		if (physicsFiles.empty())
			shapePhysicsFiles.erase(shapeName);
		else
			shapePhysicsFiles[shapeName] = std::move(physicsFiles);
	}
}

void OutfitProject::CaptureRootPhysicsData(NifFile& srcNif) {
	for (auto* physicsExtraData : GetRootPhysicsExtraData(srcNif)) {
		const std::string xmlPath = ToLower(physicsExtraData->stringData.get());
		if (xmlPath.empty())
			continue;

		// The same file linked by two sources is one choice, not two
		bool known = false;
		for (auto& knownData : rootPhysicsData) {
			if (ToLower(knownData->stringData.get()) == xmlPath) {
				known = true;
				break;
			}
		}

		if (!known)
			rootPhysicsData.push_back(physicsExtraData->Clone());
	}
}

void OutfitProject::MergeRootExtraData(NifFile& srcNif) {
	auto srcRoot = srcNif.GetRootNode();
	auto destRoot = workNif.GetRootNode();
	if (!srcRoot || !destRoot)
		return;

	// Extra data is looked up by name, so only one block of a type and name can
	// ever be read. A loaded file that brings its own is offered as a
	// replacement for the one the project already has.
	auto findMatch = [](NiExtraData* extraData, NiExtraData* other) {
		return other->GetBlockName() == std::string(extraData->GetBlockName()) && ToLower(other->name.get()) == ToLower(extraData->name.get());
	};

	auto findLoaded = [this, destRoot, &findMatch](NiExtraData* extraData) -> NiExtraData* {
		for (auto& extraDataRef : destRoot->extraDataRefs) {
			auto existing = workNif.GetHeader().GetBlock<NiExtraData>(extraDataRef);
			if (existing && findMatch(extraData, existing))
				return existing;
		}

		return nullptr;
	};

	// Only the plain value types are compared, anything else is offered in case it differs
	auto sameContent = [](NiExtraData* extraData, NiExtraData* other) {
		if (auto integerData = dynamic_cast<NiIntegerExtraData*>(extraData))
			return integerData->integerData == static_cast<NiIntegerExtraData*>(other)->integerData;
		if (auto stringData = dynamic_cast<NiStringExtraData*>(extraData))
			return stringData->stringData.get() == static_cast<NiStringExtraData*>(other)->stringData.get();
		if (auto floatData = dynamic_cast<NiFloatExtraData*>(extraData))
			return floatData->floatData == static_cast<NiFloatExtraData*>(other)->floatData;
		if (auto booleanData = dynamic_cast<NiBooleanExtraData*>(extraData))
			return booleanData->booleanData == static_cast<NiBooleanExtraData*>(other)->booleanData;
		return false;
	};

	std::vector<RootExtraDataCandidate> merging;
	for (auto& extraDataRef : srcRoot->extraDataRefs) {
		auto extraData = srcNif.GetHeader().GetBlock<NiExtraData>(extraDataRef);
		if (!extraData)
			continue;

		// Cloth and physics data is collected per source and picked on save
		if (dynamic_cast<BSClothExtraData*>(extraData) || IsPhysicsExtraData(extraData))
			continue;

		// A file listing the same name twice is offered once
		bool duplicate = std::any_of(merging.begin(), merging.end(), [&](const RootExtraDataCandidate& candidate) {
			return findMatch(extraData, candidate.source);
		});

		if (duplicate)
			continue;

		// Replacing a block with an identical one changes nothing, so it isn't offered
		auto loaded = findLoaded(extraData);
		if (loaded && sameContent(extraData, loaded))
			continue;

		merging.push_back({extraData, loaded});
	}

	if (merging.empty())
		return;

	if (!ChooseRootExtraData(merging))
		return;

	for (auto& candidate : merging) {
		// Making room for the replacement shifts block indices around, so the
		// id of the block on its way out is resolved right before it goes
		if (candidate.replaces)
			workNif.GetHeader().DeleteBlock(workNif.GetBlockID(candidate.replaces));

		auto clonedExtraData = candidate.source->Clone();
		auto clonedBlock = clonedExtraData.get();
		workNif.AssignExtraData(destRoot, std::move(clonedExtraData));

		// The clone's string indices still point into the source header
		RemapExtraDataStrings(workNif, clonedBlock);
	}
}

bool OutfitProject::ChooseRootExtraData(std::vector<RootExtraDataCandidate>& merging) {
	auto keepChosen = [&merging](const wxArrayInt& sel) {
		std::vector<RootExtraDataCandidate> chosen;
		for (size_t i = 0; i < sel.Count(); i++)
			chosen.push_back(merging[sel[i]]);

		merging = std::move(chosen);
	};

	wxArrayString extraDataNames;
	wxArrayInt preSelected;
	bool anyReplaced = false;

	for (auto& candidate : merging) {
		wxString label = wxString::Format("%s: %s", candidate.source->GetBlockName(), wxString::FromUTF8(candidate.source->name.get()));

		auto stringExtraData = dynamic_cast<NiStringExtraData*>(candidate.source);
		if (stringExtraData)
			label += " = " + wxString::FromUTF8(stringExtraData->stringData.get());

		// Replacing what the project already has is never the default
		if (candidate.replaces) {
			label += _(" (replaces the loaded one)");
			anyReplaced = true;
		}
		else
			preSelected.Add(static_cast<int>(extraDataNames.GetCount()));

		extraDataNames.Add(label);
	}

	// Unattended runs get the default answer of "everything but the replacements"
	if (!host->IsInteractive()) {
		keepChosen(preSelected);
		return true;
	}

	wxString message = _("The root node of the loaded file has extra data for the root node of the project. Please choose all the entries to merge.");
	if (anyReplaced)
		message += "\n \n" + _("The entries that aren't checked already exist in the project under the same name. Only one block of a name can be used, so merging one of them replaces the one that is currently loaded.");

	wxArrayInt selections = preSelected;
	if (!host->ChooseMany(message, _("Choose extra data"), extraDataNames, selections))
		return false;

	keepChosen(selections);
	return true;
}

void OutfitProject::CaptureShapeDeleteState(NiShape* shape, UndoStateShapeDelete& state) {
	if (!shape)
		return;

	state.shapeName = shape->name.get();
	state.wasBaseShape = IsBaseShape(shape);
	state.clothSources = clothData.GetShapeSources(state.shapeName);

	// Capture reference info so it can be restored on undo
	if (state.wasBaseShape) {
		state.refProjectFile = mRefProjectFile;
		state.refProjectName = mRefProjectName;
		state.refShapeName = mRefShapeName;
	}

	// Clone shape and all child blocks into a temporary NIF
	state.nifBackup.Create(workNif.GetHeader().GetVersion());
	state.nifBackup.CloneShape(shape, state.shapeName, &workNif);

	// The working NIF's skinning isn't updated before export, so take the editor's
	auto bones = workAnim.shapeBones.find(state.shapeName);
	auto skin = workAnim.shapeSkinning.find(state.shapeName);
	state.hadSkinning = bones != workAnim.shapeBones.end() && skin != workAnim.shapeSkinning.end();
	if (state.hadSkinning) {
		state.bones = bones->second;
		state.skin = skin->second;
	}

	// Capture slider diffs
	std::string target = ShapeToTarget(state.shapeName);
	for (size_t si = 0; si < activeSet.size(); si++) {
		SliderData& sd = activeSet[si];
		std::string targetDataName = sd.TargetDataName(target);
		if (targetDataName.empty())
			targetDataName = target + sd.name;

		TargetDataDiffs* diffSet = nullptr;
		if (state.wasBaseShape)
			diffSet = baseDiffData.GetDiffSet(targetDataName);
		else
			diffSet = morpher.GetDiffSet(targetDataName);

		if (diffSet && !diffSet->empty()) {
			UndoStateShapeSliderDiff ssd;
			ssd.sliderName = sd.name;
			ssd.targetDataName = targetDataName;
			ssd.diffs = *diffSet;
			state.sliderDiffs.push_back(std::move(ssd));
		}
	}

	// Capture texture and material references
	auto tex = shapeTextures.find(state.shapeName);
	if (tex != shapeTextures.end())
		state.textures = tex->second;

	auto mat = shapeMaterialFiles.find(state.shapeName);
	if (mat != shapeMaterialFiles.end())
		state.materialFile = mat->second;

	auto sfMat = shapeSFMaterials.find(state.shapeName);
	if (sfMat != shapeSFMaterials.end())
		state.sfMaterial = sfMat->second;

	auto physics = shapePhysicsFiles.find(state.shapeName);
	if (physics != shapePhysicsFiles.end())
		state.physicsFiles = physics->second;
}

NiShape* OutfitProject::RestoreDeletedShape(UndoStateShapeDelete& state) {
	// Find the backup shape in the temp NIF
	auto backupShapes = state.nifBackup.GetShapes();
	if (backupShapes.empty())
		return nullptr;

	NiShape* backupShape = backupShapes.front();

	// Rename if a shape with the same name already exists
	std::string restoreName = state.shapeName;
	if (workNif.FindBlockByName<NiShape>(restoreName)) {
		int suffix = 2;
		std::string candidate;
		do {
			candidate = restoreName + "_" + std::to_string(suffix++);
		} while (workNif.FindBlockByName<NiShape>(candidate));

		wxLogMessage("Restoring shape '%s' as '%s' (name conflict).", restoreName, candidate);
		restoreName = candidate;
	}

	state.shapeName = restoreName;

	// Clone shape back into working NIF
	auto restoredShape = workNif.CloneShape(backupShape, restoreName, &state.nifBackup);
	if (!restoredShape)
		return nullptr;

	clothData.SetShapeSources(restoreName, state.clothSources);

	// Loading registers custom bones the skeleton doesn't know, the captured skinning then replaces the stale NIF weights
	workAnim.LoadFromNif(&workNif, restoredShape);
	if (state.hadSkinning)
		workAnim.SetShapeSkin(restoreName, state.bones, state.skin);

	// Restore slider diffs
	std::string target = ShapeToTarget(state.shapeName);
	bool restoreAsBase = state.wasBaseShape && baseShape == nullptr;

	for (auto& ssd : state.sliderDiffs) {
		if (restoreAsBase)
			baseDiffData.LoadSet(ssd.targetDataName, target, ssd.diffs);
		else
			morpher.SetResultDiff(target, ssd.sliderName, ssd.diffs);
	}

	// Restore texture and material references
	if (!state.textures.empty())
		shapeTextures[state.shapeName] = state.textures;

	if (state.materialFile.has_value())
		shapeMaterialFiles[state.shapeName] = state.materialFile.value();

	if (state.sfMaterial)
		shapeSFMaterials[state.shapeName] = state.sfMaterial;

	if (!state.physicsFiles.empty())
		shapePhysicsFiles[state.shapeName] = state.physicsFiles;

	// Restore base shape status if appropriate
	if (restoreAsBase) {
		baseShape = restoredShape;
		morpher.LinkRefDiffData(&baseDiffData);
		mRefProjectFile = state.refProjectFile;
		mRefProjectName = state.refProjectName;
		mRefShapeName = state.refShapeName;
	}

	return restoredShape;
}

void OutfitProject::RenameShape(NiShape* shape, const std::string& newShapeName) {
	std::string shapeName = shape->name.get();
	std::string oldTarget = ShapeToTarget(shapeName);

	workNif.RenameShape(shape, newShapeName);
	clothData.RenameShape(shapeName, newShapeName);
	workAnim.RenameShape(shapeName, newShapeName);

	auto refSource = shapeRefSources.find(shapeName);
	if (refSource != shapeRefSources.end()) {
		auto source = refSource->second;
		shapeRefSources.erase(refSource);
		shapeRefSources[newShapeName] = source;
	}

	bool isBaseShape = IsBaseShape(shape);
	for (size_t si = 0; si < activeSet.size(); ++si) {
		SliderData& sd = activeSet[si];
		std::string oldDataName = sd.TargetDataName(oldTarget);
		if (oldDataName.empty()) {
			if (isBaseShape)
				oldDataName = baseDiffData.GetDataTargetName(oldTarget, sd.name);
			else
				oldDataName = morpher.GetDataTargetName(oldTarget, sd.name);
		}

		if (!oldDataName.empty()) {
			std::string newDataName = "";

			if (oldDataName == oldTarget + sd.name)
				newDataName = newShapeName + sd.name;
			else if (StringStartsWith(oldDataName, oldTarget))
				newDataName = newShapeName + oldDataName.substr(oldTarget.length());
			else if (StringEndsWith(oldDataName, sd.name))
				newDataName = newShapeName + sd.name;

			if (!newDataName.empty()) {
				if (isBaseShape)
					baseDiffData.RenameSet(oldDataName, newDataName);
				else
					morpher.RenameSet(oldDataName, newDataName);
			}
		}
	}

	activeSet.RenameShape(shapeName, newShapeName);

	// The base project's data is for the old name, so it becomes the project's own
	std::string newTarget = ShapeToTarget(newShapeName);
	for (size_t si = 0; si < activeSet.size(); ++si)
		for (auto& dataFile : activeSet[si].dataFiles)
			if (dataFile.targetName == newTarget)
				DetachSliderData(dataFile);

	auto tex = shapeTextures.find(shapeName);
	if (tex != shapeTextures.end()) {
		auto value = tex->second;
		shapeTextures.erase(tex);
		shapeTextures[newShapeName] = value;
	}

	auto mat = shapeMaterialFiles.find(shapeName);
	if (mat != shapeMaterialFiles.end()) {
		auto value = mat->second;
		shapeMaterialFiles.erase(mat);
		shapeMaterialFiles[newShapeName] = value;
	}

	auto sfMat = shapeSFMaterials.find(shapeName);
	if (sfMat != shapeSFMaterials.end()) {
		auto value = sfMat->second;
		shapeSFMaterials.erase(sfMat);
		shapeSFMaterials[newShapeName] = value;
	}

	auto physics = shapePhysicsFiles.find(shapeName);
	if (physics != shapePhysicsFiles.end()) {
		auto value = physics->second;
		shapePhysicsFiles.erase(physics);
		shapePhysicsFiles[newShapeName] = value;
	}

	if (isBaseShape) {
		baseDiffData.RenameDataTarget(oldTarget, newShapeName);
		activeSet.SetReferencedData(newShapeName, true);
	}
	else {
		morpher.RenameDataTarget(oldTarget, newShapeName);
		morpher.RenameShape(shapeName, newShapeName);
	}

	wxLogMessage("Renamed shape '%s' to '%s'.", shapeName, newShapeName);
}

void OutfitProject::UpdateNifNormals(NifFile* nif, const std::vector<Mesh*>& shapeMeshes) {
	std::vector<Vector3> liveNorms;
	for (auto& m : shapeMeshes) {
		auto shape = nif->FindBlockByName<NiShape>(m->shapeName);
		if (shape) {
			if (nif->GetHeader().GetVersion().IsSK() || nif->GetHeader().GetVersion().IsSSE()) {
				NiShader* shader = nif->GetShader(shape);
				if (shader && shader->IsModelSpace())
					continue;
			}

			liveNorms.clear();
			for (int i = 0; i < m->nVerts; i++)
				liveNorms.push_back(Mesh::TransformDirMeshToNif(m->norms[i]));

			nif->SetNormalsForShape(shape, liveNorms);
			nif->CalcTangentsForShape(shape);
		}
	}
}

void OutfitProject::ValidateNIF(NifFile& nif, const std::string& nifFilePath) {
	auto targetGame = (TargetGame)Config.GetIntValue("TargetGame");
	bool match = false;

	switch (targetGame) {
		case OB: match = nif.GetHeader().GetVersion().IsOB(); break;
		case FO3:
		case FONV: match = nif.GetHeader().GetVersion().IsFO3(); break;
		case SKYRIM: match = nif.GetHeader().GetVersion().IsSK(); break;
		case FO4:
		case FO4VR: match = nif.GetHeader().GetVersion().IsFO4(); break;
		case SKYRIMSE:
		case SKYRIMVR: match = nif.GetHeader().GetVersion().IsSSE(); break;
		case FO76: match = nif.GetHeader().GetVersion().IsFO76(); break;
		case SF: match = nif.GetHeader().GetVersion().IsSF(); break;
	}

	if (!match) {
		if ((targetGame == SKYRIMSE || targetGame == SKYRIMVR) && nif.GetHeader().GetVersion().IsSK()) {
			bool optimize = true;
			if (Config.Exists("OptimizeForSSE"))
				optimize = Config["OptimizeForSSE"] == "true";
			else if (host->IsInteractive()) {
				HostAnswer res = host->Ask(_("Would you like Skyrim NIFs to be optimized for SSE during this session?"), _("Target Game"), false, HostAnswer::Yes);
				optimize = res == HostAnswer::Yes;
				Config.SetDefaultBoolValue("OptimizeForSSE", optimize);
			}
			else {
				// Unattended runs optimize the file, but leave the choice for the session to the user
				wxLogMessage("Optimizing Skyrim NIF '%s' for SSE.", nifFilePath);
			}

			if (optimize) {
				OptOptions options;
				options.targetVersion.SetFile(V20_2_0_7);
				options.targetVersion.SetUser(12);
				options.targetVersion.SetStream(100);
				nif.OptimizeFor(options);
			}
		}
		else {
			wxLogWarning("Version of NIF file doesn't match current target game.");
			host->ShowMessage(HostMessageLevel::Warning, wxString::Format(_("File format doesn't match the current game. Use FBX export, then start a new project and import the FBX file there.")),
						 _("Version"));
		}
	}

	for (const auto& meshPath : GameDataStream::LoadExternalGeometry(nif, Config["GameDataPath"], nifFilePath))
		host->ShowMessage(HostMessageLevel::Warning, wxString::Format(_("Unable to locate external mesh data for shape. Expected path: %s"), meshPath),
					 _("External Mesh Data Load Failure"));

	for (auto& s : nif.GetShapes())
		nif.TriangulateShape(s);
}

void OutfitProject::ResetTransforms() {
	bool clearRoot = false;
	bool unskinnedFound = false;

	for (auto& s : workNif.GetShapes()) {
		if (s->IsSkinned()) {
			/*
			 * Root node, shape and global-to-skin transform aren't rendered directly for skinned meshes.
			 * They only affect different things, e.g. bounds and the global-to-parent transforms.
			 *
			 * By clearing these and recalculating bounds on export we make sure that
			 * nothing but the individual bone transforms affect visuals.
			 */

			if (!unskinnedFound)
				clearRoot = true;

			MatTransform oldXformGlobalToSkin = workAnim.shapeSkinning[s->name.get()].xformGlobalToSkin;
			MatTransform newXformGlobalToSkin;

			// Apply global-to-skin transform to vertices
			if (!newXformGlobalToSkin.IsNearlyEqualTo(oldXformGlobalToSkin)) {
				ApplyTransformToShapeGeometry(s, newXformGlobalToSkin.ComposeTransforms(oldXformGlobalToSkin.InverseTransform()));

				workAnim.ChangeGlobalToSkinTransform(s->name.get(), newXformGlobalToSkin);
				workNif.SetShapeTransformGlobalToSkin(s, newXformGlobalToSkin);
			}

			// Clear shape transform
			s->SetTransformToParent(MatTransform());

			// Clear global-to-skin transform
			workAnim.shapeSkinning[s->name.get()].xformGlobalToSkin.Clear();
			workNif.SetShapeTransformGlobalToSkin(s, MatTransform());
		}
		else {
			clearRoot = false;
			unskinnedFound = true;
		}
	}
 
	if (clearRoot) {
		// Clear root node transform
		auto rootNode = workNif.GetRootNode();
		if (rootNode)
			rootNode->SetTransformToParent(MatTransform());
	}
}

bool OutfitProject::AddShader(NiShape* s) {
	if (workNif.GetShader(s))
		return false;

	auto targetGame = (TargetGame)Config.GetIntValue("TargetGame");
	auto& hdr = workNif.GetHeader();

	switch (targetGame) {
		case OB: s->propertyRefs.AddBlockRef(hdr.AddBlock(std::make_unique<NiMaterialProperty>())); break;

		case FO3:
		case FONV:
			s->propertyRefs.AddBlockRef(hdr.AddBlock(std::make_unique<BSShaderPPLightingProperty>()));
			s->propertyRefs.AddBlockRef(hdr.AddBlock(std::make_unique<NiMaterialProperty>()));
			break;

		default:
			auto shaderPropertyRef = s->ShaderPropertyRef();
			if (shaderPropertyRef)
				shaderPropertyRef->index = hdr.AddBlock(std::make_unique<BSLightingShaderProperty>(hdr.GetVersion()));
	}

	NiShader* shader = workNif.GetShader(s);
	if (!shader)
		return false;

	auto textureSetRef = shader->TextureSetRef();
	if (textureSetRef)
		textureSetRef->index = hdr.AddBlock(std::make_unique<BSShaderTextureSet>(hdr.GetVersion()));

	// Skinned shapes need the skinned shader flag, like CreateSkinning sets it
	shader->SetSkinned(s->IsSkinned());
	return true;
}
