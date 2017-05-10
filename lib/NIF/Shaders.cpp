/*
BodySlide and Outfit Studio
Copyright (C) 2017  Caliente & ousnius
See the included LICENSE file
*/

#include "Shaders.h"

void BSShaderProperty::Init(NiHeader* hdr) {
	NiProperty::Init(hdr);

	shaderFlags = 1;
	shaderType = BSShaderType::SHADER_DEFAULT;
	shaderFlags1 = 0x82000000;
	shaderFlags2 = 1;
	environmentMapScale = 1.0f;
	uvScale = Vector2(1.0f, 1.0f);
}

void BSShaderProperty::Get(std::fstream& file) {
	NiProperty::Get(file);

	if (header->GetUserVersion() <= 11) {
		file.read((char*)&shaderFlags, 2);
		file.read((char*)&shaderType, 4);
		file.read((char*)&shaderFlags1, 4);
		file.read((char*)&shaderFlags2, 4);
		file.read((char*)&environmentMapScale, 4);
	}
	else {
		file.read((char*)&shaderFlags1, 4);
		file.read((char*)&shaderFlags2, 4);
		file.read((char*)&uvOffset, 8);
		file.read((char*)&uvScale, 8);
	}
}

void BSShaderProperty::Put(std::fstream& file) {
	NiProperty::Put(file);

	if (header->GetUserVersion() <= 11) {
		file.write((char*)&shaderFlags, 2);
		file.write((char*)&shaderType, 4);
		file.write((char*)&shaderFlags1, 4);
		file.write((char*)&shaderFlags2, 4);
		file.write((char*)&environmentMapScale, 4);
	}
	else {
		file.write((char*)&shaderFlags1, 4);
		file.write((char*)&shaderFlags2, 4);
		file.write((char*)&uvOffset, 8);
		file.write((char*)&uvScale, 8);
	}
}

uint BSShaderProperty::GetType() {
	return shaderType;
}

void BSShaderProperty::SetType(uint type) {
	shaderType = (BSShaderType)type;
}

bool BSShaderProperty::HasSpecular() {
	return (shaderFlags1 & (1 << 0)) != 0;
}

float BSShaderProperty::GetEnvironmentMapScale() {
	return environmentMapScale;
}

Vector2 BSShaderProperty::GetUVOffset() {
	return uvOffset;
}

Vector2 BSShaderProperty::GetUVScale() {
	return uvScale;
}

int BSShaderProperty::CalcBlockSize() {
	NiProperty::CalcBlockSize();

	blockSize += 8;
	if (header->GetUserVersion() <= 11)
		blockSize += 10;
	else
		blockSize += 16;

	return blockSize;
}


BSShaderTextureSet::BSShaderTextureSet(NiHeader* hdr) {
	NiObject::Init(hdr);

	if (header->GetUserVersion() == 12 && header->GetUserVersion2() >= 130)
		numTextures = 10;
	else if (header->GetUserVersion() == 12)
		numTextures = 9;
	else
		numTextures = 6;

	textures.resize(numTextures);
}

BSShaderTextureSet::BSShaderTextureSet(std::fstream& file, NiHeader* hdr) : BSShaderTextureSet(hdr) {
	Get(file);
}

void BSShaderTextureSet::Get(std::fstream& file) {
	NiObject::Get(file);

	file.read((char*)&numTextures, 4);
	textures.resize(numTextures);
	for (int i = 0; i < numTextures; i++)
		textures[i].Get(file, 4);
}

void BSShaderTextureSet::Put(std::fstream& file) {
	NiObject::Put(file);

	file.write((char*)&numTextures, 4);
	for (int i = 0; i < numTextures; i++)
		textures[i].Put(file, 4, false);
}

int BSShaderTextureSet::CalcBlockSize() {
	NiObject::CalcBlockSize();

	blockSize += 4;
	for (auto &tex : textures) {
		blockSize += 4;
		blockSize += tex.GetLength();
	}

	return blockSize;
}


