#include "NifFile.h"
#include <queue>

NifFile::NifFile() {
	isValid = false;
}

NifFile::NifFile(NifFile& other) {
	CopyFrom(other);
}

NifFile::~NifFile() {
	for (int i = 0; i < blocks.size(); i++)
		delete blocks[i];

	blocks.clear();
}

NiTriShape* NifFile::shapeForName(const string& name, int dupIndex) {
	int n = 0;
	for (int i = 0; i < hdr.numBlocks; i++) {
		if (blocks[i]->blockType == NITRISHAPE) {
			NiTriShape* shape = (NiTriShape*)blocks[i];
			if (!name.compare(shape->name)) {
				if (n >= dupIndex)
					return (shape);
				n++;
			}
		}
	}
	return nullptr;
}

NiTriStrips* NifFile::stripsForName(const string& name, int dupIndex) {
	int n = 0;
	for (int i = 0; i < hdr.numBlocks; i++) {
		if (blocks[i]->blockType == NITRISTRIPS) {
			NiTriStrips* strips = (NiTriStrips*)blocks[i];
			if (!name.compare(strips->name)) {
				if (n >= dupIndex)
					return (strips);
				n++;
			}
		}
	}
	return nullptr;
}

int NifFile::shapeDataIdForName(const string& name, int& outBlockType) {
	for (int i = 0; i < hdr.numBlocks; i++) {
		if (blocks[i]->blockType == NITRISTRIPS) {
			NiTriStrips* strips = (NiTriStrips*)blocks[i];
			if (!name.compare(strips->name)) {
				outBlockType = NITRISTRIPSDATA;
				return strips->dataRef;
			}
		}
		else if (blocks[i]->blockType == NITRISHAPE) {
			NiTriShape* shape = (NiTriShape*)blocks[i];
			if (!name.compare(shape->name)) {
				outBlockType = NITRISHAPEDATA;
				return shape->dataRef;
			}
		}
	}
	return -1;
}

int NifFile::shapeIdForName(const string& name) {
	for (int i = 0; i < hdr.numBlocks; i++) {
		if (blocks[i]->blockType == NITRISHAPE) {
			NiTriShape* shape = (NiTriShape*)blocks[i];
			if (!name.compare(shape->name))
				return (i);
		}
		else if (blocks[i]->blockType == NITRISTRIPS) {
			NiTriStrips* strips = (NiTriStrips*)blocks[i];
			if (!name.compare(strips->name))
				return (i);
		}
	}
	return -1;
}

NiNode* NifFile::nodeForName(const string& name) {
	for (int i = 0; i < hdr.numBlocks; i++) {
		if (blocks[i]->blockType == NINODE) {
			NiNode* node = (NiNode*)blocks[i];
			if (!name.compare(node->name))
				return (node);
		}
	}
	return nullptr;
}

int NifFile::nodeIdForName(const string& name) {
	for (int i = 0; i < hdr.numBlocks; i++) {
		if (blocks[i]->blockType == NINODE) {
			NiNode* node = (NiNode*)blocks[i];
			if (!name.compare(node->name))
				return (i);
		}
	}
	return -1;
}

int NifFile::shapeBoneIndex(const string& shapeName, const string& boneName) {
	int skinRef;
	NiTriShape* shape = shapeForName(shapeName);
	if (!shape) {
		NiTriStrips* strips = stripsForName(shapeName);
		if (!strips)
			return -1;
		skinRef = strips->skinInstanceRef;
	}
	else
		skinRef = shape->skinInstanceRef;

	if (skinRef == -1)
		return -1;

	BSDismemberSkinInstance* bsdSkinInst = dynamic_cast<BSDismemberSkinInstance*>(blocks[skinRef]);
	if (!bsdSkinInst) {
		NiSkinInstance* niSkinInst = dynamic_cast<NiSkinInstance*>(blocks[skinRef]);
		if (!niSkinInst)
			return -1;

		for (int i = 0; i < niSkinInst->bones.size(); i++) {
			int boneBlock = niSkinInst->bones[i];
			if (boneBlock != -1) {
				NiNode* node = (NiNode*)blocks[boneBlock];
				if (node->name == boneName)
					return i;
			}
		}
	}
	else {
		for (int i = 0; i < bsdSkinInst->bones.size(); i++) {
			int boneBlock = bsdSkinInst->bones[i];
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
		}

		if (blockCopy) {
			blockCopy->header = &hdr;
			blocks.push_back(blockCopy);
		}
	}
	this->hdr.blocks = &blocks;
}

void NifFile::Clear() {
	for (int i = 0; i < blocks.size(); i++)
		delete blocks[i];

	blocks.clear();
	hdr.Clear();
	isValid = false;
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
			else
				block = (NiObject*) new NiUnknown(file, hdr.blockSizes[i]);

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

		NiTriShape* shape = shapeForName(shapeName);
		if (shape) {
			shape->extraDataRef.push_back(strExtraDataId);
			shape->numExtraData++;
			shape->SetBlockSize(shape->CalcBlockSize() + 4);
			hdr.blockSizes[id] = shape->CalcBlockSize();
		}
		else {
			NiTriStrips* strips = stripsForName(shapeName);
			if (!strips)
				return -1;
			strips->extraDataRef.push_back(strExtraDataId);
			strips->numExtraData++;
			strips->SetBlockSize(strips->CalcBlockSize() + 4);
			hdr.blockSizes[id] = strips->CalcBlockSize();
		}
	}
	return strExtraDataId;
}

BSLightingShaderProperty* NifFile::GetShaderForShape(const string& shapeName) {
	NiTriShape* shape = shapeForName(shapeName);
	int prop1;
	if (!shape) {
		NiTriStrips* strips = stripsForName(shapeName);
		if (!strips)
			return nullptr;
		prop1 = strips->propertiesRef1;
	}
	else {
		prop1 = shape->propertiesRef1;
	}
	if (prop1 == -1)
		return nullptr;

	BSLightingShaderProperty* shader = dynamic_cast<BSLightingShaderProperty*>(blocks[prop1]);
	return shader;
}

BSShaderPPLightingProperty* NifFile::GetShaderPPForShape(const string& shapeName) {
	NiTriShape* shape = shapeForName(shapeName);
	vector<int> props;
	if (!shape) {
		NiTriStrips* strips = stripsForName(shapeName);
		if (!strips)
			return nullptr;
		props = strips->propertiesRef;
	}
	else {
		props = shape->propertiesRef;
	}
	if (props.empty())
		return nullptr;

	for (int i = 0; i < props.size(); i++) {
		BSShaderPPLightingProperty* shader = dynamic_cast<BSShaderPPLightingProperty*>(blocks[props[i]]);
		if (shader)
			return shader;
	}
	return nullptr;
}

bool NifFile::GetTextureForShape(const string& shapeName, string& outTexFile, int texIndex) {
	BSShaderTextureSet* ts = nullptr;
	BSLightingShaderProperty* shader = GetShaderForShape(shapeName);
	if (!shader) {
		BSShaderPPLightingProperty* shaderPP = GetShaderPPForShape(shapeName);
		if (!shaderPP || shaderPP->textureSetRef == -1)
			return false;

		ts = dynamic_cast<BSShaderTextureSet*>(blocks[shaderPP->textureSetRef]);
	}
	else {
		if (shader->textureSetRef == -1)
			return false;

		ts = dynamic_cast<BSShaderTextureSet*>(blocks[shader->textureSetRef]);
	}

	if (!ts || texIndex + 1 > ts->numTextures)
		return false;

	outTexFile = ts->textures[texIndex].str;
	return true;
}

void NifFile::SetTextureForShape(const string& shapeName, string& outTexFile, int texIndex) {
	BSShaderTextureSet* ts = nullptr;
	BSLightingShaderProperty* shader = GetShaderForShape(shapeName);
	if (!shader) {
		BSShaderPPLightingProperty* shaderPP = GetShaderPPForShape(shapeName);
		if (!shaderPP || shaderPP->textureSetRef == -1)
			return;

		ts = dynamic_cast<BSShaderTextureSet*>(blocks[shaderPP->textureSetRef]);
		if (!ts || texIndex + 1 > ts->numTextures)
			return;

		ts->textures[texIndex].str = outTexFile;
		hdr.blockSizes[shaderPP->textureSetRef] = ts->CalcBlockSize();
	}
	else {
		if (shader->textureSetRef == -1)
			return;

		ts = dynamic_cast<BSShaderTextureSet*>(blocks[shader->textureSetRef]);
		if (!ts || texIndex + 1 > ts->numTextures)
			return;

		ts->textures[texIndex].str = outTexFile;
		hdr.blockSizes[shader->textureSetRef] = ts->CalcBlockSize();
	}
}

