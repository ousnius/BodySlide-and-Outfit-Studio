/*
BodySlide and Outfit Studio
Copyright (C) 2016  Caliente & ousnius
See the included LICENSE file
*/

#include "NifFile.h"
#include <queue>

NifFile::NifFile() {
	isValid = false;
	hasUnknown = false;
}

NifFile::NifFile(NifFile& other) {
	CopyFrom(other);
}

NifFile::~NifFile() {
	for (int i = 0; i < blocks.size(); i++)
		delete blocks[i];

	blocks.clear();
}

NiTriBasedGeom* NifFile::geomForName(const string& name, int dupIndex) {
	int numFound = 0;
	for (auto& block : blocks) {
		ushort type = block->blockType;
		if (type == NITRISHAPE || type == NITRISTRIPS) {
			NiTriBasedGeom* geom = dynamic_cast<NiTriBasedGeom*>(block);
			if (geom && !name.compare(geom->GetName())) {
				if (numFound >= dupIndex)
					return geom;
				numFound++;
			}
		}
	}
	return nullptr;
}
BSTriShape* NifFile::geomForNameF4(const string& name, int dupIndex) {
	int numFound = 0;
	for (auto& block : blocks) {
		ushort type = block->blockType;
		if (type == BSSUBINDEXTRISHAPE || type == BSTRISHAPE || type == BSMESHLODTRISHAPE) {
			BSTriShape* geom = dynamic_cast<BSTriShape*>(block);
			if (geom && !name.compare(geom->GetName())) {
				if (numFound >= dupIndex)
					return geom;
				numFound++;
			}
		}
	}
	return nullptr;
}

NiAVObject* NifFile::avObjectForName(const string& name, int dupIndex) {
	int numFound = 0;
	for (auto& block : blocks) {
		ushort type = block->blockType;
		if (type == NITRISHAPE || type == NITRISTRIPS || type == BSSUBINDEXTRISHAPE || type == BSTRISHAPE || type == BSMESHLODTRISHAPE) {
			NiAVObject* avo = dynamic_cast<NiAVObject*>(block);
			if (avo && !name.compare(avo->GetName())) {
				if (numFound >= dupIndex)
					return avo;
				numFound++;
			}
		}
	}
	return nullptr;
}

int NifFile::shapeDataIdForName(const string& name, int& outBlockType) {
	int i = 0;
	for (auto& block : blocks) {
		ushort type = block->blockType;
		if (type == NITRISHAPE || type == NITRISTRIPS) {
			NiTriBasedGeom* geom = dynamic_cast<NiTriBasedGeom*>(block);
			if (geom && !name.compare(geom->GetName())) {
				NiTriBasedGeomData* geomData = dynamic_cast<NiTriBasedGeomData*>(GetBlock(geom->dataRef));
				outBlockType = geomData->blockType;
				return geom->dataRef;
			}
		}
		else if (type == BSSUBINDEXTRISHAPE || type == BSTRISHAPE || type == BSMESHLODTRISHAPE) {
			BSTriShape* geom = dynamic_cast<BSTriShape*>(block);
			outBlockType = type;
			if (geom && !name.compare(geom->GetName())) {
				return i;
			}
		}
		i++;
	}
	return -1;
}

int NifFile::shapeIdForName(const string& name) {
	int id = -1;
	for (auto& block : blocks) {
		id++;
		ushort type = block->blockType;
		if (type == NITRISHAPE || type == NITRISTRIPS) {
			NiTriBasedGeom* geom = dynamic_cast<NiTriBasedGeom*>(block);
			if (geom && !name.compare(geom->GetName()))
				return id;
		}
		else if (type == BSSUBINDEXTRISHAPE || type == BSTRISHAPE || type == BSMESHLODTRISHAPE) {
			BSTriShape* geom = dynamic_cast<BSTriShape*>(block);
			if (geom && !name.compare(geom->GetName())) {
				return id;
			}
		}
	}
	return -1;
}

NiNode* NifFile::nodeForName(const string& name) {
	for (auto& block : blocks) {
		if (block->blockType == NINODE) {
			NiNode* node = (NiNode*)block;
			if (!name.compare(node->GetName()))
				return node;
		}
	}
	return nullptr;
}

int NifFile::shapeBoneIndex(const string& shapeName, const string& boneName) {
	int skinRef = -1;

	BSTriShape* bsTriShape = geomForNameF4(shapeName);
	if (bsTriShape) {
		NiBoneContainer* boneList = dynamic_cast<NiBoneContainer*>(GetBlock(bsTriShape->skinInstanceRef));
		if (boneList) {
			for (int i = 0; i < boneList->bones.size(); i++) {
				NiNode* node = dynamic_cast<NiNode*>(GetBlock(boneList->bones[i]));
				if (node && node->GetName() == boneName)
					return i;
			}
		}
	}

	NiTriBasedGeom* geom = geomForName(shapeName);
	if (geom)
		skinRef = geom->skinInstanceRef;

	if (skinRef == -1)
		return -1;

	NiSkinInstance* skinInst = dynamic_cast<NiSkinInstance*>(GetBlock(skinRef));
	if (skinInst) {
		for (int i = 0; i < skinInst->bones.size(); i++) {
			NiNode* node = dynamic_cast<NiNode*>(GetBlock(skinInst->bones[i]));
			if (node && node->GetName() == boneName)
				return i;
		}
	}

	return -1;
}

void NifFile::CopyFrom(NifFile& other) {
	if (isValid)
		Clear();

	isValid = other.isValid;
	hasUnknown = other.hasUnknown;
	fileName = other.fileName;
	hdr = other.hdr;

	for (int i = 0; i < other.blocks.size(); i++) {
		NiObject* blockCopy = nullptr;
		switch (other.blocks[i]->blockType) {
		case NIUNKNOWN:
			blockCopy = new NiUnknown(other.blocks[i]->CalcBlockSize());
			((NiUnknown*)blockCopy)->Clone((NiUnknown*)other.blocks[i]);
			break;
		case NITRISHAPE:
			blockCopy = new NiTriShape((*(NiTriShape*)other.blocks[i]));
			break;
		case NITRISHAPEDATA:
			blockCopy = new NiTriShapeData((*(NiTriShapeData*)other.blocks[i]));
			break;
		case NITRISTRIPS:
			blockCopy = new NiTriStrips((*(NiTriStrips*)other.blocks[i]));
			break;
		case NITRISTRIPSDATA:
			blockCopy = new NiTriStripsData((*(NiTriStripsData*)other.blocks[i]));
			break;
		case NINODE:
			blockCopy = new NiNode((*(NiNode*)other.blocks[i]));
			break;
		case BSDISMEMBERSKININSTANCE:
			blockCopy = new BSDismemberSkinInstance((*(BSDismemberSkinInstance*)other.blocks[i]));
			break;
		case NISKINPARTITION:
			blockCopy = new NiSkinPartition((*(NiSkinPartition*)other.blocks[i]));
			break;
		case NISKININSTANCE:
			blockCopy = new NiSkinInstance((*(NiSkinInstance*)other.blocks[i]));
			break;
		case BSLIGHTINGSHADERPROPERTY:
			blockCopy = new BSLightingShaderProperty((*(BSLightingShaderProperty*)other.blocks[i]));
			break;
		case NIALPHAPROPERTY:
			blockCopy = new NiAlphaProperty((*(NiAlphaProperty*)other.blocks[i]));
			break;
		case NISKINDATA:
			blockCopy = new NiSkinData((*(NiSkinData*)other.blocks[i]));
			break;
		case BSSHADERTEXTURESET:
			blockCopy = new BSShaderTextureSet((*(BSShaderTextureSet*)other.blocks[i]));
			break;
		case NISTRINGEXTRADATA:
			blockCopy = new NiStringExtraData((*(NiStringExtraData*)other.blocks[i]));
			break;
		case BSSHADERPPLIGHTINGPROPERTY:
			blockCopy = new BSShaderPPLightingProperty((*(BSShaderPPLightingProperty*)other.blocks[i]));
			break;
		case NIMATERIALPROPERTY:
			blockCopy = new NiMaterialProperty((*(NiMaterialProperty*)other.blocks[i]));
			break;
		case NISTENCILPROPERTY:
			blockCopy = new NiStencilProperty((*(NiStencilProperty*)other.blocks[i]));
			break;
		case BSEFFECTSHADERPROPERTY:
			blockCopy = new BSEffectShaderProperty((*(BSEffectShaderProperty*)other.blocks[i]));
			break;
		case NIFLOATINTERPOLATOR:
			blockCopy = new NiFloatInterpolator((*(NiFloatInterpolator*)other.blocks[i]));
			break;
		case NITRANSFORMINTERPOLATOR:
			blockCopy = new NiTransformInterpolator((*(NiTransformInterpolator*)other.blocks[i]));
			break;
		case NIPOINT3INTERPOLATOR:
			blockCopy = new NiPoint3Interpolator((*(NiPoint3Interpolator*)other.blocks[i]));
			break;
		case BSLIGHTINGSHADERPROPERTYCOLORCONTROLLER:
			blockCopy = new BSLightingShaderPropertyColorController((*(BSLightingShaderPropertyColorController*)other.blocks[i]));
			break;
		case BSLIGHTINGSHADERPROPERTYFLOATCONTROLLER:
			blockCopy = new BSLightingShaderPropertyFloatController((*(BSLightingShaderPropertyFloatController*)other.blocks[i]));
			break;
		case BSEFFECTSHADERPROPERTYCOLORCONTROLLER:
			blockCopy = new BSEffectShaderPropertyColorController((*(BSEffectShaderPropertyColorController*)other.blocks[i]));
			break;
		case BSEFFECTSHADERPROPERTYFLOATCONTROLLER:
			blockCopy = new BSEffectShaderPropertyFloatController((*(BSEffectShaderPropertyFloatController*)other.blocks[i]));
			break;
		case BSTRISHAPE:
			blockCopy = new BSTriShape((*(BSTriShape*)other.blocks[i]));
			break;
		case BSSUBINDEXTRISHAPE:
			blockCopy = new BSSubIndexTriShape((*(BSSubIndexTriShape*)other.blocks[i]));
			break;
		case BSMESHLODTRISHAPE:
			blockCopy = new BSMeshLODTriShape(*(BSMeshLODTriShape*)other.blocks[i]);
			break;
		case BSSKININSTANCE:
			blockCopy = new BSSkinInstance(*(BSSkinInstance*)other.blocks[i]);
			break;
		case BSBONEDATA:
			blockCopy = new BSSkinBoneData(*(BSSkinBoneData*)other.blocks[i]);
			break;
		case BSCLOTHEXTRADATA:
			blockCopy = new BSClothExtraData(hdr);
			((BSClothExtraData*)blockCopy)->Clone((BSClothExtraData*)other.blocks[i]);
			break;
		case NIINTEGEREXTRADATA:
			blockCopy = new NiIntegerExtraData(*(NiIntegerExtraData*)other.blocks[i]);
			break;
		case BSXFLAGS:
			blockCopy = new BSXFlags(*(BSXFlags*)other.blocks[i]);
			break;
		case BSCONNECTPOINTPARENTS:
			blockCopy = new BSConnectPointParents(*(BSConnectPointParents*)other.blocks[i]);
			break;
		case BSCONNECTPOINTCHILDREN:
			blockCopy = new BSConnectPointChildren(*(BSConnectPointChildren*)other.blocks[i]);
			break;
		case NICOLLISIONOBJECT:
			blockCopy = new NiCollisionObject(*(NiCollisionObject*)other.blocks[i]);
			break;
		case BHKNICOLLISIONOBJECT:
			blockCopy = new bhkNiCollisionObject(*(bhkNiCollisionObject*)other.blocks[i]);
			break;
		case BHKCOLLISIONOBJECT:
			blockCopy = new bhkCollisionObject(*(bhkCollisionObject*)other.blocks[i]);
			break;
		case BHKNPCOLLISIONOBJECT:
			blockCopy = new bhkNPCollisionObject(*(bhkNPCollisionObject*)other.blocks[i]);
			break;
		case BHKPHYSICSSYSTEM:
			blockCopy = new bhkPhysicsSystem(hdr);
			((bhkPhysicsSystem*)blockCopy)->Clone((bhkPhysicsSystem*)other.blocks[i]);
			break;
		}

		if (blockCopy) {
			blockCopy->header = &hdr;
			blocks.push_back(blockCopy);
		}
	}
	hdr.blocks = &blocks;
}

void NifFile::Clear() {
	for (int i = 0; i < blocks.size(); i++)
		delete blocks[i];

	blocks.clear();
	hdr.Clear();
	isValid = false;
	hasUnknown = false;
}

int NifFile::Load(const string& filename) {
	string thisBlockTypeStr;
	ushort thisBlockTypeId;
	Clear();

	fstream file(filename.c_str(), ios_base::in | ios_base::binary);
	if (file.is_open()) {
		if (filename.rfind("\\") != string::npos)
			fileName = filename.substr(filename.rfind("\\"));
		else
			fileName = filename;

		hdr.Get(file);
		if (hdr.verStr[0] == 0) {
			Clear();
			return 1;
		}
		
		if (!(hdr.VerCheck(20, 2, 0, 7) && (hdr.userVersion == 11 || hdr.userVersion == 12))) {
			Clear();
			return 2;
		}

		hdr.blocks = &blocks;

		for (int i = 0; i < hdr.numBlocks; i++) {
			NiObject* block = nullptr;
			thisBlockTypeId = hdr.blockIndex[i];
			thisBlockTypeStr = hdr.blockTypes[thisBlockTypeId].str;
			if (!thisBlockTypeStr.compare("NiTriShapeData"))
				block = (NiObject*) new NiTriShapeData(file, hdr);
			else if (!thisBlockTypeStr.compare("NiTriShape"))
				block = (NiObject*) new NiTriShape(file, hdr);
			else if (!thisBlockTypeStr.compare("NiTriStrips"))
				block = (NiObject*) new NiTriStrips(file, hdr);
			else if (!thisBlockTypeStr.compare("NiTriStripsData"))
				block = (NiObject*) new NiTriStripsData(file, hdr);
			else if (!thisBlockTypeStr.compare("BSDismemberSkinInstance"))
				block = (NiObject*) new BSDismemberSkinInstance(file, hdr);
			else if (!thisBlockTypeStr.compare("NiNode"))
				block = (NiObject*) new NiNode(file, hdr);
			else if (!thisBlockTypeStr.compare("NiSkinData"))
				block = (NiObject*) new NiSkinData(file, hdr);
			else if (!thisBlockTypeStr.compare("NiSkinPartition"))
				block = (NiObject*) new NiSkinPartition(file, hdr);
			else if (!thisBlockTypeStr.compare("NiSkinInstance"))
				block = (NiObject*) new NiSkinInstance(file, hdr);
			else if (!thisBlockTypeStr.compare("BSLightingShaderProperty"))
				block = (NiObject*) new BSLightingShaderProperty(file, hdr);
			else if (!thisBlockTypeStr.compare("BSShaderTextureSet"))
				block = (NiObject*) new BSShaderTextureSet(file, hdr);
			else if (!thisBlockTypeStr.compare("NiAlphaProperty"))
				block = (NiObject*) new NiAlphaProperty(file, hdr);
			else if (!thisBlockTypeStr.compare("NiStringExtraData"))
				block = (NiObject*) new NiStringExtraData(file, hdr);
			else if (!thisBlockTypeStr.compare("BSShaderPPLightingProperty"))
				block = (NiObject*) new BSShaderPPLightingProperty(file, hdr);
			else if (!thisBlockTypeStr.compare("NiMaterialProperty"))
				block = (NiObject*) new NiMaterialProperty(file, hdr);
			else if (!thisBlockTypeStr.compare("NiStencilProperty"))
				block = (NiObject*) new NiStencilProperty(file, hdr);
			else if (!thisBlockTypeStr.compare("BSEffectShaderProperty"))
				block = (NiObject*) new BSEffectShaderProperty(file, hdr);
			else if (!thisBlockTypeStr.compare("NiFloatInterpolator"))
				block = (NiObject*) new NiFloatInterpolator(file, hdr);
			else if (!thisBlockTypeStr.compare("NiTransformInterpolator"))
				block = (NiObject*) new NiTransformInterpolator(file, hdr);
			else if (!thisBlockTypeStr.compare("NiPoint3Interpolator"))
				block = (NiObject*) new NiPoint3Interpolator(file, hdr);
			else if (!thisBlockTypeStr.compare("BSLightingShaderPropertyColorController"))
				block = (NiObject*) new BSLightingShaderPropertyColorController(file, hdr);
			else if (!thisBlockTypeStr.compare("BSLightingShaderPropertyFloatController"))
				block = (NiObject*) new BSLightingShaderPropertyFloatController(file, hdr);
			else if (!thisBlockTypeStr.compare("BSEffectShaderPropertyColorController"))
				block = (NiObject*) new BSEffectShaderPropertyColorController(file, hdr);
			else if (!thisBlockTypeStr.compare("BSEffectShaderPropertyFloatController"))
				block = (NiObject*) new BSEffectShaderPropertyFloatController(file, hdr);
			else if (!thisBlockTypeStr.compare("BSSubIndexTriShape"))
				block = (NiObject*) new BSSubIndexTriShape(file, hdr);
			else if (!thisBlockTypeStr.compare("BSMeshLODTriShape"))
				block = (NiObject*) new BSMeshLODTriShape(file, hdr);
			else if (!thisBlockTypeStr.compare("BSTriShape"))
				block = (NiObject*) new BSTriShape(file, hdr);
			else if (!thisBlockTypeStr.compare("BSSkin::Instance"))
				block = (NiObject*) new BSSkinInstance(file, hdr);
			else if (!thisBlockTypeStr.compare("BSSkin::BoneData"))
				block = (NiObject*) new BSSkinBoneData(file, hdr);
			else if (!thisBlockTypeStr.compare("BSClothExtraData"))
				block = (NiObject*) new BSClothExtraData(file, hdr);
			else if (!thisBlockTypeStr.compare("NiIntegerExtraData"))
				block = (NiObject*) new NiIntegerExtraData(file, hdr);
			else if (!thisBlockTypeStr.compare("BSXFlags"))
				block = (NiObject*) new BSXFlags(file, hdr);
			else if (!thisBlockTypeStr.compare("BSConnectPointParents"))
				block = (NiObject*) new BSConnectPointParents(file, hdr);
			else if (!thisBlockTypeStr.compare("BSConnectPointChildren"))
				block = (NiObject*) new BSConnectPointChildren(file, hdr);
			else if (!thisBlockTypeStr.compare("NiCollisionObject"))
				block = (NiObject*) new NiCollisionObject(file, hdr);
			else if (!thisBlockTypeStr.compare("bhkNiCollisionObject"))
				block = (NiObject*) new bhkNiCollisionObject(file, hdr);
			else if (!thisBlockTypeStr.compare("bhkCollisionObject"))
				block = (NiObject*) new bhkCollisionObject(file, hdr);
			else if (!thisBlockTypeStr.compare("bhkNPCollisionObject"))
				block = (NiObject*) new bhkNPCollisionObject(file, hdr);
			else if (!thisBlockTypeStr.compare("bhkPhysicsSystem"))
				block = (NiObject*) new bhkPhysicsSystem(file, hdr);
			else {
				hasUnknown = true;
				block = (NiObject*) new NiUnknown(file, hdr.blockSizes[i]);
			}

			if (block)
				blocks.push_back(block);
		}
		file.close();
	}
	else {
		Clear();
		return 1;
	}
	TrimTexturePaths();
	isValid = true;
	return 0;
}