BSLightingShaderProperty::BSLightingShaderProperty(NiHeader* hdr) {
	BSShaderProperty::Init(hdr);
	NiObjectNET::bBSLightingShaderProperty = true;

	if (header->GetUserVersion() == 12 && header->GetUserVersion2() >= 120) {
		shaderFlags1 = 0x80400203;
		shaderFlags2 = 0x00000081;
	}
	else {
		shaderFlags1 = 0x82400303;
		shaderFlags2 = 0x00008001;
	}

	emissiveMultiple = 1.0f;
	textureClampMode = 3;
	alpha = 1.0f;
	refractionStrength = 0.0f;

	if (header->GetUserVersion() == 12 && header->GetUserVersion2() >= 120)
		glossiness = 1.0f;
	else
		glossiness = 20.0f;

	specularColor = Vector3(1.0f, 1.0f, 1.0f);
	specularStrength = 1.0f;
	lightingEffect1 = 0.3f;
	lightingEffect2 = 2.0f;

	subsurfaceRolloff = 0.0f;
	unkFloat1 = std::numeric_limits<float>::max();
	backlightPower = 0.0f;
	grayscaleToPaletteScale = 1.0f;
	fresnelPower = 5.0f;
	wetnessSpecScale = 0.6f;
	wetnessSpecPower = 1.4f;
	wetnessMinVar = 0.2f;
	wetnessEnvmapScale = 1.0f;
	wetnessFresnelPower = 1.6f;
	wetnessMetalness = 0.0f;

	unkEnvmap = 0;
	unkSkinTint = 0;
	maxPasses = 1.0f;
	scale = 1.0f;
	parallaxInnerLayerThickness = 0.0f;
	parallaxRefractionScale = 1.0f;
	parallaxInnerLayerTextureScale.u = 1.0f;
	parallaxInnerLayerTextureScale.v = 1.0f;
	parallaxEnvmapStrength = 1.0f;
	eyeCubemapScale = 1.0f;
}

BSLightingShaderProperty::BSLightingShaderProperty(std::fstream& file, NiHeader* hdr) : BSLightingShaderProperty(hdr) {
	if (header->GetUserVersion() == 12 && header->GetUserVersion2() >= 120) {
		shaderFlags1 = 0x80400203;
		shaderFlags2 = 0x00000081;
	}
	else {
		shaderFlags1 = 0x82400303;
		shaderFlags2 = 0x00008001;
	}

	lightingEffect1 = 0.3f;
	lightingEffect2 = 2.0f;

	subsurfaceRolloff = 0.0f;
	unkFloat1 = std::numeric_limits<float>::max();
	backlightPower = 0.0f;
	grayscaleToPaletteScale = 1.0f;
	fresnelPower = 5.0f;
	wetnessSpecScale = 0.6f;
	wetnessSpecPower = 1.4f;
	wetnessMinVar = 0.2f;
	wetnessEnvmapScale = 1.0f;
	wetnessFresnelPower = 1.6f;
	wetnessMetalness = 0.0f;

	unkEnvmap = 0;
	unkSkinTint = 0;
	maxPasses = 1.0f;
	scale = 1.0f;
	parallaxInnerLayerThickness = 0.0f;
	parallaxRefractionScale = 1.0f;
	parallaxInnerLayerTextureScale.u = 1.0f;
	parallaxInnerLayerTextureScale.v = 1.0f;
	parallaxEnvmapStrength = 1.0f;
	eyeCubemapScale = 1.0f;

	Get(file);
}

void BSLightingShaderProperty::Get(std::fstream& file) {
	BSShaderProperty::Get(file);

	textureSetRef.Get(file);

	file.read((char*)&emissiveColor.x, 4);
	file.read((char*)&emissiveColor.y, 4);
	file.read((char*)&emissiveColor.z, 4);
	file.read((char*)&emissiveMultiple, 4);

	if (header->GetUserVersion() == 12 && header->GetUserVersion2() >= 130)
		wetMaterialName.Get(file, header);

	file.read((char*)&textureClampMode, 4);
	file.read((char*)&alpha, 4);
	file.read((char*)&refractionStrength, 4);
	file.read((char*)&glossiness, 4);
	file.read((char*)&specularColor.x, 4);
	file.read((char*)&specularColor.y, 4);
	file.read((char*)&specularColor.z, 4);
	file.read((char*)&specularStrength, 4);

	if (header->GetUserVersion() <= 12 && header->GetUserVersion2() < 130) {
		file.read((char*)&lightingEffect1, 4);
		file.read((char*)&lightingEffect2, 4);
	}

	if (header->GetUserVersion() == 12 && header->GetUserVersion2() >= 130) {
		file.read((char*)&subsurfaceRolloff, 4);
		file.read((char*)&unkFloat1, 4);
		file.read((char*)&backlightPower, 4);
		file.read((char*)&grayscaleToPaletteScale, 4);
		file.read((char*)&fresnelPower, 4);
		file.read((char*)&wetnessSpecScale, 4);
		file.read((char*)&wetnessSpecPower, 4);
		file.read((char*)&wetnessMinVar, 4);
		file.read((char*)&wetnessEnvmapScale, 4);
		file.read((char*)&wetnessFresnelPower, 4);
		file.read((char*)&wetnessMetalness, 4);
	}

	switch (skyrimShaderType) {
	case 1:
		file.read((char*)&environmentMapScale, 4);
		if (header->GetUserVersion() == 12 && header->GetUserVersion2() >= 130)
			file.read((char*)&unkEnvmap, 2);
		break;
	case 5:
		if (header->GetUserVersion() == 12 && header->GetUserVersion2() >= 130)
			file.read((char*)&unkSkinTint, 4);

		file.read((char*)&skinTintColor.x, 4);
		file.read((char*)&skinTintColor.y, 4);
		file.read((char*)&skinTintColor.z, 4);
		break;
	case 6:
		file.read((char*)&hairTintColor.x, 4);
		file.read((char*)&hairTintColor.y, 4);
		file.read((char*)&hairTintColor.z, 4);
		break;
	case 7:
		file.read((char*)&maxPasses, 4);
		file.read((char*)&scale, 4);
		break;
	case 11:
		file.read((char*)&parallaxInnerLayerThickness, 4);
		file.read((char*)&parallaxRefractionScale, 4);
		file.read((char*)&parallaxInnerLayerTextureScale.u, 4);
		file.read((char*)&parallaxInnerLayerTextureScale.v, 4);
		file.read((char*)&parallaxEnvmapStrength, 4);
		break;
	case 14:
		file.read((char*)&sparkleParameters.r, 4);
		file.read((char*)&sparkleParameters.g, 4);
		file.read((char*)&sparkleParameters.b, 4);
		file.read((char*)&sparkleParameters.a, 4);
		break;
	case 16:
		file.read((char*)&eyeCubemapScale, 4);
		file.read((char*)&eyeLeftReflectionCenter.x, 4);
		file.read((char*)&eyeLeftReflectionCenter.y, 4);
		file.read((char*)&eyeLeftReflectionCenter.z, 4);
		file.read((char*)&eyeRightReflectionCenter.x, 4);
		file.read((char*)&eyeRightReflectionCenter.y, 4);
		file.read((char*)&eyeRightReflectionCenter.z, 4);
		break;
	}
}

