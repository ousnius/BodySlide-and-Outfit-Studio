/*
BodySlide and Outfit Studio
Copyright (C) 2016  Caliente & ousnius
See the included LICENSE file
*/

#include "NifFile.h"

#include <set>
#include <queue>
#include <regex>


NifFile::NifFile() {
	isValid = false;
	hasUnknown = false;
}

NifFile::NifFile(NifFile& other) {
	CopyFrom(other);
}

NifFile::~NifFile() {
	Clear();
}

NiShape* NifFile::FindShapeByName(const string& name, int dupIndex) {
	int numFound = 0;
	for (auto& block : blocks) {
		BlockType type = block->blockType;
		if (type == NITRISHAPE || type == NITRISTRIPS || type == BSSUBINDEXTRISHAPE || type == BSTRISHAPE || type == BSMESHLODTRISHAPE) {
			auto geom = dynamic_cast<NiShape*>(block);
			if (geom && !name.compare(geom->GetName())) {
				if (numFound >= dupIndex)
					return geom;

				numFound++;
			}
		}
	}
	return nullptr;
}

NiAVObject* NifFile::FindAVObjectByName(const string& name, int dupIndex) {
	int numFound = 0;
	for (auto& block : blocks) {
		BlockType type = block->blockType;
		if (type == NITRISHAPE || type == NITRISTRIPS || type == BSSUBINDEXTRISHAPE || type == BSTRISHAPE || type == BSMESHLODTRISHAPE) {
			auto avo = dynamic_cast<NiAVObject*>(block);
			if (avo && !name.compare(avo->GetName())) {
				if (numFound >= dupIndex)
					return avo;

				numFound++;
			}
		}
	}
	return nullptr;
}

