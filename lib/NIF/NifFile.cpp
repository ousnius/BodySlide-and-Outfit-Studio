/*
BodySlide and Outfit Studio
Copyright (C) 2017  Caliente & ousnius
See the included LICENSE file
*/

#include "NifFile.h"

#include <set>
#include <queue>
#include <regex>


NiFactoryRegister& NiFactoryRegister::GetNiFactoryRegister() {
	static NiFactoryRegister instance;
	return instance;
}

NiFactoryRegister::NiFactoryRegister() {
	RegisterFactory<NiNode>();
	RegisterFactory<BSFadeNode>();
	RegisterFactory<BSValueNode>();
	RegisterFactory<BSLeafAnimNode>();
	RegisterFactory<BSTreeNode>();
	RegisterFactory<BSOrderedNode>();
	RegisterFactory<BSMultiBoundNode>();
	RegisterFactory<BSBlastNode>();
	RegisterFactory<BSDamageStage>();
	RegisterFactory<BSMasterParticleSystem>();
	RegisterFactory<NiBillboardNode>();
	RegisterFactory<NiSwitchNode>();
	RegisterFactory<NiTriShape>();
	RegisterFactory<NiTriShapeData>();
	RegisterFactory<NiTriStrips>();
	RegisterFactory<NiTriStripsData>();
	RegisterFactory<BSLODTriShape>();
	RegisterFactory<BSTriShape>();
	RegisterFactory<BSSubIndexTriShape>();
	RegisterFactory<BSMeshLODTriShape>();
	RegisterFactory<BSDynamicTriShape>();
	RegisterFactory<NiSkinInstance>();
	RegisterFactory<BSDismemberSkinInstance>();
	RegisterFactory<NiSkinData>();
	RegisterFactory<NiSkinPartition>();
	RegisterFactory<BSSkinInstance>();
	RegisterFactory<BSSkinBoneData>();
	RegisterFactory<BSShaderPPLightingProperty>();
	RegisterFactory<BSLightingShaderProperty>();
	RegisterFactory<BSEffectShaderProperty>();
	RegisterFactory<BSWaterShaderProperty>();
	RegisterFactory<BSSkyShaderProperty>();
	RegisterFactory<NiAlphaProperty>();
	RegisterFactory<NiMaterialProperty>();
	RegisterFactory<NiStencilProperty>();
	RegisterFactory<BSShaderTextureSet>();
	RegisterFactory<NiParticleSystem>();
	RegisterFactory<NiMeshParticleSystem>();
	RegisterFactory<BSStripParticleSystem>();
	RegisterFactory<NiParticlesData>();
	RegisterFactory<NiRotatingParticlesData>();
	RegisterFactory<NiPSysData>();
	RegisterFactory<NiMeshPSysData>();
	RegisterFactory<BSStripPSysData>();
	RegisterFactory<NiCamera>();
	RegisterFactory<BSPSysStripUpdateModifier>();
	RegisterFactory<NiPSysAgeDeathModifier>();
	RegisterFactory<BSPSysLODModifier>();
	RegisterFactory<NiPSysSpawnModifier>();
	RegisterFactory<BSPSysSimpleColorModifier>();
	RegisterFactory<NiPSysRotationModifier>();
	RegisterFactory<BSPSysScaleModifier>();
	RegisterFactory<NiPSysGravityModifier>();
	RegisterFactory<NiPSysPositionModifier>();
	RegisterFactory<NiPSysBoundUpdateModifier>();
	RegisterFactory<NiPSysDragModifier>();
	RegisterFactory<BSPSysInheritVelocityModifier>();
	RegisterFactory<BSPSysSubTexModifier>();
	RegisterFactory<NiPSysBombModifier>();
	RegisterFactory<BSWindModifier>();
	RegisterFactory<BSPSysRecycleBoundModifier>();
	RegisterFactory<BSPSysHavokUpdateModifier>();
	RegisterFactory<NiPSysSphericalCollider>();
	RegisterFactory<NiPSysPlanarCollider>();
	RegisterFactory<NiPSysColliderManager>();
	RegisterFactory<NiPSysSphereEmitter>();
	RegisterFactory<NiPSysCylinderEmitter>();
	RegisterFactory<NiPSysBoxEmitter>();
	RegisterFactory<NiPSysMeshEmitter>();
	RegisterFactory<BSLightingShaderPropertyColorController>();
	RegisterFactory<BSLightingShaderPropertyFloatController>();
	RegisterFactory<BSEffectShaderPropertyColorController>();
	RegisterFactory<BSEffectShaderPropertyFloatController>();
	RegisterFactory<BSFrustumFOVController>();
	RegisterFactory<BSLagBoneController>();
	RegisterFactory<BSProceduralLightningController>();
	RegisterFactory<NiBoneLODController>();
	RegisterFactory<NiFloatExtraDataController>();
	RegisterFactory<NiVisController>();
	RegisterFactory<NiAlphaController>();
	RegisterFactory<BSNiAlphaPropertyTestRefController>();
	RegisterFactory<NiKeyframeController>();
	RegisterFactory<NiTransformController>();
	RegisterFactory<NiMultiTargetTransformController>();
	RegisterFactory<NiPSysModifierActiveCtlr>();
	RegisterFactory<NiPSysEmitterLifeSpanCtlr>();
	RegisterFactory<NiPSysEmitterSpeedCtlr>();
	RegisterFactory<NiPSysEmitterInitialRadiusCtlr>();
	RegisterFactory<NiPSysEmitterPlanarAngleCtlr>();
	RegisterFactory<NiPSysEmitterDeclinationCtlr>();
	RegisterFactory<NiPSysGravityStrengthCtlr>();
	RegisterFactory<NiPSysInitialRotSpeedCtlr>();
	RegisterFactory<NiPSysEmitterCtlr>();
	RegisterFactory<BSPSysMultiTargetEmitterCtlr>();
	RegisterFactory<NiControllerManager>();
	RegisterFactory<NiSequence>();
	RegisterFactory<NiControllerSequence>();
	RegisterFactory<NiDefaultAVObjectPalette>();
	RegisterFactory<NiBlendBoolInterpolator>();
	RegisterFactory<NiBlendFloatInterpolator>();
	RegisterFactory<NiBlendPoint3Interpolator>();
	RegisterFactory<NiBoolInterpolator>();
	RegisterFactory<NiBoolTimelineInterpolator>();
	RegisterFactory<NiFloatInterpolator>();
	RegisterFactory<NiTransformInterpolator>();
	RegisterFactory<NiPoint3Interpolator>();
	RegisterFactory<NiPathInterpolator>();
	RegisterFactory<NiLookAtInterpolator>();
	RegisterFactory<NiPSysUpdateCtlr>();
	RegisterFactory<NiKeyframeData>();
	RegisterFactory<NiTransformData>();
	RegisterFactory<NiPosData>();
	RegisterFactory<NiBoolData>();
	RegisterFactory<NiFloatData>();
	RegisterFactory<NiBinaryExtraData>();
	RegisterFactory<NiFloatExtraData>();
	RegisterFactory<NiStringExtraData>();
	RegisterFactory<NiStringsExtraData>();
	RegisterFactory<NiBooleanExtraData>();
	RegisterFactory<NiIntegerExtraData>();
	RegisterFactory<BSXFlags>();
	RegisterFactory<BSInvMarker>();
	RegisterFactory<BSFurnitureMarkerNode>();
	RegisterFactory<BSDecalPlacementVectorExtraData>();
	RegisterFactory<BSBehaviorGraphExtraData>();
	RegisterFactory<BSBound>();
	RegisterFactory<BSBoneLODExtraData>();
	RegisterFactory<NiTextKeyExtraData>();
	RegisterFactory<BSClothExtraData>();
	RegisterFactory<BSConnectPointParents>();
	RegisterFactory<BSConnectPointChildren>();
	RegisterFactory<BSMultiBound>();
	RegisterFactory<BSMultiBoundOBB>();
	RegisterFactory<BSMultiBoundAABB>();
	RegisterFactory<NiCollisionObject>();
	RegisterFactory<bhkCollisionObject>();
	RegisterFactory<bhkNPCollisionObject>();
	RegisterFactory<bhkPCollisionObject>();
	RegisterFactory<bhkSPCollisionObject>();
	RegisterFactory<bhkBlendCollisionObject>();
	RegisterFactory<bhkPhysicsSystem>();
	RegisterFactory<bhkPlaneShape>();
	RegisterFactory<bhkConvexVerticesShape>();
	RegisterFactory<bhkBoxShape>();
	RegisterFactory<bhkSphereShape>();
	RegisterFactory<bhkTransformShape>();
	RegisterFactory<bhkConvexTransformShape>();
	RegisterFactory<bhkCapsuleShape>();
	RegisterFactory<bhkNiTriStripsShape>();
	RegisterFactory<bhkListShape>();
	RegisterFactory<bhkSimpleShapePhantom>();
	RegisterFactory<bhkHingeConstraint>();
	RegisterFactory<bhkLimitedHingeConstraint>();
	RegisterFactory<bhkRagdollConstraint>();
	RegisterFactory<bhkBreakableConstraint>();
	RegisterFactory<bhkStiffSpringConstraint>();
	RegisterFactory<bhkBallAndSocketConstraint>();
	RegisterFactory<bhkBallSocketConstraintChain>();
	RegisterFactory<bhkRigidBody>();
	RegisterFactory<bhkRigidBodyT>();
	RegisterFactory<bhkCompressedMeshShape>();
	RegisterFactory<bhkCompressedMeshShapeData>();
	RegisterFactory<bhkMoppBvTreeShape>();
}


NiShape* NifFile::FindShapeByName(const std::string& name, int dupIndex) {
	int numFound = 0;
	for (auto& block : blocks) {
		auto geom = dynamic_cast<NiShape*>(block);
		if (geom && !name.compare(geom->GetName())) {
			if (numFound >= dupIndex)
				return geom;

			numFound++;
		}
	}
	return nullptr;
}

NiAVObject* NifFile::FindAVObjectByName(const std::string& name, int dupIndex) {
	int numFound = 0;
	for (auto& block : blocks) {
		auto avo = dynamic_cast<NiAVObject*>(block);
		if (avo && !name.compare(avo->GetName())) {
			if (numFound >= dupIndex)
				return avo;

			numFound++;
		}
	}
	return nullptr;
}

NiNode* NifFile::FindNodeByName(const std::string& name) {
	for (auto& block : blocks) {
		auto node = dynamic_cast<NiNode*>(block);
		if (node && !name.compare(node->GetName()))
			return node;
	}
	return nullptr;
}

int NifFile::GetBlockID(NiObject* block) {
	if (block != nullptr) {
		auto it = find(blocks.begin(), blocks.end(), block);
		if (it != blocks.end())
			return distance(blocks.begin(), it);
	}

	return 0xFFFFFFFF;
}

NiNode* NifFile::GetParentNode(NiObject* childBlock) {
	if (childBlock != nullptr) {
		int childId = GetBlockID(childBlock);
		for (auto &block : blocks) {
			auto node = dynamic_cast<NiNode*>(block);
			if (node) {
				auto children = node->GetChildren();
				for (auto it = children.begin(); it < children.end(); it++) {
					if (childId == it->index)
						return node;
				}
			}
		}
	}

	return nullptr;
}

void NifFile::CopyFrom(const NifFile& other) {
	if (isValid)
		Clear();

	isValid = other.isValid;
	hasUnknown = other.hasUnknown;
	fileName = other.fileName;

	hdr = new NiHeader(*other.hdr);

	size_t nBlocks = other.blocks.size();
	blocks.resize(nBlocks);

	for (int i = 0; i < nBlocks; i++) {
		blocks[i] = other.blocks[i]->Clone();
		blocks[i]->header = hdr;
	}

	hdr->SetBlockReference(&blocks);
}

void NifFile::Clear() {
	isValid = false;
	hasUnknown = false;

	for (int i = 0; i < blocks.size(); i++)
		delete blocks[i];

	blocks.clear();
	hdr->Clear();
}

int NifFile::Load(const std::string& filename) {
	Clear();

	std::fstream file(filename.c_str(), std::ios_base::in | std::ios_base::binary);
	if (file.is_open()) {
		if (filename.rfind("\\") != std::string::npos)
			fileName = filename.substr(filename.rfind("\\"));
		else
			fileName = filename;

		hdr->Get(file);
		if (!hdr->IsValid()) {
			Clear();
			return 1;
		}

		if (!(hdr->VerCheck(20, 2, 0, 7) && (hdr->GetUserVersion() == 11 || hdr->GetUserVersion() == 12))) {
			Clear();
			return 2;
		}

		uint nBlocks = hdr->GetNumBlocks();
		blocks.resize(nBlocks);

		auto& nifactories = NiFactoryRegister::GetNiFactoryRegister();
		for (int i = 0; i < nBlocks; i++) {
			NiObject* block = nullptr;
			std::string blockTypeStr = hdr->GetBlockTypeStringById(i);

			auto nifactory = nifactories.GetFactoryByName(blockTypeStr);
			if (nifactory) {
				block = nifactory->Load(file, hdr);
			}
			else {
				hasUnknown = true;
				block = (NiObject*)new NiUnknown(file, hdr->GetBlockSize(i));
			}

			if (block)
				blocks[i] = block;
		}

		hdr->SetBlockReference(&blocks);
		file.close();
	}
	else {
		Clear();
		return 1;
	}

	TrimTexturePaths();
	PrepareData();
	isValid = true;

	return 0;
}

void NifFile::SetShapeOrder(const std::vector<std::string>& order) {
	if (hasUnknown)
		return;

	std::vector<int> delta;
	bool hadoffset = false;

	// Have to do this in multiple passes
	do {
		std::vector<std::string> oldOrder;
		GetShapeList(oldOrder);

		std::vector<int> oldOrderIds;
		for (auto s : oldOrder) {
			int blockID = GetBlockID(FindShapeByName(s));
			if (blockID != 0xFFFFFFFF)
				oldOrderIds.push_back(blockID);
		}

		if (order.size() != oldOrder.size())
			return;

		// Get movement offset for each item.  This is the difference between old and new position.
		delta.clear();
		delta.resize(order.size());

		for (int p = 0; p < oldOrder.size(); p++)
			delta[p] = (find(order.begin(), order.end(), oldOrder[p]) - order.begin()) - p;

		hadoffset = false;
		//Positive offsets mean that the item has moved down the list.  By necessity, that means another item has moved up the list. 
		// thus, we only need to move the "rising" items, the other blocks will naturally end up in the right place.  

		// find first negative delta, and raise it in list.  The first item can't have a negative delta 
		for (int i = 1; i < delta.size(); i++) {
			// don't move positive or zero offset items.
			if (delta[i] >= 0)
				continue;

			hadoffset = true;
			int c = 0 - delta[i];
			int p = i;
			while (c > 0) {
				hdr->SwapBlocks(oldOrderIds[p], oldOrderIds[p - 1]);
				p--;
				c--;
			}
			break;
		}

	} while (hadoffset);
}

void NifFile::PrettySortBlocks() {
	if (hasUnknown)
		return;

	auto root = dynamic_cast<NiNode*>(blocks[0]);
	if (!root)
		return;

	std::vector<int> oldChildren(root->GetNumChildren());
	for (int i = 0; i < oldChildren.size(); i++)
		oldChildren[i] = root->GetChildRef(i);

	root->ClearChildren();

	for (int i = 0; i < hdr->GetNumBlocks(); i++)
		if (find(oldChildren.begin(), oldChildren.end(), i) != oldChildren.end())
			root->AddChildRef(i);

	auto& children = root->GetChildren();
	auto bookmark = children.begin();
	auto peek = children.begin();

	for (int i = 0; peek < children.end(); i++) {
		auto block = hdr->GetBlock<NiObject>(root->GetChildRef(i));
		if (block && block->HasType<NiShape>()) {
			iter_swap(bookmark, peek);
			bookmark++;
		}
		peek++;
	}
}

