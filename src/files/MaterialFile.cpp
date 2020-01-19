/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "MaterialFile.h"
#include "../utils/StringStuff.h"

MaterialFile::MaterialFile(const Type& signature) {
	this->signature = signature;
}

MaterialFile::MaterialFile(const std::string& fileName) {
	std::ifstream input(fileName, std::ifstream::binary);
	if (!input) {
		failed = true;
		return;
	}

	if (Read(input))
		failed = true;
}

MaterialFile::MaterialFile(std::istream& input) {
	if (!input) {
		failed = true;
		return;
	}

	if (Read(input))
		failed = true;
}

int MaterialFile::Read(std::istream& input) {
	uint magic;
	input.read((char*)&magic, 4);
	if (magic != BGSM && magic != BGEM)
		return 1;

	signature = (Type)magic;

	input.read((char*)&version, 4);

	uint tileFlags;
	input.read((char*)&tileFlags, 4);
	tileU = (tileFlags & 2) != 0;
	tileV = (tileFlags & 1) != 0;

	input.read((char*)&uvOffset, 8);
	input.read((char*)&uvScale, 8);

	input.read((char*)&alpha, 4);
	byte alphaBlendMode0;
	uint alphaBlendMode1;
	uint alphaBlendMode2;
	input.read((char*)&alphaBlendMode0, 1);
	input.read((char*)&alphaBlendMode1, 4);
	input.read((char*)&alphaBlendMode2, 4);
	alphaBlendMode = ConvertAlphaBlendMode(alphaBlendMode0, alphaBlendMode1, alphaBlendMode2);
	input.read((char*)&alphaTestRef, 1);
	input.read((char*)&alphaTest, 1);

	input.read((char*)&zBufferWrite, 1);
	input.read((char*)&zBufferTest, 1);
	input.read((char*)&screenSpaceReflections, 1);
	input.read((char*)&wetnessControlScreenSpaceReflections, 1);
	input.read((char*)&decal, 1);
	input.read((char*)&twoSided, 1);
	input.read((char*)&decalNoFade, 1);
	input.read((char*)&nonOccluder, 1);

	input.read((char*)&refraction, 1);
	input.read((char*)&refractionFalloff, 1);
	input.read((char*)&refractionPower, 4);

	input.read((char*)&environmentMapping, 1);
	input.read((char*)&environmentMappingMaskScale, 4);

	input.read((char*)&grayscaleToPaletteColor, 1);

	uint length = 0;
	if (signature == BGSM) {
		std::string tmp;
		input.read((char*)&length, 4);
		tmp.resize(length);
		input.read((char*)&tmp.front(), length);
		diffuseTexture = ToOSSlashes(tmp);

		input.read((char*)&length, 4);
		tmp.resize(length);
		input.read((char*)&tmp.front(), length);
		normalTexture = ToOSSlashes(tmp);

		input.read((char*)&length, 4);
		tmp.resize(length);
		input.read((char*)&tmp.front(), length);
		smoothSpecTexture = ToOSSlashes(tmp);

		input.read((char*)&length, 4);
		tmp.resize(length);
		input.read((char*)&tmp.front(), length);
		greyscaleTexture = ToOSSlashes(tmp);

		input.read((char*)&length, 4);
		tmp.resize(length);
		input.read((char*)&tmp.front(), length);
		envmapTexture = ToOSSlashes(tmp);

		input.read((char*)&length, 4);
		tmp.resize(length);
		input.read((char*)&tmp.front(), length);
		glowTexture = ToOSSlashes(tmp);

		input.read((char*)&length, 4);
		tmp.resize(length);
		input.read((char*)&tmp.front(), length);
		innerLayerTexture = ToOSSlashes(tmp);

		input.read((char*)&length, 4);
		tmp.resize(length);
		input.read((char*)&tmp.front(), length);
		wrinklesTexture = ToOSSlashes(tmp);

		input.read((char*)&length, 4);
		tmp.resize(length);
		input.read((char*)&tmp.front(), length);
		displacementTexture = ToOSSlashes(tmp);

		input.read((char*)&enableEditorAlphaRef, 1);
		input.read((char*)&rimLighting, 1);
		input.read((char*)&rimPower, 4);
		input.read((char*)&backLightPower, 4);

		input.read((char*)&subsurfaceLighting, 1);
		input.read((char*)&subsurfaceLightingRolloff, 4);

		input.read((char*)&specularEnabled, 1);
		input.read((char*)&specularColor, 12);
		input.read((char*)&specularMult, 4);
		input.read((char*)&smoothness, 4);
		input.read((char*)&fresnelPower, 4);
		input.read((char*)&wetnessControlSpecScale, 4);
		input.read((char*)&wetnessControlSpecPowerScale, 4);
		input.read((char*)&wetnessControlSpecMinvar, 4);
		input.read((char*)&wetnessControlEnvMapScale, 4);
		input.read((char*)&wetnessControlFresnelPower, 4);
		input.read((char*)&wetnessControlMetalness, 4);

		input.read((char*)&length, 4);
		rootMaterialPath.resize(length);
		input.read((char*)&rootMaterialPath.front(), length);

		input.read((char*)&anisoLighting, 1);
		input.read((char*)&emitEnabled, 1);
		if (emitEnabled)
			input.read((char*)&emittanceColor, 12);

		input.read((char*)&emittanceMult, 4);
		input.read((char*)&modelSpaceNormals, 1);
		input.read((char*)&externalEmittance, 1);
		input.read((char*)&backLighting, 1);

		input.read((char*)&receiveShadows, 1);
		input.read((char*)&hideSecret, 1);
		input.read((char*)&castShadows, 1);
		input.read((char*)&dissolveFade, 1);
		input.read((char*)&assumeShadowmask, 1);

		input.read((char*)&glowMap, 1);
		input.read((char*)&environmentMappingWindow, 1);
		input.read((char*)&environmentMappingEye, 1);
		input.read((char*)&hair, 1);
		input.read((char*)&hairTintColor, 12);
		input.read((char*)&tree, 1);
		input.read((char*)&facegen, 1);
		input.read((char*)&skinTint, 1);

		input.read((char*)&tessellate, 1);
		input.read((char*)&displacementTextureBias, 4);
		input.read((char*)&displacementTextureScale, 4);
		input.read((char*)&tessellationPNScale, 4);
		input.read((char*)&tessellationBaseFactor, 4);
		input.read((char*)&tessellationFadeDistance, 4);

		input.read((char*)&grayscaleToPaletteScale, 4);
		if (version >= 1)
			input.read((char*)&skewSpecularAlpha, 1);
	}
	else if (signature == BGEM) {
		std::string tmp;
		input.read((char*)&length, 4);
		tmp.resize(length);
		input.read((char*)&tmp.front(), length);
		baseTexture = ToOSSlashes(tmp);

		input.read((char*)&length, 4);
		tmp.resize(length);
		input.read((char*)&tmp.front(), length);
		grayscaleTexture = ToOSSlashes(tmp);

		input.read((char*)&length, 4);
		tmp.resize(length);
		input.read((char*)&tmp.front(), length);
		fxEnvmapTexture = ToOSSlashes(tmp);

		input.read((char*)&length, 4);
		tmp.resize(length);
		input.read((char*)&tmp.front(), length);
		fxNormalTexture = ToOSSlashes(tmp);

		input.read((char*)&length, 4);
		tmp.resize(length);
		input.read((char*)&tmp.front(), length);
		envmapMaskTexture = ToOSSlashes(tmp);

		input.read((char*)&bloodEnabled, 1);
		input.read((char*)&effectLightingEnabled, 1);
		input.read((char*)&falloffEnabled, 1);
		input.read((char*)&falloffColorEnabled, 1);
		input.read((char*)&grayscaleToPaletteAlpha, 1);
		input.read((char*)&softEnabled, 1);

		input.read((char*)&baseColor, 12);
		input.read((char*)&falloffStartAngle, 4);
		input.read((char*)&falloffStopAngle, 4);
		input.read((char*)&falloffStartOpacity, 4);
		input.read((char*)&falloffStopOpacity, 4);
		input.read((char*)&lightingInfluence, 4);
		input.read((char*)&envmapMinLOD, 1);
		input.read((char*)&softDepth, 4);
	}

	return 0;
}

