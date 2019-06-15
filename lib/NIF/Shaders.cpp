/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "Shaders.h"

void NiShadeProperty::Get(NiStream& stream) {
	NiProperty::Get(stream);

	stream >> flags;
}

void NiShadeProperty::Put(NiStream& stream) {
	NiProperty::Put(stream);

	stream << flags;
}


void NiSpecularProperty::Get(NiStream& stream) {
	NiProperty::Get(stream);

	stream >> flags;
}

void NiSpecularProperty::Put(NiStream& stream) {
	NiProperty::Put(stream);

	stream << flags;
}


void NiTexturingProperty::Get(NiStream& stream) {
	NiProperty::Get(stream);

	stream >> flags;
	stream >> textureCount;

	if (textureCount > 0) {
		stream >> hasBaseTex;
		if (hasBaseTex)
			baseTex.Get(stream);
	}

	if (textureCount > 1) {
		stream >> hasDarkTex;
		if (hasDarkTex)
			darkTex.Get(stream);
	}

	if (textureCount > 2) {
		stream >> hasDetailTex;
		if (hasDetailTex)
			detailTex.Get(stream);
	}

	if (textureCount > 3) {
		stream >> hasGlossTex;
		if (hasGlossTex)
			glossTex.Get(stream);
	}

	if (textureCount > 4) {
		stream >> hasGlowTex;
		if (hasGlowTex)
			glowTex.Get(stream);
	}

	if (textureCount > 5) {
		stream >> hasBumpTex;
		if (hasBumpTex) {
			bumpTex.Get(stream);
			stream >> lumaScale;
			stream >> lumaOffset;
			stream >> bumpMatrix;
		}
	}

	if (textureCount > 6) {
		stream >> hasNormalTex;
		if (hasNormalTex)
			normalTex.Get(stream);
	}

	if (textureCount > 7) {
		stream >> hasParallaxTex;
		if (hasParallaxTex) {
			parallaxTex.Get(stream);
			stream >> parallaxFloat;
		}
	}

	if (textureCount > 8) {
		stream >> hasDecalTex0;
		if (hasDecalTex0)
			decalTex0.Get(stream);
	}

	if (textureCount > 9) {
		stream >> hasDecalTex1;
		if (hasDecalTex1)
			decalTex1.Get(stream);
	}

	if (textureCount > 10) {
		stream >> hasDecalTex2;
		if (hasDecalTex2)
			decalTex2.Get(stream);
	}

	if (textureCount > 11) {
		stream >> hasDecalTex3;
		if (hasDecalTex3)
			decalTex3.Get(stream);
	}

	stream >> numShaderTex;
	shaderTex.resize(numShaderTex);
	for (int i = 0; i < numShaderTex; i++)
		shaderTex[i].Get(stream);
}

void NiTexturingProperty::Put(NiStream& stream) {
	NiProperty::Put(stream);

	stream << flags;
	stream << textureCount;

	if (textureCount > 0) {
		stream << hasBaseTex;
		if (hasBaseTex)
			baseTex.Put(stream);
	}

	if (textureCount > 1) {
		stream << hasDarkTex;
		if (hasDarkTex)
			darkTex.Put(stream);
	}

	if (textureCount > 2) {
		stream << hasDetailTex;
		if (hasDetailTex)
			detailTex.Put(stream);
	}

	if (textureCount > 3) {
		stream << hasGlossTex;
		if (hasGlossTex)
			glossTex.Put(stream);
	}

	if (textureCount > 4) {
		stream << hasGlowTex;
		if (hasGlowTex)
			glowTex.Put(stream);
	}

	if (textureCount > 5) {
		stream << hasBumpTex;
		if (hasBumpTex) {
			bumpTex.Put(stream);
			stream << lumaScale;
			stream << lumaOffset;
			stream << bumpMatrix;
		}
	}

	if (textureCount > 6) {
		stream << hasNormalTex;
		if (hasNormalTex)
			normalTex.Put(stream);
	}

	if (textureCount > 7) {
		stream << hasParallaxTex;
		if (hasParallaxTex) {
			parallaxTex.Put(stream);
			stream << parallaxFloat;
		}
	}

	if (textureCount > 8) {
		stream << hasDecalTex0;
		if (hasDecalTex0)
			decalTex0.Put(stream);
	}

	if (textureCount > 9) {
		stream << hasDecalTex1;
		if (hasDecalTex1)
			decalTex1.Put(stream);
	}

	if (textureCount > 10) {
		stream << hasDecalTex2;
		if (hasDecalTex2)
			decalTex2.Put(stream);
	}

	if (textureCount > 11) {
		stream << hasDecalTex3;
		if (hasDecalTex3)
			decalTex3.Put(stream);
	}

	stream << numShaderTex;
	for (int i = 0; i < numShaderTex; i++)
		shaderTex[i].Put(stream);
}