void BSLightingShaderProperty::Put(std::fstream& file) {
	BSShaderProperty::Put(file);

	textureSetRef.Put(file);

	file.write((char*)&emissiveColor.x, 4);
	file.write((char*)&emissiveColor.y, 4);
	file.write((char*)&emissiveColor.z, 4);
	file.write((char*)&emissiveMultiple, 4);

	if (header->GetUserVersion() == 12 && header->GetUserVersion2() >= 130)
		wetMaterialName.Put(file);

	file.write((char*)&textureClampMode, 4);
	file.write((char*)&alpha, 4);
	file.write((char*)&refractionStrength, 4);
	file.write((char*)&glossiness, 4);
	file.write((char*)&specularColor.x, 4);
	file.write((char*)&specularColor.y, 4);
	file.write((char*)&specularColor.z, 4);
	file.write((char*)&specularStrength, 4);

	if (header->GetUserVersion() <= 12 && header->GetUserVersion2() < 130) {
		file.write((char*)&lightingEffect1, 4);
		file.write((char*)&lightingEffect2, 4);
	}

	if (header->GetUserVersion() == 12 && header->GetUserVersion2() >= 130) {
		file.write((char*)&subsurfaceRolloff, 4);
		file.write((char*)&unkFloat1, 4);
		file.write((char*)&backlightPower, 4);
		file.write((char*)&grayscaleToPaletteScale, 4);
		file.write((char*)&fresnelPower, 4);
		file.write((char*)&wetnessSpecScale, 4);
		file.write((char*)&wetnessSpecPower, 4);
		file.write((char*)&wetnessMinVar, 4);
		file.write((char*)&wetnessEnvmapScale, 4);
		file.write((char*)&wetnessFresnelPower, 4);
		file.write((char*)&wetnessMetalness, 4);
	}

	switch (skyrimShaderType) {
	case 1:
		file.write((char*)&environmentMapScale, 4);
		if (header->GetUserVersion() == 12 && header->GetUserVersion2() >= 130)
			file.write((char*)&unkEnvmap, 2);
		break;
	case 5:
		if (header->GetUserVersion() == 12 && header->GetUserVersion2() >= 130)
			file.write((char*)&unkSkinTint, 4);

		file.write((char*)&skinTintColor.x, 4);
		file.write((char*)&skinTintColor.y, 4);
		file.write((char*)&skinTintColor.z, 4);
		break;
	case 6:
		file.write((char*)&hairTintColor.x, 4);
		file.write((char*)&hairTintColor.y, 4);
		file.write((char*)&hairTintColor.z, 4);
		break;
	case 7:
		file.write((char*)&maxPasses, 4);
		file.write((char*)&scale, 4);
		break;
	case 11:
		file.write((char*)&parallaxInnerLayerThickness, 4);
		file.write((char*)&parallaxRefractionScale, 4);
		file.write((char*)&parallaxInnerLayerTextureScale.u, 4);
		file.write((char*)&parallaxInnerLayerTextureScale.v, 4);
		file.write((char*)&parallaxEnvmapStrength, 4);
		break;
	case 14:
		file.write((char*)&sparkleParameters.r, 4);
		file.write((char*)&sparkleParameters.g, 4);
		file.write((char*)&sparkleParameters.b, 4);
		file.write((char*)&sparkleParameters.a, 4);
		break;
	case 16:
		file.write((char*)&eyeCubemapScale, 4);
		file.write((char*)&eyeLeftReflectionCenter.x, 4);
		file.write((char*)&eyeLeftReflectionCenter.y, 4);
		file.write((char*)&eyeLeftReflectionCenter.z, 4);
		file.write((char*)&eyeRightReflectionCenter.x, 4);
		file.write((char*)&eyeRightReflectionCenter.y, 4);
		file.write((char*)&eyeRightReflectionCenter.z, 4);
		break;
	}
}