int MaterialFile::Write(std::ostream& output) {
	output.write((char*)&signature, 4);

	output.write((char*)&version, 4);

	uint tileFlags = 0;
	if (tileU) tileFlags += 2;
	if (tileV) tileFlags += 1;
	output.write((char*)&tileFlags, 4);

	output.write((char*)&uvOffset, 8);
	output.write((char*)&uvScale, 8);

	output.write((char*)&alpha, 4);
	byte alphaBlendMode0;
	uint alphaBlendMode1;
	uint alphaBlendMode2;
	ConvertAlphaBlendMode(alphaBlendMode, alphaBlendMode0, alphaBlendMode1, alphaBlendMode2);
	output.write((char*)&alphaBlendMode0, 1);
	output.write((char*)&alphaBlendMode1, 4);
	output.write((char*)&alphaBlendMode2, 4);
	output.write((char*)&alphaTestRef, 1);
	output.write((char*)&alphaTest, 1);

	output.write((char*)&zBufferWrite, 1);
	output.write((char*)&zBufferTest, 1);
	output.write((char*)&screenSpaceReflections, 1);
	output.write((char*)&wetnessControlScreenSpaceReflections, 1);
	output.write((char*)&decal, 1);
	output.write((char*)&twoSided, 1);
	output.write((char*)&decalNoFade, 1);
	output.write((char*)&nonOccluder, 1);

	output.write((char*)&refraction, 1);
	output.write((char*)&refractionFalloff, 1);
	output.write((char*)&refractionPower, 4);

	output.write((char*)&environmentMapping, 1);
	output.write((char*)&environmentMappingMaskScale, 4);

	output.write((char*)&grayscaleToPaletteColor, 1);

	uint length = 0;
	if (signature == BGSM) {
		std::string tmp;
		tmp = ToBackslashes(diffuseTexture);
		length = tmp.length();
		output.write((char*)&length, 4);
		output.write(tmp.c_str(), length);

		tmp = ToBackslashes(normalTexture);
		length = tmp.length();
		output.write((char*)&length, 4);
		output.write(tmp.c_str(), length);

		tmp = ToBackslashes(smoothSpecTexture);
		length = tmp.length();
		output.write((char*)&length, 4);
		output.write(tmp.c_str(), length);

		tmp = ToBackslashes(greyscaleTexture);
		length = tmp.length();
		output.write((char*)&length, 4);
		output.write(tmp.c_str(), length);

		tmp = ToBackslashes(envmapTexture);
		length = tmp.length();
		output.write((char*)&length, 4);
		output.write(tmp.c_str(), length);

		tmp = ToBackslashes(glowTexture);
		length = tmp.length();
		output.write((char*)&length, 4);
		output.write(tmp.c_str(), length);

		tmp = ToBackslashes(innerLayerTexture);
		length = tmp.length();
		output.write((char*)&length, 4);
		output.write(tmp.c_str(), length);

		tmp = ToBackslashes(wrinklesTexture);
		length = tmp.length();
		output.write((char*)&length, 4);
		output.write(tmp.c_str(), length);

		tmp = ToBackslashes(displacementTexture);
		length = tmp.length();
		output.write((char*)&length, 4);
		output.write(tmp.c_str(), length);

		output.write((char*)&enableEditorAlphaRef, 1);
		output.write((char*)&rimLighting, 1);
		output.write((char*)&rimPower, 4);
		output.write((char*)&backLightPower, 4);

		output.write((char*)&subsurfaceLighting, 1);
		output.write((char*)&subsurfaceLightingRolloff, 4);

		output.write((char*)&specularEnabled, 1);
		output.write((char*)&specularColor, 12);
		output.write((char*)&specularMult, 4);
		output.write((char*)&smoothness, 4);
		output.write((char*)&fresnelPower, 4);
		output.write((char*)&wetnessControlSpecScale, 4);
		output.write((char*)&wetnessControlSpecPowerScale, 4);
		output.write((char*)&wetnessControlSpecMinvar, 4);
		output.write((char*)&wetnessControlEnvMapScale, 4);
		output.write((char*)&wetnessControlFresnelPower, 4);
		output.write((char*)&wetnessControlMetalness, 4);

		length = rootMaterialPath.length();
		output.write((char*)&length, 4);
		output.write(rootMaterialPath.c_str(), length);

		output.write((char*)&anisoLighting, 1);
		output.write((char*)&emitEnabled, 1);
		if (emitEnabled)
			output.write((char*)&emittanceColor, 12);

		output.write((char*)&emittanceMult, 4);
		output.write((char*)&modelSpaceNormals, 1);
		output.write((char*)&externalEmittance, 1);
		output.write((char*)&backLighting, 1);

		output.write((char*)&receiveShadows, 1);
		output.write((char*)&hideSecret, 1);
		output.write((char*)&castShadows, 1);
		output.write((char*)&dissolveFade, 1);
		output.write((char*)&assumeShadowmask, 1);

		output.write((char*)&glowMap, 1);
		output.write((char*)&environmentMappingWindow, 1);
		output.write((char*)&environmentMappingEye, 1);
		output.write((char*)&hair, 1);
		output.write((char*)&hairTintColor, 12);
		output.write((char*)&tree, 1);
		output.write((char*)&facegen, 1);
		output.write((char*)&skinTint, 1);

		output.write((char*)&tessellate, 1);
		output.write((char*)&displacementTextureBias, 4);
		output.write((char*)&displacementTextureScale, 4);
		output.write((char*)&tessellationPNScale, 4);
		output.write((char*)&tessellationBaseFactor, 4);
		output.write((char*)&tessellationFadeDistance, 4);

		output.write((char*)&grayscaleToPaletteScale, 4);
		if (version >= 1)
			output.write((char*)&skewSpecularAlpha, 1);
	}
	else if (signature == BGEM) {
		std::string tmp;
		tmp = ToBackslashes(baseTexture);
		length = tmp.length();
		output.write((char*)&length, 4);
		output.write(tmp.c_str(), length);

		tmp = ToBackslashes(grayscaleTexture);
		length = tmp.length();
		output.write((char*)&length, 4);
		output.write(tmp.c_str(), length);

		tmp = ToBackslashes(fxEnvmapTexture);
		length = tmp.length();
		output.write((char*)&length, 4);
		output.write(tmp.c_str(), length);

		tmp = ToBackslashes(fxNormalTexture);
		length = tmp.length();
		output.write((char*)&length, 4);
		output.write(tmp.c_str(), length);

		tmp = ToBackslashes(envmapMaskTexture);
		length = tmp.length();
		output.write((char*)&length, 4);
		output.write(tmp.c_str(), length);

		output.write((char*)&bloodEnabled, 1);
		output.write((char*)&effectLightingEnabled, 1);
		output.write((char*)&falloffEnabled, 1);
		output.write((char*)&falloffColorEnabled, 1);
		output.write((char*)&grayscaleToPaletteAlpha, 1);
		output.write((char*)&softEnabled, 1);

		output.write((char*)&baseColor, 12);
		output.write((char*)&falloffStartAngle, 4);
		output.write((char*)&falloffStopAngle, 4);
		output.write((char*)&falloffStartOpacity, 4);
		output.write((char*)&falloffStopOpacity, 4);
		output.write((char*)&lightingInfluence, 4);
		output.write((char*)&envmapMinLOD, 1);
		output.write((char*)&softDepth, 4);
	}

	return 0;
}