bool NifFile::DeleteUnreferencedBlocks() {
	if (hasUnknown)
		return false;

	bool hadDeletions = false;
	hdr->DeleteUnreferencedBlocks(&hadDeletions);
	return hadDeletions;
}

int NifFile::RemoveUnusedStrings() {
	if (hasUnknown)
		return 0;

	return hdr->RemoveUnusedStrings();
}

int NifFile::AddNode(const std::string& nodeName, std::vector<Vector3>& rot, Vector3& trans, float scale) {
	auto root = dynamic_cast<NiNode*>(blocks[0]);
	if (!root)
		return 0xFFFFFFFF;

	auto newNode = new NiNode(hdr);
	newNode->rotation[0] = rot[0];
	newNode->rotation[1] = rot[1];
	newNode->rotation[2] = rot[2];
	newNode->translation = trans;
	newNode->scale = scale;
	newNode->SetName(nodeName);

	int newNodeId = hdr->AddBlock(newNode);
	if (newNodeId != 0xFFFFFFFF)
		root->AddChildRef(newNodeId);

	return newNodeId;
}

void NifFile::DeleteNode(const std::string& nodeName) {
	hdr->DeleteBlock(GetBlockID(FindNodeByName(nodeName)));
}

std::string NifFile::GetNodeName(const int blockID) {
	std::string name;

	auto n = hdr->GetBlock<NiNode>(blockID);
	if (n) {
		name = n->GetName();
		if (name.empty())
			name = "_unnamed_";
	}

	return name;
}

void NifFile::SetNodeName(const int blockID, const std::string& newName) {
	auto node = hdr->GetBlock<NiNode>(blockID);
	if (!node)
		return;

	node->SetName(newName, true);
}

int NifFile::AssignExtraData(const std::string& blockName, const int extraDataId, bool isNode) {
	if (extraDataId != 0xFFFFFFFF) {
		if (isNode) {
			// Assign to node
			NiNode* node = FindNodeByName(blockName);
			if (!node)
				return 0xFFFFFFFF;

			node->AddExtraDataRef(extraDataId);
		}
		else {
			// Assign to geometry
			NiAVObject* geom = FindAVObjectByName(blockName);
			if (!geom)
				return 0xFFFFFFFF;

			geom->AddExtraDataRef(extraDataId);
		}
	}
	return extraDataId;
}

int NifFile::AddStringExtraData(const std::string& blockName, const std::string& name, const std::string& stringData, bool isNode) {
	auto strExtraData = new NiStringExtraData(hdr);
	strExtraData->SetName(name);
	strExtraData->SetStringData(stringData);

	return AssignExtraData(blockName, hdr->AddBlock(strExtraData), isNode);
}

int NifFile::AddIntegerExtraData(const std::string& blockName, const std::string& name, const int integerData, bool isNode) {
	auto intExtraData = new NiIntegerExtraData(hdr);
	intExtraData->SetName(name);
	intExtraData->SetIntegerData(integerData);

	return AssignExtraData(blockName, hdr->AddBlock(intExtraData), isNode);
}

NiShader* NifFile::GetShader(const std::string& shapeName) {
	int prop1 = 0xFFFFFFFF;

	NiShape* shape = FindShapeByName(shapeName);
	if (shape)
		prop1 = shape->GetShaderPropertyRef();

	if (prop1 != 0xFFFFFFFF) {
		auto shader = hdr->GetBlock<NiShader>(prop1);
		if (shader)
			return shader;
	}
	else {
		std::vector<int> props = shape->propertyRefs.GetIndices();
		if (props.empty())
			return nullptr;

		for (int i = 0; i < props.size(); i++) {
			auto shader = hdr->GetBlock<NiShader>(props[i]);
			if (shader)
				return shader;
		}
	}

	return nullptr;
}

bool NifFile::IsShaderSkin(const std::string& shapeName) {
	NiShader* shader = GetShader(shapeName);
	if (shader)
		return shader->IsSkinTint();

	return false;
}

NiMaterialProperty* NifFile::GetMaterialProperty(const std::string& shapeName) {
	NiShape* shape = FindShapeByName(shapeName);
	if (!shape)
		return nullptr;

	for (int i = 0; i < shape->propertyRefs.GetSize(); i++) {
		auto material = hdr->GetBlock<NiMaterialProperty>(shape->propertyRefs.GetBlockRef(i));
		if (material)
			return material;
	}

	return nullptr;
}

int NifFile::GetTextureForShape(const std::string& shapeName, std::string& outTexFile, int texIndex) {
	int textureSetRef = 0xFFFFFFFF;
	outTexFile.clear();

	NiShader* shader = GetShader(shapeName);
	if (shader)
		textureSetRef = shader->GetTextureSetRef();
	else
		return 0;

	if (textureSetRef == 0xFFFFFFFF) {
		auto effectShader = dynamic_cast<BSEffectShaderProperty*>(shader);
		if (effectShader) {
			switch (texIndex) {
			case 0:
				outTexFile = effectShader->sourceTexture.GetString();
				break;
			case 1:
				outTexFile = effectShader->normalTexture.GetString();
				break;
			case 3:
				outTexFile = effectShader->greyscaleTexture.GetString();
				break;
			case 4:
				outTexFile = effectShader->envMapTexture.GetString();
				break;
			case 5:
				outTexFile = effectShader->envMaskTexture.GetString();
				break;
			}

			return 2;
		}
		else
			return 0;
	}

	auto textureSet = hdr->GetBlock<BSShaderTextureSet>(textureSetRef);
	if (!textureSet || texIndex + 1 > textureSet->numTextures)
		return 0;

	outTexFile = textureSet->textures[texIndex].GetString();
	return 1;
}

void NifFile::SetTextureForShape(const std::string& shapeName, std::string& outTexFile, int texIndex) {
	int textureSetRef = 0xFFFFFFFF;

	NiShader* shader = GetShader(shapeName);
	if (shader)
		textureSetRef = shader->GetTextureSetRef();
	else
		return;

	if (textureSetRef == 0xFFFFFFFF) {
		auto effectShader = dynamic_cast<BSEffectShaderProperty*>(shader);
		if (effectShader) {
			switch (texIndex) {
			case 0:
				effectShader->sourceTexture.SetString(outTexFile);
				break;
			case 1:
				effectShader->normalTexture.SetString(outTexFile);
				break;
			case 3:
				effectShader->greyscaleTexture.SetString(outTexFile);
				break;
			case 4:
				effectShader->envMapTexture.SetString(outTexFile);
				break;
			case 5:
				effectShader->envMaskTexture.SetString(outTexFile);
				break;
			}
		}

		return;
	}

	auto textureSet = hdr->GetBlock<BSShaderTextureSet>(textureSetRef);
	if (!textureSet || texIndex + 1 > textureSet->numTextures)
		return;

	textureSet->textures[texIndex].SetString(outTexFile);
}

void NifFile::TrimTexturePaths() {
	std::string tFile;
	std::vector<std::string> shapes;
	GetShapeList(shapes);

	for (auto &s : shapes) {
		for (int i = 0; i < 10; i++) {
			if (GetTextureForShape(s, tFile, i) && !tFile.empty()) {
				tFile = std::regex_replace(tFile, std::regex("/+|\\\\+"), "\\");													// Replace multiple slashes or forward slashes with one backslash
				tFile = std::regex_replace(tFile, std::regex("^(.*?)\\\\textures\\\\", std::regex_constants::icase), "");			// Remove everything before the first occurence of "\textures\"
				tFile = std::regex_replace(tFile, std::regex("^\\\\+"), "");														// Remove all backslashes from the front
				tFile = std::regex_replace(tFile, std::regex("^(?!^textures\\\\)", std::regex_constants::icase), "textures\\");		// If the path doesn't start with "textures\", add it to the front
				SetTextureForShape(s, tFile, i);
			}
		}
	}
}

void NifFile::CopyController(NiShader* destShader, NiShader* srcShader) {
	if (!destShader)
		return;

	if (!srcShader) {
		destShader->SetControllerRef(0xFFFFFFFF);
		return;
	}

	int shaderId = GetBlockID(destShader);
	int nextControllerRef = srcShader->GetControllerRef();

	auto srcController = srcShader->header->GetBlock<NiTimeController>(nextControllerRef);
	if (srcController) {
		// Copy first controller
		NiTimeController* destController = static_cast<NiTimeController*>(srcController->Clone());
		int controllerId = hdr->AddBlock(destController);

		destController->targetRef.index = shaderId;
		destShader->SetControllerRef(controllerId);

		CopyInterpolators(destController, srcController);
		nextControllerRef = srcController->nextControllerRef.index;

		// Recursive copy for next controller references
		while (nextControllerRef != 0xFFFFFFFF) {
			srcController = srcShader->header->GetBlock<NiTimeController>(nextControllerRef);
			if (srcController) {
				NiTimeController* destControllerRec = static_cast<NiTimeController*>(srcController->Clone());
				controllerId = hdr->AddBlock(destControllerRec);

				destControllerRec->targetRef.index = shaderId;

				// Assign new controller to previous controller
				destController->nextControllerRef.index = controllerId;
				destController = destControllerRec;

				CopyInterpolators(destControllerRec, srcController);
				nextControllerRef = srcController->nextControllerRef.index;
			}
			else
				nextControllerRef = 0xFFFFFFFF;
		}
	}
}

void NifFile::CopyInterpolators(NiTimeController* destController, NiTimeController* srcController) {
	if (!destController || !srcController)
		return;

	// Copy interpolators linked in different types of controllers
	if (srcController->HasType<BSFrustumFOVController>()) {
		auto srcCtrlrType = static_cast<BSFrustumFOVController*>(srcController);
		auto destCtrlrType = static_cast<BSFrustumFOVController*>(destController);
		destCtrlrType->interpolatorRef.index = CopyInterpolator(destController->header, srcController->header, srcCtrlrType->interpolatorRef.index);
	}
	else if (srcController->HasType<BSProceduralLightningController>()) {
		auto srcCtrlrType = static_cast<BSProceduralLightningController*>(srcController);
		auto destCtrlrType = static_cast<BSProceduralLightningController*>(destController);
		destCtrlrType->generationInterpRef.index = CopyInterpolator(destController->header, srcController->header, srcCtrlrType->generationInterpRef.index);
		destCtrlrType->mutationInterpRef.index = CopyInterpolator(destController->header, srcController->header, srcCtrlrType->generationInterpRef.index);
		destCtrlrType->subdivisionInterpRef.index = CopyInterpolator(destController->header, srcController->header, srcCtrlrType->generationInterpRef.index);
		destCtrlrType->numBranchesInterpRef.index = CopyInterpolator(destController->header, srcController->header, srcCtrlrType->generationInterpRef.index);
		destCtrlrType->numBranchesVarInterpRef.index = CopyInterpolator(destController->header, srcController->header, srcCtrlrType->generationInterpRef.index);
		destCtrlrType->lengthInterpRef.index = CopyInterpolator(destController->header, srcController->header, srcCtrlrType->generationInterpRef.index);
		destCtrlrType->lengthVarInterpRef.index = CopyInterpolator(destController->header, srcController->header, srcCtrlrType->generationInterpRef.index);
		destCtrlrType->widthInterpRef.index = CopyInterpolator(destController->header, srcController->header, srcCtrlrType->generationInterpRef.index);
		destCtrlrType->arcOffsetInterpRef.index = CopyInterpolator(destController->header, srcController->header, srcCtrlrType->generationInterpRef.index);
	}
	
	auto niSingleInterp = dynamic_cast<NiSingleInterpController*>(srcController);
	if (niSingleInterp)
		niSingleInterp->interpolatorRef.index = CopyInterpolator(destController->header, srcController->header, niSingleInterp->interpolatorRef.index);
}

int NifFile::CopyInterpolator(NiHeader* destHeader, NiHeader* srcHeader, int srcInterpId) {
	auto srcInterp = srcHeader->GetBlock<NiInterpolator>(srcInterpId);
	if (srcInterp) {
		auto destInterp = static_cast<NiInterpolator*>(srcInterp->Clone());
		destInterp->header = destHeader;

		// Copy data of interpolators as well
		if (destInterp->HasType<NiBoolInterpolator>()) {
			auto srcInterpType = static_cast<NiBoolInterpolator*>(srcInterp);
			auto srcData = srcHeader->GetBlock<NiBoolData>(srcInterpType->dataRef.index);
			if (srcData) {
				auto destData = static_cast<NiBoolData*>(srcData->Clone());
				auto destInterpType = static_cast<NiBoolInterpolator*>(destInterp);
				destInterpType->dataRef.index = hdr->AddBlock(destData);
			}
		}
		else if (destInterp->HasType<NiFloatInterpolator>()) {
			auto srcInterpType = static_cast<NiFloatInterpolator*>(srcInterp);
			auto srcData = srcHeader->GetBlock<NiFloatData>(srcInterpType->dataRef.index);
			if (srcData) {
				auto destData = static_cast<NiFloatData*>(srcData->Clone());
				auto destInterpType = static_cast<NiFloatInterpolator*>(destInterp);
				destInterpType->dataRef.index = hdr->AddBlock(destData);
			}
		}
		else if (destInterp->HasType<NiTransformInterpolator>()) {
			auto srcInterpType = static_cast<NiTransformInterpolator*>(srcInterp);
			auto srcData = srcHeader->GetBlock<NiTransformData>(srcInterpType->dataRef.index);
			if (srcData) {
				auto destData = static_cast<NiTransformData*>(srcData->Clone());
				auto destInterpType = static_cast<NiTransformInterpolator*>(destInterp);
				destInterpType->dataRef.index = hdr->AddBlock(destData);
			}
		}
		else if (destInterp->HasType<NiPoint3Interpolator>()) {
			auto srcInterpType = static_cast<NiPoint3Interpolator*>(srcInterp);
			auto srcData = srcHeader->GetBlock<NiPosData>(srcInterpType->dataRef.index);
			if (srcData) {
				auto destData = static_cast<NiPosData*>(srcData->Clone());
				auto destInterpType = static_cast<NiPoint3Interpolator*>(destInterp);
				destInterpType->dataRef.index = hdr->AddBlock(destData);
			}
		}

		return hdr->AddBlock(destInterp);
	}

	return 0xFFFFFFFF;
}