void BSLightingShaderProperty::notifyStringDelete(int stringID) {
	BSShaderProperty::notifyStringDelete(stringID);

	wetMaterialName.notifyStringDelete(stringID);
}

void BSLightingShaderProperty::GetChildRefs(std::set<int*>& refs) {
	BSShaderProperty::GetChildRefs(refs);

	refs.insert(&textureSetRef.index);
}

bool BSLightingShaderProperty::IsSkinTint() {
	return skyrimShaderType == 5;
}

bool BSLightingShaderProperty::IsSkinned() {
	return (shaderFlags1 & (1 << 1)) != 0;
}

void BSLightingShaderProperty::SetSkinned(const bool enable) {
	if (enable)
		shaderFlags1 |= 1 << 1;
	else
		shaderFlags1 &= ~(1 << 1);
}

bool BSLightingShaderProperty::IsDoubleSided() {
	return (shaderFlags2 & (1 << 4)) == 16;
}

bool BSLightingShaderProperty::IsModelSpace() {
	return (shaderFlags1 & (1 << 12)) != 0;
}

bool BSLightingShaderProperty::IsEmissive() {
	return (shaderFlags1 & (1 << 22)) != 0;
}

bool BSLightingShaderProperty::HasBacklight() {
	return (shaderFlags2 & (1 << 27)) != 0;
}

uint BSLightingShaderProperty::GetType() {
	return skyrimShaderType;
}

void BSLightingShaderProperty::SetType(uint type) {
	skyrimShaderType = type;
}

Vector3 BSLightingShaderProperty::GetSpecularColor() {
	return specularColor;
}

void BSLightingShaderProperty::SetSpecularColor(Vector3 color) {
	specularColor = color;
}

float BSLightingShaderProperty::GetSpecularStrength() {
	return specularStrength;
}

void BSLightingShaderProperty::SetSpecularStrength(float strength) {
	specularStrength = strength;
}

float BSLightingShaderProperty::GetGlossiness() {
	return glossiness;
}

void BSLightingShaderProperty::SetGlossiness(float gloss) {
	glossiness = gloss;
}

int BSLightingShaderProperty::GetTextureSetRef() {
	return textureSetRef.index;
}

void BSLightingShaderProperty::SetTextureSetRef(const int texSetRef) {
	textureSetRef.index = texSetRef;
}

Color4 BSLightingShaderProperty::GetEmissiveColor() {
	Color4 color;
	color.r = emissiveColor.x;
	color.g = emissiveColor.y;
	color.b = emissiveColor.z;
	return color;
}

void BSLightingShaderProperty::SetEmissiveColor(Color4 color) {
	emissiveColor.x = color.r;
	emissiveColor.y = color.g;
	emissiveColor.z = color.b;
}

float BSLightingShaderProperty::GetEmissiveMultiple() {
	return emissiveMultiple;
}

void BSLightingShaderProperty::SetEmissiveMultiple(float emissive) {
	emissiveMultiple = emissive;
}

float BSLightingShaderProperty::GetAlpha() {
	return alpha;
}

std::string BSLightingShaderProperty::GetWetMaterialName() {
	return wetMaterialName.GetString(header);
}

void BSLightingShaderProperty::SetWetMaterialName(const std::string& matName) {
	wetMaterialName.SetString(header, matName);
}

int BSLightingShaderProperty::CalcBlockSize() {
	BSShaderProperty::CalcBlockSize();

	blockSize += 20;

	if (header->GetUserVersion() == 12 && header->GetUserVersion2() >= 130)
		blockSize += 4;

	blockSize += 32;

	if (header->GetUserVersion() <= 12 && header->GetUserVersion2() < 130)
		blockSize += 8;

	if (header->GetUserVersion() == 12 && header->GetUserVersion2() >= 130)
		blockSize += 44;

	switch (skyrimShaderType) {
	case 1:
		blockSize += 4;
		if (header->GetUserVersion() == 12 && header->GetUserVersion2() >= 130)
			blockSize += 2;
		break;
	case 5:
		if (header->GetUserVersion() == 12 && header->GetUserVersion2() >= 130)
			blockSize += 4;

		blockSize += 12;
		break;
	case 6:
		blockSize += 12;
		break;
	case 7:
		blockSize += 8;
		break;
	case 11:
		blockSize += 20;
		break;
	case 14:
		blockSize += 16;
		break;
	case 16:
		blockSize += 28;
		break;
	}

	return blockSize;
}