NiNode* NifFile::FindNodeByName(const string& name) {
	for (auto& block : blocks) {
		if (block->blockType == NINODE) {
			auto node = dynamic_cast<NiNode*>(block);
			if (node && !name.compare(node->GetName()))
				return node;
		}
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

	hdr.SetBlockReference(&blocks);
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
	Clear();

	fstream file(filename.c_str(), ios_base::in | ios_base::binary);
	if (file.is_open()) {
		if (filename.rfind("\\") != string::npos)
			fileName = filename.substr(filename.rfind("\\"));
		else
			fileName = filename;

		hdr.Get(file);
		if (!hdr.IsValid()) {
			Clear();
			return 1;
		}
		
		if (!(hdr.VerCheck(20, 2, 0, 7) && (hdr.GetUserVersion() == 11 || hdr.GetUserVersion() == 12))) {
			Clear();
			return 2;
		}

		hdr.SetBlockReference(&blocks);

		string blockTypeStr;
		for (int i = 0; i < hdr.GetNumBlocks(); i++) {
			NiObject* block = nullptr;
			blockTypeStr = hdr.GetBlockTypeStringById(i);
			if (!blockTypeStr.compare("NiTriShapeData"))
				block = (NiObject*) new NiTriShapeData(file, hdr);
			else if (!blockTypeStr.compare("NiTriShape"))
				block = (NiObject*) new NiTriShape(file, hdr);
			else if (!blockTypeStr.compare("NiTriStrips"))
				block = (NiObject*) new NiTriStrips(file, hdr);
			else if (!blockTypeStr.compare("NiTriStripsData"))
				block = (NiObject*) new NiTriStripsData(file, hdr);
			else if (!blockTypeStr.compare("BSDismemberSkinInstance"))
				block = (NiObject*) new BSDismemberSkinInstance(file, hdr);
			else if (!blockTypeStr.compare("NiNode"))
				block = (NiObject*) new NiNode(file, hdr);
			else if (!blockTypeStr.compare("NiSkinData"))
				block = (NiObject*) new NiSkinData(file, hdr);
			else if (!blockTypeStr.compare("NiSkinPartition"))
				block = (NiObject*) new NiSkinPartition(file, hdr);
			else if (!blockTypeStr.compare("NiSkinInstance"))
				block = (NiObject*) new NiSkinInstance(file, hdr);
			else if (!blockTypeStr.compare("BSLightingShaderProperty"))
				block = (NiObject*) new BSLightingShaderProperty(file, hdr);
			else if (!blockTypeStr.compare("BSShaderTextureSet"))
				block = (NiObject*) new BSShaderTextureSet(file, hdr);
			else if (!blockTypeStr.compare("NiAlphaProperty"))
				block = (NiObject*) new NiAlphaProperty(file, hdr);
			else if (!blockTypeStr.compare("NiStringExtraData"))
				block = (NiObject*) new NiStringExtraData(file, hdr);
			else if (!blockTypeStr.compare("BSShaderPPLightingProperty"))
				block = (NiObject*) new BSShaderPPLightingProperty(file, hdr);
			else if (!blockTypeStr.compare("NiMaterialProperty"))
				block = (NiObject*) new NiMaterialProperty(file, hdr);
			else if (!blockTypeStr.compare("NiStencilProperty"))
				block = (NiObject*) new NiStencilProperty(file, hdr);
			else if (!blockTypeStr.compare("BSEffectShaderProperty"))
				block = (NiObject*) new BSEffectShaderProperty(file, hdr);
			else if (!blockTypeStr.compare("NiFloatInterpolator"))
				block = (NiObject*) new NiFloatInterpolator(file, hdr);
			else if (!blockTypeStr.compare("NiTransformInterpolator"))
				block = (NiObject*) new NiTransformInterpolator(file, hdr);
			else if (!blockTypeStr.compare("NiPoint3Interpolator"))
				block = (NiObject*) new NiPoint3Interpolator(file, hdr);
			else if (!blockTypeStr.compare("BSLightingShaderPropertyColorController"))
				block = (NiObject*) new BSLightingShaderPropertyColorController(file, hdr);
			else if (!blockTypeStr.compare("BSLightingShaderPropertyFloatController"))
				block = (NiObject*) new BSLightingShaderPropertyFloatController(file, hdr);
			else if (!blockTypeStr.compare("BSEffectShaderPropertyColorController"))
				block = (NiObject*) new BSEffectShaderPropertyColorController(file, hdr);
			else if (!blockTypeStr.compare("BSEffectShaderPropertyFloatController"))
				block = (NiObject*) new BSEffectShaderPropertyFloatController(file, hdr);
			else if (!blockTypeStr.compare("BSSubIndexTriShape"))
				block = (NiObject*) new BSSubIndexTriShape(file, hdr);
			else if (!blockTypeStr.compare("BSMeshLODTriShape"))
				block = (NiObject*) new BSMeshLODTriShape(file, hdr);
			else if (!blockTypeStr.compare("BSTriShape"))
				block = (NiObject*) new BSTriShape(file, hdr);
			else if (!blockTypeStr.compare("BSSkin::Instance"))
				block = (NiObject*) new BSSkinInstance(file, hdr);
			else if (!blockTypeStr.compare("BSSkin::BoneData"))
				block = (NiObject*) new BSSkinBoneData(file, hdr);
			else if (!blockTypeStr.compare("BSClothExtraData"))
				block = (NiObject*) new BSClothExtraData(file, hdr);
			else if (!blockTypeStr.compare("NiIntegerExtraData"))
				block = (NiObject*) new NiIntegerExtraData(file, hdr);
			else if (!blockTypeStr.compare("BSXFlags"))
				block = (NiObject*) new BSXFlags(file, hdr);
			else if (!blockTypeStr.compare("BSConnectPointParents"))
				block = (NiObject*) new BSConnectPointParents(file, hdr);
			else if (!blockTypeStr.compare("BSConnectPointChildren"))
				block = (NiObject*) new BSConnectPointChildren(file, hdr);
			else if (!blockTypeStr.compare("NiCollisionObject"))
				block = (NiObject*) new NiCollisionObject(file, hdr);
			else if (!blockTypeStr.compare("bhkNiCollisionObject"))
				block = (NiObject*) new bhkNiCollisionObject(file, hdr);
			else if (!blockTypeStr.compare("bhkCollisionObject"))
				block = (NiObject*) new bhkCollisionObject(file, hdr);
			else if (!blockTypeStr.compare("bhkNPCollisionObject"))
				block = (NiObject*) new bhkNPCollisionObject(file, hdr);
			else if (!blockTypeStr.compare("bhkPhysicsSystem"))
				block = (NiObject*) new bhkPhysicsSystem(file, hdr);
			else {
				hasUnknown = true;
				block = (NiObject*) new NiUnknown(file, hdr.GetBlockSize(i));
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

void NifFile::SetShapeOrder(const vector<string>& order) {
	if (hasUnknown)
		return;

	vector<int> delta;
	bool hadoffset = false;

	// Have to do this in multiple passes
	do {
		vector<string> oldOrder;
		GetShapeList(oldOrder);

		vector<int> oldOrderIds;
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
				hdr.SwapBlocks(oldOrderIds[p], oldOrderIds[p - 1]);
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

	vector<int> oldChildren(root->ChildrenBegin(), root->ChildrenEnd());
	root->ClearChildren();

	for (int i = 0; i < hdr.GetNumBlocks(); i++)
		if (find(oldChildren.begin(), oldChildren.end(), i) != oldChildren.end())
			root->AddChildRef(i);

	auto bookmark = root->ChildrenBegin();
	auto peek = root->ChildrenBegin();

	for (int i = 0; peek < root->ChildrenEnd(); i++) {
		auto block = hdr.GetBlock<NiObject>(root->GetChildRef(i));
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

void NifFile::RemoveUnusedStrings() {
	if (hasUnknown)
		return;

	hdr.RemoveUnusedStrings();
}

int NifFile::AddNode(const string& nodeName, vector<Vector3>& rot, Vector3& trans, float scale) {
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

	int newNodeId = hdr.AddBlock(newNode, "NiNode");
	if (newNodeId != 0xFFFFFFFF)
		root->AddChildRef(newNodeId);

	return newNodeId;
}

void NifFile::DeleteNode(const string& nodeName) {
	hdr.DeleteBlock(GetBlockID(FindNodeByName(nodeName)));
}

string NifFile::GetNodeName(int blockID) {
	string name;

	auto n = hdr.GetBlock<NiNode>(blockID);
	if (n) {
		name = n->GetName();
		if (name.empty())
			name = "_unnamed_";
	}

	return name;
}

void NifFile::SetNodeName(int blockID, const string& newName) {
	auto node = hdr.GetBlock<NiNode>(blockID);
	if (!node)
		return;

	node->SetName(newName, true);
}

int NifFile::AssignExtraData(const string& blockName, const int& extraDataId, bool isNode) {
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

int NifFile::AddStringExtraData(const string& blockName, const string& name, const string& stringData, bool isNode) {
	auto strExtraData = new NiStringExtraData(hdr);
	strExtraData->SetName(name);
	strExtraData->SetStringData(stringData);

	return AssignExtraData(blockName, hdr.AddBlock(strExtraData, "NiStringExtraData"), isNode);
}

int NifFile::AddIntegerExtraData(const string& blockName, const string& name, const int& integerData, bool isNode) {
	auto intExtraData = new NiIntegerExtraData(hdr);
	intExtraData->SetName(name);
	intExtraData->SetIntegerData(integerData);

	return AssignExtraData(blockName, hdr.AddBlock(intExtraData, "NiIntegerExtraData"), isNode);
}

NiShader* NifFile::GetShader(const string& shapeName) {
	int prop1 = 0xFFFFFFFF;

	NiShape* shape = FindShapeByName(shapeName);
	if (shape)
		prop1 = shape->GetShaderPropertyRef();

	if (prop1 != 0xFFFFFFFF) {
		auto shader = hdr.GetBlock<NiShader>(prop1);
		if (shader) {
			ushort type = shader->blockType;
			if (type == BSLIGHTINGSHADERPROPERTY ||
				type == BSEFFECTSHADERPROPERTY ||
				type == BSSHADERPPLIGHTINGPROPERTY)
				return shader;
		}
	}
	else {
		vector<int> props = shape->propertiesRef;
		if (props.empty())
			return nullptr;

		for (int i = 0; i < props.size(); i++) {
			auto shader = hdr.GetBlock<NiShader>(props[i]);
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
	NiShape* shape = FindShapeByName(shapeName);
	if (!shape)
		return nullptr;

	for (int i = 0; i < shape->numProperties; i++) {
		auto material = hdr.GetBlock<NiMaterialProperty>(shape->propertiesRef[i]);
		if (material)
			return material;
	}

	return nullptr;
}

int NifFile::GetTextureForShape(const string& shapeName, string& outTexFile, int texIndex) {
	int textureSetRef = 0xFFFFFFFF;
	outTexFile.clear();

	NiShader* shader = GetShader(shapeName);
	if (shader)
		textureSetRef = shader->GetTextureSetRef();
	else
		return 0;

	if (textureSetRef == 0xFFFFFFFF) {
		if (shader->blockType == BSEFFECTSHADERPROPERTY) {
			auto effectShader = static_cast<BSEffectShaderProperty*>(shader);
			if (texIndex == 0)
				outTexFile = effectShader->sourceTexture.GetString();
			else if (texIndex == 1)
				outTexFile = effectShader->greyscaleTexture.GetString();

			return BSEFFECTSHADERPROPERTY;
		}
		else
			return 0;
	}

	auto textureSet = hdr.GetBlock<BSShaderTextureSet>(textureSetRef);
	if (!textureSet || texIndex + 1 > textureSet->numTextures)
		return 0;

	outTexFile = textureSet->textures[texIndex].GetString();
	return 1;
}

void NifFile::SetTextureForShape(const string& shapeName, string& outTexFile, int texIndex) {
	int textureSetRef = 0xFFFFFFFF;

	NiShader* shader = GetShader(shapeName);
	if (shader)
		textureSetRef = shader->GetTextureSetRef();
	else
		return;
	
	if (textureSetRef == 0xFFFFFFFF) {
		if (shader->blockType == BSEFFECTSHADERPROPERTY) {
			auto effectShader = static_cast<BSEffectShaderProperty*>(shader);
			if (texIndex == 0)
				effectShader->sourceTexture.SetString(outTexFile);
			else if (texIndex == 1)
				effectShader->greyscaleTexture.SetString(outTexFile);
			return;
		}
		else
			return;
	}

	auto textureSet = hdr.GetBlock<BSShaderTextureSet>(textureSetRef);
	if (!textureSet || texIndex + 1 > textureSet->numTextures)
		return;

	textureSet->textures[texIndex].SetString(outTexFile);
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
	NiShape* shape = FindShapeByName(shapeDest);
	if (!shape)
		return;

	int srcShaderRef = 0xFFFFFFFF;
	int propRef1 = 0xFFFFFFFF;
	int propRef2 = 0xFFFFFFFF;

	auto srcShader = srcNif.hdr.GetBlock<NiShader>(shape->GetShaderPropertyRef());
	if (!srcShader) {
		// No shader found, look in other properties
		for (int i = 0; i < shape->numProperties; i++) {
			srcShader = srcNif.hdr.GetBlock<NiShader>(shape->propertiesRef[i]);
			if (srcShader) {
				srcShaderRef = shape->propertiesRef[i];
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

	auto srcAlphaProp = srcNif.hdr.GetBlock<NiAlphaProperty>(shape->GetAlphaPropertyRef());
	if (!srcAlphaProp) {
		// No alpha found, look in other properties
		for (int i = 0; i < shape->numProperties; i++) {
			srcAlphaProp = srcNif.hdr.GetBlock<NiAlphaProperty>(shape->propertiesRef[i]);
			if (srcAlphaProp) {
				propRef2 = i;
				break;
			}
		}
	}

	// Create destination shader and copy
	NiShader* destShader = nullptr;
	if (srcShader->blockType == BSLIGHTINGSHADERPROPERTY) {
		auto shader = static_cast<BSLightingShaderProperty*>(srcShader);
		BSLightingShaderProperty* copyShader = new BSLightingShaderProperty(*shader);
		destShader = static_cast<NiShader*>(copyShader);
	}
	else if (srcShader->blockType == BSSHADERPPLIGHTINGPROPERTY) {
		auto shader = static_cast<BSShaderPPLightingProperty*>(srcShader);
		BSShaderPPLightingProperty* copyShader = new BSShaderPPLightingProperty(*shader);
		destShader = static_cast<NiShader*>(copyShader);
	}
	else if (srcShader->blockType == BSEFFECTSHADERPROPERTY) {
		auto shader = static_cast<BSEffectShaderProperty*>(srcShader);
		BSEffectShaderProperty* copyShader = new BSEffectShaderProperty(*shader);
		destShader = static_cast<NiShader*>(copyShader);
	}

	if (!destShader)
		return;

	destShader->header = &hdr;
	if (hdr.GetUserVersion() == 12 && hdr.GetUserVersion2() >= 120)
		destShader->SetName(srcShader->GetName());
	else
		destShader->ClearName();

	// Add shader block to nif
	int shaderId = hdr.AddBlock(destShader, srcNif.hdr.GetBlockTypeStringById(srcShaderRef));

	auto srcTexSet = srcNif.hdr.GetBlock<BSShaderTextureSet>(srcShader->GetTextureSetRef());
	if (srcTexSet) {
		// Create texture set block and copy
		auto destTexSet = new BSShaderTextureSet(*srcTexSet);
		destTexSet->header = &hdr;

		// Add texture block to nif
		int texSetId = hdr.AddBlock(destTexSet, "BSShaderTextureSet");

		// Assign texture set block id to shader
		destShader->SetTextureSetRef(texSetId);
	}

	// Wet Material
	string srcWetMaterialName = srcShader->GetWetMaterialName();
	if (!srcWetMaterialName.empty())
		destShader->SetWetMaterialName(srcWetMaterialName);

	// Controller
	auto srcController = srcNif.hdr.GetBlock<NiTimeController>(srcShader->GetControllerRef());
	if (srcController) {
		NiTimeController* destController = nullptr;
		if (srcController->blockType == BSLIGHTINGSHADERPROPERTYCOLORCONTROLLER) {
			auto controller = static_cast<BSLightingShaderPropertyColorController*>(srcController);
			BSLightingShaderPropertyColorController* controllerCopy = new BSLightingShaderPropertyColorController(*controller);
			destController = static_cast<NiTimeController*>(controllerCopy);
		}
		else if (srcController->blockType == BSLIGHTINGSHADERPROPERTYFLOATCONTROLLER) {
			auto controller = static_cast<BSLightingShaderPropertyFloatController*>(srcController);
			BSLightingShaderPropertyFloatController* controllerCopy = new BSLightingShaderPropertyFloatController(*controller);
			destController = static_cast<NiTimeController*>(controllerCopy);
		}
		else if (srcController->blockType == BSEFFECTSHADERPROPERTYCOLORCONTROLLER) {
			auto controller = static_cast<BSEffectShaderPropertyColorController*>(srcController);
			BSEffectShaderPropertyColorController* controllerCopy = new BSEffectShaderPropertyColorController(*controller);
			destController = static_cast<NiTimeController*>(controllerCopy);
		}
		else if (srcController->blockType == BSEFFECTSHADERPROPERTYFLOATCONTROLLER) {
			auto controller = static_cast<BSEffectShaderPropertyFloatController*>(srcController);
			BSEffectShaderPropertyFloatController* controllerCopy = new BSEffectShaderPropertyFloatController(*controller);
			destController = static_cast<NiTimeController*>(controllerCopy);
		}
		else
			destShader->SetControllerRef(0xFFFFFFFF);

		if (destController) {
			int controllerId = hdr.AddBlock(destController, srcNif.hdr.GetBlockTypeStringById(srcShader->GetControllerRef()));
			destController->targetRef = shaderId;
			destShader->SetControllerRef(controllerId);
		}
	}

	if (srcShader->blockType == BSLIGHTINGSHADERPROPERTY || srcShader->blockType == BSEFFECTSHADERPROPERTY)
		shape->SetShaderPropertyRef(shaderId);
	else if (srcShader->blockType == BSSHADERPPLIGHTINGPROPERTY)
		shape->propertiesRef[propRef1] = shaderId;

	// Kill normals and tangents
	if (destShader->IsSkinTint() && hdr.GetUserVersion() >= 12) {
		if (shape->blockType == NITRISHAPE || shape->blockType == NITRISTRIPS) {
			shape->SetNormals(false);
			shape->SetTangents(false);
		}
	}

	// Create alpha property and copy
	if (srcAlphaProp) {
		auto destAlphaProp = new NiAlphaProperty(*srcAlphaProp);
		destAlphaProp->header = &hdr;

		if (hdr.GetUserVersion() == 12 && hdr.GetUserVersion2() >= 120)
			destAlphaProp->SetName(srcAlphaProp->GetName());
		else
			destAlphaProp->ClearName();

		int alphaPropId = hdr.AddBlock(destAlphaProp, "NiAlphaProperty");
		if (srcShader->blockType == BSLIGHTINGSHADERPROPERTY || srcShader->blockType == BSEFFECTSHADERPROPERTY)
			shape->SetAlphaPropertyRef(alphaPropId);
		else if (srcShader->blockType == BSSHADERPPLIGHTINGPROPERTY)
			shape->propertiesRef[propRef2] = alphaPropId;
	}
}

int NifFile::CopyNamedNode(string& nodeName, NifFile& srcNif) {
	NiNode* srcNode = srcNif.FindNodeByName(nodeName);
	if (!srcNode)
		return 0xFFFFFFFF;

	auto destNode = new NiNode(*srcNode);
	destNode->header = &hdr;
	destNode->SetName(nodeName);

	return hdr.AddBlock(destNode, "NiNode");
}

void NifFile::CopyGeometry(const string& shapeDest, NifFile& srcNif, const string& srcShape) {
	NiShape* srcGeom = srcNif.FindShapeByName(srcShape);
	if (!srcGeom)
		return;

	NiShape* destGeom = nullptr;
	if (srcGeom->blockType == NITRISHAPE) {
		auto shape = static_cast<NiTriShape*>(srcGeom);
		auto destShape = new NiTriShape(*shape);
		destGeom = static_cast<NiShape*>(destShape);
	}
	else if (srcGeom->blockType == NITRISTRIPS) {
		auto strips = static_cast<NiTriStrips*>(srcGeom);
		auto destStrips = new NiTriStrips(*strips);
		destGeom = static_cast<NiShape*>(destStrips);
	}
	if (srcGeom->blockType == BSSUBINDEXTRISHAPE) {
		auto shape = static_cast<BSSubIndexTriShape*>(srcGeom);
		auto destShape = new BSSubIndexTriShape(*shape);
		destGeom = static_cast<NiShape*>(destShape);
	}
	else if (srcGeom->blockType == BSMESHLODTRISHAPE) {
		auto shape = static_cast<BSMeshLODTriShape*>(srcGeom);
		auto destShape = new BSMeshLODTriShape(*shape);
		destGeom = static_cast<NiShape*>(destShape);
	}
	else if (srcGeom->blockType == BSTRISHAPE) {
		auto shape = static_cast<BSTriShape*>(srcGeom);
		auto destShape = new BSTriShape(*shape);
		destGeom = static_cast<NiShape*>(destShape);
	}

	if (!destGeom)
		return;

	destGeom->header = &hdr;
	destGeom->SetName(shapeDest);

	int destId = hdr.AddBlock(destGeom, srcNif.hdr.GetBlockTypeStringById(srcNif.GetBlockID(srcGeom)));

	auto srcGeomData = srcNif.hdr.GetBlock<NiTriBasedGeomData>(srcGeom->GetDataRef());
	if (srcGeomData) {
		NiTriBasedGeomData* destGeomData = nullptr;
		if (srcGeomData->blockType == NITRISHAPEDATA) {
			auto shapeData = static_cast<NiTriShapeData*>(srcGeomData);
			NiTriShapeData* destShapeData = new NiTriShapeData(*shapeData);
			destGeomData = static_cast<NiTriBasedGeomData*>(destShapeData);
		}
		else if (srcGeomData->blockType == NITRISTRIPSDATA) {
			auto stripsData = static_cast<NiTriStripsData*>(srcGeomData);
			NiTriStripsData* destStripsData = new NiTriStripsData(*stripsData);
			destGeomData = static_cast<NiTriBasedGeomData*>(destStripsData);
		}

		if (destGeomData) {
			destGeomData->header = &hdr;

			int destDataId = hdr.AddBlock(destGeomData, srcNif.hdr.GetBlockTypeStringById(srcGeom->GetDataRef()));
			destGeom->SetDataRef(destDataId);
		}
	}

	NiBoneContainer* destBoneCont = nullptr;
	if (srcGeom->GetSkinInstanceRef() != 0xFFFFFFFF) {
		if (destGeom->blockType == NITRISHAPE || destGeom->blockType == NITRISTRIPS) {
			auto srcSkinInst = srcNif.hdr.GetBlock<NiSkinInstance>(srcGeom->GetSkinInstanceRef());
			if (srcSkinInst) {
				auto srcSkinData = srcNif.hdr.GetBlock<NiSkinData>(srcSkinInst->GetDataRef());
				auto srcSkinPart = srcNif.hdr.GetBlock<NiSkinPartition>(srcSkinInst->GetSkinPartitionRef());

				NiSkinInstance* destSkinInst = nullptr;
				if (srcSkinInst->blockType == NISKININSTANCE) {
					destSkinInst = new NiSkinInstance(*srcSkinInst);
				}
				else if (srcSkinInst->blockType == BSDISMEMBERSKININSTANCE) {
					auto srcBsdSkinInst = static_cast<BSDismemberSkinInstance*>(srcSkinInst);
					auto destBsdSkinInst = new BSDismemberSkinInstance(*srcBsdSkinInst);
					destSkinInst = static_cast<NiSkinInstance*>(destBsdSkinInst);
				}

				if (destSkinInst) {
					destSkinInst->header = &hdr;

					// Treat skinning and partition info as blobs of anonymous data.
					auto destSkinData = new NiSkinData(*srcSkinData);
					destSkinData->header = &hdr;
					auto destSkinPart = new NiSkinPartition(*srcSkinPart);
					destSkinPart->header = &hdr;

					int destSkinId = hdr.AddBlock(destSkinInst, srcNif.hdr.GetBlockTypeStringById(srcGeom->GetSkinInstanceRef()));
					int destSkinDataId = hdr.AddBlock(destSkinData, srcNif.hdr.GetBlockTypeStringById(srcSkinInst->GetDataRef()));
					int destSkinPartId = hdr.AddBlock(destSkinPart, srcNif.hdr.GetBlockTypeStringById(srcSkinInst->GetSkinPartitionRef()));

					destGeom->SetSkinInstanceRef(destSkinId);
					destSkinInst->SetDataRef(destSkinDataId);
					destSkinInst->SetSkinPartitionRef(destSkinPartId);

					destBoneCont = static_cast<NiBoneContainer*>(destSkinInst);
				}
			}
		}
		else if (destGeom->blockType == BSTRISHAPE || destGeom->blockType == BSSUBINDEXTRISHAPE || destGeom->blockType == BSMESHLODTRISHAPE) {
			auto srcBSSkinInst = srcNif.hdr.GetBlock<BSSkinInstance>(srcGeom->GetSkinInstanceRef());
			if (srcBSSkinInst) {
				auto destBSSkinInst = new BSSkinInstance(*srcBSSkinInst);

				int destSkinInstId = hdr.AddBlock(destBSSkinInst, srcNif.hdr.GetBlockTypeStringById(srcGeom->GetSkinInstanceRef()));
				destGeom->SetSkinInstanceRef(destSkinInstId);

				auto srcBoneData = srcNif.hdr.GetBlock<BSSkinBoneData>(srcBSSkinInst->GetDataRef());
				if (srcBoneData) {
					auto destBoneData = new BSSkinBoneData(*srcBoneData);

					int destBoneDataId = hdr.AddBlock(destBoneData, srcNif.hdr.GetBlockTypeStringById(srcBSSkinInst->GetDataRef()));
					destBSSkinInst->SetDataRef(destBoneDataId);

					destBoneCont = static_cast<NiBoneContainer*>(destBSSkinInst);
				}
			}
		}
	}

	CopyShader(shapeDest, srcNif);

	for (int i = 0; i < srcGeom->numProperties; i++) {
		auto material = srcNif.hdr.GetBlock<NiMaterialProperty>(srcGeom->propertiesRef[i]);
		if (material) {
			auto destMaterial = new NiMaterialProperty(*material);
			destMaterial->header = &hdr;

			int materialId = hdr.AddBlock(destMaterial, srcNif.hdr.GetBlockTypeStringById(srcGeom->propertiesRef[i]));
			destGeom->propertiesRef[i] = materialId;
			continue;
		}

		auto stencil = srcNif.hdr.GetBlock<NiStencilProperty>(srcGeom->propertiesRef[i]);
		if (stencil) {
			auto destStencil = new NiStencilProperty(*stencil);
			destStencil->header = &hdr;

			int stencilId = hdr.AddBlock(destStencil, srcNif.hdr.GetBlockTypeStringById(srcGeom->propertiesRef[i]));
			destGeom->propertiesRef[i] = stencilId;
			continue;
		}

		auto srcUnknown = srcNif.hdr.GetBlock<NiUnknown>(srcGeom->propertiesRef[i]);
		if (srcUnknown) {
			auto destUnknown = new NiUnknown(srcUnknown->CalcBlockSize());
			destUnknown->Clone(srcUnknown);

			int unknownId = hdr.AddBlock(destUnknown, srcNif.hdr.GetBlockTypeStringById(srcGeom->propertiesRef[i]));
			destGeom->propertiesRef[i] = unknownId;
		}
	}

	vector<string> srcBoneList;
	srcNif.GetShapeBoneList(srcShape, srcBoneList);
	if (destBoneCont)
		destBoneCont->bones.clear();

	auto rootNode = dynamic_cast<NiNode*>(blocks[0]);
	if (rootNode) {
		for (auto &boneName : srcBoneList) {
			int boneID = GetBlockID(FindNodeByName(boneName));
			if (boneID == 0xFFFFFFFF) {
				boneID = CopyNamedNode(boneName, srcNif);
				rootNode->AddChildRef(boneID);
			}

			if (destBoneCont)
				destBoneCont->bones.push_back(boneID);
		}

		rootNode->AddChildRef(destId);
	}
}

int NifFile::Save(const string& filename, bool optimize) {
	fstream file(filename.c_str(), ios_base::out | ios_base::binary);
	if (file.is_open()) {
		if (optimize)
			Optimize();

		hdr.CalcAllBlockSizes();
		hdr.Put(file);

		for (int i = 0; i < hdr.GetNumBlocks(); i++)
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
	PrettySortBlocks();

	vector<string> shapes;
	GetShapeList(shapes);

	for (auto &s : shapes)
		UpdateBoundingSphere(s);

	RemoveUnusedStrings();
}


BlockType NifFile::GetShapeType(const string& shapeName) {
	NiShape* shape = FindShapeByName(shapeName);
	if (!shape)
		return NIUNKNOWN;

	return shape->blockType;
}

int NifFile::GetShapeList(vector<string>& outList) {
	outList.clear();
	for (auto& block : blocks) {
		BlockType type = block->blockType;
		if (type == NITRISHAPE || type == NITRISTRIPS || type == BSSUBINDEXTRISHAPE || type == BSTRISHAPE || type == BSMESHLODTRISHAPE) {
			auto shape = static_cast<NiShape*>(block);
			if (shape)
				outList.push_back(shape->GetName());
		}
	}
	return outList.size();
}

void NifFile::RenameShape(const string& oldName, const string& newName) {
	NiAVObject* geom = FindAVObjectByName(oldName);
	if (geom)
		geom->SetName(newName, true);
}

void NifFile::RenameDuplicateShape(const string& dupedShape) {
	int dupCount = 1;
	char buf[10];

	NiAVObject* geom = FindAVObjectByName(dupedShape);
	if (geom) {
		while ((geom = FindAVObjectByName(dupedShape, 1)) != nullptr) {
			_snprintf(buf, 10, "_%d", dupCount);
			while (hdr.FindStringId(geom->GetName() + buf) != 0xFFFFFFFF) {
				dupCount++;
				_snprintf(buf, 10, "_%d", dupCount);
			}

			geom->SetName(geom->GetName() + buf);
			dupCount++;
		}
	}
}

int NifFile::GetRootNodeID() {
	if (blocks.empty())
		return 0xFFFFFFFF;

	return 0;
}

bool NifFile::GetNodeTransform(const string& nodeName, vector<Vector3>& outRot, Vector3& outTrans, float& outScale) {
	for (auto& block : blocks) {
		if (block->blockType == NINODE) {
			auto node = static_cast<NiNode*>(block);
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
		auto root = dynamic_cast<NiNode*>(blocks[0]);
		if (root) {
			for (int i = 0; i < root->GetNumChildren(); i++) {
				auto node = hdr.GetBlock<NiNode>(root->GetChildRef(i));
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
				auto node = static_cast<NiNode*>(block);
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
	outList.clear();

	NiShape* shape = FindShapeByName(shapeName);
	if (!shape)
		return 0;

	auto skinInst = hdr.GetBlock<NiBoneContainer>(shape->GetSkinInstanceRef());
	if (!skinInst)
		return 0;

	for (int i = 0; i < skinInst->bones.size(); i++) {
		auto node = hdr.GetBlock<NiNode>(skinInst->bones[i]);
		if (node)
			outList.push_back(node->GetName());
	}

	return outList.size();
}

int NifFile::GetShapeBoneIDList(const string& shapeName, vector<int>& outList) {
	outList.clear();

	NiShape* shape = FindShapeByName(shapeName);
	if (!shape)
		return 0;

	auto skinInst = hdr.GetBlock<NiBoneContainer>(shape->GetSkinInstanceRef());
	if (!skinInst)
		return 0;

	for (int i = 0; i < skinInst->bones.size(); i++)
		outList.push_back(skinInst->bones[i]);

	return outList.size();
}

void NifFile::SetShapeBoneIDList(const string& shapeName, vector<int>& inList) {
	NiShape* shape = FindShapeByName(shapeName);
	if (!shape)
		return;

	if (shape->GetSkinInstanceRef() == 0xFFFFFFFF)
		return;

	BSSkinBoneData* boneData = nullptr;
	if (shape->blockType == BSTRISHAPE || shape->blockType == BSSUBINDEXTRISHAPE || shape->blockType == BSMESHLODTRISHAPE) {
		auto skinForBoneRef = hdr.GetBlock<BSSkinInstance>(shape->GetSkinInstanceRef());
		if (skinForBoneRef)
			boneData = hdr.GetBlock<BSSkinBoneData>(skinForBoneRef->GetDataRef());
	}

	auto boneCont = hdr.GetBlock<NiBoneContainer>(shape->GetSkinInstanceRef());
	if (!boneCont)
		return;

	boneCont->bones.clear();
	boneCont->numBones = 0;

	bool feedBoneData = false;
	if (boneData && boneData->nBones != inList.size()) {
		// Clear if size doesn't match
		boneData->nBones = 0;
		boneData->boneXforms.clear();
		feedBoneData = true;
	}

	for (auto &i : inList) {
		boneCont->bones.push_back(i);
		boneCont->numBones++;
		if (boneData && feedBoneData) {
			boneData->boneXforms.emplace_back();
			boneData->nBones++;
		}
	}

	auto skinInst = dynamic_cast<NiSkinInstance*>(boneCont);
	if (skinInst) {
		auto skinData = hdr.GetBlock<NiSkinData>(skinInst->GetDataRef());
		if (skinData) {
			int nBonesToAdd = skinInst->bones.size() - skinData->numBones;
			if (nBonesToAdd > 0) {
				for (int i = 0; i < nBonesToAdd; i++) {
					skinData->bones.emplace_back();
					skinData->bones.back().numVertices = 0;
					skinData->numBones++;
				}
			}
		}
	}
}

int NifFile::GetShapeBoneWeights(const string& shapeName, int boneIndex, unordered_map<ushort, float>& outWeights) {
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

	auto skinInst = hdr.GetBlock<NiSkinInstance>(shape->GetSkinInstanceRef());
	if (!skinInst)
		return 0;

	auto skinData = hdr.GetBlock<NiSkinData>(skinInst->GetDataRef());
	if (!skinData || boneIndex > skinData->numBones)
		return 0;

	NiSkinData::BoneData* bone = &skinData->bones[boneIndex];
	for (auto &sw : bone->vertexWeights) {
		if (sw.weight >= 0.0001f)
			outWeights.emplace(sw.index, sw.weight);
		else
			outWeights.emplace(sw.index, 0.0f);
	}

	return outWeights.size();
}

bool NifFile::GetShapeBoneTransform(const string& shapeName, const string& boneName, SkinTransform& outXform, BoundingSphere& outBounds) {
	NiShape* shape = FindShapeByName(shapeName);
	if (!shape)
		return false;

	int boneIndex = shape->GetBoneID(boneName);
	if (boneName.empty())
		boneIndex = 0xFFFFFFFF;

	return GetShapeBoneTransform(shapeName, boneIndex, outXform, outBounds);
}

bool NifFile::SetShapeBoneTransform(const string& shapeName, int boneIndex, SkinTransform& inXform, BoundingSphere& inBounds) {
	NiShape* shape = FindShapeByName(shapeName);
	if (!shape)
		return false;

	auto skinForBoneRef = hdr.GetBlock<BSSkinInstance>(shape->GetSkinInstanceRef());
	if (skinForBoneRef && boneIndex != 0xFFFFFFFF) {
		auto bsSkin = hdr.GetBlock<BSSkinBoneData>(skinForBoneRef->GetDataRef());
		if (!bsSkin)
			return false;

		bsSkin->boneXforms[boneIndex].bounds = inBounds;
		bsSkin->boneXforms[boneIndex].boneTransform = inXform;
		return true;
	}

	auto skinInst = hdr.GetBlock<NiSkinInstance>(shape->GetSkinInstanceRef());
	if (!skinInst)
		return false;

	auto skinData = hdr.GetBlock<NiSkinData>(skinInst->GetDataRef());
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
	bone->bounds = inBounds;
	return true;
}

bool NifFile::GetShapeBoneTransform(const string& shapeName, int boneIndex, SkinTransform& outXform, BoundingSphere& outBounds) {
	NiShape* shape = FindShapeByName(shapeName);
	if (!shape)
		return false;

	auto skinForBoneRef = hdr.GetBlock<BSSkinInstance>(shape->GetSkinInstanceRef());
	if (skinForBoneRef) {
		auto boneData = hdr.GetBlock<BSSkinBoneData>(skinForBoneRef->GetDataRef());
		if (boneData) {
			if (boneIndex == 0xFFFFFFFF) {
				// Overall skin transform not found in FO4 meshes :(
				return false;
			}

			outXform = boneData->boneXforms[boneIndex].boneTransform;
			outBounds = boneData->boneXforms[boneIndex].bounds;
			return true;
		}
	}

	auto skinInst = hdr.GetBlock<NiSkinInstance>(shape->GetSkinInstanceRef());
	if (!skinInst)
		return false;

	auto skinData = hdr.GetBlock<NiSkinData>(skinInst->GetDataRef());
	if (!skinData)
		return false;

	if (boneIndex == 0xFFFFFFFF) {
		// Want the overall skin transform
		outXform = skinData->skinTransform;
		outBounds = BoundingSphere();
		return true;
	}

	if (boneIndex > skinData->numBones)
		return 0;

	NiSkinData::BoneData* bone = &skinData->bones[boneIndex];
	outXform = bone->boneTransform;
	outBounds = bone->bounds;
	return true;
}

void NifFile::UpdateShapeBoneID(const string& shapeName, int oldID, int newID) {
	NiShape* shape = FindShapeByName(shapeName);
	if (!shape)
		return;

	auto boneCont = hdr.GetBlock<NiBoneContainer>(shape->GetSkinInstanceRef());
	if (!boneCont)
		return;

	for (auto &bp : boneCont->bones) {
		if (bp == oldID) {
			bp = newID;
			return;
		}
	}
}

// Not implemented for BSTriShape, use SetShapeVertWeights instead
void NifFile::SetShapeBoneWeights(const string& shapeName, int boneIndex, unordered_map<ushort, float>& inWeights) {
	NiShape* shape = FindShapeByName(shapeName);
	if (!shape)
		return;

	auto skinInst = hdr.GetBlock<NiSkinInstance>(shape->GetSkinInstanceRef());
	if (!skinInst)
		return;

	auto skinData = hdr.GetBlock<NiSkinData>(skinInst->GetDataRef());
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

void NifFile::SetShapeVertWeights(const string& shapeName, int vertIndex, vector<byte>& boneids, vector<float>& weights) {
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

bool NifFile::GetShapeSegments(const string& shapeName, BSSubIndexTriShape::BSSITSSegmentation& segmentation) {
	BSSubIndexTriShape* siTriShape = dynamic_cast<BSSubIndexTriShape*>(FindShapeByName(shapeName));
	if (!siTriShape)
		return false;

	segmentation = siTriShape->GetSegmentation();
	return true;
}

void NifFile::SetShapeSegments(const string& shapeName, const BSSubIndexTriShape::BSSITSSegmentation& segmentation) {
	BSSubIndexTriShape* siTriShape = dynamic_cast<BSSubIndexTriShape*>(FindShapeByName(shapeName));
	if (!siTriShape)
		return;

	siTriShape->SetSegmentation(segmentation);
}

bool NifFile::GetShapePartitions(const string& shapeName, vector<BSDismemberSkinInstance::PartitionInfo>& partitionInfo, vector<vector<ushort>>& verts, vector<vector<Triangle>>& tris) {
	NiShape* shape = FindShapeByName(shapeName);
	if (!shape)
		return false;

	auto bsdSkinInst = hdr.GetBlock<BSDismemberSkinInstance>(shape->GetSkinInstanceRef());
	if (bsdSkinInst)
		partitionInfo = bsdSkinInst->GetPartitions();
	else
		partitionInfo.clear();

	auto skinInst = hdr.GetBlock<NiSkinInstance>(shape->GetSkinInstanceRef());
	if (!skinInst)
		return false;

	auto skinPart = hdr.GetBlock<NiSkinPartition>(skinInst->GetSkinPartitionRef());
	if (!skinPart)
		return false;

	for (auto &part : skinPart->partitions) {
		verts.push_back(part.vertexMap);
		tris.push_back(part.triangles);
	}

	return true;
}

void NifFile::SetShapePartitions(const string& shapeName, const vector<BSDismemberSkinInstance::PartitionInfo>& partitionInfo, const vector<vector<ushort>>& verts, const vector<vector<Triangle>>& tris) {
	NiShape* shape = FindShapeByName(shapeName);
	if (!shape)
		return;

	auto bsdSkinInst = hdr.GetBlock<BSDismemberSkinInstance>(shape->GetSkinInstanceRef());
	if (bsdSkinInst)
		bsdSkinInst->SetPartitions(partitionInfo);

	auto skinInst = hdr.GetBlock<NiSkinInstance>(shape->GetSkinInstanceRef());
	if (!skinInst)
		return;

	auto skinPart = hdr.GetBlock<NiSkinPartition>(skinInst->GetSkinPartitionRef());
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

		skinPart->partitions[i].numTriangles = tris[i].size();
		if (!tris[i].empty())
			skinPart->partitions[i].triangles = tris[i];
	}
}

const vector<Vector3>* NifFile::GetRawVertsForShape(const string& shapeName) {
	NiShape* shape = FindShapeByName(shapeName);
	if (!shape)
		return nullptr;

	if (shape->blockType == NITRISHAPE || shape->blockType == NITRISTRIPS) {
		auto geomData = hdr.GetBlock<NiGeometryData>(shape->GetDataRef());
		if (geomData)
			return &geomData->vertices;
	}
	else if (shape->blockType == BSSUBINDEXTRISHAPE || shape->blockType == BSTRISHAPE || shape->blockType == BSMESHLODTRISHAPE) {
		auto bsTriShape = dynamic_cast<BSTriShape*>(shape);
		if (bsTriShape)
			return bsTriShape->GetRawVerts();
	}

	return nullptr;
}

bool NifFile::GetTrisForShape(const string& shapeName, vector<Triangle>* outTris) {
	NiShape* shape = FindShapeByName(shapeName);
	if (!shape)
		return false;

	if (shape->blockType == NITRISHAPE) {
		auto shapeData = hdr.GetBlock<NiTriShapeData>(shape->GetDataRef());
		if (shapeData) {
			*outTris = shapeData->triangles;
			return true;
		}
	}
	else if (shape->blockType == NITRISTRIPS) {
		auto stripsData = hdr.GetBlock<NiTriStripsData>(shape->GetDataRef());
		if (stripsData) {
			stripsData->StripsToTris(outTris);
			return true;
		}
	}
	else if (shape->blockType == BSSUBINDEXTRISHAPE || shape->blockType == BSTRISHAPE || shape->blockType == BSMESHLODTRISHAPE) {
		auto bsTriShape = dynamic_cast<BSTriShape*>(shape);
		if (bsTriShape) {
			*outTris = bsTriShape->triangles;
			return true;
		}
	}

	return false;
}

bool NifFile::ReorderTriangles(const string& shapeName, const vector<uint>& triangleIndices) {
	NiShape* shape = FindShapeByName(shapeName);
	if (!shape)
		return false;

	vector<Triangle> triangles;
	if (shape->blockType == NITRISHAPE) {
		auto shapeData = hdr.GetBlock<NiTriShapeData>(shape->GetDataRef());
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
	else if (shape->blockType == NITRISTRIPS) {
		return false;
	}
	else if (shape->blockType == BSSUBINDEXTRISHAPE || shape->blockType == BSTRISHAPE || shape->blockType == BSMESHLODTRISHAPE) {
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

const vector<Vector3>* NifFile::GetNormalsForShape(const string& shapeName, bool transform) {
	NiShape* shape = FindShapeByName(shapeName);
	if (!shape)
		return nullptr;

	if (shape->blockType == NITRISHAPE || shape->blockType == NITRISTRIPS) {
		auto geomData = hdr.GetBlock<NiGeometryData>(shape->GetDataRef());
		if (geomData)
			return &geomData->normals;
	}
	else if (shape->blockType == BSSUBINDEXTRISHAPE || shape->blockType == BSTRISHAPE || shape->blockType == BSMESHLODTRISHAPE) {
		auto bsTriShape = dynamic_cast<BSTriShape*>(shape);
		if (bsTriShape)
			return bsTriShape->GetNormalData(transform);
	}

	return nullptr;
}

const vector<Vector2>* NifFile::GetUvsForShape(const string& shapeName) {
	NiShape* shape = FindShapeByName(shapeName);
	if (!shape)
		return nullptr;

	if (shape->blockType == NITRISHAPE || shape->blockType == NITRISTRIPS) {
		auto geomData = hdr.GetBlock<NiGeometryData>(shape->GetDataRef());
		if (geomData)
			return &geomData->uvSets;
	}
	else if (shape->blockType == BSSUBINDEXTRISHAPE || shape->blockType == BSTRISHAPE || shape->blockType == BSMESHLODTRISHAPE) {
		auto bsTriShape = dynamic_cast<BSTriShape*>(shape);
		if (bsTriShape)
			return bsTriShape->GetUVData();
	}

	return nullptr;
}

bool NifFile::GetUvsForShape(const string& shapeName, vector<Vector2>& outUvs) {
	const vector<Vector2>* uvData = GetUvsForShape(shapeName);
	if (uvData) {
		outUvs.assign(uvData->begin(), uvData->end());
		return true;
	}

	return false;
}

bool NifFile::GetVertsForShape(const string& shapeName, vector<Vector3>& outVerts) {
	NiShape* shape = FindShapeByName(shapeName);
	if (!shape) {
		outVerts.clear();
		return false;
	}

	if (shape->blockType == NITRISHAPE || shape->blockType == NITRISTRIPS) {
		auto geomData = hdr.GetBlock<NiGeometryData>(shape->GetDataRef());
		if (geomData) {
			outVerts.resize(geomData->numVertices);

			for (int i = 0; i < geomData->numVertices; i++)
				outVerts[i] = geomData->vertices[i];

			return true;
		}
	}
	else if (shape->blockType == BSSUBINDEXTRISHAPE || shape->blockType == BSTRISHAPE || shape->blockType == BSMESHLODTRISHAPE) {
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

int NifFile::GetVertCountForShape(const string& shapeName) {
	NiShape* shape = FindShapeByName(shapeName);
	if (!shape)
		return 0xFFFFFFFF;

	if (shape->blockType == NITRISHAPE || shape->blockType == NITRISTRIPS) {
		auto geomData = hdr.GetBlock<NiGeometryData>(shape->GetDataRef());
		if (geomData)
			return geomData->numVertices;
	}
	else if (shape->blockType == BSSUBINDEXTRISHAPE || shape->blockType == BSTRISHAPE || shape->blockType == BSMESHLODTRISHAPE) {
		auto bsTriShape = dynamic_cast<BSTriShape*>(shape);
		if (bsTriShape)
			return bsTriShape->numVertices;
	}

	return 0xFFFFFFFF;
}

void NifFile::SetVertsForShape(const string& shapeName, const vector<Vector3>& verts) {
	NiShape* shape = FindShapeByName(shapeName);
	if (!shape)
		return;

	if (shape->blockType == NITRISHAPE || shape->blockType == NITRISTRIPS) {
		auto geomData = hdr.GetBlock<NiGeometryData>(shape->GetDataRef());
		if (geomData) {
			if (verts.size() != geomData->vertices.size())
				return;

			for (int i = 0; i < geomData->vertices.size(); i++)
				geomData->vertices[i] = verts[i];
		}
	}
	else if (shape->blockType == BSSUBINDEXTRISHAPE || shape->blockType == BSTRISHAPE || shape->blockType == BSMESHLODTRISHAPE) {
		auto bsTriShape = dynamic_cast<BSTriShape*>(shape);
		if (bsTriShape) {
			if (verts.size() != bsTriShape->numVertices)
				return;

			for (int i = 0; i < bsTriShape->numVertices; i++)
				bsTriShape->vertData[i].vert = verts[i];
		}
	}
}

void NifFile::SetUvsForShape(const string& shapeName, const vector<Vector2>& uvs) {
	NiShape* shape = FindShapeByName(shapeName);
	if (!shape)
		return;

	if (shape->blockType == NITRISHAPE || shape->blockType == NITRISTRIPS) {
		auto geomData = hdr.GetBlock<NiGeometryData>(shape->GetDataRef());
		if (geomData) {
			if (uvs.size() != geomData->vertices.size())
				return;

			geomData->uvSets.assign(uvs.begin(), uvs.end());
		}
	}
	else if (shape->blockType == BSSUBINDEXTRISHAPE || shape->blockType == BSTRISHAPE || shape->blockType == BSMESHLODTRISHAPE) {
		auto bsTriShape = dynamic_cast<BSTriShape*>(shape);
		if (bsTriShape) {
			if (uvs.size() != bsTriShape->vertData.size())
				return;

			for (int i = 0; i < bsTriShape->numVertices; i++)
				bsTriShape->vertData[i].uv = uvs[i];
		}
	}
}

void NifFile::InvertUVsForShape(const string& shapeName, bool invertX, bool invertY) {
	NiShape* shape = FindShapeByName(shapeName);
	if (!shape)
		return;

	if (shape->blockType == NITRISHAPE || shape->blockType == NITRISTRIPS) {
		auto geomData = hdr.GetBlock<NiGeometryData>(shape->GetDataRef());
		if (geomData) {
			if (invertX)
				for (int i = 0; i < geomData->uvSets.size(); ++i)
					geomData->uvSets[i].u = 1.0f - geomData->uvSets[i].u;

			if (invertY)
				for (int i = 0; i < geomData->uvSets.size(); ++i)
					geomData->uvSets[i].v = 1.0f - geomData->uvSets[i].v;
		}
	}
	else if (shape->blockType == BSSUBINDEXTRISHAPE || shape->blockType == BSTRISHAPE || shape->blockType == BSMESHLODTRISHAPE) {
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

void NifFile::SetNormalsForShape(const string& shapeName, const vector<Vector3>& norms) {
	NiShape* shape = FindShapeByName(shapeName);
	if (!shape)
		return;

	if (shape->blockType == NITRISHAPE || shape->blockType == NITRISTRIPS) {
		auto geomData = hdr.GetBlock<NiGeometryData>(shape->GetDataRef());
		if (geomData) {
			geomData->SetNormals(true);

			for (int i = 0; i < geomData->vertices.size(); i++)
				geomData->normals[i] = norms[i];
		}
	}
	else if (shape->blockType == BSSUBINDEXTRISHAPE || shape->blockType == BSTRISHAPE || shape->blockType == BSMESHLODTRISHAPE) {
		auto bsTriShape = dynamic_cast<BSTriShape*>(shape);
		if (bsTriShape)
			bsTriShape->SetNormals(norms);
	}
}

void NifFile::CalcNormalsForShape(const string& shapeName, const bool& smooth, const float& smoothThresh) {
	NiShape* shape = FindShapeByName(shapeName);
	if (!shape)
		return;

	if (shape->blockType == NITRISHAPE || shape->blockType == NITRISTRIPS) {
		auto geomData = hdr.GetBlock<NiGeometryData>(shape->GetDataRef());
		if (geomData)
			geomData->RecalcNormals(smooth, smoothThresh);
	}
	else if (shape->blockType == BSSUBINDEXTRISHAPE || shape->blockType == BSTRISHAPE || shape->blockType == BSMESHLODTRISHAPE) {
		auto bsTriShape = dynamic_cast<BSTriShape*>(shape);
		if (bsTriShape)
			bsTriShape->RecalcNormals(smooth, smoothThresh);
	}
}

void NifFile::CalcTangentsForShape(const string& shapeName) {
	NiShape* shape = FindShapeByName(shapeName);
	if (!shape)
		return;

	if (shape->blockType == NITRISHAPE || shape->blockType == NITRISTRIPS) {
		auto geomData = hdr.GetBlock<NiGeometryData>(shape->GetDataRef());
		if (geomData)
			geomData->CalcTangentSpace();
	}
	else if (shape->blockType == BSSUBINDEXTRISHAPE || shape->blockType == BSTRISHAPE || shape->blockType == BSMESHLODTRISHAPE) {
		auto bsTriShape = dynamic_cast<BSTriShape*>(shape);
		if (bsTriShape)
			bsTriShape->CalcTangentSpace();
	}
}

void NifFile::ClearShapeTransform(const string& shapeName) {
	NiAVObject* avo = FindAVObjectByName(shapeName);
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

void NifFile::SetRootScale(const float& newScale) {
	auto root = dynamic_cast<NiNode*>(blocks[0]);
	if (root)
		root->scale = newScale;
}

void NifFile::GetShapeTranslation(const string& shapeName, Vector3& outVec) {
	NiAVObject* avo = FindAVObjectByName(shapeName);
	if (avo)
		outVec = avo->translation;

	auto root = dynamic_cast<NiNode*>(blocks[0]);
	if (root)
		outVec += root->translation;

	//if (outVec.DistanceTo(Vector3(0.0f, 0.0f, 0.0f)) < EPSILON)
	//	outVec.Zero();
}

void NifFile::SetShapeTranslation(const string& shapeName, const Vector3& newTrans) {
	NiAVObject* avo = FindAVObjectByName(shapeName);
	if (avo)
		avo->translation = newTrans;
}

void NifFile::GetShapeScale(const string& shapeName, float& outScale) {
	NiAVObject* avo = FindAVObjectByName(shapeName);
	if (avo)
		outScale = avo->scale;
}

void NifFile::SetShapeScale(const string& shapeName, const float& newScale) {
	NiAVObject* avo = FindAVObjectByName(shapeName);
	if (avo)
		avo->scale = newScale;
}

void NifFile::ApplyShapeTranslation(const string& shapeName, const Vector3& offset) {
	NiShape* shape = FindShapeByName(shapeName);
	if (!shape)
		return;

	if (shape->blockType == NITRISHAPE || shape->blockType == NITRISTRIPS) {
		auto geom = dynamic_cast<NiGeometry*>(shape);
		if (!geom)
			return;

		auto geomData = hdr.GetBlock<NiGeometryData>(shape->GetDataRef());
		if (geomData) {
			for (int i = 0; i < geomData->vertices.size(); i++)
				geomData->vertices[i] += geom->translation + offset;

			geom->translation = Vector3(0.0f, 0.0f, 0.0f);
		}
	}
	else if (shape->blockType == BSSUBINDEXTRISHAPE || shape->blockType == BSTRISHAPE || shape->blockType == BSMESHLODTRISHAPE) {
		auto bsTriShape = dynamic_cast<BSTriShape*>(shape);
		if (bsTriShape) {
			for (int i = 0; i < bsTriShape->numVertices; i++)
				bsTriShape->vertData[i].vert += bsTriShape->translation + offset;

			bsTriShape->translation = Vector3(0.0f, 0.0f, 0.0f);
		}
	}
}

void NifFile::MoveVertex(const string& shapeName, const Vector3& pos, const int& id) {
	NiShape* shape = FindShapeByName(shapeName);
	if (!shape)
		return;

	if (shape->blockType == NITRISHAPE || shape->blockType == NITRISTRIPS) {
		auto geomData = hdr.GetBlock<NiGeometryData>(shape->GetDataRef());
		if (geomData && geomData->numVertices > id)
			geomData->vertices[id] = pos;
	}
	else if (shape->blockType == BSSUBINDEXTRISHAPE || shape->blockType == BSTRISHAPE || shape->blockType == BSMESHLODTRISHAPE) {
		auto bsTriShape = dynamic_cast<BSTriShape*>(shape);
		if (bsTriShape && bsTriShape->numVertices > id)
			bsTriShape->vertData[id].vert = pos;
	}
}

void NifFile::OffsetShape(const string& shapeName, const Vector3& offset, unordered_map<ushort, float>* mask) {
	NiShape* shape = FindShapeByName(shapeName);
	if (!shape)
		return;

	if (shape->blockType == NITRISHAPE || shape->blockType == NITRISTRIPS) {
		auto geomData = hdr.GetBlock<NiGeometryData>(shape->GetDataRef());
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
	else if (shape->blockType == BSSUBINDEXTRISHAPE || shape->blockType == BSTRISHAPE || shape->blockType == BSMESHLODTRISHAPE) {
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

void NifFile::ScaleShape(const string& shapeName, const Vector3& scale, unordered_map<ushort, float>* mask) {
	NiShape* shape = FindShapeByName(shapeName);
	if (!shape)
		return;

	Vector3 root;
	GetRootTranslation(root);

	if (shape->blockType == NITRISHAPE || shape->blockType == NITRISTRIPS) {
		auto geomData = hdr.GetBlock<NiGeometryData>(shape->GetDataRef());
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
	else if (shape->blockType == BSSUBINDEXTRISHAPE || shape->blockType == BSTRISHAPE || shape->blockType == BSMESHLODTRISHAPE) {
		auto bsTriShape = dynamic_cast<BSTriShape*>(shape);
		if (!bsTriShape)
			return;

		unordered_map<ushort, Vector3> diff;
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

void NifFile::RotateShape(const string& shapeName, const Vector3& angle, unordered_map<ushort, float>* mask) {
	NiShape* shape = FindShapeByName(shapeName);
	if (!shape)
		return;

	Vector3 root;
	GetRootTranslation(root);

	if (shape->blockType == NITRISHAPE || shape->blockType == NITRISTRIPS) {
		auto geomData = hdr.GetBlock<NiGeometryData>(shape->GetDataRef());
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
	else if (shape->blockType == BSSUBINDEXTRISHAPE || shape->blockType == BSTRISHAPE || shape->blockType == BSMESHLODTRISHAPE) {
		auto bsTriShape = dynamic_cast<BSTriShape*>(shape);
		if (!bsTriShape)
			return;

		unordered_map<ushort, Vector3> diff;
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

bool NifFile::GetAlphaForShape(const string& shapeName, ushort& outFlags, byte& outThreshold) {
	NiShape* shape = FindShapeByName(shapeName);
	if (!shape)
		return false;

	int alphaRef = shape->GetAlphaPropertyRef();
	if (alphaRef == 0xFFFFFFFF) {
		for (int i = 0; i < shape->numProperties; i++) {
			auto alphaProp = hdr.GetBlock<NiAlphaProperty>(shape->propertiesRef[i]);
			if (alphaProp) {
				alphaRef = shape->propertiesRef[i];
				break;
			}
		}
	}

	auto alpha = hdr.GetBlock<NiAlphaProperty>(alphaRef);
	if (!alpha)
		return false;

	outFlags = alpha->flags;
	outThreshold = alpha->threshold;
	return true;
}

void NifFile::SetAlphaForShape(const string& shapeName, ushort flags, ushort threshold) {
	NiShape* shape = FindShapeByName(shapeName);
	if (!shape)
		return;

	int alphaRef = shape->GetAlphaPropertyRef();
	if (alphaRef == 0xFFFFFFFF) {
		for (int i = 0; i < shape->numProperties; i++) {
			auto alphaProp = hdr.GetBlock<NiAlphaProperty>(shape->propertiesRef[i]);
			if (alphaProp) {
				alphaRef = shape->propertiesRef[i];
				break;
			}
		}
	}

	if (alphaRef == 0xFFFFFFFF) {
		NiShader* shader = GetShader(shapeName);
		if (!shader)
			return;

		auto alphaProp = new NiAlphaProperty(hdr);
		alphaRef = hdr.AddBlock(alphaProp, "NiAlphaProperty");

		if (shader->blockType == BSLIGHTINGSHADERPROPERTY || shader->blockType == BSEFFECTSHADERPROPERTY)
			shape->SetAlphaPropertyRef(alphaRef);
		else if (shader->blockType == BSSHADERPPLIGHTINGPROPERTY)
			shape->propertiesRef.push_back(alphaRef);
		else
			return;
	}

	auto alpha = hdr.GetBlock<NiAlphaProperty>(alphaRef);
	if (!alpha)
		return;

	alpha->flags = flags;
	alpha->threshold = (byte)threshold;
}

bool NifFile::IsShapeSkinned(const string& shapeName) {
	NiShape* shape = FindShapeByName(shapeName);
	if (shape)
		return shape->IsSkinned();

	return false;
}

void NifFile::DeleteShape(const string& shapeName) {
	NiShape* shape = FindShapeByName(shapeName);
	if (!shape)
		return;

	hdr.DeleteBlock(shape->GetDataRef());
	DeleteShader(shapeName);
	DeleteSkinning(shapeName);

	for (int i = 0; i < shape->numProperties; i++) {
		hdr.DeleteBlock(shape->propertiesRef[i]);
		i--;
	}

	for (int i = 0; i < shape->GetNumExtraData(); i++) {
		hdr.DeleteBlock(shape->GetExtraDataRef(i));
		i--;
	}

	int shapeID = GetBlockID(shape);
	hdr.DeleteBlock(shapeID);
}

void NifFile::DeleteShader(const string& shapeName) {
	NiShape* shape = FindShapeByName(shapeName);
	if (!shape)
		return;

	if (shape->GetShaderPropertyRef() != 0xFFFFFFFF) {
		auto shader = hdr.GetBlock<NiShader>(shape->GetShaderPropertyRef());
		if (shader) {
			hdr.DeleteBlock(shader->GetTextureSetRef());
			hdr.DeleteBlock(shader->GetControllerRef());
			hdr.DeleteBlock(shape->GetShaderPropertyRef());
			shape->SetShaderPropertyRef(0xFFFFFFFF);
		}
	}

	DeleteAlpha(shapeName);

	for (int i = 0; i < shape->numProperties; i++) {
		auto shader = hdr.GetBlock<NiShader>(shape->propertiesRef[i]);
		if (shader) {
			if (shader->blockType == BSSHADERPPLIGHTINGPROPERTY || shader->blockType == NIMATERIALPROPERTY) {
				hdr.DeleteBlock(shader->GetTextureSetRef());
				hdr.DeleteBlock(shader->GetControllerRef());
				hdr.DeleteBlock(shape->propertiesRef[i]);
				shader->SetTextureSetRef(0xFFFFFFFF);
				shape->propertiesRef[i] = 0xFFFFFFFF;
				i--;
				continue;
			}
		}
	}
}

void NifFile::DeleteAlpha(const string& shapeName) {
	NiShape* shape = FindShapeByName(shapeName);
	if (!shape)
		return;

	auto alpha = hdr.GetBlock<NiAlphaProperty>(shape->GetAlphaPropertyRef());
	if (alpha) {
		hdr.DeleteBlock(shape->GetAlphaPropertyRef());
		shape->SetAlphaPropertyRef(0xFFFFFFFF);
	}

	for (int i = 0; i < shape->numProperties; i++) {
		alpha = hdr.GetBlock<NiAlphaProperty>(shape->propertiesRef[i]);
		if (alpha) {
			hdr.DeleteBlock(shape->propertiesRef[i]);
			shape->propertiesRef[i] = 0xFFFFFFFF;
			i--;
			continue;
		}
	}
}

void NifFile::DeleteSkinning(const string& shapeName) {
	NiShape* shape = FindShapeByName(shapeName);
	if (!shape)
		return;

	auto skinInst = hdr.GetBlock<NiSkinInstance>(shape->GetSkinInstanceRef());
	if (skinInst) {
		hdr.DeleteBlock(skinInst->GetDataRef());
		hdr.DeleteBlock(skinInst->GetSkinPartitionRef());
		hdr.DeleteBlock(shape->GetSkinInstanceRef());
		shape->SetSkinInstanceRef(0xFFFFFFFF);
	}

	auto bsSkinInst = hdr.GetBlock<BSSkinInstance>(shape->GetSkinInstanceRef());
	if (bsSkinInst) {
		hdr.DeleteBlock(bsSkinInst->GetDataRef());
		hdr.DeleteBlock(shape->GetSkinInstanceRef());
		shape->SetSkinInstanceRef(0xFFFFFFFF);
	}

	shape->SetSkinned(false);

	NiShader* shader = GetShader(shapeName);
	if (shader)
		shader->SetSkinned(false);
}

void NifFile::DeleteVertsForShape(const string& shapeName, const vector<ushort>& indices) {
	if (indices.size() == 0)
		return;

	NiShape* shape = FindShapeByName(shapeName);
	if (!shape)
		return;

	int skinRef = shape->GetSkinInstanceRef();

	auto geomData = hdr.GetBlock<NiTriBasedGeomData>(shape->GetDataRef());
	if (geomData) {
		geomData->notifyVerticesDelete(indices);
		if (geomData->numVertices == 0 || geomData->numTriangles == 0) {
			// Deleted all verts or tris, remove shape and children
			DeleteShape(shapeName);
			return;
		}
	}

	auto bsTriShape = dynamic_cast<BSTriShape*>(shape);
	if (bsTriShape) {
		bsTriShape->notifyVerticesDelete(indices);
		if (bsTriShape->numVertices == 0 || bsTriShape->numTriangles == 0) {
			// Deleted all verts or tris, remove shape and children
			DeleteShape(shapeName);
			return;
		}
	}

	auto skinInst = hdr.GetBlock<NiSkinInstance>(skinRef);
	if (skinInst) {
		auto skinData = hdr.GetBlock<NiSkinData>(skinInst->GetDataRef());
		if (skinData)
			skinData->notifyVerticesDelete(indices);

		auto skinPartition = hdr.GetBlock<NiSkinPartition>(skinInst->GetSkinPartitionRef());
		if (skinPartition) {
			skinPartition->notifyVerticesDelete(indices);
			vector<int> emptyIndices;
			if (skinPartition->RemoveEmptyPartitions(emptyIndices)) {
				if (skinInst->blockType == BSDISMEMBERSKININSTANCE) {
					auto bsdSkinInst = static_cast<BSDismemberSkinInstance*>(skinInst);
					for (int i = emptyIndices.size() - 1; i >= 0; i--)
						bsdSkinInst->RemovePartition(i);

					UpdatePartitionFlags(shapeName);
				}
			}
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

		if (v.IsZero(true))
			continue;

		outDiffData[i] = v;
	}

	return 0;
}

void NifFile::UpdateSkinPartitions(const string& shapeName) {
	NiShape* shape = FindShapeByName(shapeName);
	if (!shape)
		return;

	NiSkinData* skinData = nullptr;
	NiSkinPartition* skinPart = nullptr;
	auto skinInst = hdr.GetBlock<NiSkinInstance>(shape->GetSkinInstanceRef());
	if (skinInst) {
		skinData = hdr.GetBlock<NiSkinData>(skinInst->GetDataRef());
		skinPart = hdr.GetBlock<NiSkinPartition>(skinInst->GetSkinPartitionRef());

		if (!skinData || !skinPart)
			return;
	}
	else
		return;

	auto bsdSkinInst = dynamic_cast<BSDismemberSkinInstance*>(skinInst);

	vector<Triangle> tris;
	ushort numTriangles;
	ushort numVerts;
	if (shape->blockType == NITRISHAPE) {
		auto shapeData = hdr.GetBlock<NiTriShapeData>(shape->GetDataRef());
		if (!shapeData)
			return;

		tris = shapeData->triangles;
		numTriangles = shapeData->numTriangles;
		numVerts = shapeData->numVertices;
	}
	else if (shape->blockType == NITRISTRIPS) {
		auto stripsData = hdr.GetBlock<NiTriStripsData>(shape->GetDataRef());
		if (!stripsData)
			return;

		stripsData->StripsToTris(&tris);
		numTriangles = stripsData->numTriangles;
		numVerts = stripsData->numVertices;
	}
	else
		return;

	// Align triangles for comparisons
	for (auto &t : tris)
		t.rot();

	// Make maps of vertices to bones and weights
	unordered_map<ushort, vector<int>> vertBones;
	unordered_map<ushort, vector<float>> vertWeights;

	int boneIndex = 0;
	for (auto &bone : skinData->bones) {
		for (auto &bw : bone.vertexWeights) {
			int weightIndex = 0;
			for (auto &w : vertWeights[bw.index]) {
				if (bw.weight > w)
					break;
				weightIndex++;
			}
			vertBones[bw.index].insert(vertBones[bw.index].begin() + weightIndex, boneIndex);
			vertWeights[bw.index].insert(vertWeights[bw.index].begin() + weightIndex, bw.weight);
		}
		boneIndex++;
	}

	// Enforce maximum vertex bone weight count
	int maxBonesPerVertex = 4;

	for (auto &vb : vertBones)
		if (vb.second.size() > maxBonesPerVertex)
			vb.second.erase(vb.second.begin() + maxBonesPerVertex);

	for (auto &vw : vertWeights)
		if (vw.second.size() > maxBonesPerVertex)
			vw.second.erase(vw.second.begin() + maxBonesPerVertex);

	unordered_map<int, vector<int>> vertTris;
	for (int t = 0; t < tris.size(); t++) {
		vertTris[tris[t].p1].push_back(t);
		vertTris[tris[t].p2].push_back(t);
		vertTris[tris[t].p3].push_back(t);
	}

	// Lambda for finding bones that have the tri assigned
	set<int> triBones;
	auto fTriBones = [&triBones, &tris, &vertBones](const int& tri) {
		triBones.clear();

		if (tri >= 0 && tris.size() > tri) {
			ushort* p = &tris[tri].p1;
			for (int i = 0; i < 3; i++, p++)
				for (auto &tb : vertBones[*p])
					triBones.insert(tb);
		}
	};

	unordered_map<int, int> triParts;
	triParts.reserve(numTriangles);
	for (int i = 0; i < numTriangles; i++)
		triParts.emplace(i, -1);

	vector<bool> usedTris;
	usedTris.resize(numTriangles);

	vector<bool> usedVerts;
	usedVerts.resize(numVerts);

	// 18 for pre-SK
	int maxBonesPerPartition = hdr.GetUserVersion() >= 12 ? numeric_limits<int>::max() : 18;
	unordered_map<int, set<int>> partBones;

	vector<NiSkinPartition::PartitionBlock> partitions;
	for (int partID = 0; partID < skinPart->partitions.size(); partID++) {
		fill(usedVerts.begin(), usedVerts.end(), false);

		ushort numTrisInPart = 0;
		for (int it = 0; it < skinPart->partitions[partID].numTriangles;) {
			// Find the actual tri index from the partition tri index
			Triangle tri;
			tri.p1 = skinPart->partitions[partID].vertexMap[skinPart->partitions[partID].triangles[it].p1];
			tri.p2 = skinPart->partitions[partID].vertexMap[skinPart->partitions[partID].triangles[it].p2];
			tri.p3 = skinPart->partitions[partID].vertexMap[skinPart->partitions[partID].triangles[it].p3];
			tri.rot();

			// Find current tri in full list
			auto realTri = find(tris.begin(), tris.end(), tri);
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

			queue<int> vertQueue;
			auto fSelectVerts = [&usedVerts, &tris, &vertQueue](const int& tri) {
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
					int newBoneCount = 0;
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

		unordered_map<int, int> vertMap;
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
			part.triangles.push_back(tri);
		}

		part.numTriangles = part.triangles.size();

		unordered_map<int, int> boneLookup;
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
			for (int bi = 0; bi < vertBones[v].size(); bi++) {
				if (bi == 4)
					break;

				pb[bi] = boneLookup[vertBones[v][bi]];
				pw[bi] = vertWeights[v][bi];
				tot += pw[bi];
			}

			for (int bi = 0; bi < 4; bi++)
				pw[bi] /= tot;

			part.boneIndices.push_back(b);
			part.vertexWeights.push_back(vw);
		}

		partitions.push_back(move(part));
	}

	skinPart->numPartitions = partitions.size();
	skinPart->partitions = move(partitions);

	UpdatePartitionFlags(shapeName);
}

void NifFile::UpdatePartitionFlags(const string& shapeName) {
	NiShape* shape = FindShapeByName(shapeName);
	if (!shape)
		return;

	auto bsdSkinInst = hdr.GetBlock<BSDismemberSkinInstance>(shape->GetSkinInstanceRef());
	if (!bsdSkinInst)
		return;

	auto skinPart = hdr.GetBlock<NiSkinPartition>(bsdSkinInst->GetSkinPartitionRef());
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

void NifFile::CreateSkinning(const string& shapeName) {
	NiShape* shape = FindShapeByName(shapeName);
	if (!shape)
		return;

	if (shape->blockType == NITRISHAPE) {
		if (shape->GetSkinInstanceRef() == 0xFFFFFFFF) {
			auto nifSkinData = new NiSkinData(hdr);
			int skinDataID = hdr.AddBlock(nifSkinData, "NiSkinData");

			auto nifSkinPartition = new NiSkinPartition(hdr);
			int partID = hdr.AddBlock(nifSkinPartition, "NiSkinPartition");

			auto nifDismemberInst = new BSDismemberSkinInstance(hdr);
			int dismemberID = hdr.AddBlock(nifDismemberInst, "BSDismemberSkinInstance");

			nifDismemberInst->SetDataRef(skinDataID);
			nifDismemberInst->SetSkinPartitionRef(partID);
			nifDismemberInst->SetSkeletonRootRef(0);
			shape->SetSkinInstanceRef(dismemberID);
			shape->SetSkinned(true);

			NiShader* shader = GetShader(shapeName);
			if (shader)
				shader->SetSkinned(true);
		}
	}
	else if (shape->blockType == NITRISTRIPS) {
		// TO-DO
		return;

		if (shape->GetSkinInstanceRef() == 0xFFFFFFFF) {
			auto nifSkinData = new NiSkinData(hdr);
			int skinDataID = hdr.AddBlock(nifSkinData, "NiSkinData");

			auto nifSkinPartition = new NiSkinPartition(hdr);
			int partID = hdr.AddBlock(nifSkinPartition, "NiSkinPartition");

			auto nifDismemberInst = new BSDismemberSkinInstance(hdr);
			int skinID = hdr.AddBlock(nifDismemberInst, "BSDismemberSkinInstance");

			nifDismemberInst->SetDataRef(skinDataID);
			nifDismemberInst->SetSkinPartitionRef(partID);
			nifDismemberInst->SetSkeletonRootRef(0);
			shape->SetSkinInstanceRef(skinID);
			shape->SetSkinned(true);
		}
	}
	else if (shape->blockType == BSSUBINDEXTRISHAPE || shape->blockType == BSTRISHAPE || shape->blockType == BSMESHLODTRISHAPE) {
		if (shape->GetSkinInstanceRef() == 0xFFFFFFFF) {
			auto newSkinInst = new BSSkinInstance(hdr);
			int skinInstID = hdr.AddBlock(newSkinInst, "BSSkin::Instance");

			auto newBoneData = new BSSkinBoneData(hdr);
			int boneDataRef = hdr.AddBlock(newBoneData, "BSSkin::BoneData");

			newSkinInst->SetTargetRef(GetRootNodeID());
			newSkinInst->SetDataRef(boneDataRef);
			shape->SetSkinInstanceRef(skinInstID);
			shape->SetSkinned(true);

			NiShader* shader = GetShader(shapeName);
			if (shader)
				shader->SetSkinned(true);
		}
	}
}

void NifFile::UpdateBoundingSphere(const string& shapeName) {
	NiShape* shape = FindShapeByName(shapeName);
	if (!shape)
		return;

	shape->UpdateBounds();
}