void NifFile::CopyShader(const std::string& shapeDest, NifFile& srcNif) {
	NiShape* shape = FindShapeByName(shapeDest);
	if (!shape)
		return;

	int srcShaderRef = 0xFFFFFFFF;
	int propRef1 = 0xFFFFFFFF;
	int propRef2 = 0xFFFFFFFF;

	auto srcShader = srcNif.hdr->GetBlock<NiShader>(shape->GetShaderPropertyRef());
	if (!srcShader) {
		// No shader found, look in other properties
		for (int i = 0; i < shape->propertyRefs.GetSize(); i++) {
			srcShader = srcNif.hdr->GetBlock<NiShader>(shape->propertyRefs.GetBlockRef(i));
			if (srcShader) {
				srcShaderRef = shape->propertyRefs.GetBlockRef(i);
				propRef1 = i;
				break;
			}
		}

		// Still no shader found, return
		if (!srcShader)
			return;
	}
	else
		srcShaderRef = shape->GetShaderPropertyRef();

	auto srcAlphaProp = srcNif.hdr->GetBlock<NiAlphaProperty>(shape->GetAlphaPropertyRef());
	if (!srcAlphaProp) {
		// No alpha found, look in other properties
		for (int i = 0; i < shape->propertyRefs.GetSize(); i++) {
			srcAlphaProp = srcNif.hdr->GetBlock<NiAlphaProperty>(shape->propertyRefs.GetBlockRef(i));
			if (srcAlphaProp) {
				propRef2 = i;
				break;
			}
		}
	}

	// Clone shader from source
	NiShader* destShader = static_cast<NiShader*>(srcShader->Clone());
	destShader->header = hdr;
	if (hdr->GetUserVersion() == 12 && hdr->GetUserVersion2() >= 120)
		destShader->SetName(srcShader->GetName());
	else
		destShader->ClearName();

	// Add shader block to nif
	int shaderId = hdr->AddBlock(destShader);

	// Extra Data
	for (int i = 0; i < srcShader->GetNumExtraData(); i++) {
		auto srcExtraData = srcNif.hdr->GetBlock<NiExtraData>(srcShader->GetExtraDataRef(i));
		if (srcExtraData) {
			auto destExtraData = static_cast<NiExtraData*>(srcExtraData->Clone());
			destExtraData->header = hdr;
			destExtraData->SetName(srcExtraData->GetName());

			int extraDataId = hdr->AddBlock(destExtraData);
			destShader->SetExtraDataRef(i, extraDataId);

			if (destExtraData->HasType<NiStringExtraData>()) {
				auto strSrcExtraData = dynamic_cast<NiStringExtraData*>(srcExtraData);
				auto strDestExtraData = dynamic_cast<NiStringExtraData*>(destExtraData);
				if (strSrcExtraData && strDestExtraData)
					strDestExtraData->SetStringData(strSrcExtraData->GetStringData());
			}
		}
	}

	// Controller
	CopyController(destShader, srcShader);

	auto srcTexSet = srcNif.hdr->GetBlock<BSShaderTextureSet>(srcShader->GetTextureSetRef());
	if (srcTexSet) {
		// Create texture set block and copy
		auto destTexSet = srcTexSet->Clone();
		destTexSet->header = hdr;

		// Add texture block to nif
		int texSetId = hdr->AddBlock(destTexSet);

		// Assign texture set block id to shader
		destShader->SetTextureSetRef(texSetId);
	}

	// Wet Material
	std::string srcWetMaterialName = srcShader->GetWetMaterialName();
	if (!srcWetMaterialName.empty())
		destShader->SetWetMaterialName(srcWetMaterialName);

	if (srcShader->HasType<BSShaderPPLightingProperty>())
		shape->propertyRefs.SetBlockRef(propRef1, shaderId);
	else
		shape->SetShaderPropertyRef(shaderId);

	// Kill normals and tangents
	if (destShader->IsSkinTint() && hdr->GetUserVersion() >= 12) {
		if (shape->HasType<NiTriBasedGeom>()) {
			shape->SetNormals(false);
			shape->SetTangents(false);
		}
	}

	// Create alpha property and copy
	if (srcAlphaProp) {
		auto destAlphaProp = srcAlphaProp->Clone();
		destAlphaProp->header = hdr;

		if (hdr->GetUserVersion() == 12 && hdr->GetUserVersion2() >= 120)
			destAlphaProp->SetName(srcAlphaProp->GetName());
		else
			destAlphaProp->ClearName();

		int alphaPropId = hdr->AddBlock(destAlphaProp);
		if (srcShader->HasType<BSShaderPPLightingProperty>())
			shape->propertyRefs.SetBlockRef(propRef2, alphaPropId);
		else
			shape->SetAlphaPropertyRef(alphaPropId);
	}
}

int NifFile::CopyNamedNode(std::string& nodeName, NifFile& srcNif) {
	NiNode* srcNode = srcNif.FindNodeByName(nodeName);
	if (!srcNode)
		return 0xFFFFFFFF;

	auto destNode = srcNode->Clone();
	destNode->header = hdr;
	destNode->SetName(nodeName);

	return hdr->AddBlock(destNode);
}

void NifFile::CopyGeometry(const std::string& shapeDest, NifFile& srcNif, const std::string& srcShape) {
	NiShape* srcGeom = srcNif.FindShapeByName(srcShape);
	if (!srcGeom)
		return;

	// Shape
	NiShape* destGeom = static_cast<NiShape*>(srcGeom->Clone());
	destGeom->header = hdr;
	destGeom->SetName(shapeDest);

	int destId = hdr->AddBlock(destGeom);

	// Extra Data
	for (int i = 0; i < srcGeom->GetNumExtraData(); i++) {
		auto srcExtraData = srcNif.hdr->GetBlock<NiExtraData>(srcGeom->GetExtraDataRef(i));
		if (srcExtraData) {
			auto destExtraData = static_cast<NiExtraData*>(srcExtraData->Clone());
			destExtraData->header = hdr;
			destExtraData->SetName(srcExtraData->GetName());

			int extraDataId = hdr->AddBlock(destExtraData);
			destGeom->SetExtraDataRef(i, extraDataId);

			if (destExtraData->HasType<NiStringExtraData>()) {
				auto strSrcExtraData = dynamic_cast<NiStringExtraData*>(srcExtraData);
				auto strDestExtraData = dynamic_cast<NiStringExtraData*>(destExtraData);
				if (strSrcExtraData && strDestExtraData)
					strDestExtraData->SetStringData(strSrcExtraData->GetStringData());
			}
		}
	}

	// Collision Object
	auto srcCollisionObj = srcNif.hdr->GetBlock<NiCollisionObject>(srcGeom->GetCollisionRef());
	if (srcCollisionObj) {
		auto destCollisionObj = srcCollisionObj->Clone();
		destCollisionObj->header = hdr;

		int collisionId = hdr->AddBlock(destCollisionObj);
		destGeom->SetCollisionRef(collisionId);
	}

	// Geometry Data
	auto srcGeomData = srcNif.hdr->GetBlock<NiTriBasedGeomData>(srcGeom->GetDataRef());
	if (srcGeomData) {
		NiTriBasedGeomData* destGeomData = static_cast<NiTriBasedGeomData*>(srcGeomData->Clone());
		destGeomData->header = hdr;

		int destDataId = hdr->AddBlock(destGeomData);
		destGeom->SetDataRef(destDataId);
	}

	// Skinning
	NiBoneContainer* destBoneCont = nullptr;
	if (srcGeom->GetSkinInstanceRef() != 0xFFFFFFFF) {
		if (destGeom->HasType<NiTriBasedGeom>() || (destGeom->HasType<BSTriShape>() && hdr->GetUserVersion2() == 100)) {
			auto srcSkinInst = srcNif.hdr->GetBlock<NiSkinInstance>(srcGeom->GetSkinInstanceRef());
			if (srcSkinInst) {
				auto srcSkinData = srcNif.hdr->GetBlock<NiSkinData>(srcSkinInst->GetDataRef());
				auto srcSkinPart = srcNif.hdr->GetBlock<NiSkinPartition>(srcSkinInst->GetSkinPartitionRef());

				NiSkinInstance* destSkinInst = srcSkinInst->Clone();
				destSkinInst->header = hdr;

				// Treat skinning and partition info as blobs of anonymous data.
				auto destSkinData = srcSkinData->Clone();
				destSkinData->header = hdr;
				auto destSkinPart = srcSkinPart->Clone();
				destSkinPart->header = hdr;

				int destSkinId = hdr->AddBlock(destSkinInst);
				int destSkinDataId = hdr->AddBlock(destSkinData);
				int destSkinPartId = hdr->AddBlock(destSkinPart);

				destGeom->SetSkinInstanceRef(destSkinId);
				destSkinInst->SetDataRef(destSkinDataId);
				destSkinInst->SetSkinPartitionRef(destSkinPartId);

				destBoneCont = static_cast<NiBoneContainer*>(destSkinInst);
			}
		}
		else if (destGeom->HasType<BSTriShape>()) {
			auto srcBSSkinInst = srcNif.hdr->GetBlock<BSSkinInstance>(srcGeom->GetSkinInstanceRef());
			if (srcBSSkinInst) {
				auto destBSSkinInst = srcBSSkinInst->Clone();

				int destSkinInstId = hdr->AddBlock(destBSSkinInst);
				destGeom->SetSkinInstanceRef(destSkinInstId);

				auto srcBoneData = srcNif.hdr->GetBlock<BSSkinBoneData>(srcBSSkinInst->GetDataRef());
				if (srcBoneData) {
					auto destBoneData = srcBoneData->Clone();

					int destBoneDataId = hdr->AddBlock(destBoneData);
					destBSSkinInst->SetDataRef(destBoneDataId);

					destBoneCont = static_cast<NiBoneContainer*>(destBSSkinInst);
				}
			}
		}
	}

	// Shader
	CopyShader(shapeDest, srcNif);

	// Properties
	for (int i = 0; i < srcGeom->propertyRefs.GetSize(); i++) {
		auto srcProp = srcNif.hdr->GetBlock<NiProperty>(srcGeom->propertyRefs.GetBlockRef(i));
		if (srcProp) {
			auto destProp = srcProp->Clone();
			destProp->header = hdr;

			int propId = hdr->AddBlock(destProp);
			destGeom->propertyRefs.SetBlockRef(i, propId);
		}
	}

	std::vector<std::string> srcBoneList;
	srcNif.GetShapeBoneList(srcShape, srcBoneList);
	if (destBoneCont)
		destBoneCont->boneRefs.Clear();

	// Bones
	auto rootNode = dynamic_cast<NiNode*>(blocks[0]);
	if (rootNode) {
		for (auto &boneName : srcBoneList) {
			int boneID = GetBlockID(FindNodeByName(boneName));
			if (boneID == 0xFFFFFFFF) {
				boneID = CopyNamedNode(boneName, srcNif);
				rootNode->AddChildRef(boneID);
			}

			if (destBoneCont)
				destBoneCont->boneRefs.AddBlockRef(boneID);
		}

		rootNode->AddChildRef(destId);
	}
}

int NifFile::Save(const std::string& filename, bool optimize, bool sortBlocks) {
	std::fstream file(filename.c_str(), std::ios_base::out | std::ios_base::binary);
	if (file.is_open()) {
		FinalizeData();

		if (optimize)
			Optimize();

		if (sortBlocks)
			PrettySortBlocks();

		hdr->CalcAllBlockSizes();
		hdr->Put(file);

		for (int i = 0; i < hdr->GetNumBlocks(); i++)
			blocks[i]->Put(file);

		uint endPad = 1;
		file.write((char*)&endPad, 4);
		endPad = 0;
		file.write((char*)&endPad, 4);
		file.close();
	}
	else
		return 1;

	return 0;
}

void NifFile::Optimize() {
	std::vector<std::string> shapes;
	GetShapeList(shapes);

	for (auto &s : shapes)
		UpdateBoundingSphere(s);

	DeleteUnreferencedBlocks();
	RemoveUnusedStrings();
}

OptResultSSE NifFile::OptimizeForSSE(const OptOptionsSSE& options) {
	OptResultSSE result;
	if (!(hdr->GetUserVersion() == 12 && hdr->GetUserVersion2() == 83)) {
		result.versionMismatch = true;
		return result;
	}

	std::vector<std::string> shapes;
	GetShapeList(shapes);
	for (auto &s : shapes) {
		bool renamed = RenameDuplicateShape(s);
		if (renamed)
			result.shapesRenamed.push_back(s);
	}

	hdr->SetVersion(20, 2, 0, 7, 12, 100);

	GetShapeList(shapes);
	for (auto &s : shapes) {
		NiShape* shape = FindShapeByName(s);
		if (shape) {
			NiNode* parentNode = GetParentNode(shape);
			if (!parentNode)
				continue;

			auto geomData = hdr->GetBlock<NiGeometryData>(shape->GetDataRef());
			if (geomData) {
				bool removeVertexColors = true;
				bool hasTangents = geomData->HasTangents();
				std::vector<Vector3>* vertices = &geomData->vertices;
				std::vector<Vector3>* normals = &geomData->normals;
				std::vector<Vector2>* uvs = &geomData->uvSets;
				const std::vector<Color4>& colors = geomData->vertexColors;
				std::vector<Triangle> triangles;

				if (geomData->HasType<NiTriShapeData>()) {
					auto shapeData = hdr->GetBlock<NiTriShapeData>(shape->GetDataRef());
					if (shapeData)
						triangles = shapeData->triangles;
				}
				else if (geomData->HasType<NiTriStripsData>()) {
					auto stripsData = hdr->GetBlock<NiTriStripsData>(shape->GetDataRef());
					if (stripsData) {
						triangles.reserve(stripsData->numTriangles);
						stripsData->StripsToTris(&triangles);
					}
				}

				// Only remove vertex colors if all are 0xFFFFFFFF
				Color4 ffffffff(1.0f, 1.0f, 1.0f, 1.0f);
				for (auto &c : colors) {
					if (ffffffff != c) {
						removeVertexColors = false;
						break;
					}
				}

				if (!colors.empty() && removeVertexColors)
					result.shapesVColorsRemoved.push_back(s);

				bool headPartEyes = false;
				NiShader* shader = GetShader(s);
				if (shader) {
					auto bslsp = dynamic_cast<BSLightingShaderProperty*>(shader);
					if (bslsp) {
						// Remember eyes flag for later
						if ((bslsp->shaderFlags1 & (1 << 17)) != 0)
							headPartEyes = true;

						// No normals and tangents with model space maps
						if ((bslsp->shaderFlags1 & (1 << 12)) != 0) {
							if (!normals->empty())
								result.shapesNormalsRemoved.push_back(s);

							normals = nullptr;
						}

						// Disable flag if vertex colors were removed
						if (removeVertexColors)
							bslsp->shaderFlags2 &= ~(1 << 5);
					}

					auto bsesp = dynamic_cast<BSEffectShaderProperty*>(shader);
					if (bsesp) {
						// Remember eyes flag for later
						if ((bsesp->shaderFlags1 & (1 << 17)) != 0)
							headPartEyes = true;

						// Disable flag if vertex colors were removed
						if (removeVertexColors)
							bsesp->shaderFlags2 &= ~(1 << 5);
					}
				}

				BSTriShape* bsShape = nullptr;
				if (options.headParts)
					bsShape = new BSDynamicTriShape(hdr);
				else
					bsShape = new BSTriShape(hdr);

				bsShape->SetName(shape->GetName());
				bsShape->SetControllerRef(shape->GetControllerRef());
				bsShape->SetSkinInstanceRef(shape->GetSkinInstanceRef());
				bsShape->SetShaderPropertyRef(shape->GetShaderPropertyRef());
				bsShape->SetAlphaPropertyRef(shape->GetAlphaPropertyRef());
				bsShape->SetCollisionRef(shape->GetCollisionRef());
				bsShape->propertyRefs.SetIndices(shape->propertyRefs.GetIndices());

				for (int i = 0; i < shape->GetNumExtraData(); i++)
					bsShape->AddExtraDataRef(shape->GetExtraDataRef(i));

				bsShape->rotation[0] = shape->rotation[0];
				bsShape->rotation[1] = shape->rotation[1];
				bsShape->rotation[2] = shape->rotation[2];
				bsShape->scale = shape->scale;
				bsShape->translation = shape->translation;

				bsShape->Create(vertices, &triangles, uvs, normals);
				bsShape->flags = shape->flags;

				if (!shape->IsSkinned())
					bsShape->SetBounds(geomData->GetBounds());

				if (bsShape->numVertices > 0) {
					if (!removeVertexColors && colors.size() > 0) {
						bsShape->SetVertexColors(true);
						for (int i = 0; i < bsShape->numVertices; i++) {
							float f = std::max(0.0f, std::min(1.0f, colors[i].r));
							bsShape->vertData[i].colorData[0] = (byte)std::floor(f == 1.0f ? 255 : f * 256.0);

							f = std::max(0.0f, std::min(1.0f, colors[i].g));
							bsShape->vertData[i].colorData[1] = (byte)std::floor(f == 1.0f ? 255 : f * 256.0);

							f = std::max(0.0f, std::min(1.0f, colors[i].b));
							bsShape->vertData[i].colorData[2] = (byte)std::floor(f == 1.0f ? 255 : f * 256.0);

							f = std::max(0.0f, std::min(1.0f, colors[i].a));
							bsShape->vertData[i].colorData[3] = (byte)std::floor(f == 1.0f ? 255 : f * 256.0);
						}
					}

					// Find NiOptimizeKeep string
					for (int i = 0; i < bsShape->GetNumExtraData(); i++) {
						auto stringData = hdr->GetBlock<NiStringExtraData>(bsShape->GetExtraDataRef(i));
						if (stringData) {
							if (stringData->GetStringData().find("NiOptimizeKeep") != std::string::npos) {
								bsShape->particleDataSize = bsShape->numVertices * 6 + bsShape->numTriangles * 3;
								bsShape->particleVerts = *vertices;

								bsShape->particleNorms.resize(vertices->size(), Vector3(1.0f, 0.0f, 0.0f));
								if (normals && normals->size() == vertices->size())
									bsShape->particleNorms = *normals;

								bsShape->particleTris = bsShape->triangles;
							}
						}
					}

					if (shape->IsSkinned()) {
						bsShape->SetSkinned(true);

						auto skinInst = hdr->GetBlock<NiSkinInstance>(shape->GetSkinInstanceRef());
						if (skinInst) {
							auto skinPart = hdr->GetBlock<NiSkinPartition>(skinInst->GetSkinPartitionRef());
							if (skinPart) {
								bool triangulated = TriangulatePartitions(s);
								if (triangulated)
									result.shapesPartTriangulated.push_back(s);

								for (int partID = 0; partID < skinPart->numPartitions; partID++) {
									NiSkinPartition::PartitionBlock& part = skinPart->partitions[partID];

									for (int i = 0; i < part.numVertices; i++) {
										const ushort v = part.vertexMap[i];

										if (part.hasVertexWeights) {
											bsShape->vertData[v].weights[0] = part.vertexWeights[i].w1;
											bsShape->vertData[v].weights[1] = part.vertexWeights[i].w2;
											bsShape->vertData[v].weights[2] = part.vertexWeights[i].w3;
											bsShape->vertData[v].weights[3] = part.vertexWeights[i].w4;
										}

										if (part.hasBoneIndices) {
											bsShape->vertData[v].weightBones[0] = part.bones[part.boneIndices[i].i1];
											bsShape->vertData[v].weightBones[1] = part.bones[part.boneIndices[i].i2];
											bsShape->vertData[v].weightBones[2] = part.bones[part.boneIndices[i].i3];
											bsShape->vertData[v].weightBones[3] = part.bones[part.boneIndices[i].i4];
										}
									}

									std::vector<Triangle> realTris(part.numTriangles);
									for (int i = 0; i < part.numTriangles; i++) {
										// Find the actual tri index from the partition tri index
										Triangle tri;
										tri.p1 = part.vertexMap[part.triangles[i].p1];
										tri.p2 = part.vertexMap[part.triangles[i].p2];
										tri.p3 = part.vertexMap[part.triangles[i].p3];

										tri.rot();
										realTris[i] = tri;
									}

									part.triangles = realTris;
									part.trueTriangles = realTris;
								}
							}
						}
					}
					else
						bsShape->SetSkinned(false);
				}
				else
					bsShape->SetVertices(false);

				if (!hasTangents && bsShape->HasTangents())
					result.shapesTangentsAdded.push_back(s);

				if (options.headParts) {
					if (headPartEyes)
						bsShape->vertFlags7 |= 1 << 4;

					hdr->ReplaceBlock(GetBlockID(shape), bsShape);
				}
				else
					hdr->ReplaceBlock(GetBlockID(shape), bsShape);

				UpdateSkinPartitions(s);
			}
		}
	}

	hdr->DeleteUnreferencedBlocks();
	return result;
}

