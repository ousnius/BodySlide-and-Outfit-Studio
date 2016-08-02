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

NiShape* NifFile::shapeForName(const string& name, int dupIndex) {
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

NiTriBasedGeom* NifFile::geomForName(const string& name, int dupIndex) {
	int numFound = 0;
	for (auto& block : blocks) {
		BlockType type = block->blockType;
		if (type == NITRISHAPE || type == NITRISTRIPS) {
			auto geom = dynamic_cast<NiTriBasedGeom*>(block);
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
		BlockType type = block->blockType;
		if (type == BSSUBINDEXTRISHAPE || type == BSTRISHAPE || type == BSMESHLODTRISHAPE) {
			auto geom = dynamic_cast<BSTriShape*>(block);
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

int NifFile::shapeDataIdForName(const string& name, BlockType& outBlockType) {
	int i = 0;
	for (auto& block : blocks) {
		BlockType type = block->blockType;
		if (type == NITRISHAPE || type == NITRISTRIPS) {
			auto geom = dynamic_cast<NiTriBasedGeom*>(block);
			if (geom && !name.compare(geom->GetName())) {
				auto geomData = hdr.GetBlock<NiTriBasedGeomData>(geom->GetDataRef());
				outBlockType = geomData->blockType;
				return geom->GetDataRef();
			}
		}
		else if (type == BSSUBINDEXTRISHAPE || type == BSTRISHAPE || type == BSMESHLODTRISHAPE) {
			auto geom = dynamic_cast<BSTriShape*>(block);
			outBlockType = type;
			if (geom && !name.compare(geom->GetName()))
				return i;
		}
		i++;
	}
	return 0xFFFFFFFF;
}

int NifFile::shapeIdForName(const string& name) {
	int id = 0;
	for (auto& block : blocks) {
		BlockType type = block->blockType;
		if (type == NITRISHAPE || type == NITRISTRIPS || type == BSSUBINDEXTRISHAPE || type == BSTRISHAPE || type == BSMESHLODTRISHAPE) {
			auto geom = dynamic_cast<NiShape*>(block);
			if (geom && !name.compare(geom->GetName()))
				return id;
		}
		id++;
	}
	return 0xFFFFFFFF;
}

int NifFile::shapeBoneIndex(const string& shapeName, const string& boneName) {
	NiShape* shape = shapeForName(shapeName);
	if (shape) {
		auto boneCont = hdr.GetBlock<NiBoneContainer>(shape->GetSkinInstanceRef());
		if (boneCont) {
			for (int i = 0; i < boneCont->numBones; i++) {
				auto node = hdr.GetBlock<NiNode>(boneCont->bones[i]);
				if (node && node->GetName() == boneName)
					return i;
			}
		}
	}

	return 0xFFFFFFFF;
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
	vector<int> delta;
	bool hadoffset = false;

	// Have to do this in multiple passes
	do {
		vector<string> oldOrder;
		GetShapeList(oldOrder);

		vector<int> oldOrderIds;
		for (auto s : oldOrder)
			oldOrderIds.push_back(shapeIdForName(s));

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

int NifFile::AddNode(const string& nodeName, vector<Vector3>& rot, Vector3& trans, float scale) {
	auto root = dynamic_cast<NiNode*>(blocks[0]);
	if (!root)
		return 0xFFFFFFFF;

	NiNode* newNode = new NiNode(hdr);
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
	hdr.DeleteBlock(GetNodeID(nodeName));
}

string NifFile::NodeName(int blockID) {
	string name;

	auto n = hdr.GetBlock<NiNode>(blockID);
	if (n) {
		name = n->GetName();
		if (name.empty())
			name = "_unnamed_";
	}

	return name;
}

int NifFile::AssignExtraData(const string& blockName, const int& extraDataId, bool isNode) {
	if (extraDataId != 0xFFFFFFFF) {
		if (isNode) {
			// Assign to node
			NiNode* node = nodeForName(blockName);
			if (!node)
				return 0xFFFFFFFF;

			node->AddExtraDataRef(extraDataId);
		}
		else {
			// Assign to geometry
			NiAVObject* geom = avObjectForName(blockName);
			if (!geom)
				return 0xFFFFFFFF;

			geom->AddExtraDataRef(extraDataId);
		}
	}
	return extraDataId;
}

int NifFile::AddStringExtraData(const string& blockName, const string& name, const string& stringData, bool isNode) {
	NiStringExtraData* strExtraData = new NiStringExtraData(hdr);
	strExtraData->SetName(name);
	strExtraData->SetStringData(stringData);

	return AssignExtraData(blockName, hdr.AddBlock(strExtraData, "NiStringExtraData"), isNode);
}

int NifFile::AddIntegerExtraData(const string& blockName, const string& name, const int& integerData, bool isNode) {
	NiIntegerExtraData* intExtraData = new NiIntegerExtraData(hdr);
	intExtraData->SetName(name);
	intExtraData->SetIntegerData(integerData);

	return AssignExtraData(blockName, hdr.AddBlock(intExtraData, "NiIntegerExtraData"), isNode);
}

NiShader* NifFile::GetShader(const string& shapeName) {
	int prop1 = 0xFFFFFFFF;

	NiShape* shape = shapeForName(shapeName);
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
	NiTriBasedGeom* geom = geomForName(shapeName);
	if (!geom)
		return nullptr;

	vector<int> props = geom->propertiesRef;
	if (props.empty())
		return nullptr;

	for (int i = 0; i < props.size(); i++) {
		auto material = hdr.GetBlock<NiMaterialProperty>(props[i]);
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
			BSEffectShaderProperty* effectShader = (BSEffectShaderProperty*)shader;
			if (texIndex == 0)
				outTexFile = effectShader->sourceTexture.GetString();
			else if (texIndex == 1)
				outTexFile = effectShader->greyscaleTexture.GetString();

			return BSEFFECTSHADERPROPERTY;
		}
		else
			return 0;
	}

	BSShaderTextureSet* textureSet = dynamic_cast<BSShaderTextureSet*>(hdr.GetBlock(textureSetRef));
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
			BSEffectShaderProperty* effectShader = (BSEffectShaderProperty*)shader;
			if (texIndex == 0)
				effectShader->sourceTexture.SetString(outTexFile);
			else if (texIndex == 1)
				effectShader->greyscaleTexture.SetString(outTexFile);
			return;
		}
		else
			return;
	}

	BSShaderTextureSet* textureSet = dynamic_cast<BSShaderTextureSet*>(hdr.GetBlock(textureSetRef));
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
	NiShape* shape = shapeForName(shapeDest);
	NiShader* srcShader = nullptr;
	NiAlphaProperty* srcAlphaProp = nullptr;

	int srcShaderRef = 0xFFFFFFFF;
	int propRef1 = 0xFFFFFFFF;
	int propRef2 = 0xFFFFFFFF;

	if (!shape)
		return;

	srcShader = dynamic_cast<NiShader*>(srcNif.hdr.GetBlock(shape->GetShaderPropertyRef()));
	if (!srcShader) {
		// No shader found, look in other properties
		for (int i = 0; i < shape->numProperties; i++) {
			srcShader = dynamic_cast<NiShader*>(srcNif.hdr.GetBlock(shape->propertiesRef[i]));
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

	srcAlphaProp = dynamic_cast<NiAlphaProperty*>(srcNif.hdr.GetBlock(shape->GetAlphaPropertyRef()));
	if (!srcAlphaProp) {
		// No alpha found, look in other properties
		for (int i = 0; i < shape->numProperties; i++) {
			srcAlphaProp = dynamic_cast<NiAlphaProperty*>(srcNif.hdr.GetBlock(shape->propertiesRef[i]));
			if (srcAlphaProp) {
				propRef2 = i;
				break;
			}
		}
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
	if (hdr.GetUserVersion() == 12 && hdr.GetUserVersion2() >= 120)
		destShader->SetName(srcShader->GetName());
	else
		destShader->ClearName();

	// Add shader block to nif
	int shaderId = hdr.AddBlock(destShader, srcNif.hdr.GetBlockTypeStringById(srcShaderRef));

	BSShaderTextureSet* destTexSet = nullptr;
	BSShaderTextureSet* srcTexSet = dynamic_cast<BSShaderTextureSet*>(srcNif.hdr.GetBlock(srcShader->GetTextureSetRef()));
	if (srcTexSet) {
		// Create texture set block and copy
		destTexSet = new BSShaderTextureSet(*srcTexSet);
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
	NiTimeController* destController = nullptr;
	NiTimeController* srcController = dynamic_cast<NiTimeController*>(srcNif.hdr.GetBlock(srcShader->GetControllerRef()));
	if (srcController) {
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
			destShader->SetControllerRef(0xFFFFFFFF);
	}

	if (destController) {
		int controllerId = hdr.AddBlock(destController, srcNif.hdr.GetBlockTypeStringById(srcShader->GetControllerRef()));
		destController->targetRef = shaderId;
		destShader->SetControllerRef(controllerId);
	}

	if (srcShader->blockType == BSLIGHTINGSHADERPROPERTY || srcShader->blockType == BSEFFECTSHADERPROPERTY)
		shape->SetShaderPropertyRef(shaderId);
	else if (srcShader->blockType == BSSHADERPPLIGHTINGPROPERTY)
		shape->propertiesRef[propRef1] = shaderId;

	// Kill normals and tangents
	if (destShader->IsSkinTint() && hdr.GetUserVersion() >= 12) {
		if (shape->blockType == NITRISHAPEDATA || shape->blockType == NITRISTRIPSDATA) {
			shape->SetNormals(false);
			shape->SetTangents(false);
		}
	}

	// Create alpha property and copy
	NiAlphaProperty* destAlphaProp = nullptr;
	if (srcAlphaProp) {
		destAlphaProp = new NiAlphaProperty(*srcAlphaProp);
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
	NiNode* srcNode = srcNif.nodeForName(nodeName);
	if (!srcNode)
		return 0xFFFFFFFF;

	NiNode* destNode = new NiNode(*srcNode);
	destNode->header = &hdr;
	destNode->SetName(nodeName);

	return hdr.AddBlock(destNode, "NiNode");
}

void NifFile::CopyGeometry(const string& shapeDest, NifFile& srcNif, const string& srcShape) {
	NiShape* srcGeom = srcNif.shapeForName(srcShape);
	if (!srcGeom)
		return;

	NiShape* destGeom = nullptr;
	if (srcGeom->blockType == NITRISHAPE) {
		NiTriShape* shape = (NiTriShape*)srcGeom;
		NiTriShape* destShape = new NiTriShape(*shape);
		destGeom = dynamic_cast<NiShape*>(destShape);
	}
	else if (srcGeom->blockType == NITRISTRIPS) {
		NiTriStrips* strips = (NiTriStrips*)srcGeom;
		NiTriStrips* destStrips = new NiTriStrips(*strips);
		destGeom = dynamic_cast<NiShape*>(destStrips);
	}
	if (srcGeom->blockType == BSSUBINDEXTRISHAPE) {
		BSSubIndexTriShape* shape = static_cast<BSSubIndexTriShape*>(srcGeom);
		BSSubIndexTriShape* destShape = new BSSubIndexTriShape(*shape);
		destGeom = static_cast<NiShape*>(destShape);
	}
	else if (srcGeom->blockType == BSMESHLODTRISHAPE) {
		BSMeshLODTriShape* shape = static_cast<BSMeshLODTriShape*>(srcGeom);
		BSMeshLODTriShape* destShape = new BSMeshLODTriShape(*shape);
		destGeom = static_cast<NiShape*>(destShape);
	}
	else if (srcGeom->blockType == BSTRISHAPE) {
		BSTriShape* shape = static_cast<BSTriShape*>(srcGeom);
		BSTriShape* destShape = new BSTriShape(*shape);
		destGeom = static_cast<NiShape*>(destShape);
	}

	if (!destGeom)
		return;

	destGeom->header = &hdr;
	destGeom->SetName(shapeDest);

	int destId = hdr.AddBlock(destGeom, srcNif.hdr.GetBlockTypeStringById(srcNif.shapeIdForName(srcShape)));

	NiTriBasedGeomData* srcGeomData = dynamic_cast<NiTriBasedGeomData*>(srcNif.hdr.GetBlock(srcGeom->GetDataRef()));
	if (srcGeomData) {
		NiTriBasedGeomData* destGeomData = nullptr;
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

		if (destGeomData) {
			destGeomData->header = &hdr;

			int destDataId = hdr.AddBlock(destGeomData, srcNif.hdr.GetBlockTypeStringById(srcGeom->GetDataRef()));
			destGeom->SetDataRef(destDataId);
		}
	}

	NiBoneContainer* destBoneCont = nullptr;
	if (srcGeom->GetSkinInstanceRef() != 0xFFFFFFFF) {
		if (destGeom->blockType == NITRISHAPEDATA || destGeom->blockType == NITRISTRIPSDATA) {
			NiSkinInstance* srcSkinInst = dynamic_cast<NiSkinInstance*>(srcNif.hdr.GetBlock(srcGeom->GetSkinInstanceRef()));
			if (srcSkinInst) {
				NiSkinData* srcSkinData = dynamic_cast<NiSkinData*>(srcNif.hdr.GetBlock(srcSkinInst->GetDataRef()));
				NiSkinPartition* srcSkinPart = dynamic_cast<NiSkinPartition*>(srcNif.hdr.GetBlock(srcSkinInst->GetSkinPartitionRef()));

				NiSkinInstance* destSkinInst = nullptr;
				if (srcSkinInst->blockType == NISKININSTANCE) {
					destSkinInst = new NiSkinInstance(*srcSkinInst);
				}
				else if (srcSkinInst->blockType == BSDISMEMBERSKININSTANCE) {
					BSDismemberSkinInstance* srcBsdSkinInst = static_cast<BSDismemberSkinInstance*>(srcSkinInst);
					BSDismemberSkinInstance* destBsdSkinInst = new BSDismemberSkinInstance(*srcBsdSkinInst);
					destSkinInst = dynamic_cast<NiSkinInstance*>(destBsdSkinInst);
				}

				if (destSkinInst) {
					destSkinInst->header = &hdr;

					// Treat skinning and partition info as blobs of anonymous data.
					NiSkinData* destSkinData = new NiSkinData(*srcSkinData);
					destSkinData->header = &hdr;
					NiSkinPartition* destSkinPart = new NiSkinPartition(*srcSkinPart);
					destSkinPart->header = &hdr;

					int destSkinId = hdr.AddBlock(destSkinInst, srcNif.hdr.GetBlockTypeStringById(srcGeom->GetSkinInstanceRef()));
					int destSkinDataId = hdr.AddBlock(destSkinData, srcNif.hdr.GetBlockTypeStringById(srcSkinInst->GetDataRef()));
					int destSkinPartId = hdr.AddBlock(destSkinPart, srcNif.hdr.GetBlockTypeStringById(srcSkinInst->GetSkinPartitionRef()));

					destGeom->SetSkinInstanceRef(destSkinId);
					destSkinInst->SetDataRef(destSkinDataId);
					destSkinInst->SetSkinPartitionRef(destSkinPartId);

					destBoneCont = dynamic_cast<NiBoneContainer*>(destSkinInst);
				}
			}
		}
		else if (destGeom->blockType == BSTRISHAPE || destGeom->blockType == BSSUBINDEXTRISHAPE || destGeom->blockType == BSMESHLODTRISHAPE) {
			BSSkinInstance* srcBSSkinInst = dynamic_cast<BSSkinInstance*>(srcNif.hdr.GetBlock(srcGeom->GetSkinInstanceRef()));
			if (srcBSSkinInst) {
				BSSkinInstance* destBSSkinInst = new BSSkinInstance(*srcBSSkinInst);

				int destSkinInstId = hdr.AddBlock(destBSSkinInst, srcNif.hdr.GetBlockTypeStringById(srcGeom->GetSkinInstanceRef()));
				destGeom->SetSkinInstanceRef(destSkinInstId);

				BSSkinBoneData* srcBoneData = dynamic_cast<BSSkinBoneData*>(srcNif.hdr.GetBlock(srcBSSkinInst->GetDataRef()));
				if (srcBoneData) {
					BSSkinBoneData* destBoneData = new BSSkinBoneData(*srcBoneData);

					int destBoneDataId = hdr.AddBlock(destBoneData, srcNif.hdr.GetBlockTypeStringById(srcBSSkinInst->GetDataRef()));
					destBSSkinInst->SetDataRef(destBoneDataId);

					destBoneCont = dynamic_cast<NiBoneContainer*>(destBSSkinInst);
				}
			}
		}
	}

	CopyShader(shapeDest, srcNif);

	for (int i = 0; i < srcGeom->numProperties; i++) {
		NiMaterialProperty* material = dynamic_cast<NiMaterialProperty*>(srcNif.hdr.GetBlock(srcGeom->propertiesRef[i]));
		if (material) {
			NiMaterialProperty* destMaterial = new NiMaterialProperty(*material);
			destMaterial->header = &hdr;

			int materialId = hdr.AddBlock(destMaterial, srcNif.hdr.GetBlockTypeStringById(srcGeom->propertiesRef[i]));
			destGeom->propertiesRef[i] = materialId;
			continue;
		}

		NiStencilProperty* stencil = dynamic_cast<NiStencilProperty*>(srcNif.hdr.GetBlock(srcGeom->propertiesRef[i]));
		if (stencil) {
			NiStencilProperty* destStencil = new NiStencilProperty(*stencil);
			destStencil->header = &hdr;

			int stencilId = hdr.AddBlock(destStencil, srcNif.hdr.GetBlockTypeStringById(srcGeom->propertiesRef[i]));
			destGeom->propertiesRef[i] = stencilId;
			continue;
		}

		NiUnknown* srcUnknown = dynamic_cast<NiUnknown*>(srcNif.hdr.GetBlock(srcGeom->propertiesRef[i]));
		if (srcUnknown) {
			NiUnknown* destUnknown = new NiUnknown(srcUnknown->CalcBlockSize());
			destUnknown->Clone(srcUnknown);

			int unknownId = hdr.AddBlock(destUnknown, srcNif.hdr.GetBlockTypeStringById(srcGeom->propertiesRef[i]));
			destGeom->propertiesRef[i] = unknownId;
		}
	}

	vector<string> srcBoneList;
	srcNif.GetShapeBoneList(srcShape, srcBoneList);
	if (destBoneCont)
		destBoneCont->bones.clear();

	NiNode* rootNode = dynamic_cast<NiNode*>(blocks[0]);
	if (rootNode) {
		for (auto &boneName : srcBoneList) {
			int bonePtr = GetNodeID(boneName);
			if (bonePtr == 0xFFFFFFFF) {
				bonePtr = CopyNamedNode(boneName, srcNif);
				rootNode->AddChildRef(bonePtr);
			}

			if (destBoneCont)
				destBoneCont->bones.push_back(bonePtr);
		}

		rootNode->AddChildRef(destId);
	}
}

int NifFile::Save(const string& filename) {
	fstream file(filename.c_str(), ios_base::out | ios_base::binary);
	if (file.is_open()) {
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


BlockType NifFile::GetShapeType(const string& shapeName) {
	BlockType type;
	shapeDataIdForName(shapeName, type);
	return type;
}

int NifFile::GetShapeList(vector<string>& outList) {
	outList.clear();
	for (auto& block : blocks) {
		BlockType type = block->blockType;
		if (type == NITRISHAPE || type == NITRISTRIPS || type == BSSUBINDEXTRISHAPE || type == BSTRISHAPE || type == BSMESHLODTRISHAPE) {
			NiShape* shape = dynamic_cast<NiShape*>(block);
			if (shape)
				outList.push_back(shape->GetName());
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
			while (hdr.FindStringId(geom->GetName() + buf) != 0xFFFFFFFF) {
				dupCount++;
				_snprintf(buf, 10, "_%d", dupCount);
			}

			geom->SetName(geom->GetName() + buf);
			dupCount++;
		}
	}
}

void NifFile::SetNodeName(int blockID, const string& newName) {
	NiNode* node = dynamic_cast<NiNode*>(hdr.GetBlock(blockID));
	if (!node)
		return;

	node->SetName(newName, true);
}

int NifFile::GetNodeID(const string& nodeName) {
	int id = 0xFFFFFFFF;
	for (auto& block : blocks) {
		id++;
		if (block->blockType == NINODE) {
			NiNode* node = (NiNode*)block;
			if (!node->GetName().compare(nodeName))
				return id;
		}
	}
	return 0xFFFFFFFF;
}

int NifFile::GetRootNodeID() {
	if (blocks.empty())
		return 0xFFFFFFFF;

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
			for (int i = 0; i < root->GetNumChildren(); i++) {
				NiNode* node = dynamic_cast<NiNode*>(hdr.GetBlock(root->GetChildRef(i)));
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
	outList.clear();

	NiShape* shape = shapeForName(shapeName);
	if (!shape)
		return 0;

	NiBoneContainer* skinInst = dynamic_cast<NiBoneContainer*>(hdr.GetBlock(shape->GetSkinInstanceRef()));
	if (!skinInst)
		return 0;

	for (int i = 0; i < skinInst->bones.size(); i++) {
		NiNode* node = dynamic_cast<NiNode*>(hdr.GetBlock(skinInst->bones[i]));
		if (node)
			outList.push_back(node->GetName());
	}

	return outList.size();
}

int NifFile::GetShapeBoneIDList(const string& shapeName, vector<int>& outList) {
	outList.clear();

	NiShape* shape = shapeForName(shapeName);
	if (!shape)
		return 0;

	NiBoneContainer* skinInst = dynamic_cast<NiBoneContainer*>(hdr.GetBlock(shape->GetSkinInstanceRef()));
	if (!skinInst)
		return 0;

	for (int i = 0; i < skinInst->bones.size(); i++)
		outList.push_back(skinInst->bones[i]);

	return outList.size();
}

void NifFile::SetShapeBoneIDList(const string& shapeName, vector<int>& inList) {
	NiShape* shape = shapeForName(shapeName);
	if (!shape)
		return;

	if (shape->GetSkinInstanceRef() == 0xFFFFFFFF)
		return;

	BSSkinBoneData* boneData = nullptr;
	if (shape->blockType == BSTRISHAPE || shape->blockType == BSSUBINDEXTRISHAPE || shape->blockType == BSMESHLODTRISHAPE) {
		BSSkinInstance* skinForBoneRef = dynamic_cast<BSSkinInstance*>(hdr.GetBlock(shape->GetSkinInstanceRef()));
		if (skinForBoneRef)
			boneData = dynamic_cast<BSSkinBoneData*>(hdr.GetBlock(skinForBoneRef->GetDataRef()));
	}

	NiBoneContainer* boneCont = dynamic_cast<NiBoneContainer*>(hdr.GetBlock(shape->GetSkinInstanceRef()));
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

	NiSkinInstance* skinInst = dynamic_cast<NiSkinInstance*>(boneCont);
	if (skinInst) {
		NiSkinData* skinData = dynamic_cast<NiSkinData*>(hdr.GetBlock(skinInst->GetDataRef()));
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

	NiShape* shape = shapeForName(shapeName);
	if (!shape)
		return 0;

	BSTriShape* bsTriShape = dynamic_cast<BSTriShape*>(shape);
	if (bsTriShape) {
		for (int vid = 0; vid < bsTriShape->numVertices; vid++) {
			for (int i = 0; i < 4; i++) {
				if (bsTriShape->vertData[vid].weightBones[i] == boneIndex && bsTriShape->vertData[vid].weights[i] != 0) {
					outWeights[vid] = bsTriShape->vertData[vid].weights[i];
				}
			}
		}

		return outWeights.size();
	}

	NiSkinInstance* skinInst = dynamic_cast<NiSkinInstance*>(hdr.GetBlock(shape->GetSkinInstanceRef()));
	if (!skinInst)
		return 0;

	NiSkinData* skinData = dynamic_cast<NiSkinData*>(hdr.GetBlock(skinInst->GetDataRef()));
	if (!skinData || boneIndex > skinData->numBones)
		return 0;

	NiSkinData::BoneData* bone = &skinData->bones[boneIndex];
	for (auto &sw : bone->vertexWeights) {
		outWeights[sw.index] = sw.weight;
		if (sw.weight < 0.0001f)
			outWeights[sw.index] = 0.0f;
	}

	return outWeights.size();
}

bool NifFile::GetShapeBoneTransform(const string& shapeName, const string& boneName, SkinTransform& outXform, BoundingSphere& outBounds) {
	int boneIndex = shapeBoneIndex(shapeName, boneName);
	if (boneName.empty())
		boneIndex = 0xFFFFFFFF;

	return GetShapeBoneTransform(shapeName, boneIndex, outXform, outBounds);
}

bool NifFile::SetShapeBoneTransform(const string& shapeName, int boneIndex, SkinTransform& inXform, BoundingSphere& inBounds) {
	int skinRef = 0xFFFFFFFF;

	NiTriBasedGeom* geom = geomForName(shapeName);
	if (geom) {
		skinRef = geom->GetSkinInstanceRef();
	}
	else {
		BSTriShape* siTriShape = geomForNameF4(shapeName);
		if (siTriShape) {
			skinRef = siTriShape->GetSkinInstanceRef();
			if (skinRef == 0xFFFFFFFF)
				return false;

			BSSkinInstance* skinForBoneRef = dynamic_cast<BSSkinInstance*>(hdr.GetBlock(skinRef));
			if (skinForBoneRef && boneIndex != 0xFFFFFFFF) {
				BSSkinBoneData* bsSkin = dynamic_cast<BSSkinBoneData*>(hdr.GetBlock(skinForBoneRef->GetDataRef()));
				bsSkin->boneXforms[boneIndex].bounds = inBounds;
				bsSkin->boneXforms[boneIndex].boneTransform = inXform;
			}
		}
	}

	if (skinRef == 0xFFFFFFFF)
		return false;

	NiSkinInstance* skinInst = dynamic_cast<NiSkinInstance*>(hdr.GetBlock(skinRef));
	if (!skinInst)
		return false;

	if (skinInst->GetDataRef() == 0xFFFFFFFF)
		return false;

	NiSkinData* skinData = (NiSkinData*)hdr.GetBlock(skinInst->GetDataRef());
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
	int skinRef = 0xFFFFFFFF;

	NiTriBasedGeom* geom = geomForName(shapeName);
	if (geom) {
		skinRef = geom->GetSkinInstanceRef();
	}
	else {
		BSTriShape* siTriShape = geomForNameF4(shapeName);
		if (siTriShape) {
			skinRef = siTriShape->GetSkinInstanceRef();
			if (skinRef != 0xFFFFFFFF) {
				BSSkinInstance* skinForBoneRef = dynamic_cast<BSSkinInstance*>(hdr.GetBlock(skinRef));
				if (skinForBoneRef) {
					int boneDataRef = skinForBoneRef->GetDataRef();
					if (boneDataRef != 0xFFFFFFFF) {
						if (boneIndex == 0xFFFFFFFF) {
							// overall skin transform not found in fo4 meshes :(
							return false;
						}

						BSSkinBoneData* boneData = dynamic_cast<BSSkinBoneData*>(hdr.GetBlock(boneDataRef));
						outXform = boneData->boneXforms[boneIndex].boneTransform;
						outBounds = boneData->boneXforms[boneIndex].bounds;
						return true;
					}
				}
			}
		}
	}

	if (skinRef == 0xFFFFFFFF)
		return false;

	NiSkinInstance* skinInst = dynamic_cast<NiSkinInstance*>(hdr.GetBlock(skinRef));
	if (!skinInst)
		return false;

	if (skinInst->GetDataRef() == 0xFFFFFFFF)
		return false;

	NiSkinData* skinData = (NiSkinData*)hdr.GetBlock(skinInst->GetDataRef());
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
	int skinRef = 0xFFFFFFFF;

	NiTriBasedGeom* geom = geomForName(shapeName);
	if (geom) {
		skinRef = geom->GetSkinInstanceRef();
	}
	else {
		BSTriShape* siTriShape = geomForNameF4(shapeName);
		if (siTriShape)
			skinRef = siTriShape->GetSkinInstanceRef();
	}

	if (skinRef == 0xFFFFFFFF)
		return;

	NiBoneContainer* boneCont = dynamic_cast<NiBoneContainer*>(hdr.GetBlock(skinRef));
	if (!boneCont)
		return;

	for (auto &bp : boneCont->bones) {
		if (bp == oldID) {
			bp = newID;
			return;
		}
	}
}

void NifFile::SetShapeBoneWeights(const string& shapeName, int boneIndex, unordered_map<ushort, float>& inWeights) {
	int skinRef = 0xFFFFFFFF;

	NiTriBasedGeom* geom = geomForName(shapeName);
	if (geom)
		skinRef = geom->GetSkinInstanceRef();
	else {
		// BSTriShape* siTriShape = geomForNameF4(shapeName);
		// NOT Implemented.  use SetShapeVertWeights instead.
	}

	if (skinRef == 0xFFFFFFFF)
		return;

	NiSkinInstance* skinInst = dynamic_cast<NiSkinInstance*>(hdr.GetBlock(skinRef));
	if (!skinInst)
		return;

	if (skinInst->GetDataRef() == 0xFFFFFFFF)
		return;

	NiSkinData* skinData = (NiSkinData*)hdr.GetBlock(skinInst->GetDataRef());
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
	BSTriShape* siTriShape = geomForNameF4(shapeName);
	if (!siTriShape)
		return;

	memset(siTriShape->vertData[vertIndex].weights, 0, sizeof(float) * 4);
	memset(siTriShape->vertData[vertIndex].weightBones, 0, sizeof(byte) * 4);

	// sum weights to normalize weight values
	float sum = 0.0f;
	for (int i = 0; i < weights.size(); i++)
		sum += weights[i];

	int num = (weights.size() < 4 ? weights.size() : 4);

	for (int i = 0; i < num; i++) {
		siTriShape->vertData[vertIndex].weightBones[i] = boneids[i];
		siTriShape->vertData[vertIndex].weights[i] = weights[i] / sum;
	}
}

bool NifFile::GetShapeSegments(const string& shapeName, BSSubIndexTriShape::BSSITSSegmentation& segmentation) {
	BSSubIndexTriShape* siTriShape = dynamic_cast<BSSubIndexTriShape*>(geomForNameF4(shapeName));
	if (!siTriShape)
		return false;

	segmentation = siTriShape->GetSegmentation();
	return true;
}

void NifFile::SetShapeSegments(const string& shapeName, const BSSubIndexTriShape::BSSITSSegmentation& segmentation) {
	BSSubIndexTriShape* siTriShape = dynamic_cast<BSSubIndexTriShape*>(geomForNameF4(shapeName));
	if (!siTriShape)
		return;

	siTriShape->SetSegmentation(segmentation);
}

const vector<Vector3>* NifFile::GetRawVertsForShape(const string& shapeName) {
	BlockType bType;
	int dataRef = shapeDataIdForName(shapeName, bType);
	if (dataRef == 0xFFFFFFFF)
		return nullptr;

	if (bType == NITRISHAPEDATA || bType == NITRISTRIPSDATA) {
		NiGeometryData* geomData = static_cast<NiGeometryData*>(hdr.GetBlock(dataRef));
		return &geomData->vertices;
	}
	else if (bType == BSSUBINDEXTRISHAPE || bType == BSTRISHAPE || bType == BSMESHLODTRISHAPE) {
		BSTriShape* siTriShape = static_cast<BSTriShape*>(hdr.GetBlock(dataRef));
		return siTriShape->GetRawVerts();
	}

	return nullptr;
}

bool NifFile::GetTrisForShape(const string& shapeName, vector<Triangle>* outTris) {
	BlockType bType;
	int dataID = shapeDataIdForName(shapeName, bType);
	if (dataID == 0xFFFFFFFF)
		return false;

	if (bType == NITRISHAPEDATA) {
		NiTriShapeData* shapeData = static_cast<NiTriShapeData*>(hdr.GetBlock(dataID));
		*outTris = shapeData->triangles;
		return true;
	}
	else if (bType == NITRISTRIPSDATA) {
		NiTriStripsData* stripsData = static_cast<NiTriStripsData*>(hdr.GetBlock(dataID));
		stripsData->StripsToTris(outTris);
		return true;
	}
	else if (bType == BSSUBINDEXTRISHAPE || bType == BSTRISHAPE || bType == BSMESHLODTRISHAPE) {
		BSTriShape* shapeData = static_cast<BSTriShape*>(hdr.GetBlock(dataID));
		*outTris = shapeData->triangles;
		return true;
	}

	return false;
}

bool NifFile::ReorderTriangles(const string& shapeName, const vector<ushort>& triangleIndices) {
	BlockType bType;
	int dataID = shapeDataIdForName(shapeName, bType);
	if (dataID == 0xFFFFFFFF)
		return false;

	vector<Triangle> triangles;
	if (bType == NITRISHAPEDATA) {
		NiTriShapeData* shapeData = static_cast<NiTriShapeData*>(hdr.GetBlock(dataID));
		if (triangleIndices.size() != shapeData->numTriangles)
			return false;

		for (auto &id : triangleIndices)
			if (shapeData->triangles.size() >= id)
				triangles.push_back(shapeData->triangles[id]);

		if (triangles.size() != shapeData->numTriangles)
			return false;

		shapeData->triangles = triangles;
	}
	else if (bType == NITRISTRIPSDATA) {
		return false;
	}
	else if (bType == BSSUBINDEXTRISHAPE || bType == BSTRISHAPE || bType == BSMESHLODTRISHAPE) {
		BSTriShape* shapeData = static_cast<BSTriShape*>(hdr.GetBlock(dataID));
		if (triangleIndices.size() != shapeData->numTriangles)
			return false;

		for (auto &id : triangleIndices)
			if (shapeData->triangles.size() >= id)
				triangles.push_back(shapeData->triangles[id]);

		if (triangles.size() != shapeData->numTriangles)
			return false;

		shapeData->triangles = triangles;
	}

	return true;
}

const vector<Vector3>* NifFile::GetNormalsForShape(const string& shapeName, bool transform) {
	BlockType bType;
	int dataID = shapeDataIdForName(shapeName, bType);
	if (dataID == 0xFFFFFFFF)
		return nullptr;

	if (bType == NITRISHAPEDATA || bType == NITRISTRIPSDATA) {
		NiGeometryData* geomData = static_cast<NiGeometryData*>(hdr.GetBlock(dataID));
		if (geomData->HasNormals())
			return &geomData->normals;
	}
	else if (bType == BSSUBINDEXTRISHAPE || bType == BSTRISHAPE || bType == BSMESHLODTRISHAPE) {
		BSTriShape* shapeData = static_cast<BSTriShape*>(hdr.GetBlock(dataID));
		return shapeData->GetNormalData(transform);
	}

	return nullptr;
}

const vector<Vector2>* NifFile::GetUvsForShape(const string& shapeName) {
	BlockType bType;
	int dataID = shapeDataIdForName(shapeName, bType);
	if (dataID == 0xFFFFFFFF)
		return nullptr;

	if (bType == NITRISHAPEDATA || bType == NITRISTRIPSDATA) {
		NiGeometryData* geomData = static_cast<NiGeometryData*>(hdr.GetBlock(dataID));
		if (geomData->numUVSets > 0)
			return &geomData->uvSets;
	}
	else if (bType == BSSUBINDEXTRISHAPE || bType == BSTRISHAPE || bType == BSMESHLODTRISHAPE) {
		BSTriShape* shapeData = static_cast<BSTriShape*>(hdr.GetBlock(dataID));
		return shapeData->GetUVData();
	}

	return nullptr;
}

bool NifFile::GetUvsForShape(const string& shapeName, vector<Vector2>& outUvs) {
	BlockType bType;
	int dataID = shapeDataIdForName(shapeName, bType);
	if (dataID == 0xFFFFFFFF)
		return false;

	const vector<Vector2>* uvData = GetUvsForShape(shapeName);
	if (uvData) {
		outUvs.assign(uvData->begin(), uvData->end());
		return true;
	}

	return false;
}

bool NifFile::GetVertsForShape(const string& shapeName, vector<Vector3>& outVerts) {
	BlockType bType;
	int dataRef = shapeDataIdForName(shapeName, bType);
	if (dataRef == 0xFFFFFFFF) {
		outVerts.clear();
		return false;
	}

	if (bType == NITRISHAPEDATA || bType == NITRISTRIPSDATA) {
		NiGeometryData* geomData = static_cast<NiGeometryData*>(hdr.GetBlock(dataRef));
		outVerts.resize(geomData->numVertices);

		for (int i = 0; i < geomData->numVertices; i++)
			outVerts[i] = geomData->vertices[i];

		return true;
	}
	else if (bType == BSSUBINDEXTRISHAPE || bType == BSTRISHAPE || bType == BSMESHLODTRISHAPE) {
		BSTriShape* siTriShape = static_cast<BSTriShape*>(hdr.GetBlock(dataRef));
		outVerts.resize(siTriShape->numVertices);

		for (int i = 0; i < siTriShape->numVertices; i++)
			outVerts[i] = siTriShape->vertData[i].vert;

		return true;
	}

	outVerts.clear();
	return false;
}

int NifFile::GetVertCountForShape(const string& shapeName) {
	BlockType bType;
	int dataID = shapeDataIdForName(shapeName, bType);
	if (dataID == 0xFFFFFFFF)
		return 0xFFFFFFFF;

	if (bType == NITRISHAPEDATA || bType == NITRISTRIPSDATA) {
		NiGeometryData* geomData = static_cast<NiGeometryData*>(hdr.GetBlock(dataID));
		return geomData->numVertices;
	}
	else if (bType == BSSUBINDEXTRISHAPE || bType == BSTRISHAPE || bType == BSMESHLODTRISHAPE) {
		BSTriShape* siTriShape = static_cast<BSTriShape*>(hdr.GetBlock(dataID));
		return siTriShape->numVertices;
	}

	return 0xFFFFFFFF;
}

void NifFile::SetVertsForShape(const string& shapeName, const vector<Vector3>& verts) {
	BlockType bType;
	int dataID = shapeDataIdForName(shapeName, bType);
	if (dataID == 0xFFFFFFFF)
		return;

	if (bType == NITRISHAPEDATA || bType == NITRISTRIPSDATA) {
		NiGeometryData* geomData = static_cast<NiGeometryData*>(hdr.GetBlock(dataID));
		if (verts.size() != geomData->vertices.size())
			return;

		for (int i = 0; i < geomData->vertices.size(); i++)
			geomData->vertices[i] = verts[i];
	}
	else if (bType == BSSUBINDEXTRISHAPE || bType == BSTRISHAPE || bType == BSMESHLODTRISHAPE) {
		BSTriShape* siTriShape = static_cast<BSTriShape*>(hdr.GetBlock(dataID));
		if (verts.size() != siTriShape->numVertices)
			return;

		for (int i = 0; i < siTriShape->numVertices; i++)
			siTriShape->vertData[i].vert = verts[i];
	}
}

void NifFile::SetUvsForShape(const string& shapeName, const vector<Vector2>& uvs) {
	BlockType bType;
	int dataID = shapeDataIdForName(shapeName, bType);
	if (dataID == 0xFFFFFFFF)
		return;

	if (bType == NITRISHAPEDATA || bType == NITRISTRIPSDATA) {
		NiGeometryData* geomData = static_cast<NiGeometryData*>(hdr.GetBlock(dataID));
		if (uvs.size() != geomData->vertices.size())
			return;

		geomData->uvSets.assign(uvs.begin(), uvs.end());
	}
	else if (bType == BSSUBINDEXTRISHAPE || bType == BSTRISHAPE || bType == BSMESHLODTRISHAPE) {
		BSTriShape* siTriShape = static_cast<BSTriShape*>(hdr.GetBlock(dataID));
		if (uvs.size() != siTriShape->vertData.size())
			return;

		for (int i = 0; i < siTriShape->numVertices; i++)
			siTriShape->vertData[i].uv = uvs[i];
	}
}

void NifFile::InvertUVsForShape(const string& shapeName, bool invertX, bool invertY) {
	BlockType bType;
	int dataID = shapeDataIdForName(shapeName, bType);
	if (dataID == 0xFFFFFFFF)
		return;

	if (bType == NITRISHAPEDATA || bType == NITRISTRIPSDATA) {
		NiGeometryData* geomData = static_cast<NiGeometryData*>(hdr.GetBlock(dataID));
		if (invertX)
			for (int i = 0; i < geomData->uvSets.size(); ++i)
				geomData->uvSets[i].u = 1.0f - geomData->uvSets[i].u;

		if (invertY)
			for (int i = 0; i < geomData->uvSets.size(); ++i)
				geomData->uvSets[i].v = 1.0f - geomData->uvSets[i].v;
	}
	else if (bType == BSSUBINDEXTRISHAPE || bType == BSTRISHAPE || bType == BSMESHLODTRISHAPE) {
		BSTriShape* siTriShape = static_cast<BSTriShape*>(hdr.GetBlock(dataID));
		if (invertX)
			for (int i = 0; i < siTriShape->vertData.size(); ++i)
				siTriShape->vertData[i].uv.u = 1.0f - siTriShape->vertData[i].uv.u;

		if (invertY)
			for (int i = 0; i < siTriShape->vertData.size(); ++i)
				siTriShape->vertData[i].uv.v = 1.0f - siTriShape->vertData[i].uv.v;
	}
}

void NifFile::SetNormalsForShape(const string& shapeName, const vector<Vector3>& norms) {
	BlockType bType;
	int dataID = shapeDataIdForName(shapeName, bType);
	if (dataID == 0xFFFFFFFF)
		return;

	if (bType == NITRISHAPEDATA || bType == NITRISTRIPSDATA) {
		NiGeometryData* geomData = static_cast<NiGeometryData*>(hdr.GetBlock(dataID));
		geomData->SetNormals(true);

		for (int i = 0; i < geomData->vertices.size(); i++)
			geomData->normals[i] = norms[i];
	}
	else if (bType == BSSUBINDEXTRISHAPE || bType == BSTRISHAPE || bType == BSMESHLODTRISHAPE) {
		BSTriShape* shape = static_cast<BSTriShape*>(hdr.GetBlock(dataID));
		shape->SetNormals(norms);
	}
}

void NifFile::CalcNormalsForShape(const string& shapeName, const bool& smooth, const float& smoothThresh) {
	BlockType bType;
	int dataID = shapeDataIdForName(shapeName, bType);
	if (dataID == 0xFFFFFFFF)
		return;

	if (bType == NITRISHAPEDATA || bType == NITRISTRIPSDATA) {
		NiGeometryData* geomData = static_cast<NiGeometryData*>(hdr.GetBlock(dataID));
		geomData->RecalcNormals(smooth, smoothThresh);
	}
	else if (bType == BSSUBINDEXTRISHAPE || bType == BSTRISHAPE || bType == BSMESHLODTRISHAPE) {
		BSTriShape* shape = static_cast<BSTriShape*>(hdr.GetBlock(dataID));
		shape->RecalcNormals(smooth, smoothThresh);
	}
}

void NifFile::CalcTangentsForShape(const string& shapeName) {
	BlockType bType;
	int dataID = shapeDataIdForName(shapeName, bType);
	if (dataID == 0xFFFFFFFF)
		return;

	if (bType == NITRISHAPEDATA || bType == NITRISTRIPSDATA) {
		NiGeometryData* geomData = static_cast<NiGeometryData*>(hdr.GetBlock(dataID));
		geomData->CalcTangentSpace();
	}
	else if (bType == BSSUBINDEXTRISHAPE || bType == BSTRISHAPE || bType == BSMESHLODTRISHAPE) {
		BSTriShape* shape = static_cast<BSTriShape*>(hdr.GetBlock(dataID));
		shape->CalcTangentSpace();
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
	BlockType bType;
	int dataRef = shapeDataIdForName(shapeName, bType);
	if (dataRef == 0xFFFFFFFF)
		return;

	if (bType == NITRISHAPEDATA || bType == NITRISTRIPSDATA) {
		NiTriBasedGeom* geom = geomForName(shapeName);
		if (!geom)
			return;

		NiGeometryData* geomData = static_cast<NiGeometryData*>(hdr.GetBlock(dataRef));
		for (int i = 0; i < geomData->vertices.size(); i++)
			geomData->vertices[i] += geom->translation + offset;

		geom->translation = Vector3(0.0f, 0.0f, 0.0f);
	}
	else if (bType == BSSUBINDEXTRISHAPE || bType == BSTRISHAPE || bType == BSMESHLODTRISHAPE) {
		BSTriShape* geom = static_cast<BSTriShape*>(hdr.GetBlock(dataRef));		
		for (int i = 0; i < geom->numVertices; i++)
			geom->vertData[i].vert += geom->translation + offset;

		geom->translation = Vector3(0.0f, 0.0f, 0.0f);
	}
}

void NifFile::MoveVertex(const string& shapeName, const Vector3& pos, const int& id) {
	BlockType bType;
	int dataRef = shapeDataIdForName(shapeName, bType);
	if (dataRef == 0xFFFFFFFF)
		return;

	if (bType == NITRISHAPEDATA || bType == NITRISTRIPSDATA) {
		NiGeometryData* geomData = static_cast<NiGeometryData*>(hdr.GetBlock(dataRef));
		if (geomData->numVertices > id)
			geomData->vertices[id] = pos;
	}
	else if (bType == BSSUBINDEXTRISHAPE || bType == BSTRISHAPE || bType == BSMESHLODTRISHAPE) {
		BSTriShape* geom = static_cast<BSTriShape*>(hdr.GetBlock(dataRef));
		if (geom->numVertices > id)
			geom->vertData[id].vert = pos;
	}
}

void NifFile::OffsetShape(const string& shapeName, const Vector3& offset, unordered_map<ushort, float>* mask) {
	BlockType bType;
	int dataRef = shapeDataIdForName(shapeName, bType);
	if (dataRef == 0xFFFFFFFF)
		return;

	if (bType == NITRISHAPEDATA || bType == NITRISTRIPSDATA) {
		NiGeometryData* geomData = static_cast<NiGeometryData*>(hdr.GetBlock(dataRef));
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
		BSTriShape* geomData = static_cast<BSTriShape*>(hdr.GetBlock(dataRef));
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
	BlockType bType;
	int dataRef = shapeDataIdForName(shapeName, bType);
	if (dataRef == 0xFFFFFFFF)
		return;

	Vector3 root;
	GetRootTranslation(root);

	if (bType == NITRISHAPEDATA || bType == NITRISTRIPSDATA) {
		NiGeometryData* geomData = static_cast<NiGeometryData*>(hdr.GetBlock(dataRef));

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
		BSTriShape* geomData = static_cast<BSTriShape*>(hdr.GetBlock(dataRef));

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
	BlockType bType;
	int dataRef = shapeDataIdForName(shapeName, bType);
	if (dataRef == 0xFFFFFFFF)
		return;

	Vector3 root;
	GetRootTranslation(root);
	if (bType == NITRISHAPEDATA || bType == NITRISTRIPSDATA) {
		NiGeometryData* geomData = static_cast<NiGeometryData*>(hdr.GetBlock(dataRef));

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
		BSTriShape* geomData = static_cast<BSTriShape*>(hdr.GetBlock(dataRef));

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
	int alphaRef = 0xFFFFFFFF;

	NiTriBasedGeom* geom = geomForName(shapeName);
	if (geom) {
		alphaRef = geom->GetAlphaPropertyRef();
		if (alphaRef == 0xFFFFFFFF) {
			for (int i = 0; i < geom->numProperties; i++) {
				NiAlphaProperty* alphaProp = dynamic_cast<NiAlphaProperty*>(hdr.GetBlock(geom->propertiesRef[i]));
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
			alphaRef = siTriShape->GetAlphaPropertyRef();
	}

	if (alphaRef == 0xFFFFFFFF)
		return false;

	NiAlphaProperty* alpha = dynamic_cast<NiAlphaProperty*>(hdr.GetBlock(alphaRef));
	if (!alpha)
		return false;

	outFlags = alpha->flags;
	outThreshold = alpha->threshold;
	return true;
}

void NifFile::SetAlphaForShape(const string& shapeName, ushort flags, ushort threshold) {
	int alphaRef = 0xFFFFFFFF;

	NiTriBasedGeom* geom = geomForName(shapeName);
	if (geom) {
		alphaRef = geom->GetAlphaPropertyRef();
		if (alphaRef == 0xFFFFFFFF) {
			for (int i = 0; i < geom->numProperties; i++) {
				NiAlphaProperty* alphaProp = dynamic_cast<NiAlphaProperty*>(hdr.GetBlock(geom->propertiesRef[i]));
				if (alphaProp) {
					alphaRef = geom->propertiesRef[i];
					break;
				}
			}
		}

		if (alphaRef == 0xFFFFFFFF) {
			NiShader* shader = GetShader(shapeName);
			if (!shader)
				return;

			NiAlphaProperty* alphaProp = new NiAlphaProperty(hdr);
			alphaRef = hdr.AddBlock(alphaProp, "NiAlphaProperty");

			if (shader->blockType == BSLIGHTINGSHADERPROPERTY || shader->blockType == BSEFFECTSHADERPROPERTY)
				geom->SetAlphaPropertyRef(alphaRef);
			else if (shader->blockType == BSSHADERPPLIGHTINGPROPERTY)
				geom->propertiesRef.push_back(alphaRef);
			else
				return;
		}
	}
	else {
		BSTriShape* siTriShape = geomForNameF4(shapeName);
		if (siTriShape) {
			alphaRef = siTriShape->GetAlphaPropertyRef();
			if (alphaRef == 0xFFFFFFFF) {
				NiShader* shader = GetShader(shapeName);
				if (!shader)
					return;

				NiAlphaProperty* alphaProp = new NiAlphaProperty(hdr);

				alphaRef = hdr.AddBlock(alphaProp, "NiAlphaProperty");
				siTriShape->SetAlphaPropertyRef(alphaRef);
			}
		}
	}

	if (alphaRef == 0xFFFFFFFF)
		return;

	NiAlphaProperty* alpha = dynamic_cast<NiAlphaProperty*>(hdr.GetBlock(alphaRef));
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
		hdr.DeleteBlock(geom->GetDataRef());
		DeleteShader(shapeName);
		DeleteSkinning(shapeName);

		for (int i = 0; i < geom->numProperties; i++) {
			hdr.DeleteBlock(geom->propertiesRef[i]);
			i--;
		}

		for (int i = 0; i < geom->GetNumExtaData(); i++)
			hdr.DeleteBlock(geom->GetExtraDataRef(i));

		int shapeID = shapeIdForName(shapeName);
		hdr.DeleteBlock(shapeID);
	}
	else {
		BSTriShape* siTriShape = geomForNameF4(shapeName);
		if (siTriShape) {
			DeleteShader(shapeName);
			DeleteSkinning(shapeName);

			int shapeID = shapeIdForName(shapeName);
			hdr.DeleteBlock(shapeID);
		}
	}
}

void NifFile::DeleteShader(const string& shapeName) {
	NiTriBasedGeom* geom = geomForName(shapeName);
	if (geom) {
		if (geom->GetShaderPropertyRef() != 0xFFFFFFFF) {
			NiShader* shader = dynamic_cast<NiShader*>(hdr.GetBlock(geom->GetShaderPropertyRef()));
			if (shader) {
				hdr.DeleteBlock(shader->GetTextureSetRef());
				hdr.DeleteBlock(shader->GetControllerRef());
				hdr.DeleteBlock(geom->GetShaderPropertyRef());
				geom->SetShaderPropertyRef(0xFFFFFFFF);
			}
		}

		DeleteAlpha(shapeName);

		for (int i = 0; i < geom->numProperties; i++) {
			NiShader* shader = dynamic_cast<NiShader*>(hdr.GetBlock(geom->propertiesRef[i]));
			if (shader) {
				if (shader->blockType == BSSHADERPPLIGHTINGPROPERTY || shader->blockType == NIMATERIALPROPERTY) {
					hdr.DeleteBlock(shader->GetTextureSetRef());
					hdr.DeleteBlock(shader->GetControllerRef());
					hdr.DeleteBlock(geom->propertiesRef[i]);
					shader->SetTextureSetRef(0xFFFFFFFF);
					geom->propertiesRef[i] = 0xFFFFFFFF;
					i--;
					continue;
				}
			}
		}
	}
	else {
		BSTriShape* siTriShape = geomForNameF4(shapeName);
		if (siTriShape) {
			if (siTriShape->GetShaderPropertyRef() != 0xFFFFFFFF) {
				NiShader* shader = dynamic_cast<NiShader*>(hdr.GetBlock(siTriShape->GetShaderPropertyRef()));
				if (shader) {
					hdr.DeleteBlock(shader->GetTextureSetRef());
					hdr.DeleteBlock(shader->GetControllerRef());
					hdr.DeleteBlock(siTriShape->GetShaderPropertyRef());
					siTriShape->SetShaderPropertyRef(0xFFFFFFFF);
				}

				DeleteAlpha(shapeName);
			}
		}
	}
}

void NifFile::DeleteAlpha(const string& shapeName) {
	NiTriBasedGeom* geom = geomForName(shapeName);
	if (geom) {
		if (geom->GetAlphaPropertyRef() != 0xFFFFFFFF) {
			NiAlphaProperty* alpha = dynamic_cast<NiAlphaProperty*>(hdr.GetBlock(geom->GetAlphaPropertyRef()));
			if (alpha) {
				hdr.DeleteBlock(geom->GetAlphaPropertyRef());
				geom->SetAlphaPropertyRef(0xFFFFFFFF);
			}
		}

		for (int i = 0; i < geom->numProperties; i++) {
			NiAlphaProperty* alpha = dynamic_cast<NiAlphaProperty*>(hdr.GetBlock(geom->propertiesRef[i]));
			if (alpha) {
				hdr.DeleteBlock(geom->propertiesRef[i]);
				geom->propertiesRef[i] = 0xFFFFFFFF;
				i--;
				continue;
			}
		}
	}
	else {
		BSTriShape* siTriShape = geomForNameF4(shapeName);
		if (siTriShape) {
			if (siTriShape->GetAlphaPropertyRef() != 0xFFFFFFFF) {
				NiAlphaProperty* alpha = dynamic_cast<NiAlphaProperty*>(hdr.GetBlock(siTriShape->GetAlphaPropertyRef()));
				if (alpha) {
					hdr.DeleteBlock(siTriShape->GetAlphaPropertyRef());
					siTriShape->SetAlphaPropertyRef(0xFFFFFFFF);
				}
			}
		}
	}
}

void NifFile::DeleteSkinning(const string& shapeName) {
	NiTriBasedGeom* geom = geomForName(shapeName);
	if (geom) {
		if (geom->GetSkinInstanceRef() != 0xFFFFFFFF) {
			NiSkinInstance* skinInst = dynamic_cast<NiSkinInstance*>(hdr.GetBlock(geom->GetSkinInstanceRef()));
			if (skinInst) {
				hdr.DeleteBlock(skinInst->GetDataRef());
				hdr.DeleteBlock(skinInst->GetSkinPartitionRef());
				hdr.DeleteBlock(geom->GetSkinInstanceRef());
				geom->SetSkinInstanceRef(0xFFFFFFFF);
			}
		}
	}
	else {
		BSTriShape* siTriShape = geomForNameF4(shapeName);
		if (siTriShape) {
			if (siTriShape->GetSkinInstanceRef() != 0xFFFFFFFF) {
				BSSkinInstance* bsSkinInst = dynamic_cast<BSSkinInstance*>(hdr.GetBlock(siTriShape->GetSkinInstanceRef()));
				if (bsSkinInst) {
					hdr.DeleteBlock(bsSkinInst->GetDataRef());
					hdr.DeleteBlock(siTriShape->GetSkinInstanceRef());
					siTriShape->SetSkinInstanceRef(0xFFFFFFFF);
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

	int dataRef = 0xFFFFFFFF;
	int skinRef = 0xFFFFFFFF;

	NiTriBasedGeom* geom = geomForName(shapeName);
	if (geom) {
		dataRef = geom->GetDataRef();
		skinRef = geom->GetSkinInstanceRef();

		if (dataRef != 0xFFFFFFFF) {
			NiTriBasedGeomData* geomData = dynamic_cast<NiTriBasedGeomData*>(hdr.GetBlock(dataRef));
			if (geomData) {
				geomData->notifyVerticesDelete(indices);
				if (geomData->numVertices == 0 || geomData->numTriangles == 0) {
					// Deleted all verts or tris, remove shape and children
					DeleteShape(shapeName);
					return;
				}
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
		}
	}

	if (skinRef == 0xFFFFFFFF)
		return;

	NiSkinInstance* skinInst = dynamic_cast<NiSkinInstance*>(hdr.GetBlock(skinRef));
	if (skinInst) {
		NiSkinData* skinData = (NiSkinData*)hdr.GetBlock(skinInst->GetDataRef());
		skinData->notifyVerticesDelete(indices);

		NiSkinPartition* skinPartition = (NiSkinPartition*)hdr.GetBlock(skinInst->GetSkinPartitionRef());
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
	int skinRef = 0xFFFFFFFF;
	NiTriBasedGeom* geom = geomForName(shapeName);
	if (geom)
		skinRef = geom->GetSkinInstanceRef();

	if (skinRef == 0xFFFFFFFF)
		return;

	NiSkinData* skinData = nullptr;
	NiSkinPartition* skinPart = nullptr;
	NiSkinInstance* skinInst = dynamic_cast<NiSkinInstance*>(hdr.GetBlock(skinRef));
	if (skinInst) {
		skinData = (NiSkinData*)hdr.GetBlock(skinInst->GetDataRef());
		skinPart = (NiSkinPartition*)hdr.GetBlock(skinInst->GetSkinPartitionRef());
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
}

void NifFile::BuildSkinPartitions(const string& shapeName, int maxBonesPerPartition) {
	BlockType bType;
	int dataRef = shapeDataIdForName(shapeName, bType);
	if (dataRef == 0xFFFFFFFF)
		return;

	int skinRef = 0xFFFFFFFF;
	NiTriBasedGeom* geom = nullptr;
	NiTriShapeData* shapeData = nullptr;
	NiTriStripsData* stripsData = nullptr;
	BSTriShape* bsGeom = nullptr;

	if (bType == NITRISHAPEDATA) {
		geom = geomForName(shapeName);
		if (!geom)
			return;

		skinRef = geom->GetSkinInstanceRef();
		shapeData = (NiTriShapeData*)hdr.GetBlock(geom->GetDataRef());
		if (skinRef == 0xFFFFFFFF) {
			NiSkinData* nifSkinData = new NiSkinData(hdr);
			int skinDataID = hdr.AddBlock((NiObject*)nifSkinData, "NiSkinData");

			NiSkinPartition* nifSkinPartition = new NiSkinPartition(hdr);
			int partID = hdr.AddBlock((NiObject*)nifSkinPartition, "NiSkinPartition");

			BSDismemberSkinInstance* nifDismemberInst = new BSDismemberSkinInstance(hdr);
			int dismemberID = hdr.AddBlock((NiObject*)nifDismemberInst, "BSDismemberSkinInstance");

			nifDismemberInst->SetDataRef(skinDataID);
			nifDismemberInst->SetSkinPartitionRef(partID);
			nifDismemberInst->SetSkeletonRootRef(0);
			geom->SetSkinInstanceRef(dismemberID);
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

		skinRef = geom->GetSkinInstanceRef();
		stripsData = (NiTriStripsData*)hdr.GetBlock(geom->GetDataRef());
		if (skinRef == 0xFFFFFFFF) {
			NiSkinData* nifSkinData = new NiSkinData(hdr);
			int skinDataID = hdr.AddBlock((NiObject*)nifSkinData, "NiSkinData");

			NiSkinPartition* nifSkinPartition = new NiSkinPartition(hdr);
			int partID = hdr.AddBlock((NiObject*)nifSkinPartition, "NiSkinPartition");

			BSDismemberSkinInstance* nifDismemberInst = new BSDismemberSkinInstance(hdr);
			int skinID = hdr.AddBlock((NiObject*)nifDismemberInst, "BSDismemberSkinInstance");
			
			nifDismemberInst->SetDataRef(skinDataID);
			nifDismemberInst->SetSkinPartitionRef(partID);
			nifDismemberInst->SetSkeletonRootRef(0);
			geom->SetSkinInstanceRef(skinID);
			skinRef = skinID;
		}
	}
	else if (bType == BSSUBINDEXTRISHAPE || bType == BSTRISHAPE || bType == BSMESHLODTRISHAPE) {
		bsGeom = geomForNameF4(shapeName);
		if (!bsGeom)
			return;

		skinRef = bsGeom->GetSkinInstanceRef();
		if (skinRef == 0xFFFFFFFF) {
			BSSkinInstance* newSkinInst = new BSSkinInstance(hdr);
			newSkinInst->SetTargetRef(GetRootNodeID());
			skinRef = hdr.AddBlock(newSkinInst, "BSSkin::Instance");

			BSSkinBoneData* newBoneData = new BSSkinBoneData(hdr);
			int boneDataRef = hdr.AddBlock(newBoneData, "BSSkin::BoneData");

			newSkinInst->SetDataRef(boneDataRef);
			bsGeom->SetSkinInstanceRef(skinRef);
		}

		bsGeom->SetSkinned(true);

		NiShader* shader = GetShader(shapeName);
		if (shader)
			shader->SetSkinned(true);

		return;
	}

	NiSkinData* skinData = nullptr;
	NiSkinPartition* skinPart = nullptr;
	NiSkinInstance* skinInst = dynamic_cast<NiSkinInstance*>(hdr.GetBlock(skinRef));
	if (skinInst) {
		skinData = (NiSkinData*)hdr.GetBlock(skinInst->GetDataRef());
		skinPart = (NiSkinPartition*)hdr.GetBlock(skinInst->GetSkinPartitionRef());
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
		bsdSkinInst->partitions.resize(bsdSkinInst->numPartitions);
		for (int i = 0; i < bsdSkinInst->numPartitions; i++) {
			BSDismemberSkinInstance::Partition p;
			if (hdr.VerCheck(20, 2, 0, 7) && hdr.GetUserVersion() == 11)
				p.partID = 0;
			else
				p.partID = 32;

			if (i == 0)
				p.flags = 257;
			else
				p.flags = 257;

			bsdSkinInst->partitions[i] = p;
		}
	}
}

void NifFile::UpdateBoundingSphere(const string& shapeName) {
	BlockType bType;
	int dataRef = shapeDataIdForName(shapeName, bType);
	if (dataRef == 0xFFFFFFFF)
		return;

	if (bType == NITRISHAPEDATA || bType == NITRISTRIPSDATA) {
		NiTriBasedGeom* geom = geomForName(shapeName);
		if (!geom)
			return;

		NiTriBasedGeomData* geomData = dynamic_cast<NiTriBasedGeomData*>(hdr.GetBlock(dataRef));
		if (!geomData)
			return;

		geomData->bounds = BoundingSphere(geomData->vertices);
	}
	else if (bType == BSSUBINDEXTRISHAPE || bType == BSTRISHAPE || bType == BSMESHLODTRISHAPE) {
		BSTriShape* geom = static_cast<BSTriShape*>(hdr.GetBlock(dataRef));
		const vector<Vector3>* vertices = geom->GetRawVerts();
		geom->bounds = BoundingSphere(*vertices);
	}
}