BSEffectShaderProperty::BSEffectShaderProperty(NiHeader* hdr) {
	BSShaderProperty::Init(hdr);

	textureClampMode = 0;
	falloffStartAngle = 1.0f;
	falloffStopAngle = 1.0f;
	falloffStartOpacity = 0.0f;
	falloffStopOpacity = 0.0f;
	emissiveColor.r = 0.0f;
	emissiveColor.g = 0.0f;
	emissiveColor.b = 0.0f;
	emissiveColor.a = 0.0f;
	emissiveMultiple = 0.0f;
	softFalloffDepth = 0.0f;

	envMapScale = 1.0f;
}

BSEffectShaderProperty::BSEffectShaderProperty(std::fstream& file, NiHeader* hdr) : BSEffectShaderProperty(hdr) {
	Get(file);
}

void BSEffectShaderProperty::Get(std::fstream& file) {
	BSShaderProperty::Get(file);

	sourceTexture.Get(file, 4);
	file.read((char*)&textureClampMode, 4);

	file.read((char*)&falloffStartAngle, 4);
	file.read((char*)&falloffStopAngle, 4);
	file.read((char*)&falloffStartOpacity, 4);
	file.read((char*)&falloffStopOpacity, 4);
	file.read((char*)&emissiveColor, 16);
	file.read((char*)&emissiveMultiple, 4);
	file.read((char*)&softFalloffDepth, 4);
	greyscaleTexture.Get(file, 4);

	if (header->GetUserVersion() == 12 && header->GetUserVersion2() >= 130) {
		envMapTexture.Get(file, 4);
		normalTexture.Get(file, 4);
		envMaskTexture.Get(file, 4);
		file.read((char*)&envMapScale, 4);
	}
}

void BSEffectShaderProperty::Put(std::fstream& file) {
	BSShaderProperty::Put(file);

	sourceTexture.Put(file, 4, false);
	file.write((char*)&textureClampMode, 4);

	file.write((char*)&falloffStartAngle, 4);
	file.write((char*)&falloffStopAngle, 4);
	file.write((char*)&falloffStartOpacity, 4);
	file.write((char*)&falloffStopOpacity, 4);
	file.write((char*)&emissiveColor, 16);
	file.write((char*)&emissiveMultiple, 4);
	file.write((char*)&softFalloffDepth, 4);
	greyscaleTexture.Put(file, 4, false);

	if (header->GetUserVersion() == 12 && header->GetUserVersion2() >= 130) {
		envMapTexture.Put(file, 4, false);
		normalTexture.Put(file, 4, false);
		envMaskTexture.Put(file, 4, false);
		file.write((char*)&envMapScale, 4);
	}
}

bool BSEffectShaderProperty::IsSkinTint() {
	return (shaderFlags1 & (1 << 21)) != 0;
}

bool BSEffectShaderProperty::IsSkinned() {
	return (shaderFlags1 & (1 << 1)) != 0;
}

void BSEffectShaderProperty::SetSkinned(const bool enable) {
	if (enable)
		shaderFlags1 |= 1 << 1;
	else
		shaderFlags1 &= ~(1 << 1);
}

bool BSEffectShaderProperty::IsDoubleSided() {
	return (shaderFlags2 & (1 << 4)) == 16;
}

bool BSEffectShaderProperty::IsModelSpace() {
	return (shaderFlags1 & (1 << 12)) != 0;
}

bool BSEffectShaderProperty::IsEmissive() {
	return (shaderFlags1 & (1 << 22)) != 0;
}

bool BSEffectShaderProperty::HasBacklight() {
	return (shaderFlags2 & (1 << 27)) != 0;
}

float BSEffectShaderProperty::GetEnvironmentMapScale() {
	return envMapScale;
}

Color4 BSEffectShaderProperty::GetEmissiveColor() {
	return emissiveColor;
}

void BSEffectShaderProperty::SetEmissiveColor(Color4 color) {
	emissiveColor = color;
}

float BSEffectShaderProperty::GetEmissiveMultiple() {
	return emissiveMultiple;
}

void BSEffectShaderProperty::SetEmissiveMultiple(float emissive) {
	emissiveMultiple = emissive;
}

int BSEffectShaderProperty::CalcBlockSize() {
	BSShaderProperty::CalcBlockSize();

	blockSize += 52;
	blockSize += sourceTexture.GetLength();
	blockSize += greyscaleTexture.GetLength();

	if (header->GetUserVersion() == 12 && header->GetUserVersion2() >= 130) {
		blockSize += 16;
		blockSize += envMapTexture.GetLength();
		blockSize += normalTexture.GetLength();
		blockSize += envMaskTexture.GetLength();
	}

	return blockSize;
}


BSWaterShaderProperty::BSWaterShaderProperty(NiHeader* hdr) {
	BSShaderProperty::Init(hdr);

	waterFlags = 0;
}

BSWaterShaderProperty::BSWaterShaderProperty(std::fstream& file, NiHeader* hdr) : BSWaterShaderProperty(hdr) {
	Get(file);
}