void NifFile::PrepareData() {
	std::vector<std::string> shapes;
	GetShapeList(shapes);

	for (auto &s : shapes) {
		// Move triangle and vertex data from partition to shape
		if (hdr->GetUserVersion() >= 12 && hdr->GetUserVersion2() == 100) {
			NiShape* shape = FindShapeByName(s);
			if (shape) {
				BSTriShape* bsTriShape = dynamic_cast<BSTriShape*>(shape);
				if (!bsTriShape)
					continue;

				auto skinInst = hdr->GetBlock<NiSkinInstance>(shape->GetSkinInstanceRef());
				if (!skinInst)
					continue;

				auto skinPart = hdr->GetBlock<NiSkinPartition>(skinInst->GetSkinPartitionRef());
				if (!skinPart)
					continue;

				bsTriShape->vertData = skinPart->vertData;
				bsTriShape->numVertices = skinPart->dataSize / skinPart->vertexSize;
				bsTriShape->numTriangles = 0;
				bsTriShape->triangles.clear();

				for (auto &part : skinPart->partitions)
					for (auto &tri : part.trueTriangles)
						bsTriShape->triangles.push_back(tri);

				bsTriShape->numTriangles = bsTriShape->triangles.size();

				auto dynamicShape = dynamic_cast<BSDynamicTriShape*>(bsTriShape);
				if (dynamicShape) {
					for (int i = 0; i < dynamicShape->numVertices; i++) {
						dynamicShape->vertData[i].vert.x = dynamicShape->dynamicData[i].x;
						dynamicShape->vertData[i].vert.y = dynamicShape->dynamicData[i].y;
						dynamicShape->vertData[i].vert.z = dynamicShape->dynamicData[i].z;
					}
				}
			}
		}
	}
}

void NifFile::FinalizeData() {
	std::vector<std::string> shapes;
	GetShapeList(shapes);

	for (auto &s : shapes) {
		// Move triangle and vertex data from shape to partition
		if (hdr->GetUserVersion() >= 12 && hdr->GetUserVersion2() == 100) {
			NiShape* shape = FindShapeByName(s);
			if (shape) {
				BSTriShape* bsTriShape = dynamic_cast<BSTriShape*>(shape);
				if (!bsTriShape)
					continue;

				auto skinInst = hdr->GetBlock<NiSkinInstance>(shape->GetSkinInstanceRef());
				if (!skinInst)
					continue;

				auto skinPart = hdr->GetBlock<NiSkinPartition>(skinInst->GetSkinPartitionRef());
				if (!skinPart)
					continue;

				bsTriShape->CalcDataSizes();

				skinPart->numVertices = bsTriShape->numVertices;
				skinPart->dataSize = bsTriShape->dataSize;
				skinPart->vertexSize = bsTriShape->vertexSize;
				skinPart->vertData = bsTriShape->vertData;
				skinPart->vertFlags1 = bsTriShape->vertFlags1;
				skinPart->vertFlags2 = bsTriShape->vertFlags2;
				skinPart->vertFlags3 = bsTriShape->vertFlags3;
				skinPart->vertFlags4 = bsTriShape->vertFlags4;
				skinPart->vertFlags5 = bsTriShape->vertFlags5;
				skinPart->vertFlags6 = bsTriShape->vertFlags6;
				skinPart->vertFlags7 = bsTriShape->vertFlags7;
				skinPart->vertFlags8 = bsTriShape->vertFlags8;
			}
		}
	}
}

int NifFile::GetShapeList(std::vector<std::string>& outList) {
	outList.clear();
	for (auto& block : blocks) {
		auto shape = dynamic_cast<NiShape*>(block);
		if (shape)
			outList.push_back(shape->GetName());
	}
	return outList.size();
}

void NifFile::RenameShape(const std::string& oldName, const std::string& newName) {
	NiAVObject* geom = FindAVObjectByName(oldName);
	if (geom)
		geom->SetName(newName, true);
}

bool NifFile::RenameDuplicateShape(const std::string& dupedShape) {
	int dupCount = 1;
	char buf[10];

	NiAVObject* geom = FindAVObjectByName(dupedShape);
	if (geom) {
		while ((geom = FindAVObjectByName(dupedShape, 1)) != nullptr) {
			_snprintf(buf, 10, "_%d", dupCount);
			while (hdr->FindStringId(geom->GetName() + buf) != 0xFFFFFFFF) {
				dupCount++;
				_snprintf(buf, 10, "_%d", dupCount);
			}

			geom->SetName(geom->GetName() + buf);
			dupCount++;
		}
	}

	return (dupCount > 1);
}

int NifFile::GetRootNodeID() {
	if (blocks.empty())
		return 0xFFFFFFFF;

	return 0;
}

bool NifFile::GetNodeTransform(const std::string& nodeName, std::vector<Vector3>& outRot, Vector3& outTrans, float& outScale) {
	for (auto& block : blocks) {
		auto node = dynamic_cast<NiNode*>(block);
		if (node && !node->GetName().compare(nodeName)) {
			outRot.clear();
			outRot.push_back(node->rotation[0]);
			outRot.push_back(node->rotation[1]);
			outRot.push_back(node->rotation[2]);
			outTrans = node->translation;
			outScale = node->scale;
			return true;
		}
	}
	return false;
}

bool NifFile::SetNodeTransform(const std::string& nodeName, SkinTransform& inXform, const bool rootChildrenOnly) {
	if (rootChildrenOnly) {
		auto root = dynamic_cast<NiNode*>(blocks[0]);
		if (root) {
			for (int i = 0; i < root->GetNumChildren(); i++) {
				auto node = hdr->GetBlock<NiNode>(root->GetChildRef(i));
				if (node) {
					if (!node->GetName().compare(nodeName)) {
						node->rotation[0] = inXform.rotation[0];
						node->rotation[1] = inXform.rotation[1];
						node->rotation[2] = inXform.rotation[2];
						node->translation = inXform.translation;
						node->scale = inXform.scale;
						return true;
					}
				}
			}
		}
	}
	else {
		for (auto& block : blocks) {
			auto node = dynamic_cast<NiNode*>(block);
			if (node && !node->GetName().compare(nodeName)) {
				node->rotation[0] = inXform.rotation[0];
				node->rotation[1] = inXform.rotation[1];
				node->rotation[2] = inXform.rotation[2];
				node->translation = inXform.translation;
				node->scale = inXform.scale;
				return true;
			}
		}
	}

	return false;
}

int NifFile::GetShapeBoneList(const std::string& shapeName, std::vector<std::string>& outList) {
	outList.clear();

	NiShape* shape = FindShapeByName(shapeName);
	if (!shape)
		return 0;

	auto skinInst = hdr->GetBlock<NiBoneContainer>(shape->GetSkinInstanceRef());
	if (!skinInst)
		return 0;

	for (int i = 0; i < skinInst->boneRefs.GetSize(); i++) {
		auto node = hdr->GetBlock<NiNode>(skinInst->boneRefs.GetBlockRef(i));
		if (node)
			outList.push_back(node->GetName());
	}

	return outList.size();
}

int NifFile::GetShapeBoneIDList(const std::string& shapeName, std::vector<int>& outList) {
	outList.clear();

	NiShape* shape = FindShapeByName(shapeName);
	if (!shape)
		return 0;

	auto skinInst = hdr->GetBlock<NiBoneContainer>(shape->GetSkinInstanceRef());
	if (!skinInst)
		return 0;

	for (int i = 0; i < skinInst->boneRefs.GetSize(); i++)
		outList.push_back(skinInst->boneRefs.GetBlockRef(i));

	return outList.size();
}

void NifFile::SetShapeBoneIDList(const std::string& shapeName, std::vector<int>& inList) {
	NiShape* shape = FindShapeByName(shapeName);
	if (!shape)
		return;

	if (shape->GetSkinInstanceRef() == 0xFFFFFFFF)
		return;

	BSSkinBoneData* boneData = nullptr;
	if (shape->HasType<BSTriShape>()) {
		auto skinForBoneRef = hdr->GetBlock<BSSkinInstance>(shape->GetSkinInstanceRef());
		if (skinForBoneRef)
			boneData = hdr->GetBlock<BSSkinBoneData>(skinForBoneRef->GetDataRef());
	}

	auto boneCont = hdr->GetBlock<NiBoneContainer>(shape->GetSkinInstanceRef());
	if (!boneCont)
		return;

	boneCont->boneRefs.Clear();

	bool feedBoneData = false;
	if (boneData && boneData->nBones != inList.size()) {
		// Clear if size doesn't match
		boneData->nBones = 0;
		boneData->boneXforms.clear();
		feedBoneData = true;
	}

	for (auto &i : inList) {
		boneCont->boneRefs.AddBlockRef(i);
		if (boneData && feedBoneData) {
			boneData->boneXforms.emplace_back();
			boneData->nBones++;
		}
	}

	auto skinInst = dynamic_cast<NiSkinInstance*>(boneCont);
	if (skinInst) {
		auto skinData = hdr->GetBlock<NiSkinData>(skinInst->GetDataRef());
		if (skinData) {
			feedBoneData = false;

			if (skinData->numBones != inList.size()) {
				// Clear if size doesn't match
				skinData->numBones = 0;
				skinData->bones.clear();
				feedBoneData = true;
			}

			if (feedBoneData) {
				skinData->bones.resize(inList.size());
				skinData->numBones = skinData->bones.size();
			}
		}
	}
}

int NifFile::GetShapeBoneWeights(const std::string& shapeName, const int boneIndex, std::unordered_map<ushort, float>& outWeights) {
	outWeights.clear();

	NiShape* shape = FindShapeByName(shapeName);
	if (!shape)
		return 0;

	auto bsTriShape = dynamic_cast<BSTriShape*>(shape);
	if (bsTriShape) {
		outWeights.reserve(bsTriShape->numVertices);
		for (int vid = 0; vid < bsTriShape->numVertices; vid++) {
			for (int i = 0; i < 4; i++) {
				if (bsTriShape->vertData[vid].weightBones[i] == boneIndex && bsTriShape->vertData[vid].weights[i] != 0)
					outWeights.emplace(vid, bsTriShape->vertData[vid].weights[i]);
			}
		}

		return outWeights.size();
	}

	auto skinInst = hdr->GetBlock<NiSkinInstance>(shape->GetSkinInstanceRef());
	if (!skinInst)
		return 0;

	auto skinData = hdr->GetBlock<NiSkinData>(skinInst->GetDataRef());
	if (!skinData || boneIndex > skinData->numBones)
		return 0;

	NiSkinData::BoneData* bone = &skinData->bones[boneIndex];
	for (auto &sw : bone->vertexWeights) {
		if (sw.weight >= EPSILON)
			outWeights.emplace(sw.index, sw.weight);
		else
			outWeights.emplace(sw.index, 0.0f);
	}

	return outWeights.size();
}

bool NifFile::GetShapeBoneTransform(const std::string& shapeName, const std::string& boneName, SkinTransform& outXform) {
	NiShape* shape = FindShapeByName(shapeName);
	if (!shape)
		return false;

	int boneIndex = shape->GetBoneID(boneName);
	if (boneName.empty())
		boneIndex = 0xFFFFFFFF;

	return GetShapeBoneTransform(shapeName, boneIndex, outXform);
}

bool NifFile::SetShapeBoneTransform(const std::string& shapeName, const int boneIndex, SkinTransform& inXform) {
	NiShape* shape = FindShapeByName(shapeName);
	if (!shape)
		return false;

	auto skinForBoneRef = hdr->GetBlock<BSSkinInstance>(shape->GetSkinInstanceRef());
	if (skinForBoneRef && boneIndex != 0xFFFFFFFF) {
		auto bsSkin = hdr->GetBlock<BSSkinBoneData>(skinForBoneRef->GetDataRef());
		if (!bsSkin)
			return false;

		bsSkin->boneXforms[boneIndex].boneTransform = inXform;
		return true;
	}

	auto skinInst = hdr->GetBlock<NiSkinInstance>(shape->GetSkinInstanceRef());
	if (!skinInst)
		return false;

	auto skinData = hdr->GetBlock<NiSkinData>(skinInst->GetDataRef());
	if (!skinData)
		return false;

	if (boneIndex == 0xFFFFFFFF) {
		// Set the overall skin transform
		skinData->skinTransform = inXform;
		return true;
	}

	if (boneIndex > skinData->numBones)
		return false;

	NiSkinData::BoneData* bone = &skinData->bones[boneIndex];
	bone->boneTransform = inXform;
	return true;
}