void NifFile::SetShapeOrder(vector<string> order) {

	vector<int>delta;
	bool hadoffset = false;

	// Have to do this in multiple passes
	do {			
		vector<string> oldOrder;
		GetShapeList(oldOrder);

		vector<int> oldOrderIds;
		for (auto s : oldOrder) {
			oldOrderIds.push_back(shapeIdForName(s));
		}


		if (order.size() != oldOrder.size())
			return;
	
		// Get movement offset for each item.  This is the difference between old and new position.
		delta.clear();
		delta.resize(order.size());
		for (int p = 0; p < oldOrder.size(); p++) {
			delta[p] = (std::find(order.begin(), order.end(), oldOrder[p]) - order.begin()) - p;
		}

		hadoffset = false;
		//Positive offsets mean that the item has moved down the list.  By necessity, that means another item has moved up the list. 
		// thus, we only need to move the "rising" items, the other blocks will naturally end up in the right place.  

		// find first negative delta, and raise it in list.  The first item can't have a negative delta 
		for (int i = 1; i< delta.size(); i++) {
			// don't move positive or zero offset items.
			if (delta[i] >= 0) {
				continue;
			}
			hadoffset = true;
			int c = 0 - delta[i];
			int p = i;
			while (c > 0) {
				SwapBlocks(oldOrderIds[p], oldOrderIds[p-1]);
				p--;
				c--;
			}
			break;
		}	
	
	} while(hadoffset);

}

void NifFile::PrettySortBlocks() {
	NiNode* root = (NiNode*)blocks[0];
	vector<int> oldchildren(root->children.begin(), root->children.end());
	root->children.clear();
	root->numChildren = 0;

	for (int i = 0; i < blocks.size(); i++) {
		if (std::find(oldchildren.begin(), oldchildren.end(), i) != oldchildren.end()) {
			root->children.push_back(i);
			root->numChildren++;
		}
	}

	auto bookmark = root->children.begin();
	auto peek = root->children.begin();

	for (int i = 0; peek < root->children.end(); i++) {
		NiObject* block = GetBlock(root->children[i]);
		if (block) {
			if (block->blockType == NITRISHAPE || block->blockType == NITRISTRIPS ||
				block->blockType == BSTRISHAPE || block->blockType == BSSUBINDEXTRISHAPE || block->blockType == BSMESHLODTRISHAPE) {
				iter_swap(bookmark, peek);
				bookmark++;
			}
		}
		peek++;
	}
}

void NifFile::SwapBlocks(int blockIndexLo, int blockIndexHi) {
	if (blockIndexLo == -1 || blockIndexHi == -1)
		return;

	// First swap data
	iter_swap(hdr.blockIndex.begin() + blockIndexLo, hdr.blockIndex.begin() + blockIndexHi);
	iter_swap(hdr.blockSizes.begin() + blockIndexLo, hdr.blockSizes.begin() + blockIndexHi);
	iter_swap(blocks.begin() + blockIndexLo, blocks.begin() + blockIndexHi);

	// Next tell all the blocks that the swap happened
	for (int i = 0; i < hdr.numBlocks; i++)
		blocks[i]->notifyBlockSwap(blockIndexLo, blockIndexHi);
}

void NifFile::DeleteBlock(int blockIndex) {
	if (blockIndex == -1)
		return;

	ushort blockTypeId = hdr.blockIndex[blockIndex];
	int blockTypeRefCount = 0;
	for (int i = 0; i < hdr.blockIndex.size(); i++)
		if (hdr.blockIndex[i] == blockTypeId)
			blockTypeRefCount++;

	string blockTypeStr = hdr.blockTypes[blockTypeId].str;
	if (blockTypeRefCount < 2) {
		hdr.blockTypes.erase(hdr.blockTypes.begin() + blockTypeId);
		hdr.numBlockTypes--;
		for (int i = 0; i < hdr.blockIndex.size(); i++)
			if (hdr.blockIndex[i] > blockTypeId)
				hdr.blockIndex[i]--;
	}

	delete blocks[blockIndex];
	blocks.erase(blocks.begin() + blockIndex);
	hdr.numBlocks--;
	hdr.blockIndex.erase(hdr.blockIndex.begin() + blockIndex);
	hdr.blockSizes.erase(hdr.blockSizes.begin() + blockIndex);
	for (int i = 0; i < hdr.numBlocks; i++) {
		blocks[i]->notifyBlockDelete(blockIndex);
		hdr.blockSizes[i] = blocks[i]->CalcBlockSize();
	}
}

void NifFile::DeleteBlockByType(string typeStr) {
	ushort blockTypeId;
	for (blockTypeId = 0; blockTypeId < hdr.numBlockTypes; blockTypeId++)
		if (hdr.blockTypes[blockTypeId].str == typeStr)
			break;

	if (blockTypeId == hdr.numBlockTypes)
		return;

	vector<int> indexes;
	for (int i = 0; i < hdr.numBlocks; i++)
		if (hdr.blockIndex[i] == blockTypeId)
			indexes.push_back(i);

	for (int j = indexes.size() - 1; j >= 0; j--)
		DeleteBlock(indexes[j]);
}

int NifFile::AddBlock(NiObject* newBlock, const string& blockTypeName) {
	ushort btID = hdr.AddOrFindBlockTypeId(blockTypeName);
	hdr.blockIndex.push_back(btID);
	hdr.blockSizes.push_back(newBlock->CalcBlockSize());
	blocks.push_back(newBlock);
	hdr.numBlocks = blocks.size();
	return hdr.numBlocks - 1;
}

NiObject* NifFile::GetBlock(int blockId) {
	if (blockId >= 0 && blockId < hdr.numBlocks)
		return blocks[blockId];
	return nullptr;
}

int NifFile::AddNode(const string& nodeName, vector<Vector3>& rot, Vector3& trans, float scale) {
	NiNode* newNode = new NiNode(hdr);
	newNode->rotation[0] = rot[0];
	newNode->rotation[1] = rot[1];
	newNode->rotation[2] = rot[2];
	newNode->translation = trans;
	newNode->scale = scale;
	newNode->SetName(nodeName);

	int newNodeId = AddBlock(newNode, "NiNode");
	if (newNodeId != -1) {
		NiNode* rootNode = (NiNode*)blocks[0];
		rootNode->children.push_back(newNodeId);
		rootNode->numChildren++;
		rootNode->CalcBlockSize();
		hdr.blockSizes[0] += 4;
	}
	return newNodeId;
}

void NifFile::DeleteNode(const string& nodeName) {
	DeleteBlock(GetNodeID(nodeName));
}

string NifFile::NodeName(int blockID) {
	NiNode* n = dynamic_cast<NiNode*>(GetBlock(blockID));
	if (!n)
		return "";
	if (n->GetName().empty())
		return "_unnamed_";
	return n->GetName();
}

int NifFile::AssignExtraData(const string& shapeName, const int& extraDataId) {
	if (extraDataId != -1) {
		int id = shapeIdForName(shapeName);
		if (id == -1)
			return -1;

		NiTriBasedGeom* geom = geomForName(shapeName);
		if (geom) {
			geom->extraDataRef.push_back(extraDataId);
			geom->numExtraData++;
			hdr.blockSizes[id] = geom->CalcBlockSize();
		}
		else {
			BSTriShape* siTriShape = geomForNameF4(shapeName);
			if (!siTriShape)
				return -1;

			siTriShape->extraDataRef.push_back(extraDataId);
			siTriShape->numExtraData++;
			hdr.blockSizes[id] = siTriShape->CalcBlockSize();
		}
	}
	return extraDataId;
}

int NifFile::AddStringExtraData(const string& shapeName, const string& name, const string& stringData) {
	NiStringExtraData* strExtraData = new NiStringExtraData(hdr);
	strExtraData->SetName(name);
	strExtraData->SetStringData(stringData);

	return AssignExtraData(shapeName, AddBlock(strExtraData, "NiStringExtraData"));
}

int NifFile::AddIntegerExtraData(const string& shapeName, const string& name, const int& integerData) {
	NiIntegerExtraData* intExtraData = new NiIntegerExtraData(hdr);
	intExtraData->SetName(name);
	intExtraData->SetIntegerData(integerData);

	return AssignExtraData(shapeName, AddBlock(intExtraData, "NiIntegerExtraData"));
}

NiShader* NifFile::GetShaderF4(const string& shapeName) {
	int prop1 = -1;

	BSTriShape* geom = geomForNameF4(shapeName);
	if (geom)
		prop1 = geom->shaderPropertyRef;
	else
		return nullptr;

	if (prop1 != -1) {
		NiShader* shader = dynamic_cast<NiShader*>(GetBlock(prop1));
		if (shader) {
			ushort type = shader->blockType;
			if (type == BSLIGHTINGSHADERPROPERTY ||
				type == BSEFFECTSHADERPROPERTY ||
				type == BSSHADERPPLIGHTINGPROPERTY)
				return shader;
		}
	}

	return nullptr;
}

NiShader* NifFile::GetShader(const string& shapeName) {
	int prop1 = -1;

	NiTriBasedGeom* geom = geomForName(shapeName);
	if (geom)
		prop1 = geom->propertiesRef1;
	else
		return GetShaderF4(shapeName);

	if (prop1 != -1) {
		NiShader* shader = dynamic_cast<NiShader*>(GetBlock(prop1));
		if (shader) {
			ushort type = shader->blockType;
			if (type == BSLIGHTINGSHADERPROPERTY ||
				type == BSEFFECTSHADERPROPERTY ||
				type == BSSHADERPPLIGHTINGPROPERTY)
				return shader;
		}
	}
	else {
		vector<int> props = geom->propertiesRef;
		if (props.empty())
			return nullptr;

		for (int i = 0; i < props.size(); i++) {
			NiShader* shader = dynamic_cast<NiShader*>(GetBlock(props[i]));
			if (shader) {
				ushort type = shader->blockType;
				if (type == BSLIGHTINGSHADERPROPERTY ||
					type == BSEFFECTSHADERPROPERTY ||
					type == BSSHADERPPLIGHTINGPROPERTY)
					return shader;
			}
		}
	}

	return nullptr;
}

bool NifFile::IsShaderSkin(const string& shapeName) {
	NiShader* shader = GetShader(shapeName);
	if (shader)
		return shader->IsSkinTint();

	return false;
}

NiMaterialProperty* NifFile::GetMaterialProperty(const string& shapeName) {
	NiTriBasedGeom* geom = geomForName(shapeName);
	if (!geom)
		return nullptr;

	vector<int> props = geom->propertiesRef;
	if (props.empty())
		return nullptr;

	for (int i = 0; i < props.size(); i++) {
		NiMaterialProperty* material = dynamic_cast<NiMaterialProperty*>(GetBlock(props[i]));
		if (material)
			return material;
	}

	return nullptr;
}

int NifFile::GetTextureForShape(const string& shapeName, string& outTexFile, int texIndex) {
	int textureSetRef = -1;
	outTexFile.clear();

	NiShader* shader = GetShader(shapeName);
	if (shader)
		textureSetRef = shader->GetTextureSetRef();
	else
		return 0;

	if (textureSetRef == -1) {
		if (shader->blockType == BSEFFECTSHADERPROPERTY) {
			BSEffectShaderProperty* effectShader = (BSEffectShaderProperty*)shader;
			if (texIndex == 0)
				outTexFile = effectShader->sourceTexture.str;
			else if (texIndex == 1)
				outTexFile = effectShader->greyscaleTexture.str;

			return BSEFFECTSHADERPROPERTY;
		}
		else
			return 0;
	}

	BSShaderTextureSet* textureSet = dynamic_cast<BSShaderTextureSet*>(GetBlock(textureSetRef));
	if (!textureSet || texIndex + 1 > textureSet->numTextures)
		return 0;

	outTexFile = textureSet->textures[texIndex].str;
	return 1;
}

void NifFile::SetTextureForShape(const string& shapeName, string& outTexFile, int texIndex) {
	int textureSetRef = -1;

	NiShader* shader = GetShader(shapeName);
	if (shader)
		textureSetRef = shader->GetTextureSetRef();
	else
		return;
	
	if (textureSetRef == -1) {
		if (shader->blockType == BSEFFECTSHADERPROPERTY) {
			BSEffectShaderProperty* effectShader = (BSEffectShaderProperty*)shader;
			if (texIndex == 0)
				effectShader->sourceTexture.str = outTexFile;
			else if (texIndex == 1)
				effectShader->greyscaleTexture.str = outTexFile;
			return;
		}
		else
			return;
	}

	BSShaderTextureSet* textureSet = dynamic_cast<BSShaderTextureSet*>(GetBlock(textureSetRef));
	if (!textureSet || texIndex + 1 > textureSet->numTextures)
		return;

	textureSet->textures[texIndex].str = outTexFile;
	hdr.blockSizes[textureSetRef] = textureSet->CalcBlockSize();
}

void NifFile::TrimTexturePaths() {
	string tFile;
	vector<string> shapes;
	GetShapeList(shapes);

	for (auto &s : shapes) {
		for (int i = 0; i < 10; i++) {
			if (GetTextureForShape(s, tFile, i) && !tFile.empty()) {
				tFile = regex_replace(tFile, regex("/+|\\\\+"), "\\"); // Replace multiple slashes or forward slashes with one backslash
				tFile = regex_replace(tFile, regex("^\\\\+", regex_constants::icase), ""); // Remove all backslashes from the front
				tFile = regex_replace(tFile, regex(".*?Data\\\\", regex_constants::icase), ""); // Remove everything before and including the data path root
				tFile = regex_replace(tFile, regex("^(?!^textures\\\\)", regex_constants::icase), "textures\\"); // Add textures root path if not existing}
				SetTextureForShape(s, tFile, i);
			}
		}
	}
}

