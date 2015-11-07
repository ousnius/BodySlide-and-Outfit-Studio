/*
BodySlide and Outfit Studio
Copyright (C) 2015  Caliente & ousnius
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
			if (geom && !name.compare(geom->name)) {
				if (numFound >= dupIndex)
					return geom;
				numFound++;
			}
		}
	}
	return nullptr;
}

int NifFile::shapeDataIdForName(const string& name, int& outBlockType) {
	for (auto& block : blocks) {
		ushort type = block->blockType;
		if (type == NITRISHAPE || type == NITRISTRIPS) {
			NiTriBasedGeom* geom = dynamic_cast<NiTriBasedGeom*>(block);
			if (geom && !name.compare(geom->name)) {
				NiTriBasedGeomData* geomData = dynamic_cast<NiTriBasedGeomData*>(blocks[geom->dataRef]);
				outBlockType = geomData->blockType;
				return geom->dataRef;
			}
		}
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
			if (geom && !name.compare(geom->name))
				return id;
		}
	}
	return -1;
}

NiNode* NifFile::nodeForName(const string& name) {
	for (auto& block : blocks) {
		if (block->blockType == NINODE) {
			NiNode* node = (NiNode*)block;
			if (!name.compare(node->name))
				return node;
		}
	}
	return nullptr;
}

int NifFile::nodeIdForName(const string& name) {
	int id = -1;
	for (auto& block : blocks) {
		id++;
		if (block->blockType == NINODE) {
			NiNode* node = (NiNode*)block;
			if (!name.compare(node->name))
				return id;
		}
	}
	return -1;
}

int NifFile::shapeBoneIndex(const string& shapeName, const string& boneName) {
	int skinRef = -1;

	NiTriBasedGeom* geom = geomForName(shapeName);
	if (geom)
		skinRef = geom->skinInstanceRef;

	if (skinRef == -1)
		return -1;

	NiSkinInstance* skinInst = dynamic_cast<NiSkinInstance*>(blocks[skinRef]);
	if (skinInst) {
		for (int i = 0; i < skinInst->bones.size(); i++) {
			int boneBlock = skinInst->bones[i];
			if (boneBlock != -1) {
				NiNode* node = (NiNode*)blocks[boneBlock];
				if (node->name == boneName)
					return i;
			}
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


	//for (int i = 0; i < oldOrder.size(); i++)
	//	SwapBlocks(shapeIdForName(oldOrder[i]), shapeIdForName(order[i]));
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
	ushort btID = this->AddOrFindBlockTypeId(blockTypeName);
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
	newNode->nameRef = AddOrFindStringId(nodeName);
	newNode->name = nodeName;

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

string NifFile::NodeName(int blockID) {
	NiNode* n = dynamic_cast<NiNode*>(blocks[blockID]);
	if (!n)
		return "";
	if (n->name.empty())
		return "_unnamed_";
	return n->name;
}

int NifFile::AddStringExtraData(const string& shapeName, const string& name, const string& stringData) {
	NiStringExtraData* strExtraData = new NiStringExtraData(hdr);
	strExtraData->nameRef = AddOrFindStringId(name);
	strExtraData->name = name;
	strExtraData->stringDataRef = AddOrFindStringId(stringData);
	strExtraData->stringData = stringData;

	int strExtraDataId = AddBlock(strExtraData, "NiStringExtraData");
	if (strExtraDataId != -1) {
		int id = shapeIdForName(shapeName);
		if (id == -1)
			return -1;

		NiTriBasedGeom* geom = geomForName(shapeName);
		if (geom) {
			geom->extraDataRef.push_back(strExtraDataId);
			geom->numExtraData++;
			hdr.blockSizes[id] = geom->CalcBlockSize();
		}
		else
			return -1;
	}
	return strExtraDataId;
}

NiShader* NifFile::GetShader(const string& shapeName) {
	int prop1 = -1;

	NiTriBasedGeom* geom = geomForName(shapeName);
	if (geom)
		prop1 = geom->propertiesRef1;
	else
		return nullptr;

	if (prop1 != -1) {
		NiShader* shader = dynamic_cast<NiShader*>(blocks[prop1]);
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
			NiShader* shader = dynamic_cast<NiShader*>(blocks[props[i]]);
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
		return shader->IsSkin();

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
		NiMaterialProperty* material = dynamic_cast<NiMaterialProperty*>(blocks[props[i]]);
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

	BSShaderTextureSet* textureSet = dynamic_cast<BSShaderTextureSet*>(blocks[textureSetRef]);
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

	BSShaderTextureSet* textureSet = dynamic_cast<BSShaderTextureSet*>(blocks[textureSetRef]);
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
		for (int i = 0; i < 9; i++) {
			if (GetTextureForShape(s, tFile, i) && !tFile.empty()) {
				tFile = regex_replace(tFile, regex("/+|\\\\+"), "\\"); // Replace multiple slashes or forward slashes with one backslash
				tFile = regex_replace(tFile, regex("^\\\\+", regex_constants::icase), ""); // Remove all backslashes from the front
				tFile = regex_replace(tFile, regex(".*?Data\\\\", regex_constants::icase), ""); // Remove everything before and including the data path root
				tFile = regex_replace(tFile, regex("^(?!^textures\\\\)", regex_constants::icase), "textures\\"); // Add textures root path if not existing
				SetTextureForShape(s, tFile, i);
			}
		}
	}
}

bool NifFile::HasBlockType(string typeStr) {
	for (int i = 0; i < hdr.blockTypes.size(); i++)
		if (hdr.blockTypes[i].str == typeStr)
			return true;
	return false;
}

ushort NifFile::AddOrFindBlockTypeId(const string& blockTypeName) {
	NiString nstr;
	ushort typeId = (ushort)hdr.blockTypes.size();
	for (ushort i = 0; i < hdr.blockTypes.size(); i++) {
		if (hdr.blockTypes[i].str == blockTypeName) {
			typeId = i;
			break;
		}
	}

	// Shader block type not found, add it
	if (typeId == hdr.blockTypes.size()) {
		nstr.str = blockTypeName;
		hdr.blockTypes.push_back(nstr);
		hdr.numBlockTypes++;
	}
	return typeId;
}

int NifFile::FindStringId(const string& str) {
	for (int i = 0; i < hdr.strings.size(); i++)
		if (hdr.strings[i].str.compare(str) == 0)
			return i;
	return -1;
}

int NifFile::AddOrFindStringId(const string& str) {
	for (int i = 0; i < hdr.strings.size(); i++)
		if (hdr.strings[i].str.compare(str) == 0)
			return i;

	int r = hdr.strings.size();
	NiString ns;
	ns.outputNull = true;
	ns.str = str;
	hdr.strings.push_back(ns);
	hdr.numStrings++;
	if (str.length() > hdr.maxStringLen)
		hdr.maxStringLen = str.length();
	return r;
}

void NifFile::CopyShader(const string& shapeDest, int srcShaderRef, NifFile& srcNif, bool addAlpha, int propRef1 = -1, int propRef2 = -1) {
	int dataRef = -1;

	NiTriBasedGeom* geom = geomForName(shapeDest);
	if (geom)
		dataRef = geom->dataRef;
	else
		return;

	// Get source shader
	NiShader* srcShader = dynamic_cast<NiShader*>(srcNif.GetBlock(srcShaderRef));
	if (!srcShader)
		return;

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
	destShader->nameRef = 0xFFFFFFFF;

	// Add shader block to nif
	int shaderId = blocks.size();
	blocks.push_back(destShader);
	hdr.numBlocks++;

	// Texture Set
	BSShaderTextureSet* destTexSet = nullptr;
	int textureSetRef = srcShader->GetTextureSetRef();
	if (textureSetRef != -1) {
		// Create texture set block and copy
		BSShaderTextureSet* srcTexSet = dynamic_cast<BSShaderTextureSet*>(srcNif.GetBlock(textureSetRef));
		destTexSet = new BSShaderTextureSet(*srcTexSet);
		destTexSet->header = &hdr;

		// Add texture block to nif
		int texsetId = blocks.size();
		blocks.push_back(destTexSet);
		hdr.numBlocks++;

		// Assign texture set block id to shader
		destShader->SetTextureSetRef(texsetId);
	}

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

	if (srcShader->blockType == BSLIGHTINGSHADERPROPERTY)
		geom->propertiesRef1 = shaderId;
	else if (srcShader->blockType == BSSHADERPPLIGHTINGPROPERTY)
		geom->propertiesRef[propRef1] = shaderId;
	else if (srcShader->blockType == BSEFFECTSHADERPROPERTY)
		geom->propertiesRef1 = shaderId;

	// Kill normals, set numUVSets to 1
	if (dataRef != -1 && destShader->IsSkin() && hdr.userVersion >= 12) {
		NiTriBasedGeomData* geomData = dynamic_cast<NiTriBasedGeomData*>(blocks[dataRef]);
		if (geomData) {
			geomData->hasNormals = 0;
			geomData->normals.clear();
			geomData->bitangents.clear();
			geomData->tangents.clear();
			geomData->numUVSets = 1;
			hdr.blockSizes[dataRef] = geomData->CalcBlockSize();
		}
	}

	// Add shader and other sizes to block size info
	hdr.blockSizes.push_back(destShader->CalcBlockSize());

	if (destTexSet)
		hdr.blockSizes.push_back(destTexSet->CalcBlockSize());

	if (destController)
		hdr.blockSizes.push_back(destController->CalcBlockSize());

	if (addAlpha) {
		NiAlphaProperty* alphaProp = new NiAlphaProperty(hdr);
		if (srcShader->blockType == BSLIGHTINGSHADERPROPERTY)
			geom->propertiesRef2 = blocks.size();
		else if (srcShader->blockType == BSSHADERPPLIGHTINGPROPERTY)
			geom->propertiesRef[propRef2] = blocks.size();
		else if (srcShader->blockType == BSEFFECTSHADERPROPERTY)
			geom->propertiesRef2 = blocks.size();
		blocks.push_back(alphaProp);
		hdr.numBlocks++;
		hdr.blockSizes.push_back(alphaProp->CalcBlockSize());
	}

	// Record the block type in the block type index.
	ushort shaderTypeId = AddOrFindBlockTypeId(srcNif.hdr.blockTypes[srcNif.hdr.blockIndex[srcShaderRef]].str);
	hdr.blockIndex.push_back(shaderTypeId);

	if (destTexSet) {
		ushort texSetTypeId = AddOrFindBlockTypeId(srcNif.hdr.blockTypes[srcNif.hdr.blockIndex[textureSetRef]].str);
		hdr.blockIndex.push_back(texSetTypeId);
	}

	if (destController) {
		ushort controllerTypeId = AddOrFindBlockTypeId(srcNif.hdr.blockTypes[srcNif.hdr.blockIndex[srcShader->controllerRef]].str);
		hdr.blockIndex.push_back(controllerTypeId);
	}

	if (addAlpha) {
		ushort alphaBlockTypeId = AddOrFindBlockTypeId("NiAlphaProperty");
		hdr.blockIndex.push_back(alphaBlockTypeId);
	}
}

int NifFile::CopyNamedNode(string& nodeName, NifFile& srcNif) {
	NiNode* srcNode = srcNif.nodeForName(nodeName);
	if (!srcNode)
		return -1;

	NiNode* destNode = new NiNode((*srcNode));
	destNode->header = &hdr;
	int strPtr = AddOrFindStringId(nodeName);
	destNode->nameRef = strPtr;
	destNode->name = nodeName;

	int blockID = blocks.size();
	blocks.push_back(destNode);
	hdr.blockSizes.push_back(destNode->CalcBlockSize());
	hdr.blockIndex.push_back(AddOrFindBlockTypeId("NiNode"));
	hdr.numBlocks++;

	return blockID;
}

void NifFile::CopyGeometry(const string& shapeDest, NifFile& srcNif, const string& srcShape) {
	NiTriBasedGeom* srcGeom = srcNif.geomForName(srcShape);
	if (!srcGeom)
		return;

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
	destGeom->nameRef = AddOrFindStringId(shapeDest);
	destGeom->name = shapeDest;

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
	hdr.blockIndex.push_back(AddOrFindBlockTypeId(srcNif.hdr.blockTypes[srcNif.hdr.blockIndex[srcGeomId]].str));

	if (srcGeomData)
		hdr.blockIndex.push_back(AddOrFindBlockTypeId(srcNif.hdr.blockTypes[srcNif.hdr.blockIndex[srcGeom->dataRef]].str));

	if (!skipSkinInst) {
		hdr.blockIndex.push_back(AddOrFindBlockTypeId(srcNif.hdr.blockTypes[srcNif.hdr.blockIndex[srcGeom->skinInstanceRef]].str));
		hdr.blockIndex.push_back(AddOrFindBlockTypeId(srcNif.hdr.blockTypes[srcNif.hdr.blockIndex[srcSkinInst->dataRef]].str));
		hdr.blockIndex.push_back(AddOrFindBlockTypeId(srcNif.hdr.blockTypes[srcNif.hdr.blockIndex[srcSkinInst->skinPartitionRef]].str));
	}

	hdr.blockSizes.push_back(destGeom->CalcBlockSize());
	if (srcGeomData)
		hdr.blockSizes.push_back(destGeomData->CalcBlockSize());

	if (!skipSkinInst) {
		hdr.blockSizes.push_back(destSkinInst->CalcBlockSize());
		hdr.blockSizes.push_back(destSkinData->CalcBlockSize());
		hdr.blockSizes.push_back(destSkinPart->CalcBlockSize());
	}

	bool shaderAlpha = false;
	NiShader* srcShader = dynamic_cast<NiShader*>(srcNif.GetBlock(srcGeom->propertiesRef1));
	if (srcShader) {
		if (srcGeom->propertiesRef2 != -1)
			shaderAlpha = true;

		CopyShader(shapeDest, srcGeom->propertiesRef1, srcNif, shaderAlpha);
	}
	else {
		int propRef1 = -1;
		int propRef2 = -1;
		for (int i = 0; i < srcGeom->numProperties; i++) {
			if (!srcShader) {
				srcShader = dynamic_cast<NiShader*>(srcNif.GetBlock(srcGeom->propertiesRef[i]));
				if (srcShader) {
					propRef1 = i;
					continue;
				}
			}
			if (!shaderAlpha) {
				NiAlphaProperty* alpha = dynamic_cast<NiAlphaProperty*>(srcNif.GetBlock(srcGeom->propertiesRef[i]));
				if (alpha) {
					shaderAlpha = true;
					propRef2 = i;
					continue;
				}
			}

			NiMaterialProperty* material = dynamic_cast<NiMaterialProperty*>(srcNif.GetBlock(srcGeom->propertiesRef[i]));
			if (material) {
				NiMaterialProperty* destMaterial = new NiMaterialProperty(*material);
				destMaterial->header = &hdr;
				int materialId = blocks.size();
				blocks.push_back(destMaterial);
				hdr.numBlocks++;
				hdr.blockSizes.push_back(destMaterial->CalcBlockSize());
				hdr.blockIndex.push_back(AddOrFindBlockTypeId(srcNif.hdr.blockTypes[srcNif.hdr.blockIndex[srcGeom->propertiesRef[i]]].str));
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
				hdr.blockIndex.push_back(AddOrFindBlockTypeId(srcNif.hdr.blockTypes[srcNif.hdr.blockIndex[srcGeom->propertiesRef[i]]].str));
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
				hdr.blockIndex.push_back(AddOrFindBlockTypeId(srcNif.hdr.blockTypes[srcNif.hdr.blockIndex[srcGeom->propertiesRef[i]]].str));
				destGeom->propertiesRef[i] = unknownId;
			}
		}

		if (propRef1 != -1) {
			if (srcShader)
				CopyShader(shapeDest, srcGeom->propertiesRef[propRef1], srcNif, shaderAlpha, propRef1, propRef2);
			else
				destGeom->propertiesRef[propRef1] = -1;
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
		bonePtr = nodeIdForName(boneName);
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

int NifFile::GetShapeList(vector<string>& outList) {
	outList.clear();
	for (auto& block : blocks) {
		ushort type = block->blockType;
		if (type == NITRISHAPE || type == NITRISTRIPS) {
			NiTriBasedGeom* geom = dynamic_cast<NiTriBasedGeom*>(block);
			if (geom)
				outList.push_back(geom->name);
		}
	}
	return outList.size();
}

void NifFile::RenameShape(const string& oldName, const string& newName) {
	uint strID;

	NiTriBasedGeom* geom = geomForName(oldName);
	if (geom) {
		strID = geom->nameRef;
		geom->name = newName;
	}
	else
		return;

	hdr.strings[strID].str = newName;
	if (hdr.maxStringLen < newName.length())
		hdr.maxStringLen = newName.length();

	wxLogMessage("Renamed shape '%s' to '%s' in file '%s'.", oldName, newName, fileName);
}

void NifFile::RenameDuplicateShape(const string& dupedShape) {
	int dupCount = 1;
	char buf[10];

	NiTriBasedGeom* geom = geomForName(dupedShape);
	if (geom) {
		while ((geom = geomForName(dupedShape, 1)) != nullptr) {
			_snprintf(buf, 10, "_%d", dupCount);
			while (FindStringId(geom->name + buf) != -1) {
				dupCount++;
				_snprintf(buf, 10, "_%d", dupCount);
			}
			geom->name = geom->name + buf;
			geom->nameRef = AddOrFindStringId(geom->name);
			dupCount++;
		}
	}
}

void NifFile::SetNodeName(int blockID, const string& newName) {
	NiNode* node = dynamic_cast<NiNode*>(blocks[blockID]);
	if (!node)
		return;

	uint strID = node->nameRef;
	node->name = newName;
	hdr.strings[strID].str = newName;
	if (hdr.maxStringLen < newName.length())
		hdr.maxStringLen = newName.length();
}
	
int NifFile::GetNodeID(const string& nodeName) {
	int id = -1;
	for (auto& block : blocks) {
		id++;
		if (block->blockType == NINODE) {
			NiNode* node = (NiNode*)block;
			if (!node->name.compare(nodeName))
				return id;
		}
	}
	return -1;
}

bool NifFile::GetNodeTransform(const string& nodeName, vector<Vector3>& outRot, Vector3& outTrans, float& outScale) {
	for (auto& block : blocks) {
		if (block->blockType == NINODE) {
			NiNode* node = (NiNode*)block;
			if (!node->name.compare(nodeName)) {
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

bool NifFile::SetNodeTransform(const string& nodeName, SkinTransform& inXform) {
	for (auto& block : blocks) {
		if (block->blockType == NINODE) {
			NiNode* node = (NiNode*)block;
			if (!node->name.compare(nodeName)) {
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

int NifFile::GetShapeBoneList(const string& shapeName, vector<string>& outList) {
	int skinRef = -1;
	outList.clear();

	NiTriBasedGeom* geom = geomForName(shapeName);
	if (geom)
		skinRef = geom->skinInstanceRef;

	if (skinRef == -1)
		return 0;

	NiSkinInstance* skinInst = dynamic_cast<NiSkinInstance*>(blocks[skinRef]);
	if (!skinInst)
		return 0;

	int boneBlock;
	for (int i = 0; i < skinInst->bones.size(); i++) {
		boneBlock = skinInst->bones[i];
		if (boneBlock != -1) {
			NiNode* node = (NiNode*)blocks[boneBlock];
			outList.push_back(node->name);
		}
	}
	return outList.size();
}

int NifFile::GetShapeBoneIDList(const string& shapeName, vector<int>& outList) {
	int skinRef = -1;
	outList.clear();

	NiTriBasedGeom* geom = geomForName(shapeName);
	if (geom)
		skinRef = geom->skinInstanceRef;

	if (skinRef == -1)
		return 0;

	NiSkinInstance* skinInst = dynamic_cast<NiSkinInstance*>(blocks[skinRef]);
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

	NiTriBasedGeom* geom = geomForName(shapeName);
	if (geom)
		skinRef = geom->skinInstanceRef;

	if (skinRef == -1)
		return;

	NiSkinInstance* skinInst = dynamic_cast<NiSkinInstance*>(blocks[skinRef]);
	if (!skinInst)
		return;

	skinInst->bones.clear();
	skinInst->numBones = 0;
	for (int i = 0; i < inList.size(); i++) {
		skinInst->bones.push_back(inList[i]);
		skinInst->numBones++;
	}
	hdr.blockSizes[skinRef] = skinInst->CalcBlockSize();

	NiSkinData* skinData = (NiSkinData*)blocks[skinInst->dataRef];
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

int NifFile::GetShapeBoneWeights(const string& shapeName, int boneIndex, unordered_map<ushort, float>& outWeights) {
	int skinRef = -1;
	outWeights.clear();

	NiTriBasedGeom* geom = geomForName(shapeName);
	if (geom)
		skinRef = geom->skinInstanceRef;

	if (skinRef == -1)
		return 0;

	NiSkinInstance* skinInst = dynamic_cast<NiSkinInstance*>(blocks[skinRef]);
	if (!skinInst)
		return 0;

	if (skinInst->dataRef == -1)
		return 0;

	NiSkinData* skinData = (NiSkinData*)blocks[skinInst->dataRef];
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
	if (geom)
		skinRef = geom->skinInstanceRef;

	if (skinRef == -1)
		return false;

	NiSkinInstance* skinInst = dynamic_cast<NiSkinInstance*>(blocks[skinRef]);
	if (!skinInst)
		return false;

	if (skinInst->dataRef == -1)
		return false;

	NiSkinData* skinData = (NiSkinData*)blocks[skinInst->dataRef];
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
	if (geom)
		skinRef = geom->skinInstanceRef;

	if (skinRef == -1)
		return false;

	NiSkinInstance* skinInst = dynamic_cast<NiSkinInstance*>(blocks[skinRef]);
	if (!skinInst)
		return false;

	if (skinInst->dataRef == -1)
		return false;

	NiSkinData* skinData = (NiSkinData*)blocks[skinInst->dataRef];
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

	if (skinRef == -1)
		return;

	NiSkinInstance* skinInst = dynamic_cast<NiSkinInstance*>(blocks[skinRef]);
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

	if (skinRef == -1)
		return;

	NiSkinInstance* skinInst = dynamic_cast<NiSkinInstance*>(blocks[skinRef]);
	if (!skinInst)
		return;

	if (skinInst->dataRef == -1)
		return;

	NiSkinData* skinData = (NiSkinData*)blocks[skinInst->dataRef];
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

const vector<Vector3>* NifFile::GetRawVertsForShape(const string& shapeName) {
	int bType;
	int dataRef = shapeDataIdForName(shapeName, bType);
	if (dataRef == -1)
		return nullptr;

	NiTriBasedGeomData* geomData = dynamic_cast<NiTriBasedGeomData*>(blocks[dataRef]);
	return &geomData->vertices;
}

bool NifFile::GetTrisForShape(const string& shapeName, vector<Triangle>* outTris) {
	NiTriBasedGeom* geom = geomForName(shapeName);
	if (!geom)
		return false;

	int dataRef = geom->dataRef;
	if (dataRef == -1 || dataRef >= hdr.numBlocks)
		return false;

	if (geom->blockType == NITRISHAPE) {
		NiTriShapeData* shapeData = (NiTriShapeData*)(blocks[dataRef]);
		*outTris = shapeData->triangles;
		return true;
	}
	else if (geom->blockType == NITRISTRIPS) {
		NiTriStripsData* stripsData = (NiTriStripsData*)(blocks[dataRef]);
		stripsData->StripsToTris(outTris);
		return true;
	}
	return false;
}

const vector<Vector3>* NifFile::GetNormalsForShape(const string& shapeName) {
	int bType;
	int dataID = shapeDataIdForName(shapeName, bType);
	if (dataID == -1)
		return nullptr;

	if (bType == NITRISHAPEDATA) {
		NiTriShapeData* shapeData = (NiTriShapeData*)blocks[dataID];
		if (shapeData->hasNormals)
			return &shapeData->normals;
	}
	else if (bType == NITRISTRIPSDATA) {
		NiTriStripsData* stripsData = (NiTriStripsData*)blocks[dataID];
		if (stripsData->hasNormals)
			return &stripsData->normals;
	}
	return nullptr;
}

const vector<Vector2>* NifFile::GetUvsForShape(const string& shapeName) {
	int bType;
	int dataID = shapeDataIdForName(shapeName, bType);
	if (dataID == -1)
		return nullptr;

	if (bType == NITRISHAPEDATA) {
		NiTriShapeData* shapeData = (NiTriShapeData*)blocks[dataID];
		if (shapeData->numUVSets > 0)
			return &shapeData->uvSets;
	}
	else if (bType == NITRISTRIPSDATA) {
		NiTriStripsData* stripsData = (NiTriStripsData*)blocks[dataID];
		if (stripsData->numUVSets > 0)
			return &stripsData->uvSets;
	}
	return nullptr;
}

bool NifFile::GetUvsForShape(const string& shapeName, vector<Vector2>& outUvs) {
	int bType;
	int dataID = shapeDataIdForName(shapeName, bType);
	if (dataID == -1)
		return false;

	if (bType == NITRISHAPEDATA) {
		NiTriShapeData* shapeData = (NiTriShapeData*)blocks[dataID];
		outUvs.assign(shapeData->uvSets.begin(), shapeData->uvSets.end());
		return true;
	}
	else if (bType == NITRISTRIPSDATA) {
		NiTriStripsData* stripsData = (NiTriStripsData*)blocks[dataID];
		outUvs.assign(stripsData->uvSets.begin(), stripsData->uvSets.end());
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
		//NiTriShapeData* shapeData = (NiTriShapeData*)blocks[dataID];
		// dosomthin
	}
	else {
		//NiTriStripsData* stripsData = (NiTriStripsData*)blocks[dataID];
		// dosomthin
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
		NiTriShapeData* shapeData = (NiTriShapeData*)blocks[dataRef];
		for (auto &v : shapeData->vertices)
			outVerts.push_back(v);
		return true;
	}
	else if (bType == NITRISTRIPSDATA) {
		NiTriStripsData* stripsData = (NiTriStripsData*)blocks[dataRef];
		for (auto &v : stripsData->vertices)
			outVerts.push_back(v);
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
		NiTriShapeData* shapeData = (NiTriShapeData*)blocks[dataID];
		if (shapeData)
			return shapeData->numVertices;
	}
	else if (bType == NITRISTRIPSDATA) {
		NiTriStripsData* stripsData = (NiTriStripsData*)blocks[dataID];
		if (stripsData)
			return stripsData->numVertices;
	}
	return -1;
}

void NifFile::SetVertsForShape(const string& shapeName, const vector<Vector3>& verts) {
	int bType;
	int dataID = shapeDataIdForName(shapeName, bType);
	if (dataID == -1)
		return;

	if (bType == NITRISHAPEDATA) {
		NiTriShapeData* shapeData = (NiTriShapeData*)blocks[dataID];
		if (verts.size() != shapeData->vertices.size())
			return;

		for (int i = 0; i < shapeData->vertices.size(); i++)
			shapeData->vertices[i] = verts[i];
	}
	else if (bType == NITRISTRIPSDATA) {
		NiTriStripsData* stripsData = (NiTriStripsData*)blocks[dataID];
		if (verts.size() != stripsData->vertices.size())
			return;

		for (int i = 0; i < stripsData->vertices.size(); i++)
			stripsData->vertices[i] = verts[i];
	}
}

void NifFile::SetUvsForShape(const string& shapeName, const vector<Vector2>& uvs) {
	int bType;
	int dataID = shapeDataIdForName(shapeName, bType);
	if (dataID == -1)
		return;

	if (bType == NITRISHAPEDATA) {
		NiTriShapeData* shapeData = (NiTriShapeData*)blocks[dataID];
		if (uvs.size() != shapeData->vertices.size())
			return;

		shapeData->uvSets.assign(uvs.begin(), uvs.end());
	}
	else if (bType == NITRISTRIPSDATA) {
		NiTriStripsData* stripsData = (NiTriStripsData*)blocks[dataID];
		if (uvs.size() != stripsData->vertices.size())
			return;

		stripsData->uvSets.assign(uvs.begin(), uvs.end());
	}
}

void NifFile::SetNormalsForShape(const string& shapeName, const vector<Vector3>& norms) {
	int bType;
	int dataID = shapeDataIdForName(shapeName, bType);
	if (dataID == -1)
		return;

	if (bType == NITRISHAPEDATA) {
		NiTriShapeData* shapeData = (NiTriShapeData*)blocks[dataID];
		if (norms.size() != shapeData->normals.size())
			shapeData->normals.resize(norms.size());
		for (int i = 0; i < shapeData->vertices.size(); i++)
			shapeData->normals[i] = norms[i];
		shapeData->hasNormals = true;
		shapeData->numUVSets |= 1;
		hdr.blockSizes[dataID] = shapeData->CalcBlockSize();
	}
	else if (bType == NITRISTRIPSDATA) {
		NiTriStripsData* stripsData = (NiTriStripsData*)blocks[dataID];
		if (norms.size() != stripsData->normals.size())
			stripsData->normals.resize(norms.size());
		for (int i = 0; i < stripsData->vertices.size(); i++)
			stripsData->normals[i] = norms[i];
		stripsData->hasNormals = true;
		stripsData->numUVSets |= 1;
		hdr.blockSizes[dataID] = stripsData->CalcBlockSize();
	}
}
	
void NifFile::CalcTangentsForShape(const string& shapeName) {
	int bType;
	int dataID = shapeDataIdForName(shapeName, bType);
	if (dataID == -1)
		return;

	if (bType == NITRISHAPEDATA) {
		NiTriShapeData* shapeData = (NiTriShapeData*)blocks[dataID];
		shapeData->CalcTangentSpace();
		hdr.blockSizes[dataID] = shapeData->CalcBlockSize();
	}
	else if (bType == NITRISTRIPSDATA) {
		NiTriStripsData* stripsData = (NiTriStripsData*)blocks[dataID];
		stripsData->CalcTangentSpace();
		hdr.blockSizes[dataID] = stripsData->CalcBlockSize();
	}
}

void NifFile::ClearShapeTransform(const string& shapeName) {
	NiTriBasedGeom* geom = geomForName(shapeName);
	if (geom) {
		geom->translation.Zero();
		geom->scale = 1.0f;
		geom->rotation[0] = Vector3(1.0f, 0.0f, 0.0f);
		geom->rotation[1] = Vector3(0.0f, 1.0f, 0.0f);
		geom->rotation[2] = Vector3(0.0f, 0.0f, 1.0f);
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
	NiTriBasedGeom* geom = geomForName(shapeName);
	if (geom) {
		xFormShape.translation = geom->translation;
		xFormShape.scale = geom->scale;
		xFormShape.rotation[0] = geom->rotation[0];
		xFormShape.rotation[1] = geom->rotation[1];
		xFormShape.rotation[2] = geom->rotation[2];
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
	NiTriBasedGeom* geom = geomForName(shapeName);
	if (geom)
		outVec = geom->translation;

	NiNode* root = dynamic_cast<NiNode*>(blocks[0]);
	if (root)
		outVec += root->translation;

	//if (outVec.DistanceTo(Vector3(0.0f, 0.0f, 0.0f)) < EPSILON)
	//	outVec.Zero();
}

void NifFile::SetShapeTranslation(const string& shapeName, const Vector3& newTrans) {
	NiTriBasedGeom* geom = geomForName(shapeName);
	if (geom)
		geom->translation = newTrans;
}

void NifFile::GetShapeScale(const string& shapeName, float& outScale) {
	NiTriBasedGeom* geom = geomForName(shapeName);
	if (geom)
		outScale = geom->scale;
}

void NifFile::SetShapeScale(const string& shapeName, const float& newScale) {
	NiTriBasedGeom* geom = geomForName(shapeName);
	if (geom)
		geom->scale = newScale;
}

void NifFile::ApplyShapeTranslation(const string& shapeName, const Vector3& offset) {
	int bType;
	int dataRef = shapeDataIdForName(shapeName, bType);
	if (dataRef == -1)
		return;

	NiTriBasedGeom* geom = geomForName(shapeName);
	if (!geom)
		return;

	NiTriBasedGeomData* geomData = dynamic_cast<NiTriBasedGeomData*>(blocks[dataRef]);
	if (!geomData)
		return;

	for (int i = 0; i < geomData->vertices.size(); i++)
		geomData->vertices[i] += geom->translation + offset;

	geom->translation = Vector3(0.0f, 0.0f, 0.0f);
}

void NifFile::MoveVertex(const string& shapeName, const Vector3& pos, const int& id) {
	int bType;
	int dataRef = shapeDataIdForName(shapeName, bType);
	if (dataRef == -1)
		return;

	NiTriBasedGeomData* geomData = dynamic_cast<NiTriBasedGeomData*>(blocks[dataRef]);
	if (geomData && geomData->numVertices > id)
		geomData->vertices[id] = pos;
}

void NifFile::OffsetShape(const string& shapeName, const Vector3& offset, unordered_map<ushort, float>* mask) {
	int bType;
	int dataRef = shapeDataIdForName(shapeName, bType);
	if (dataRef == -1)
		return;

	NiTriBasedGeomData* geomData = dynamic_cast<NiTriBasedGeomData*>(blocks[dataRef]);
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

void NifFile::ScaleShape(const string& shapeName, const float& scale, unordered_map<ushort, float>* mask) {
	int bType;
	int dataRef = shapeDataIdForName(shapeName, bType);
	if (dataRef == -1)
		return;

	NiTriBasedGeomData* geomData = dynamic_cast<NiTriBasedGeomData*>(blocks[dataRef]);
	if (!geomData)
		return;

	Vector3 root;
	GetRootTranslation(root);

	unordered_map<ushort, Vector3> diff;
	for (int i = 0; i < geomData->vertices.size(); i++) {
		Vector3 target = geomData->vertices[i] - root;
		target *= scale;
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

void NifFile::RotateShape(const string& shapeName, const Vector3& angle, unordered_map<ushort, float>* mask) {
	int bType;
	int dataRef = shapeDataIdForName(shapeName, bType);
	if (dataRef == -1)
		return;

	NiTriBasedGeomData* geomData = dynamic_cast<NiTriBasedGeomData*>(blocks[dataRef]);
	if (!geomData)
		return;

	Vector3 root;
	GetRootTranslation(root);

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

void NifFile::GetAlphaForShape(const string& shapeName, ushort& outFlags, byte& outThreshold) {
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

	if (alphaRef == -1)
		return;

	NiAlphaProperty* alpha = dynamic_cast<NiAlphaProperty*>(GetBlock(alphaRef));
	if (!alpha)
		return;

	outFlags = alpha->flags;
	outThreshold = alpha->threshold;
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
	}

	if (alphaRef == -1)
		return;

	NiAlphaProperty* alpha = dynamic_cast<NiAlphaProperty*>(GetBlock(alphaRef));
	if (!alpha)
		return;

	alpha->flags = flags;
	alpha->threshold = (byte)threshold;
}

void NifFile::DeleteShape(const string& shapeName) {
	NiTriBasedGeom* geom = geomForName(shapeName);
	if (geom) {
		DeleteBlock(geom->dataRef);
		if (geom->skinInstanceRef != -1) {
			NiSkinInstance* skinInst = dynamic_cast<NiSkinInstance*>(GetBlock(geom->skinInstanceRef));
			if (skinInst) {
				DeleteBlock(skinInst->dataRef);
				DeleteBlock(skinInst->skinPartitionRef);
				DeleteBlock(geom->skinInstanceRef);
			}
		}

		if (geom->propertiesRef1 != -1) {
			NiShader* shader = dynamic_cast<NiShader*>(GetBlock(geom->propertiesRef1));
			if (shader) {
				DeleteBlock(shader->GetTextureSetRef());
				DeleteBlock(geom->propertiesRef1);
			}
		}
		if (geom->propertiesRef2 != -1)
			DeleteBlock(geom->propertiesRef2);

		for (int i = 0; i < geom->numProperties; i++) {
			NiShader* shader = dynamic_cast<NiShader*>(GetBlock(geom->propertiesRef[i]));
			if (shader) {
				DeleteBlock(shader->GetTextureSetRef());
				DeleteBlock(geom->propertiesRef[i]);
				i--;
			}
			else {
				DeleteBlock(geom->propertiesRef[i]);
				i--;
			}
		}

		for (int i = 0; i < geom->numExtraData; i++)
			DeleteBlock(geom->extraDataRef[i]);
	}

	int shapeID = shapeIdForName(shapeName);
	DeleteBlock(shapeID);
}

void NifFile::DeleteShader(const string& shapeName) {
	NiTriBasedGeom* geom = geomForName(shapeName);
	if (geom) {
		if (geom->propertiesRef1 != -1) {
			NiShader* shader = dynamic_cast<NiShader*>(GetBlock(geom->propertiesRef1));
			if (shader) {
				DeleteBlock(shader->GetTextureSetRef());
				DeleteBlock(geom->propertiesRef1);
			}
		}
		if (geom->propertiesRef2 != -1)
			DeleteBlock(geom->propertiesRef2);

		for (int i = 0; i < geom->numProperties; i++) {
			NiShader* shader = dynamic_cast<NiShader*>(GetBlock(geom->propertiesRef[i]));
			if (shader) {
				if (shader->blockType == BSSHADERPPLIGHTINGPROPERTY || shader->blockType == NIMATERIALPROPERTY) {
					DeleteBlock(shader->GetTextureSetRef());
					DeleteBlock(geom->propertiesRef[i]);
					i--;
					continue;
				}
			}
		}
	}
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
			NiTriBasedGeomData* geomData = dynamic_cast<NiTriBasedGeomData*>(blocks[dataRef]);
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

	if (skinRef == -1)
		return;

	NiSkinInstance* skinInst = dynamic_cast<NiSkinInstance*>(blocks[skinRef]);
	if (skinInst) {
		NiSkinData* skinData = (NiSkinData*)blocks[skinInst->dataRef];
		skinData->notifyVerticesDelete(indices);
		hdr.blockSizes[skinInst->dataRef] = skinData->CalcBlockSize();

		NiSkinPartition* skinPartition = (NiSkinPartition*)blocks[skinInst->skinPartitionRef];
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

		if (!v.IsZero(true))
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
	skinRef = geom->skinInstanceRef;

	if (skinRef == -1)
		return;

	NiSkinData* skinData = nullptr;
	NiSkinPartition* skinPart = nullptr;
	NiSkinInstance* skinInst = dynamic_cast<NiSkinInstance*>(blocks[skinRef]);
	if (skinInst) {
		skinData = (NiSkinData*)blocks[skinInst->dataRef];
		skinPart = (NiSkinPartition*)blocks[skinInst->skinPartitionRef];
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

	NiTriBasedGeom* geom = geomForName(shapeName);
	if (!geom)
		return;

	int skinRef = -1;
	NiTriShapeData* shapeData = nullptr;
	NiTriStripsData* stripsData = nullptr;
	if (bType == NITRISHAPEDATA) {
		skinRef = geom->skinInstanceRef;
		shapeData = (NiTriShapeData*)blocks[geom->dataRef];
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
		}
	}
	else {
		// TO-DO
		return;

		skinRef = geom->skinInstanceRef;
		stripsData = (NiTriStripsData*)blocks[geom->dataRef];
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

	NiSkinData* skinData = nullptr;
	NiSkinPartition* skinPart = nullptr;
	NiSkinInstance* skinInst = dynamic_cast<NiSkinInstance*>(blocks[skinRef]);
	if (skinInst) {
		skinData = (NiSkinData*)blocks[skinInst->dataRef];
		skinPart = (NiSkinPartition*)blocks[skinInst->skinPartitionRef];
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