bool NifFile::SetShapeBoneBounds(const std::string& shapeName, const int boneIndex, BoundingSphere& inBounds) {
	NiShape* shape = FindShapeByName(shapeName);
	if (!shape)
		return false;

	auto skinForBoneRef = hdr->GetBlock<BSSkinInstance>(shape->GetSkinInstanceRef());
	if (skinForBoneRef && boneIndex != 0xFFFFFFFF) {
		auto bsSkin = hdr->GetBlock<BSSkinBoneData>(skinForBoneRef->GetDataRef());
		if (!bsSkin)
			return false;

		bsSkin->boneXforms[boneIndex].bounds = inBounds;
		return true;
	}

	auto skinInst = hdr->GetBlock<NiSkinInstance>(shape->GetSkinInstanceRef());
	if (!skinInst)
		return false;

	auto skinData = hdr->GetBlock<NiSkinData>(skinInst->GetDataRef());
	if (!skinData)
		return false;

	if (boneIndex > skinData->numBones)
		return false;

	NiSkinData::BoneData* bone = &skinData->bones[boneIndex];
	bone->bounds = inBounds;
	return true;
}

bool NifFile::GetShapeBoneTransform(const std::string& shapeName, const int boneIndex, SkinTransform& outXform) {
	NiShape* shape = FindShapeByName(shapeName);
	if (!shape)
		return false;

	auto skinForBoneRef = hdr->GetBlock<BSSkinInstance>(shape->GetSkinInstanceRef());
	if (skinForBoneRef) {
		auto boneData = hdr->GetBlock<BSSkinBoneData>(skinForBoneRef->GetDataRef());
		if (boneData) {
			if (boneIndex == 0xFFFFFFFF) {
				// Overall skin transform not found in FO4 meshes :(
				return false;
			}

			outXform = boneData->boneXforms[boneIndex].boneTransform;
			return true;
		}
	}

	auto skinInst = hdr->GetBlock<NiSkinInstance>(shape->GetSkinInstanceRef());
	if (!skinInst)
		return false;

	auto skinData = hdr->GetBlock<NiSkinData>(skinInst->GetDataRef());
	if (!skinData)
		return false;

	if (boneIndex == 0xFFFFFFFF) {
		// Want the overall skin transform
		outXform = skinData->skinTransform;
		return true;
	}

	if (boneIndex > skinData->numBones)
		return 0;

	NiSkinData::BoneData* bone = &skinData->bones[boneIndex];
	outXform = bone->boneTransform;
	return true;
}

bool NifFile::GetShapeBoneBounds(const std::string& shapeName, const int boneIndex, BoundingSphere& outBounds) {
	NiShape* shape = FindShapeByName(shapeName);
	if (!shape)
		return false;

	auto skinForBoneRef = hdr->GetBlock<BSSkinInstance>(shape->GetSkinInstanceRef());
	if (skinForBoneRef) {
		auto boneData = hdr->GetBlock<BSSkinBoneData>(skinForBoneRef->GetDataRef());
		if (boneData) {
			outBounds = boneData->boneXforms[boneIndex].bounds;
			return true;
		}
	}

	auto skinInst = hdr->GetBlock<NiSkinInstance>(shape->GetSkinInstanceRef());
	if (!skinInst)
		return false;

	auto skinData = hdr->GetBlock<NiSkinData>(skinInst->GetDataRef());
	if (!skinData)
		return false;

	if (boneIndex > skinData->numBones)
		return 0;

	NiSkinData::BoneData* bone = &skinData->bones[boneIndex];
	outBounds = bone->bounds;
	return true;
}

void NifFile::UpdateShapeBoneID(const std::string& shapeName, const int oldID, const int newID) {
	NiShape* shape = FindShapeByName(shapeName);
	if (!shape)
		return;

	auto boneCont = hdr->GetBlock<NiBoneContainer>(shape->GetSkinInstanceRef());
	if (!boneCont)
		return;

	for (auto &bp : boneCont->boneRefs.GetRefs()) {
		if (bp.index == oldID) {
			bp.index = newID;
			return;
		}
	}
}

// Not implemented for BSTriShape, use SetShapeVertWeights instead
void NifFile::SetShapeBoneWeights(const std::string& shapeName, const int boneIndex, std::unordered_map<ushort, float>& inWeights) {
	NiShape* shape = FindShapeByName(shapeName);
	if (!shape)
		return;

	auto skinInst = hdr->GetBlock<NiSkinInstance>(shape->GetSkinInstanceRef());
	if (!skinInst)
		return;

	auto skinData = hdr->GetBlock<NiSkinData>(skinInst->GetDataRef());
	if (!skinData)
		return;

	if (boneIndex > skinData->numBones)
		return;

	NiSkinData::BoneData* bone = &skinData->bones[boneIndex];
	bone->vertexWeights.clear();
	for (auto &sw : inWeights)
		if (sw.second >= 0.0001f)
			bone->vertexWeights.emplace_back(SkinWeight(sw.first, sw.second));

	bone->numVertices = (ushort)bone->vertexWeights.size();
}

void NifFile::SetShapeVertWeights(const std::string& shapeName, const int vertIndex, std::vector<byte>& boneids, std::vector<float>& weights) {
	NiShape* shape = FindShapeByName(shapeName);
	if (!shape)
		return;

	BSTriShape* bsTriShape = dynamic_cast<BSTriShape*>(shape);
	if (!bsTriShape)
		return;

	memset(bsTriShape->vertData[vertIndex].weights, 0, sizeof(float) * 4);
	memset(bsTriShape->vertData[vertIndex].weightBones, 0, sizeof(byte) * 4);

	// Sum weights to normalize values
	float sum = 0.0f;
	for (int i = 0; i < weights.size(); i++)
		sum += weights[i];

	int num = (weights.size() < 4 ? weights.size() : 4);

	for (int i = 0; i < num; i++) {
		bsTriShape->vertData[vertIndex].weightBones[i] = boneids[i];
		bsTriShape->vertData[vertIndex].weights[i] = weights[i] / sum;
	}
}

bool NifFile::GetShapeSegments(const std::string& shapeName, BSSubIndexTriShape::BSSITSSegmentation& segmentation) {
	BSSubIndexTriShape* siTriShape = dynamic_cast<BSSubIndexTriShape*>(FindShapeByName(shapeName));
	if (!siTriShape)
		return false;

	segmentation = siTriShape->GetSegmentation();
	return true;
}

void NifFile::SetShapeSegments(const std::string& shapeName, const BSSubIndexTriShape::BSSITSSegmentation& segmentation) {
	BSSubIndexTriShape* siTriShape = dynamic_cast<BSSubIndexTriShape*>(FindShapeByName(shapeName));
	if (!siTriShape)
		return;

	siTriShape->SetSegmentation(segmentation);
}

bool NifFile::GetShapePartitions(const std::string& shapeName, std::vector<BSDismemberSkinInstance::PartitionInfo>& partitionInfo, std::vector<std::vector<ushort>>& verts, std::vector<std::vector<Triangle>>& tris) {
	NiShape* shape = FindShapeByName(shapeName);
	if (!shape)
		return false;

	auto bsdSkinInst = hdr->GetBlock<BSDismemberSkinInstance>(shape->GetSkinInstanceRef());
	if (bsdSkinInst)
		partitionInfo = bsdSkinInst->GetPartitions();
	else
		partitionInfo.clear();

	auto skinInst = hdr->GetBlock<NiSkinInstance>(shape->GetSkinInstanceRef());
	if (!skinInst)
		return false;

	auto skinPart = hdr->GetBlock<NiSkinPartition>(skinInst->GetSkinPartitionRef());
	if (!skinPart)
		return false;

	for (auto &part : skinPart->partitions) {
		verts.push_back(part.vertexMap);
		tris.push_back(part.triangles);
	}

	return true;
}

void NifFile::SetShapePartitions(const std::string& shapeName, const std::vector<BSDismemberSkinInstance::PartitionInfo>& partitionInfo, const std::vector<std::vector<ushort>>& verts, const std::vector<std::vector<Triangle>>& tris) {
	NiShape* shape = FindShapeByName(shapeName);
	if (!shape)
		return;

	auto bsdSkinInst = hdr->GetBlock<BSDismemberSkinInstance>(shape->GetSkinInstanceRef());
	if (bsdSkinInst)
		bsdSkinInst->SetPartitions(partitionInfo);

	auto skinInst = hdr->GetBlock<NiSkinInstance>(shape->GetSkinInstanceRef());
	if (!skinInst)
		return;

	auto skinPart = hdr->GetBlock<NiSkinPartition>(skinInst->GetSkinPartitionRef());
	if (!skinPart)
		return;

	skinPart->numPartitions = verts.size();
	skinPart->partitions.resize(verts.size());
	for (int i = 0; i < skinPart->numPartitions; i++) {
		skinPart->partitions[i].numVertices = verts[i].size();
		if (!verts[i].empty()) {
			skinPart->partitions[i].hasVertexMap = true;
			skinPart->partitions[i].vertexMap = verts[i];
		}
		else
			skinPart->partitions[i].vertexMap.clear();

		skinPart->partitions[i].numTriangles = tris[i].size();
		if (!tris[i].empty())
			skinPart->partitions[i].triangles = tris[i];
		else
			skinPart->partitions[i].triangles.clear();
	}

	std::vector<int> emptyIndices;
	if (skinPart->RemoveEmptyPartitions(emptyIndices)) {
		if (bsdSkinInst) {
			for (auto &i : emptyIndices)
				bsdSkinInst->RemovePartition(i);

			UpdatePartitionFlags(shapeName);
		}
	}
}

void NifFile::SetDefaultPartition(const std::string& shapeName) {
	NiShape* shape = FindShapeByName(shapeName);
	if (!shape)
		return;

	std::vector<Vector3> verts;
	std::vector<Triangle> tris;
	if (shape->HasType<NiTriShape>()) {
		auto shapeData = hdr->GetBlock<NiTriShapeData>(shape->GetDataRef());
		if (!shapeData)
			return;

		verts = shapeData->vertices;
		tris = shapeData->triangles;
	}
	else if (shape->HasType<NiTriStrips>()) {
		auto stripsData = hdr->GetBlock<NiTriStripsData>(shape->GetDataRef());
		if (!stripsData)
			return;

		verts = stripsData->vertices;
		stripsData->StripsToTris(&tris);
	}
	else if (shape->HasType<BSTriShape>()) {
		auto bsTriShape = dynamic_cast<BSTriShape*>(shape);
		if (!bsTriShape)
			return;

		auto rawVerts = bsTriShape->GetRawVerts();
		if (rawVerts)
			verts = (*rawVerts);

		tris = bsTriShape->triangles;
	}

	auto bsdSkinInst = hdr->GetBlock<BSDismemberSkinInstance>(shape->GetSkinInstanceRef());
	if (bsdSkinInst) {
		BSDismemberSkinInstance::PartitionInfo partInfo;
		partInfo.flags = 1;
		partInfo.partID = hdr->GetUserVersion() >= 12 ? 32 : 0;

		bsdSkinInst->ClearPartitions();
		bsdSkinInst->AddPartition(partInfo);
	}

	auto skinInst = hdr->GetBlock<NiSkinInstance>(shape->GetSkinInstanceRef());
	if (!skinInst)
		return;

	auto skinPart = hdr->GetBlock<NiSkinPartition>(skinInst->GetSkinPartitionRef());
	if (skinPart) {
		NiSkinPartition::PartitionBlock part;
		if (!verts.empty()) {
			part.hasVertexMap = true;
			part.numVertices = verts.size();

			std::vector<ushort> vertIndices(part.numVertices);
			for (int i = 0; i < vertIndices.size(); i++)
				vertIndices[i] = i;

			part.vertexMap = vertIndices;
		}

		if (!tris.empty()) {
			part.numTriangles = tris.size();
			part.triangles = tris;
		}

		skinPart->partitions.clear();
		skinPart->partitions.push_back(part);
		skinPart->numPartitions = 1;
	}
}

const std::vector<Vector3>* NifFile::GetRawVertsForShape(const std::string& shapeName) {
	NiShape* shape = FindShapeByName(shapeName);
	if (!shape)
		return nullptr;

	if (shape->HasType<NiTriBasedGeom>()) {
		auto geomData = hdr->GetBlock<NiGeometryData>(shape->GetDataRef());
		if (geomData)
			return &geomData->vertices;
	}
	else if (shape->HasType<BSTriShape>()) {
		auto bsTriShape = dynamic_cast<BSTriShape*>(shape);
		if (bsTriShape)
			return bsTriShape->GetRawVerts();
	}

	return nullptr;
}

bool NifFile::GetTrisForShape(const std::string& shapeName, std::vector<Triangle>* outTris) {
	NiShape* shape = FindShapeByName(shapeName);
	if (!shape)
		return false;

	if (shape->HasType<NiTriShape>()) {
		auto shapeData = hdr->GetBlock<NiTriShapeData>(shape->GetDataRef());
		if (shapeData) {
			*outTris = shapeData->triangles;
			return true;
		}
	}
	else if (shape->HasType<NiTriStrips>()) {
		auto stripsData = hdr->GetBlock<NiTriStripsData>(shape->GetDataRef());
		if (stripsData) {
			stripsData->StripsToTris(outTris);
			return true;
		}
	}
	else if (shape->HasType<BSTriShape>()) {
		auto bsTriShape = dynamic_cast<BSTriShape*>(shape);
		if (bsTriShape) {
			*outTris = bsTriShape->triangles;
			return true;
		}
	}

	return false;
}

bool NifFile::ReorderTriangles(const std::string& shapeName, const std::vector<uint>& triangleIndices) {
	NiShape* shape = FindShapeByName(shapeName);
	if (!shape)
		return false;

	std::vector<Triangle> triangles;

	if (shape->HasType<NiTriShape>()) {
		auto shapeData = hdr->GetBlock<NiTriShapeData>(shape->GetDataRef());
		if (!shapeData)
			return false;

		if (triangleIndices.size() != shapeData->numTriangles)
			return false;

		for (auto &id : triangleIndices)
			if (shapeData->triangles.size() >= id)
				triangles.push_back(shapeData->triangles[id]);

		if (triangles.size() != shapeData->numTriangles)
			return false;

		shapeData->triangles = triangles;
	}
	else if (shape->HasType<NiTriStrips>()) {
		return false;
	}
	else if (shape->HasType<BSTriShape>()) {
		auto bsTriShape = dynamic_cast<BSTriShape*>(shape);
		if (!bsTriShape)
			return false;

		if (triangleIndices.size() != bsTriShape->numTriangles)
			return false;

		for (auto &id : triangleIndices)
			if (bsTriShape->triangles.size() >= id)
				triangles.push_back(bsTriShape->triangles[id]);

		if (triangles.size() != bsTriShape->numTriangles)
			return false;

		bsTriShape->triangles = triangles;
	}

	return true;
}

const std::vector<Vector3>* NifFile::GetNormalsForShape(const std::string& shapeName, bool transform) {
	NiShape* shape = FindShapeByName(shapeName);
	if (!shape)
		return nullptr;

	if (shape->HasType<NiTriBasedGeom>()) {
		auto geomData = hdr->GetBlock<NiGeometryData>(shape->GetDataRef());
		if (geomData)
			return &geomData->normals;
	}
	else if (shape->HasType<BSTriShape>()) {
		auto bsTriShape = dynamic_cast<BSTriShape*>(shape);
		if (bsTriShape)
			return bsTriShape->GetNormalData(transform);
	}

	return nullptr;
}

const std::vector<Vector2>* NifFile::GetUvsForShape(const std::string& shapeName) {
	NiShape* shape = FindShapeByName(shapeName);
	if (!shape)
		return nullptr;

	if (shape->HasType<NiTriBasedGeom>()) {
		auto geomData = hdr->GetBlock<NiGeometryData>(shape->GetDataRef());
		if (geomData)
			return &geomData->uvSets;
	}
	else if (shape->HasType<BSTriShape>()) {
		auto bsTriShape = dynamic_cast<BSTriShape*>(shape);
		if (bsTriShape)
			return bsTriShape->GetUVData();
	}

	return nullptr;
}