void BSWaterShaderProperty::Get(std::fstream& file) {
	BSShaderProperty::Get(file);

	file.read((char*)&waterFlags, 4);
}

void BSWaterShaderProperty::Put(std::fstream& file) {
	BSShaderProperty::Put(file);

	file.write((char*)&waterFlags, 4);
}

bool BSWaterShaderProperty::IsSkinTint() {
	return (shaderFlags1 & (1 << 21)) != 0;
}

bool BSWaterShaderProperty::IsSkinned() {
	return (shaderFlags1 & (1 << 1)) != 0;
}

void BSWaterShaderProperty::SetSkinned(const bool enable) {
	if (enable)
		shaderFlags1 |= 1 << 1;
	else
		shaderFlags1 &= ~(1 << 1);
}

bool BSWaterShaderProperty::IsDoubleSided() {
	return (shaderFlags2 & (1 << 4)) == 16;
}

bool BSWaterShaderProperty::IsModelSpace() {
	return (shaderFlags1 & (1 << 12)) != 0;
}

bool BSWaterShaderProperty::IsEmissive() {
	return (shaderFlags1 & (1 << 22)) != 0;
}

bool BSWaterShaderProperty::HasBacklight() {
	return (shaderFlags2 & (1 << 27)) != 0;
}

int BSWaterShaderProperty::CalcBlockSize() {
	BSShaderProperty::CalcBlockSize();

	blockSize += 4;

	return blockSize;
}


BSSkyShaderProperty::BSSkyShaderProperty(NiHeader* hdr) {
	BSShaderProperty::Init(hdr);

	skyFlags = 0;
}

BSSkyShaderProperty::BSSkyShaderProperty(std::fstream& file, NiHeader* hdr) : BSSkyShaderProperty(hdr) {
	Get(file);
}

void BSSkyShaderProperty::Get(std::fstream& file) {
	BSShaderProperty::Get(file);

	baseTexture.Get(file, 4);
	file.read((char*)&skyFlags, 4);
}

void BSSkyShaderProperty::Put(std::fstream& file) {
	BSShaderProperty::Put(file);

	baseTexture.Put(file, 4, false);
	file.write((char*)&skyFlags, 4);
}

bool BSSkyShaderProperty::IsSkinTint() {
	return (shaderFlags1 & (1 << 21)) != 0;
}

bool BSSkyShaderProperty::IsSkinned() {
	return (shaderFlags1 & (1 << 1)) != 0;
}

void BSSkyShaderProperty::SetSkinned(const bool enable) {
	if (enable)
		shaderFlags1 |= 1 << 1;
	else
		shaderFlags1 &= ~(1 << 1);
}

bool BSSkyShaderProperty::IsDoubleSided() {
	return (shaderFlags2 & (1 << 4)) == 16;
}

bool BSSkyShaderProperty::IsModelSpace() {
	return (shaderFlags1 & (1 << 12)) != 0;
}

bool BSSkyShaderProperty::IsEmissive() {
	return (shaderFlags1 & (1 << 22)) != 0;
}

bool BSSkyShaderProperty::HasBacklight() {
	return (shaderFlags2 & (1 << 27)) != 0;
}

int BSSkyShaderProperty::CalcBlockSize() {
	BSShaderProperty::CalcBlockSize();

	blockSize += 8;
	blockSize += baseTexture.GetLength();

	return blockSize;
}


void BSShaderLightingProperty::Init(NiHeader* hdr) {
	BSShaderProperty::Init(hdr);

	textureClampMode = 3;
}

void BSShaderLightingProperty::Get(std::fstream& file) {
	BSShaderProperty::Get(file);

	if (header->GetUserVersion() <= 11)
		file.read((char*)&textureClampMode, 4);
}

void BSShaderLightingProperty::Put(std::fstream& file) {
	BSShaderProperty::Put(file);

	if (header->GetUserVersion() <= 11)
		file.write((char*)&textureClampMode, 4);
}

int BSShaderLightingProperty::CalcBlockSize() {
	BSShaderProperty::CalcBlockSize();

	if (header->GetUserVersion() <= 11)
		blockSize += 4;

	return blockSize;
}


BSShaderPPLightingProperty::BSShaderPPLightingProperty(NiHeader* hdr) {
	BSShaderLightingProperty::Init(hdr);

	refractionStrength = 0.0;
	refractionFirePeriod = 0;
	parallaxMaxPasses = 4.0f;
	parallaxScale = 1.0f;
	emissiveColor.r = 0.0f;
	emissiveColor.g = 0.0f;
	emissiveColor.b = 0.0f;
	emissiveColor.a = 0.0f;
}

BSShaderPPLightingProperty::BSShaderPPLightingProperty(std::fstream& file, NiHeader* hdr) : BSShaderPPLightingProperty(hdr) {
	Get(file);
}