MaterialFile::AlphaBlendModeType MaterialFile::ConvertAlphaBlendMode(const byte a, const uint b, const uint c) {
	if (a == 0 && b == 6 && c == 7)
		return AlphaBlendModeType::Unknown;
	else if (a == 0 && b == 0 && c == 0)
		return AlphaBlendModeType::None;
	else if (a == 1 && b == 6 && c == 7)
		return AlphaBlendModeType::Standard;
	else if (a == 1 && b == 6 && c == 0)
		return AlphaBlendModeType::Additive;
	else if (a == 1 && b == 4 && c == 1)
		return AlphaBlendModeType::Multiplicative;
	else
		return AlphaBlendModeType::None;
}

void MaterialFile::ConvertAlphaBlendMode(const AlphaBlendModeType& type, byte& a, uint& b, uint& c) {
	if (type == AlphaBlendModeType::Unknown) {
		a = 0;
		b = 6;
		c = 7;
	}
	else if (type == AlphaBlendModeType::None) {
		a = 0;
		b = 0;
		c = 0;
	}
	else if (type == AlphaBlendModeType::Standard) {
		a = 1;
		b = 6;
		c = 7;
	}
	else if (type == AlphaBlendModeType::Additive) {
		a = 1;
		b = 6;
		c = 0;
	}
	else if (type == AlphaBlendModeType::Multiplicative) {
		a = 1;
		b = 4;
		c = 1;
	}
	else {
		a = 0;
		b = 0;
		c = 0;
	}
}
