/*
BodySlide and Outfit Studio
Copyright (C) 2017  Caliente & ousnius
See the included LICENSE file
*/

#include "Shaders.h"

void BSShaderProperty::Get(NiStream& stream) {
	NiProperty::Get(stream);

	if (stream.GetVersion().User() <= 11) {
		stream >> shaderFlags;
		stream >> shaderType;
		stream >> shaderFlags1;
		stream >> shaderFlags2;
		stream >> environmentMapScale;
	}
	else {
		stream >> shaderFlags1;
		stream >> shaderFlags2;
		stream >> uvOffset;
		stream >> uvScale;
	}
}

void BSShaderProperty::Put(NiStream& stream) {
	NiProperty::Put(stream);

	if (stream.GetVersion().User() <= 11) {
		stream << shaderFlags;
		stream << shaderType;
		stream << shaderFlags1;
		stream << shaderFlags2;
		stream << environmentMapScale;
	}
	else {
		stream << shaderFlags1;
		stream << shaderFlags2;
		stream << uvOffset;
		stream << uvScale;
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

int BSShaderProperty::CalcBlockSize(NiVersion& version) {
	NiProperty::CalcBlockSize(version);

	blockSize += 8;
	if (version.User() <= 11)
		blockSize += 10;
	else
		blockSize += 16;

	return blockSize;
}


BSShaderTextureSet::BSShaderTextureSet() {
	textures.resize(numTextures);
}

BSShaderTextureSet::BSShaderTextureSet(NiVersion& version) {
	if (version.User() == 12 && version.User2() >= 130)
		numTextures = 10;
	else if (version.User() == 12)
		numTextures = 9;
	else
		numTextures = 6;

	textures.resize(numTextures);
}

BSShaderTextureSet::BSShaderTextureSet(NiStream& stream) : BSShaderTextureSet() {
	Get(stream);
}

void BSShaderTextureSet::Get(NiStream& stream) {
	NiObject::Get(stream);

	stream >> numTextures;
	textures.resize(numTextures);
	for (int i = 0; i < numTextures; i++)
		textures[i].Get(stream, 4);
}

void BSShaderTextureSet::Put(NiStream& stream) {
	NiObject::Put(stream);

	stream << numTextures;
	for (int i = 0; i < numTextures; i++)
		textures[i].Put(stream, 4, false);
}

int BSShaderTextureSet::CalcBlockSize(NiVersion& version) {
	NiObject::CalcBlockSize(version);

	blockSize += 4;
	for (auto &tex : textures) {
		blockSize += 4;
		blockSize += tex.GetLength();
	}

	return blockSize;
}

BSLightingShaderProperty::BSLightingShaderProperty() {
	NiObjectNET::bBSLightingShaderProperty = true;

	shaderFlags1 = 0x80400203;
	shaderFlags2 = 0x00000081;

	unkFloat1 = std::numeric_limits<float>::max();
}

BSLightingShaderProperty::BSLightingShaderProperty(NiVersion& version) : BSLightingShaderProperty() {
	if (version.User() == 12 && version.User2() >= 120) {
		shaderFlags1 = 0x80400203;
		shaderFlags2 = 0x00000081;
	}
	else {
		shaderFlags1 = 0x82400303;
		shaderFlags2 = 0x00008001;
	}

	if (version.User() == 12 && version.User2() >= 120)
		glossiness = 1.0f;
	else
		glossiness = 20.0f;
}

BSLightingShaderProperty::BSLightingShaderProperty(NiStream& stream) : BSLightingShaderProperty(stream.GetVersion()) {
	Get(stream);
}

void BSLightingShaderProperty::Get(NiStream& stream) {
	BSShaderProperty::Get(stream);

	textureSetRef.Get(stream);

	stream >> emissiveColor;
	stream >> emissiveMultiple;

	if (stream.GetVersion().User() == 12 && stream.GetVersion().User2() >= 130)
		wetMaterialName.Get(stream);

	stream >> textureClampMode;
	stream >> alpha;
	stream >> refractionStrength;
	stream >> glossiness;
	stream >> specularColor;
	stream >> specularStrength;

	if (stream.GetVersion().User() <= 12 && stream.GetVersion().User2() < 130) {
		stream >> lightingEffect1;
		stream >> lightingEffect2;
	}

	if (stream.GetVersion().User() == 12 && stream.GetVersion().User2() >= 130) {
		stream >> subsurfaceRolloff;
		stream >> unkFloat1;
		stream >> backlightPower;
		stream >> grayscaleToPaletteScale;
		stream >> fresnelPower;
		stream >> wetnessSpecScale;
		stream >> wetnessSpecPower;
		stream >> wetnessMinVar;
		stream >> wetnessEnvmapScale;
		stream >> wetnessFresnelPower;
		stream >> wetnessMetalness;
	}

	switch (skyrimShaderType) {
	case 1:
		stream >> environmentMapScale;
		if (stream.GetVersion().User() == 12 && stream.GetVersion().User2() >= 130)
			stream >> unkEnvmap;
		break;
	case 5:
		if (stream.GetVersion().User() == 12 && stream.GetVersion().User2() >= 130)
			stream >> unkSkinTint;

		stream >> skinTintColor;
		break;
	case 6:
		stream >> hairTintColor;
		break;
	case 7:
		stream >> maxPasses;
		stream >> scale;
		break;
	case 11:
		stream >> parallaxInnerLayerThickness;
		stream >> parallaxRefractionScale;
		stream >> parallaxInnerLayerTextureScale;
		stream >> parallaxEnvmapStrength;
		break;
	case 14:
		stream >> sparkleParameters;
		break;
	case 16:
		stream >> eyeCubemapScale;
		stream >> eyeLeftReflectionCenter;
		stream >> eyeRightReflectionCenter;
		break;
	}
}

void BSLightingShaderProperty::Put(NiStream& stream) {
	BSShaderProperty::Put(stream);

	textureSetRef.Put(stream);

	stream << emissiveColor;
	stream << emissiveMultiple;

	if (stream.GetVersion().User() == 12 && stream.GetVersion().User2() >= 130)
		wetMaterialName.Put(stream);

	stream << textureClampMode;
	stream << alpha;
	stream << refractionStrength;
	stream << glossiness;
	stream << specularColor;
	stream << specularStrength;

	if (stream.GetVersion().User() <= 12 && stream.GetVersion().User2() < 130) {
		stream << lightingEffect1;
		stream << lightingEffect2;
	}

	if (stream.GetVersion().User() == 12 && stream.GetVersion().User2() >= 130) {
		stream << subsurfaceRolloff;
		stream << unkFloat1;
		stream << backlightPower;
		stream << grayscaleToPaletteScale;
		stream << fresnelPower;
		stream << wetnessSpecScale;
		stream << wetnessSpecPower;
		stream << wetnessMinVar;
		stream << wetnessEnvmapScale;
		stream << wetnessFresnelPower;
		stream << wetnessMetalness;
	}

	switch (skyrimShaderType) {
	case 1:
		stream << environmentMapScale;
		if (stream.GetVersion().User() == 12 && stream.GetVersion().User2() >= 130)
			stream << unkEnvmap;
		break;
	case 5:
		if (stream.GetVersion().User() == 12 && stream.GetVersion().User2() >= 130)
			stream << unkSkinTint;

		stream << skinTintColor;
		break;
	case 6:
		stream << hairTintColor;
		break;
	case 7:
		stream << maxPasses;
		stream << scale;
		break;
	case 11:
		stream << parallaxInnerLayerThickness;
		stream << parallaxRefractionScale;
		stream << parallaxInnerLayerTextureScale;
		stream << parallaxEnvmapStrength;
		break;
	case 14:
		stream << sparkleParameters;
		break;
	case 16:
		stream << eyeCubemapScale;
		stream << eyeLeftReflectionCenter;
		stream << eyeRightReflectionCenter;
		break;
	}
}

void BSLightingShaderProperty::GetStringRefs(std::set<StringRef*>& refs) {
	BSShaderProperty::GetStringRefs(refs);

	refs.insert(&wetMaterialName);
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

void BSLightingShaderProperty::SetType(const uint type) {
	skyrimShaderType = type;
}

Vector3 BSLightingShaderProperty::GetSpecularColor() {
	return specularColor;
}

void BSLightingShaderProperty::SetSpecularColor(const Vector3& color) {
	specularColor = color;
}

float BSLightingShaderProperty::GetSpecularStrength() {
	return specularStrength;
}

void BSLightingShaderProperty::SetSpecularStrength(const float strength) {
	specularStrength = strength;
}

float BSLightingShaderProperty::GetGlossiness() {
	return glossiness;
}

void BSLightingShaderProperty::SetGlossiness(const float gloss) {
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

void BSLightingShaderProperty::SetEmissiveColor(const Color4& color) {
	emissiveColor.x = color.r;
	emissiveColor.y = color.g;
	emissiveColor.z = color.b;
}

float BSLightingShaderProperty::GetEmissiveMultiple() {
	return emissiveMultiple;
}

void BSLightingShaderProperty::SetEmissiveMultiple(const float emissive) {
	emissiveMultiple = emissive;
}

float BSLightingShaderProperty::GetAlpha() {
	return alpha;
}

std::string BSLightingShaderProperty::GetWetMaterialName() {
	return wetMaterialName.GetString();
}

void BSLightingShaderProperty::SetWetMaterialName(const std::string& matName) {
	wetMaterialName.SetString(matName);
}

int BSLightingShaderProperty::CalcBlockSize(NiVersion& version) {
	BSShaderProperty::CalcBlockSize(version);

	blockSize += 20;

	if (version.User() == 12 && version.User2() >= 130)
		blockSize += 4;

	blockSize += 32;

	if (version.User() <= 12 && version.User2() < 130)
		blockSize += 8;

	if (version.User() == 12 && version.User2() >= 130)
		blockSize += 44;

	switch (skyrimShaderType) {
	case 1:
		blockSize += 4;
		if (version.User() == 12 && version.User2() >= 130)
			blockSize += 2;
		break;
	case 5:
		if (version.User() == 12 && version.User2() >= 130)
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


BSEffectShaderProperty::BSEffectShaderProperty(NiStream& stream) : BSEffectShaderProperty() {
	Get(stream);
}

void BSEffectShaderProperty::Get(NiStream& stream) {
	BSShaderProperty::Get(stream);

	sourceTexture.Get(stream, 4);
	stream >> textureClampMode;

	stream >> falloffStartAngle;
	stream >> falloffStopAngle;
	stream >> falloffStartOpacity;
	stream >> falloffStopOpacity;
	stream >> emissiveColor;
	stream >> emissiveMultiple;
	stream >> softFalloffDepth;
	greyscaleTexture.Get(stream, 4);

	if (stream.GetVersion().User() == 12 && stream.GetVersion().User2() >= 130) {
		envMapTexture.Get(stream, 4);
		normalTexture.Get(stream, 4);
		envMaskTexture.Get(stream, 4);
		stream >> envMapScale;
	}
}

void BSEffectShaderProperty::Put(NiStream& stream) {
	BSShaderProperty::Put(stream);

	sourceTexture.Put(stream, 4, false);
	stream << textureClampMode;

	stream << falloffStartAngle;
	stream << falloffStopAngle;
	stream << falloffStartOpacity;
	stream << falloffStopOpacity;
	stream << emissiveColor;
	stream << emissiveMultiple;
	stream << softFalloffDepth;
	greyscaleTexture.Put(stream, 4, false);

	if (stream.GetVersion().User() == 12 && stream.GetVersion().User2() >= 130) {
		envMapTexture.Put(stream, 4, false);
		normalTexture.Put(stream, 4, false);
		envMaskTexture.Put(stream, 4, false);
		stream << envMapScale;
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

void BSEffectShaderProperty::SetEmissiveColor(const Color4& color) {
	emissiveColor = color;
}

float BSEffectShaderProperty::GetEmissiveMultiple() {
	return emissiveMultiple;
}

void BSEffectShaderProperty::SetEmissiveMultiple(const float emissive) {
	emissiveMultiple = emissive;
}

int BSEffectShaderProperty::CalcBlockSize(NiVersion& version) {
	BSShaderProperty::CalcBlockSize(version);

	blockSize += 52;
	blockSize += sourceTexture.GetLength();
	blockSize += greyscaleTexture.GetLength();

	if (version.User() == 12 && version.User2() >= 130) {
		blockSize += 16;
		blockSize += envMapTexture.GetLength();
		blockSize += normalTexture.GetLength();
		blockSize += envMaskTexture.GetLength();
	}

	return blockSize;
}


BSWaterShaderProperty::BSWaterShaderProperty(NiStream& stream) : BSWaterShaderProperty() {
	Get(stream);
}

void BSWaterShaderProperty::Get(NiStream& stream) {
	BSShaderProperty::Get(stream);

	stream >> waterFlags;
}

void BSWaterShaderProperty::Put(NiStream& stream) {
	BSShaderProperty::Put(stream);

	stream << waterFlags;
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

int BSWaterShaderProperty::CalcBlockSize(NiVersion& version) {
	BSShaderProperty::CalcBlockSize(version);

	blockSize += 4;

	return blockSize;
}


BSSkyShaderProperty::BSSkyShaderProperty(NiStream& stream) : BSSkyShaderProperty() {
	Get(stream);
}

void BSSkyShaderProperty::Get(NiStream& stream) {
	BSShaderProperty::Get(stream);

	baseTexture.Get(stream, 4);
	stream >> skyFlags;
}

void BSSkyShaderProperty::Put(NiStream& stream) {
	BSShaderProperty::Put(stream);

	baseTexture.Put(stream, 4, false);
	stream << skyFlags;
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

int BSSkyShaderProperty::CalcBlockSize(NiVersion& version) {
	BSShaderProperty::CalcBlockSize(version);

	blockSize += 8;
	blockSize += baseTexture.GetLength();

	return blockSize;
}


void BSShaderLightingProperty::Get(NiStream& stream) {
	BSShaderProperty::Get(stream);

	if (stream.GetVersion().User() <= 11)
		stream >> textureClampMode;
}

void BSShaderLightingProperty::Put(NiStream& stream) {
	BSShaderProperty::Put(stream);

	if (stream.GetVersion().User() <= 11)
		stream << textureClampMode;
}

int BSShaderLightingProperty::CalcBlockSize(NiVersion& version) {
	BSShaderProperty::CalcBlockSize(version);

	if (version.User() <= 11)
		blockSize += 4;

	return blockSize;
}


BSShaderPPLightingProperty::BSShaderPPLightingProperty(NiStream& stream) : BSShaderPPLightingProperty() {
	Get(stream);
}

void BSShaderPPLightingProperty::Get(NiStream& stream) {
	BSShaderLightingProperty::Get(stream);

	textureSetRef.Get(stream);

	if (stream.GetVersion().User() == 11) {
		stream >> refractionStrength;
		stream >> refractionFirePeriod;
		stream >> parallaxMaxPasses;
		stream >> parallaxScale;
	}

	if (stream.GetVersion().User() >= 12)
		stream >> emissiveColor;
}

void BSShaderPPLightingProperty::Put(NiStream& stream) {
	BSShaderLightingProperty::Put(stream);

	textureSetRef.Put(stream);

	if (stream.GetVersion().User() == 11) {
		stream << refractionStrength;
		stream << refractionFirePeriod;
		stream << parallaxMaxPasses;
		stream << parallaxScale;
	}

	if (stream.GetVersion().User() >= 12)
		stream << emissiveColor;
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

int BSShaderPPLightingProperty::CalcBlockSize(NiVersion& version) {
	BSShaderLightingProperty::CalcBlockSize(version);

	blockSize += 4;

	if (version.User() == 11)
		blockSize += 16;

	if (version.User() >= 12)
		blockSize += 16;

	return blockSize;
}


BSShaderNoLightingProperty::BSShaderNoLightingProperty(NiStream& stream) : BSShaderNoLightingProperty() {
	Get(stream);
}

void BSShaderNoLightingProperty::Get(NiStream& stream) {
	BSShaderLightingProperty::Get(stream);

	baseTexture.Get(stream, 4);

	if (stream.GetVersion().User2() > 26) {
		stream >> falloffStartAngle;
		stream >> falloffStopAngle;
		stream >> falloffStartOpacity;
		stream >> falloffStopOpacity;
	}
}

void BSShaderNoLightingProperty::Put(NiStream& stream) {
	BSShaderLightingProperty::Put(stream);

	baseTexture.Put(stream, 4, false);

	if (stream.GetVersion().User2() > 26) {
		stream << falloffStartAngle;
		stream << falloffStopAngle;
		stream << falloffStartOpacity;
		stream << falloffStopOpacity;
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

int BSShaderNoLightingProperty::CalcBlockSize(NiVersion& version) {
	BSShaderLightingProperty::CalcBlockSize(version);

	blockSize += 4;
	blockSize += baseTexture.GetLength();

	if (version.User2() > 26)
		blockSize += 16;

	return blockSize;
}


NiAlphaProperty::NiAlphaProperty(NiStream& stream) : NiAlphaProperty() {
	Get(stream);
}

void NiAlphaProperty::Get(NiStream& stream) {
	NiProperty::Get(stream);

	stream >> flags;
	stream >> threshold;
}

void NiAlphaProperty::Put(NiStream& stream) {
	NiProperty::Put(stream);

	stream << flags;
	stream << threshold;
}

int NiAlphaProperty::CalcBlockSize(NiVersion& version) {
	NiProperty::CalcBlockSize(version);

	blockSize += 3;

	return blockSize;
}


NiMaterialProperty::NiMaterialProperty(NiStream& stream) : NiMaterialProperty() {
	Get(stream);
}

void NiMaterialProperty::Get(NiStream& stream) {
	NiProperty::Get(stream);

	NiVersion& version = stream.GetVersion();
	if (!(version.File() == NiVersion::Get(20, 2, 0, 7) && version.User() >= 11 && version.User2() > 21)) {
		stream >> colorAmbient;
		stream >> colorDiffuse;
	}

	stream >> colorSpecular;
	stream >> colorEmissive;
	stream >> glossiness;
	stream >> alpha;

	if (version.File() == NiVersion::Get(20, 2, 0, 7) && version.User() >= 11 && version.User2() > 21)
		stream >> emitMulti;
}

void NiMaterialProperty::Put(NiStream& stream) {
	NiProperty::Put(stream);

	NiVersion& version = stream.GetVersion();
	if (!(version.File() == NiVersion::Get(20, 2, 0, 7) && version.User() >= 11 && version.User2() > 21)) {
		stream << colorAmbient;
		stream << colorDiffuse;
	}

	stream << colorSpecular;
	stream << colorEmissive;
	stream << glossiness;
	stream << alpha;

	if (version.File() == NiVersion::Get(20, 2, 0, 7) && version.User() >= 11 && version.User2() > 21)
		stream << emitMulti;
}

bool NiMaterialProperty::IsEmissive() {
	return true;
}

Vector3 NiMaterialProperty::GetSpecularColor() {
	return colorSpecular;
}

void NiMaterialProperty::SetSpecularColor(const Vector3& color) {
	colorSpecular = color;
}

float NiMaterialProperty::GetGlossiness() {
	return glossiness;
}

void NiMaterialProperty::SetGlossiness(const float gloss) {
	glossiness = gloss;
}

Color4 NiMaterialProperty::GetEmissiveColor() {
	Color4 color;
	color.r = colorEmissive.x;
	color.g = colorEmissive.y;
	color.b = colorEmissive.z;
	return color;
}

void NiMaterialProperty::SetEmissiveColor(const Color4& color) {
	colorEmissive.x = color.r;
	colorEmissive.y = color.g;
	colorEmissive.z = color.b;
}

float NiMaterialProperty::GetEmissiveMultiple() {
	return emitMulti;
}

void NiMaterialProperty::SetEmissiveMultiple(const float emissive) {
	emitMulti = emissive;
}

float NiMaterialProperty::GetAlpha() {
	return alpha;
}

int NiMaterialProperty::CalcBlockSize(NiVersion& version) {
	NiProperty::CalcBlockSize(version);

	if (!(version.File() == NiVersion::Get(20, 2, 0, 7) && version.User() >= 11 && version.User2() > 21))
		blockSize += 24;
	else
		blockSize += 4;

	blockSize += 32;

	return blockSize;
}


NiStencilProperty::NiStencilProperty(NiStream& stream) : NiStencilProperty() {
	Get(stream);
}

void NiStencilProperty::Get(NiStream& stream) {
	NiProperty::Get(stream);

	stream >> flags;
	stream >> stencilRef;
	stream >> stencilMask;
}

void NiStencilProperty::Put(NiStream& stream) {
	NiProperty::Put(stream);

	stream << flags;
	stream << stencilRef;
	stream << stencilMask;
}

int NiStencilProperty::CalcBlockSize(NiVersion& version) {
	NiProperty::CalcBlockSize(version);

	blockSize += 10;

	return blockSize;
}