bool NifFile::GetUvsForShape(const std::string& shapeName, std::vector<Vector2>& outUvs) {
	const std::vector<Vector2>* uvData = GetUvsForShape(shapeName);
	if (uvData) {
		outUvs.assign(uvData->begin(), uvData->end());
		return true;
	}

	return false;
}

bool NifFile::GetVertsForShape(const std::string& shapeName, std::vector<Vector3>& outVerts) {
	NiShape* shape = FindShapeByName(shapeName);
	if (!shape) {
		outVerts.clear();
		return false;
	}

	if (shape->HasType<NiTriBasedGeom>()) {
		auto geomData = hdr->GetBlock<NiGeometryData>(shape->GetDataRef());
		if (geomData) {
			outVerts.resize(geomData->numVertices);

			for (int i = 0; i < geomData->numVertices; i++)
				outVerts[i] = geomData->vertices[i];

			return true;
		}
	}
	else if (shape->HasType<BSTriShape>()) {
		auto bsTriShape = dynamic_cast<BSTriShape*>(shape);
		if (bsTriShape) {
			outVerts.resize(bsTriShape->numVertices);

			for (int i = 0; i < bsTriShape->numVertices; i++)
				outVerts[i] = bsTriShape->vertData[i].vert;

			return true;
		}
	}

	outVerts.clear();
	return false;
}

int NifFile::GetVertCountForShape(const std::string& shapeName) {
	NiShape* shape = FindShapeByName(shapeName);
	if (!shape)
		return 0xFFFFFFFF;

	if (shape->HasType<NiTriBasedGeom>()) {
		auto geomData = hdr->GetBlock<NiGeometryData>(shape->GetDataRef());
		if (geomData)
			return geomData->numVertices;
	}
	else if (shape->HasType<BSTriShape>()) {
		auto bsTriShape = dynamic_cast<BSTriShape*>(shape);
		if (bsTriShape)
			return bsTriShape->numVertices;
	}

	return 0xFFFFFFFF;
}

void NifFile::SetVertsForShape(const std::string& shapeName, const std::vector<Vector3>& verts) {
	NiShape* shape = FindShapeByName(shapeName);
	if (!shape)
		return;

	if (shape->HasType<NiTriBasedGeom>()) {
		auto geomData = hdr->GetBlock<NiGeometryData>(shape->GetDataRef());
		if (geomData) {
			if (verts.size() != geomData->vertices.size())
				return;

			for (int i = 0; i < geomData->vertices.size(); i++)
				geomData->vertices[i] = verts[i];
		}
	}
	else if (shape->HasType<BSTriShape>()) {
		auto bsTriShape = dynamic_cast<BSTriShape*>(shape);
		if (bsTriShape) {
			if (verts.size() != bsTriShape->numVertices)
				return;

			for (int i = 0; i < bsTriShape->numVertices; i++)
				bsTriShape->vertData[i].vert = verts[i];
		}
	}
}

void NifFile::SetUvsForShape(const std::string& shapeName, const std::vector<Vector2>& uvs) {
	NiShape* shape = FindShapeByName(shapeName);
	if (!shape)
		return;

	if (shape->HasType<NiTriBasedGeom>()) {
		auto geomData = hdr->GetBlock<NiGeometryData>(shape->GetDataRef());
		if (geomData) {
			if (uvs.size() != geomData->vertices.size())
				return;

			geomData->uvSets.assign(uvs.begin(), uvs.end());
		}
	}
	else if (shape->HasType<BSTriShape>()) {
		auto bsTriShape = dynamic_cast<BSTriShape*>(shape);
		if (bsTriShape) {
			if (uvs.size() != bsTriShape->vertData.size())
				return;

			for (int i = 0; i < bsTriShape->numVertices; i++)
				bsTriShape->vertData[i].uv = uvs[i];
		}
	}
}

void NifFile::InvertUVsForShape(const std::string& shapeName, bool invertX, bool invertY) {
	NiShape* shape = FindShapeByName(shapeName);
	if (!shape)
		return;

	if (shape->HasType<NiTriBasedGeom>()) {
		auto geomData = hdr->GetBlock<NiGeometryData>(shape->GetDataRef());
		if (geomData) {
			if (invertX)
				for (int i = 0; i < geomData->uvSets.size(); ++i)
					geomData->uvSets[i].u = 1.0f - geomData->uvSets[i].u;

			if (invertY)
				for (int i = 0; i < geomData->uvSets.size(); ++i)
					geomData->uvSets[i].v = 1.0f - geomData->uvSets[i].v;
		}
	}
	else if (shape->HasType<BSTriShape>()) {
		auto bsTriShape = dynamic_cast<BSTriShape*>(shape);
		if (bsTriShape) {
			if (invertX)
				for (int i = 0; i < bsTriShape->vertData.size(); ++i)
					bsTriShape->vertData[i].uv.u = 1.0f - bsTriShape->vertData[i].uv.u;

			if (invertY)
				for (int i = 0; i < bsTriShape->vertData.size(); ++i)
					bsTriShape->vertData[i].uv.v = 1.0f - bsTriShape->vertData[i].uv.v;
		}
	}
}

void NifFile::SetNormalsForShape(const std::string& shapeName, const std::vector<Vector3>& norms) {
	NiShape* shape = FindShapeByName(shapeName);
	if (!shape)
		return;

	if (shape->HasType<NiTriBasedGeom>()) {
		auto geomData = hdr->GetBlock<NiGeometryData>(shape->GetDataRef());
		if (geomData) {
			geomData->SetNormals(true);

			for (int i = 0; i < geomData->vertices.size(); i++)
				geomData->normals[i] = norms[i];
		}
	}
	else if (shape->HasType<BSTriShape>()) {
		auto bsTriShape = dynamic_cast<BSTriShape*>(shape);
		if (bsTriShape)
			bsTriShape->SetNormals(norms);
	}
}

void NifFile::CalcNormalsForShape(const std::string& shapeName, const bool smooth, const float smoothThresh) {
	NiShape* shape = FindShapeByName(shapeName);
	if (!shape)
		return;

	if (hdr->GetUserVersion() == 12 && hdr->GetUserVersion2() <= 100) {
		NiShader* shader = GetShader(shapeName);
		if (shader && shader->IsSkinTint())
			return;
	}

	if (shape->HasType<NiTriBasedGeom>()) {
		auto geomData = hdr->GetBlock<NiGeometryData>(shape->GetDataRef());
		if (geomData)
			geomData->RecalcNormals(smooth, smoothThresh);
	}
	else if (shape->HasType<BSTriShape>()) {
		auto bsTriShape = dynamic_cast<BSTriShape*>(shape);
		if (bsTriShape)
			bsTriShape->RecalcNormals(smooth, smoothThresh);
	}
}

void NifFile::CalcTangentsForShape(const std::string& shapeName) {
	NiShape* shape = FindShapeByName(shapeName);
	if (!shape)
		return;

	if (shape->HasType<NiTriBasedGeom>()) {
		auto geomData = hdr->GetBlock<NiGeometryData>(shape->GetDataRef());
		if (geomData)
			geomData->CalcTangentSpace();
	}
	else if (shape->HasType<BSTriShape>()) {
		auto bsTriShape = dynamic_cast<BSTriShape*>(shape);
		if (bsTriShape)
			bsTriShape->CalcTangentSpace();
	}
}

void NifFile::ClearShapeTransform(const std::string& shapeName) {
	NiAVObject* avo = FindAVObjectByName(shapeName);
	if (avo) {
		avo->translation.Zero();
		avo->scale = 1.0f;
		avo->rotation[0] = Vector3(1.0f, 0.0f, 0.0f);
		avo->rotation[1] = Vector3(0.0f, 1.0f, 0.0f);
		avo->rotation[2] = Vector3(0.0f, 0.0f, 1.0f);
	}
}

void NifFile::GetShapeTransform(const std::string& shapeName, Matrix4& outTransform) {
	SkinTransform xFormRoot;
	auto root = dynamic_cast<NiNode*>(blocks[0]);
	if (root) {
		xFormRoot.translation = root->translation;
		xFormRoot.scale = root->scale;
		xFormRoot.rotation[0] = root->rotation[0];
		xFormRoot.rotation[1] = root->rotation[1];
		xFormRoot.rotation[2] = root->rotation[2];
	}

	SkinTransform xFormShape;
	NiAVObject* avo = FindAVObjectByName(shapeName);
	if (avo) {
		xFormShape.translation = avo->translation;
		xFormShape.scale = avo->scale;
		xFormShape.rotation[0] = avo->rotation[0];
		xFormShape.rotation[1] = avo->rotation[1];
		xFormShape.rotation[2] = avo->rotation[2];
	}

	Matrix4 matRoot = xFormRoot.ToMatrix();
	Matrix4 matShape = xFormShape.ToMatrix();
	outTransform = matRoot * matShape;
}

void NifFile::ClearRootTransform() {
	auto root = dynamic_cast<NiNode*>(blocks[0]);
	if (root) {
		root->translation.Zero();
		root->scale = 1.0f;
		root->rotation[0] = Vector3(1.0f, 0.0f, 0.0f);
		root->rotation[1] = Vector3(0.0f, 1.0f, 0.0f);
		root->rotation[2] = Vector3(0.0f, 0.0f, 1.0f);
	}
}

void NifFile::GetRootTranslation(Vector3& outVec) {
	auto root = dynamic_cast<NiNode*>(blocks[0]);
	if (root)
		outVec = root->translation;
	else
		outVec.Zero();
}

void NifFile::SetRootTranslation(const Vector3& newTrans) {
	auto root = dynamic_cast<NiNode*>(blocks[0]);
	if (root)
		root->translation = newTrans;
}

void NifFile::GetRootScale(float& outScale) {
	auto root = dynamic_cast<NiNode*>(blocks[0]);
	if (root)
		outScale = root->scale;
	else
		outScale = 1.0f;
}

void NifFile::SetRootScale(const float newScale) {
	auto root = dynamic_cast<NiNode*>(blocks[0]);
	if (root)
		root->scale = newScale;
}

void NifFile::GetShapeTranslation(const std::string& shapeName, Vector3& outVec) {
	NiAVObject* avo = FindAVObjectByName(shapeName);
	if (avo)
		outVec = avo->translation;

	auto root = dynamic_cast<NiNode*>(blocks[0]);
	if (root)
		outVec += root->translation;

	//if (outVec.DistanceTo(Vector3(0.0f, 0.0f, 0.0f)) < EPSILON)
	//	outVec.Zero();
}

void NifFile::SetShapeTranslation(const std::string& shapeName, const Vector3& newTrans) {
	NiAVObject* avo = FindAVObjectByName(shapeName);
	if (avo)
		avo->translation = newTrans;
}

void NifFile::GetShapeScale(const std::string& shapeName, float& outScale) {
	NiAVObject* avo = FindAVObjectByName(shapeName);
	if (avo)
		outScale = avo->scale;
}

void NifFile::SetShapeScale(const std::string& shapeName, const float newScale) {
	NiAVObject* avo = FindAVObjectByName(shapeName);
	if (avo)
		avo->scale = newScale;
}

void NifFile::ApplyShapeTranslation(const std::string& shapeName, const Vector3& offset) {
	NiShape* shape = FindShapeByName(shapeName);
	if (!shape)
		return;

	if (shape->HasType<NiTriBasedGeom>()) {
		auto geom = dynamic_cast<NiGeometry*>(shape);
		if (!geom)
			return;

		auto geomData = hdr->GetBlock<NiGeometryData>(shape->GetDataRef());
		if (geomData) {
			for (int i = 0; i < geomData->vertices.size(); i++)
				geomData->vertices[i] += geom->translation + offset;

			geom->translation = Vector3(0.0f, 0.0f, 0.0f);
		}
	}
	else if (shape->HasType<BSTriShape>()) {
		auto bsTriShape = dynamic_cast<BSTriShape*>(shape);
		if (bsTriShape) {
			for (int i = 0; i < bsTriShape->numVertices; i++)
				bsTriShape->vertData[i].vert += bsTriShape->translation + offset;

			bsTriShape->translation = Vector3(0.0f, 0.0f, 0.0f);
		}
	}
}

void NifFile::MoveVertex(const std::string& shapeName, const Vector3& pos, const int id) {
	NiShape* shape = FindShapeByName(shapeName);
	if (!shape)
		return;

	if (shape->HasType<NiTriBasedGeom>()) {
		auto geomData = hdr->GetBlock<NiGeometryData>(shape->GetDataRef());
		if (geomData && geomData->numVertices > id)
			geomData->vertices[id] = pos;
	}
	else if (shape->HasType<BSTriShape>()) {
		auto bsTriShape = dynamic_cast<BSTriShape*>(shape);
		if (bsTriShape && bsTriShape->numVertices > id)
			bsTriShape->vertData[id].vert = pos;
	}
}

void NifFile::OffsetShape(const std::string& shapeName, const Vector3& offset, std::unordered_map<ushort, float>* mask) {
	NiShape* shape = FindShapeByName(shapeName);
	if (!shape)
		return;

	if (shape->HasType<NiTriBasedGeom>()) {
		auto geomData = hdr->GetBlock<NiGeometryData>(shape->GetDataRef());
		if (geomData) {
			for (int i = 0; i < geomData->vertices.size(); i++) {
				if (mask) {
					float maskFactor = 1.0f;
					Vector3 diff = offset;
					if (mask->find(i) != mask->end()) {
						maskFactor = 1.0f - (*mask)[i];
						diff *= maskFactor;
					}
					geomData->vertices[i] += diff;
				}
				else
					geomData->vertices[i] += offset;
			}
		}
	}
	else if (shape->HasType<BSTriShape>()) {
		auto bsTriShape = dynamic_cast<BSTriShape*>(shape);
		if (bsTriShape) {
			for (int i = 0; i < bsTriShape->numVertices; i++) {
				if (mask) {
					float maskFactor = 1.0f;
					Vector3 diff = offset;
					if (mask->find(i) != mask->end()) {
						maskFactor = 1.0f - (*mask)[i];
						diff *= maskFactor;
					}
					bsTriShape->vertData[i].vert += diff;
				}
				else
					bsTriShape->vertData[i].vert += offset;
			}
		}
	}
}

void NifFile::ScaleShape(const std::string& shapeName, const Vector3& scale, std::unordered_map<ushort, float>* mask) {
	NiShape* shape = FindShapeByName(shapeName);
	if (!shape)
		return;

	Vector3 root;
	GetRootTranslation(root);

	if (shape->HasType<NiTriBasedGeom>()) {
		auto geomData = hdr->GetBlock<NiGeometryData>(shape->GetDataRef());
		if (!geomData)
			return;

		std::unordered_map<ushort, Vector3> diff;
		for (int i = 0; i < geomData->vertices.size(); i++) {
			Vector3 target = geomData->vertices[i] - root;
			target.x *= scale.x;
			target.y *= scale.y;
			target.z *= scale.z;
			diff[i] = geomData->vertices[i] - target;

			if (mask) {
				float maskFactor = 1.0f;
				if (mask->find(i) != mask->end()) {
					maskFactor = 1.0f - (*mask)[i];
					diff[i] *= maskFactor;
					target = geomData->vertices[i] - root + diff[i];
				}
			}
			geomData->vertices[i] = target;
		}
	}
	else if (shape->HasType<BSTriShape>()) {
		auto bsTriShape = dynamic_cast<BSTriShape*>(shape);
		if (!bsTriShape)
			return;

		std::unordered_map<ushort, Vector3> diff;
		for (int i = 0; i < bsTriShape->numVertices; i++) {
			Vector3 target = bsTriShape->vertData[i].vert - root;
			target.x *= scale.x;
			target.y *= scale.y;
			target.z *= scale.z;
			diff[i] = bsTriShape->vertData[i].vert - target;

			if (mask) {
				float maskFactor = 1.0f;
				if (mask->find(i) != mask->end()) {
					maskFactor = 1.0f - (*mask)[i];
					diff[i] *= maskFactor;
					target = bsTriShape->vertData[i].vert - root + diff[i];
				}
			}
			bsTriShape->vertData[i].vert = target;
		}
	}
}

