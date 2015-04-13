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

NifBlockTriShape* NifFile::shapeForName(const string& name, int dupIndex) {
	int n = 0;
	for (int i = 0; i < hdr.numBlocks; i++) {
		if (blocks[i]->blockType == NIFBLOCK_TRISHAPE) {
			NifBlockTriShape* shape = (NifBlockTriShape*)blocks[i];
			if (!name.compare(shape->shapeName)) {
				if (n >= dupIndex)
					return (shape);
				n++;
			}
		}
	}
	return NULL;
}

NifBlockTriStrips* NifFile::stripsForName(const string& name, int dupIndex) {
	int n = 0;
	for (int i = 0; i < hdr.numBlocks; i++) {
		if (blocks[i]->blockType == NIFBLOCK_TRISTRIPS) {
			NifBlockTriStrips* strips = (NifBlockTriStrips*)blocks[i];
			if (!name.compare(strips->shapeName))
				if (n >= dupIndex)
					return (strips);
		}
	}
	return NULL;
}

int NifFile::shapeDataIdForName(const string& name, int& outBlockType) {
	for (int i = 0; i < hdr.numBlocks; i++) {
		if (blocks[i]->blockType == NIFBLOCK_TRISTRIPS) {
			NifBlockTriStrips* strips = (NifBlockTriStrips*)blocks[i];
			if (!name.compare(strips->shapeName)) {
				outBlockType = NIFBLOCK_TRISTRIPSDATA;
				return strips->dataRef;
			}
		}
		else if (blocks[i]->blockType == NIFBLOCK_TRISHAPE) {
			NifBlockTriShape* shape = (NifBlockTriShape*)blocks[i];
			if (!name.compare(shape->shapeName)) {
				outBlockType = NIFBLOCK_TRISHAPEDATA;
				return shape->dataRef;
			}
		}
	}
	return -1;
}

int NifFile::shapeIdForName(const string& name) {
	for (int i = 0; i < hdr.numBlocks; i++) {
		if (blocks[i]->blockType == NIFBLOCK_TRISHAPE) {
			NifBlockTriShape* shape = (NifBlockTriShape*)blocks[i];
			if (!name.compare(shape->shapeName))
				return (i);
		}
		else if (blocks[i]->blockType == NIFBLOCK_TRISTRIPS) {
			NifBlockTriStrips* strips = (NifBlockTriStrips*)blocks[i];
			if (!name.compare(strips->shapeName))
				return (i);
		}
	}
	return -1;
}

NifBlockNiNode* NifFile::nodeForName(const string& name) {
	for (int i = 0; i < hdr.numBlocks; i++) {
		if (blocks[i]->blockType == NIFBLOCK_NINODE) {
			NifBlockNiNode* node = (NifBlockNiNode*)blocks[i];
			if (!name.compare(node->nodeName))
				return (node);
		}
	}
	return NULL;
}

int NifFile::nodeIdForName(const string& name) {
	for (int i = 0; i < hdr.numBlocks; i++) {
		if (blocks[i]->blockType == NIFBLOCK_NINODE) {
			NifBlockNiNode* node = (NifBlockNiNode*)blocks[i];
			if (!name.compare(node->nodeName))
				return (i);
		}
	}
	return -1;
}