void BSShaderPPLightingProperty::Get(std::fstream& file) {
	BSShaderLightingProperty::Get(file);

	textureSetRef.Get(file);

	if (header->GetUserVersion() == 11) {
		file.read((char*)&refractionStrength, 4);
		file.read((char*)&refractionFirePeriod, 4);
		file.read((char*)&parallaxMaxPasses, 4);
		file.read((char*)&parallaxScale, 4);
	}

	if (header->GetUserVersion() >= 12) {
		file.read((char*)&emissiveColor.r, 4);
		file.read((char*)&emissiveColor.g, 4);
		file.read((char*)&emissiveColor.b, 4);
		file.read((char*)&emissiveColor.a, 4);
	}
}

void BSShaderPPLightingProperty::Put(std::fstream& file) {
	BSShaderLightingProperty::Put(file);

	textureSetRef.Put(file);

	if (header->GetUserVersion() == 11) {
		file.write((char*)&refractionStrength, 4);
		file.write((char*)&refractionFirePeriod, 4);
		file.write((char*)&parallaxMaxPasses, 4);
		file.write((char*)&parallaxScale, 4);
	}

	if (header->GetUserVersion() >= 12) {
		file.write((char*)&emissiveColor.r, 4);
		file.write((char*)&emissiveColor.g, 4);
		file.write((char*)&emissiveColor.b, 4);
		file.write((char*)&emissiveColor.a, 4);
	}
}

void BSShaderPPLightingProperty::GetChildRefs(std::set<int*>& refs) {
	BSShaderLightingProperty::GetChildRefs(refs);

	refs.insert(&textureSetRef.index);
}

bool BSShaderPPLightingProperty::IsSkinTint() {
	return shaderType == 0x0000000e;
}

bool BSShaderPPLightingProperty::IsSkinned() {
	return (shaderFlags1 & (1 << 1)) != 0;
}

void BSShaderPPLightingProperty::SetSkinned(const bool enable) {
	if (enable)
		shaderFlags1 |= 1 << 1;
	else
		shaderFlags1 &= ~(1 << 1);
}

int BSShaderPPLightingProperty::GetTextureSetRef() {
	return textureSetRef.index;
}

void BSShaderPPLightingProperty::SetTextureSetRef(const int texSetRef) {
	textureSetRef.index = texSetRef;
}

int BSShaderPPLightingProperty::CalcBlockSize() {
	BSShaderLightingProperty::CalcBlockSize();

	blockSize += 4;

	if (header->GetUserVersion() == 11)
		blockSize += 16;

	if (header->GetUserVersion() >= 12)
		blockSize += 16;

	return blockSize;
}


BSShaderNoLightingProperty::BSShaderNoLightingProperty(NiHeader* hdr) {
	BSShaderLightingProperty::Init(hdr);

	falloffStartAngle = 1.0f;
	falloffStopAngle = 0.0f;
	falloffStartOpacity = 1.0f;
	falloffStopOpacity = 1.0f;

}

BSShaderNoLightingProperty::BSShaderNoLightingProperty(std::fstream& file, NiHeader* hdr) : BSShaderNoLightingProperty(hdr) {
	Get(file);
}

void BSShaderNoLightingProperty::Get(std::fstream& file) {
	BSShaderLightingProperty::Get(file);

	baseTexture.Get(file, 4);

	if (header->GetUserVersion2() > 26) {
		file.read((char*)&falloffStartAngle, 4);
		file.read((char*)&falloffStopAngle, 4);
		file.read((char*)&falloffStartOpacity, 4);
		file.read((char*)&falloffStopOpacity, 4);
	}
}

void BSShaderNoLightingProperty::Put(std::fstream& file) {
	BSShaderLightingProperty::Put(file);

	baseTexture.Put(file, 4, false);

	if (header->GetUserVersion2() > 26) {
		file.write((char*)&falloffStartAngle, 4);
		file.write((char*)&falloffStopAngle, 4);
		file.write((char*)&falloffStartOpacity, 4);
		file.write((char*)&falloffStopOpacity, 4);
	}
}

bool BSShaderNoLightingProperty::IsSkinTint() {
	return shaderType == 0x0000000e;
}

bool BSShaderNoLightingProperty::IsSkinned() {
	return (shaderFlags1 & (1 << 1)) != 0;
}

void BSShaderNoLightingProperty::SetSkinned(const bool enable) {
	if (enable)
		shaderFlags1 |= 1 << 1;
	else
		shaderFlags1 &= ~(1 << 1);
}

int BSShaderNoLightingProperty::CalcBlockSize() {
	BSShaderLightingProperty::CalcBlockSize();

	blockSize += 4;
	blockSize += baseTexture.GetLength();

	if (header->GetUserVersion2() > 26)
		blockSize += 16;

	return blockSize;
}


NiAlphaProperty::NiAlphaProperty(NiHeader* hdr) {
	NiProperty::Init(hdr);

	flags = 4844;
	threshold = 128;
}