void NiTexturingProperty::GetChildRefs(std::set<Ref*>& refs) {
	NiProperty::GetChildRefs(refs);

	baseTex.GetChildRefs(refs);
	darkTex.GetChildRefs(refs);
	detailTex.GetChildRefs(refs);
	glossTex.GetChildRefs(refs);
	glowTex.GetChildRefs(refs);
	bumpTex.GetChildRefs(refs);
	normalTex.GetChildRefs(refs);
	parallaxTex.GetChildRefs(refs);
	decalTex0.GetChildRefs(refs);
	decalTex1.GetChildRefs(refs);
	decalTex2.GetChildRefs(refs);
	decalTex3.GetChildRefs(refs);

	for (auto &t : shaderTex)
		t.GetChildRefs(refs);
}


void NiVertexColorProperty::Get(NiStream& stream) {
	NiProperty::Get(stream);

	stream >> flags;
}

void NiVertexColorProperty::Put(NiStream& stream) {
	NiProperty::Put(stream);

	stream << flags;
}


void NiDitherProperty::Get(NiStream& stream) {
	NiProperty::Get(stream);

	stream >> flags;
}

void NiDitherProperty::Put(NiStream& stream) {
	NiProperty::Put(stream);

	stream << flags;
}


void NiFogProperty::Get(NiStream& stream) {
	NiProperty::Get(stream);

	stream >> flags;
	stream >> fogDepth;
	stream >> fogColor;
}

void NiFogProperty::Put(NiStream& stream) {
	NiProperty::Put(stream);

	stream << flags;
	stream << fogDepth;
	stream << fogColor;
}


void NiWireframeProperty::Get(NiStream& stream) {
	NiProperty::Get(stream);

	stream >> flags;
}

void NiWireframeProperty::Put(NiStream& stream) {
	NiProperty::Put(stream);

	stream << flags;
}


void NiZBufferProperty::Get(NiStream& stream) {
	NiProperty::Get(stream);

	stream >> flags;
}

void NiZBufferProperty::Put(NiStream& stream) {
	NiProperty::Put(stream);

	stream << flags;
}


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

uint BSShaderProperty::GetShaderType() {
	return shaderType;
}

void BSShaderProperty::SetShaderType(uint type) {
	shaderType = (BSShaderType)type;
}

bool BSShaderProperty::IsSkinTinted() {
	return shaderType == SHADER_SKIN;
}

bool BSShaderProperty::IsFaceTinted() {
	return shaderType == SHADER_SKIN;
}

bool BSShaderProperty::IsSkinned() {
	return (shaderFlags1 & (1 << 1)) != 0;
}

void BSShaderProperty::SetSkinned(const bool enable) {
	if (enable)
		shaderFlags1 |= 1 << 1;
	else
		shaderFlags1 &= ~(1 << 1);
}

bool BSShaderProperty::IsDoubleSided() {
	return (shaderFlags2 & (1 << 4)) != 0;
}

bool BSShaderProperty::IsModelSpace() {
	return (shaderFlags1 & (1 << 12)) != 0;
}