void NifFile::RotateShape(const std::string& shapeName, const Vector3& angle, std::unordered_map<ushort, float>* mask) {
	NiShape* shape = FindShapeByName(shapeName);
	if (!shape)
		return;

	Vector3 root;
	GetRootTranslation(root);

	if (shape->HasType<NiTriBasedGeom>()) {
		auto geomData = hdr->GetBlock<NiGeometryData>(shape->GetDataRef());
		if (!geomData)
			return;

		std::unordered_map<ushort, Vector3> diff;
		for (int i = 0; i < geomData->vertices.size(); i++) {
			Vector3 target = geomData->vertices[i] - root;
			Matrix4 mat;
			mat.Rotate(angle.x * DEG2RAD, Vector3(1.0f, 0.0f, 0.0f));
			mat.Rotate(angle.y * DEG2RAD, Vector3(0.0f, 1.0f, 0.0f));
			mat.Rotate(angle.z * DEG2RAD, Vector3(0.0f, 0.0f, 1.0f));
			target = mat * target;
			diff[i] = geomData->vertices[i] - target;

			if (mask) {
				float maskFactor = 1.0f;
				if (mask->find(i) != mask->end()) {
					maskFactor = 1.0f - (*mask)[i];
					diff[i] *= maskFactor;
					target = geomData->vertices[i] - root + diff[i];
				}
			}
			geomData->vertices[i] = target;
		}
	}
	else if (shape->HasType<BSTriShape>()) {
		auto bsTriShape = dynamic_cast<BSTriShape*>(shape);
		if (!bsTriShape)
			return;

		std::unordered_map<ushort, Vector3> diff;
		for (int i = 0; i < bsTriShape->numVertices; i++) {
			Vector3 target = bsTriShape->vertData[i].vert - root;
			Matrix4 mat;
			mat.Rotate(angle.x * DEG2RAD, Vector3(1.0f, 0.0f, 0.0f));
			mat.Rotate(angle.y * DEG2RAD, Vector3(0.0f, 1.0f, 0.0f));
			mat.Rotate(angle.z * DEG2RAD, Vector3(0.0f, 0.0f, 1.0f));
			target = mat * target;
			diff[i] = bsTriShape->vertData[i].vert - target;

			if (mask) {
				float maskFactor = 1.0f;
				if (mask->find(i) != mask->end()) {
					maskFactor = 1.0f - (*mask)[i];
					diff[i] *= maskFactor;
					target = bsTriShape->vertData[i].vert - root + diff[i];
				}
			}
			bsTriShape->vertData[i].vert = target;
		}
	}
}

bool NifFile::GetAlphaForShape(const std::string& shapeName, ushort& outFlags, byte& outThreshold) {
	NiShape* shape = FindShapeByName(shapeName);
	if (!shape)
		return false;

	int alphaRef = shape->GetAlphaPropertyRef();
	if (alphaRef == 0xFFFFFFFF) {
		for (int i = 0; i < shape->propertyRefs.GetSize(); i++) {
			auto alphaProp = hdr->GetBlock<NiAlphaProperty>(shape->propertyRefs.GetBlockRef(i));
			if (alphaProp) {
				alphaRef = shape->propertyRefs.GetBlockRef(i);
				break;
			}
		}
	}

	auto alpha = hdr->GetBlock<NiAlphaProperty>(alphaRef);
	if (!alpha)
		return false;

	outFlags = alpha->flags;
	outThreshold = alpha->threshold;
	return true;
}

void NifFile::SetAlphaForShape(const std::string& shapeName, ushort flags, ushort threshold) {
	NiShape* shape = FindShapeByName(shapeName);
	if (!shape)
		return;

	int alphaRef = shape->GetAlphaPropertyRef();
	if (alphaRef == 0xFFFFFFFF) {
		for (int i = 0; i < shape->propertyRefs.GetSize(); i++) {
			auto alphaProp = hdr->GetBlock<NiAlphaProperty>(shape->propertyRefs.GetBlockRef(i));
			if (alphaProp) {
				alphaRef = shape->propertyRefs.GetBlockRef(i);
				break;
			}
		}
	}

	if (alphaRef == 0xFFFFFFFF) {
		NiShader* shader = GetShader(shapeName);
		if (!shader)
			return;

		auto alphaProp = new NiAlphaProperty(hdr);
		alphaRef = hdr->AddBlock(alphaProp);

		if (shader->HasType<BSShaderPPLightingProperty>())
			shape->propertyRefs.AddBlockRef(alphaRef);
		else
			shape->SetAlphaPropertyRef(alphaRef);
	}

	auto alpha = hdr->GetBlock<NiAlphaProperty>(alphaRef);
	if (!alpha)
		return;

	alpha->flags = flags;
	alpha->threshold = (byte)threshold;
}

bool NifFile::IsShapeSkinned(const std::string& shapeName) {
	NiShape* shape = FindShapeByName(shapeName);
	if (shape)
		return shape->IsSkinned();

	return false;
}

void NifFile::DeleteShape(const std::string& shapeName) {
	NiShape* shape = FindShapeByName(shapeName);
	if (!shape)
		return;

	hdr->DeleteBlock(shape->GetDataRef());
	DeleteShader(shapeName);
	DeleteSkinning(shapeName);

	for (int i = shape->propertyRefs.GetSize() - 1; i >= 0; --i)
		hdr->DeleteBlock(shape->propertyRefs.GetBlockRef(i));

	for (int i = shape->GetNumExtraData() - 1; i >= 0; --i)
		hdr->DeleteBlock(shape->GetExtraDataRef(i));

	int shapeID = GetBlockID(shape);
	hdr->DeleteBlock(shapeID);
}

void NifFile::DeleteShader(const std::string& shapeName) {
	NiShape* shape = FindShapeByName(shapeName);
	if (!shape)
		return;

	if (shape->GetShaderPropertyRef() != 0xFFFFFFFF) {
		auto shader = hdr->GetBlock<NiShader>(shape->GetShaderPropertyRef());
		if (shader) {
			hdr->DeleteBlock(shader->GetTextureSetRef());
			hdr->DeleteBlock(shader->GetControllerRef());
			hdr->DeleteBlock(shape->GetShaderPropertyRef());
			shape->SetShaderPropertyRef(0xFFFFFFFF);
		}
	}

	DeleteAlpha(shapeName);

	for (int i = 0; i < shape->propertyRefs.GetSize(); i++) {
		auto shader = hdr->GetBlock<NiShader>(shape->propertyRefs.GetBlockRef(i));
		if (shader) {
			if (shader->HasType<BSShaderPPLightingProperty>() || shader->HasType<NiMaterialProperty>()) {
				hdr->DeleteBlock(shader->GetTextureSetRef());
				hdr->DeleteBlock(shader->GetControllerRef());
				hdr->DeleteBlock(shape->propertyRefs.GetBlockRef(i));
				i--;
				continue;
			}
		}
	}
}

void NifFile::DeleteAlpha(const std::string& shapeName) {
	NiShape* shape = FindShapeByName(shapeName);
	if (!shape)
		return;

	auto alpha = hdr->GetBlock<NiAlphaProperty>(shape->GetAlphaPropertyRef());
	if (alpha) {
		hdr->DeleteBlock(shape->GetAlphaPropertyRef());
		shape->SetAlphaPropertyRef(0xFFFFFFFF);
	}

	for (int i = 0; i < shape->propertyRefs.GetSize(); i++) {
		alpha = hdr->GetBlock<NiAlphaProperty>(shape->propertyRefs.GetBlockRef(i));
		if (alpha) {
			hdr->DeleteBlock(shape->propertyRefs.GetBlockRef(i));
			i--;
			continue;
		}
	}
}

void NifFile::DeleteSkinning(const std::string& shapeName) {
	NiShape* shape = FindShapeByName(shapeName);
	if (!shape)
		return;

	auto skinInst = hdr->GetBlock<NiSkinInstance>(shape->GetSkinInstanceRef());
	if (skinInst) {
		hdr->DeleteBlock(skinInst->GetDataRef());
		hdr->DeleteBlock(skinInst->GetSkinPartitionRef());
		hdr->DeleteBlock(shape->GetSkinInstanceRef());
		shape->SetSkinInstanceRef(0xFFFFFFFF);
	}

	auto bsSkinInst = hdr->GetBlock<BSSkinInstance>(shape->GetSkinInstanceRef());
	if (bsSkinInst) {
		hdr->DeleteBlock(bsSkinInst->GetDataRef());
		hdr->DeleteBlock(shape->GetSkinInstanceRef());
		shape->SetSkinInstanceRef(0xFFFFFFFF);
	}

	shape->SetSkinned(false);

	NiShader* shader = GetShader(shapeName);
	if (shader)
		shader->SetSkinned(false);
}

bool NifFile::DeleteVertsForShape(const std::string& shapeName, const std::vector<ushort>& indices) {
	if (indices.empty())
		return false;

	NiShape* shape = FindShapeByName(shapeName);
	if (!shape)
		return false;

	int skinRef = shape->GetSkinInstanceRef();

	auto geomData = hdr->GetBlock<NiTriBasedGeomData>(shape->GetDataRef());
	if (geomData) {
		geomData->notifyVerticesDelete(indices);
		if (geomData->numVertices == 0 || geomData->numTriangles == 0) {
			// Deleted all verts or tris, remove shape and children
			DeleteShape(shapeName);
			return true;
		}
	}

	auto bsTriShape = dynamic_cast<BSTriShape*>(shape);
	if (bsTriShape) {
		bsTriShape->notifyVerticesDelete(indices);
		if (bsTriShape->numVertices == 0 || bsTriShape->numTriangles == 0) {
			// Deleted all verts or tris, remove shape and children
			DeleteShape(shapeName);
			return true;
		}
	}

	auto skinInst = hdr->GetBlock<NiSkinInstance>(skinRef);
	if (skinInst) {
		auto skinData = hdr->GetBlock<NiSkinData>(skinInst->GetDataRef());
		if (skinData)
			skinData->notifyVerticesDelete(indices);

		auto skinPartition = hdr->GetBlock<NiSkinPartition>(skinInst->GetSkinPartitionRef());
		if (skinPartition) {
			skinPartition->notifyVerticesDelete(indices);
			std::vector<int> emptyIndices;
			if (skinPartition->RemoveEmptyPartitions(emptyIndices)) {
				if (skinInst->HasType<BSDismemberSkinInstance>()) {
					auto bsdSkinInst = static_cast<BSDismemberSkinInstance*>(skinInst);
					for (auto &i : emptyIndices)
						bsdSkinInst->RemovePartition(i);

					UpdatePartitionFlags(shapeName);
				}
			}
		}
	}

	return false;
}

int NifFile::CalcShapeDiff(const std::string& shapeName, const std::vector<Vector3>* targetData, std::unordered_map<ushort, Vector3>& outDiffData, float scale) {
	outDiffData.clear();
	const std::vector<Vector3>* myData = GetRawVertsForShape(shapeName);
	if (!myData)
		return 1;

	if (!targetData)
		return 2;

	if (myData->size() != targetData->size())
		return 3;

	for (int i = 0; i < myData->size(); i++) {
		Vector3 v;
		v.x = (targetData->at(i).x * scale) - myData->at(i).x;
		v.y = (targetData->at(i).y * scale) - myData->at(i).y;
		v.z = (targetData->at(i).z * scale) - myData->at(i).z;

		if (v.IsZero(true))
			continue;

		outDiffData[i] = v;
	}

	return 0;
}

int NifFile::CalcUVDiff(const std::string& shapeName, const std::vector<Vector2>* targetData, std::unordered_map<ushort, Vector3>& outDiffData, float scale) {
	outDiffData.clear();
	const std::vector<Vector2>* myData = GetUvsForShape(shapeName);
	if (!myData)
		return 1;

	if (!targetData)
		return 2;

	if (myData->size() != targetData->size())
		return 3;

	for (int i = 0; i < myData->size(); i++) {
		Vector3 v;
		v.x = (targetData->at(i).u - myData->at(i).u) * scale;
		v.y = (targetData->at(i).v - myData->at(i).v) * scale;

		if (v.IsZero(true))
			continue;

		outDiffData[i] = v;
	}

	return 0;
}