void NifFile::TrimTexturePaths() {
	string tFile;
	vector<string> shapes;
	GetShapeList(shapes);

	for (auto &s : shapes) {
		for (int i = 0; i < 9; i++) {
			if (GetTextureForShape(s, tFile, i) && !tFile.empty()) {
				tFile = regex_replace(tFile, regex("/+|\\\\+"), "\\"); // Replace multiple slashes or forward slashes with one backslash
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

void NifFile::CopyShader(const string& shapeDest, BSLightingShaderProperty* srcShader, NifFile& srcNif, bool addAlpha) {
	int dataRef;
	int* props1 = nullptr;
	int* props2 = nullptr;
	bool isStrips;

	NiTriShape* shape = shapeForName(shapeDest);
	if (!shape) {
		NiTriStrips* strips = stripsForName(shapeDest);
		if (!strips)
			return;

		props1 = &strips->propertiesRef1;
		props2 = &strips->propertiesRef2;
		isStrips = true;
		dataRef = strips->dataRef;
	}
	else {
		props1 = &shape->propertiesRef1;
		props2 = &shape->propertiesRef2;
		isStrips = false;
		dataRef = shape->dataRef;
	}

	// Create destination shader block and copy
	BSLightingShaderProperty* destShader = new BSLightingShaderProperty((*srcShader));
	destShader->header = &hdr;
	destShader->nameRef = 0xFFFFFFFF;

	// Add shader block to nif
	int shaderId = blocks.size();
	blocks.push_back(destShader);
	hdr.numBlocks++;

	// Texture Set
	BSShaderTextureSet* destTexSet = nullptr;
	BSShaderTextureSet* srcTexSet = dynamic_cast<BSShaderTextureSet*>(srcNif.GetBlock(srcShader->textureSetRef));
	bool texSetFound = false;
	if (srcTexSet) {
		// Create texture set block and copy
		texSetFound = true;
		destTexSet = new BSShaderTextureSet((*srcTexSet));
		destTexSet->header = &hdr;

		// Add texture block to nif
		int texsetId = blocks.size();
		blocks.push_back(destTexSet);
		hdr.numBlocks++;

		// Assign texture set block id to shader
		destShader->textureSetRef = texsetId;
	}

	// Controller
	NiUnknown* destController = nullptr;
	NiUnknown* srcController = dynamic_cast<NiUnknown*>(srcNif.GetBlock(srcShader->controllerRef));
	bool controllerFound = false;
	if (srcController) {
		controllerFound = true;
		destController = new NiUnknown(srcController->CalcBlockSize());
		destController->Clone(srcController);
		int controllerId = blocks.size();
		blocks.push_back(destController);
		hdr.numBlocks++;
		destShader->controllerRef = controllerId;
	}

	(*props1) = shaderId;

	// Kill normals, set numUVSets to 1
	if (destShader->IsSkinShader() && hdr.userVersion >= 12) {
		if (isStrips) {
			NiTriStripsData* stripsData = ((NiTriStripsData*)blocks[dataRef]);
			stripsData->hasNormals = 0;
			stripsData->normals.clear();
			stripsData->bitangents.clear();
			stripsData->tangents.clear();
			stripsData->numUVSets = 1;
			hdr.blockSizes[dataRef] = stripsData->CalcBlockSize();
		}
		else {
			NiTriShapeData* shapeData = ((NiTriShapeData*)blocks[dataRef]);
			shapeData->hasNormals = 0;
			shapeData->normals.clear();
			shapeData->bitangents.clear();
			shapeData->tangents.clear();
			shapeData->numUVSets = 1;
			hdr.blockSizes[dataRef] = shapeData->CalcBlockSize();
		}
	}

	// Add shader and other sizes to block size info
	hdr.blockSizes.push_back(destShader->CalcBlockSize());

	if (texSetFound)
		hdr.blockSizes.push_back(destTexSet->CalcBlockSize());

	if (controllerFound)
		hdr.blockSizes.push_back(destController->CalcBlockSize());

	if (addAlpha) {
		NiAlphaProperty* alphaProp = new NiAlphaProperty(hdr);
		(*props2) = blocks.size();
		blocks.push_back(alphaProp);
		hdr.numBlocks++;
		hdr.blockSizes.push_back(alphaProp->CalcBlockSize());
	}

	// Record the block type in the block type index.
	ushort shaderTypeId = AddOrFindBlockTypeId("BSLightingShaderProperty");
	hdr.blockIndex.push_back(shaderTypeId);

	if (texSetFound) {
		ushort texSetTypeId = AddOrFindBlockTypeId("BSShaderTextureSet");
		hdr.blockIndex.push_back(texSetTypeId);
	}

	if (controllerFound) {
		ushort controllerTypeId = AddOrFindBlockTypeId(srcNif.hdr.blockTypes[srcNif.hdr.blockIndex[srcShader->controllerRef]].str);
		hdr.blockIndex.push_back(controllerTypeId);
	}

	if (addAlpha) {
		ushort alphaBlockTypeId = AddOrFindBlockTypeId("NiAlphaProperty");
		hdr.blockIndex.push_back(alphaBlockTypeId);
	}
}

void NifFile::CopyShaderPP(const string& shapeDest, BSShaderPPLightingProperty* srcShader, NifFile& srcNif, bool addAlpha) {
	int dataRef;
	int* props1 = nullptr;
	int* props2 = nullptr;
	bool isStrips;

	NiTriShape* shape = shapeForName(shapeDest);
	if (!shape) {
		NiTriStrips* strips = stripsForName(shapeDest);
		if (!strips)
			return;

		BSShaderPPLightingProperty* shaderPP = nullptr;
		NiAlphaProperty* alpha = nullptr;
		for (int i = 0; i < strips->numProperties; i++) {
			if (!shaderPP) {
				shaderPP = dynamic_cast<BSShaderPPLightingProperty*>(srcNif.GetBlock(strips->propertiesRef[i]));
				if (shaderPP)
					props1 = &strips->propertiesRef[i];
			}

			if (!alpha) {
				alpha = dynamic_cast<NiAlphaProperty*>(srcNif.GetBlock(strips->propertiesRef[i]));
				if (alpha)
					props2 = &strips->propertiesRef[i];
			}
		}
		isStrips = true;
		dataRef = strips->dataRef;
	}
	else {
		BSShaderPPLightingProperty* shaderPP = nullptr;
		NiAlphaProperty* alpha = nullptr;
		for (int i = 0; i < shape->numProperties; i++) {
			if (!shaderPP) {
				shaderPP = dynamic_cast<BSShaderPPLightingProperty*>(srcNif.GetBlock(shape->propertiesRef[i]));
				if (shaderPP)
					props1 = &shape->propertiesRef[i];
			}

			if (!alpha) {
				alpha = dynamic_cast<NiAlphaProperty*>(srcNif.GetBlock(shape->propertiesRef[i]));
				if (alpha)
					props2 = &shape->propertiesRef[i];
			}
		}
		isStrips = false;
		dataRef = shape->dataRef;
	}

	// Create destination shader block and copy
	BSShaderPPLightingProperty* destShader = new BSShaderPPLightingProperty((*srcShader));
	destShader->header = &hdr;
	destShader->nameRef = 0xFFFFFFFF;

	// Add shader block to nif
	int shaderId = blocks.size();
	blocks.push_back(destShader);
	hdr.numBlocks++;

	// Texture Set
	BSShaderTextureSet* destTexSet = nullptr;
	BSShaderTextureSet* srcTexSet = dynamic_cast<BSShaderTextureSet*>(srcNif.GetBlock(srcShader->textureSetRef));
	bool texSetFound = false;
	if (srcTexSet) {
		// Create texture set block and copy
		texSetFound = true;
		destTexSet = new BSShaderTextureSet((*srcTexSet));
		destTexSet->header = &hdr;

		// Add texture block to nif
		int texsetId = blocks.size();
		blocks.push_back(destTexSet);
		hdr.numBlocks++;

		// Assign texture set block id to shader
		destShader->textureSetRef = texsetId;
	}

	// Controller
	NiUnknown* destController = nullptr;
	NiUnknown* srcController = dynamic_cast<NiUnknown*>(srcNif.GetBlock(srcShader->controllerRef));
	bool controllerFound = false;
	if (srcController) {
		controllerFound = true;
		destController = new NiUnknown(srcController->CalcBlockSize());
		destController->Clone(srcController);
		int controllerId = blocks.size();
		blocks.push_back(destController);
		hdr.numBlocks++;
		destShader->controllerRef = controllerId;
	}

	(*props1) = shaderId;

	// Kill normals, set numUVSets to 1
	if (destShader->IsSkinShader() && hdr.userVersion >= 12) {
		if (isStrips) {
			NiTriStripsData* stripsData = ((NiTriStripsData*)blocks[dataRef]);
			stripsData->hasNormals = 0;
			stripsData->normals.clear();
			stripsData->bitangents.clear();
			stripsData->tangents.clear();
			stripsData->numUVSets = 1;
			hdr.blockSizes[dataRef] = stripsData->CalcBlockSize();
		}
		else {
			NiTriShapeData* shapeData = ((NiTriShapeData*)blocks[dataRef]);
			shapeData->hasNormals = 0;
			shapeData->normals.clear();
			shapeData->bitangents.clear();
			shapeData->tangents.clear();
			shapeData->numUVSets = 1;
			hdr.blockSizes[dataRef] = shapeData->CalcBlockSize();
		}
	}

	// Add shader and other sizes to block size info
	hdr.blockSizes.push_back(destShader->CalcBlockSize());

	if (texSetFound)
		hdr.blockSizes.push_back(destTexSet->CalcBlockSize());

	if (controllerFound)
		hdr.blockSizes.push_back(destController->CalcBlockSize());

	if (addAlpha) {
		NiAlphaProperty* alphaProp = new NiAlphaProperty(hdr);
		(*props2) = blocks.size();
		blocks.push_back(alphaProp);
		hdr.numBlocks++;
		hdr.blockSizes.push_back(alphaProp->CalcBlockSize());
	}

	// Record the block type in the block type index.
	ushort shaderTypeId = AddOrFindBlockTypeId("BSShaderPPLightingProperty");
	hdr.blockIndex.push_back(shaderTypeId);

	if (texSetFound) {
		ushort texSetTypeId = AddOrFindBlockTypeId("BSShaderTextureSet");
		hdr.blockIndex.push_back(texSetTypeId);
	}

	if (controllerFound) {
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

void NifFile::CopyStrips(const string& shapeDest, NifFile& srcNif, const string& srcShape) {
	NiTriStrips* src = srcNif.stripsForName(srcShape);
	if (!src)
		return;

	NiTriStripsData* srcData = (NiTriStripsData*)srcNif.GetBlock(src->dataRef);
	BSDismemberSkinInstance* srcBSDSkinInst = nullptr;
	NiSkinInstance* srcNiSkinInst = nullptr;
	NiSkinData* srcSkinData = nullptr;
	NiSkinPartition* srcSkinPart = nullptr;

	BSDismemberSkinInstance* destBSDSkinInst = nullptr;
	NiSkinInstance* destNiSkinInst = nullptr;
	NiSkinData* destSkinData = nullptr;
	NiSkinPartition* destSkinPart = nullptr;

	bool skipSkinInst = false;
	if (src->skinInstanceRef == -1)
		skipSkinInst = true;

	bool skipBSDSkinInst = false;
	if (!skipSkinInst) {
		srcBSDSkinInst = dynamic_cast<BSDismemberSkinInstance*>(srcNif.GetBlock(src->skinInstanceRef));
		if (!srcBSDSkinInst) {
			skipBSDSkinInst = true;
			srcNiSkinInst = dynamic_cast<NiSkinInstance*>(srcNif.GetBlock(src->skinInstanceRef));
			if (!srcNiSkinInst)
				skipSkinInst = true;

			if (!skipSkinInst) {
				srcSkinData = (NiSkinData*)srcNif.GetBlock(srcNiSkinInst->dataRef);
				srcSkinPart = (NiSkinPartition*)srcNif.GetBlock(srcNiSkinInst->skinPartitionRef);
				destNiSkinInst = new NiSkinInstance((*srcNiSkinInst));
				destNiSkinInst->header = &hdr;
			}
		}
		else {
			srcSkinData = (NiSkinData*)srcNif.GetBlock(srcBSDSkinInst->dataRef);
			srcSkinPart = (NiSkinPartition*)srcNif.GetBlock(srcBSDSkinInst->skinPartitionRef);
			destBSDSkinInst = new BSDismemberSkinInstance((*srcBSDSkinInst));
			destBSDSkinInst->header = &hdr;
		}
	}

	NiTriStrips* dest = new NiTriStrips((*src));
	dest->header = &hdr;
	dest->nameRef = AddOrFindStringId(shapeDest);
	dest->name = shapeDest;

	NiTriStripsData* destData = new NiTriStripsData((*srcData));
	destData->header = &hdr;

	if (!skipSkinInst) {
		// Treat skinning and partition info as blobs of anonymous data.
		destSkinData = new NiSkinData((*srcSkinData));
		destSkinData->header = &hdr;
		destSkinPart = new NiSkinPartition((*srcSkinPart));
		destSkinPart->header = &hdr;
	}

	int destId = blocks.size();
	blocks.push_back(dest);
	hdr.numBlocks++;

	int destDataId = blocks.size();
	blocks.push_back(destData);
	hdr.numBlocks++;

	int destSkinId = -1;
	int destSkinDataId = -1;
	int destSkinPartId = -1;
	if (!skipSkinInst) {
		destSkinId = blocks.size();
		if (skipBSDSkinInst)
			blocks.push_back(destNiSkinInst);
		else
			blocks.push_back(destBSDSkinInst);
		hdr.numBlocks++;

		destSkinDataId = blocks.size();
		blocks.push_back(destSkinData);
		hdr.numBlocks++;

		destSkinPartId = blocks.size();
		blocks.push_back(destSkinPart);
		hdr.numBlocks++;
	}

	dest->dataRef = destDataId;
	dest->skinInstanceRef = destSkinId;

	if (!skipSkinInst) {
		if (skipBSDSkinInst) {
			destNiSkinInst->dataRef = destSkinDataId;
			destNiSkinInst->skinPartitionRef = destSkinPartId;
		}
		else {
			destBSDSkinInst->dataRef = destSkinDataId;
			destBSDSkinInst->skinPartitionRef = destSkinPartId;
		}
	}

	hdr.blockIndex.push_back(AddOrFindBlockTypeId("NiTriStrips"));
	hdr.blockIndex.push_back(AddOrFindBlockTypeId("NiTriStripsData"));
	if (!skipSkinInst) {
		if (skipBSDSkinInst)
			hdr.blockIndex.push_back(AddOrFindBlockTypeId("NiSkinInstance"));
		else
			hdr.blockIndex.push_back(AddOrFindBlockTypeId("BSDismemberSkinInstance"));
		hdr.blockIndex.push_back(AddOrFindBlockTypeId("NiSkinData"));
		hdr.blockIndex.push_back(AddOrFindBlockTypeId("NiSkinPartition"));
	}

	hdr.blockSizes.push_back(dest->CalcBlockSize());
	hdr.blockSizes.push_back(destData->CalcBlockSize());
	if (!skipSkinInst) {
		if (skipBSDSkinInst)
			hdr.blockSizes.push_back(destNiSkinInst->CalcBlockSize());
		else
			hdr.blockSizes.push_back(destBSDSkinInst->CalcBlockSize());
		hdr.blockSizes.push_back(destSkinData->CalcBlockSize());
		hdr.blockSizes.push_back(destSkinPart->CalcBlockSize());
	}

	bool shaderAlpha = false;
	BSLightingShaderProperty* srcShader = dynamic_cast<BSLightingShaderProperty*>(srcNif.GetBlock(src->propertiesRef1));
	if (!srcShader) {
		BSShaderPPLightingProperty* srcShaderPP = nullptr;
		for (int i = 0; i < src->numProperties; i++) {
			if (!srcShaderPP) {
				srcShaderPP = dynamic_cast<BSShaderPPLightingProperty*>(srcNif.GetBlock(src->propertiesRef[i]));
				if (srcShaderPP)
					continue;
			}
			if (!shaderAlpha) {
				NiAlphaProperty* alpha = dynamic_cast<NiAlphaProperty*>(srcNif.GetBlock(src->propertiesRef[i]));
				if (alpha) {
					shaderAlpha = true;
					continue;
				}
			}

			NiMaterialProperty* material = dynamic_cast<NiMaterialProperty*>(srcNif.GetBlock(src->propertiesRef[i]));
			if (material) {
				NiMaterialProperty* destMaterial = new NiMaterialProperty((*material));
				destMaterial->header = &hdr;
				int materialId = blocks.size();
				blocks.push_back(destMaterial);
				hdr.numBlocks++;
				hdr.blockSizes.push_back(destMaterial->CalcBlockSize());
				hdr.blockIndex.push_back(AddOrFindBlockTypeId("NiMaterialProperty"));
				dest->propertiesRef[i] = materialId;
				continue;
			}

			NiStencilProperty* stencil = dynamic_cast<NiStencilProperty*>(srcNif.GetBlock(src->propertiesRef[i]));
			if (stencil) {
				NiStencilProperty* destStencil = new NiStencilProperty((*stencil));
				destStencil->header = &hdr;
				int stencilId = blocks.size();
				blocks.push_back(destStencil);
				hdr.numBlocks++;
				hdr.blockSizes.push_back(destStencil->CalcBlockSize());
				hdr.blockIndex.push_back(AddOrFindBlockTypeId("NiStencilProperty"));
				dest->propertiesRef[i] = stencilId;
				continue;
			}

			NiUnknown* srcUnknown = dynamic_cast<NiUnknown*>(srcNif.GetBlock(src->propertiesRef[i]));
			if (srcUnknown) {
				NiUnknown* destUnknown = new NiUnknown(srcUnknown->CalcBlockSize());
				destUnknown->Clone(srcUnknown);
				int unknownId = blocks.size();
				blocks.push_back(destUnknown);
				hdr.numBlocks++;
				hdr.blockSizes.push_back(destUnknown->CalcBlockSize());
				hdr.blockIndex.push_back(AddOrFindBlockTypeId(srcNif.hdr.blockTypes[srcNif.hdr.blockIndex[src->propertiesRef[i]]].str));
				dest->propertiesRef[i] = unknownId;
			}
		}
		if (srcShaderPP)
			CopyShaderPP(shapeDest, srcShaderPP, srcNif, shaderAlpha);
		else
			dest->propertiesRef1 = -1;
	}
	else {
		if (src->propertiesRef2 != -1)
			shaderAlpha = true;
		CopyShader(shapeDest, srcShader, srcNif, shaderAlpha);
	}

	vector<string> srcBoneList;
	srcNif.GetShapeBoneList(srcShape, srcBoneList);
	if (!skipSkinInst) {
		if (skipBSDSkinInst)
			destNiSkinInst->bones.clear();
		else
			destBSDSkinInst->bones.clear();
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
		if (skipBSDSkinInst)
			destNiSkinInst->bones.push_back(bonePtr);
		else
			destBSDSkinInst->bones.push_back(bonePtr);
	}

	rootNode->children.push_back(destId);
	rootNode->numChildren++;
	hdr.blockSizes[0] = rootNode->CalcBlockSize();
}

void NifFile::CopyShape(const string& shapeDest, NifFile& srcNif, const string& srcShape) {
	NiTriShape* src = srcNif.shapeForName(srcShape);
	if (!src) {
		CopyStrips(shapeDest, srcNif, srcShape);
		return;
	}

	NiTriShapeData* srcData = (NiTriShapeData*)srcNif.GetBlock(src->dataRef);
	BSDismemberSkinInstance* srcBSDSkinInst = nullptr;
	NiSkinInstance* srcNiSkinInst = nullptr;
	NiSkinData* srcSkinData = nullptr;
	NiSkinPartition* srcSkinPart = nullptr;

	BSDismemberSkinInstance* destBSDSkinInst = nullptr;
	NiSkinInstance* destNiSkinInst = nullptr;
	NiSkinData* destSkinData = nullptr;
	NiSkinPartition* destSkinPart = nullptr;

	bool skipSkinInst = false;
	if (src->skinInstanceRef == -1)
		skipSkinInst = true;

	bool skipBSDSkinInst = false;
	if (!skipSkinInst) {
		srcBSDSkinInst = dynamic_cast<BSDismemberSkinInstance*>(srcNif.GetBlock(src->skinInstanceRef));
		if (!srcBSDSkinInst) {
			skipBSDSkinInst = true;
			srcNiSkinInst = dynamic_cast<NiSkinInstance*>(srcNif.GetBlock(src->skinInstanceRef));
			if (!srcNiSkinInst)
				skipSkinInst = true;

			if (!skipSkinInst) {
				srcSkinData = (NiSkinData*)srcNif.GetBlock(srcNiSkinInst->dataRef);
				srcSkinPart = (NiSkinPartition*)srcNif.GetBlock(srcNiSkinInst->skinPartitionRef);
				destNiSkinInst = new NiSkinInstance((*srcNiSkinInst));
				destNiSkinInst->header = &hdr;
			}
		}
		else {
			srcSkinData = (NiSkinData*)srcNif.GetBlock(srcBSDSkinInst->dataRef);
			srcSkinPart = (NiSkinPartition*)srcNif.GetBlock(srcBSDSkinInst->skinPartitionRef);
			destBSDSkinInst = new BSDismemberSkinInstance((*srcBSDSkinInst));
			destBSDSkinInst->header = &hdr;
		}
	}

	NiTriShape* dest = new NiTriShape((*src));
	dest->header = &hdr;
	dest->nameRef = AddOrFindStringId(shapeDest);
	dest->name = shapeDest;

	NiTriShapeData* destData = new NiTriShapeData((*srcData));
	destData->header = &hdr;

	if (!skipSkinInst) {
		// Treat skinning and partition info as blobs of anonymous data.
		destSkinData = new NiSkinData((*srcSkinData));
		destSkinData->header = &hdr;
		destSkinPart = new NiSkinPartition((*srcSkinPart));
		destSkinPart->header = &hdr;
	}

	int destId = blocks.size();
	blocks.push_back(dest);
	hdr.numBlocks++;

	int destDataId = blocks.size();
	blocks.push_back(destData);
	hdr.numBlocks++;

	int destSkinId = -1;
	int destSkinDataId = -1;
	int destSkinPartId = -1;
	if (!skipSkinInst) {
		destSkinId = blocks.size();
		if (skipBSDSkinInst)
			blocks.push_back(destNiSkinInst);
		else
			blocks.push_back(destBSDSkinInst);
		hdr.numBlocks++;

		destSkinDataId = blocks.size();
		blocks.push_back(destSkinData);
		hdr.numBlocks++;

		destSkinPartId = blocks.size();
		blocks.push_back(destSkinPart);
		hdr.numBlocks++;
	}

	dest->dataRef = destDataId;
	dest->skinInstanceRef = destSkinId;

	if (!skipSkinInst) {
		if (skipBSDSkinInst) {
			destNiSkinInst->dataRef = destSkinDataId;
			destNiSkinInst->skinPartitionRef = destSkinPartId;
		}
		else {
			destBSDSkinInst->dataRef = destSkinDataId;
			destBSDSkinInst->skinPartitionRef = destSkinPartId;
		}
	}

	hdr.blockIndex.push_back(AddOrFindBlockTypeId("NiTriShape"));
	hdr.blockIndex.push_back(AddOrFindBlockTypeId("NiTriShapeData"));
	if (!skipSkinInst) {
		if (skipBSDSkinInst)
			hdr.blockIndex.push_back(AddOrFindBlockTypeId("NiSkinInstance"));
		else
			hdr.blockIndex.push_back(AddOrFindBlockTypeId("BSDismemberSkinInstance"));
		hdr.blockIndex.push_back(AddOrFindBlockTypeId("NiSkinData"));
		hdr.blockIndex.push_back(AddOrFindBlockTypeId("NiSkinPartition"));
	}

	hdr.blockSizes.push_back(dest->CalcBlockSize());
	hdr.blockSizes.push_back(destData->CalcBlockSize());
	if (!skipSkinInst) {
		if (skipBSDSkinInst)
			hdr.blockSizes.push_back(destNiSkinInst->CalcBlockSize());
		else
			hdr.blockSizes.push_back(destBSDSkinInst->CalcBlockSize());
		hdr.blockSizes.push_back(destSkinData->CalcBlockSize());
		hdr.blockSizes.push_back(destSkinPart->CalcBlockSize());
	}

	bool shaderAlpha = false;
	BSLightingShaderProperty* srcShader = dynamic_cast<BSLightingShaderProperty*>(srcNif.GetBlock(src->propertiesRef1));
	if (!srcShader) {
		BSShaderPPLightingProperty* srcShaderPP = nullptr;
		for (int i = 0; i < src->numProperties; i++) {
			if (!srcShaderPP) {
				srcShaderPP = dynamic_cast<BSShaderPPLightingProperty*>(srcNif.GetBlock(src->propertiesRef[i]));
				if (srcShaderPP)
					continue;
			}
			if (!shaderAlpha) {
				NiAlphaProperty* alpha = dynamic_cast<NiAlphaProperty*>(srcNif.GetBlock(src->propertiesRef[i]));
				if (alpha) {
					shaderAlpha = true;
					continue;
				}
			}

			NiMaterialProperty* material = dynamic_cast<NiMaterialProperty*>(srcNif.GetBlock(src->propertiesRef[i]));
			if (material) {
				NiMaterialProperty* destMaterial = new NiMaterialProperty((*material));
				destMaterial->header = &hdr;
				int materialId = blocks.size();
				blocks.push_back(destMaterial);
				hdr.numBlocks++;
				hdr.blockSizes.push_back(destMaterial->CalcBlockSize());
				hdr.blockIndex.push_back(AddOrFindBlockTypeId("NiMaterialProperty"));
				dest->propertiesRef[i] = materialId;
				continue;
			}

			NiStencilProperty* stencil = dynamic_cast<NiStencilProperty*>(srcNif.GetBlock(src->propertiesRef[i]));
			if (stencil) {
				NiStencilProperty* destStencil = new NiStencilProperty((*stencil));
				destStencil->header = &hdr;
				int stencilId = blocks.size();
				blocks.push_back(destStencil);
				hdr.numBlocks++;
				hdr.blockSizes.push_back(destStencil->CalcBlockSize());
				hdr.blockIndex.push_back(AddOrFindBlockTypeId("NiStencilProperty"));
				dest->propertiesRef[i] = stencilId;
				continue;
			}

			NiUnknown* srcUnknown = dynamic_cast<NiUnknown*>(srcNif.GetBlock(src->propertiesRef[i]));
			if (srcUnknown) {
				NiUnknown* destUnknown = new NiUnknown(srcUnknown->CalcBlockSize());
				destUnknown->Clone(srcUnknown);
				int unknownId = blocks.size();
				blocks.push_back(destUnknown);
				hdr.numBlocks++;
				hdr.blockSizes.push_back(destUnknown->CalcBlockSize());
				hdr.blockIndex.push_back(AddOrFindBlockTypeId(srcNif.hdr.blockTypes[srcNif.hdr.blockIndex[src->propertiesRef[i]]].str));
				dest->propertiesRef[i] = unknownId;
			}
		}
		if (srcShaderPP)
			CopyShaderPP(shapeDest, srcShaderPP, srcNif, shaderAlpha);
		else
			dest->propertiesRef1 = -1;
	}
	else {
		if (src->propertiesRef2 != -1)
			shaderAlpha = true;
		CopyShader(shapeDest, srcShader, srcNif, shaderAlpha);
	}

	vector<string> srcBoneList;
	srcNif.GetShapeBoneList(srcShape, srcBoneList);
	if (!skipSkinInst) {
		if (skipBSDSkinInst)
			destNiSkinInst->bones.clear();
		else
			destBSDSkinInst->bones.clear();
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
		if (skipBSDSkinInst)
			destNiSkinInst->bones.push_back(bonePtr);
		else
			destBSDSkinInst->bones.push_back(bonePtr);
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
	} else
		return 1;

	return 0;
}

int NifFile::ExportShapeObj(const string& filename, const string& shape, float scale, Vector3 offset) {
	ofstream file(filename.c_str(), ios_base::binary);
	if (file.fail())
		return 1;

	const vector<Vector3>* verts = GetRawVertsForShape(shape);
	const vector<Triangle>* tris = GetTrisForShape(shape);
	const vector<Vector2>* uvs = GetUvsForShape(shape);

	vector<Triangle> stripTris;
	if (!tris)
		GetTrisForTriStripShape(shape, &stripTris);

	Vector3 shapeTrans;
	GetShapeTranslation(shape, shapeTrans);

	float xtrans = shapeTrans.x + offset.x;
	float ytrans = shapeTrans.y + offset.y;
	float ztrans = shapeTrans.z + offset.z;

	file << "# Shape export from Caliente's Body Slide" << endl;
	file << "# original nif: " << fileName << endl;
	file << "# translation: " << xtrans << ", " << ytrans << ", " << ztrans;
	file << "# scale: " << scale << endl << endl;

	file << "g " << shape << endl;
	file << "usemtl NoMaterial" << endl << endl;

	for (int i = 0; i < verts->size(); i++) {
		file << "v " << (verts->at(i).x + xtrans) * scale
			<< " " << (verts->at(i).y + ytrans) * scale
			<< " " << (verts->at(i).z + ztrans) * scale
			<< endl;
	}
	file << endl;
	for (int i = 0; i < uvs->size(); i++) {
		file << "vt " << uvs->at(i).u << " " << (1.0f - uvs->at(i).v) << endl;
	}
	file << endl;

	// NiTriShapeData
	if (tris) {
		for (int i = 0; i < tris->size(); i++) {
			file << "f " << tris->at(i).p1 + 1 << "/" << tris->at(i).p1 + 1 << " "
				<< tris->at(i).p2 + 1 << "/" << tris->at(i).p2 + 1 << " "
				<< tris->at(i).p3 + 1 << "/" << tris->at(i).p3 + 1
				<< endl;
		}
	}
	// NiTriStripsData
	else {
		for (int i = 0; i < stripTris.size(); i++) {
			file << "f " << stripTris.at(i).p1 + 1 << "/" << stripTris.at(i).p1 + 1 << " "
				<< stripTris.at(i).p2 + 1 << "/" << stripTris.at(i).p2 + 1 << " "
				<< stripTris.at(i).p3 + 1 << "/" << stripTris.at(i).p3 + 1
				<< endl;
		}
	}
	file << endl;

	file.close();
	return 0;
}

int NifFile::GetShapeList(vector<string>& outList) {
	outList.clear();
	for (int i = 0; i < hdr.numBlocks; i++) {
		if (blocks[i]->blockType == NITRISHAPE) {
			NiTriShape* shape = (NiTriShape*)blocks[i];
			outList.push_back(shape->name);
		}
		else if (blocks[i]->blockType == NITRISTRIPS) {
			NiTriStrips* strips = (NiTriStrips*)blocks[i];
			outList.push_back(strips->name);
		}
	}
	return outList.size();
}

void NifFile::RenameShape(const string& oldName, const string& newName) {
	uint strID;
	NiTriShape* shape = shapeForName(oldName);
	if (shape) {
		strID = shape->nameRef;
		shape->name = newName;
	}
	else {
		NiTriStrips* strips = stripsForName(oldName);
		if (!strips)
			return;
		strID = strips->nameRef;
		strips->name = newName;
	}

	hdr.strings[strID].str = newName;
	if (hdr.maxStringLen < newName.length())
		hdr.maxStringLen = newName.length();
}

void NifFile::RenameDuplicateShape(const string& dupedShape) {
	NiTriShape* shape = shapeForName(dupedShape);
	int dupCount = 1;
	char buf[10];
	if (shape) {
		while ((shape = shapeForName(dupedShape, 1)) != nullptr) {
			_snprintf(buf, 10, "_%d", dupCount);
			while (FindStringId(shape->name + buf) != -1) {
				dupCount++;
				_snprintf(buf, 10, "_%d", dupCount);
			}
			shape->name = shape->name + buf;
			shape->nameRef = AddOrFindStringId(shape->name);
			dupCount++;
		}
	}
	else {
		NiTriStrips* strips = stripsForName(dupedShape);
		if (!strips)
			return;

		while ((strips = stripsForName(dupedShape, 1)) != nullptr) {
			_snprintf(buf, 10, "_%d", dupCount);
			while (FindStringId(strips->name + buf) != -1) {
				dupCount++;
				_snprintf(buf, 10, "_%d", dupCount);
			}
			strips->name = strips->name + buf;
			strips->nameRef = AddOrFindStringId(strips->name);
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

int NifFile::GetShaderList(vector<string>& outList) {
	outList.clear();
	for (int i = 0; i < hdr.numBlocks; i++) {
		if (blocks[i]->blockType == BSLIGHTINGSHADERPROPERTY) {
			BSLightingShaderProperty* shader = dynamic_cast<BSLightingShaderProperty*>(blocks[i]);
			outList.push_back(shader->name);
		}
		else if (blocks[i]->blockType == BSSHADERPPLIGHTINGPROPERTY) {
			BSShaderPPLightingProperty* shader = dynamic_cast<BSShaderPPLightingProperty*>(blocks[i]);
			outList.push_back(shader->name);
		}
	}
	return outList.size();
}
	
int NifFile::GetNodeID(const string& nodeName) {
	for (int i = 0; i < hdr.numBlocks; i++) {
		if (blocks[i]->blockType == NINODE) {
			NiNode* node = (NiNode*)blocks[i];
			if (!node->name.compare(nodeName))
				return i;
		}
	}
	return -1;
}

bool NifFile::GetNodeTransform(const string& nodeName, vector<Vector3>& outRot, Vector3& outTrans, float& outScale) {
	for (int i = 0; i < hdr.numBlocks; i++) {
		if (blocks[i]->blockType == NINODE) {
			NiNode* node = (NiNode*)blocks[i];
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
	for (int i = 0; i < hdr.numBlocks; i++) {
		if (blocks[i]->blockType == NINODE) {
			NiNode* node = (NiNode*)blocks[i];
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
	outList.clear();

	int skinRef = -1;
	NiTriShape* shape = shapeForName(shapeName);
	if (shape) {
		skinRef = shape->skinInstanceRef;
	}
	else {
		NiTriStrips* strips = stripsForName(shapeName);
		if (!strips)
			return 0;
		skinRef = strips->skinInstanceRef;
	}
	if (skinRef == -1)
		return 0;

	BSDismemberSkinInstance* bsdSkinInst = dynamic_cast<BSDismemberSkinInstance*>(blocks[skinRef]);
	NiSkinInstance* niSkinInst = nullptr;
	bool skipBSDSkinInst = false;
	if (!bsdSkinInst) {
		skipBSDSkinInst = true;
		niSkinInst = dynamic_cast<NiSkinInstance*>(blocks[skinRef]);
		if (!niSkinInst)
			return 0;
	}

	int boneBlock;
	if (skipBSDSkinInst) {
		for (int i = 0; i < niSkinInst->bones.size(); i++) {
			boneBlock = niSkinInst->bones[i];
			if (boneBlock != -1) {
				NiNode* node = (NiNode*)blocks[boneBlock];
				outList.push_back(node->name);
			}
		}
	}
	else {
		for (int i = 0; i < bsdSkinInst->bones.size(); i++) {
			boneBlock = bsdSkinInst->bones[i];
			if (boneBlock != -1) {
				NiNode* node = (NiNode*)blocks[boneBlock];
				outList.push_back(node->name);
			}
		}
	}
	return outList.size();
}

int NifFile::GetShapeBoneIDList(const string& shapeName, vector<int>& outList) {
	outList.clear();

	int skinRef = -1;
	NiTriShape* shape = shapeForName(shapeName);
	if (shape) {
		skinRef = shape->skinInstanceRef;
	}
	else {
		NiTriStrips* strips = stripsForName(shapeName);
		if (!strips)
			return 0;
		skinRef = strips->skinInstanceRef;
	}
	if (skinRef == -1)
		return 0;

	BSDismemberSkinInstance* bsdSkinInst = dynamic_cast<BSDismemberSkinInstance*>(blocks[skinRef]);
	NiSkinInstance* niSkinInst = nullptr;
	bool skipBSDSkinInst = false;
	if (!bsdSkinInst) {
		skipBSDSkinInst = true;
		niSkinInst = dynamic_cast<NiSkinInstance*>(blocks[skinRef]);
		if (!niSkinInst)
			return 0;
	}

	int boneBlock;
	if (skipBSDSkinInst) {
		for (int i = 0; i < niSkinInst->bones.size(); i++) {
			boneBlock = niSkinInst->bones[i];
			outList.push_back(boneBlock);
		}
	}
	else {
		for (int i = 0; i < bsdSkinInst->bones.size(); i++) {
			boneBlock = bsdSkinInst->bones[i];
			outList.push_back(boneBlock);
		}
	}
	return outList.size();
}

void NifFile::SetShapeBoneIDList(const string& shapeName, vector<int>& inList) {
	int skinRef = -1;
	NiTriShape* shape = shapeForName(shapeName);
	if (shape) {
		skinRef = shape->skinInstanceRef;
	}
	else {
		NiTriStrips* strips = stripsForName(shapeName);
		if (!strips) return;
		skinRef = strips->skinInstanceRef;
	}
	if (skinRef == -1)
		return;

	BSDismemberSkinInstance* bsdSkinInst = dynamic_cast<BSDismemberSkinInstance*>(blocks[skinRef]);
	NiSkinInstance* niSkinInst = nullptr;
	bool skipBSDSkinInst = false;
	if (!bsdSkinInst) {
		skipBSDSkinInst = true;
		niSkinInst = dynamic_cast<NiSkinInstance*>(blocks[skinRef]);
		if (!niSkinInst)
			return;
	}

	NiSkinData* skinData = nullptr;
	if (skipBSDSkinInst) {
		niSkinInst->bones.clear();
		niSkinInst->numBones = 0;
		for (int i = 0; i < inList.size(); i++) {
			niSkinInst->bones.push_back(inList[i]);
			niSkinInst->numBones++;
		}
		hdr.blockSizes[skinRef] = niSkinInst->CalcBlockSize();

		skinData = (NiSkinData*)blocks[niSkinInst->dataRef];
		int nBonesToAdd = niSkinInst->bones.size() - skinData->numBones;
		if (nBonesToAdd > 0) {
			for (int i = 0; i < nBonesToAdd; i++) {
				skinData->bones.emplace_back();
				skinData->bones.back().numVertices = 0;
				skinData->numBones++;
			}
			hdr.blockSizes[niSkinInst->dataRef] = skinData->CalcBlockSize();
		}
	}
	else {
		bsdSkinInst->bones.clear();
		bsdSkinInst->numBones = 0;
		for (int i = 0; i < inList.size(); i++) {
			bsdSkinInst->bones.push_back(inList[i]);
			bsdSkinInst->numBones++;
		}
		hdr.blockSizes[skinRef] = bsdSkinInst->CalcBlockSize();

		skinData = (NiSkinData*)blocks[bsdSkinInst->dataRef];
		int nBonesToAdd = bsdSkinInst->bones.size() - skinData->numBones;
		if (nBonesToAdd > 0) {
			for (int i = 0; i < nBonesToAdd; i++) {
				skinData->bones.emplace_back();
				skinData->bones.back().numVertices = 0;
				skinData->numBones++;
			}
			hdr.blockSizes[bsdSkinInst->dataRef] = skinData->CalcBlockSize();
		}
	}
}

int NifFile::GetShapeBoneWeights(const string& shapeName, int boneIndex, unordered_map<ushort, float>& outWeights) {
	outWeights.clear();

	int skinRef;
	NiTriShape* shape = shapeForName(shapeName);
	if (shape) {
		skinRef = shape->skinInstanceRef;
	}
	else {
		NiTriStrips* strips = stripsForName(shapeName);
		if (!strips)
			return 0;
		skinRef = strips->skinInstanceRef;
	}
	if (skinRef == -1)
		return 0;

	BSDismemberSkinInstance* bsdSkinInst = dynamic_cast<BSDismemberSkinInstance*>(blocks[skinRef]);
	NiSkinInstance* niSkinInst = nullptr;
	bool skipBSDSkinInst = false;
	if (!bsdSkinInst) {
		skipBSDSkinInst = true;
		niSkinInst = dynamic_cast<NiSkinInstance*>(blocks[skinRef]);
		if (!niSkinInst || niSkinInst->skinPartitionRef == -1)
			return 0;
	}
	else if (bsdSkinInst->skinPartitionRef == -1)
			return 0;

	NiSkinData* skinData = nullptr;
	if (skipBSDSkinInst)
		skinData = (NiSkinData*)blocks[niSkinInst->dataRef];
	else
		skinData = (NiSkinData*)blocks[bsdSkinInst->dataRef];

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
	int skinRef;
	NiTriShape* shape = shapeForName(shapeName);
	if (shape) {
		skinRef = shape->skinInstanceRef;
	}
	else {
		NiTriStrips* strips = stripsForName(shapeName);
		if (!strips)
			return false;
		skinRef = strips->skinInstanceRef;
	}
	if (skinRef == -1)
		return false;

	BSDismemberSkinInstance* bsdSkinInst = dynamic_cast<BSDismemberSkinInstance*>(blocks[skinRef]);
	NiSkinInstance* niSkinInst = nullptr;
	bool skipBSDSkinInst = false;
	if (!bsdSkinInst) {
		skipBSDSkinInst = true;
		niSkinInst = dynamic_cast<NiSkinInstance*>(blocks[skinRef]);
		if (!niSkinInst || niSkinInst->skinPartitionRef == -1)
			return false;
	}
	else if (bsdSkinInst->skinPartitionRef == -1)
		return false;

	NiSkinData* skinData = nullptr;
	if (skipBSDSkinInst)
		skinData = (NiSkinData*)blocks[niSkinInst->dataRef];
	else
		skinData = (NiSkinData*)blocks[bsdSkinInst->dataRef];

	if (boneIndex == -1) { // set the overall skin transform
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
	int skinRef;
	NiTriShape* shape = shapeForName(shapeName);
	if (shape) {
		skinRef = shape->skinInstanceRef;
	}
	else {
		NiTriStrips* strips = stripsForName(shapeName);
		if (!strips)
			return false;
		skinRef = strips->skinInstanceRef;
	}
	if (skinRef == -1)
		return false;

	BSDismemberSkinInstance* bsdSkinInst = dynamic_cast<BSDismemberSkinInstance*>(blocks[skinRef]);
	NiSkinInstance* niSkinInst = nullptr;
	bool skipBSDSkinInst = false;
	if (!bsdSkinInst) {
		skipBSDSkinInst = true;
		niSkinInst = dynamic_cast<NiSkinInstance*>(blocks[skinRef]);
		if (!niSkinInst || niSkinInst->skinPartitionRef == -1)
			return false;
	}
	else if (bsdSkinInst->skinPartitionRef == -1)
		return false;

	NiSkinData* skinData = nullptr;
	if (skipBSDSkinInst)
		skinData = (NiSkinData*)blocks[niSkinInst->dataRef];
	else
		skinData = (NiSkinData*)blocks[bsdSkinInst->dataRef];

	if (boneIndex == -1) { // want the overall skin transform
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
	int skinRef;
	NiTriShape* shape = shapeForName(shapeName);
	if (shape) {
		skinRef = shape->skinInstanceRef;
	}
	else {
		NiTriStrips* strips = stripsForName(shapeName);
		if (!strips)
			return;
		skinRef = strips->skinInstanceRef;
	}
	if (skinRef == -1)
		return;

	BSDismemberSkinInstance* bsdSkinInst = dynamic_cast<BSDismemberSkinInstance*>(blocks[skinRef]);
	NiSkinInstance* niSkinInst = nullptr;
	bool skipBSDSkinInst = false;
	if (!bsdSkinInst) {
		skipBSDSkinInst = true;
		niSkinInst = dynamic_cast<NiSkinInstance*>(blocks[skinRef]);
		if (!niSkinInst || niSkinInst->skinPartitionRef == -1)
			return;
	}
	else if (bsdSkinInst->skinPartitionRef == -1)
		return;

	if (skipBSDSkinInst) {
		for (auto &bp : niSkinInst->bones) {
			if (bp == oldID) {
				bp = newID;
				return;
			}
		}
	}
	else {
		for (auto &bp : bsdSkinInst->bones) {
			if (bp == oldID) {
				bp = newID;
				return;
			}
		}
	}
}

void NifFile::SetShapeBoneWeights(const string& shapeName, int boneIndex, unordered_map<ushort, float>& inWeights) {
	int skinRef;
	NiTriShape* shape = shapeForName(shapeName);
	if (shape) {
		skinRef = shape->skinInstanceRef;
	}
	else {
		NiTriStrips* strips = stripsForName(shapeName);
		if (!strips)
			return;
		skinRef = strips->skinInstanceRef;
	}
	if (skinRef == -1)
		return;

	BSDismemberSkinInstance* bsdSkinInst = dynamic_cast<BSDismemberSkinInstance*>(blocks[skinRef]);
	NiSkinInstance* niSkinInst = nullptr;
	bool skipBSDSkinInst = false;
	if (!bsdSkinInst) {
		skipBSDSkinInst = true;
		niSkinInst = dynamic_cast<NiSkinInstance*>(blocks[skinRef]);
		if (!niSkinInst || niSkinInst->skinPartitionRef == -1)
			return;
	}
	else if (bsdSkinInst->skinPartitionRef == -1)
		return;

	NiSkinData* skinData = nullptr;
	if (skipBSDSkinInst)
		skinData = (NiSkinData*)blocks[niSkinInst->dataRef];
	else
		skinData = (NiSkinData*)blocks[bsdSkinInst->dataRef];

	if (boneIndex > skinData->numBones)
		return;

	NiSkinData::BoneData* bone = &skinData->bones[boneIndex];
	bone->vertexWeights.clear();
	for (auto &sw : inWeights)
		if (sw.second >= 0.0001f)
			bone->vertexWeights.emplace_back(SkinWeight(sw.first, sw.second));

	bone->numVertices = (ushort)bone->vertexWeights.size();
	if (skipBSDSkinInst)
		hdr.blockSizes[niSkinInst->dataRef] = skinData->CalcBlockSize();
	else
		hdr.blockSizes[bsdSkinInst->dataRef] = skinData->CalcBlockSize();
}

const vector<Vector3>* NifFile::GetRawVertsForShape(const string& shapeName) {
	int bType;
	int dataID = shapeDataIdForName(shapeName, bType);
	if (dataID == -1)
		return nullptr;

	if (bType == NITRISHAPEDATA) {
		NiTriShapeData* shapeData = (NiTriShapeData*)blocks[dataID];
		return &shapeData->vertices;
	}
	else {
		NiTriStripsData* stripsData = (NiTriStripsData*)blocks[dataID];
		return &stripsData->vertices;
	}
}

const vector<Triangle>* NifFile::GetTrisForShape(const string& shapeName) {
	NiTriShape* shape = shapeForName(shapeName);
	if (!shape)
		return nullptr;

	int dataBlock = shape->dataRef;
	if (dataBlock == -1 || dataBlock >= hdr.numBlocks)
		return nullptr;

	NiTriShapeData* shapeData = (NiTriShapeData*)blocks[dataBlock];
	return &shapeData->triangles;
}
	
bool NifFile::GetTrisForTriStripShape(const string& shapeName, vector<Triangle>* outTris) {
	NiTriStrips* strips = stripsForName(shapeName);
	if (!strips)
		return false;

	int dataBlock = strips->dataRef;
	if (dataBlock == -1 || dataBlock >= hdr.numBlocks)
		return false;

	NiTriStripsData* stripsData = (NiTriStripsData*)blocks[dataBlock];
	stripsData->StripsToTris(outTris);
	return true;
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
	else {
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
	else {
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
	else {
		NiTriStripsData* stripsData = (NiTriStripsData*)blocks[dataID];
		outUvs.assign(stripsData->uvSets.begin(), stripsData->uvSets.end());
		return true;
	}
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

	Matrix4 xForm;
	int bType;
	int dataID = shapeDataIdForName(shapeName, bType);
	if (dataID == -1)
		return false;

	if (bType == NITRISHAPEDATA) {
		NiTriShapeData* shapeData = (NiTriShapeData*)blocks[dataID];
		xForm.Translate(shapeData->virtOffset);
		xForm.Scale(shapeData->virtScale, shapeData->virtScale, shapeData->virtScale);
		for (int i = 0; i < shapeData->vertices.size(); i++) {
			if (shapeData->virtScale != 1.0f)
				outVerts.push_back(xForm * shapeData->vertices[i]);
			else
				outVerts.push_back(shapeData->vertices[i] + shapeData->virtOffset);
		}
	}
	else {
		NiTriStripsData* stripsData = (NiTriStripsData*)blocks[dataID];
		xForm.Translate(stripsData->virtOffset);
		xForm.Scale(stripsData->virtScale, stripsData->virtScale, stripsData->virtScale);
		for (int i = 0; i < stripsData->vertices.size(); i++) {
			if (stripsData->virtScale != 1.0f)
				outVerts.push_back(xForm * stripsData->vertices[i]);
			else
				outVerts.push_back(stripsData->vertices[i] + stripsData->virtOffset);
		}
	}
	return true;
}

int NifFile::GetVertCountForShape(const string& shapeName) {
	int bType;
	int dataID = shapeDataIdForName(shapeName, bType);
	if (dataID == -1)
		return -1;

	if (bType == NITRISHAPEDATA) {
		NiTriShapeData* shapeData = dynamic_cast<NiTriShapeData*>(blocks[dataID]);
		if (shapeData)
			return shapeData->numVertices;
	}
	else {
		NiTriStripsData* stripsData = dynamic_cast<NiTriStripsData*>(blocks[dataID]);
		if (stripsData)
			return stripsData->numVertices;
	}
	return -1;
}

void NifFile::SetVertsForShape(const string& shapeName, const vector<Vector3>& verts) {
	Matrix4 xForm;
	int bType;
	int dataID = shapeDataIdForName(shapeName, bType);
	if (dataID == -1)
		return;

	if (bType == NITRISHAPEDATA) {
		NiTriShapeData* shapeData = (NiTriShapeData*)blocks[dataID];
		if (verts.size() != shapeData->vertices.size())
			return;

		for (int i = 0; i < shapeData->vertices.size(); i++) {
			shapeData->vertices[i] = verts[i];
			xForm.Translate(shapeData->virtOffset);
			if (shapeData->virtScale != 0.0f) {
				float negatedScale = shapeData->virtScale * (1 / shapeData->virtScale);
				xForm.Scale(negatedScale, negatedScale, negatedScale);
			}
		}
	}
	else {
		NiTriStripsData* stripsData = (NiTriStripsData*)blocks[dataID];
		if (verts.size() != stripsData->vertices.size())
			return;

		for (int i = 0; i < stripsData->vertices.size(); i++) {
			stripsData->vertices[i] = verts[i];
			xForm.Translate(stripsData->virtOffset);
			if (stripsData->virtScale != 0.0f) {
				float negatedScale = stripsData->virtScale * (1 / stripsData->virtScale);
				xForm.Scale(negatedScale, negatedScale, negatedScale);
			}
		}
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
	else {
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
	else {
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
	else {
		NiTriStripsData* stripsData = (NiTriStripsData*)blocks[dataID];
		stripsData->CalcTangentSpace();
		hdr.blockSizes[dataID] = stripsData->CalcBlockSize();
	}
}

void NifFile::ClearShapeTransform(const string& shapeName) {
	NiTriShape* shape = shapeForName(shapeName);
	if (!shape) {
		NiTriStrips* strips = stripsForName(shapeName);
		if (!strips)
			return;
		strips->translation.Zero();
		strips->scale = 1.0f;
		strips->rotation[0] = Vector3(1.0f, 0.0f, 0.0f);
		strips->rotation[1] = Vector3(0.0f, 1.0f, 0.0f);
		strips->rotation[2] = Vector3(0.0f, 0.0f, 1.0f);
	}
	else {
		shape->translation.Zero();
		shape->scale = 1.0f;
		shape->rotation[0] = Vector3(1.0f, 0.0f, 0.0f);
		shape->rotation[1] = Vector3(0.0f, 1.0f, 0.0f);
		shape->rotation[2] = Vector3(0.0f, 0.0f, 1.0f);
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
	NiTriShape* shape = shapeForName(shapeName);
	if (!shape) {
		NiTriStrips* strips = stripsForName(shapeName);
		if (!strips)
			return;
		xFormShape.translation = strips->translation;
		xFormShape.scale = strips->scale;
		xFormShape.rotation[0] = strips->rotation[0];
		xFormShape.rotation[1] = strips->rotation[1];
		xFormShape.rotation[2] = strips->rotation[2];
	}
	else {
		xFormShape.translation = shape->translation;
		xFormShape.scale = shape->scale;
		xFormShape.rotation[0] = shape->rotation[0];
		xFormShape.rotation[1] = shape->rotation[1];
		xFormShape.rotation[2] = shape->rotation[2];
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
	NiTriShape* shape = shapeForName(shapeName);
	if (!shape) {
		NiTriStrips* strips = stripsForName(shapeName);
		if (!strips)
			return;
		outVec = strips->translation;
	}
	else
		outVec = shape->translation;

	NiNode* root = dynamic_cast<NiNode*>(blocks[0]);
	if (root)
		outVec += root->translation;

	//if (outVec.DistanceTo(Vector3(0.0f, 0.0f, 0.0f)) < EPSILON)
	//	outVec.Zero();
}

void NifFile::SetShapeTranslation(const string& shapeName, const Vector3& newTrans) {
	NiTriShape* shape = shapeForName(shapeName);
	if (!shape) {
		NiTriStrips* strips = stripsForName(shapeName);
		if (!strips)
			return;
		strips->translation = newTrans;
	}
	else
		shape->translation = newTrans;
}

void NifFile::GetShapeScale(const string& shapeName, float& outScale) {
	NiTriShape* shape = shapeForName(shapeName);
	if (!shape) {
		NiTriStrips* strips = stripsForName(shapeName);
		if (!strips)
			return;
		outScale = strips->scale;
	}
	else
		outScale = shape->scale;
}

void NifFile::SetShapeScale(const string& shapeName, const float& newScale) {
	NiTriShape* shape = shapeForName(shapeName);
	if (!shape) {
		NiTriStrips* strips = stripsForName(shapeName);
		if (!strips)
			return;

		strips->scale = newScale;
	}
	else
		shape->scale = newScale;
}

void NifFile::ApplyShapeTranslation(const string& shapeName, const Vector3& offset) {
	int bType;
	int dataID = shapeDataIdForName(shapeName, bType);
	if (dataID == -1)
		return;

	if (bType == NITRISHAPEDATA) {
		NiTriShapeData* shapeData = (NiTriShapeData*)blocks[dataID];
		NiTriShape* shape = shapeForName(shapeName);
		for (int i = 0; i < shapeData->vertices.size(); i++)
			shapeData->vertices[i] += shape->translation + offset;
		shape->translation = Vector3(0.0f, 0.0f, 0.0f);
	}
	else {
		NiTriStripsData* stripsData = (NiTriStripsData*)blocks[dataID];
		NiTriStrips* strips = stripsForName(shapeName);
		for (int i = 0; i < stripsData->vertices.size(); i++)
			stripsData->vertices[i] += strips->translation + offset;
		strips->translation = Vector3(0.0f, 0.0f, 0.0f);
	}
}
	
/* Sets a phantom offset value in a NiTriShape/StripsData record, which will be added to vertex values when fetched later.
If sum is true, the offset is added to the current offset, allowing a series of independant changes.*/
void NifFile::VirtualOffsetShape(const string& shapeName, const Vector3& offset, bool sum) {
	int bType;
	int dataID = shapeDataIdForName(shapeName, bType);
	if (dataID == -1)
		return;

	if (bType == NITRISHAPEDATA) {
		NiTriShapeData* shapeData = (NiTriShapeData*)blocks[dataID];
		if (sum)
			shapeData->virtOffset += offset;
		else
			shapeData->virtOffset = offset;
	}
	else {
		NiTriStripsData* stripsData = (NiTriStripsData*)blocks[dataID];
		if (sum)
			stripsData->virtOffset += offset;
		else
			stripsData->virtOffset = offset;
	}
}
	
/* Sets a phantom scale value in a NiTriShape/StripsData record, which will be added to vertex values when fetched later.*/
void NifFile::VirtualScaleShape(const string& shapeName, float scale, bool fromCenter) {
	int bType;
	int dataID = shapeDataIdForName(shapeName, bType);
	if (dataID == -1)
		return;

	if (bType == NITRISHAPEDATA) {
		NiTriShapeData* shapeData = (NiTriShapeData*)blocks[dataID];
		shapeData->scaleFromCenter = fromCenter;
		shapeData->virtScale = scale;
	}
	else {
		NiTriStripsData* stripsData = (NiTriStripsData*)blocks[dataID];
		stripsData->scaleFromCenter = fromCenter;
		stripsData->virtScale = scale;
	}
}

Vector3 NifFile::GetShapeVirtualOffset(const string& shapeName) {
	Vector3 ret(0.0f, 0.0f, 0.0f);
	int bType;
	int dataID = shapeDataIdForName(shapeName, bType);
	if (dataID == -1)
		return ret;

	if (bType == NITRISHAPEDATA) {
		NiTriShapeData* shapeData = (NiTriShapeData*)blocks[dataID];
		ret = shapeData->virtOffset;
	}
	else {
		NiTriStripsData* stripsData = (NiTriStripsData*)blocks[dataID];
		ret = stripsData->virtOffset;
	}
	return ret;
}
		
void NifFile::GetShapeVirtualScale(const string& shapeName, float& scale, bool& fromCenterFlag) {
	int bType;
	int dataID = shapeDataIdForName(shapeName, bType);
	if (dataID == -1)
		return;

	if (bType == NITRISHAPEDATA) {
		NiTriShapeData* shapeData = (NiTriShapeData*)blocks[dataID];
		scale = shapeData->virtScale;
		fromCenterFlag = shapeData->scaleFromCenter;
	}
	else {
		NiTriStripsData* stripsData = (NiTriStripsData*)blocks[dataID];
		scale = stripsData->virtScale;
		fromCenterFlag = stripsData->scaleFromCenter;
	}
	return;
}

void NifFile::MoveVertex(const string& shapeName, const Vector3& pos, const int& id) {
	int bType;
	int dataID = shapeDataIdForName(shapeName, bType);
	if (dataID == -1)
		return;

	vector<Vector3>* verts;
	if (bType == NITRISHAPEDATA) {
		NiTriShapeData* shapeData = (NiTriShapeData*)blocks[dataID];
		verts = &shapeData->vertices;
	}
	else {
		NiTriStripsData* stripsData = (NiTriStripsData*)blocks[dataID];
		verts = &stripsData->vertices;
	}
	(*verts)[id] = pos;
}

void NifFile::OffsetShape(const string& shapeName, const Vector3& offset, unordered_map<ushort, float>* mask) {
	int bType;
	int dataID = shapeDataIdForName(shapeName, bType);
	if (dataID == -1)
		return;

	vector<Vector3>* verts;
	if (bType == NITRISHAPEDATA) {
		NiTriShapeData* shapeData = (NiTriShapeData*)blocks[dataID];
		verts = &shapeData->vertices;
		// dosomthin
	}
	else {
		NiTriStripsData* stripsData = (NiTriStripsData*)blocks[dataID];
		verts = &stripsData->vertices;
		// dosomthin
	}

	float m;
	Vector3 o;
	for (int i = 0; i < verts->size(); i++) {
		if (mask) {
			m = 1.0f;
			o = offset;
			if (mask->find(i) != mask->end()) {
				m = 1.0f - (*mask)[i];
				o *= m;
			}
			(*verts)[i] += o;
		}
		else
			(*verts)[i] += offset;
	}
}

void NifFile::ScaleShape(const string& shapeName, const float& scale, unordered_map<ushort, float>* mask) {
	int bType;
	int dataID = shapeDataIdForName(shapeName, bType);
	if (dataID == -1)
		return;

	vector<Vector3>* verts;
	if (bType == NITRISHAPEDATA) {
		NiTriShapeData* shapeData = (NiTriShapeData*)blocks[dataID];
		verts = &shapeData->vertices;
	}
	else {
		NiTriStripsData* stripsData = (NiTriStripsData*)blocks[dataID];
		verts = &stripsData->vertices;
	}

	Vector3 root;
	GetRootTranslation(root);

	unordered_map<int, Vector3> diff;
	for (int i = 0; i < verts->size(); i++) {
		Vector3 target = (*verts)[i] - root;
		target *= scale;
		diff[i] = (*verts)[i] - target;

		if (mask) {
			float maskFactor = 1.0f;
			if (mask->find(i) != mask->end()) {
				maskFactor = 1.0f - (*mask)[i];
				diff[i] *= maskFactor;
				target = (*verts)[i] - root + diff[i];
			}
		}
		(*verts)[i] = target;
	}
}

void NifFile::RotateShape(const string& shapeName, const Vector3& angle, unordered_map<ushort, float>* mask) {
	int bType;
	int dataID = shapeDataIdForName(shapeName, bType);
	if (dataID == -1)
		return;

	vector<Vector3>* verts;
	if (bType == NITRISHAPEDATA) {
		NiTriShapeData* shapeData = (NiTriShapeData*)blocks[dataID];
		verts = &shapeData->vertices;
	}
	else {
		NiTriStripsData* stripsData = (NiTriStripsData*)blocks[dataID];
		verts = &stripsData->vertices;
	}

	Vector3 root;
	GetRootTranslation(root);

	unordered_map<int, Vector3> diff;
	for (int i = 0; i < verts->size(); i++) {
		Vector3 target = (*verts)[i] - root;
		Matrix4 mat;
		mat.Rotate(angle.x * DEG2RAD, Vector3(1.0f, 0.0f, 0.0f));
		mat.Rotate(angle.y * DEG2RAD, Vector3(0.0f, 1.0f, 0.0f));
		mat.Rotate(angle.z * DEG2RAD, Vector3(0.0f, 0.0f, 1.0f));
		target = mat * target;
		diff[i] = (*verts)[i] - target;

		if (mask) {
			float maskFactor = 1.0f;
			if (mask->find(i) != mask->end()) {
				maskFactor = 1.0f - (*mask)[i];
				diff[i] *= maskFactor;
				target = (*verts)[i] - root + diff[i];
			}
		}
		(*verts)[i] = target;
	}
}

void NifFile::GetAlphaForShape(const string& shapeName, ushort& outFlags, byte& outThreshold) {
	int alphaRef = -1;
	NiTriShape* shape = shapeForName(shapeName);
	if (!shape) {
		NiTriStrips* strips = stripsForName(shapeName);
		if (!strips)
			return;

		alphaRef = strips->propertiesRef2;
		if (alphaRef == -1) {
			NiAlphaProperty* alphaProp = nullptr;
			for (int i = 0; i < strips->numProperties; i++) {
				if (!alphaProp) {
					alphaProp = dynamic_cast<NiAlphaProperty*>(GetBlock(strips->propertiesRef[i]));
					if (alphaProp)
						alphaRef = strips->propertiesRef[i];
				}
				else
					break;
			}
		}
	}
	else {
		alphaRef = shape->propertiesRef2;
		if (alphaRef == -1) {
			NiAlphaProperty* alphaProp = nullptr;
			for (int i = 0; i < shape->numProperties; i++) {
				if (!alphaProp) {
					alphaProp = dynamic_cast<NiAlphaProperty*>(GetBlock(shape->propertiesRef[i]));
					if (alphaProp)
						alphaRef = shape->propertiesRef[i];
				}
				else
					break;
			}
		}
	}

	NiAlphaProperty* alpha = dynamic_cast<NiAlphaProperty*>(GetBlock(alphaRef));
	if (!alpha)
		return;

	outFlags = alpha->flags;
	outThreshold = alpha->threshold;
}

void NifFile::SetAlphaForShape(const string& shapeName, ushort flags, ushort threshold) {
	int alphaRef = -1;
	NiTriShape* shape = shapeForName(shapeName);
	if (!shape) {
		NiTriStrips* strips = stripsForName(shapeName);
		if (!strips)
			return;

		alphaRef = strips->propertiesRef2;
		if (alphaRef == -1) {
			NiAlphaProperty* alphaProp = nullptr;
			for (int i = 0; i < strips->numProperties; i++) {
				if (!alphaProp) {
					alphaProp = dynamic_cast<NiAlphaProperty*>(GetBlock(strips->propertiesRef[i]));
					if (alphaProp)
						alphaRef = strips->propertiesRef[i];
				}
				else
					break;
			}
		}
	}
	else {
		alphaRef = shape->propertiesRef2;
		if (alphaRef == -1) {
			NiAlphaProperty* alphaProp = nullptr;
			for (int i = 0; i < shape->numProperties; i++) {
				if (!alphaProp) {
					alphaProp = dynamic_cast<NiAlphaProperty*>(GetBlock(shape->propertiesRef[i]));
					if (alphaProp)
						alphaRef = shape->propertiesRef[i];
				}
				else
					break;
			}
		}
	}

	NiAlphaProperty* alpha = dynamic_cast<NiAlphaProperty*>(GetBlock(alphaRef));
	if (!alpha)
		return;

	alpha->flags = flags;
	alpha->threshold = (byte)threshold;
}

void NifFile::DeleteShape(const string& shapeName) {
	NiTriShape* shape = shapeForName(shapeName);
	if (!shape) {
		NiTriStrips* strips = stripsForName(shapeName);
		if (!strips)
			return;

		DeleteBlock(strips->dataRef);
		if (strips->skinInstanceRef != -1) {
			BSDismemberSkinInstance* bsdSkinInst = dynamic_cast<BSDismemberSkinInstance*>(GetBlock(strips->skinInstanceRef));
			if (bsdSkinInst) {
				DeleteBlock(bsdSkinInst->dataRef);
				DeleteBlock(bsdSkinInst->skinPartitionRef);
				DeleteBlock(strips->skinInstanceRef);
			}
			else {
				NiSkinInstance* niSkinInst = dynamic_cast<NiSkinInstance*>(GetBlock(strips->skinInstanceRef));
				if (niSkinInst) {
					DeleteBlock(niSkinInst->dataRef);
					DeleteBlock(niSkinInst->skinPartitionRef);
					DeleteBlock(strips->skinInstanceRef);
				}
			}
		}

		if (strips->propertiesRef1 != -1) {
			BSLightingShaderProperty* shader = dynamic_cast<BSLightingShaderProperty*>(GetBlock(strips->propertiesRef1));
			if (shader) {
				DeleteBlock(shader->textureSetRef);
				DeleteBlock(strips->propertiesRef1);
			}
		}
		if (strips->propertiesRef2 != -1)
			DeleteBlock(strips->propertiesRef2);

		for (int i = 0; i < strips->numProperties; i++) {
			BSShaderPPLightingProperty* shaderPP = dynamic_cast<BSShaderPPLightingProperty*>(GetBlock(strips->propertiesRef[i]));
			if (shaderPP) {
				DeleteBlock(shaderPP->textureSetRef);
				DeleteBlock(strips->propertiesRef[i]);
				i--;
			}
			else {
				DeleteBlock(strips->propertiesRef[i]);
				i--;
			}
		}

		for (int i = 0; i < strips->numExtraData; i++)
			DeleteBlock(strips->extraDataRef[i]);
	}
	else {
		DeleteBlock(shape->dataRef);
		if (shape->skinInstanceRef != -1) {
			BSDismemberSkinInstance* bsdSkinInst = dynamic_cast<BSDismemberSkinInstance*>(GetBlock(shape->skinInstanceRef));
			if (bsdSkinInst) {
				DeleteBlock(bsdSkinInst->dataRef);
				DeleteBlock(bsdSkinInst->skinPartitionRef);
				DeleteBlock(shape->skinInstanceRef);
			}
			else {
				NiSkinInstance* niSkinInst = dynamic_cast<NiSkinInstance*>(GetBlock(shape->skinInstanceRef));
				if (niSkinInst) {
					DeleteBlock(niSkinInst->dataRef);
					DeleteBlock(niSkinInst->skinPartitionRef);
					DeleteBlock(shape->skinInstanceRef);
				}
			}
		}
		if (shape->propertiesRef1 != -1) {
			BSLightingShaderProperty* shader = dynamic_cast<BSLightingShaderProperty*>(GetBlock(shape->propertiesRef1));
			if (shader) {
				DeleteBlock(shader->textureSetRef);
				DeleteBlock(shape->propertiesRef1);
			}
		}
		if (shape->propertiesRef2 != -1)
			DeleteBlock(shape->propertiesRef2);

		for (int i = 0; i < shape->numProperties; i++) {
			BSShaderPPLightingProperty* shaderPP = dynamic_cast<BSShaderPPLightingProperty*>(GetBlock(shape->propertiesRef[i]));
			if (shaderPP) {
				DeleteBlock(shaderPP->textureSetRef);
				DeleteBlock(shape->propertiesRef[i]);
				i--;
			}
			else {
				DeleteBlock(shape->propertiesRef[i]);
				i--;
			}
		}

		for (int i = 0; i < shape->numExtraData; i++)
			DeleteBlock(shape->extraDataRef[i]);
	}
	int shapeID = shapeIdForName(shapeName);

	DeleteBlock(shapeID);
}

void NifFile::DeleteVertsForShape(const string& shapeName, const vector<ushort>& indices) {
	if (indices.size() == 0)
		return;

	NiTriShape* shape = shapeForName(shapeName);
	int dataRef = -1;
	int skinRef = -1;
	if (!shape) {
		NiTriStrips* strips = stripsForName(shapeName);
		if (!strips)
			return;

		dataRef = strips->dataRef;
		skinRef = strips->skinInstanceRef;
		NiTriStripsData* shapeData = (NiTriStripsData*)blocks[dataRef];
		shapeData->notifyVerticesDelete(indices);
		if (shapeData->numVertices == 0 || shapeData->numTriangles == 0) {
			// Deleted all verts or tris! Remove entire shape and children.
			DeleteShape(shapeName);
			return;
		}
		hdr.blockSizes[dataRef] = shapeData->CalcBlockSize();
	}
	else {
		dataRef = shape->dataRef;
		skinRef = shape->skinInstanceRef;
		NiTriShapeData* shapeData = (NiTriShapeData*)blocks[dataRef];
		shapeData->notifyVerticesDelete(indices);
		if (shapeData->numVertices == 0 || shapeData->numTriangles == 0) {
			// Deleted all verts or tris! Remove entire shape and children.
			DeleteShape(shapeName);
			return;
		}
		hdr.blockSizes[dataRef] = shapeData->CalcBlockSize();
	}

	if (skinRef == -1)
		return;

	BSDismemberSkinInstance* bsdSkinInst = dynamic_cast<BSDismemberSkinInstance*>(blocks[skinRef]);
	if (bsdSkinInst) {
		NiSkinData* skinData = (NiSkinData*)blocks[bsdSkinInst->dataRef];
		skinData->notifyVerticesDelete(indices);
		hdr.blockSizes[bsdSkinInst->dataRef] = skinData->CalcBlockSize();

		NiSkinPartition* skinPartition = (NiSkinPartition*)blocks[bsdSkinInst->skinPartitionRef];
		skinPartition->notifyVerticesDelete(indices);
		vector<int> emptyIndices;
		if (skinPartition->RemoveEmptyPartitions(emptyIndices)) {
			bsdSkinInst->numPartitions -= emptyIndices.size();
			for (int i = emptyIndices.size() - 1; i >= 0; i--)
				bsdSkinInst->partitions.erase(bsdSkinInst->partitions.begin() + i);
		}
		hdr.blockSizes[bsdSkinInst->skinPartitionRef] = skinPartition->CalcBlockSize();
		hdr.blockSizes[skinRef] = bsdSkinInst->CalcBlockSize();
	}
	else {
		NiSkinInstance* niSkinInst = dynamic_cast<NiSkinInstance*>(blocks[skinRef]);
		if (niSkinInst) {
			NiSkinData* skinData = (NiSkinData*)blocks[niSkinInst->dataRef];
			skinData->notifyVerticesDelete(indices);
			hdr.blockSizes[niSkinInst->dataRef] = skinData->CalcBlockSize();

			NiSkinPartition* skinPartition = (NiSkinPartition*)blocks[niSkinInst->skinPartitionRef];
			skinPartition->notifyVerticesDelete(indices);
			vector<int> emptyIndices;
			skinPartition->RemoveEmptyPartitions(emptyIndices);
			hdr.blockSizes[niSkinInst->skinPartitionRef] = skinPartition->CalcBlockSize();
			hdr.blockSizes[skinRef] = niSkinInst->CalcBlockSize();
		}
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

		//if (fabs(v.x) > EPSILON || fabs(v.y) > EPSILON || fabs(v.z) > EPSILON)
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
	int bType;
	int shapeDataID = shapeDataIdForName(shapeName, bType);
	if (shapeDataID == -1)
		return;

	int skinRef;
	if (bType == NITRISHAPEDATA) {
		NiTriShape* shape = shapeForName(shapeName);
		skinRef = shape->skinInstanceRef;
	}
	else {
		NiTriStrips* strips = stripsForName(shapeName);
		skinRef = strips->skinInstanceRef;
	}
	if (skinRef == -1)
		return;

	bool skipBSDSkinInst = false;
	BSDismemberSkinInstance* bsdSkinInst = dynamic_cast<BSDismemberSkinInstance*>(blocks[skinRef]);
	NiSkinInstance* niSkinInst = nullptr;
	NiSkinData* skinData = nullptr;
	NiSkinPartition* skinPart = nullptr;
	if (!bsdSkinInst) {
		skipBSDSkinInst = true;
		niSkinInst = dynamic_cast<NiSkinInstance*>(blocks[skinRef]);
		if (!niSkinInst)
			return;

		skinData = (NiSkinData*)blocks[niSkinInst->dataRef];
		skinPart = (NiSkinPartition*)blocks[niSkinInst->skinPartitionRef];
	}
	else {
		skinData = (NiSkinData*)blocks[bsdSkinInst->dataRef];
		skinPart = (NiSkinPartition*)blocks[bsdSkinInst->skinPartitionRef];
	}

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

	if (!skipBSDSkinInst) {
		unordered_map<int, int> partIDCount;
		for (int i = 0; i < bsdSkinInst->numPartitions; i++) {
			auto pidc = partIDCount.find(bsdSkinInst->partitions[i].partID);
			if (pidc == partIDCount.end())
				partIDCount[bsdSkinInst->partitions[i].partID] = 1;
			else
				partIDCount[bsdSkinInst->partitions[i].partID]++;
		}
		hdr.blockSizes[bsdSkinInst->skinPartitionRef] = skinPart->CalcBlockSize();
	}
	else
		hdr.blockSizes[niSkinInst->skinPartitionRef] = skinPart->CalcBlockSize();
}

void NifFile::BuildSkinPartitions(const string& shapeName, int maxBonesPerPartition) {
	int bType;
	int shapeDataID = shapeDataIdForName(shapeName, bType);
	if (shapeDataID == -1)
		return;

	int skinRef;
	NiTriShapeData* shapeData = nullptr;
	NiTriStripsData* stripsData = nullptr;
	if (bType == NITRISHAPEDATA) {
		NiTriShape* shape = shapeForName(shapeName);
		if (!shape)
			return;

		skinRef = shape->skinInstanceRef;
		shapeData = (NiTriShapeData*)blocks[shape->dataRef];
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
			shape->skinInstanceRef = dismemberID;
			skinRef = dismemberID;
		}
	}
	else {
		NiTriStrips* strips = stripsForName(shapeName);
		if (!strips)
			return;

		// TO-DO
		return;

		skinRef = strips->skinInstanceRef;
		stripsData = (NiTriStripsData*)blocks[strips->dataRef];
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
			strips->skinInstanceRef = skinID;
			skinRef = skinID;
		}
	}

	bool skipBSDSkinInst = false;
	BSDismemberSkinInstance* bsdSkinInst = dynamic_cast<BSDismemberSkinInstance*>(blocks[skinRef]);
	NiSkinInstance* niSkinInst = nullptr;
	NiSkinData* skinData = nullptr;
	NiSkinPartition* skinPart = nullptr;

	if (!bsdSkinInst) {
		skipBSDSkinInst = true;
		niSkinInst = dynamic_cast<NiSkinInstance*>(blocks[skinRef]);
		if (!niSkinInst)
			return;

		skinData = (NiSkinData*)blocks[niSkinInst->dataRef];
		skinPart = (NiSkinPartition*)blocks[niSkinInst->skinPartitionRef];
	}
	else {
		skinData = (NiSkinData*)blocks[bsdSkinInst->dataRef];
		skinPart = (NiSkinPartition*)blocks[bsdSkinInst->skinPartitionRef];
	}

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
	if (!skipBSDSkinInst) {
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
		hdr.blockSizes[bsdSkinInst->skinPartitionRef] = skinPart->CalcBlockSize();
		hdr.blockSizes[skinRef] = bsdSkinInst->CalcBlockSize();
	}
	else {
		hdr.blockSizes[niSkinInst->skinPartitionRef] = skinPart->CalcBlockSize();
		hdr.blockSizes[skinRef] = niSkinInst->CalcBlockSize();
	}
}