bool BSShaderProperty::IsEmissive() {
	return (shaderFlags1 & (1 << 22)) != 0;
}

bool BSShaderProperty::HasSpecular() {
	return (shaderFlags1 & (1 << 0)) != 0;
}

bool BSShaderProperty::HasVertexColors() {
	return (shaderFlags2 & (1 << 5)) != 0;
}

void BSShaderProperty::SetVertexColors(const bool enable) {
	if (enable)
		shaderFlags2 |= 1 << 5;
	else
		shaderFlags2 &= ~(1 << 5);
}

bool BSShaderProperty::HasVertexAlpha() {
	return (shaderFlags1 & (1 << 3)) != 0;
}

void BSShaderProperty::SetVertexAlpha(const bool enable) {
	if (enable)
		shaderFlags1 |= 1 << 3;
	else
		shaderFlags1 &= ~(1 << 3);
}

bool BSShaderProperty::HasBacklight() {
	// Skyrim
	return (shaderFlags2 & (1 << 27)) != 0;
}

bool BSShaderProperty::HasRimlight() {
	// Skyrim
	return (shaderFlags2 & (1 << 26)) != 0;
}

bool BSShaderProperty::HasSoftlight() {
	// Skyrim
	return (shaderFlags2 & (1 << 25)) != 0;
}

bool BSShaderProperty::HasGlowmap() {
	return (shaderFlags2 & (1 << 6)) != 0;
}

bool BSShaderProperty::HasGreyscaleColor() {
	return (shaderFlags1 & (1 << 3)) != 0;
}