void NifFile::CopyShader(const string& shapeDest, NifFile& srcNif) {
	NiTriBasedGeom* geom = geomForName(shapeDest);
	BSTriShape* siTriShape = nullptr;
	NiShader* srcShader = nullptr;
	NiAlphaProperty* srcAlphaProp = nullptr;

	int srcDataRef = -1;
	int srcShaderRef = -1;
	int propRef1 = -1;
	int propRef2 = -1;

	if (geom) {
		srcDataRef = geom->dataRef;

		srcShader = dynamic_cast<NiShader*>(srcNif.GetBlock(geom->propertiesRef1));
		if (!srcShader) {
			// No shader found, look in other properties
			for (int i = 0; i < geom->numProperties; i++) {
				srcShader = dynamic_cast<NiShader*>(srcNif.GetBlock(geom->propertiesRef[i]));
				if (srcShader) {
					srcShaderRef = geom->propertiesRef[i];
					propRef1 = i;
					break;
				}
			}

			// Still no shader found, return
			if (!srcShader)
				return;
		}
		else
			srcShaderRef = geom->propertiesRef1;

		srcAlphaProp = dynamic_cast<NiAlphaProperty*>(srcNif.GetBlock(geom->propertiesRef2));
		if (!srcAlphaProp) {
			// No alpha found, look in other properties
			for (int i = 0; i < geom->numProperties; i++) {
				srcAlphaProp = dynamic_cast<NiAlphaProperty*>(srcNif.GetBlock(geom->propertiesRef[i]));
				if (srcAlphaProp) {
					propRef2 = i;
					break;
				}
			}
		}
	}
	else {
		siTriShape = geomForNameF4(shapeDest);
		if (!siTriShape)
			return;

		srcShaderRef = siTriShape->shaderPropertyRef;
		srcShader = dynamic_cast<NiShader*>(srcNif.GetBlock(srcShaderRef));
		if (!srcShader)
			return;

		srcAlphaProp = dynamic_cast<NiAlphaProperty*>(srcNif.GetBlock(siTriShape->alphaPropertyRef));
	}


	// Create destination shader and copy
	NiShader* destShader = nullptr;
	if (srcShader->blockType == BSLIGHTINGSHADERPROPERTY) {
		BSLightingShaderProperty* shader = (BSLightingShaderProperty*)srcShader;
		BSLightingShaderProperty* copyShader = new BSLightingShaderProperty(*shader);
		destShader = dynamic_cast<NiShader*>(copyShader);
	}
	else if (srcShader->blockType == BSSHADERPPLIGHTINGPROPERTY) {
		BSShaderPPLightingProperty* shader = (BSShaderPPLightingProperty*)srcShader;
		BSShaderPPLightingProperty* copyShader = new BSShaderPPLightingProperty(*shader);
		destShader = dynamic_cast<NiShader*>(copyShader);
	}
	else if (srcShader->blockType == BSEFFECTSHADERPROPERTY) {
		BSEffectShaderProperty* shader = (BSEffectShaderProperty*)srcShader;
		BSEffectShaderProperty* copyShader = new BSEffectShaderProperty(*shader);
		destShader = dynamic_cast<NiShader*>(copyShader);
	}

	if (!destShader)
		return;

	destShader->header = &hdr;
	if (hdr.userVersion == 12 && hdr.userVersion2 >= 120)
		destShader->SetName(srcShader->GetName());
	else
		destShader->ClearName();

	// Add shader block to nif
	int shaderId = blocks.size();
	blocks.push_back(destShader);
	hdr.numBlocks++;

	BSShaderTextureSet* destTexSet = nullptr;
	BSShaderTextureSet* srcTexSet = dynamic_cast<BSShaderTextureSet*>(srcNif.GetBlock(srcShader->GetTextureSetRef()));
	if (srcTexSet) {
		// Create texture set block and copy
		destTexSet = new BSShaderTextureSet(*srcTexSet);
		destTexSet->header = &hdr;

		// Add texture block to nif
		int texsetId = blocks.size();
		blocks.push_back(destTexSet);
		hdr.numBlocks++;

		// Assign texture set block id to shader
		destShader->SetTextureSetRef(texsetId);
	}

	// Wet Material
	string srcWetMaterialName = srcShader->GetWetMaterialName();
	if (!srcWetMaterialName.empty())
		destShader->SetWetMaterialName(srcWetMaterialName);

	// Controller
	int controllerId = -1;
	NiTimeController* destController = nullptr;
	NiTimeController* srcController = dynamic_cast<NiTimeController*>(srcNif.GetBlock(srcShader->controllerRef));
	if (srcController) {
		controllerId = blocks.size();
		if (srcController->blockType == BSLIGHTINGSHADERPROPERTYCOLORCONTROLLER) {
			BSLightingShaderPropertyColorController* controller = (BSLightingShaderPropertyColorController*)srcController;
			BSLightingShaderPropertyColorController* controllerCopy = new BSLightingShaderPropertyColorController(*controller);
			destController = dynamic_cast<NiTimeController*>(controllerCopy);
		}
		else if (srcController->blockType == BSLIGHTINGSHADERPROPERTYFLOATCONTROLLER) {
			BSLightingShaderPropertyFloatController* controller = (BSLightingShaderPropertyFloatController*)srcController;
			BSLightingShaderPropertyFloatController* controllerCopy = new BSLightingShaderPropertyFloatController(*controller);
			destController = dynamic_cast<NiTimeController*>(controllerCopy);
		}
		else if (srcController->blockType == BSEFFECTSHADERPROPERTYCOLORCONTROLLER) {
			BSEffectShaderPropertyColorController* controller = (BSEffectShaderPropertyColorController*)srcController;
			BSEffectShaderPropertyColorController* controllerCopy = new BSEffectShaderPropertyColorController(*controller);
			destController = dynamic_cast<NiTimeController*>(controllerCopy);
		}
		else if (srcController->blockType == BSEFFECTSHADERPROPERTYFLOATCONTROLLER) {
			BSEffectShaderPropertyFloatController* controller = (BSEffectShaderPropertyFloatController*)srcController;
			BSEffectShaderPropertyFloatController* controllerCopy = new BSEffectShaderPropertyFloatController(*controller);
			destController = dynamic_cast<NiTimeController*>(controllerCopy);
		}
		else
			destShader->controllerRef = -1;
	}

	if (destController) {
		destController->targetRef = shaderId;
		blocks.push_back(destController);
		hdr.numBlocks++;
		destShader->controllerRef = controllerId;
	}

	if (geom) {
		if (srcShader->blockType == BSLIGHTINGSHADERPROPERTY)
			geom->propertiesRef1 = shaderId;
		else if (srcShader->blockType == BSSHADERPPLIGHTINGPROPERTY)
			geom->propertiesRef[propRef1] = shaderId;
		else if (srcShader->blockType == BSEFFECTSHADERPROPERTY)
			geom->propertiesRef1 = shaderId;
	}
	else
		siTriShape->shaderPropertyRef = shaderId;

	// Kill normals, set numUVSets to 1
	if (srcDataRef != -1 && destShader->IsSkinTint() && hdr.userVersion >= 12) {
		NiTriBasedGeomData* geomData = dynamic_cast<NiTriBasedGeomData*>(GetBlock(srcDataRef));
		if (geomData) {
			geomData->hasNormals = 0;
			geomData->normals.clear();
			geomData->bitangents.clear();
			geomData->tangents.clear();
			geomData->numUVSets = 1;
			hdr.blockSizes[srcDataRef] = geomData->CalcBlockSize();
		}
	}

	// Add shader and other sizes to block size info
	hdr.blockSizes.push_back(destShader->CalcBlockSize());

	if (destTexSet)
		hdr.blockSizes.push_back(destTexSet->CalcBlockSize());

	if (destController)
		hdr.blockSizes.push_back(destController->CalcBlockSize());

	// Create alpha property and copy
	NiAlphaProperty* destAlphaProp = nullptr;
	if (srcAlphaProp) {
		destAlphaProp = new NiAlphaProperty(*srcAlphaProp);
		destAlphaProp->header = &hdr;

		if (hdr.userVersion == 12 && hdr.userVersion2 >= 120)
			destAlphaProp->SetName(srcAlphaProp->GetName());
		else
			destAlphaProp->ClearName();

		if (geom) {
			if (srcShader->blockType == BSLIGHTINGSHADERPROPERTY)
				geom->propertiesRef2 = blocks.size();
			else if (srcShader->blockType == BSSHADERPPLIGHTINGPROPERTY)
				geom->propertiesRef[propRef2] = blocks.size();
			else if (srcShader->blockType == BSEFFECTSHADERPROPERTY)
				geom->propertiesRef2 = blocks.size();
		}
		else
			siTriShape->alphaPropertyRef = blocks.size();

		blocks.push_back(destAlphaProp);
		hdr.numBlocks++;
		hdr.blockSizes.push_back(destAlphaProp->CalcBlockSize());
	}

	// Record the block type in the block type index.
	ushort shaderTypeId = hdr.AddOrFindBlockTypeId(srcNif.hdr.blockTypes[srcNif.hdr.blockIndex[srcShaderRef]].str);
	hdr.blockIndex.push_back(shaderTypeId);

	if (destTexSet) {
		ushort texSetTypeId = hdr.AddOrFindBlockTypeId("BSShaderTextureSet");
		hdr.blockIndex.push_back(texSetTypeId);
	}

	if (destController) {
		ushort controllerTypeId = hdr.AddOrFindBlockTypeId(srcNif.hdr.blockTypes[srcNif.hdr.blockIndex[srcShader->controllerRef]].str);
		hdr.blockIndex.push_back(controllerTypeId);
	}

	if (destAlphaProp) {
		ushort alphaBlockTypeId = hdr.AddOrFindBlockTypeId("NiAlphaProperty");
		hdr.blockIndex.push_back(alphaBlockTypeId);
	}
}

int NifFile::CopyNamedNode(string& nodeName, NifFile& srcNif) {
	NiNode* srcNode = srcNif.nodeForName(nodeName);
	if (!srcNode)
		return -1;

	NiNode* destNode = new NiNode((*srcNode));
	destNode->header = &hdr;
	destNode->SetName(nodeName);

	int blockID = blocks.size();
	blocks.push_back(destNode);
	hdr.blockSizes.push_back(destNode->CalcBlockSize());
	hdr.blockIndex.push_back(hdr.AddOrFindBlockTypeId("NiNode"));
	hdr.numBlocks++;

	return blockID;
}

void NifFile::CopyGeometry(const string& shapeDest, NifFile& srcNif, const string& srcShape) {
	NiTriBasedGeom* srcGeom = srcNif.geomForName(srcShape);
	if (srcGeom) {
		CopyGeometry(shapeDest, srcNif, srcShape, srcGeom);
	}
	else {
		BSTriShape* srcGeomF4 = srcNif.geomForNameF4(srcShape);
		if (srcGeomF4)
			CopyGeometry(shapeDest, srcNif, srcShape, srcGeomF4);
	}
}

void NifFile::CopyGeometry(const string& shapeDest, NifFile& srcNif, const string& srcShape, NiTriBasedGeom* srcGeom) {
	bool skipSkinInst = false;
	if (srcGeom->skinInstanceRef == -1)
		skipSkinInst = true;

	NiSkinInstance* srcSkinInst = nullptr;
	NiSkinInstance* destSkinInst = nullptr;
	NiSkinData* srcSkinData = nullptr;
	NiSkinData* destSkinData = nullptr;
	NiSkinPartition* srcSkinPart = nullptr;
	NiSkinPartition* destSkinPart = nullptr;
	if (!skipSkinInst) {
		srcSkinInst = dynamic_cast<NiSkinInstance*>(srcNif.GetBlock(srcGeom->skinInstanceRef));
		if (srcSkinInst) {
			srcSkinData = (NiSkinData*)srcNif.GetBlock(srcSkinInst->dataRef);
			srcSkinPart = (NiSkinPartition*)srcNif.GetBlock(srcSkinInst->skinPartitionRef);

			if (srcSkinInst->blockType == NISKININSTANCE) {
				destSkinInst = new NiSkinInstance(*srcSkinInst);
			}
			else if (srcSkinInst->blockType == BSDISMEMBERSKININSTANCE) {
				BSDismemberSkinInstance* srcBsdSkinInst = (BSDismemberSkinInstance*)srcSkinInst;
				BSDismemberSkinInstance* destBsdSkinInst = new BSDismemberSkinInstance(*srcBsdSkinInst);
				destSkinInst = dynamic_cast<NiSkinInstance*>(destBsdSkinInst);
			}

			if (destSkinInst)
				destSkinInst->header = &hdr;
			else
				skipSkinInst = true;
		}
		else
			skipSkinInst = true;
	}

	NiTriBasedGeom* destGeom = nullptr;
	if (srcGeom->blockType == NITRISHAPE) {
		NiTriShape* shape = (NiTriShape*)srcGeom;
		NiTriShape* destShape = new NiTriShape(*shape);
		destGeom = dynamic_cast<NiTriBasedGeom*>(destShape);
	}
	else if (srcGeom->blockType == NITRISTRIPS) {
		NiTriStrips* strips = (NiTriStrips*)srcGeom;
		NiTriStrips* destStrips = new NiTriStrips(*strips);
		destGeom = dynamic_cast<NiTriBasedGeom*>(destStrips);
	}

	destGeom->header = &hdr;
	destGeom->SetName(shapeDest);

	NiTriBasedGeomData* destGeomData = nullptr;
	NiTriBasedGeomData* srcGeomData = dynamic_cast<NiTriBasedGeomData*>(srcNif.GetBlock(srcGeom->dataRef));
	if (srcGeomData) {
		if (srcGeomData->blockType == NITRISHAPEDATA) {
			NiTriShapeData* shapeData = (NiTriShapeData*)srcGeomData;
			NiTriShapeData* destShapeData = new NiTriShapeData(*shapeData);
			destGeomData = dynamic_cast<NiTriBasedGeomData*>(destShapeData);
		}
		else if (srcGeomData->blockType == NITRISTRIPSDATA) {
			NiTriStripsData* stripsData = (NiTriStripsData*)srcGeomData;
			NiTriStripsData* destStripsData = new NiTriStripsData(*stripsData);
			destGeomData = dynamic_cast<NiTriBasedGeomData*>(destStripsData);
		}
	}

	destGeomData->header = &hdr;

	if (!skipSkinInst) {
		// Treat skinning and partition info as blobs of anonymous data.
		destSkinData = new NiSkinData(*srcSkinData);
		destSkinData->header = &hdr;
		destSkinPart = new NiSkinPartition(*srcSkinPart);
		destSkinPart->header = &hdr;
	}

	int destId = blocks.size();
	blocks.push_back(destGeom);
	hdr.numBlocks++;

	int destDataId = -1;
	if (srcGeomData) {
		destDataId = blocks.size();
		blocks.push_back(destGeomData);
		hdr.numBlocks++;
	}

	int destSkinId = -1;
	int destSkinDataId = -1;
	int destSkinPartId = -1;
	if (!skipSkinInst) {
		destSkinId = blocks.size();
		blocks.push_back(destSkinInst);
		hdr.numBlocks++;

		destSkinDataId = blocks.size();
		blocks.push_back(destSkinData);
		hdr.numBlocks++;

		destSkinPartId = blocks.size();
		blocks.push_back(destSkinPart);
		hdr.numBlocks++;
	}

	destGeom->dataRef = destDataId;
	destGeom->skinInstanceRef = destSkinId;

	if (!skipSkinInst) {
		destSkinInst->dataRef = destSkinDataId;
		destSkinInst->skinPartitionRef = destSkinPartId;
	}

	int srcGeomId = srcNif.shapeIdForName(srcShape);
	hdr.blockIndex.push_back(hdr.AddOrFindBlockTypeId(srcNif.hdr.blockTypes[srcNif.hdr.blockIndex[srcGeomId]].str));

	if (srcGeomData)
		hdr.blockIndex.push_back(hdr.AddOrFindBlockTypeId(srcNif.hdr.blockTypes[srcNif.hdr.blockIndex[srcGeom->dataRef]].str));

	if (!skipSkinInst) {
		hdr.blockIndex.push_back(hdr.AddOrFindBlockTypeId(srcNif.hdr.blockTypes[srcNif.hdr.blockIndex[srcGeom->skinInstanceRef]].str));
		hdr.blockIndex.push_back(hdr.AddOrFindBlockTypeId(srcNif.hdr.blockTypes[srcNif.hdr.blockIndex[srcSkinInst->dataRef]].str));
		hdr.blockIndex.push_back(hdr.AddOrFindBlockTypeId(srcNif.hdr.blockTypes[srcNif.hdr.blockIndex[srcSkinInst->skinPartitionRef]].str));
	}

	hdr.blockSizes.push_back(destGeom->CalcBlockSize());
	if (srcGeomData)
		hdr.blockSizes.push_back(destGeomData->CalcBlockSize());

	if (!skipSkinInst) {
		hdr.blockSizes.push_back(destSkinInst->CalcBlockSize());
		hdr.blockSizes.push_back(destSkinData->CalcBlockSize());
		hdr.blockSizes.push_back(destSkinPart->CalcBlockSize());
	}

	CopyShader(shapeDest, srcNif);

	for (int i = 0; i < srcGeom->numProperties; i++) {
		NiMaterialProperty* material = dynamic_cast<NiMaterialProperty*>(srcNif.GetBlock(srcGeom->propertiesRef[i]));
		if (material) {
			NiMaterialProperty* destMaterial = new NiMaterialProperty(*material);
			destMaterial->header = &hdr;
			int materialId = blocks.size();
			blocks.push_back(destMaterial);
			hdr.numBlocks++;
			hdr.blockSizes.push_back(destMaterial->CalcBlockSize());
			hdr.blockIndex.push_back(hdr.AddOrFindBlockTypeId(srcNif.hdr.blockTypes[srcNif.hdr.blockIndex[srcGeom->propertiesRef[i]]].str));
			destGeom->propertiesRef[i] = materialId;
			continue;
		}

		NiStencilProperty* stencil = dynamic_cast<NiStencilProperty*>(srcNif.GetBlock(srcGeom->propertiesRef[i]));
		if (stencil) {
			NiStencilProperty* destStencil = new NiStencilProperty(*stencil);
			destStencil->header = &hdr;
			int stencilId = blocks.size();
			blocks.push_back(destStencil);
			hdr.numBlocks++;
			hdr.blockSizes.push_back(destStencil->CalcBlockSize());
			hdr.blockIndex.push_back(hdr.AddOrFindBlockTypeId(srcNif.hdr.blockTypes[srcNif.hdr.blockIndex[srcGeom->propertiesRef[i]]].str));
			destGeom->propertiesRef[i] = stencilId;
			continue;
		}

		NiUnknown* srcUnknown = dynamic_cast<NiUnknown*>(srcNif.GetBlock(srcGeom->propertiesRef[i]));
		if (srcUnknown) {
			NiUnknown* destUnknown = new NiUnknown(srcUnknown->CalcBlockSize());
			destUnknown->Clone(srcUnknown);
			int unknownId = blocks.size();
			blocks.push_back(destUnknown);
			hdr.numBlocks++;
			hdr.blockSizes.push_back(destUnknown->CalcBlockSize());
			hdr.blockIndex.push_back(hdr.AddOrFindBlockTypeId(srcNif.hdr.blockTypes[srcNif.hdr.blockIndex[srcGeom->propertiesRef[i]]].str));
			destGeom->propertiesRef[i] = unknownId;
		}
	}

	vector<string> srcBoneList;
	if (!skipSkinInst) {
		srcNif.GetShapeBoneList(srcShape, srcBoneList);
		destSkinInst->bones.clear();
	}

	int bonePtr;
	NiNode* rootNode = (NiNode*)blocks[0];
	for (auto &boneName : srcBoneList) {
		bonePtr = GetNodeID(boneName);
		if (bonePtr == -1) {
			bonePtr = CopyNamedNode(boneName, srcNif);
			rootNode->children.push_back(bonePtr);
			rootNode->numChildren++;
			hdr.blockSizes[0] = rootNode->CalcBlockSize();
		}
		destSkinInst->bones.push_back(bonePtr);
	}

	rootNode->children.push_back(destId);
	rootNode->numChildren++;
	hdr.blockSizes[0] = rootNode->CalcBlockSize();
}