NiAlphaProperty::NiAlphaProperty(std::fstream& file, NiHeader* hdr) : NiAlphaProperty(hdr) {
	Get(file);
}

void NiAlphaProperty::Get(std::fstream& file) {
	NiProperty::Get(file);

	file.read((char*)&flags, 2);
	file.read((char*)&threshold, 1);
}

void NiAlphaProperty::Put(std::fstream& file) {
	NiProperty::Put(file);

	file.write((char*)&flags, 2);
	file.write((char*)&threshold, 1);
}

int NiAlphaProperty::CalcBlockSize() {
	NiProperty::CalcBlockSize();

	blockSize += 3;

	return blockSize;
}


NiMaterialProperty::NiMaterialProperty(NiHeader* hdr) {
	NiProperty::Init(hdr);

	glossiness = 1.0f;
	alpha = 1.0f;
	emitMulti = 1.0f;
}

NiMaterialProperty::NiMaterialProperty(std::fstream& file, NiHeader* hdr) : NiMaterialProperty(hdr) {
	Get(file);
}

void NiMaterialProperty::Get(std::fstream& file) {
	NiProperty::Get(file);

	if (!(header->VerCheck(20, 2, 0, 7, true) && header->GetUserVersion() >= 11 && header->GetUserVersion2() > 21)) {
		file.read((char*)&colorAmbient, 12);
		file.read((char*)&colorDiffuse, 12);
	}

	file.read((char*)&colorSpecular, 12);
	file.read((char*)&colorEmissive, 12);
	file.read((char*)&glossiness, 4);
	file.read((char*)&alpha, 4);

	if (header->VerCheck(20, 2, 0, 7, true) && header->GetUserVersion() >= 11 && header->GetUserVersion2() > 21)
		file.read((char*)&emitMulti, 4);
}

void NiMaterialProperty::Put(std::fstream& file) {
	NiProperty::Put(file);

	if (!(header->VerCheck(20, 2, 0, 7, true) && header->GetUserVersion() >= 11 && header->GetUserVersion2() > 21)) {
		file.write((char*)&colorAmbient, 12);
		file.write((char*)&colorDiffuse, 12);
	}

	file.write((char*)&colorSpecular, 12);
	file.write((char*)&colorEmissive, 12);
	file.write((char*)&glossiness, 4);
	file.write((char*)&alpha, 4);

	if (header->VerCheck(20, 2, 0, 7, true) && header->GetUserVersion() >= 11 && header->GetUserVersion2() > 21)
		file.write((char*)&emitMulti, 4);
}

bool NiMaterialProperty::IsEmissive() {
	return true;
}

Vector3 NiMaterialProperty::GetSpecularColor() {
	return colorSpecular;
}

void NiMaterialProperty::SetSpecularColor(Vector3 color) {
	colorSpecular = color;
}

float NiMaterialProperty::GetGlossiness() {
	return glossiness;
}

void NiMaterialProperty::SetGlossiness(float gloss) {
	glossiness = gloss;
}

Color4 NiMaterialProperty::GetEmissiveColor() {
	Color4 color;
	color.r = colorEmissive.x;
	color.g = colorEmissive.y;
	color.b = colorEmissive.z;
	return color;
}

void NiMaterialProperty::SetEmissiveColor(Color4 color) {
	colorEmissive.x = color.r;
	colorEmissive.y = color.g;
	colorEmissive.z = color.b;
}

float NiMaterialProperty::GetEmissiveMultiple() {
	return emitMulti;
}

void NiMaterialProperty::SetEmissiveMultiple(float emissive) {
	emitMulti = emissive;
}

float NiMaterialProperty::GetAlpha() {
	return alpha;
}

int NiMaterialProperty::CalcBlockSize() {
	NiProperty::CalcBlockSize();

	if (!(header->VerCheck(20, 2, 0, 7, true) && header->GetUserVersion() >= 11 && header->GetUserVersion2() > 21))
		blockSize += 24;
	else
		blockSize += 4;

	blockSize += 32;

	return blockSize;
}


NiStencilProperty::NiStencilProperty(NiHeader* hdr) {
	NiProperty::Init(hdr);

	flags = 19840;
	stencilRef = 0;
	stencilMask = 0xffffffff;
}

NiStencilProperty::NiStencilProperty(std::fstream& file, NiHeader* hdr) : NiStencilProperty(hdr) {
	Get(file);
}

void NiStencilProperty::Get(std::fstream& file) {
	NiProperty::Get(file);

	file.read((char*)&flags, 2);
	file.read((char*)&stencilRef, 4);
	file.read((char*)&stencilMask, 4);
}

void NiStencilProperty::Put(std::fstream& file) {
	NiProperty::Put(file);

	file.write((char*)&flags, 2);
	file.write((char*)&stencilRef, 4);
	file.write((char*)&stencilMask, 4);
}

int NiStencilProperty::CalcBlockSize() {
	NiProperty::CalcBlockSize();

	blockSize += 10;

	return blockSize;
}