bool BSShaderProperty::HasEnvironmentMapping() {
	return (shaderFlags1 & (1 << 7)) != 0;
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


void TallGrassShaderProperty::Get(NiStream& stream) {
	BSShaderProperty::Get(stream);

	fileName.Get(stream, 4);
}

void TallGrassShaderProperty::Put(NiStream& stream) {
	BSShaderProperty::Put(stream);

	fileName.Put(stream, 4, false);
}


void SkyShaderProperty::Get(NiStream& stream) {
	BSShaderLightingProperty::Get(stream);

	fileName.Get(stream, 4);
	stream >> skyObjectType;
}

void SkyShaderProperty::Put(NiStream& stream) {
	BSShaderLightingProperty::Put(stream);

	fileName.Put(stream, 4, false);
	stream << skyObjectType;
}


void TileShaderProperty::Get(NiStream& stream) {
	BSShaderLightingProperty::Get(stream);

	fileName.Get(stream, 4);
}

void TileShaderProperty::Put(NiStream& stream) {
	BSShaderLightingProperty::Put(stream);

	fileName.Put(stream, 4, false);
}


BSShaderTextureSet::BSShaderTextureSet(NiVersion& version) {
	if (version.User() == 12 && version.Stream() >= 130)
		numTextures = 10;
	else if (version.User() == 12)
		numTextures = 9;
	else
		numTextures = 6;

	textures.resize(numTextures);
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

BSLightingShaderProperty::BSLightingShaderProperty() {
	NiObjectNET::bBSLightingShaderProperty = true;

	shaderFlags1 = 0x80400203;
	shaderFlags2 = 0x00000081;

	unkFloat1 = std::numeric_limits<float>::max();
}

BSLightingShaderProperty::BSLightingShaderProperty(NiVersion& version) : BSLightingShaderProperty() {
	if (version.User() == 12 && version.Stream() >= 120) {
		shaderFlags1 = 0x80400203;
		shaderFlags2 = 0x00000081;
	}
	else {
		shaderFlags1 = 0x82400303;
		shaderFlags2 = 0x00008001;
	}

	if (version.User() == 12 && version.Stream() >= 120)
		glossiness = 1.0f;
	else
		glossiness = 20.0f;
}

void BSLightingShaderProperty::Get(NiStream& stream) {
	BSShaderProperty::Get(stream);

	textureSetRef.Get(stream);

	stream >> emissiveColor;
	stream >> emissiveMultiple;

	if (stream.GetVersion().User() == 12 && stream.GetVersion().Stream() >= 130)
		wetMaterialName.Get(stream);

	stream >> textureClampMode;
	stream >> alpha;
	stream >> refractionStrength;
	stream >> glossiness;
	stream >> specularColor;
	stream >> specularStrength;

	if (stream.GetVersion().User() <= 12 && stream.GetVersion().Stream() < 130) {
		stream >> softlighting;
		stream >> rimlightPower;
	}

	if (stream.GetVersion().User() == 12 && stream.GetVersion().Stream() >= 130) {
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

	switch (bslspShaderType) {
	case 1:
		stream >> environmentMapScale;
		if (stream.GetVersion().User() == 12 && stream.GetVersion().Stream() >= 130)
			stream >> unkEnvmap;
		break;
	case 5:
		if (stream.GetVersion().User() == 12 && stream.GetVersion().Stream() >= 130)
			stream >> skinTintAlpha;

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

	if (stream.GetVersion().User() == 12 && stream.GetVersion().Stream() >= 130)
		wetMaterialName.Put(stream);

	stream << textureClampMode;
	stream << alpha;
	stream << refractionStrength;
	stream << glossiness;
	stream << specularColor;
	stream << specularStrength;

	if (stream.GetVersion().User() <= 12 && stream.GetVersion().Stream() < 130) {
		stream << softlighting;
		stream << rimlightPower;
	}

	if (stream.GetVersion().User() == 12 && stream.GetVersion().Stream() >= 130) {
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

	switch (bslspShaderType) {
	case 1:
		stream << environmentMapScale;
		if (stream.GetVersion().User() == 12 && stream.GetVersion().Stream() >= 130)
			stream << unkEnvmap;
		break;
	case 5:
		if (stream.GetVersion().User() == 12 && stream.GetVersion().Stream() >= 130)
			stream << skinTintAlpha;

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

void BSLightingShaderProperty::GetChildRefs(std::set<Ref*>& refs) {
	BSShaderProperty::GetChildRefs(refs);

	refs.insert(&textureSetRef);
}

bool BSLightingShaderProperty::IsSkinTinted() {
	return bslspShaderType == BSLSP_SKINTINT;
}

bool BSLightingShaderProperty::IsFaceTinted() {
	return bslspShaderType == BSLSP_FACE;
}

bool BSLightingShaderProperty::HasGlowmap() {
	return bslspShaderType == BSLSP_GLOWMAP && BSShaderProperty::HasGlowmap();
}

bool BSLightingShaderProperty::HasEnvironmentMapping() {
	return bslspShaderType == BSLSP_ENVMAP && BSShaderProperty::HasEnvironmentMapping();
}

uint BSLightingShaderProperty::GetShaderType() {
	return bslspShaderType;
}

void BSLightingShaderProperty::SetShaderType(const uint type) {
	bslspShaderType = type;
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
	return textureSetRef.GetIndex();
}

void BSLightingShaderProperty::SetTextureSetRef(const int texSetRef) {
	textureSetRef.SetIndex(texSetRef);
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

float BSLightingShaderProperty::GetBacklightPower() {
	return backlightPower;
}

float BSLightingShaderProperty::GetRimlightPower() {
	return rimlightPower;
}

float BSLightingShaderProperty::GetSoftlight() {
	return softlighting;
}

float BSLightingShaderProperty::GetSubsurfaceRolloff() {
	return subsurfaceRolloff;
}

float BSLightingShaderProperty::GetGrayscaleToPaletteScale() {
	return grayscaleToPaletteScale;
}

float BSLightingShaderProperty::GetFresnelPower() {
	return fresnelPower;
}

std::string BSLightingShaderProperty::GetWetMaterialName() {
	return wetMaterialName.GetString();
}

void BSLightingShaderProperty::SetWetMaterialName(const std::string& matName) {
	wetMaterialName.SetString(matName);
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

	if (stream.GetVersion().User() == 12 && stream.GetVersion().Stream() >= 130) {
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

	if (stream.GetVersion().User() == 12 && stream.GetVersion().Stream() >= 130) {
		envMapTexture.Put(stream, 4, false);
		normalTexture.Put(stream, 4, false);
		envMaskTexture.Put(stream, 4, false);
		stream << envMapScale;
	}
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


void BSWaterShaderProperty::Get(NiStream& stream) {
	BSShaderProperty::Get(stream);

	stream >> waterFlags;
}

void BSWaterShaderProperty::Put(NiStream& stream) {
	BSShaderProperty::Put(stream);

	stream << waterFlags;
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


void BSShaderPPLightingProperty::Get(NiStream& stream) {
	BSShaderLightingProperty::Get(stream);

	textureSetRef.Get(stream);

	if (stream.GetVersion().User() == 11 && stream.GetVersion().Stream() > 14) {
		stream >> refractionStrength;
		stream >> refractionFirePeriod;
	}

	if (stream.GetVersion().User() == 11 && stream.GetVersion().Stream() > 24) {
		stream >> parallaxMaxPasses;
		stream >> parallaxScale;
	}

	if (stream.GetVersion().User() >= 12)
		stream >> emissiveColor;
}

void BSShaderPPLightingProperty::Put(NiStream& stream) {
	BSShaderLightingProperty::Put(stream);

	textureSetRef.Put(stream);

	if (stream.GetVersion().User() == 11 && stream.GetVersion().Stream() > 14) {
		stream << refractionStrength;
		stream << refractionFirePeriod;
	}

	if (stream.GetVersion().User() == 11 && stream.GetVersion().Stream() > 24) {
		stream << parallaxMaxPasses;
		stream << parallaxScale;
	}

	if (stream.GetVersion().User() >= 12)
		stream << emissiveColor;
}

void BSShaderPPLightingProperty::GetChildRefs(std::set<Ref*>& refs) {
	BSShaderLightingProperty::GetChildRefs(refs);

	refs.insert(&textureSetRef);
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
	return textureSetRef.GetIndex();
}

void BSShaderPPLightingProperty::SetTextureSetRef(const int texSetRef) {
	textureSetRef.SetIndex(texSetRef);
}


void BSShaderNoLightingProperty::Get(NiStream& stream) {
	BSShaderLightingProperty::Get(stream);

	baseTexture.Get(stream, 4);

	if (stream.GetVersion().Stream() > 26) {
		stream >> falloffStartAngle;
		stream >> falloffStopAngle;
		stream >> falloffStartOpacity;
		stream >> falloffStopOpacity;
	}
}

void BSShaderNoLightingProperty::Put(NiStream& stream) {
	BSShaderLightingProperty::Put(stream);

	baseTexture.Put(stream, 4, false);

	if (stream.GetVersion().Stream() > 26) {
		stream << falloffStartAngle;
		stream << falloffStopAngle;
		stream << falloffStartOpacity;
		stream << falloffStopOpacity;
	}
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


void NiMaterialProperty::Get(NiStream& stream) {
	NiProperty::Get(stream);

	if (stream.GetVersion().Stream() < 26) {
		stream >> colorAmbient;
		stream >> colorDiffuse;
	}

	stream >> colorSpecular;
	stream >> colorEmissive;
	stream >> glossiness;
	stream >> alpha;

	if (stream.GetVersion().Stream() > 21)
		stream >> emitMulti;
}

void NiMaterialProperty::Put(NiStream& stream) {
	NiProperty::Put(stream);

	if (stream.GetVersion().Stream() < 26) {
		stream << colorAmbient;
		stream << colorDiffuse;
	}

	stream << colorSpecular;
	stream << colorEmissive;
	stream << glossiness;
	stream << alpha;

	if (stream.GetVersion().Stream() > 21)
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