void NifFile::CopyGeometry(const string& shapeDest, NifFile& srcNif, const string& srcShape, BSTriShape* srcGeom) {
	BSTriShape* destGeom = nullptr;
	NiNode* rootNode = (NiNode*)blocks[0];
	int newBlockSize;
	if (srcGeom->blockType == BSSUBINDEXTRISHAPE) {
		BSSubIndexTriShape* shape = (BSSubIndexTriShape*)srcGeom;
		BSSubIndexTriShape* destShape = new BSSubIndexTriShape(*shape);
		newBlockSize = destShape->CalcBlockSize();
		destGeom = dynamic_cast<BSSubIndexTriShape*>(destShape);
	}
	else  {
		BSTriShape* destShape = (BSTriShape*)srcGeom;
		newBlockSize = destShape->CalcBlockSize();
		destGeom = new BSTriShape(*destShape);
	}

	destGeom->header = &hdr;
	destGeom->SetName(shapeDest);

	blocks.push_back(destGeom);
	rootNode->children.push_back(hdr.numBlocks);
	rootNode->numChildren++;
	hdr.numBlocks++;
	hdr.blockSizes.push_back(newBlockSize);
	hdr.blockIndex.push_back(hdr.AddOrFindBlockTypeId(srcNif.hdr.blockTypes[srcNif.hdr.blockIndex[srcNif.shapeIdForName(srcShape)]].str));

	bool skipSkinInst = true;
	BSSkinInstance* newSkinInst = nullptr;
	if (srcGeom->skinInstanceRef != -1) {
		skipSkinInst = false;
		newSkinInst = new BSSkinInstance(*(BSSkinInstance*)srcNif.GetBlock(srcGeom->skinInstanceRef));
		hdr.blockSizes.push_back(newSkinInst->CalcBlockSize());
		hdr.blockIndex.push_back(hdr.AddOrFindBlockTypeId(srcNif.hdr.blockTypes[srcNif.hdr.blockIndex[srcGeom->skinInstanceRef]].str));
		blocks.push_back(newSkinInst);
		destGeom->skinInstanceRef = hdr.numBlocks;
		hdr.numBlocks++;
		if (newSkinInst->boneDataRef != -1) {
			BSSkinBoneData* newBoneData = new BSSkinBoneData(*(BSSkinBoneData*)srcNif.GetBlock(newSkinInst->boneDataRef));
			hdr.blockSizes.push_back(newBoneData->CalcBlockSize());
			hdr.blockIndex.push_back(hdr.AddOrFindBlockTypeId(srcNif.hdr.blockTypes[srcNif.hdr.blockIndex[newSkinInst->boneDataRef]].str));
			blocks.push_back(newBoneData);
			newSkinInst->boneDataRef = hdr.numBlocks;
			hdr.numBlocks++;
		}
	}

	CopyShader(shapeDest, srcNif);

	vector<string> srcBoneList;
	if (!skipSkinInst) {
		srcNif.GetShapeBoneList(srcShape, srcBoneList);
		newSkinInst->bones.clear();
	}

	int bonePtr;
	for (auto &boneName : srcBoneList) {
		bonePtr = GetNodeID(boneName);
		if (bonePtr == -1) {
			bonePtr = CopyNamedNode(boneName, srcNif);
			rootNode->children.push_back(bonePtr);
			rootNode->numChildren++;
		}
		newSkinInst->bones.push_back(bonePtr);
	}

	hdr.blockSizes[0] = rootNode->CalcBlockSize();
}

