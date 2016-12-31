/*
BodySlide and Outfit Studio
Copyright (C) 2017  Caliente & ousnius
See the included LICENSE file
*/

#pragma once

#include "../utils/Object3d.h"
#include <fstream>

using namespace std;

class MaterialFile {
	bool failed = false;

public:
	enum Type {
		BGSM = 0x4D534742,
		BGEM = 0x4D454742
	};

	enum AlphaBlendModeType {
		Unknown,
		None,
		Standard,
		Additive,
		Multiplicative
	};

	// Base
	Type signature = Type::BGSM;
	uint version = 1;
	bool tileU = true;
	bool tileV = true;
	Vector2 uvOffset;
	Vector2 uvScale{ 1.0f, 1.0f };
	float alpha = 1.0f;
	AlphaBlendModeType alphaBlendMode = AlphaBlendModeType::Unknown;
	char alphaTestRef = -128;
	bool alphaTest = false;
	bool zBufferWrite = true;
	bool zBufferTest = true;
	bool screenSpaceReflections = false;
	bool wetnessControlScreenSpaceReflections = false;
	bool decal = false;
	bool twoSided = false;
	bool decalNoFade = false;
	bool nonOccluder = false;
	bool refraction = false;
	bool refractionFalloff = false;
	float refractionPower = 0.0f;
	bool environmentMapping = false;
	float environmentMappingMaskScale = 1.0f;
	bool grayscaleToPaletteColor = false;

	// BGSM 0x4D534742
	string diffuseTexture;
	string normalTexture;
	string smoothSpecTexture;
	string greyscaleTexture;
	string envmapTexture;
	string glowTexture;
	string innerLayerTexture;
	string wrinklesTexture;
	string displacementTexture;
	bool enableEditorAlphaRef = false;
	bool rimLighting = false;
	float rimPower = 2.0f;
	float backLightPower;
	bool subsurfaceLighting = false;
	float subsurfaceLightingRolloff = 0.3f;
	bool specularEnabled = false;
	Vector3 specularColor = { 1.0f, 1.0f, 1.0f };
	float specularMult = 1.0f;
	float smoothness = 1.0f;
	float fresnelPower = 5.0f;
	float wetnessControlSpecScale = -1.0f;
	float wetnessControlSpecPowerScale = -1.0f;
	float wetnessControlSpecMinvar = -1.0f;
	float wetnessControlEnvMapScale = -1.0f;
	float wetnessControlFresnelPower = -1.0f;
	float wetnessControlMetalness = -1.0f;
	string rootMaterialPath;
	bool anisoLighting = false;
	bool emitEnabled = false;
	Vector3 emittanceColor = { 1.0f, 1.0f, 1.0f };
	float emittanceMult = 1.0f;
	bool modelSpaceNormals = false;
	bool externalEmittance = false;
	bool backLighting = false;
	bool receiveShadows = false;
	bool hideSecret = false;
	bool castShadows = false;
	bool dissolveFade = false;
	bool assumeShadowmask = false;
	bool glowMap = false;
	bool environmentMappingWindow = false;
	bool environmentMappingEye = false;
	bool hair = false;
	Vector3 hairTintColor = { 0.5f, 0.5f, 0.5f };
	bool tree = false;
	bool facegen = false;
	bool skinTint = false;
	bool tessellate = false;
	float displacementTextureBias = -0.5f;
	float displacementTextureScale = 10.0f;
	float tessellationPNScale = 1.0f;
	float tessellationBaseFactor = 1.0f;
	float tessellationFadeDistance = 0.0f;
	float grayscaleToPaletteScale = 1.0f;
	bool skewSpecularAlpha = false;

	// BGEM 0x4D454742
	string baseTexture;
	string grayscaleTexture;
	string fxEnvmapTexture;
	string fxNormalTexture;
	string envmapMaskTexture;
	bool bloodEnabled = false;
	bool effectLightingEnabled = false;
	bool falloffEnabled = false;
	bool falloffColorEnabled = false;
	bool grayscaleToPaletteAlpha = false;
	bool softEnabled = false;
	Vector3 baseColor = { 1.0f, 1.0f, 1.0f };
	float baseColorScale = 1.0f;
	float falloffStartAngle = 1.0f;
	float falloffStopAngle = 1.0f;
	float falloffStartOpacity = 0.0f;
	float falloffStopOpacity = 0.0f;
	float lightingInfluence = 1.0f;
	byte envmapMinLOD = 0;
	float softDepth = 100.0f;

	MaterialFile() {
		signature = BGSM;
	}

	MaterialFile(const Type&);
	MaterialFile(const string&);
	MaterialFile(istream&);

	int Read(istream&);
	int Write(ostream&);

	bool Failed() {
		return failed;
	}

	AlphaBlendModeType ConvertAlphaBlendMode(const byte, const uint, const uint);
	void ConvertAlphaBlendMode(const AlphaBlendModeType&, byte&, uint&, uint&);
};