void NifFile::UpdateSkinPartitions(const std::string& shapeName) {
	NiShape* shape = FindShapeByName(shapeName);
	if (!shape)
		return;

	NiSkinData* skinData = nullptr;
	NiSkinPartition* skinPart = nullptr;
	auto skinInst = hdr->GetBlock<NiSkinInstance>(shape->GetSkinInstanceRef());
	if (skinInst) {
		skinData = hdr->GetBlock<NiSkinData>(skinInst->GetDataRef());
		skinPart = hdr->GetBlock<NiSkinPartition>(skinInst->GetSkinPartitionRef());

		if (!skinData || !skinPart)
			return;
	}
	else
		return;

	TriangulatePartitions(shapeName);

	BSTriShape* bsTriShape = nullptr;
	auto bsdSkinInst = dynamic_cast<BSDismemberSkinInstance*>(skinInst);

	std::vector<Triangle> tris;
	ushort numTriangles;
	ushort numVerts;

	if (shape->HasType<NiTriShape>()) {
		auto shapeData = hdr->GetBlock<NiTriShapeData>(shape->GetDataRef());
		if (!shapeData)
			return;

		tris = shapeData->triangles;
		numTriangles = shapeData->numTriangles;
		numVerts = shapeData->numVertices;
	}
	else if (shape->HasType<NiTriStrips>()) {
		auto stripsData = hdr->GetBlock<NiTriStripsData>(shape->GetDataRef());
		if (!stripsData)
			return;

		stripsData->StripsToTris(&tris);
		numTriangles = stripsData->numTriangles;
		numVerts = stripsData->numVertices;
	}
	else if (shape->HasType<BSTriShape>()) {
		bsTriShape = dynamic_cast<BSTriShape*>(shape);
		if (!bsTriShape)
			return;

		tris = bsTriShape->triangles;
		numTriangles = bsTriShape->numTriangles;
		numVerts = bsTriShape->numVertices;
		bsTriShape->CalcDataSizes();
	}
	else
		return;

	// Align triangles for comparisons
	for (auto &t : tris)
		t.rot();

	// Make maps of vertices to bones and weights
	std::unordered_map<ushort, std::vector<SkinWeight>> vertBoneWeights;

	int boneIndex = 0;
	for (auto &bone : skinData->bones) {
		for (auto &bw : bone.vertexWeights)
			vertBoneWeights[bw.index].push_back(SkinWeight(boneIndex, bw.weight));

		boneIndex++;
	}

	// Sort weights and corresponding bones
	for (auto &bw : vertBoneWeights)
		sort(bw.second.begin(), bw.second.end(), BoneWeightsSort());

	// Enforce maximum vertex bone weight count
	int maxBonesPerVertex = 4;

	for (auto &bw : vertBoneWeights)
		if (bw.second.size() > maxBonesPerVertex)
			bw.second.erase(bw.second.begin() + maxBonesPerVertex, bw.second.end());

	std::unordered_map<int, std::vector<int>> vertTris;
	for (int t = 0; t < tris.size(); t++) {
		vertTris[tris[t].p1].push_back(t);
		vertTris[tris[t].p2].push_back(t);
		vertTris[tris[t].p3].push_back(t);
	}

	// Lambda for finding bones that have the tri assigned
	std::set<int> triBones;
	auto fTriBones = [&triBones, &tris, &vertBoneWeights](const int tri) {
		triBones.clear();

		if (tri >= 0 && tris.size() > tri) {
			ushort* p = &tris[tri].p1;
			for (int i = 0; i < 3; i++, p++)
				for (auto &tb : vertBoneWeights[*p])
					triBones.insert(tb.index);
		}
	};

	std::unordered_map<int, int> triParts;
	triParts.reserve(numTriangles);
	for (int i = 0; i < numTriangles; i++)
		triParts.emplace(i, -1);

	std::vector<bool> usedTris;
	usedTris.resize(numTriangles);

	std::vector<bool> usedVerts;
	usedVerts.resize(numVerts);

	// 18 for pre-SK
	int maxBonesPerPartition = hdr->GetUserVersion() >= 12 ? std::numeric_limits<int>::max() : 18;
	std::unordered_map<int, std::set<int>> partBones;

	std::vector<NiSkinPartition::PartitionBlock> partitions;
	for (int partID = 0; partID < skinPart->partitions.size(); partID++) {
		fill(usedVerts.begin(), usedVerts.end(), false);

		ushort numTrisInPart = 0;
		for (int it = 0; it < skinPart->partitions[partID].numTriangles;) {
			// Find the actual tri index from the partition tri index
			Triangle tri;
			if (bsTriShape) {
				tri = skinPart->partitions[partID].triangles[it];
			}
			else {
				tri.p1 = skinPart->partitions[partID].vertexMap[skinPart->partitions[partID].triangles[it].p1];
				tri.p2 = skinPart->partitions[partID].vertexMap[skinPart->partitions[partID].triangles[it].p2];
				tri.p3 = skinPart->partitions[partID].vertexMap[skinPart->partitions[partID].triangles[it].p3];
			}

			tri.rot();

			// Find current tri in full list
			auto realTri = find_if(tris.begin(), tris.end(), [&tri](const Triangle& t) { return t.CompareIndices(tri); });
			if (realTri == tris.end()) {
				it++;
				continue;
			}

			int triIndex = realTri - tris.begin();

			// Conditional increment in loop
			if (usedTris[triIndex]) {
				it++;
				continue;
			}

			// Get associated bones for the current tri
			fTriBones(triIndex);

			if (triBones.size() > maxBonesPerPartition) {
				// TODO: get rid of some bone influences on this tri before trying to put it anywhere
			}

			// How many new bones are in the tri's bone list?
			int newBoneCount = 0;
			for (auto &tb : triBones)
				if (partBones[partID].find(tb) == partBones[partID].end())
					newBoneCount++;

			if (partBones[partID].size() + newBoneCount > maxBonesPerPartition) {
				// Too many bones for this partition, make a new partition starting with this triangle
				NiSkinPartition::PartitionBlock tempPart;
				tempPart.triangles.assign(skinPart->partitions[partID].triangles.begin() + numTrisInPart, skinPart->partitions[partID].triangles.end());
				tempPart.numTriangles = tempPart.triangles.size();

				tempPart.vertexMap = skinPart->partitions[partID].vertexMap;
				tempPart.numVertices = tempPart.vertexMap.size();

				skinPart->partitions.insert(skinPart->partitions.begin() + partID + 1, tempPart);
				skinPart->numPartitions++;

				if (bsdSkinInst) {
					auto partInfo = bsdSkinInst->GetPartitions();

					BSDismemberSkinInstance::PartitionInfo info;
					info.flags = 1;
					info.partID = partInfo[partID].partID;
					partInfo.insert(partInfo.begin() + partID, info);

					bsdSkinInst->SetPartitions(partInfo);
				}

				// Partition will be recreated and filled later
				break;
			}

			partBones[partID].insert(triBones.begin(), triBones.end());
			triParts[triIndex] = partID;
			usedTris[triIndex] = true;
			usedVerts[tri.p1] = true;
			usedVerts[tri.p2] = true;
			usedVerts[tri.p3] = true;
			numTrisInPart++;

			std::queue<int> vertQueue;
			auto fSelectVerts = [&usedVerts, &tris, &vertQueue](const int tri) {
				if (!usedVerts[tris[tri].p1]) {
					usedVerts[tris[tri].p1] = true;
					vertQueue.push(tris[tri].p1);
				}
				if (!usedVerts[tris[tri].p2]) {
					usedVerts[tris[tri].p2] = true;
					vertQueue.push(tris[tri].p2);
				}
				if (!usedVerts[tris[tri].p3]) {
					usedVerts[tris[tri].p3] = true;
					vertQueue.push(tris[tri].p3);
				}
			};

			// Select the tri's unvisited verts for adjacency examination
			fSelectVerts(triIndex);

			while (!vertQueue.empty()) {
				int adjVert = vertQueue.front();
				vertQueue.pop();

				for (auto &adjTri : vertTris[adjVert]) {
					// Skip triangles we've already assigned
					if (usedTris[adjTri])
						continue;

					// Get associated bones for the current tri
					fTriBones(adjTri);

					if (triBones.size() > maxBonesPerPartition) {
						// TODO: get rid of some bone influences on this tri before trying to put it anywhere
					}

					// How many new bones are in the tri's bonelist?
					newBoneCount = 0;
					for (auto &tb : triBones)
						if (partBones[partID].find(tb) == partBones[partID].end())
							newBoneCount++;

					// Too many bones for this partition, ignore this tri, it's catched in the outer loop later
					if (partBones[partID].size() + newBoneCount > maxBonesPerPartition)
						continue;

					// Save the next set of adjacent verts
					fSelectVerts(adjTri);

					partBones[partID].insert(triBones.begin(), triBones.end());
					triParts[adjTri] = partID;
					usedTris[adjTri] = true;
					numTrisInPart++;
				}
			}

			// Next outer triangle
			it++;
		}

		NiSkinPartition::PartitionBlock part;
		part.hasBoneIndices = true;
		part.hasFaces = true;
		part.hasVertexMap = true;
		part.hasVertexWeights = true;
		part.numWeightsPerVertex = maxBonesPerVertex;

		std::unordered_map<int, int> vertMap;
		for (int triID = 0; triID < tris.size(); triID++) {
			if (triParts[triID] != partID)
				continue;

			Triangle tri = tris[triID];
			if (vertMap.find(tri.p1) == vertMap.end()) {
				vertMap[tri.p1] = part.numVertices;
				part.vertexMap.push_back(tri.p1);
				tri.p1 = part.numVertices++;
			}
			else
				tri.p1 = vertMap[tri.p1];

			if (vertMap.find(tri.p2) == vertMap.end()) {
				vertMap[tri.p2] = part.numVertices;
				part.vertexMap.push_back(tri.p2);
				tri.p2 = part.numVertices++;
			}
			else
				tri.p2 = vertMap[tri.p2];

			if (vertMap.find(tri.p3) == vertMap.end()) {
				vertMap[tri.p3] = part.numVertices;
				part.vertexMap.push_back(tri.p3);
				tri.p3 = part.numVertices++;
			}
			else
				tri.p3 = vertMap[tri.p3];

			tri.rot();

			if (bsTriShape) {
				part.triangles.push_back(tris[triID]);
				part.trueTriangles.push_back(tris[triID]);
			}
			else
				part.triangles.push_back(tri);
		}

		part.numTriangles = part.triangles.size();

		// Copy relevant data from shape to partition
		if (bsTriShape) {
			part.vertFlags1 = bsTriShape->vertFlags1;
			part.vertFlags2 = bsTriShape->vertFlags2;
			part.vertFlags3 = bsTriShape->vertFlags3;
			part.vertFlags4 = bsTriShape->vertFlags4;
			part.vertFlags5 = bsTriShape->vertFlags5;
			part.vertFlags6 = bsTriShape->vertFlags6;
			part.vertFlags7 = bsTriShape->vertFlags7;
			part.vertFlags8 = bsTriShape->vertFlags8;
		}

		std::unordered_map<int, int> boneLookup;
		boneLookup.reserve(partBones[partID].size());
		part.numBones = partBones[partID].size();
		part.bones.reserve(part.numBones);

		for (auto &b : partBones[partID]) {
			part.bones.push_back(b);
			boneLookup[b] = part.bones.size() - 1;
		}

		for (auto &v : part.vertexMap) {
			BoneIndices b;
			b.i1 = b.i2 = b.i3 = b.i4 = 0;

			VertexWeight vw;
			vw.w1 = vw.w2 = vw.w3 = vw.w4 = 0.0f;

			byte* pb = &b.i1;
			float* pw = &vw.w1;

			float tot = 0.0f;
			for (int bi = 0; bi < vertBoneWeights[v].size(); bi++) {
				if (bi == 4)
					break;

				pb[bi] = boneLookup[vertBoneWeights[v][bi].index];
				pw[bi] = vertBoneWeights[v][bi].weight;
				tot += pw[bi];
			}

			for (int bi = 0; bi < 4; bi++)
				pw[bi] /= tot;

			part.boneIndices.push_back(b);
			part.vertexWeights.push_back(vw);
		}

		partitions.push_back(std::move(part));
	}

	skinPart->numPartitions = partitions.size();
	skinPart->partitions = std::move(partitions);

	if (bsTriShape) {
		skinPart->numVertices = bsTriShape->numVertices;
		skinPart->dataSize = bsTriShape->dataSize;
		skinPart->vertexSize = bsTriShape->vertexSize;
		skinPart->vertData = bsTriShape->vertData;
		skinPart->vertFlags1 = bsTriShape->vertFlags1;
		skinPart->vertFlags2 = bsTriShape->vertFlags2;
		skinPart->vertFlags3 = bsTriShape->vertFlags3;
		skinPart->vertFlags4 = bsTriShape->vertFlags4;
		skinPart->vertFlags5 = bsTriShape->vertFlags5;
		skinPart->vertFlags6 = bsTriShape->vertFlags6;
		skinPart->vertFlags7 = bsTriShape->vertFlags7;
		skinPart->vertFlags8 = bsTriShape->vertFlags8;
	}

	UpdatePartitionFlags(shapeName);
}

bool NifFile::TriangulatePartitions(const std::string& shapeName) {
	NiShape* shape = FindShapeByName(shapeName);
	if (!shape)
		return false;

	NiSkinData* skinData = nullptr;
	NiSkinPartition* skinPart = nullptr;
	auto skinInst = hdr->GetBlock<NiSkinInstance>(shape->GetSkinInstanceRef());
	if (skinInst) {
		skinData = hdr->GetBlock<NiSkinData>(skinInst->GetDataRef());
		skinPart = hdr->GetBlock<NiSkinPartition>(skinInst->GetSkinPartitionRef());

		if (!skinData || !skinPart)
			return false;
	}
	else
		return false;

	bool triangulated = false;
	std::vector<NiSkinPartition::PartitionBlock> partitions;
	for (int partID = 0; partID < skinPart->partitions.size(); partID++) {
		// Triangulate partition if stripified
		if (skinPart->partitions[partID].numStrips > 0) {
			std::vector<Triangle> stripTris;
			for (auto &strip : skinPart->partitions[partID].strips) {
				if (strip.size() < 3)
					continue;

				ushort a;
				ushort b = strip[0];
				ushort c = strip[1];
				bool flip = false;

				for (int s = 2; s < strip.size(); s++) {
					a = b;
					b = c;
					c = strip[s];

					if (a != b && b != c && c != a) {
						if (!flip)
							stripTris.push_back(Triangle(a, b, c));
						else
							stripTris.push_back(Triangle(a, c, b));
					}

					flip = !flip;
				}
			}

			skinPart->partitions[partID].numTriangles = stripTris.size();
			skinPart->partitions[partID].triangles = move(stripTris);
			skinPart->partitions[partID].numStrips = 0;
			skinPart->partitions[partID].strips.clear();
			skinPart->partitions[partID].stripLengths.clear();
			triangulated = true;
		}
	}

	return triangulated;
}

void NifFile::UpdatePartitionFlags(const std::string& shapeName) {
	NiShape* shape = FindShapeByName(shapeName);
	if (!shape)
		return;

	auto bsdSkinInst = hdr->GetBlock<BSDismemberSkinInstance>(shape->GetSkinInstanceRef());
	if (!bsdSkinInst)
		return;

	auto skinPart = hdr->GetBlock<NiSkinPartition>(bsdSkinInst->GetSkinPartitionRef());
	if (!skinPart)
		return;

	auto partInfo = bsdSkinInst->GetPartitions();
	for (int i = 0; i < partInfo.size(); i++) {
		if (i != 0) {
			// Start a new set if the previous bones are different
			if (skinPart->partitions[i].bones != skinPart->partitions[i - 1].bones)
				partInfo[i].flags = 257;
			else
				partInfo[i].flags = 1;
		}
		else
			partInfo[i].flags = 257;
	}

	bsdSkinInst->SetPartitions(partInfo);
}

void NifFile::CreateSkinning(const std::string& shapeName) {
	NiShape* shape = FindShapeByName(shapeName);
	if (!shape)
		return;

	if (shape->HasType<NiTriShape>()) {
		if (shape->GetSkinInstanceRef() == 0xFFFFFFFF) {
			auto nifSkinData = new NiSkinData(hdr);
			int skinDataID = hdr->AddBlock(nifSkinData);

			auto nifSkinPartition = new NiSkinPartition(hdr);
			int partID = hdr->AddBlock(nifSkinPartition);

			auto nifDismemberInst = new BSDismemberSkinInstance(hdr);
			int dismemberID = hdr->AddBlock(nifDismemberInst);

			nifDismemberInst->SetDataRef(skinDataID);
			nifDismemberInst->SetSkinPartitionRef(partID);
			nifDismemberInst->SetSkeletonRootRef(0);
			shape->SetSkinInstanceRef(dismemberID);
			shape->SetSkinned(true);
		}
	}
	else if (shape->HasType<NiTriStrips>()) {
		if (shape->GetSkinInstanceRef() == 0xFFFFFFFF) {
			auto nifSkinData = new NiSkinData(hdr);
			int skinDataID = hdr->AddBlock(nifSkinData);

			auto nifSkinPartition = new NiSkinPartition(hdr);
			int partID = hdr->AddBlock(nifSkinPartition);

			auto nifDismemberInst = new BSDismemberSkinInstance(hdr);
			int skinID = hdr->AddBlock(nifDismemberInst);

			nifDismemberInst->SetDataRef(skinDataID);
			nifDismemberInst->SetSkinPartitionRef(partID);
			nifDismemberInst->SetSkeletonRootRef(0);
			shape->SetSkinInstanceRef(skinID);
			shape->SetSkinned(true);
		}
	}
	else if (shape->HasType<BSTriShape>()) {
		if (shape->GetSkinInstanceRef() == 0xFFFFFFFF) {
			int skinInstID;
			if (hdr->GetUserVersion2() == 100) {
				auto nifSkinData = new NiSkinData(hdr);
				int skinDataID = hdr->AddBlock(nifSkinData);

				auto nifSkinPartition = new NiSkinPartition(hdr);
				int partID = hdr->AddBlock(nifSkinPartition);

				auto nifDismemberInst = new BSDismemberSkinInstance(hdr);
				skinInstID = hdr->AddBlock(nifDismemberInst);

				nifDismemberInst->SetDataRef(skinDataID);
				nifDismemberInst->SetSkinPartitionRef(partID);
				nifDismemberInst->SetSkeletonRootRef(0);
			}
			else {
				auto newSkinInst = new BSSkinInstance(hdr);
				skinInstID = hdr->AddBlock(newSkinInst);

				auto newBoneData = new BSSkinBoneData(hdr);
				int boneDataRef = hdr->AddBlock(newBoneData);

				newSkinInst->SetTargetRef(GetRootNodeID());
				newSkinInst->SetDataRef(boneDataRef);
			}

			shape->SetSkinInstanceRef(skinInstID);
			shape->SetSkinned(true);
		}
	}

	NiShader* shader = GetShader(shapeName);
	if (shader)
		shader->SetSkinned(true);
}

void NifFile::UpdateBoundingSphere(const std::string& shapeName) {
	NiShape* shape = FindShapeByName(shapeName);
	if (!shape)
		return;

	shape->UpdateBounds();
}

void NifFile::SetShapeDynamic(const std::string& shapeName) {
	NiShape* shape = FindShapeByName(shapeName);
	if (!shape)
		return;

	// Set consistency flag to mutable
	auto geomData = hdr->GetBlock<NiGeometryData>(shape->GetDataRef());
	if (geomData)
		geomData->consistencyFlags = 0;
}