int NifFile::shapeBoneIndex(const string& shapeName, const string& boneName) {
	int skinRef;
	NifBlockTriShape* shape = shapeForName(shapeName);
	if (!shape) {
		NifBlockTriStrips* strips = stripsForName(shapeName);
		if (!strips)
			return -1;
		skinRef = strips->skinRef;
	}
	else
		skinRef = shape->skinRef;

	if (skinRef == -1)
		return -1;

	NifBlockBSDismemberment* bsdSkinInst = dynamic_cast<NifBlockBSDismemberment*>(blocks[skinRef]);
	if (!bsdSkinInst) {
		NifBlockNiSkinInstance* niSkinInst = dynamic_cast<NifBlockNiSkinInstance*>(blocks[skinRef]);
		if (!niSkinInst)
			return -1;

		for (int i = 0; i < niSkinInst->bonePtrs.size(); i++) {
			int boneBlock = niSkinInst->bonePtrs[i];
			if (boneBlock != -1) {
				NifBlockNiNode* node = (NifBlockNiNode*)blocks[boneBlock];
				if (node->nodeName == boneName)
					return i;
			}
		}
	}
	else {
		for (int i = 0; i < bsdSkinInst->bonePtrs.size(); i++) {
			int boneBlock = bsdSkinInst->bonePtrs[i];
			if (boneBlock != -1) {
				NifBlockNiNode* node = (NifBlockNiNode*)blocks[boneBlock];
				if (node->nodeName == boneName)
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
	this->fileName = other.fileName;
	this->hdr = other.hdr;
	NifBlock* nb;
	for (int i = 0; i < other.blocks.size(); i++) {
		switch (other.blocks[i]->blockType) {
		case NIFBLOCK_UNKNOWN:
			nb = new NifBlockUnknown(other.blocks[i]->CalcBlockSize());
			((NifBlockUnknown*)nb)->Clone((NifBlockUnknown*)other.blocks[i]);
			break;
		case NIFBLOCK_TRISHAPE:
			nb = new NifBlockTriShape((*(NifBlockTriShape*)other.blocks[i]));
			break;
		case NIFBLOCK_TRISHAPEDATA:
			nb = new NifBlockTriShapeData((*(NifBlockTriShapeData*)other.blocks[i]));
			break;
		case NIFBLOCK_TRISTRIPS:
			nb = new NifBlockTriStrips((*(NifBlockTriStrips*)other.blocks[i]));
			break;
		case NIFBLOCK_TRISTRIPSDATA:
			nb = new NifBlockTriStripsData((*(NifBlockTriStripsData*)other.blocks[i]));
			break;
		case NIFBLOCK_NINODE:
			nb = new NifBlockNiNode((*(NifBlockNiNode*)other.blocks[i]));
			break;
		case NIFBLOCK_BSDISMEMBER:
			nb = new NifBlockBSDismemberment((*(NifBlockBSDismemberment*)other.blocks[i]));
			break;
		case NIFBLOCK_NISKINPARTITION:
			nb = new NifBlockNiSkinPartition((*(NifBlockNiSkinPartition*)other.blocks[i]));
			break;
		case NIFBLOCK_NISKININSTANCE:
			nb = new NifBlockNiSkinInstance((*(NifBlockNiSkinInstance*)other.blocks[i]));
			break;
		case NIFBLOCK_BSLGTSHADEPROP:
			nb = new NifBlockBSLightShadeProp((*(NifBlockBSLightShadeProp*)other.blocks[i]));
			break;
		case NIFBLOCK_ALPHAPROPERTY:
			nb = new NifBlockAlphaProperty((*(NifBlockAlphaProperty*)other.blocks[i]));
			break;
		case NIFBLOCK_NISKINDATA:
			nb = new NifBlockNiSkinData((*(NifBlockNiSkinData*)other.blocks[i]));
			break;
		case NIFBLOCK_BSSHADERTEXSET:
			nb = new NifBlockBSShaderTextureSet((*(NifBlockBSShaderTextureSet*)other.blocks[i]));
			break;
		case NIFBLOCK_STRINGEXTRADATA:
			nb = new NifBlockStringExtraData((*(NifBlockStringExtraData*)other.blocks[i]));
		}
		blocks.push_back(nb);
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
	NifBlock* block = NULL;
	string thisBlockTypeStr;
	unsigned short thisBlockTypeId;
	Clear();

	fstream file(filename.c_str(), ios_base::in | ios_base::binary);
	if (file.is_open()) {
		if (filename.rfind("\\") != string::npos)
			fileName = filename.substr(filename.rfind("\\"));
		else
			fileName = filename;
		hdr.Get(file, hdr);
		if (hdr.verStr[0] == 0) {
			isValid = false;
			return 1;
		}
		hdr.blocks = &blocks;

		for (int i = 0; i < hdr.numBlocks; i++) {
			thisBlockTypeId = hdr.blockIndex[i];
			thisBlockTypeStr = hdr.blockTypes[thisBlockTypeId].str;
			if (!thisBlockTypeStr.compare("NiTriShapeData")) {
				block = (NifBlock*) new NifBlockTriShapeData(file, hdr);
				block->SetBlockSize(hdr.blockSizes[i]);
			}
			else if (!thisBlockTypeStr.compare("NiTriShape")) {
				block = (NifBlock*) new NifBlockTriShape(file, hdr);
				block->SetBlockSize(hdr.blockSizes[i]);
			}
			else if (!thisBlockTypeStr.compare("NiTriStrips")) {
				block = (NifBlock*) new NifBlockTriStrips(file, hdr);
				block->SetBlockSize(hdr.blockSizes[i]);
			}
			else if (!thisBlockTypeStr.compare("NiTriStripsData")) {
				block = (NifBlock*) new NifBlockTriStripsData(file, hdr);
				block->SetBlockSize(hdr.blockSizes[i]);
			}
			else if (!thisBlockTypeStr.compare("BSDismemberSkinInstance")) {
				block = (NifBlock*) new NifBlockBSDismemberment(file, hdr);
				block->SetBlockSize(hdr.blockSizes[i]);
			}
			else if (!thisBlockTypeStr.compare("NiNode")) {
				block = (NifBlock*) new NifBlockNiNode(file, hdr);
				block->SetBlockSize(hdr.blockSizes[i]);
			}
			else if (!thisBlockTypeStr.compare("NiSkinData")) {
				block = (NifBlock*) new NifBlockNiSkinData(file, hdr);
				block->SetBlockSize(hdr.blockSizes[i]);
			}
			else if (!thisBlockTypeStr.compare("NiSkinPartition")) {
				block = (NifBlock*) new NifBlockNiSkinPartition(file, hdr);
				block->SetBlockSize(hdr.blockSizes[i]);
			}
			else if (!thisBlockTypeStr.compare("NiSkinInstance")) {
				block = (NifBlock*) new NifBlockNiSkinInstance(file, hdr);
				block->SetBlockSize(hdr.blockSizes[i]);
			}
			else if (!thisBlockTypeStr.compare("BSLightingShaderProperty")) {
				block = (NifBlock*) new NifBlockBSLightShadeProp(file, hdr);
				block->SetBlockSize(hdr.blockSizes[i]);
			}
			else if (!thisBlockTypeStr.compare("BSShaderTextureSet")) {
				block = (NifBlock*) new NifBlockBSShaderTextureSet(file, hdr);
				block->SetBlockSize(hdr.blockSizes[i]);
			}
			else if (!thisBlockTypeStr.compare("NiStringExtraData")) {
				block = (NifBlock*) new NifBlockStringExtraData(file, hdr);
				block->SetBlockSize(hdr.blockSizes[i]);
			}
			else {
				block = (NifBlock*) new NifBlockUnknown(file, hdr.blockSizes[i]);
			}
			blocks.push_back(block);
		}
		file.close();
	}
	else {
		isValid = false;
		return 1;
	}
	TrimTexturePaths();
	isValid = true;
	return 0;
}

void NifFile::DeleteBlock(int blockIndex) {
	if (blockIndex == -1)
		return;

	unsigned short blockTypeId = hdr.blockIndex[blockIndex];
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
		blocks[i]->notifyBlockDelete(blockIndex, hdr);
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

int NifFile::AddBlock(NifBlock* newBlock, const string& blockTypeName) {
	ushort btID = this->AddOrFindBlockTypeId(blockTypeName);
	hdr.blockIndex.push_back(btID);
	hdr.blockSizes.push_back(newBlock->CalcBlockSize());
	blocks.push_back(newBlock);
	hdr.numBlocks = blocks.size();
	return hdr.numBlocks - 1;
}

NifBlock* NifFile::GetBlock(int blockId) {
	if (blockId >= 0 && blockId < hdr.numBlocks)
		return blocks[blockId];
	return NULL;
}

int NifFile::AddNode(const string& nodeName, vector<vector3>& rot, vector3& trans, float scale) {
	NifBlockNiNode* newNode = new NifBlockNiNode();
	newNode->rotation[0] = rot[0];
	newNode->rotation[1] = rot[1];
	newNode->rotation[2] = rot[2];
	newNode->translation = trans;
	newNode->scale = scale;
	newNode->nameID = AddOrFindStringId(nodeName);
	newNode->nodeName = nodeName;

	int newNodeId = AddBlock(newNode, "NiNode");
	if (newNodeId != -1) {
		NifBlockNiNode* rootNode = (NifBlockNiNode*)blocks[0];
		rootNode->children.push_back(newNodeId);
		rootNode->numChildren++;
		rootNode->CalcBlockSize();
		hdr.blockSizes[0] += 4;
	}
	return newNodeId;
}

string NifFile::NodeName(int blockID) {
	NifBlockNiNode* n = dynamic_cast<NifBlockNiNode*>(blocks[blockID]);
	if (!n)
		return "";
	if (n->nodeName.empty())
		return "_unnamed_";
	return n->nodeName;
}

int NifFile::AddStringExtraData(const string& shapeName, const string& name, const string& stringData) {
	NifBlockStringExtraData* strExtraData = new NifBlockStringExtraData();
	strExtraData->nameID = AddOrFindStringId(name);
	strExtraData->name = name;
	strExtraData->stringDataId = AddOrFindStringId(stringData);
	strExtraData->stringData = stringData;
	//strExtraData->bytesRemaining = stringData.size() + 4;

	int strExtraDataId = AddBlock(strExtraData, "NiStringExtraData");
	if (strExtraDataId != -1) {
		int id = shapeIdForName(shapeName);
		if (id == -1)
			return -1;

		NifBlockTriShape* shape = shapeForName(shapeName);
		if (shape) {
			shape->extradata.push_back(strExtraDataId);
			shape->numExtra++;
			shape->SetBlockSize(shape->CalcBlockSize() + 4);
			hdr.blockSizes[id] = shape->CalcBlockSize();
		}
		else {
			NifBlockTriStrips* strips = stripsForName(shapeName);
			if (!strips)
				return -1;
			strips->extradata.push_back(strExtraDataId);
			strips->numExtra++;
			strips->SetBlockSize(strips->CalcBlockSize() + 4);
			hdr.blockSizes[id] = strips->CalcBlockSize();
		}
	}
	return strExtraDataId;
}

NifBlockBSLightShadeProp* NifFile::GetShader(const string& shaderName) {
	for (int i = 0; i < blocks.size(); i++){
		if (blocks[i]->blockType == NIFBLOCK_BSLGTSHADEPROP) {
			NifBlockBSLightShadeProp* result = dynamic_cast<NifBlockBSLightShadeProp*>(blocks[i]);
			if (result->shaderName == shaderName)
				return result;
		}
	}
	return NULL;
}

NifBlockBSLightShadeProp* NifFile::GetShaderForShape(const string& shapeName) {
	NifBlockTriShape* shape = shapeForName(shapeName);
	int prop1;
	if (!shape) {
		NifBlockTriStrips* strips = stripsForName(shapeName);
		if (!strips)
			return NULL;
		prop1 = strips->propertiesRef1;
	}
	else {
		prop1 = shape->propertiesRef1;
	}
	if (prop1 == -1)
		return NULL;

	NifBlockBSLightShadeProp* shader = dynamic_cast<NifBlockBSLightShadeProp*>(blocks[prop1]);
	return shader;
}

bool NifFile::GetTextureForShape(const string& shapeName, string& outTexFile, int texIndex) {
	auto shader = GetShaderForShape(shapeName);
	if (!shader || shader->texsetRef == -1)
		return false;

	NifBlockBSShaderTextureSet* ts = dynamic_cast<NifBlockBSShaderTextureSet*>(blocks[shader->texsetRef]);
	if (!ts)
		return false;

	outTexFile = ts->textures[texIndex].str;
	return true;
}

void NifFile::SetTextureForShape(const string& shapeName, string& outTexFile, int texIndex) {
	auto shader = GetShaderForShape(shapeName);
	if (!shader || shader->texsetRef == -1)
		return;

	NifBlockBSShaderTextureSet* ts = dynamic_cast<NifBlockBSShaderTextureSet*>(blocks[shader->texsetRef]);
	if (!ts)
		return;

	ts->textures[texIndex].str = outTexFile;
	hdr.blockSizes[shader->texsetRef] = ts->CalcBlockSize();
}

void NifFile::TrimTexturePaths() {
	string tFile;
	vector<string> shapes;
	GetShapeList(shapes);

	for (auto s : shapes) {
		auto shader = GetShaderForShape(s);
		if (shader) {
			for (int i = 0; i < 9; i++) {
				GetTextureForShape(s, tFile, i);
				tFile = regex_replace(tFile, regex("/+|\\\\+"), "\\"); // Replace multiple slashes or forward slashes with one backslash
				tFile = regex_replace(tFile, regex(".*textures\\\\", regex_constants::icase), ""); // Remove everything before and including the texture path
				tFile = regex_replace(tFile, regex("AstridBody", regex_constants::icase), "femalebody_1"); // Change astrid body to femalebody_1
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
	NifString nstr;
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
	NifString ns;
	ns.outputNull = true;
	ns.str = str;
	hdr.strings.push_back(ns);
	hdr.numStrings++;
	if (str.length() > hdr.maxStringLen)
		hdr.maxStringLen = str.length();
	return r;
}

void NifFile::CopyShader(const string& shapeDest, string& shaderName, NifFile& srcNif, bool addAlpha) {
	NifBlockTriShape* shape = shapeForName(shapeDest);
	if (!shape) {
		NifBlockTriStrips* strips = stripsForName(shapeDest);
		if (!strips)
			return;
	}

	// Get source block info
	auto srcShader = srcNif.GetShader(shaderName);
	if (!srcShader)
		return;

	NifBlockBSLightShadeProp* shader = dynamic_cast<NifBlockBSLightShadeProp*>(srcShader);
	if (!shader)
		return;

	CopyShader(shapeDest, shader, srcNif, addAlpha);
}
	
void NifFile::CopyShader(const string& shapeDest, NifBlockBSLightShadeProp* srcShader, NifFile& srcNif, bool addAlpha) {
	int dataRef;
	int* props1, *props2;
	bool isStrips;
	NifBlockTriShape* shape = shapeForName(shapeDest);
	if (!shape) {
		NifBlockTriStrips* strips = stripsForName(shapeDest);
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

	// Get source block info
	NifBlockBSShaderTextureSet* srcTexSet = dynamic_cast<NifBlockBSShaderTextureSet*>(srcNif.GetBlock(srcShader->texsetRef));
	bool texSetFound = false;
	if (srcTexSet)
		texSetFound = true;

	// Create destination shader block and copy
	NifBlockBSLightShadeProp* destShader = new NifBlockBSLightShadeProp();
	destShader->Clone(srcShader);
	destShader->nameID = (uint)-1;

	// Add shader block to nif
	int shaderId = blocks.size();
	blocks.push_back(destShader);
	hdr.numBlocks++;

	NifBlockBSShaderTextureSet* destTexSet = NULL;
	int texsetId;

	if (texSetFound) {
		// Create texture set block and copy
		destTexSet = new NifBlockBSShaderTextureSet((*srcTexSet));

		// Add texture block to nif
		texsetId = blocks.size();
		blocks.push_back(destTexSet);
		hdr.numBlocks++;

		// Assign textureset block id to shader
		destShader->texsetRef = texsetId;
	}

	(*props1) = shaderId;
	if (destShader->IsSkinShader()) { // Skin shader. Kill normals, set numUVs to 1
		if (isStrips) {
			NifBlockTriStripsData* stripsData = ((NifBlockTriStripsData*)blocks[dataRef]);
			stripsData->hasNormals = 0;
			stripsData->normals.clear();
			stripsData->binormals.clear();
			stripsData->tangents.clear();
			stripsData->numUVs = 1;
			hdr.blockSizes[dataRef] = stripsData->CalcBlockSize();
		}
		else {
			NifBlockTriShapeData* shapeData = ((NifBlockTriShapeData*)blocks[dataRef]);
			shapeData->hasNormals = 0;
			shapeData->normals.clear();
			shapeData->binormals.clear();
			shapeData->tangents.clear();
			shapeData->numUVs = 1;
			hdr.blockSizes[dataRef] = shapeData->CalcBlockSize();
		}
	}

	// Add shader and textureset sizes to block size info
	hdr.blockSizes.push_back(destShader->CalcBlockSize());
	if (texSetFound) {
		hdr.blockSizes.push_back(destTexSet->CalcBlockSize());
	}
	if (addAlpha) {
		NifBlockAlphaProperty* alphaProp = new NifBlockAlphaProperty;
		(*props2) = blocks.size();
		blocks.push_back(alphaProp);
		hdr.numBlocks++;
		hdr.blockSizes.push_back(alphaProp->CalcBlockSize());
	}

	ushort shaderTypeId = AddOrFindBlockTypeId("BSLightingShaderProperty");
	// Record the block type in the block type index.
	hdr.blockIndex.push_back(shaderTypeId);

	if (texSetFound) {
		ushort texSetTypeId = AddOrFindBlockTypeId("BSShaderTextureSet");
		// Record the block type in the block type index
		hdr.blockIndex.push_back(texSetTypeId);
	}

	if (addAlpha) {
		ushort alphaBlockTypeId = AddOrFindBlockTypeId("NiAlphaProperty");
		hdr.blockIndex.push_back(alphaBlockTypeId);
	}
}

int NifFile::CopyNamedNode(string& nodeName, NifFile& srcNif) {
	NifBlockNiNode* srcNode = srcNif.nodeForName(nodeName);
	if (!srcNode)
		return -1;

	NifBlockNiNode* destNode = new NifBlockNiNode((*srcNode));
	int strPtr = AddOrFindStringId(nodeName);
	destNode->nameID = strPtr;
	destNode->nodeName = nodeName;

	int blockID = blocks.size();
	blocks.push_back(destNode);
	hdr.blockSizes.push_back(destNode->CalcBlockSize());
	hdr.blockIndex.push_back(AddOrFindBlockTypeId("NiNode"));
	hdr.numBlocks++;

	return blockID;
}

void NifFile::CopyStrips(const string& shapeDest, NifFile& srcNif, const string& srcShape) {
	NifBlockTriStrips* src = srcNif.stripsForName(srcShape);
	if (!src)
		return;

	bool skipSkinInst = false;
	if (src->skinRef == -1)
		skipSkinInst = true;

	NifBlockTriStripsData* srcData = (NifBlockTriStripsData*)srcNif.GetBlock(src->dataRef);
	NifBlockBSLightShadeProp* srcShader = dynamic_cast<NifBlockBSLightShadeProp*>(srcNif.GetBlock(src->propertiesRef1));
	NifBlockBSDismemberment* srcBSDSkinInst = NULL;
	NifBlockNiSkinInstance* srcNiSkinInst = NULL;
	NifBlockNiSkinData* srcSkinData = NULL;
	NifBlockNiSkinPartition* srcSkinPart = NULL;

	NifBlockBSDismemberment* destBSDSkinInst = NULL;
	NifBlockNiSkinInstance* destNiSkinInst = NULL;
	NifBlockNiSkinData* destSkinData = NULL;
	NifBlockNiSkinPartition* destSkinPart = NULL;

	bool skipBSDSkinInst = false;
	if (!skipSkinInst) {
		srcBSDSkinInst = dynamic_cast<NifBlockBSDismemberment*>(srcNif.GetBlock(src->skinRef));
		if (!srcBSDSkinInst) {
			skipBSDSkinInst = true;
			srcNiSkinInst = dynamic_cast<NifBlockNiSkinInstance*>(srcNif.GetBlock(src->skinRef));
			if (!srcNiSkinInst)
				skipSkinInst = true;

			if (!skipSkinInst) {
				srcSkinData = (NifBlockNiSkinData*)srcNif.GetBlock(srcNiSkinInst->dataRef);
				srcSkinPart = (NifBlockNiSkinPartition*)srcNif.GetBlock(srcNiSkinInst->skinRef);
				destNiSkinInst = new NifBlockNiSkinInstance((*srcNiSkinInst));
			}
		}
		else {
			srcSkinData = (NifBlockNiSkinData*)srcNif.GetBlock(srcBSDSkinInst->dataRef);
			srcSkinPart = (NifBlockNiSkinPartition*)srcNif.GetBlock(srcBSDSkinInst->skinRef);
			destBSDSkinInst = new NifBlockBSDismemberment((*srcBSDSkinInst));
		}
	}

	NifBlockTriStrips* dest = new NifBlockTriStrips((*src));
	dest->nameID = AddOrFindStringId(shapeDest);
	dest->shapeName = shapeDest;

	NifBlockTriStripsData* destData = new NifBlockTriStripsData((*srcData));

	if (!skipSkinInst) {
		// Treat skinning and partition info as blobs of anonymous data.
		destSkinData = new NifBlockNiSkinData((*srcSkinData));
		destSkinPart = new NifBlockNiSkinPartition((*srcSkinPart));
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
	dest->skinRef = destSkinId;

	if (!skipSkinInst) {
		if (skipBSDSkinInst) {
			destNiSkinInst->dataRef = destSkinDataId;
			destNiSkinInst->skinRef = destSkinPartId;
		}
		else {
			destBSDSkinInst->dataRef = destSkinDataId;
			destBSDSkinInst->skinRef = destSkinPartId;
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

	if (srcShader) {
		bool shaderAlpha = false;
		if (src->propertiesRef2 != -1)
			shaderAlpha = true;
		CopyShader(shapeDest, srcShader, srcNif, shaderAlpha);
	}
	else
		dest->propertiesRef1 = -1;

	vector<string> srcBoneList;
	srcNif.GetShapeBoneList(srcShape, srcBoneList);
	if (!skipSkinInst) {
		if (skipBSDSkinInst)
			destNiSkinInst->bonePtrs.clear();
		else
			destBSDSkinInst->bonePtrs.clear();
	}

	int bonePtr;
	NifBlockNiNode* rootNode = (NifBlockNiNode*)blocks[0];
	for (auto boneName : srcBoneList) {
		bonePtr = nodeIdForName(boneName);
		if (bonePtr == -1) {
			bonePtr = CopyNamedNode(boneName, srcNif);
			rootNode->children.push_back(bonePtr);
			rootNode->numChildren++;
			hdr.blockSizes[0] = rootNode->CalcBlockSize();
		}
		if (skipBSDSkinInst)
			destNiSkinInst->bonePtrs.push_back(bonePtr);
		else
			destBSDSkinInst->bonePtrs.push_back(bonePtr);
	}

	rootNode->children.push_back(destId);
	rootNode->numChildren++;
	hdr.blockSizes[0] = rootNode->CalcBlockSize();
}

void NifFile::CopyShape(const string& shapeDest, NifFile& srcNif, const string& srcShape) {
	NifBlockTriShape* src = srcNif.shapeForName(srcShape);
	if (!src) {
		CopyStrips(shapeDest, srcNif, srcShape);
		return;
	}

	bool skipSkinInst = false;
	if (src->skinRef == -1)
		skipSkinInst = true;

	NifBlockTriShapeData* srcData = (NifBlockTriShapeData*)srcNif.GetBlock(src->dataRef);
	NifBlockBSLightShadeProp* srcShader = dynamic_cast<NifBlockBSLightShadeProp*>(srcNif.GetBlock(src->propertiesRef1));
	NifBlockBSDismemberment* srcBSDSkinInst = NULL;
	NifBlockNiSkinInstance* srcNiSkinInst = NULL;
	NifBlockNiSkinData* srcSkinData = NULL;
	NifBlockNiSkinPartition* srcSkinPart = NULL;

	NifBlockBSDismemberment* destBSDSkinInst = NULL;
	NifBlockNiSkinInstance* destNiSkinInst = NULL;
	NifBlockNiSkinData* destSkinData = NULL;
	NifBlockNiSkinPartition* destSkinPart = NULL;

	bool skipBSDSkinInst = false;
	if (!skipSkinInst) {
		srcBSDSkinInst = dynamic_cast<NifBlockBSDismemberment*>(srcNif.GetBlock(src->skinRef));
		if (!srcBSDSkinInst) {
			skipBSDSkinInst = true;
			srcNiSkinInst = dynamic_cast<NifBlockNiSkinInstance*>(srcNif.GetBlock(src->skinRef));
			if (!srcNiSkinInst)
				skipSkinInst = true;

			if (!skipSkinInst) {
				srcSkinData = (NifBlockNiSkinData*)srcNif.GetBlock(srcNiSkinInst->dataRef);
				srcSkinPart = (NifBlockNiSkinPartition*)srcNif.GetBlock(srcNiSkinInst->skinRef);
				destNiSkinInst = new NifBlockNiSkinInstance((*srcNiSkinInst));
			}
		}
		else {
			srcSkinData = (NifBlockNiSkinData*)srcNif.GetBlock(srcBSDSkinInst->dataRef);
			srcSkinPart = (NifBlockNiSkinPartition*)srcNif.GetBlock(srcBSDSkinInst->skinRef);
			destBSDSkinInst = new NifBlockBSDismemberment((*srcBSDSkinInst));
		}
	}

	NifBlockTriShape* dest = new NifBlockTriShape((*src));
	dest->nameID = AddOrFindStringId(shapeDest);
	dest->shapeName = shapeDest;

	NifBlockTriShapeData* destData = new NifBlockTriShapeData((*srcData));

	if (!skipSkinInst) {
		// Treat skinning and partition info as blobs of anonymous data.
		destSkinData = new NifBlockNiSkinData((*srcSkinData));
		destSkinPart = new NifBlockNiSkinPartition((*srcSkinPart));
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
	dest->skinRef = destSkinId;

	if (!skipSkinInst) {
		if (skipBSDSkinInst) {
			destNiSkinInst->dataRef = destSkinDataId;
			destNiSkinInst->skinRef = destSkinPartId;
		}
		else {
			destBSDSkinInst->dataRef = destSkinDataId;
			destBSDSkinInst->skinRef = destSkinPartId;
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

	if (srcShader) {
		bool shaderAlpha = false;
		if (src->propertiesRef2 != -1)
			shaderAlpha = true;
		CopyShader(shapeDest, srcShader, srcNif, shaderAlpha);
	}
	else
		dest->propertiesRef1 = -1;

	vector<string> srcBoneList;
	srcNif.GetShapeBoneList(srcShape, srcBoneList);
	if (!skipSkinInst) {
		if (skipBSDSkinInst)
			destNiSkinInst->bonePtrs.clear();
		else
			destBSDSkinInst->bonePtrs.clear();
	}

	int bonePtr;
	NifBlockNiNode* rootNode = (NifBlockNiNode*)blocks[0];
	for (auto boneName : srcBoneList) {
		bonePtr = nodeIdForName(boneName);
		if (bonePtr == -1) {
			bonePtr = CopyNamedNode(boneName, srcNif);
			rootNode->children.push_back(bonePtr);
			rootNode->numChildren++;
			hdr.blockSizes[0] = rootNode->CalcBlockSize();
		}
		if (skipBSDSkinInst)
			destNiSkinInst->bonePtrs.push_back(bonePtr);
		else
			destBSDSkinInst->bonePtrs.push_back(bonePtr);
	}

	rootNode->children.push_back(destId);
	rootNode->numChildren++;
	hdr.blockSizes[0] = rootNode->CalcBlockSize();
}

int NifFile::Save(const string& filename) {
	fstream file(filename.c_str(), ios_base::out | ios_base::binary);
	if (file.is_open()) {
		hdr.Put(file, hdr);
		for (int i = 0; i < hdr.numBlocks; i++)
			blocks[i]->Put(file, hdr);

		uint endPad = 1;
		file.write((char*)&endPad, 4);
		endPad = 0;
		file.write((char*)&endPad, 4);
		file.close();
	} else
		return 1;

	return 0;
}

int NifFile::ExportShapeObj(const string& filename, const string& shape, float scale, vector3 offset) {
	ofstream file(filename.c_str(), ios_base::binary);
	if (file.fail())
		return 1;

	const vector<vector3>* verts = GetRawVertsForShape(shape);
	const vector<triangle>* tris = GetTrisForShape(shape);
	const vector<vector2>* uvs = GetUvsForShape(shape);

	vec3 shapeTrans;
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

	for (int i = 0; i < tris->size(); i++) {
		file << "f " << tris->at(i).p1 + 1 << "/" << tris->at(i).p1 + 1 << " "
			<< tris->at(i).p2 + 1 << "/" << tris->at(i).p2 + 1 << " "
			<< tris->at(i).p3 + 1 << "/" << tris->at(i).p3 + 1
			<< endl;
	}
	file << endl;

	file.close();
	return 0;
}

void NifFile::RecalculateNormals() {
	for (int i = 0; i < hdr.numBlocks; i++)
		if (blocks[i]->blockType == NIFBLOCK_TRISHAPEDATA)
			((NifBlockTriShapeData*)blocks[i])->RecalcNormals();
}

void NifFile::SetUserVersions(unsigned int ver1, unsigned int ver2) {
	hdr.userVer = ver1;
	hdr.userVer2 = ver2;
	for (int i = 0; i < hdr.numBlocks; i++) {
		blocks[i]->notifyVersionChange(ver1, ver2);
		hdr.blockSizes[i] = blocks[i]->CalcBlockSize();
	}
}

int NifFile::GetShapeList(vector<string>& outList) {
	outList.clear();
	for (int i = 0; i < hdr.numBlocks; i++) {
		if (blocks[i]->blockType == NIFBLOCK_TRISHAPE) {
			NifBlockTriShape* shape = (NifBlockTriShape*)blocks[i];
			outList.push_back(shape->shapeName);
		}
		else if (blocks[i]->blockType == NIFBLOCK_TRISTRIPS) {
			NifBlockTriStrips* strips = (NifBlockTriStrips*)blocks[i];
			outList.push_back(strips->shapeName);
		}
	}
	return outList.size();
}

void NifFile::RenameShape(const string& oldName, const string& newName) {
	uint strID;
	NifBlockTriShape* shape = shapeForName(oldName);
	if (shape) {
		strID = shape->nameID;
		shape->shapeName = newName;
	}
	else {
		NifBlockTriStrips* strips = stripsForName(oldName);
		if (!strips)
			return;
		strID = strips->nameID;
		strips->shapeName = newName;
	}

	hdr.strings[strID].str = newName;
	if (hdr.maxStringLen < newName.length())
		hdr.maxStringLen = newName.length();
}

void NifFile::RenameDuplicateShape(const string& dupedShape) {
	NifBlockTriShape* shape = shapeForName(dupedShape);
	int dupCount = 1;
	char buf[10];
	if (shape) {
		while ((shape = shapeForName(dupedShape, 1)) != NULL) {
			_snprintf(buf, 10, "_%d", dupCount);
			while (FindStringId(shape->shapeName + buf) != -1) {
				dupCount++;
				_snprintf(buf, 10, "_%d", dupCount);
			}
			shape->shapeName = shape->shapeName + buf;
			shape->nameID = AddOrFindStringId(shape->shapeName);
			dupCount++;
		}
	}
	else {
		NifBlockTriStrips* strips = stripsForName(dupedShape);
		if (!strips)
			return;

		while ((strips = stripsForName(dupedShape, 1)) != NULL) {
			_snprintf(buf, 10, "_%d", dupCount);
			while (FindStringId(strips->shapeName + buf) != -1) {
				dupCount++;
				_snprintf(buf, 10, "_%d", dupCount);
			}
			strips->shapeName = strips->shapeName + buf;
			strips->nameID = AddOrFindStringId(strips->shapeName);
			dupCount++;
		}
	}
}

void NifFile::SetNodeName(int blockID, const string& newName) {
	NifBlockNiNode* node = dynamic_cast<NifBlockNiNode*>(blocks[blockID]);
	if (!node)
		return;

	uint strID = node->nameID;
	node->nodeName = newName;
	hdr.strings[strID].str = newName;
	if (hdr.maxStringLen < newName.length())
		hdr.maxStringLen = newName.length();
}

int NifFile::GetShaderList(vector<string>& outList) {
	outList.clear();
	for (int i = 0; i < hdr.numBlocks; i++) {
		if (blocks[i]->blockType == NIFBLOCK_BSLGTSHADEPROP) {
			NifBlockBSLightShadeProp* shader = dynamic_cast<NifBlockBSLightShadeProp*>(blocks[i]);
			outList.push_back(shader->shaderName);
		}
	}
	return outList.size();
}
	
int NifFile::GetNodeID(const string& nodeName) {
	for (int i = 0; i < hdr.numBlocks; i++) {
		if (blocks[i]->blockType == NIFBLOCK_NINODE) {
			NifBlockNiNode* node = (NifBlockNiNode*)blocks[i];
			if (!node->nodeName.compare(nodeName))
				return i;
		}
	}
	return -1;
}

bool NifFile::GetNodeTransform(const string& nodeName, vector<vector3>& outRot, vector3& outTrans, float& outScale) {
	for (int i = 0; i < hdr.numBlocks; i++) {
		if (blocks[i]->blockType == NIFBLOCK_NINODE) {
			NifBlockNiNode* node = (NifBlockNiNode*)blocks[i];
			if (!node->nodeName.compare(nodeName)) {
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

bool NifFile::SetNodeTransform(const string& nodeName, skin_transform& inXform) {
	for (int i = 0; i < hdr.numBlocks; i++) {
		if (blocks[i]->blockType == NIFBLOCK_NINODE) {
			NifBlockNiNode* node = (NifBlockNiNode*)blocks[i];
			if (!node->nodeName.compare(nodeName)) {
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
	NifBlockTriShape* shape = shapeForName(shapeName);
	if (shape) {
		skinRef = shape->skinRef;
	}
	else {
		NifBlockTriStrips* strips = stripsForName(shapeName);
		if (!strips)
			return 0;
		skinRef = strips->skinRef;
	}
	if (skinRef == -1)
		return 0;

	NifBlockBSDismemberment* bsdSkinInst = dynamic_cast<NifBlockBSDismemberment*>(blocks[skinRef]);
	NifBlockNiSkinInstance* niSkinInst = NULL;
	bool skipBSDSkinInst = false;
	if (!bsdSkinInst) {
		skipBSDSkinInst = true;
		niSkinInst = dynamic_cast<NifBlockNiSkinInstance*>(blocks[skinRef]);
		if (!niSkinInst)
			return 0;
	}

	int boneBlock;
	if (skipBSDSkinInst) {
		for (int i = 0; i < niSkinInst->bonePtrs.size(); i++) {
			boneBlock = niSkinInst->bonePtrs[i];
			if (boneBlock != -1) {
				NifBlockNiNode* node = (NifBlockNiNode*)blocks[boneBlock];
				outList.push_back(node->nodeName);
			}
		}
	}
	else {
		for (int i = 0; i < bsdSkinInst->bonePtrs.size(); i++) {
			boneBlock = bsdSkinInst->bonePtrs[i];
			if (boneBlock != -1) {
				NifBlockNiNode* node = (NifBlockNiNode*)blocks[boneBlock];
				outList.push_back(node->nodeName);
			}
		}
	}
	return outList.size();
}

int NifFile::GetShapeBoneIDList(const string& shapeName, vector<int>& outList) {
	outList.clear();

	int skinRef = -1;
	NifBlockTriShape* shape = shapeForName(shapeName);
	if (shape) {
		skinRef = shape->skinRef;
	}
	else {
		NifBlockTriStrips* strips = stripsForName(shapeName);
		if (!strips)
			return 0;
		skinRef = strips->skinRef;
	}
	if (skinRef == -1)
		return 0;

	NifBlockBSDismemberment* bsdSkinInst = dynamic_cast<NifBlockBSDismemberment*>(blocks[skinRef]);
	NifBlockNiSkinInstance* niSkinInst = NULL;
	bool skipBSDSkinInst = false;
	if (!bsdSkinInst) {
		skipBSDSkinInst = true;
		niSkinInst = dynamic_cast<NifBlockNiSkinInstance*>(blocks[skinRef]);
		if (!niSkinInst)
			return 0;
	}

	int boneBlock;
	if (skipBSDSkinInst) {
		for (int i = 0; i < niSkinInst->bonePtrs.size(); i++) {
			boneBlock = niSkinInst->bonePtrs[i];
			outList.push_back(boneBlock);
		}
	}
	else {
		for (int i = 0; i < bsdSkinInst->bonePtrs.size(); i++) {
			boneBlock = bsdSkinInst->bonePtrs[i];
			outList.push_back(boneBlock);
		}
	}
	return outList.size();
}

void NifFile::SetShapeBoneIDList(const string& shapeName, vector<int>& inList) {
	int skinRef = -1;
	NifBlockTriShape* shape = shapeForName(shapeName);
	if (shape) {
		skinRef = shape->skinRef;
	}
	else {
		NifBlockTriStrips* strips = stripsForName(shapeName);
		if (!strips) return;
		skinRef = strips->skinRef;
	}
	if (skinRef == -1)
		return;

	NifBlockBSDismemberment* bsdSkinInst = dynamic_cast<NifBlockBSDismemberment*>(blocks[skinRef]);
	NifBlockNiSkinInstance* niSkinInst = NULL;
	bool skipBSDSkinInst = false;
	if (!bsdSkinInst) {
		skipBSDSkinInst = true;
		niSkinInst = dynamic_cast<NifBlockNiSkinInstance*>(blocks[skinRef]);
		if (!niSkinInst)
			return;
	}

	NifBlockNiSkinData* skinData = NULL;
	if (skipBSDSkinInst) {
		niSkinInst->bonePtrs.clear();
		niSkinInst->numBones = 0;
		for (int i = 0; i < inList.size(); i++) {
			niSkinInst->bonePtrs.push_back(inList[i]);
			niSkinInst->numBones++;
		}
		hdr.blockSizes[skinRef] = niSkinInst->CalcBlockSize();

		skinData = (NifBlockNiSkinData*)blocks[niSkinInst->dataRef];
		int nBonesToAdd = niSkinInst->bonePtrs.size() - skinData->numBones;
		if (nBonesToAdd > 0) {
			for (int i = 0; i < nBonesToAdd; i++) {
				skinData->Bones.emplace_back();
				skinData->Bones.back().numVerts = 0;
			}
			hdr.blockSizes[niSkinInst->dataRef] = skinData->CalcBlockSize();
		}
	}
	else {
		bsdSkinInst->bonePtrs.clear();
		bsdSkinInst->numBones = 0;
		for (int i = 0; i < inList.size(); i++) {
			bsdSkinInst->bonePtrs.push_back(inList[i]);
			bsdSkinInst->numBones++;
		}
		hdr.blockSizes[skinRef] = bsdSkinInst->CalcBlockSize();

		skinData = (NifBlockNiSkinData*)blocks[bsdSkinInst->dataRef];
		int nBonesToAdd = bsdSkinInst->bonePtrs.size() - skinData->numBones;
		if (nBonesToAdd > 0) {
			for (int i = 0; i < nBonesToAdd; i++) {
				skinData->Bones.emplace_back();
				skinData->Bones.back().numVerts = 0;
			}
			hdr.blockSizes[bsdSkinInst->dataRef] = skinData->CalcBlockSize();
		}
	}
	skinData->numBones = skinData->Bones.size();
}

int NifFile::GetShapeBoneWeights(const string& shapeName, int boneIndex, unordered_map<ushort, float>& outWeights) {
	outWeights.clear();

	int skinRef;
	NifBlockTriShape* shape = shapeForName(shapeName);
	if (shape) {
		skinRef = shape->skinRef;
	}
	else {
		NifBlockTriStrips* strips = stripsForName(shapeName);
		if (!strips)
			return 0;
		skinRef = strips->skinRef;
	}
	if (skinRef == -1)
		return 0;

	NifBlockBSDismemberment* bsdSkinInst = dynamic_cast<NifBlockBSDismemberment*>(blocks[skinRef]);
	NifBlockNiSkinInstance* niSkinInst = NULL;
	bool skipBSDSkinInst = false;
	if (!bsdSkinInst) {
		skipBSDSkinInst = true;
		niSkinInst = dynamic_cast<NifBlockNiSkinInstance*>(blocks[skinRef]);
		if (!niSkinInst || niSkinInst->skinRef == -1)
			return 0;
	}
	else if (bsdSkinInst->skinRef == -1)
			return 0;

	NifBlockNiSkinData* skinData = NULL;
	if (skipBSDSkinInst)
		skinData = (NifBlockNiSkinData*)blocks[niSkinInst->dataRef];
	else
		skinData = (NifBlockNiSkinData*)blocks[bsdSkinInst->dataRef];

	if (boneIndex > skinData->Bones.size())
		return 0;

	NifBlockNiSkinData::BoneData* bone = &skinData->Bones[boneIndex];
	for (auto sw : bone->skin_weights) {
		outWeights[sw.index] = sw.weight;
		if (sw.weight < 0.0001f)
			outWeights[sw.index] = 0.0f;
	}
	return outWeights.size();
}

bool NifFile::GetShapeBoneTransform(const string& shapeName, const string& boneName, skin_transform& outXform, vector3& outSphereOffset, float& outSphereRadius) {
	int boneIndex = shapeBoneIndex(shapeName, boneName);
	if (boneName.empty())
		boneIndex = -1;

	return GetShapeBoneTransform(shapeName, boneIndex, outXform, outSphereOffset, outSphereRadius);
}

bool NifFile::SetShapeBoneTransform(const string& shapeName, int boneIndex, skin_transform& inXform, vector3& inSphereOffset, float inSphereRadius) {
	int skinRef;
	NifBlockTriShape* shape = shapeForName(shapeName);
	if (shape) {
		skinRef = shape->skinRef;
	}
	else {
		NifBlockTriStrips* strips = stripsForName(shapeName);
		if (!strips)
			return false;
		skinRef = strips->skinRef;
	}
	if (skinRef == -1)
		return false;

	NifBlockBSDismemberment* bsdSkinInst = dynamic_cast<NifBlockBSDismemberment*>(blocks[skinRef]);
	NifBlockNiSkinInstance* niSkinInst = NULL;
	bool skipBSDSkinInst = false;
	if (!bsdSkinInst) {
		skipBSDSkinInst = true;
		niSkinInst = dynamic_cast<NifBlockNiSkinInstance*>(blocks[skinRef]);
		if (!niSkinInst || niSkinInst->skinRef == -1)
			return false;
	}
	else if (bsdSkinInst->skinRef == -1)
		return false;

	NifBlockNiSkinData* skinData = NULL;
	if (skipBSDSkinInst)
		skinData = (NifBlockNiSkinData*)blocks[niSkinInst->dataRef];
	else
		skinData = (NifBlockNiSkinData*)blocks[bsdSkinInst->dataRef];

	if (boneIndex == -1) { // set the overall skin transform
		skinData->rootTransform = inXform;
		return true;
	}
	if (boneIndex > skinData->Bones.size())
		return false;

	NifBlockNiSkinData::BoneData* bone = &skinData->Bones[boneIndex];
	bone->boneTransform = inXform;
	bone->boundSphereOffset = inSphereOffset;
	bone->boundSphereRadius = inSphereRadius;
	return true;
}

bool NifFile::GetShapeBoneTransform(const string& shapeName, int boneIndex, skin_transform& outXform, vector3& outSphereOffset, float& outSphereRadius) {
	int skinRef;
	NifBlockTriShape* shape = shapeForName(shapeName);
	if (shape) {
		skinRef = shape->skinRef;
	}
	else {
		NifBlockTriStrips* strips = stripsForName(shapeName);
		if (!strips)
			return false;
		skinRef = strips->skinRef;
	}
	if (skinRef == -1)
		return false;

	NifBlockBSDismemberment* bsdSkinInst = dynamic_cast<NifBlockBSDismemberment*>(blocks[skinRef]);
	NifBlockNiSkinInstance* niSkinInst = NULL;
	bool skipBSDSkinInst = false;
	if (!bsdSkinInst) {
		skipBSDSkinInst = true;
		niSkinInst = dynamic_cast<NifBlockNiSkinInstance*>(blocks[skinRef]);
		if (!niSkinInst || niSkinInst->skinRef == -1)
			return false;
	}
	else if (bsdSkinInst->skinRef == -1)
		return false;

	NifBlockNiSkinData* skinData = NULL;
	if (skipBSDSkinInst)
		skinData = (NifBlockNiSkinData*)blocks[niSkinInst->dataRef];
	else
		skinData = (NifBlockNiSkinData*)blocks[bsdSkinInst->dataRef];

	if (boneIndex == -1) { // want the overall skin transform
		outXform = skinData->rootTransform;
		outSphereOffset = vec3(0.0f, 0.0f, 0.0f);
		outSphereRadius = 0.0f;
		return true;
	}

	if (boneIndex > skinData->Bones.size()) return 0;
	NifBlockNiSkinData::BoneData* bone = &skinData->Bones[boneIndex];
	outXform = bone->boneTransform;
	outSphereOffset = bone->boundSphereOffset;
	outSphereRadius = bone->boundSphereRadius;
	return true;
}

void NifFile::UpdateShapeBoneID(const string& shapeName, int oldID, int newID) {
	int skinRef;
	NifBlockTriShape* shape = shapeForName(shapeName);
	if (shape) {
		skinRef = shape->skinRef;
	}
	else {
		NifBlockTriStrips* strips = stripsForName(shapeName);
		if (!strips)
			return;
		skinRef = strips->skinRef;
	}
	if (skinRef == -1)
		return;

	NifBlockBSDismemberment* bsdSkinInst = dynamic_cast<NifBlockBSDismemberment*>(blocks[skinRef]);
	NifBlockNiSkinInstance* niSkinInst = NULL;
	bool skipBSDSkinInst = false;
	if (!bsdSkinInst) {
		skipBSDSkinInst = true;
		niSkinInst = dynamic_cast<NifBlockNiSkinInstance*>(blocks[skinRef]);
		if (!niSkinInst || niSkinInst->skinRef == -1)
			return;
	}
	else if (bsdSkinInst->skinRef == -1)
		return;

	if (skipBSDSkinInst) {
		for (auto &bp : niSkinInst->bonePtrs) {
			if (bp == oldID) {
				bp = newID;
				return;
			}
		}
	}
	else {
		for (auto &bp : bsdSkinInst->bonePtrs) {
			if (bp == oldID) {
				bp = newID;
				return;
			}
		}
	}
}

void NifFile::SetShapeBoneWeights(const string& shapeName, int boneIndex, unordered_map<ushort, float>& inWeights) {
	int skinRef;
	NifBlockTriShape* shape = shapeForName(shapeName);
	if (shape) {
		skinRef = shape->skinRef;
	}
	else {
		NifBlockTriStrips* strips = stripsForName(shapeName);
		if (!strips)
			return;
		skinRef = strips->skinRef;
	}
	if (skinRef == -1)
		return;

	NifBlockBSDismemberment* bsdSkinInst = dynamic_cast<NifBlockBSDismemberment*>(blocks[skinRef]);
	NifBlockNiSkinInstance* niSkinInst = NULL;
	bool skipBSDSkinInst = false;
	if (!bsdSkinInst) {
		skipBSDSkinInst = true;
		niSkinInst = dynamic_cast<NifBlockNiSkinInstance*>(blocks[skinRef]);
		if (!niSkinInst || niSkinInst->skinRef == -1)
			return;
	}
	else if (bsdSkinInst->skinRef == -1)
		return;

	NifBlockNiSkinData* skinData = NULL;
	if (skipBSDSkinInst)
		skinData = (NifBlockNiSkinData*)blocks[niSkinInst->dataRef];
	else
		skinData = (NifBlockNiSkinData*)blocks[bsdSkinInst->dataRef];

	if (boneIndex > skinData->Bones.size())
		return;

	NifBlockNiSkinData::BoneData* bone = &skinData->Bones[boneIndex];
	ushort oldCount = bone->numVerts;
	bone->skin_weights.clear();
	for (auto sw : inWeights)
		if (sw.second >= 0.0001f)
			bone->skin_weights.emplace_back(skin_weight(sw.first, sw.second));

	bone->numVerts = (ushort)bone->skin_weights.size();
	if (skipBSDSkinInst)
		hdr.blockSizes[niSkinInst->dataRef] = skinData->CalcBlockSize();
	else
		hdr.blockSizes[bsdSkinInst->dataRef] = skinData->CalcBlockSize();
}

const vector<vector3>* NifFile::GetRawVertsForShape(const string& shapeName) {
	int bType;
	int dataID = shapeDataIdForName(shapeName, bType);
	if (dataID == -1)
		return NULL;

	if (bType == NIFBLOCK_TRISHAPEDATA) {
		NifBlockTriShapeData* shapeData = (NifBlockTriShapeData*)blocks[dataID];
		return &shapeData->vertices;
	}
	else {
		NifBlockTriStripsData* stripsData = (NifBlockTriStripsData*)blocks[dataID];
		return &stripsData->vertices;
	}
}

const vector<triangle>* NifFile::GetTrisForShape(const string& shapeName) {
	NifBlockTriShape* shape = shapeForName(shapeName);
	if (!shape)
		return NULL;

	int dataBlock = shape->dataRef;
	if (dataBlock == -1 || dataBlock >= hdr.numBlocks)
		return NULL;

	NifBlockTriShapeData* shapeData = (NifBlockTriShapeData*)blocks[dataBlock];
	return &shapeData->tris;
}
	
bool NifFile::GetTrisForTriStripShape(const string& shapeName, vector<triangle>* outTris) {
	NifBlockTriStrips* strips = stripsForName(shapeName);
	if (!strips)
		return false;

	int dataBlock = strips->dataRef;
	if (dataBlock == -1 || dataBlock >= hdr.numBlocks)
		return false;

	NifBlockTriStripsData* stripsData = (NifBlockTriStripsData*)blocks[dataBlock];
	stripsData->StripsToTris(outTris);
	return true;
}

const vector<vector3>* NifFile::GetNormalsForShape(const string& shapeName) {
	int bType;
	int dataID = shapeDataIdForName(shapeName, bType);
	if (dataID == -1)
		return NULL;

	if (bType == NIFBLOCK_TRISHAPEDATA) {
		NifBlockTriShapeData* shapeData = (NifBlockTriShapeData*)blocks[dataID];
		if (shapeData->hasNormals)
			return &shapeData->normals;
	}
	else {
		NifBlockTriStripsData* stripsData = (NifBlockTriStripsData*)blocks[dataID];
		if (stripsData->hasNormals)
			return &stripsData->normals;
	}
	return NULL;
}

const vector<vector2>* NifFile::GetUvsForShape(const string& shapeName) {
	int bType;
	int dataID = shapeDataIdForName(shapeName, bType);
	if (dataID == -1) return NULL;
	if (bType == NIFBLOCK_TRISHAPEDATA) {
		NifBlockTriShapeData* shapeData = (NifBlockTriShapeData*)blocks[dataID];
		if (shapeData->numUVs > 0)
			return &shapeData->uvs;
	}
	else {
		NifBlockTriStripsData* stripsData = (NifBlockTriStripsData*)blocks[dataID];
		if (stripsData->numUVs > 0)
			return &stripsData->uvs;
	}
	return NULL;
}

bool NifFile::GetUvsForShape(const string& shapeName, vector<vector2>& outUvs) {
	int bType;
	int dataID = shapeDataIdForName(shapeName, bType);
	if (dataID == -1)
		return false;

	if (bType == NIFBLOCK_TRISHAPEDATA) {
		NifBlockTriShapeData* shapeData = (NifBlockTriShapeData*)blocks[dataID];
		outUvs.assign(shapeData->uvs.begin(), shapeData->uvs.end());
		return true;
	}
	else {
		NifBlockTriStripsData* stripsData = (NifBlockTriStripsData*)blocks[dataID];
		outUvs.assign(stripsData->uvs.begin(), stripsData->uvs.end());
		return true;
	}
}

const vector<vector<int>>* NifFile::GetSeamVertsForShape(const string& shapeName) {
	int bType;
	int dataID = shapeDataIdForName(shapeName, bType);
	if (dataID == -1)
		return NULL;

	if (bType == NIFBLOCK_TRISHAPEDATA) {
		//NifBlockTriShapeData* shapeData = (NifBlockTriShapeData*)blocks[dataID];
		// dosomthin
	}
	else {
		//NifBlockTriStripsData* stripsData = (NifBlockTriStripsData*)blocks[dataID];
		// dosomthin
	}
	return NULL;
}

bool NifFile::GetVertsForShape(const string& shapeName, vector<vector3>& outVerts) {
	outVerts.clear();

	Mat4 xForm;
	int bType;
	int dataID = shapeDataIdForName(shapeName, bType);
	if (dataID == -1)
		return false;

	if (bType == NIFBLOCK_TRISHAPEDATA) {
		NifBlockTriShapeData* shapeData = (NifBlockTriShapeData*)blocks[dataID];
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
		NifBlockTriStripsData* stripsData = (NifBlockTriStripsData*)blocks[dataID];
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

	if (bType == NIFBLOCK_TRISHAPEDATA) {
		NifBlockTriShapeData* shapeData = dynamic_cast<NifBlockTriShapeData*>(blocks[dataID]);
		if (shapeData)
			return shapeData->numverts;
	}
	else {
		NifBlockTriStripsData* stripsData = dynamic_cast<NifBlockTriStripsData*>(blocks[dataID]);
		if (stripsData)
			return stripsData->numverts;
	}
	return -1;
}

void NifFile::SetVertsForShape(const string& shapeName, const vector<vector3>& verts) {
	Mat4 xForm;
	int bType;
	int dataID = shapeDataIdForName(shapeName, bType);
	if (dataID == -1)
		return;

	if (bType == NIFBLOCK_TRISHAPEDATA) {
		NifBlockTriShapeData* shapeData = (NifBlockTriShapeData*)blocks[dataID];
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
		NifBlockTriStripsData* stripsData = (NifBlockTriStripsData*)blocks[dataID];
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

void NifFile::SetUvsForShape(const string& shapeName, const vector<vector2>& uvs) {
	int bType;
	int dataID = shapeDataIdForName(shapeName, bType);
	if (dataID == -1)
		return;

	if (bType == NIFBLOCK_TRISHAPEDATA) {
		NifBlockTriShapeData* shapeData = (NifBlockTriShapeData*)blocks[dataID];
		if (uvs.size() != shapeData->vertices.size())
			return;

		shapeData->uvs.assign(uvs.begin(), uvs.end());
	}
	else {
		NifBlockTriStripsData* stripsData = (NifBlockTriStripsData*)blocks[dataID];
		if (uvs.size() != stripsData->vertices.size())
			return;

		stripsData->uvs.assign(uvs.begin(), uvs.end());
	}
}

void NifFile::SetNormalsForShape(const string& shapeName, const vector<vector3>& norms) {
	int bType;
	int dataID = shapeDataIdForName(shapeName, bType);
	if (dataID == -1)
		return;

	if (bType == NIFBLOCK_TRISHAPEDATA) {
		NifBlockTriShapeData* shapeData = (NifBlockTriShapeData*)blocks[dataID];
		if (norms.size() != shapeData->normals.size())
			shapeData->normals.resize(norms.size());
		for (int i = 0; i < shapeData->vertices.size(); i++)
			shapeData->normals[i] = norms[i];
		shapeData->hasNormals = true;
		shapeData->numUVs |= 1;
		hdr.blockSizes[dataID] = shapeData->CalcBlockSize();
	}
	else {
		NifBlockTriStripsData* stripsData = (NifBlockTriStripsData*)blocks[dataID];
		if (norms.size() != stripsData->normals.size())
			stripsData->normals.resize(norms.size());
		for (int i = 0; i < stripsData->vertices.size(); i++)
			stripsData->normals[i] = norms[i];
		stripsData->hasNormals = true;
		stripsData->numUVs |= 1;
		hdr.blockSizes[dataID] = stripsData->CalcBlockSize();
	}
}
	
void NifFile::CalcTangentsForShape(const string& shapeName) {
	int bType;
	int dataID = shapeDataIdForName(shapeName, bType);
	if (dataID == -1)
		return;

	if (bType == NIFBLOCK_TRISHAPEDATA) {
		NifBlockTriShapeData* shapeData = (NifBlockTriShapeData*)blocks[dataID];
		shapeData->CalcTangentSpace();
		hdr.blockSizes[dataID] = shapeData->CalcBlockSize();
	}
	else {
		NifBlockTriStripsData* stripsData = (NifBlockTriStripsData*)blocks[dataID];
		stripsData->CalcTangentSpace();
		hdr.blockSizes[dataID] = stripsData->CalcBlockSize();
	}
}

void NifFile::GetRootTranslation(vector3& outVec) {
	NifBlockNiNode* root = dynamic_cast<NifBlockNiNode*>(blocks[0]);
	if (root)
		outVec = root->translation;
	else
		outVec.Zero();
}

void NifFile::SetRootTranslation(vector3& newTrans) {
	NifBlockNiNode* root = dynamic_cast<NifBlockNiNode*>(blocks[0]);
	if (root)
		root->translation = newTrans;
}

void NifFile::GetRootScale(float& outScale) {
	NifBlockNiNode* root = dynamic_cast<NifBlockNiNode*>(blocks[0]);
	if (root)
		outScale = root->scale;
	else
		outScale = 1.0f;
}

void NifFile::SetRootScale(const float& newScale) {
	NifBlockNiNode* root = dynamic_cast<NifBlockNiNode*>(blocks[0]);
	if (root)
		root->scale = newScale;
}

void NifFile::GetShapeTranslation(const string& shapeName, vector3& outVec) {
	NifBlockTriShape* shape = shapeForName(shapeName);
	if (!shape) {
		NifBlockTriStrips* strips = stripsForName(shapeName);
		if (!strips)
			return;
		outVec = strips->translation;
	}
	else
		outVec = shape->translation;

	NifBlockNiNode* root = dynamic_cast<NifBlockNiNode*>(blocks[0]);
	if (root)
		outVec += root->translation;

	//if (outVec.DistanceTo(vec3(0.0f, 0.0f, 0.0f)) < EPSILON)
	//	outVec.Zero();
}

void NifFile::SetShapeTranslation(const string& shapeName, vector3& newTrans) {
	NifBlockTriShape* shape = shapeForName(shapeName);
	if (!shape) {
		NifBlockTriStrips* strips = stripsForName(shapeName);
		if (!strips)
			return;
		strips->translation = newTrans;
	}
	else
		shape->translation = newTrans;
}

void NifFile::GetShapeScale(const string& shapeName, float& outScale) {
	NifBlockTriShape* shape = shapeForName(shapeName);
	if (!shape) {
		NifBlockTriStrips* strips = stripsForName(shapeName);
		if (!strips)
			return;
		outScale = strips->scale;
	}
	else
		outScale = shape->scale;
}

void NifFile::SetShapeScale(const string& shapeName, const float& newScale) {
	NifBlockTriShape* shape = shapeForName(shapeName);
	if (!shape) {
		NifBlockTriStrips* strips = stripsForName(shapeName);
		if (!strips)
			return;

		strips->scale = newScale;
	}
	else
		shape->scale = newScale;
}

void NifFile::ApplyShapeTranslation(const string& shapeName, const vector3& offset) {
	int bType;
	int dataID = shapeDataIdForName(shapeName, bType);
	if (dataID == -1)
		return;

	if (bType == NIFBLOCK_TRISHAPEDATA) {
		NifBlockTriShapeData* shapeData = (NifBlockTriShapeData*)blocks[dataID];
		NifBlockTriShape* shape = shapeForName(shapeName);
		for (int i = 0; i < shapeData->vertices.size(); i++)
			shapeData->vertices[i] += shape->translation + offset;
		shape->translation = vec3(0.0f, 0.0f, 0.0f);
	}
	else {
		NifBlockTriStripsData* stripsData = (NifBlockTriStripsData*)blocks[dataID];
		NifBlockTriStrips* strips = stripsForName(shapeName);
		for (int i = 0; i < stripsData->vertices.size(); i++)
			stripsData->vertices[i] += strips->translation + offset;
		strips->translation = vec3(0.0f, 0.0f, 0.0f);
	}
}
	
/* Sets a phantom offset value in a NiTriShape/StripsData record, which will be added to vertex values when fetched later.
If sum is true, the offset is added to the current offset, allowing a series of independant changes.*/
void NifFile::VirtualOffsetShape(const string& shapeName, const vector3& offset, bool sum) {
	int bType;
	int dataID = shapeDataIdForName(shapeName, bType);
	if (dataID == -1)
		return;

	if (bType == NIFBLOCK_TRISHAPEDATA) {
		NifBlockTriShapeData* shapeData = (NifBlockTriShapeData*)blocks[dataID];
		if (sum)
			shapeData->virtOffset += offset;
		else
			shapeData->virtOffset = offset;
	}
	else {
		NifBlockTriStripsData* stripsData = (NifBlockTriStripsData*)blocks[dataID];
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

	if (bType == NIFBLOCK_TRISHAPEDATA) {
		NifBlockTriShapeData* shapeData = (NifBlockTriShapeData*)blocks[dataID];
		shapeData->scaleFromCenter = fromCenter;
		shapeData->virtScale = scale;
	}
	else {
		NifBlockTriStripsData* stripsData = (NifBlockTriStripsData*)blocks[dataID];
		stripsData->scaleFromCenter = fromCenter;
		stripsData->virtScale = scale;
	}
}

vector3 NifFile::GetShapeVirtualOffset(const string& shapeName) {
	vector3 ret(0.0f, 0.0f, 0.0f);
	int bType;
	int dataID = shapeDataIdForName(shapeName, bType);
	if (dataID == -1)
		return ret;

	if (bType == NIFBLOCK_TRISHAPEDATA) {
		NifBlockTriShapeData* shapeData = (NifBlockTriShapeData*)blocks[dataID];
		ret = shapeData->virtOffset;
	}
	else {
		NifBlockTriStripsData* stripsData = (NifBlockTriStripsData*)blocks[dataID];
		ret = stripsData->virtOffset;
	}
	return ret;
}
		
void NifFile::GetShapeVirtualScale(const string& shapeName, float& scale, bool& fromCenterFlag) {
	int bType;
	int dataID = shapeDataIdForName(shapeName, bType);
	if (dataID == -1)
		return;

	if (bType == NIFBLOCK_TRISHAPEDATA) {
		NifBlockTriShapeData* shapeData = (NifBlockTriShapeData*)blocks[dataID];
		scale = shapeData->virtScale;
		fromCenterFlag = shapeData->scaleFromCenter;
	}
	else {
		NifBlockTriStripsData* stripsData = (NifBlockTriStripsData*)blocks[dataID];
		scale = stripsData->virtScale;
		fromCenterFlag = stripsData->scaleFromCenter;
	}
	return;
}

void NifFile::MoveVertex(const string& shapeName, const vector3& pos, const int& id) {
	int bType;
	int dataID = shapeDataIdForName(shapeName, bType);
	if (dataID == -1)
		return;

	vector<vec3>* verts;
	if (bType == NIFBLOCK_TRISHAPEDATA) {
		NifBlockTriShapeData* shapeData = (NifBlockTriShapeData*)blocks[dataID];
		verts = &shapeData->vertices;
	}
	else {
		NifBlockTriStripsData* stripsData = (NifBlockTriStripsData*)blocks[dataID];
		verts = &stripsData->vertices;
	}
	(*verts)[id] = pos;
}

void NifFile::OffsetShape(const string& shapeName, const vector3& offset, unordered_map<int, float>* mask) {
	int bType;
	int dataID = shapeDataIdForName(shapeName, bType);
	if (dataID == -1)
		return;

	vector<vec3>* verts;
	if (bType == NIFBLOCK_TRISHAPEDATA) {
		NifBlockTriShapeData* shapeData = (NifBlockTriShapeData*)blocks[dataID];
		verts = &shapeData->vertices;
		// dosomthin
	}
	else {
		NifBlockTriStripsData* stripsData = (NifBlockTriStripsData*)blocks[dataID];
		verts = &stripsData->vertices;
		// dosomthin
	}

	float m;
	vec3 o;
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

void NifFile::ScaleShape(const string& shapeName, const float& scale, unordered_map<int, float>* mask) {
	int bType;
	int dataID = shapeDataIdForName(shapeName, bType);
	if (dataID == -1)
		return;

	vector<vec3>* verts;
	if (bType == NIFBLOCK_TRISHAPEDATA) {
		NifBlockTriShapeData* shapeData = (NifBlockTriShapeData*)blocks[dataID];
		verts = &shapeData->vertices;
	}
	else {
		NifBlockTriStripsData* stripsData = (NifBlockTriStripsData*)blocks[dataID];
		verts = &stripsData->vertices;
	}

	vec3 root;
	GetRootTranslation(root);

	unordered_map<int, vec3> diff;
	for (int i = 0; i < verts->size(); i++) {
		vec3 target = (*verts)[i] - root;
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

void NifFile::RotateShape(const string& shapeName, const vec3& angle, unordered_map<int, float>* mask) {
	int bType;
	int dataID = shapeDataIdForName(shapeName, bType);
	if (dataID == -1)
		return;

	vector<vec3>* verts;
	if (bType == NIFBLOCK_TRISHAPEDATA) {
		NifBlockTriShapeData* shapeData = (NifBlockTriShapeData*)blocks[dataID];
		verts = &shapeData->vertices;
	}
	else {
		NifBlockTriStripsData* stripsData = (NifBlockTriStripsData*)blocks[dataID];
		verts = &stripsData->vertices;
	}

	vec3 root;
	GetRootTranslation(root);

	unordered_map<int, vec3> diff;
	for (int i = 0; i < verts->size(); i++) {
		vec3 target = (*verts)[i] - root;
		Mat4 mat;
		mat.Rotate(angle.x * DEG2RAD, vec3(1.0f, 0.0f, 0.0f));
		mat.Rotate(angle.y * DEG2RAD, vec3(0.0f, 1.0f, 0.0f));
		mat.Rotate(angle.z * DEG2RAD, vec3(0.0f, 0.0f, 1.0f));
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

void NifFile::GetAlphaForShape(const string& shapeName, unsigned short& outFlags, BYTE& outThreshold) {
	int alphaRef = -1;
	NifBlockTriShape* shape = shapeForName(shapeName);
	if (!shape) {
		NifBlockTriStrips* strips = stripsForName(shapeName);
		if (!strips)
			return;
		alphaRef = strips->propertiesRef2;
	}
	else
		alphaRef = shape->propertiesRef2;

	if (alphaRef == -1)
		return;

	NifBlockAlphaProperty* alpha = dynamic_cast<NifBlockAlphaProperty*>(GetBlock(alphaRef));
	if (!alpha)
		return;

	outFlags = alpha->flags;
	outThreshold = alpha->threshold;
}

void NifFile::SetAlphaForShape(const string& shapeName, unsigned short flags, unsigned short threshold) {
	int alphaRef = -1;
	NifBlockTriShape* shape = shapeForName(shapeName);
	if (!shape) {
		NifBlockTriStrips* strips = stripsForName(shapeName);
		if (!strips)
			return;
		alphaRef = strips->propertiesRef2;
	}
	else
		alphaRef = shape->propertiesRef2;

	if (alphaRef == -1)
		return;

	NifBlockAlphaProperty* alpha = dynamic_cast<NifBlockAlphaProperty*>(GetBlock(alphaRef));
	if (!alpha)
		return;

	alpha->flags = flags;
	alpha->threshold = (BYTE)threshold;
}

void NifFile::DeleteShape(const string& shapeName) {
	NifBlockTriShape* shape = shapeForName(shapeName);
	if (!shape) {
		NifBlockTriStrips* strips = stripsForName(shapeName);
		if (!strips)
			return;

		DeleteBlock(strips->dataRef);
		if (strips->skinRef != -1) {
			NifBlockBSDismemberment* bsdSkinInst = dynamic_cast<NifBlockBSDismemberment*>(GetBlock(strips->skinRef));
			if (bsdSkinInst) {
				DeleteBlock(bsdSkinInst->dataRef);
				DeleteBlock(bsdSkinInst->skinRef);
				DeleteBlock(strips->skinRef);
			}
			else {
				NifBlockNiSkinInstance* niSkinInst = dynamic_cast<NifBlockNiSkinInstance*>(GetBlock(strips->skinRef));
				if (niSkinInst) {
					DeleteBlock(niSkinInst->dataRef);
					DeleteBlock(niSkinInst->skinRef);
					DeleteBlock(strips->skinRef);
				}
			}
		}
		if (strips->propertiesRef1 != -1) {
			NifBlockBSLightShadeProp* shader = dynamic_cast<NifBlockBSLightShadeProp*>(GetBlock(strips->propertiesRef1));
			if (shader) {
				DeleteBlock(shader->texsetRef);
				DeleteBlock(strips->propertiesRef1);
			}
		}
		if (strips->propertiesRef2 != -1)
			DeleteBlock(strips->propertiesRef2);

		for (int i = 0; i < strips->numExtra; i++)
			DeleteBlock(strips->extradata[i]);

	}
	else {
		DeleteBlock(shape->dataRef);
		if (shape->skinRef != -1) {
			NifBlockBSDismemberment* bsdSkinInst = dynamic_cast<NifBlockBSDismemberment*>(GetBlock(shape->skinRef));
			if (bsdSkinInst) {
				DeleteBlock(bsdSkinInst->dataRef);
				DeleteBlock(bsdSkinInst->skinRef);
				DeleteBlock(shape->skinRef);
			}
			else {
				NifBlockNiSkinInstance* niSkinInst = dynamic_cast<NifBlockNiSkinInstance*>(GetBlock(shape->skinRef));
				if (niSkinInst) {
					DeleteBlock(niSkinInst->dataRef);
					DeleteBlock(niSkinInst->skinRef);
					DeleteBlock(shape->skinRef);
				}
			}
		}
		if (shape->propertiesRef1 != -1) {
			NifBlockBSLightShadeProp* shader = dynamic_cast<NifBlockBSLightShadeProp*>(GetBlock(shape->propertiesRef1));
			if (shader) {
				DeleteBlock(shader->texsetRef);
				DeleteBlock(shape->propertiesRef1);
			}
		}
		if (shape->propertiesRef2 != -1)
			DeleteBlock(shape->propertiesRef2);

		for (int i = 0; i < shape->numExtra; i++)
			DeleteBlock(shape->extradata[i]);
	}
	int shapeID = shapeIdForName(shapeName);

	DeleteBlock(shapeID);
}

void NifFile::DeleteVertsForShape(const string& shapeName, const vector<ushort>& indices) {
	if (indices.size() == 0)
		return;

	NifBlockTriShape* shape = shapeForName(shapeName);
	int dataRef = -1;
	int skinRef = -1;
	if (!shape) {
		NifBlockTriStrips* strips = stripsForName(shapeName);
		if (!strips)
			return;

		dataRef = strips->dataRef;
		skinRef = strips->skinRef;
		NifBlockTriStripsData* shapeData = (NifBlockTriStripsData*)blocks[dataRef];
		shapeData->notifyVerticesDelete(indices);
		if (shapeData->numverts == 0 || shapeData->numTriangles == 0) {
			// Deleted all verts or tris! Remove entire shape and children.
			DeleteShape(shapeName);
			return;
		}
		hdr.blockSizes[dataRef] = shapeData->CalcBlockSize();
	}
	else {
		dataRef = shape->dataRef;
		skinRef = shape->skinRef;
		NifBlockTriShapeData* shapeData = (NifBlockTriShapeData*)blocks[dataRef];
		shapeData->notifyVerticesDelete(indices);
		if (shapeData->numverts == 0 || shapeData->numTriangles == 0) {
			// Deleted all verts or tris! Remove entire shape and children.
			DeleteShape(shapeName);
			return;
		}
		hdr.blockSizes[dataRef] = shapeData->CalcBlockSize();
	}

	if (skinRef == -1)
		return;

	NifBlockBSDismemberment* bsdSkinInst = dynamic_cast<NifBlockBSDismemberment*>(blocks[skinRef]);
	if (bsdSkinInst) {
		NifBlockNiSkinData* skinData = (NifBlockNiSkinData*)blocks[bsdSkinInst->dataRef];
		skinData->notifyVerticesDelete(indices);
		hdr.blockSizes[bsdSkinInst->dataRef] = skinData->CalcBlockSize();

		NifBlockNiSkinPartition* skinPartition = (NifBlockNiSkinPartition*)blocks[bsdSkinInst->skinRef];
		skinPartition->notifyVerticesDelete(indices);
		vector<int> empty_indices;
		if (skinPartition->RemoveEmptyPartitions(empty_indices)) {
			bsdSkinInst->numPartitions -= empty_indices.size();
			for (int i = empty_indices.size() - 1; i >= 0; i--)
				bsdSkinInst->partitions.erase(bsdSkinInst->partitions.begin() + i);
		}
		hdr.blockSizes[bsdSkinInst->skinRef] = skinPartition->CalcBlockSize();
		hdr.blockSizes[skinRef] = bsdSkinInst->CalcBlockSize();
	}
	else {
		NifBlockNiSkinInstance* niSkinInst = dynamic_cast<NifBlockNiSkinInstance*>(blocks[skinRef]);
		if (niSkinInst) {
			NifBlockNiSkinData* skinData = (NifBlockNiSkinData*)blocks[niSkinInst->dataRef];
			skinData->notifyVerticesDelete(indices);
			hdr.blockSizes[niSkinInst->dataRef] = skinData->CalcBlockSize();

			NifBlockNiSkinPartition* skinPartition = (NifBlockNiSkinPartition*)blocks[niSkinInst->skinRef];
			skinPartition->notifyVerticesDelete(indices);
			vector<int> empty_indices;
			skinPartition->RemoveEmptyPartitions(empty_indices);
			hdr.blockSizes[niSkinInst->skinRef] = skinPartition->CalcBlockSize();
			hdr.blockSizes[skinRef] = niSkinInst->CalcBlockSize();
		}
	}
}

int NifFile::CalcShapeDiff(const string& shapeName, const vector<vector3>* targetData, unordered_map<int, vector3>& outDiffData, float scale) {
	vector3 v;
	outDiffData.clear();
	const vector<vector3>* myData = GetRawVertsForShape(shapeName);
	if (!myData) return 1;
	if (!targetData) return 2;
	if (myData->size() != targetData->size()) return 3;
	for (int i = 0; i < myData->size(); i++) {
		v.x = (targetData->at(i).x * scale) - myData->at(i).x;
		v.y = (targetData->at(i).y * scale) - myData->at(i).y;
		v.z = (targetData->at(i).z * scale) - myData->at(i).z;

		//if (fabs(v.x) > EPSILON || fabs(v.y) > EPSILON || fabs(v.z) > EPSILON)
		outDiffData[i] = v;
	}
	return 0;
}

int NifFile::ShapeDiff(const string& baseShapeName, const string& targetShape, unordered_map<int, vector3>& outDiffData) {
	const vector<vector3>* targetData = GetRawVertsForShape(targetShape);
	if(!targetData) return 2;
	return CalcShapeDiff(baseShapeName,targetData,outDiffData);
}

void NifFile::UpdateSkinPartitions(const string& shapeName) {
	int bType;
	int shapeDataID = shapeDataIdForName(shapeName, bType);
	if (shapeDataID == -1)
		return;

	int skinRef;
	NifBlockTriShapeData* shapeData = NULL;
	NifBlockTriStripsData* stripsData = NULL;
	if (bType == NIFBLOCK_TRISHAPEDATA) {
		NifBlockTriShape* shape = shapeForName(shapeName);
		shapeData = (NifBlockTriShapeData*)blocks[shape->dataRef];
		skinRef = shape->skinRef;
	}
	else {
		NifBlockTriStrips* shape = stripsForName(shapeName);
		stripsData = (NifBlockTriStripsData*)blocks[shape->dataRef];
		skinRef = shape->skinRef;
	}
	if (skinRef == -1)
		return;

	bool skipBSDSkinInst = false;
	NifBlockBSDismemberment* bsdSkinInst = dynamic_cast<NifBlockBSDismemberment*>(blocks[skinRef]);
	NifBlockNiSkinInstance* niSkinInst = NULL;
	NifBlockNiSkinData* skinData = NULL;
	NifBlockNiSkinPartition* skinPart = NULL;
	if (!bsdSkinInst) {
		skipBSDSkinInst = true;
		niSkinInst = dynamic_cast<NifBlockNiSkinInstance*>(blocks[skinRef]);
		if (!niSkinInst)
			return;

		skinData = (NifBlockNiSkinData*)blocks[niSkinInst->dataRef];
		skinPart = (NifBlockNiSkinPartition*)blocks[niSkinInst->skinRef];
	}
	else {
		skinData = (NifBlockNiSkinData*)blocks[bsdSkinInst->dataRef];
		skinPart = (NifBlockNiSkinPartition*)blocks[bsdSkinInst->skinRef];
	}

	if (skinPart->needsBuild) {
		BuildSkinPartitions(shapeName);
		return;
	}

	map<int, vector<int>> vertBones;
	map<int, vector<float>> vertWeights;
	int b = 0;
	for (auto bone : skinData->Bones) {
		for (auto bw : bone.skin_weights) {
			int i = 0;
			for (auto w : vertWeights[bw.index]) {
				if (bw.weight > w)
					break;
				i++;
			}
			vertBones[bw.index].insert(vertBones[bw.index].begin() + i, b);
			vertWeights[bw.index].insert(vertWeights[bw.index].begin() + i, bw.weight);
		}
		b++;
	}

	for (auto& part : skinPart->partitionBlocks) {
		set<int> bones;
		set<unsigned short> verts(part.vertMap.begin(), part.vertMap.end());
		for (auto v : part.vertMap) {
			for (int bi = 0; bi < vertBones[v].size(); bi++) {
				if (bi == 4) {
					break;
				}
				int bid = vertBones[v][bi];
				bones.insert(bid);
			}
		}

		unordered_map<int, int> bonelookup;
		part.bones.clear();
		part.boneindex.clear();
		part.vertWeights.clear();
		for (auto b : bones) {
			part.bones.push_back(b);
			bonelookup[b] = part.bones.size() - 1;
		}

		for (auto v : part.vertMap) {
			boneIndices b;
			vertexWeight vw;
			b.i1 = b.i2 = b.i3 = b.i4 = 0;
			vw.w1 = vw.w2 = vw.w3 = vw.w4 = 0;
			BYTE* pb = (BYTE*)&b.i1;
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
			for (int bi = 0; bi < 4; bi++)
				pw[bi] /= tot;

			part.boneindex.push_back(b);
			part.vertWeights.push_back(vw);
		}
		part.numBones = part.bones.size();
	}

	if (!skipBSDSkinInst) {
		unordered_map<int, int> partIDCount;
		for (int i = 0; i < bsdSkinInst->numPartitions; i++) {
			auto pidc = partIDCount.find(bsdSkinInst->partitions[i].partID);
			if (pidc == partIDCount.end()) {
				bsdSkinInst->partitions[i].flags = 257;
				partIDCount[bsdSkinInst->partitions[i].partID] = 1;
			}
			else {
				bsdSkinInst->partitions[i].flags = 257;
				partIDCount[bsdSkinInst->partitions[i].partID]++;
			}
		}
		hdr.blockSizes[bsdSkinInst->skinRef] = skinPart->CalcBlockSize();
	}
	else
		hdr.blockSizes[niSkinInst->skinRef] = skinPart->CalcBlockSize();
}

void NifFile::BuildSkinPartitions(const string& shapeName, int maxBonesPerPartition) {
	int bType;
	int shapeDataID = shapeDataIdForName(shapeName, bType);
	if (shapeDataID == -1)
		return;

	int skinRef;
	NifBlockTriShapeData* shapeData = NULL;
	NifBlockTriStripsData* stripsData = NULL;
	if (bType == NIFBLOCK_TRISHAPEDATA) {
		NifBlockTriShape* shape = shapeForName(shapeName);
		if (!shape)
			return;

		skinRef = shape->skinRef;
		shapeData = (NifBlockTriShapeData*)blocks[shape->dataRef];
		if (skinRef == -1) {
			NifBlockNiSkinData* nifSkinData = new NifBlockNiSkinData();
			int skinDataID = AddBlock((NifBlock*)nifSkinData, "NiSkinData");

			NifBlockNiSkinPartition* nifSkinPartition = new NifBlockNiSkinPartition();
			int partID = AddBlock((NifBlock*)nifSkinPartition, "NiSkinPartition");

			NifBlockBSDismemberment* nifDismemberInst = new NifBlockBSDismemberment();
			int dismemberID = AddBlock((NifBlock*)nifDismemberInst, "BSDismemberSkinInstance");

			nifDismemberInst->dataRef = skinDataID;
			nifDismemberInst->skinRef = partID;
			nifDismemberInst->skeletonRoot = 0;
			shape->skinRef = dismemberID;
			skinRef = dismemberID;
		}
	}
	else {
		NifBlockTriStrips* strips = stripsForName(shapeName);
		if (!strips)
			return;

		// TO-DO
		return;

		skinRef = strips->skinRef;
		stripsData = (NifBlockTriStripsData*)blocks[strips->dataRef];
		if (skinRef == -1) {
			NifBlockNiSkinData* nifSkinData = new NifBlockNiSkinData();
			int skinDataID = AddBlock((NifBlock*)nifSkinData, "NiSkinData");

			NifBlockNiSkinPartition* nifSkinPartition = new NifBlockNiSkinPartition();
			int partID = AddBlock((NifBlock*)nifSkinPartition, "NiSkinPartition");

			NifBlockBSDismemberment* nifDismemberInst = new NifBlockBSDismemberment();
			int skinID = AddBlock((NifBlock*)nifDismemberInst, "BSDismemberSkinInstance");

			nifDismemberInst->dataRef = skinDataID;
			nifDismemberInst->skinRef = partID;
			nifDismemberInst->skeletonRoot = 0;
			strips->skinRef = skinID;
			skinRef = skinID;
		}
	}

	bool skipBSDSkinInst = false;
	NifBlockBSDismemberment* bsdSkinInst = dynamic_cast<NifBlockBSDismemberment*>(blocks[skinRef]);
	NifBlockNiSkinInstance* niSkinInst = NULL;
	NifBlockNiSkinData* skinData = NULL;
	NifBlockNiSkinPartition* skinPart = NULL;

	if (!bsdSkinInst) {
		skipBSDSkinInst = true;
		niSkinInst = dynamic_cast<NifBlockNiSkinInstance*>(blocks[skinRef]);
		if (!niSkinInst)
			return;

		skinData = (NifBlockNiSkinData*)blocks[niSkinInst->dataRef];
		skinPart = (NifBlockNiSkinPartition*)blocks[niSkinInst->skinRef];
	}
	else {
		skinData = (NifBlockNiSkinData*)blocks[bsdSkinInst->dataRef];
		skinPart = (NifBlockNiSkinPartition*)blocks[bsdSkinInst->skinRef];
	}

	vector<triangle> tris;
	ushort numTriangles;
	ushort numVerts;
	if (bType == NIFBLOCK_TRISHAPEDATA) {
		tris = shapeData->tris;
		numTriangles = shapeData->numTriangles;
		numVerts = shapeData->numverts;
	}
	else {
		stripsData->StripsToTris(&tris);
		numTriangles = stripsData->numTriangles;
		numVerts = stripsData->numverts;
	}

	int maxBonesPerVertex = 4;

	unordered_map<int, vector<int>> vertTris;
	for (int t = 0; t < tris.size(); t++) {
		vertTris[tris[t].p1].push_back(t);
		vertTris[tris[t].p2].push_back(t);
		vertTris[tris[t].p3].push_back(t);
	}

	unordered_map<int, vector<int>> vertBones;
	unordered_map<int, vector<float>> vertWeights;
	int b = 0;
	for (auto bone : skinData->Bones) {
		for (auto bw : bone.skin_weights) {
			int i = 0;
			for (auto w : vertWeights[bw.index]) {
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
	for (auto vb : vertBones) {
		if (vb.second.size() > maxBonesPerVertex) {
			vb.second.erase(vb.second.begin() + maxBonesPerVertex);
		}
	}
	for (auto vw : vertWeights) {
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
		for (auto ip : p) {
			if (ip == 516)
				int a = 4;
			for (auto tb : vertBones[ip])
				testBones.insert(tb);
		}
	};

	int partID = 0;
	int partCount = 0;
	//unordered_map<int, vector<int>> partTris;
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

		// Get associated bones for the current triangle.
		fTriBones(it);

		if (testBones.size() > maxBonesPerPartition) {
			// TODO: get rid of some bone influences on this tri before trying to put it anywhere.  
		}

		// How many new bones are in the tri's bonelist?
		int newBoneCount = 0;
		for (auto tb : testBones) {
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

		// Select the triangle's un-visited verts for adjancency examination.
		fSelectVerts(it);

		while (!vertQueue.empty()) {
			int adjVert = vertQueue.front();
			vertQueue.pop();

			for (auto adjTri : vertTris[adjVert]) {
				if (triAssign[adjTri])			// skip triangles we've already assigned
					continue;

				// Get associated bones for the current triangle.
				fTriBones(adjTri);

				if (testBones.size() > maxBonesPerPartition) {
					// TODO: get rid of some bone influences on this tri before trying to put it anywhere.  
				}

				// How many new bones are in the tri's bonelist?
				int newBoneCount = 0;
				for (auto tb : testBones) {
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
		// next outer triangle
		it++;
	}

	skinPart->partitionBlocks.clear();
	skinPart->numPartitions = partID + 1;

	for (int pID = 0; pID <= partID; pID++) {
		NifBlockNiSkinPartition::PartitionBlock pblock;
		pblock.hasBoneIndices = true;
		pblock.hasFaces = true;
		pblock.hasVertMap = true;
		pblock.hasVertWeights = true;
		pblock.numStrips = 0;
		pblock.numWeightsPerVert = maxBonesPerVertex;
		pblock.numVertices = 0;
		pblock.unknown = 0;

		//pblock.numTriangles = partTris[pID].size();
		unordered_map<int, int> vertMap;

		//for(auto triID : partTris[pID]) {
		for (int triID = 0; triID < tris.size(); triID++) {
			if (triParts[triID] != pID)
				continue;
			tri t = tris[triID];
			t.rot();
			if (vertMap.find(t.p1) == vertMap.end()) {
				vertMap[t.p1] = pblock.numVertices;
				pblock.vertMap.push_back(t.p1);
				t.p1 = pblock.numVertices++;
			}
			else {
				t.p1 = vertMap[t.p1];
			}
			if (vertMap.find(t.p2) == vertMap.end()) {
				vertMap[t.p2] = pblock.numVertices;
				pblock.vertMap.push_back(t.p2);
				t.p2 = pblock.numVertices++;
			}
			else {
				t.p2 = vertMap[t.p2];
			}
			if (vertMap.find(t.p3) == vertMap.end()) {
				vertMap[t.p3] = pblock.numVertices;
				pblock.vertMap.push_back(t.p3);
				t.p3 = pblock.numVertices++;
			}
			else {
				t.p3 = vertMap[t.p3];
			}
			pblock.tris.push_back(t);
		}
		pblock.numTriangles = pblock.tris.size();
		pblock.numBones = partBones[pID].size();
		unordered_map<int, int> bonelookup;
		for (auto b : partBones[pID]) {
			pblock.bones.push_back(b);
			bonelookup[b] = pblock.bones.size() - 1;
		}
		for (auto v : pblock.vertMap) {
			boneIndices b;
			vertexWeight vw;
			b.i1 = b.i2 = b.i3 = b.i4 = 0;
			vw.w1 = vw.w2 = vw.w3 = vw.w4 = 0;
			BYTE* pb = (BYTE*)&b.i1;
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
			pblock.boneindex.push_back(b);
			pblock.vertWeights.push_back(vw);
		}

		skinPart->partitionBlocks.push_back(move(pblock));

	}
	if (!skipBSDSkinInst) {
		bsdSkinInst->numPartitions = skinPart->numPartitions;
		bsdSkinInst->partitions.clear();
		for (int i = 0; i < bsdSkinInst->numPartitions; i++) {
			NifBlockBSDismemberment::Partition p;
			p.partID = 32;
			if (i == 0)
				p.flags = 257;
			else
				p.flags = 257;
			bsdSkinInst->partitions.push_back(p);
		}
		hdr.blockSizes[bsdSkinInst->skinRef] = skinPart->CalcBlockSize();
		hdr.blockSizes[skinRef] = bsdSkinInst->CalcBlockSize();
	}
	else {
		hdr.blockSizes[niSkinInst->skinRef] = skinPart->CalcBlockSize();
		hdr.blockSizes[skinRef] = niSkinInst->CalcBlockSize();
	}
}