int NifFile::Save(const string& filename) {
	fstream file(filename.c_str(), ios_base::out | ios_base::binary);
	if (file.is_open()) {
		for (int i = 0; i < hdr.numBlocks; i++)
			hdr.blockSizes[i] = blocks[i]->CalcBlockSize();

		hdr.Put(file);

		for (int i = 0; i < hdr.numBlocks; i++)
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


int NifFile::GetShapeType(const string& shapeName) {
	int type;
	shapeDataIdForName(shapeName, type);
	return type;
}

int NifFile::GetShapeList(vector<string>& outList) {
	outList.clear();
	for (auto& block : blocks) {
		ushort type = block->blockType;
		if (type == NITRISHAPE || type == NITRISTRIPS || type == BSSUBINDEXTRISHAPE || type == BSTRISHAPE || type == BSMESHLODTRISHAPE) {
			NiAVObject* avo = dynamic_cast<NiAVObject*>(block);
			if (avo)
				outList.push_back(avo->GetName());
		}
	}
	return outList.size();
}

void NifFile::RenameShape(const string& oldName, const string& newName) {
	NiAVObject* geom = avObjectForName(oldName);
	if (geom)
		geom->SetName(newName, true);
}

void NifFile::RenameDuplicateShape(const string& dupedShape) {
	int dupCount = 1;
	char buf[10];

	NiAVObject* geom = avObjectForName(dupedShape);
	if (geom) {
		while ((geom = avObjectForName(dupedShape, 1)) != nullptr) {
			_snprintf(buf, 10, "_%d", dupCount);
			while (hdr.FindStringId(geom->GetName() + buf) != -1) {
				dupCount++;
				_snprintf(buf, 10, "_%d", dupCount);
			}

			geom->SetName(geom->GetName() + buf);
			dupCount++;
		}
	}
}

void NifFile::SetNodeName(int blockID, const string& newName) {
	NiNode* node = dynamic_cast<NiNode*>(GetBlock(blockID));
	if (!node)
		return;

	node->SetName(newName, true);
}

int NifFile::GetNodeID(const string& nodeName) {
	int id = -1;
	for (auto& block : blocks) {
		id++;
		if (block->blockType == NINODE) {
			NiNode* node = (NiNode*)block;
			if (!node->GetName().compare(nodeName))
				return id;
		}
	}
	return -1;
}

int NifFile::GetRootNodeID() {
	if (blocks.empty())
		return -1;

	return 0;
}

bool NifFile::GetNodeTransform(const string& nodeName, vector<Vector3>& outRot, Vector3& outTrans, float& outScale) {
	for (auto& block : blocks) {
		if (block->blockType == NINODE) {
			NiNode* node = (NiNode*)block;
			if (!node->GetName().compare(nodeName)) {
				outRot.clear();
				outRot.push_back(node->rotation[0]);
				outRot.push_back(node->rotation[1]);
				outRot.push_back(node->rotation[2]);
				outTrans = node->translation;
				outScale = node->scale;
				return true;
			}
		}
	}
	return false;
}

bool NifFile::SetNodeTransform(const string& nodeName, SkinTransform& inXform, const bool& rootChildrenOnly) {
	if (rootChildrenOnly) {
		NiNode* root = dynamic_cast<NiNode*>(blocks[0]);
		if (root) {
			for (auto& child : root->children) {
				NiNode* node = dynamic_cast<NiNode*>(GetBlock(child));
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
			if (block->blockType == NINODE) {
				NiNode* node = (NiNode*)block;
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

	return false;
}

int NifFile::GetShapeBoneList(const string& shapeName, vector<string>& outList) {
	int skinRef = -1;
	outList.clear();

	NiTriBasedGeom* geom = geomForName(shapeName);
	if (geom) {
		skinRef = geom->skinInstanceRef;
	}
	else {
		BSTriShape* siTriShape = geomForNameF4(shapeName);
		if (siTriShape)
			skinRef = siTriShape->skinInstanceRef;
	}

	if (skinRef == -1)
		return 0;

	NiBoneContainer* skinInst = dynamic_cast<NiBoneContainer*>(GetBlock(skinRef));
	if (!skinInst)
		return 0;

	int boneBlock;
	for (int i = 0; i < skinInst->bones.size(); i++) {
		boneBlock = skinInst->bones[i];
		if (boneBlock != -1) {
			NiNode* node = (NiNode*)GetBlock(boneBlock);
			outList.push_back(node->GetName());
		}
	}
	return outList.size();
}

int NifFile::GetShapeBoneIDList(const string& shapeName, vector<int>& outList) {
	int skinRef = -1;
	outList.clear();

	NiTriBasedGeom* geom = geomForName(shapeName);
	if (geom) {
		skinRef = geom->skinInstanceRef;
	}
	else {
		BSTriShape* siTriShape = geomForNameF4(shapeName);
		if (siTriShape) {
			skinRef = siTriShape->skinInstanceRef;
		}
	}


	if (skinRef == -1)
		return 0;

	NiBoneContainer* skinInst = dynamic_cast<NiBoneContainer*>(GetBlock(skinRef));
	if (!skinInst)
		return 0;

	int boneBlock;
	for (int i = 0; i < skinInst->bones.size(); i++) {
		boneBlock = skinInst->bones[i];
		outList.push_back(boneBlock);
	}
	return outList.size();
}

void NifFile::SetShapeBoneIDList(const string& shapeName, vector<int>& inList) {
	int skinRef = -1;
	BSSkinBoneData* boneData = nullptr;
	NiTriBasedGeom* geom = geomForName(shapeName);
	if (geom) {
		skinRef = geom->skinInstanceRef;
	}
	else {
		BSTriShape* siTriShape = geomForNameF4(shapeName);
		if (siTriShape) {
			skinRef = siTriShape->skinInstanceRef;
			BSSkinInstance * skinForBoneRef = dynamic_cast<BSSkinInstance*>(GetBlock(skinRef));
			if (skinForBoneRef)
				boneData = dynamic_cast<BSSkinBoneData*>(GetBlock(skinForBoneRef->boneDataRef));
		}
	}

	if (skinRef == -1)
		return;

	NiBoneContainer* boneCont = dynamic_cast<NiBoneContainer*>(GetBlock(skinRef));
	if (!boneCont)
		return;

	boneCont->bones.clear();
	boneCont->numBones = 0;
	bool feedBoneData = false;
	if (boneData && boneData->nBones != inList.size()) {						// bone data array,  clear it out if it doesn't match size
		boneData->nBones = 0;
		boneData->boneXforms.clear();
		feedBoneData = true;
	}

	for (int i = 0; i < inList.size(); i++) {
		boneCont->bones.push_back(inList[i]);
		boneCont->numBones++;
		if (boneData && feedBoneData) {
			boneData->boneXforms.emplace_back();
			boneData->nBones++;
			boneData->CalcBlockSize();
		}
	}
	hdr.blockSizes[skinRef] = boneCont->CalcBlockSize();

	NiSkinInstance* skinInst = dynamic_cast<NiSkinInstance*>(boneCont);
	if (skinInst) {
		NiSkinData* skinData = (NiSkinData*)GetBlock(skinInst->dataRef);
		int nBonesToAdd = skinInst->bones.size() - skinData->numBones;
		if (nBonesToAdd > 0) {
			for (int i = 0; i < nBonesToAdd; i++) {
				skinData->bones.emplace_back();
				skinData->bones.back().numVertices = 0;
				skinData->numBones++;
			}
			hdr.blockSizes[skinInst->dataRef] = skinData->CalcBlockSize();
		}
	}
}

int NifFile::GetShapeBoneWeights(const string& shapeName, int boneIndex, unordered_map<ushort, float>& outWeights) {
	int skinRef = -1;
	outWeights.clear();

	NiTriBasedGeom* geom = geomForName(shapeName);
	if (geom) {
		skinRef = geom->skinInstanceRef;
	}
	else {
		BSTriShape* siTriShape = geomForNameF4(shapeName);
		if (siTriShape) {
			for (int vid = 0; vid < siTriShape->numVertices; vid++) {
				for (int i = 0; i < 4; i++) {
					if (siTriShape->vertData[vid].weightBones[i] == boneIndex && siTriShape->vertData[vid].weights[i] != 0) {
						outWeights[vid] = siTriShape->vertData[vid].weights[i];
					}
				}
			}
			return outWeights.size();
		}
	}

	if (skinRef == -1)
		return 0;

	NiSkinInstance* skinInst = dynamic_cast<NiSkinInstance*>(GetBlock(skinRef));
	if (!skinInst)
		return 0;

	if (skinInst->dataRef == -1)
		return 0;

	NiSkinData* skinData = (NiSkinData*)GetBlock(skinInst->dataRef);
	if (boneIndex > skinData->numBones)
		return 0;

	NiSkinData::BoneData* bone = &skinData->bones[boneIndex];
	for (auto &sw : bone->vertexWeights) {
		outWeights[sw.index] = sw.weight;
		if (sw.weight < 0.0001f)
			outWeights[sw.index] = 0.0f;
	}
	return outWeights.size();
}

bool NifFile::GetShapeBoneTransform(const string& shapeName, const string& boneName, SkinTransform& outXform, Vector3& outSphereOffset, float& outSphereRadius) {
	int boneIndex = shapeBoneIndex(shapeName, boneName);
	if (boneName.empty())
		boneIndex = -1;

	return GetShapeBoneTransform(shapeName, boneIndex, outXform, outSphereOffset, outSphereRadius);
}

bool NifFile::SetShapeBoneTransform(const string& shapeName, int boneIndex, SkinTransform& inXform, Vector3& inSphereOffset, float inSphereRadius) {
	int skinRef = -1;

	NiTriBasedGeom* geom = geomForName(shapeName);
	if (geom) {
		skinRef = geom->skinInstanceRef;
	}
	else {
		BSTriShape* siTriShape = geomForNameF4(shapeName);
		if (siTriShape) {
			skinRef = siTriShape->skinInstanceRef;
			if (skinRef == -1)
				return false;

			BSSkinInstance * skinForBoneRef = dynamic_cast<BSSkinInstance*>(GetBlock(skinRef));
			if (skinForBoneRef && boneIndex != -1) {
				BSSkinBoneData* bsSkin = dynamic_cast<BSSkinBoneData*>(GetBlock(skinForBoneRef->boneDataRef));
				bsSkin->boneXforms[boneIndex].boundSphereOffset = inSphereOffset;
				bsSkin->boneXforms[boneIndex].boundSphereRadius = inSphereRadius;
				bsSkin->boneXforms[boneIndex].boneTransform = inXform;
			}
		}
	}

	if (skinRef == -1)
		return false;

	NiSkinInstance* skinInst = dynamic_cast<NiSkinInstance*>(GetBlock(skinRef));
	if (!skinInst)
		return false;

	if (skinInst->dataRef == -1)
		return false;

	NiSkinData* skinData = (NiSkinData*)GetBlock(skinInst->dataRef);
	if (boneIndex == -1) {
		// Set the overall skin transform
		skinData->skinTransform = inXform;
		return true;
	}
	if (boneIndex > skinData->numBones)
		return false;

	NiSkinData::BoneData* bone = &skinData->bones[boneIndex];
	bone->boneTransform = inXform;
	bone->boundSphereOffset = inSphereOffset;
	bone->boundSphereRadius = inSphereRadius;
	return true;
}

bool NifFile::GetShapeBoneTransform(const string& shapeName, int boneIndex, SkinTransform& outXform, Vector3& outSphereOffset, float& outSphereRadius) {
	int skinRef = -1;

	NiTriBasedGeom* geom = geomForName(shapeName);
	if (geom) {
		skinRef = geom->skinInstanceRef;
	}
	else {
		BSTriShape* siTriShape = geomForNameF4(shapeName);
		if (siTriShape) {
			skinRef = siTriShape->skinInstanceRef;
			if (skinRef != -1) {
				BSSkinInstance* skinForBoneRef = dynamic_cast<BSSkinInstance*>(GetBlock(skinRef));
				if (skinForBoneRef) {
					int boneDataRef = skinForBoneRef->boneDataRef;
					if (boneDataRef != -1) {
						if (boneIndex == -1) {
							// overall skin transform not found in fo4 meshes :(
							return false;
						}

						BSSkinBoneData* boneData = dynamic_cast<BSSkinBoneData*>(GetBlock(boneDataRef));
						outXform = boneData->boneXforms[boneIndex].boneTransform;
						outSphereOffset = boneData->boneXforms[boneIndex].boundSphereOffset;
						outSphereRadius = boneData->boneXforms[boneIndex].boundSphereRadius;
						return true;
					}
				}
			}
		}
	}

	if (skinRef == -1)
		return false;

	NiSkinInstance* skinInst = dynamic_cast<NiSkinInstance*>(GetBlock(skinRef));
	if (!skinInst)
		return false;

	if (skinInst->dataRef == -1)
		return false;

	NiSkinData* skinData = (NiSkinData*)GetBlock(skinInst->dataRef);
	if (boneIndex == -1) {
		// Want the overall skin transform
		outXform = skinData->skinTransform;
		outSphereOffset = Vector3(0.0f, 0.0f, 0.0f);
		outSphereRadius = 0.0f;
		return true;
	}

	if (boneIndex > skinData->numBones)
		return 0;

	NiSkinData::BoneData* bone = &skinData->bones[boneIndex];
	outXform = bone->boneTransform;
	outSphereOffset = bone->boundSphereOffset;
	outSphereRadius = bone->boundSphereRadius;
	return true;
}

void NifFile::UpdateShapeBoneID(const string& shapeName, int oldID, int newID) {
	int skinRef = -1;

	NiTriBasedGeom* geom = geomForName(shapeName);
	if (geom)
		skinRef = geom->skinInstanceRef;
	else {
		BSTriShape* siTriShape = geomForNameF4(shapeName);
		if (siTriShape) {
			skinRef = siTriShape->skinInstanceRef;
			if (skinRef != -1) {
				for (auto& b : dynamic_cast<BSSkinInstance*>(GetBlock(skinRef))->bones) {
					if (b == oldID)
						b = newID;
				}
			}
		}
	}

	if (skinRef == -1)
		return;

	NiSkinInstance* skinInst = dynamic_cast<NiSkinInstance*>(GetBlock(skinRef));
	if (!skinInst)
		return;

	for (auto &bp : skinInst->bones) {
		if (bp == oldID) {
			bp = newID;
			return;
		}
	}
}

void NifFile::SetShapeBoneWeights(const string& shapeName, int boneIndex, unordered_map<ushort, float>& inWeights) {
	int skinRef = -1;

	NiTriBasedGeom* geom = geomForName(shapeName);
	if (geom)
		skinRef = geom->skinInstanceRef;
	else {
		// BSTriShape* siTriShape = geomForNameF4(shapeName);
		// NOT Implemented.  use SetShapeVertWeights instead.
	}

	if (skinRef == -1)
		return;

	NiSkinInstance* skinInst = dynamic_cast<NiSkinInstance*>(GetBlock(skinRef));
	if (!skinInst)
		return;

	if (skinInst->dataRef == -1)
		return;

	NiSkinData* skinData = (NiSkinData*)GetBlock(skinInst->dataRef);
	if (boneIndex > skinData->numBones)
		return;

	NiSkinData::BoneData* bone = &skinData->bones[boneIndex];
	bone->vertexWeights.clear();
	for (auto &sw : inWeights)
		if (sw.second >= 0.0001f)
			bone->vertexWeights.emplace_back(SkinWeight(sw.first, sw.second));

	bone->numVertices = (ushort)bone->vertexWeights.size();
	hdr.blockSizes[skinInst->dataRef] = skinData->CalcBlockSize();
}

void NifFile::SetShapeVertWeights(const string& shapeName, int vertIndex, vector<unsigned char>& boneids, vector<float>& weights) {
	BSTriShape* siTriShape = geomForNameF4(shapeName);
	if (!siTriShape) {
		return;
	}

	memset(siTriShape->vertData[vertIndex].weights, 0, sizeof(float) * 4);
	memset(siTriShape->vertData[vertIndex].weightBones, 0, sizeof(unsigned char) * 4);

	// sum weights to normalize weight values
	float sum = 0.0f;
	for (int i = 0; i < weights.size(); i++) {
		sum += weights[i];
	}

	int num = (weights.size() < 4 ? weights.size() : 4);

	for (int i = 0; i < num; i++) {
		siTriShape->vertData[vertIndex].weightBones[i] = boneids[i];
		siTriShape->vertData[vertIndex].weights[i] = weights[i] / sum;
	}
}

const vector<Vector3>* NifFile::GetRawVertsForShape(const string& shapeName) {
	int bType;
	int dataRef = shapeDataIdForName(shapeName, bType);
	if (dataRef == -1)
		return nullptr;
	if (bType == NITRISHAPEDATA || bType == NITRISTRIPSDATA) {
		NiTriBasedGeomData* geomData = static_cast<NiTriBasedGeomData*>(GetBlock(dataRef));
		return &geomData->vertices;
	}
	else if (bType == BSSUBINDEXTRISHAPE || bType == BSTRISHAPE || bType == BSMESHLODTRISHAPE) {
		BSTriShape* siTriShape = static_cast<BSTriShape*>(GetBlock(dataRef));
		return siTriShape->GetRawVerts();
	}
	return nullptr;
}

bool NifFile::GetTrisForShape(const string& shapeName, vector<Triangle>* outTris) {
	int bType;
	int dataID = shapeDataIdForName(shapeName, bType);
	if (dataID == -1)
		return false;

	if (bType == NITRISHAPEDATA) {
		NiTriShapeData* shapeData = static_cast<NiTriShapeData*>(GetBlock(dataID));
		*outTris = shapeData->triangles;
		return true;
	}
	else if (bType == NITRISTRIPSDATA) {
		NiTriStripsData* stripsData = static_cast<NiTriStripsData*>(GetBlock(dataID));
		stripsData->StripsToTris(outTris);
		return true;
	}
	else if (bType == BSSUBINDEXTRISHAPE || bType == BSTRISHAPE || bType == BSMESHLODTRISHAPE) {
		BSTriShape* shapeData = static_cast<BSTriShape*>(GetBlock(dataID));
		*outTris = shapeData->triangles;
		return true;
	}

	return false;
}

const vector<Vector3>* NifFile::GetNormalsForShape(const string& shapeName, bool transform) {
	int bType;
	int dataID = shapeDataIdForName(shapeName, bType);
	if (dataID == -1)
		return nullptr;

	if (bType == NITRISHAPEDATA) {
		NiTriShapeData* shapeData = (NiTriShapeData*)GetBlock(dataID);
		if (shapeData->hasNormals)
			return &shapeData->normals;
	}
	else if (bType == NITRISTRIPSDATA) {
		NiTriStripsData* stripsData = (NiTriStripsData*)GetBlock(dataID);
		if (stripsData->hasNormals)
			return &stripsData->normals;
	}
	else if (bType == BSSUBINDEXTRISHAPE || bType == BSTRISHAPE || bType == BSMESHLODTRISHAPE) {
		BSTriShape* shapeData = (BSTriShape*)(GetBlock(dataID));
		return shapeData->GetNormalData(transform);
	}

	return nullptr;
}

const vector<Vector3>* NifFile::GetTangentsForShape(const string& shapeName, bool transform) {
	int bType;
	int dataID = shapeDataIdForName(shapeName, bType);
	if (dataID == -1)
		return nullptr;

	if (bType == NITRISHAPEDATA) {
		return nullptr;
	}
	else if (bType == NITRISTRIPSDATA) {
		return nullptr;
	}
	else if (bType == BSSUBINDEXTRISHAPE || bType == BSTRISHAPE || bType == BSMESHLODTRISHAPE) {
		BSTriShape* shapeData = (BSTriShape*)(GetBlock(dataID));
		return shapeData->GetTangentData(transform);
	}

	return nullptr;
}

const vector<Vector3>* NifFile::GetBitangentsForShape(const string& shapeName, bool transform) {
	int bType;
	int dataID = shapeDataIdForName(shapeName, bType);
	if (dataID == -1)
		return nullptr;

	if (bType == NITRISHAPEDATA) {
		return nullptr;
	}
	else if (bType == NITRISTRIPSDATA) {
		return nullptr;
	}
	else if (bType == BSSUBINDEXTRISHAPE || bType == BSTRISHAPE || bType == BSMESHLODTRISHAPE) {
		BSTriShape* shapeData = (BSTriShape*)(GetBlock(dataID));
		return shapeData->GetBitangentData(transform);
	}

	return nullptr;
}

const vector<Vector2>* NifFile::GetUvsForShape(const string& shapeName) {
	int bType;
	int dataID = shapeDataIdForName(shapeName, bType);
	if (dataID == -1)
		return nullptr;

	if (bType == NITRISHAPEDATA) {
		NiTriShapeData* shapeData = (NiTriShapeData*)GetBlock(dataID);
		if (shapeData->numUVSets > 0)
			return &shapeData->uvSets;
	}
	else if (bType == NITRISTRIPSDATA) {
		NiTriStripsData* stripsData = (NiTriStripsData*)GetBlock(dataID);
		if (stripsData->numUVSets > 0)
			return &stripsData->uvSets;
	}
	else if (bType == BSSUBINDEXTRISHAPE || bType == BSTRISHAPE || bType == BSMESHLODTRISHAPE) {
		BSTriShape* shapeData = (BSTriShape*)(GetBlock(dataID));
		return shapeData->GetUVData();
	}

	return nullptr;
}

bool NifFile::GetUvsForShape(const string& shapeName, vector<Vector2>& outUvs) {
	int bType;
	int dataID = shapeDataIdForName(shapeName, bType);
	if (dataID == -1)
		return false;

	if (bType == NITRISHAPEDATA) {
		NiTriShapeData* shapeData = (NiTriShapeData*)GetBlock(dataID);
		outUvs.assign(shapeData->uvSets.begin(), shapeData->uvSets.end());
		return true;
	}
	else if (bType == NITRISTRIPSDATA) {
		NiTriStripsData* stripsData = (NiTriStripsData*)GetBlock(dataID);
		outUvs.assign(stripsData->uvSets.begin(), stripsData->uvSets.end());
		return true;
	}
	else if (bType == BSSUBINDEXTRISHAPE || bType == BSTRISHAPE || bType == BSMESHLODTRISHAPE) {
		const vector<Vector2>* uvdata = GetUvsForShape(shapeName);
		outUvs.assign(uvdata->begin(), uvdata->end());
		return true;
	}

	return false;
}

const vector<vector<int>>* NifFile::GetSeamVertsForShape(const string& shapeName) {
	int bType;
	int dataID = shapeDataIdForName(shapeName, bType);
	if (dataID == -1)
		return nullptr;

	if (bType == NITRISHAPEDATA) {
		//NiTriShapeData* shapeData = (NiTriShapeData*)GetBlock(dataID);
		// dosomthin
	}
	else if (bType == NITRISTRIPSDATA) {
		//NiTriStripsData* stripsData = (NiTriStripsData*)GetBlock(dataID);
		// dosomthin
	}
	else if (bType == BSSUBINDEXTRISHAPE || bType == BSTRISHAPE || bType == BSMESHLODTRISHAPE) {

	}
	return nullptr;
}

bool NifFile::GetVertsForShape(const string& shapeName, vector<Vector3>& outVerts) {
	outVerts.clear();

	int bType;
	int dataRef = shapeDataIdForName(shapeName, bType);
	if (dataRef == -1)
		return false;

	if (bType == NITRISHAPEDATA) {
		NiTriShapeData* shapeData = static_cast<NiTriShapeData*>(GetBlock(dataRef));
		for (auto &v : shapeData->vertices)
			outVerts.push_back(v);
		return true;
	}
	else if (bType == NITRISTRIPSDATA) {
		NiTriStripsData* stripsData = static_cast<NiTriStripsData*>(GetBlock(dataRef));
		for (auto &v : stripsData->vertices)
			outVerts.push_back(v);
		return true;
	}
	else if (bType == BSSUBINDEXTRISHAPE || bType == BSTRISHAPE || bType == BSMESHLODTRISHAPE) {
		BSTriShape* siTriShape = static_cast<BSTriShape*>(GetBlock(dataRef));
		for (auto &v : siTriShape->vertData)
			outVerts.push_back(v.vert);
		return true;
	}
	return false;
}

int NifFile::GetVertCountForShape(const string& shapeName) {
	int bType;
	int dataID = shapeDataIdForName(shapeName, bType);
	if (dataID == -1)
		return -1;

	if (bType == NITRISHAPEDATA) {
		NiTriShapeData* shapeData = static_cast<NiTriShapeData*>(GetBlock(dataID));
		if (shapeData)
			return shapeData->numVertices;
	}
	else if (bType == NITRISTRIPSDATA) {
		NiTriStripsData* stripsData = static_cast<NiTriStripsData*>(GetBlock(dataID));
		if (stripsData)
			return stripsData->numVertices;
	}
	else if (bType == BSSUBINDEXTRISHAPE || bType == BSTRISHAPE || bType == BSMESHLODTRISHAPE) {
		BSTriShape* siTriShape = static_cast<BSTriShape*>(GetBlock(dataID));
		return siTriShape->numVertices;
	}
	return -1;
}

void NifFile::SetVertsForShape(const string& shapeName, const vector<Vector3>& verts) {
	int bType;
	int dataID = shapeDataIdForName(shapeName, bType);
	if (dataID == -1)
		return;

	if (bType == NITRISHAPEDATA) {
		NiTriShapeData* shapeData = static_cast<NiTriShapeData*>(GetBlock(dataID));
		if (verts.size() != shapeData->vertices.size())
			return;

		for (int i = 0; i < shapeData->vertices.size(); i++)
			shapeData->vertices[i] = verts[i];
	}
	else if (bType == NITRISTRIPSDATA) {
		NiTriStripsData* stripsData = static_cast<NiTriStripsData*>(GetBlock(dataID));
		if (verts.size() != stripsData->vertices.size())
			return;

		for (int i = 0; i < stripsData->vertices.size(); i++)
			stripsData->vertices[i] = verts[i];
	}
	else if (bType == BSSUBINDEXTRISHAPE || bType == BSTRISHAPE || bType == BSMESHLODTRISHAPE) {
		BSTriShape* siTriShape = static_cast<BSTriShape*>(GetBlock(dataID));
		if (verts.size() != siTriShape->numVertices)
			return;

		for (int i = 0; i < siTriShape->numVertices; i++)
			siTriShape->vertData[i].vert = verts[i];
	}
}

void NifFile::SetUvsForShape(const string& shapeName, const vector<Vector2>& uvs) {
	int bType;
	int dataID = shapeDataIdForName(shapeName, bType);
	if (dataID == -1)
		return;

	if (bType == NITRISHAPEDATA) {
		NiTriShapeData* shapeData = static_cast<NiTriShapeData*>(GetBlock(dataID));
		if (uvs.size() != shapeData->vertices.size())
			return;

		shapeData->uvSets.assign(uvs.begin(), uvs.end());
	}
	else if (bType == NITRISTRIPSDATA) {
		NiTriStripsData* stripsData = static_cast<NiTriStripsData*>(GetBlock(dataID));
		if (uvs.size() != stripsData->vertices.size())
			return;

		stripsData->uvSets.assign(uvs.begin(), uvs.end());
	}
	else if (bType == BSSUBINDEXTRISHAPE || bType == BSTRISHAPE || bType == BSMESHLODTRISHAPE) {
		BSTriShape* siTriShape = static_cast<BSTriShape*>(GetBlock(dataID));
		if (uvs.size() != siTriShape->vertData.size())
			return;

		for (int i = 0; i < siTriShape->numVertices; i++)
			siTriShape->vertData[i].uv = uvs[i];
	}
}

void NifFile::InvertUVsForShape(const string& shapeName, bool invertX, bool invertY) {
	int bType;
	int dataID = shapeDataIdForName(shapeName, bType);
	if (dataID == -1)
		return;

	if (bType == NITRISHAPEDATA) {
		NiTriShapeData* shapeData = static_cast<NiTriShapeData*>(GetBlock(dataID));
		if (invertX)
			for (int i = 0; i < shapeData->uvSets.size(); ++i)
				shapeData->uvSets[i].u = 1.0f - shapeData->uvSets[i].u;

		if (invertY)
			for (int i = 0; i < shapeData->uvSets.size(); ++i)
				shapeData->uvSets[i].v = 1.0f - shapeData->uvSets[i].v;
	}
	else if (bType == NITRISTRIPSDATA) {
		NiTriStripsData* stripsData = static_cast<NiTriStripsData*>(GetBlock(dataID));
		if (invertX)
			for (int i = 0; i < stripsData->uvSets.size(); ++i)
				stripsData->uvSets[i].u = 1.0f - stripsData->uvSets[i].u;

		if (invertY)
			for (int i = 0; i < stripsData->uvSets.size(); ++i)
				stripsData->uvSets[i].v = 1.0f - stripsData->uvSets[i].v;
	}
	else if (bType == BSSUBINDEXTRISHAPE || bType == BSTRISHAPE || bType == BSMESHLODTRISHAPE) {
		BSTriShape* siTriShape = static_cast<BSTriShape*>(GetBlock(dataID));
		if (invertX)
			for (int i = 0; i < siTriShape->vertData.size(); ++i)
				siTriShape->vertData[i].uv.u = 1.0f - siTriShape->vertData[i].uv.u;

		if (invertY)
			for (int i = 0; i < siTriShape->vertData.size(); ++i)
				siTriShape->vertData[i].uv.v = 1.0f - siTriShape->vertData[i].uv.v;
	}
}

void NifFile::SetNormalsForShape(const string& shapeName, const vector<Vector3>& norms) {
	int bType;
	int dataID = shapeDataIdForName(shapeName, bType);
	if (dataID == -1)
		return;

	if (bType == NITRISHAPEDATA) {
		NiTriShapeData* shapeData = (NiTriShapeData*)GetBlock(dataID);
		if (norms.size() != shapeData->normals.size())
			shapeData->normals.resize(norms.size());
		for (int i = 0; i < shapeData->vertices.size(); i++)
			shapeData->normals[i] = norms[i];
		shapeData->hasNormals = true;
		shapeData->numUVSets |= 1;
		hdr.blockSizes[dataID] = shapeData->CalcBlockSize();
	}
	else if (bType == NITRISTRIPSDATA) {
		NiTriStripsData* stripsData = (NiTriStripsData*)GetBlock(dataID);
		if (norms.size() != stripsData->normals.size())
			stripsData->normals.resize(norms.size());
		for (int i = 0; i < stripsData->vertices.size(); i++)
			stripsData->normals[i] = norms[i];
		stripsData->hasNormals = true;
		stripsData->numUVSets |= 1;
		hdr.blockSizes[dataID] = stripsData->CalcBlockSize();
	}
	else if (bType == BSSUBINDEXTRISHAPE || bType == BSTRISHAPE || bType == BSMESHLODTRISHAPE) {
		BSTriShape* shape = (BSTriShape*)GetBlock(dataID);
		shape->SetNormals(norms);
		hdr.blockSizes[dataID] = shape->CalcBlockSize();
	}
}

void NifFile::SmoothNormalsForShape(const string& shapeName) {
	int bType;
	int dataID = shapeDataIdForName(shapeName, bType);
	if (dataID == -1)
		return;

	if (bType == NITRISHAPEDATA) {
		//NiTriShapeData* shapeData = (NiTriShapeData*)GetBlock(dataID);
		//shapeData->SmoothNormals();
	}
	else if (bType == NITRISTRIPSDATA) {
		//NiTriStripsData* stripsData = (NiTriStripsData*)GetBlock(dataID);
		//stripsData->SmoothNormals();
	}
	else if (bType == BSSUBINDEXTRISHAPE || bType == BSTRISHAPE || bType == BSMESHLODTRISHAPE) {
		BSTriShape* shape = (BSTriShape*)GetBlock(dataID);
		shape->RecalcNormals(true);
	}
}

void NifFile::CalcNormalsForShape(const string& shapeName) {
	int bType;
	int dataID = shapeDataIdForName(shapeName, bType);
	if (dataID == -1)
		return;

	if (bType == NITRISHAPEDATA) {
		NiTriShapeData* shapeData = (NiTriShapeData*)GetBlock(dataID);
		shapeData->RecalcNormals();
		hdr.blockSizes[dataID] = shapeData->CalcBlockSize();
	}
	else if (bType == NITRISTRIPSDATA) {
		NiTriStripsData* stripsData = (NiTriStripsData*)GetBlock(dataID);
		stripsData->RecalcNormals();
		hdr.blockSizes[dataID] = stripsData->CalcBlockSize();
	}
	else if (bType == BSSUBINDEXTRISHAPE || bType == BSTRISHAPE || bType == BSMESHLODTRISHAPE) {
		BSTriShape* shape = (BSTriShape*)GetBlock(dataID);
		shape->RecalcNormals();
		hdr.blockSizes[dataID] = shape->CalcBlockSize();
	}
}

void NifFile::CalcTangentsForShape(const string& shapeName) {
	int bType;
	int dataID = shapeDataIdForName(shapeName, bType);
	if (dataID == -1)
		return;

	if (bType == NITRISHAPEDATA) {
		NiTriShapeData* shapeData = (NiTriShapeData*)GetBlock(dataID);
		shapeData->CalcTangentSpace();
		hdr.blockSizes[dataID] = shapeData->CalcBlockSize();
	}
	else if (bType == NITRISTRIPSDATA) {
		NiTriStripsData* stripsData = (NiTriStripsData*)GetBlock(dataID);
		stripsData->CalcTangentSpace();
		hdr.blockSizes[dataID] = stripsData->CalcBlockSize();
	}
	else if (bType == BSSUBINDEXTRISHAPE || bType == BSTRISHAPE || bType == BSMESHLODTRISHAPE) {
		BSTriShape* shape = (BSTriShape*)GetBlock(dataID);
		shape->CalcTangentSpace();
		hdr.blockSizes[dataID] = shape->CalcBlockSize();
	}
}

void NifFile::ClearShapeTransform(const string& shapeName) {
	NiAVObject* avo = avObjectForName(shapeName);
	if (avo) {
		avo->translation.Zero();
		avo->scale = 1.0f;
		avo->rotation[0] = Vector3(1.0f, 0.0f, 0.0f);
		avo->rotation[1] = Vector3(0.0f, 1.0f, 0.0f);
		avo->rotation[2] = Vector3(0.0f, 0.0f, 1.0f);
	}
}

void NifFile::GetShapeTransform(const string& shapeName, Matrix4& outTransform) {
	SkinTransform xFormRoot;
	NiNode* root = dynamic_cast<NiNode*>(blocks[0]);
	if (root) {
		xFormRoot.translation = root->translation;
		xFormRoot.scale = root->scale;
		xFormRoot.rotation[0] = root->rotation[0];
		xFormRoot.rotation[1] = root->rotation[1];
		xFormRoot.rotation[2] = root->rotation[2];
	}

	SkinTransform xFormShape;
	NiAVObject* avo = avObjectForName(shapeName);
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
	NiNode* root = dynamic_cast<NiNode*>(blocks[0]);
	if (root) {
		root->translation.Zero();
		root->scale = 1.0f;
		root->rotation[0] = Vector3(1.0f, 0.0f, 0.0f);
		root->rotation[1] = Vector3(0.0f, 1.0f, 0.0f);
		root->rotation[2] = Vector3(0.0f, 0.0f, 1.0f);
	}
}

void NifFile::GetRootTranslation(Vector3& outVec) {
	NiNode* root = dynamic_cast<NiNode*>(blocks[0]);
	if (root)
		outVec = root->translation;
	else
		outVec.Zero();
}

void NifFile::SetRootTranslation(const Vector3& newTrans) {
	NiNode* root = dynamic_cast<NiNode*>(blocks[0]);
	if (root)
		root->translation = newTrans;
}

void NifFile::GetRootScale(float& outScale) {
	NiNode* root = dynamic_cast<NiNode*>(blocks[0]);
	if (root)
		outScale = root->scale;
	else
		outScale = 1.0f;
}

void NifFile::SetRootScale(const float& newScale) {
	NiNode* root = dynamic_cast<NiNode*>(blocks[0]);
	if (root)
		root->scale = newScale;
}

void NifFile::GetShapeTranslation(const string& shapeName, Vector3& outVec) {
	NiAVObject* avo = avObjectForName(shapeName);
	if (avo)
		outVec = avo->translation;

	NiNode* root = dynamic_cast<NiNode*>(blocks[0]);
	if (root)
		outVec += root->translation;

	//if (outVec.DistanceTo(Vector3(0.0f, 0.0f, 0.0f)) < EPSILON)
	//	outVec.Zero();
}

void NifFile::SetShapeTranslation(const string& shapeName, const Vector3& newTrans) {
	NiAVObject* avo = avObjectForName(shapeName);
	if (avo)
		avo->translation = newTrans;
}

void NifFile::GetShapeScale(const string& shapeName, float& outScale) {
	NiAVObject* avo = avObjectForName(shapeName);
	if (avo)
		outScale = avo->scale;
}

void NifFile::SetShapeScale(const string& shapeName, const float& newScale) {
	NiAVObject* avo = avObjectForName(shapeName);
	if (avo)
		avo->scale = newScale;
}

void NifFile::ApplyShapeTranslation(const string& shapeName, const Vector3& offset) {
	int bType;
	int dataRef = shapeDataIdForName(shapeName, bType);
	if (dataRef == -1)
		return;

	if (bType == NITRISHAPEDATA || bType == NITRISTRIPSDATA) {
		NiTriBasedGeom* geom = geomForName(shapeName);
		if (!geom)
			return;

		NiTriBasedGeomData* geomData = dynamic_cast<NiTriBasedGeomData*>(GetBlock(dataRef));
		if (!geomData)
			return;

		for (int i = 0; i < geomData->vertices.size(); i++)
			geomData->vertices[i] += geom->translation + offset;

		geom->translation = Vector3(0.0f, 0.0f, 0.0f);
	}
	else if (bType == BSSUBINDEXTRISHAPE || bType == BSTRISHAPE || bType == BSMESHLODTRISHAPE) {
		BSTriShape* geom = dynamic_cast<BSTriShape*>(GetBlock(dataRef));
		if (!geom)
			return;
		
		for (int i = 0; i < geom->numVertices; i++)
			geom->vertData[i].vert += geom->translation + offset;

		geom->translation = Vector3(0.0f, 0.0f, 0.0f);
	}
}

void NifFile::MoveVertex(const string& shapeName, const Vector3& pos, const int& id) {
	int bType;
	int dataRef = shapeDataIdForName(shapeName, bType);
	if (dataRef == -1)
		return;

	if (bType == NITRISHAPEDATA || bType == NITRISTRIPSDATA) {
		NiTriBasedGeomData* geomData = dynamic_cast<NiTriBasedGeomData*>(GetBlock(dataRef));
		if (geomData && geomData->numVertices > id)
			geomData->vertices[id] = pos;
	}
	else if (bType == BSSUBINDEXTRISHAPE || bType == BSTRISHAPE || bType == BSMESHLODTRISHAPE) {
		BSTriShape* geom = dynamic_cast<BSTriShape*>(GetBlock(dataRef));
		if (geom && geom->numVertices > id)
			geom->vertData[id].vert = pos;
	}
}

void NifFile::OffsetShape(const string& shapeName, const Vector3& offset, unordered_map<ushort, float>* mask) {
	int bType;
	int dataRef = shapeDataIdForName(shapeName, bType);
	if (dataRef == -1)
		return;

	if (bType == NITRISHAPEDATA || bType == NITRISTRIPSDATA) {
		NiTriBasedGeomData* geomData = dynamic_cast<NiTriBasedGeomData*>(GetBlock(dataRef));
		if (!geomData)
			return;

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
	else if (bType == BSSUBINDEXTRISHAPE || bType == BSTRISHAPE || bType == BSMESHLODTRISHAPE) {
		BSTriShape* geomData = dynamic_cast<BSTriShape*>(GetBlock(dataRef));
		if (!geomData)
			return;

		for (int i = 0; i < geomData->numVertices; i++) {
			if (mask) {
				float maskFactor = 1.0f;
				Vector3 diff = offset;
				if (mask->find(i) != mask->end()) {
					maskFactor = 1.0f - (*mask)[i];
					diff *= maskFactor;
				}
				geomData->vertData[i].vert += diff;
			}
			else
				geomData->vertData[i].vert += offset;
		}
	}
}

void NifFile::ScaleShape(const string& shapeName, const Vector3& scale, unordered_map<ushort, float>* mask) {
	int bType;
	int dataRef = shapeDataIdForName(shapeName, bType);
	if (dataRef == -1)
		return;

	Vector3 root;
	GetRootTranslation(root);

	if (bType == NITRISHAPEDATA || bType == NITRISTRIPSDATA) {
		NiTriBasedGeomData* geomData = dynamic_cast<NiTriBasedGeomData*>(GetBlock(dataRef));
		if (!geomData)
			return;

		unordered_map<ushort, Vector3> diff;
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
	else if (bType == BSSUBINDEXTRISHAPE || bType == BSTRISHAPE || bType == BSMESHLODTRISHAPE) {
		BSTriShape* geomData = dynamic_cast<BSTriShape*>(GetBlock(dataRef));
		if (!geomData)
			return;

		unordered_map<ushort, Vector3> diff;
		for (int i = 0; i < geomData->numVertices; i++) {
			Vector3 target = geomData->vertData[i].vert - root;
			target.x *= scale.x;
			target.y *= scale.y;
			target.z *= scale.z;
			diff[i] = geomData->vertData[i].vert - target;

			if (mask) {
				float maskFactor = 1.0f;
				if (mask->find(i) != mask->end()) {
					maskFactor = 1.0f - (*mask)[i];
					diff[i] *= maskFactor;
					target = geomData->vertData[i].vert - root + diff[i];
				}
			}
			geomData->vertData[i].vert = target;
		}
	}
}

void NifFile::RotateShape(const string& shapeName, const Vector3& angle, unordered_map<ushort, float>* mask) {
	int bType;
	int dataRef = shapeDataIdForName(shapeName, bType);
	if (dataRef == -1)
		return;

	Vector3 root;
	GetRootTranslation(root);
	if (bType == NITRISHAPEDATA || bType == NITRISTRIPSDATA) {
		NiTriBasedGeomData* geomData = dynamic_cast<NiTriBasedGeomData*>(GetBlock(dataRef));
		if (!geomData)
			return;

		unordered_map<ushort, Vector3> diff;
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
	else if (bType == BSSUBINDEXTRISHAPE || bType == BSTRISHAPE || bType == BSMESHLODTRISHAPE) {
		BSTriShape* geomData = dynamic_cast<BSTriShape*>(GetBlock(dataRef));
		if (!geomData)
			return;

		unordered_map<ushort, Vector3> diff;
		for (int i = 0; i < geomData->numVertices; i++) {
			Vector3 target = geomData->vertData[i].vert - root;
			Matrix4 mat;
			mat.Rotate(angle.x * DEG2RAD, Vector3(1.0f, 0.0f, 0.0f));
			mat.Rotate(angle.y * DEG2RAD, Vector3(0.0f, 1.0f, 0.0f));
			mat.Rotate(angle.z * DEG2RAD, Vector3(0.0f, 0.0f, 1.0f));
			target = mat * target;
			diff[i] = geomData->vertData[i].vert - target;

			if (mask) {
				float maskFactor = 1.0f;
				if (mask->find(i) != mask->end()) {
					maskFactor = 1.0f - (*mask)[i];
					diff[i] *= maskFactor;
					target = geomData->vertData[i].vert - root + diff[i];
				}
			}
			geomData->vertData[i].vert = target;
		}
	}
}

bool NifFile::GetAlphaForShape(const string& shapeName, ushort& outFlags, byte& outThreshold) {
	int alphaRef = -1;

	NiTriBasedGeom* geom = geomForName(shapeName);
	if (geom) {
		alphaRef = geom->propertiesRef2;
		if (alphaRef == -1) {
			for (int i = 0; i < geom->numProperties; i++) {
				NiAlphaProperty* alphaProp = dynamic_cast<NiAlphaProperty*>(GetBlock(geom->propertiesRef[i]));
				if (alphaProp) {
					alphaRef = geom->propertiesRef[i];
					break;
				}
			}
		}
	}
	else {
		BSTriShape* siTriShape = geomForNameF4(shapeName);
		if (siTriShape)
			alphaRef = siTriShape->alphaPropertyRef;
	}

	if (alphaRef == -1)
		return false;

	NiAlphaProperty* alpha = dynamic_cast<NiAlphaProperty*>(GetBlock(alphaRef));
	if (!alpha)
		return false;

	outFlags = alpha->flags;
	outThreshold = alpha->threshold;
	return true;
}

void NifFile::SetAlphaForShape(const string& shapeName, ushort flags, ushort threshold) {
	int alphaRef = -1;

	NiTriBasedGeom* geom = geomForName(shapeName);
	if (geom) {
		alphaRef = geom->propertiesRef2;
		if (alphaRef == -1) {
			for (int i = 0; i < geom->numProperties; i++) {
				NiAlphaProperty* alphaProp = dynamic_cast<NiAlphaProperty*>(GetBlock(geom->propertiesRef[i]));
				if (alphaProp) {
					alphaRef = geom->propertiesRef[i];
					break;
				}
			}
		}

		if (alphaRef == -1) {
			NiShader* shader = GetShader(shapeName);
			if (!shader)
				return;

			alphaRef = blocks.size();

			NiAlphaProperty* alphaProp = new NiAlphaProperty(hdr);
			if (shader->blockType == BSLIGHTINGSHADERPROPERTY)
				geom->propertiesRef2 = alphaRef;
			else if (shader->blockType == BSSHADERPPLIGHTINGPROPERTY)
				geom->propertiesRef.push_back(alphaRef);
			else if (shader->blockType == BSEFFECTSHADERPROPERTY)
				geom->propertiesRef2 = alphaRef;
			else
				return;

			blocks.push_back(alphaProp);
			hdr.numBlocks++;
			hdr.blockSizes.push_back(alphaProp->CalcBlockSize());

			ushort alphaBlockTypeId = hdr.AddOrFindBlockTypeId("NiAlphaProperty");
			hdr.blockIndex.push_back(alphaBlockTypeId);
		}
	}
	else {
		BSTriShape* siTriShape = geomForNameF4(shapeName);
		if (siTriShape) {
			alphaRef = siTriShape->alphaPropertyRef;
			if (alphaRef == -1) {
				NiShader* shader = GetShader(shapeName);
				if (!shader)
					return;

				alphaRef = blocks.size();

				NiAlphaProperty* alphaProp = new NiAlphaProperty(hdr);
				siTriShape->alphaPropertyRef = alphaRef;

				blocks.push_back(alphaProp);
				hdr.numBlocks++;
				hdr.blockSizes.push_back(alphaProp->CalcBlockSize());

				ushort alphaBlockTypeId = hdr.AddOrFindBlockTypeId("NiAlphaProperty");
				hdr.blockIndex.push_back(alphaBlockTypeId);
			}
		}
	}

	if (alphaRef == -1)
		return;

	NiAlphaProperty* alpha = dynamic_cast<NiAlphaProperty*>(GetBlock(alphaRef));
	if (!alpha)
		return;

	alpha->flags = flags;
	alpha->threshold = (byte)threshold;
}

bool NifFile::IsShapeSkinned(const string& shapeName) {
	NiTriBasedGeom* geom = geomForName(shapeName);
	if (geom) {
		return geom->IsSkinned();
	}
	else {
		BSTriShape* siTriShape = geomForNameF4(shapeName);
		if (siTriShape)
			return siTriShape->IsSkinned();
	}

	return false;
}

void NifFile::DeleteShape(const string& shapeName) {
	NiTriBasedGeom* geom = geomForName(shapeName);
	if (geom) {
		DeleteBlock(geom->dataRef);
		DeleteShader(shapeName);
		DeleteSkinning(shapeName);

		for (int i = 0; i < geom->numProperties; i++) {
			DeleteBlock(geom->propertiesRef[i]);
			i--;
		}

		for (int i = 0; i < geom->numExtraData; i++)
			DeleteBlock(geom->extraDataRef[i]);

		int shapeID = shapeIdForName(shapeName);
		DeleteBlock(shapeID);
	}
	else {
		BSTriShape* siTriShape = geomForNameF4(shapeName);
		if (siTriShape) {
			DeleteShader(shapeName);
			DeleteSkinning(shapeName);

			int shapeID = shapeIdForName(shapeName);
			DeleteBlock(shapeID);
		}
	}
}

void NifFile::DeleteShader(const string& shapeName) {
	NiTriBasedGeom* geom = geomForName(shapeName);
	if (geom) {
		if (geom->propertiesRef1 != -1) {
			NiShader* shader = dynamic_cast<NiShader*>(GetBlock(geom->propertiesRef1));
			if (shader) {
				DeleteBlock(shader->GetTextureSetRef());
				DeleteBlock(shader->controllerRef);
				DeleteBlock(geom->propertiesRef1);
				geom->propertiesRef1 = -1;
			}
		}

		DeleteAlpha(shapeName);

		for (int i = 0; i < geom->numProperties; i++) {
			NiShader* shader = dynamic_cast<NiShader*>(GetBlock(geom->propertiesRef[i]));
			if (shader) {
				if (shader->blockType == BSSHADERPPLIGHTINGPROPERTY || shader->blockType == NIMATERIALPROPERTY) {
					DeleteBlock(shader->GetTextureSetRef());
					DeleteBlock(shader->controllerRef);
					DeleteBlock(geom->propertiesRef[i]);
					shader->SetTextureSetRef(-1);
					geom->propertiesRef[i] = -1;
					i--;
					continue;
				}
			}
		}
	}
	else {
		BSTriShape* siTriShape = geomForNameF4(shapeName);
		if (siTriShape) {
			if (siTriShape->shaderPropertyRef != -1) {
				NiShader* shader = dynamic_cast<NiShader*>(GetBlock(siTriShape->shaderPropertyRef));
				if (shader) {
					DeleteBlock(shader->GetTextureSetRef());
					DeleteBlock(shader->controllerRef);
					DeleteBlock(siTriShape->shaderPropertyRef);
					siTriShape->shaderPropertyRef = -1;
				}

				DeleteAlpha(shapeName);
			}
		}
	}
}

void NifFile::DeleteAlpha(const string& shapeName) {
	NiTriBasedGeom* geom = geomForName(shapeName);
	if (geom) {
		if (geom->propertiesRef2 != -1) {
			NiAlphaProperty* alpha = dynamic_cast<NiAlphaProperty*>(GetBlock(geom->propertiesRef2));
			if (alpha) {
				DeleteBlock(geom->propertiesRef2);
				geom->propertiesRef2 = -1;
			}
		}

		for (int i = 0; i < geom->numProperties; i++) {
			NiAlphaProperty* alpha = dynamic_cast<NiAlphaProperty*>(GetBlock(geom->propertiesRef[i]));
			if (alpha) {
				DeleteBlock(geom->propertiesRef[i]);
				geom->propertiesRef[i] = -1;
				i--;
				continue;
			}
		}
	}
	else {
		BSTriShape* siTriShape = geomForNameF4(shapeName);
		if (siTriShape) {
			if (siTriShape->alphaPropertyRef != -1) {
				NiAlphaProperty* alpha = dynamic_cast<NiAlphaProperty*>(GetBlock(siTriShape->alphaPropertyRef));
				if (alpha) {
					DeleteBlock(siTriShape->alphaPropertyRef);
					siTriShape->alphaPropertyRef = -1;
				}
			}
		}
	}
}

void NifFile::DeleteSkinning(const string& shapeName) {
	NiTriBasedGeom* geom = geomForName(shapeName);
	if (geom) {
		if (geom->skinInstanceRef != -1) {
			NiSkinInstance* skinInst = dynamic_cast<NiSkinInstance*>(GetBlock(geom->skinInstanceRef));
			if (skinInst) {
				DeleteBlock(skinInst->dataRef);
				DeleteBlock(skinInst->skinPartitionRef);
				DeleteBlock(geom->skinInstanceRef);
				geom->skinInstanceRef = -1;
			}
		}
	}
	else {
		BSTriShape* siTriShape = geomForNameF4(shapeName);
		if (siTriShape) {
			if (siTriShape->skinInstanceRef != -1) {
				BSSkinInstance* bsSkinInst = dynamic_cast<BSSkinInstance*>(GetBlock(siTriShape->skinInstanceRef));
				if (bsSkinInst) {
					DeleteBlock(bsSkinInst->boneDataRef);
					DeleteBlock(siTriShape->skinInstanceRef);
					siTriShape->skinInstanceRef = -1;
				}
			}

			siTriShape->SetSkinned(false);
		}
	}

	NiShader* shader = GetShader(shapeName);
	if (shader)
		shader->SetSkinned(false);
}

void NifFile::DeleteVertsForShape(const string& shapeName, const vector<ushort>& indices) {
	if (indices.size() == 0)
		return;

	int dataRef = -1;
	int skinRef = -1;

	NiTriBasedGeom* geom = geomForName(shapeName);
	if (geom) {
		dataRef = geom->dataRef;
		skinRef = geom->skinInstanceRef;

		if (dataRef != -1) {
			NiTriBasedGeomData* geomData = dynamic_cast<NiTriBasedGeomData*>(GetBlock(dataRef));
			if (geomData) {
				geomData->notifyVerticesDelete(indices);
				if (geomData->numVertices == 0 || geomData->numTriangles == 0) {
					// Deleted all verts or tris, remove shape and children
					DeleteShape(shapeName);
					return;
				}
				hdr.blockSizes[dataRef] = geomData->CalcBlockSize();
			}
		}
	}
	else {
		BSTriShape* siTriShape = geomForNameF4(shapeName);
		if (siTriShape) {
			siTriShape->notifyVerticesDelete(indices);
			if (siTriShape->numVertices == 0 || siTriShape->numTriangles == 0) {
				// Deleted all verts or tris, remove shape and children
				DeleteShape(shapeName);
				return;
			}

			int shapeID = shapeIdForName(shapeName);
			if (shapeID != -1)
				hdr.blockSizes[shapeID] = siTriShape->CalcBlockSize();
		}
	}

	if (skinRef == -1)
		return;

	NiSkinInstance* skinInst = dynamic_cast<NiSkinInstance*>(GetBlock(skinRef));
	if (skinInst) {
		NiSkinData* skinData = (NiSkinData*)GetBlock(skinInst->dataRef);
		skinData->notifyVerticesDelete(indices);
		hdr.blockSizes[skinInst->dataRef] = skinData->CalcBlockSize();

		NiSkinPartition* skinPartition = (NiSkinPartition*)GetBlock(skinInst->skinPartitionRef);
		skinPartition->notifyVerticesDelete(indices);
		vector<int> emptyIndices;
		if (skinPartition->RemoveEmptyPartitions(emptyIndices)) {
			if (skinInst->blockType == BSDISMEMBERSKININSTANCE) {
				BSDismemberSkinInstance* bsdSkinInst = dynamic_cast<BSDismemberSkinInstance*>(skinInst);
				bsdSkinInst->numPartitions -= emptyIndices.size();
				for (int i = emptyIndices.size() - 1; i >= 0; i--)
					bsdSkinInst->partitions.erase(bsdSkinInst->partitions.begin() + i);
			}
		}
		hdr.blockSizes[skinInst->skinPartitionRef] = skinPartition->CalcBlockSize();
		hdr.blockSizes[skinRef] = skinInst->CalcBlockSize();
	}
}

int NifFile::CalcShapeDiff(const string& shapeName, const vector<Vector3>* targetData, unordered_map<ushort, Vector3>& outDiffData, float scale) {
	outDiffData.clear();
	const vector<Vector3>* myData = GetRawVertsForShape(shapeName);
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

int NifFile::ShapeDiff(const string& baseShapeName, const string& targetShape, unordered_map<ushort, Vector3>& outDiffData) {
	const vector<Vector3>* targetData = GetRawVertsForShape(targetShape);
	if (!targetData)
		return 2;

	return CalcShapeDiff(baseShapeName, targetData, outDiffData);
}

void NifFile::UpdateSkinPartitions(const string& shapeName) {
	int skinRef = -1;
	NiTriBasedGeom* geom = geomForName(shapeName);
	if (geom)
		skinRef = geom->skinInstanceRef;

	if (skinRef == -1)
		return;

	NiSkinData* skinData = nullptr;
	NiSkinPartition* skinPart = nullptr;
	NiSkinInstance* skinInst = dynamic_cast<NiSkinInstance*>(GetBlock(skinRef));
	if (skinInst) {
		skinData = (NiSkinData*)GetBlock(skinInst->dataRef);
		skinPart = (NiSkinPartition*)GetBlock(skinInst->skinPartitionRef);
	}
	else
		return;

	if (skinPart->needsBuild) {
		BuildSkinPartitions(shapeName);
		return;
	}

	//Make maps of new bone weights
	map<ushort, vector<int>> vertBones;
	map<ushort, vector<float>> vertWeights;
	int b = 0;
	for (auto &bone : skinData->bones) {
		for (auto &bw : bone.vertexWeights) {
			int i = 0;
			for (auto &w : vertWeights[bw.index]) {
				if (bw.weight > w)
					break;
				i++;
			}
			vertBones[bw.index].insert(vertBones[bw.index].begin() + i, b);
			vertWeights[bw.index].insert(vertWeights[bw.index].begin() + i, bw.weight);
		}
		b++;
	}

	//Erase influences over 4
	for (auto &bones : vertBones) {
		for (int bi = 0; bi < bones.second.size(); bi++) {
			if (bi == 4) {
				vertBones[bones.first].erase(vertBones[bones.first].begin() + 4, vertBones[bones.first].end());
				vertWeights[bones.first].erase(vertWeights[bones.first].begin() + 4, vertWeights[bones.first].end());
				break;
			}
		}
	}

	for (auto& part : skinPart->partitions) {
		//Make list of new bones
		set<int> bones;
		for (auto &v : part.vertexMap) {
			for (int bi = 0; bi < vertBones[v].size(); bi++) {
				int bid = vertBones[v][bi];
				bones.insert(bid);
			}
		}

		//Clear old data
		part.bones.clear();
		part.boneIndices.clear();
		part.vertexWeights.clear();

		//Add new bones to partition
		unordered_map<int, int> boneLookup;
		for (auto &b : bones) {
			part.bones.push_back(b);
			boneLookup[b] = part.bones.size() - 1;
		}

		for (auto &v : part.vertexMap) {
			BoneIndices b;
			VertexWeight vw;
			b.i1 = b.i2 = b.i3 = b.i4 = 0;
			vw.w1 = vw.w2 = vw.w3 = vw.w4 = 0;

			//Normalize weights
			byte* pb = (byte*)&b.i1;
			float* pw = (float*)&vw.w1;
			float tot = 0.0f;
			for (int bi = 0; bi < vertBones[v].size(); bi++) {
				pb[bi] = boneLookup[vertBones[v][bi]];
				pw[bi] = vertWeights[v][bi];
				tot += pw[bi];
			}
			for (int bi = 0; bi < 4; bi++)
				pw[bi] /= tot;

			//Add new bone indices and weights to partition
			part.boneIndices.push_back(b);
			part.vertexWeights.push_back(vw);
		}
		part.numBones = part.bones.size();
	}

	if (skinInst->blockType == BSDISMEMBERSKININSTANCE) {
		BSDismemberSkinInstance* bsdSkinInst = dynamic_cast<BSDismemberSkinInstance*>(skinInst);
		unordered_map<int, int> partIDCount;
		for (int i = 0; i < bsdSkinInst->numPartitions; i++) {
			auto pidc = partIDCount.find(bsdSkinInst->partitions[i].partID);
			if (pidc == partIDCount.end())
				partIDCount[bsdSkinInst->partitions[i].partID] = 1;
			else
				partIDCount[bsdSkinInst->partitions[i].partID]++;
		}
	}
	hdr.blockSizes[skinInst->skinPartitionRef] = skinPart->CalcBlockSize();
}

void NifFile::BuildSkinPartitions(const string& shapeName, int maxBonesPerPartition) {
	int bType;
	int dataRef = shapeDataIdForName(shapeName, bType);
	if (dataRef == -1)
		return;

	int skinRef = -1;
	NiTriBasedGeom* geom = nullptr;
	NiTriShapeData* shapeData = nullptr;
	NiTriStripsData* stripsData = nullptr;
	BSTriShape* bsGeom = nullptr;

	if (bType == NITRISHAPEDATA) {
		geom = geomForName(shapeName);
		if (!geom)
			return;

		skinRef = geom->skinInstanceRef;
		shapeData = (NiTriShapeData*)GetBlock(geom->dataRef);
		if (skinRef == -1) {
			NiSkinData* nifSkinData = new NiSkinData(hdr);
			int skinDataID = AddBlock((NiObject*)nifSkinData, "NiSkinData");

			NiSkinPartition* nifSkinPartition = new NiSkinPartition(hdr);
			int partID = AddBlock((NiObject*)nifSkinPartition, "NiSkinPartition");

			BSDismemberSkinInstance* nifDismemberInst = new BSDismemberSkinInstance(hdr);
			int dismemberID = AddBlock((NiObject*)nifDismemberInst, "BSDismemberSkinInstance");

			nifDismemberInst->dataRef = skinDataID;
			nifDismemberInst->skinPartitionRef = partID;
			nifDismemberInst->skeletonRootRef = 0;
			geom->skinInstanceRef = dismemberID;
			skinRef = dismemberID;

			NiShader* shader = GetShader(shapeName);
			if (shader)
				shader->SetSkinned(true);
		}
	}
	else if (bType == NITRISTRIPSDATA) {
		geom = geomForName(shapeName);
		if (!geom)
			return;

		// TO-DO
		return;

		skinRef = geom->skinInstanceRef;
		stripsData = (NiTriStripsData*)GetBlock(geom->dataRef);
		if (skinRef == -1) {
			NiSkinData* nifSkinData = new NiSkinData(hdr);
			int skinDataID = AddBlock((NiObject*)nifSkinData, "NiSkinData");

			NiSkinPartition* nifSkinPartition = new NiSkinPartition(hdr);
			int partID = AddBlock((NiObject*)nifSkinPartition, "NiSkinPartition");

			BSDismemberSkinInstance* nifDismemberInst = new BSDismemberSkinInstance(hdr);
			int skinID = AddBlock((NiObject*)nifDismemberInst, "BSDismemberSkinInstance");

			nifDismemberInst->dataRef = skinDataID;
			nifDismemberInst->skinPartitionRef = partID;
			nifDismemberInst->skeletonRootRef = 0;
			geom->skinInstanceRef = skinID;
			skinRef = skinID;
		}
	}
	else if (bType == BSSUBINDEXTRISHAPE || bType == BSTRISHAPE || bType == BSMESHLODTRISHAPE) {
		bsGeom = geomForNameF4(shapeName);
		if (!bsGeom)
			return;

		skinRef = bsGeom->skinInstanceRef;
		if (skinRef == -1) {
			BSSkinInstance* newSkinInst = new BSSkinInstance(hdr);
			newSkinInst->targetRef = GetRootNodeID();
			skinRef = AddBlock(newSkinInst, "BSSkin::Instance");

			BSSkinBoneData* newBoneData = new BSSkinBoneData(hdr);
			int boneDataRef = AddBlock(newBoneData, "BSSkin::BoneData");

			newSkinInst->boneDataRef = boneDataRef;
			bsGeom->skinInstanceRef = skinRef;
		}

		bsGeom->SetSkinned(true);

		NiShader* shader = GetShader(shapeName);
		if (shader)
			shader->SetSkinned(true);

		return;
	}

	NiSkinData* skinData = nullptr;
	NiSkinPartition* skinPart = nullptr;
	NiSkinInstance* skinInst = dynamic_cast<NiSkinInstance*>(GetBlock(skinRef));
	if (skinInst) {
		skinData = (NiSkinData*)GetBlock(skinInst->dataRef);
		skinPart = (NiSkinPartition*)GetBlock(skinInst->skinPartitionRef);
	}
	else
		return;

	vector<Triangle> tris;
	ushort numTriangles;
	ushort numVerts;
	if (bType == NITRISHAPEDATA) {
		tris = shapeData->triangles;
		numTriangles = shapeData->numTriangles;
		numVerts = shapeData->numVertices;
	}
	else {
		stripsData->StripsToTris(&tris);
		numTriangles = stripsData->numTriangles;
		numVerts = stripsData->numVertices;
	}

	int maxBonesPerVertex = 4;

	unordered_map<int, vector<int>> vertTris;
	for (int t = 0; t < tris.size(); t++) {
		vertTris[tris[t].p1].push_back(t);
		vertTris[tris[t].p2].push_back(t);
		vertTris[tris[t].p3].push_back(t);
	}

	unordered_map<ushort, vector<int>> vertBones;
	unordered_map<ushort, vector<float>> vertWeights;
	int b = 0;

	for (auto &bone : skinData->bones) {
		for (auto &bw : bone.vertexWeights) {
			int i = 0;
			for (auto &w : vertWeights[bw.index]) {
				if (bw.weight > w)
					break;
				i++;
			}
			vertBones[bw.index].insert(vertBones[bw.index].begin() + i, b);
			vertWeights[bw.index].insert(vertWeights[bw.index].begin() + i, bw.weight);
		}
		b++;
	}

	// Enforce vertex bone weight maximum count
	for (auto &vb : vertBones) {
		if (vb.second.size() > maxBonesPerVertex) {
			vb.second.erase(vb.second.begin() + maxBonesPerVertex);
		}
	}
	for (auto &vw : vertWeights) {
		if (vw.second.size() > maxBonesPerVertex) {
			vw.second.erase(vw.second.begin() + maxBonesPerVertex);
		}
	}

	set<int> testBones;
	auto fTriBones = [&](int x) {
		testBones.clear();
		int p[3];
		p[0] = tris[x].p1;
		p[1] = tris[x].p2;
		p[2] = tris[x].p3;
		for (auto &ip : p) {
			//if (ip == 516)
			//int a = 4;
			for (auto &tb : vertBones[ip])
				testBones.insert(tb);
		}
	};

	int partID = 0;
	unordered_map<int, int> triParts;
	unordered_map<int, set<int>> partBones;

	bool* triAssign;
	triAssign = (bool*)calloc(numTriangles, sizeof(bool));
	bool* usedVerts;
	usedVerts = (bool*)calloc(numVerts, sizeof(bool));

	for (int it = 0; it < numTriangles;) { /* conditional increment in loop */
		if (triAssign[it]) {
			it++;
			continue;
		}

		// Get associated bones for the current tri.
		fTriBones(it);

		if (testBones.size() > maxBonesPerPartition) {
			// TODO: get rid of some bone influences on this tri before trying to put it anywhere.  
		}

		// How many new bones are in the tri's bonelist?
		int newBoneCount = 0;
		for (auto &tb : testBones) {
			if (partBones[partID].find(tb) == partBones[partID].end()) {
				newBoneCount++;
			}
		}
		if (partBones[partID].size() + newBoneCount > maxBonesPerPartition) {
			// too many bones for this partition, start on the next partition with this tri.
			partID++;
			continue;
		}


		partBones[partID].insert(testBones.begin(), testBones.end());
		//partTris[partID].push_back(it);
		triParts[it] = partID;
		triAssign[it] = true;

		memset(usedVerts, 0, sizeof(bool) * numVerts);
		queue<int> vertQueue;

		auto fSelectVerts = [&](int triID) {
			if (!usedVerts[tris[triID].p1]) {
				usedVerts[tris[triID].p1] = true;
				vertQueue.push(tris[triID].p1);
			}
			if (!usedVerts[tris[triID].p2]) {
				usedVerts[tris[triID].p2] = true;
				vertQueue.push(tris[triID].p2);
			}
			if (!usedVerts[tris[triID].p3]) {
				usedVerts[tris[triID].p3] = true;
				vertQueue.push(tris[triID].p3);
			}
		};

		// Select the tri's un-visited verts for adjancency examination.
		fSelectVerts(it);

		while (!vertQueue.empty()) {
			int adjVert = vertQueue.front();
			vertQueue.pop();

			for (auto &adjTri : vertTris[adjVert]) {
				if (triAssign[adjTri])			// skip triangles we've already assigned
					continue;

				// Get associated bones for the current tri.
				fTriBones(adjTri);

				if (testBones.size() > maxBonesPerPartition) {
					// TODO: get rid of some bone influences on this tri before trying to put it anywhere.  
				}

				// How many new bones are in the tri's bonelist?
				int newBoneCount = 0;
				for (auto &tb : testBones) {
					if (partBones[partID].find(tb) == partBones[partID].end()) {
						newBoneCount++;
					}
				}
				if (partBones[partID].size() + newBoneCount > maxBonesPerPartition) {
					// too many bones for this partition, ignore this tri, we'll catch it in the outer loop later
					continue;
				}

				fSelectVerts(adjTri);			// save the next set of adjacent verts

				partBones[partID].insert(testBones.begin(), testBones.end());
				//	partTris[partID].push_back(adjTri);
				triParts[adjTri] = partID;
				triAssign[adjTri] = true;
			}
		}
		// next outer tri
		it++;
	}

	skinPart->partitions.clear();
	skinPart->numPartitions = partID + 1;

	for (int pID = 0; pID <= partID; pID++) {
		NiSkinPartition::PartitionBlock pblock;
		pblock.hasBoneIndices = true;
		pblock.hasFaces = true;
		pblock.hasVertexMap = true;
		pblock.hasVertexWeights = true;
		pblock.numStrips = 0;
		pblock.numWeightsPerVertex = maxBonesPerVertex;
		pblock.numVertices = 0;
		pblock.unkShort = 0;

		//pblock.numTriangles = partTris[pID].size();
		unordered_map<int, int> vertMap;

		//for (auto &triID : partTris[pID]) {
		for (int triID = 0; triID < tris.size(); triID++) {
			if (triParts[triID] != pID)
				continue;
			Triangle t = tris[triID];
			t.rot();
			if (vertMap.find(t.p1) == vertMap.end()) {
				vertMap[t.p1] = pblock.numVertices;
				pblock.vertexMap.push_back(t.p1);
				t.p1 = pblock.numVertices++;
			}
			else {
				t.p1 = vertMap[t.p1];
			}
			if (vertMap.find(t.p2) == vertMap.end()) {
				vertMap[t.p2] = pblock.numVertices;
				pblock.vertexMap.push_back(t.p2);
				t.p2 = pblock.numVertices++;
			}
			else {
				t.p2 = vertMap[t.p2];
			}
			if (vertMap.find(t.p3) == vertMap.end()) {
				vertMap[t.p3] = pblock.numVertices;
				pblock.vertexMap.push_back(t.p3);
				t.p3 = pblock.numVertices++;
			}
			else {
				t.p3 = vertMap[t.p3];
			}
			pblock.triangles.push_back(t);
		}
		pblock.numTriangles = pblock.triangles.size();
		pblock.numBones = partBones[pID].size();
		unordered_map<int, int> bonelookup;
		for (auto &b : partBones[pID]) {
			pblock.bones.push_back(b);
			bonelookup[b] = pblock.bones.size() - 1;
		}
		for (auto &v : pblock.vertexMap) {
			BoneIndices b;
			VertexWeight vw;
			b.i1 = b.i2 = b.i3 = b.i4 = 0;
			vw.w1 = vw.w2 = vw.w3 = vw.w4 = 0;
			byte* pb = (byte*)&b.i1;
			float* pw = (float*)&vw.w1;
			float tot = 0.0f;
			for (int bi = 0; bi < vertBones[v].size(); bi++) {
				if (bi == 4) {
					break;
				}

				pb[bi] = bonelookup[vertBones[v][bi]];
				pw[bi] = vertWeights[v][bi];
				tot += pw[bi];
			}
			for (int bi = 0; bi < 4; bi++) {
				pw[bi] /= tot;
			}
			pblock.boneIndices.push_back(b);
			pblock.vertexWeights.push_back(vw);
		}

		skinPart->partitions.push_back(move(pblock));

	}

	if (skinInst->blockType == BSDISMEMBERSKININSTANCE) {
		BSDismemberSkinInstance* bsdSkinInst = dynamic_cast<BSDismemberSkinInstance*>(skinInst);
		bsdSkinInst->numPartitions = skinPart->numPartitions;
		bsdSkinInst->partitions.clear();
		for (int i = 0; i < bsdSkinInst->numPartitions; i++) {
			BSDismemberSkinInstance::Partition p;
			if (hdr.VerCheck(20, 2, 0, 7) && hdr.userVersion == 11)
				p.partID = 0;
			else
				p.partID = 32;

			if (i == 0)
				p.flags = 257;
			else
				p.flags = 257;

			bsdSkinInst->partitions.push_back(p);
		}
	}
	hdr.blockSizes[skinInst->skinPartitionRef] = skinPart->CalcBlockSize();
	hdr.blockSizes[skinRef] = skinInst->CalcBlockSize();
}

void NifFile::UpdateBoundingSphere(const string& shapeName) {
	int bType;
	int dataRef = shapeDataIdForName(shapeName, bType);
	if (dataRef == -1)
		return;

	if (bType == NITRISHAPEDATA || bType == NITRISTRIPSDATA) {
		NiTriBasedGeom* geom = geomForName(shapeName);
		if (!geom)
			return;

		NiTriBasedGeomData* geomData = dynamic_cast<NiTriBasedGeomData*>(GetBlock(dataRef));
		if (!geomData)
			return;

		Vector3 a(FLT_MAX, FLT_MAX, FLT_MAX);
		Vector3 b(-FLT_MAX, -FLT_MAX, -FLT_MAX);
		for (auto &v : geomData->vertices) {
			a.x = min(a.x, v.x);
			a.y = min(a.y, v.y);
			a.z = min(a.z, v.z);
			b.x = max(b.x, v.x);
			b.y = max(b.y, v.y);
			b.z = max(b.z, v.z);
		}

		geomData->center = (a + b) / 2.0f;
		for (auto &v : geomData->vertices)
			geomData->radius = max(geomData->radius, geomData->center.DistanceTo(v));
	}
	else if (bType == BSSUBINDEXTRISHAPE || bType == BSTRISHAPE || bType == BSMESHLODTRISHAPE) {
		BSTriShape* geom = dynamic_cast<BSTriShape*>(GetBlock(dataRef));
		if (!geom)
			return;

		Vector3 a(FLT_MAX, FLT_MAX, FLT_MAX);
		Vector3 b(-FLT_MAX, -FLT_MAX, -FLT_MAX);
		for (int i = 0; i < geom->vertData.size(); i++) {
			a.x = min(a.x, geom->vertData[i].vert.x);
			a.y = min(a.y, geom->vertData[i].vert.y);
			a.z = min(a.z, geom->vertData[i].vert.z);
			b.x = max(b.x, geom->vertData[i].vert.x);
			b.y = max(b.y, geom->vertData[i].vert.y);
			b.z = max(b.z, geom->vertData[i].vert.z);
		}

		geom->center = (a + b) / 2.0f;
		for (int i = 0; i < geom->vertData.size(); i++)
			geom->radius = max(geom->radius, geom->center.DistanceTo(geom->vertData[i].vert));
	}
}
