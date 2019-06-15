/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "BasicTypes.h"
#include "Objects.h"

enum BSShaderType : uint {
	SHADER_TALL_GRASS,
	SHADER_DEFAULT,
	SHADER_SKY = 10,
	SHADER_SKIN = 14,
	SHADER_WATER = 17,
	SHADER_LIGHTING30 = 29,
	SHADER_TILE = 32,
	SHADER_NOLIGHTING
};

enum BSLightingShaderPropertyShaderType : uint {
	BSLSP_DEFAULT,
	BSLSP_ENVMAP,
	BSLSP_GLOWMAP,
	BSLSP_PARALLAX,
	BSLSP_FACE,
	BSLSP_SKINTINT,
	BSLSP_HAIRTINT,
	BSLSP_PARALLAXOCC,
	BSLSP_MULTITEXTURELANDSCAPE,
	BSLSP_LODLANDSCAPE,
	BSLSP_SNOW,
	BSLSP_MULTILAYERPARALLAX,
	BSLSP_TREEANIM,
	BSLSP_LODOBJECTS,
	BSLSP_MULTIINDEXSNOW,
	BSLSP_LODOBJECTSHD,
	BSLSP_EYE,
	BSLSP_CLOUD,
	BSLSP_LODLANDSCAPENOISE,
	BSLSP_MULTITEXTURELANDSCAPELODBLEND,
	BSLSP_DISMEMBERMENT,
	BSLSP_LAST = BSLSP_DISMEMBERMENT
};

class NiProperty : public NiObjectNET {
};

class NiShadeProperty : public NiProperty {
private:
	ushort flags = 0;

public:
	static constexpr const char* BlockName = "NiShadeProperty";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);

	NiShadeProperty* Clone() { return new NiShadeProperty(*this); }
};

class NiSpecularProperty : public NiProperty {
private:
	ushort flags = 0;

public:
	static constexpr const char* BlockName = "NiSpecularProperty";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);

	NiSpecularProperty* Clone() { return new NiSpecularProperty(*this); }
};

struct TexTransform {
	Vector2 translation;
	Vector2 tiling;
	float wRotation = 0.0f;
	uint transformType = 0;
	Vector2 offset;
};

class TexDesc {
private:
	BlockRef<NiSourceTexture> sourceRef;
	ushort flags = 0;
	bool hasTexTransform = false;
	TexTransform transform;

public:
	void Get(NiStream& stream) {
		sourceRef.Get(stream);
		stream >> flags;
		stream >> hasTexTransform;

		if (hasTexTransform)
			stream >> transform;
	}

	void Put(NiStream& stream) {
		sourceRef.Put(stream);
		stream << flags;
		stream << hasTexTransform;

		if (hasTexTransform)
			stream << transform;
	}

	void GetChildRefs(std::set<Ref*>& refs) {
		refs.insert(&sourceRef);
	}

	int GetSourceRef() {
		return sourceRef.GetIndex();
	}

	void SetSourceRef(int srcRef) {
		sourceRef.SetIndex(srcRef);
	}
};

class ShaderTexDesc {
private:
	bool isUsed = false;
	TexDesc data;
	uint mapIndex = 0;

public:
	void Get(NiStream& stream) {
		stream >> isUsed;

		if (isUsed) {
			data.Get(stream);
			stream >> mapIndex;
		}
	}

	void Put(NiStream& stream) {
		stream << isUsed;

		if (isUsed) {
			data.Put(stream);
			stream << mapIndex;
		}
	}

	void GetChildRefs(std::set<Ref*>& refs) {
		data.GetChildRefs(refs);
	}
};

class NiTexturingProperty : public NiProperty {
private:
	ushort flags = 0;
	uint textureCount = 0;

	bool hasBaseTex = false;
	TexDesc baseTex;

	bool hasDarkTex = false;
	TexDesc darkTex;

	bool hasDetailTex = false;
	TexDesc detailTex;

	bool hasGlossTex = false;
	TexDesc glossTex;

	bool hasGlowTex = false;
	TexDesc glowTex;

	bool hasBumpTex = false;
	TexDesc bumpTex;
	float lumaScale = 1.0f;
	float lumaOffset = 0.0f;
	Vector4 bumpMatrix;

	bool hasNormalTex = false;
	TexDesc normalTex;

	bool hasParallaxTex = false;
	TexDesc parallaxTex;
	float parallaxFloat = 0.0f;

	bool hasDecalTex0 = false;
	TexDesc decalTex0;

	bool hasDecalTex1 = false;
	TexDesc decalTex1;

	bool hasDecalTex2 = false;
	TexDesc decalTex2;

	bool hasDecalTex3 = false;
	TexDesc decalTex3;

	uint numShaderTex = 0;
	std::vector<ShaderTexDesc> shaderTex;

public:
	static constexpr const char* BlockName = "NiTexturingProperty";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetChildRefs(std::set<Ref*>& refs);

	NiTexturingProperty* Clone() { return new NiTexturingProperty(*this); }
};

class NiVertexColorProperty : public NiProperty {
private:
	ushort flags = 0;

public:
	static constexpr const char* BlockName = "NiVertexColorProperty";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);

	NiVertexColorProperty* Clone() { return new NiVertexColorProperty(*this); }
};

class NiDitherProperty : public NiProperty {
private:
	ushort flags = 0;

public:
	static constexpr const char* BlockName = "NiDitherProperty";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);

	NiDitherProperty* Clone() { return new NiDitherProperty(*this); }
};

class NiFogProperty : public NiProperty {
private:
	ushort flags = 0;
	float fogDepth = 1.0f;
	Color3 fogColor;

public:
	static constexpr const char* BlockName = "NiFogProperty";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);

	NiFogProperty* Clone() { return new NiFogProperty(*this); }
};

class NiWireframeProperty : public NiProperty {
private:
	ushort flags = 0;

public:
	static constexpr const char* BlockName = "NiWireframeProperty";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);

	NiWireframeProperty* Clone() { return new NiWireframeProperty(*this); }
};

class NiZBufferProperty : public NiProperty {
private:
	ushort flags = 3;

public:
	static constexpr const char* BlockName = "NiZBufferProperty";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);

	NiZBufferProperty* Clone() { return new NiZBufferProperty(*this); }
};

class NiShader : public NiProperty {
public:
	virtual bool IsSkinTinted() { return false; }
	virtual bool IsFaceTinted() { return false; }
	virtual bool IsSkinned() { return false; }
	virtual void SetSkinned(const bool) {}
	virtual bool IsDoubleSided() { return false; }
	virtual bool IsModelSpace() { return false; }
	virtual bool IsEmissive() { return false; }
	virtual bool HasSpecular() { return true; }
	virtual bool HasVertexColors() { return false; }
	virtual void SetVertexColors(const bool) {}
	virtual bool HasVertexAlpha() { return false; }
	virtual void SetVertexAlpha(const bool) {}
	virtual bool HasBacklight() { return false; }
	virtual bool HasRimlight() { return false; }
	virtual bool HasSoftlight() { return false; }
	virtual bool HasGlowmap() { return false; }
	virtual bool HasGreyscaleColor() { return false; }
	virtual bool HasEnvironmentMapping() { return false; }
	virtual uint GetShaderType() { return 0; }
	virtual void SetShaderType(const uint) {}
	virtual Vector2 GetUVOffset() { return Vector2(); }
	virtual Vector2 GetUVScale() { return Vector2(1.0f, 1.0f); }
	virtual Vector3 GetSpecularColor() { return Vector3(); }
	virtual void SetSpecularColor(const Vector3&) {}
	virtual float GetSpecularStrength() { return 0.0f; }
	virtual void SetSpecularStrength(const float) {}
	virtual float GetGlossiness() { return 0.0f; }
	virtual void SetGlossiness(const float) {}
	virtual float GetEnvironmentMapScale() { return 0.0f; }
	virtual int GetTextureSetRef() { return 0xFFFFFFFF; }
	virtual void SetTextureSetRef(const int) {}
	virtual Color4 GetEmissiveColor() { return Color4(); }
	virtual void SetEmissiveColor(const Color4&) {}
	virtual float GetEmissiveMultiple() { return 0.0f; }
	virtual void SetEmissiveMultiple(const float) {}
	virtual float GetAlpha() { return 1.0f; }
	virtual float GetBacklightPower() { return 0.0f; }
	virtual float GetRimlightPower() { return 2.0f; }
	virtual float GetSoftlight() { return 0.3f; }
	virtual float GetSubsurfaceRolloff() { return 0.3f; }
	virtual float GetGrayscaleToPaletteScale() { return 1.0; }
	virtual float GetFresnelPower() { return 5.0f; }
	virtual std::string GetWetMaterialName() { return std::string(); }
	virtual void SetWetMaterialName(const std::string&) {}
};

class BSShaderProperty : public NiShader {
public:
	ushort shaderFlags = 1;
	BSShaderType shaderType = SHADER_DEFAULT;
	uint shaderFlags1 = 0x82000000;
	uint shaderFlags2 = 1;
	float environmentMapScale = 1.0f;
	Vector2 uvOffset;
	Vector2 uvScale = Vector2(1.0f, 1.0f);

	void Get(NiStream& stream);
	void Put(NiStream& stream);

	uint GetShaderType();
	void SetShaderType(const uint type);
	bool IsSkinTinted();
	bool IsFaceTinted();
	bool IsSkinned();
	void SetSkinned(const bool enable);
	bool IsDoubleSided();
	bool IsModelSpace();
	bool IsEmissive();
	bool HasSpecular();
	bool HasVertexColors();
	void SetVertexColors(const bool enable);
	bool HasVertexAlpha();
	void SetVertexAlpha(const bool enable);
	bool HasBacklight();
	bool HasRimlight();
	bool HasSoftlight();
	bool HasGlowmap();
	bool HasGreyscaleColor();
	bool HasEnvironmentMapping();
	float GetEnvironmentMapScale();
	Vector2 GetUVOffset();
	Vector2 GetUVScale();
};

class WaterShaderProperty : public BSShaderProperty {
public:
	static constexpr const char* BlockName = "WaterShaderProperty";
	virtual const char* GetBlockName() { return BlockName; }

	WaterShaderProperty* Clone() { return new WaterShaderProperty(*this); }
};

class HairShaderProperty : public BSShaderProperty {
public:
	static constexpr const char* BlockName = "HairShaderProperty";
	virtual const char* GetBlockName() { return BlockName; }

	HairShaderProperty* Clone() { return new HairShaderProperty(*this); }
};

class DistantLODShaderProperty : public BSShaderProperty {
public:
	static constexpr const char* BlockName = "DistantLODShaderProperty";
	virtual const char* GetBlockName() { return BlockName; }

	DistantLODShaderProperty* Clone() { return new DistantLODShaderProperty(*this); }
};

class BSDistantTreeShaderProperty : public BSShaderProperty {
public:
	static constexpr const char* BlockName = "BSDistantTreeShaderProperty";
	virtual const char* GetBlockName() { return BlockName; }

	BSDistantTreeShaderProperty* Clone() { return new BSDistantTreeShaderProperty(*this); }
};

class TallGrassShaderProperty : public BSShaderProperty {
private:
	NiString fileName;

public:
	static constexpr const char* BlockName = "TallGrassShaderProperty";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);

	TallGrassShaderProperty* Clone() { return new TallGrassShaderProperty(*this); }
};

class VolumetricFogShaderProperty : public BSShaderProperty {
public:
	static constexpr const char* BlockName = "VolumetricFogShaderProperty";
	virtual const char* GetBlockName() { return BlockName; }

	VolumetricFogShaderProperty* Clone() { return new VolumetricFogShaderProperty(*this); }
};

class BSShaderTextureSet : public NiObject {
public:
	int numTextures = 10;
	std::vector<NiString> textures = std::vector<NiString>(10);

	BSShaderTextureSet() {}
	BSShaderTextureSet(NiVersion& version);

	static constexpr const char* BlockName = "BSShaderTextureSet";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	BSShaderTextureSet* Clone() { return new BSShaderTextureSet(*this); }
};

class BSLightingShaderProperty : public BSShaderProperty {
private:
	StringRef wetMaterialName;
	BlockRef<BSShaderTextureSet> textureSetRef;

public:
	Vector3 emissiveColor;
	float emissiveMultiple = 1.0f;
	uint textureClampMode = 3;
	float alpha = 1.0f;
	float refractionStrength = 0.0f;
	float glossiness = 1.0f;
	Vector3 specularColor = Vector3(1.0f, 1.0f, 1.0f);
	float specularStrength = 1.0f;
	float softlighting = 0.3f;				// User Version <= 12, User Version 2 < 130
	float rimlightPower = 2.0f;				// User Version <= 12, User Version 2 < 130

	float subsurfaceRolloff = 0.3f;			// User Version == 12, User Version 2 >= 130
	float unkFloat1 = 0.0f;					// User Version == 12, User Version 2 >= 130
	float backlightPower = 0.0f;			// User Version == 12, User Version 2 >= 130
	float grayscaleToPaletteScale = 1.0f;	// User Version == 12, User Version 2 >= 130
	float fresnelPower = 5.0f;				// User Version == 12, User Version 2 >= 130
	float wetnessSpecScale = 0.6f;			// User Version == 12, User Version 2 >= 130
	float wetnessSpecPower = 1.4f;			// User Version == 12, User Version 2 >= 130
	float wetnessMinVar = 0.2f;				// User Version == 12, User Version 2 >= 130
	float wetnessEnvmapScale = 1.0f;		// User Version == 12, User Version 2 >= 130
	float wetnessFresnelPower = 1.6f;		// User Version == 12, User Version 2 >= 130
	float wetnessMetalness = 0.0f;			// User Version == 12, User Version 2 >= 130

	ushort unkEnvmap = 0;											// Shader Type == 1, User Version == 12, User Version 2 >= 130
	Vector3 skinTintColor = Vector3(1.0f, 1.0f, 1.0f);				// Shader Type == 5
	float skinTintAlpha = 0.0f;										// Shader Type == 5, User Version == 12, User Version 2 >= 130
	Vector3 hairTintColor = Vector3(1.0f, 1.0f, 1.0f);				// Shader Type == 6
	float maxPasses = 1.0f;											// Shader Type == 7
	float scale = 1.0f;												// Shader Type == 7
	float parallaxInnerLayerThickness = 0.0f;						// Shader Type == 11
	float parallaxRefractionScale = 1.0f;							// Shader Type == 11
	Vector2 parallaxInnerLayerTextureScale = Vector2(1.0f, 1.0f);	// Shader Type == 11
	float parallaxEnvmapStrength = 1.0f;							// Shader Type == 11
	Color4 sparkleParameters;										// Shader Type == 14
	float eyeCubemapScale = 1.0f;									// Shader Type == 16
	Vector3 eyeLeftReflectionCenter;								// Shader Type == 16
	Vector3 eyeRightReflectionCenter;								// Shader Type == 16

	BSLightingShaderProperty();
	BSLightingShaderProperty(NiVersion& version);

	static constexpr const char* BlockName = "BSLightingShaderProperty";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetStringRefs(std::set<StringRef*>& refs);
	void GetChildRefs(std::set<Ref*>& refs);
	BSLightingShaderProperty* Clone() { return new BSLightingShaderProperty(*this); }

	bool IsSkinTinted();
	bool IsFaceTinted();
	bool HasGlowmap();
	bool HasEnvironmentMapping();
	uint GetShaderType();
	void SetShaderType(const uint type);
	Vector3 GetSpecularColor();
	void SetSpecularColor(const Vector3& color);
	float GetSpecularStrength();
	void SetSpecularStrength(const float strength);
	float GetGlossiness();
	void SetGlossiness(const float gloss);
	int GetTextureSetRef();
	void SetTextureSetRef(const int texSetRef);
	Color4 GetEmissiveColor();
	void SetEmissiveColor(const Color4& color);
	float GetEmissiveMultiple();
	void SetEmissiveMultiple(const float emissive);
	float GetAlpha();
	float GetBacklightPower();
	float GetRimlightPower();
	float GetSoftlight();
	float GetSubsurfaceRolloff();
	float GetGrayscaleToPaletteScale();
	float GetFresnelPower();
	std::string GetWetMaterialName();
	void SetWetMaterialName(const std::string& matName);
};

class BSEffectShaderProperty : public BSShaderProperty {
public:
	NiString sourceTexture;
	uint textureClampMode = 0;
	float falloffStartAngle = 1.0f;
	float falloffStopAngle = 1.0f;
	float falloffStartOpacity = 0.0f;
	float falloffStopOpacity = 0.0f;
	Color4 emissiveColor;
	float emissiveMultiple = 0.0f;
	float softFalloffDepth = 0.0f;
	NiString greyscaleTexture;

	NiString envMapTexture;
	NiString normalTexture;
	NiString envMaskTexture;
	float envMapScale = 1.0f;

	static constexpr const char* BlockName = "BSEffectShaderProperty";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	BSEffectShaderProperty* Clone() { return new BSEffectShaderProperty(*this); }

	float GetEnvironmentMapScale();
	Color4 GetEmissiveColor();
	void SetEmissiveColor(const Color4& color);
	float GetEmissiveMultiple();
	void SetEmissiveMultiple(const float emissive);
};

class BSWaterShaderProperty : public BSShaderProperty {
private:
	uint waterFlags = 0;

public:
	static constexpr const char* BlockName = "BSWaterShaderProperty";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	BSWaterShaderProperty* Clone() { return new BSWaterShaderProperty(*this); }
};

class BSSkyShaderProperty : public BSShaderProperty {
private:
	NiString baseTexture;
	uint skyFlags = 0;

public:
	static constexpr const char* BlockName = "BSSkyShaderProperty";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	BSSkyShaderProperty* Clone() { return new BSSkyShaderProperty(*this); }
};

class BSShaderLightingProperty : public BSShaderProperty {
public:
	uint textureClampMode = 3;				// User Version <= 11

	void Get(NiStream& stream);
	void Put(NiStream& stream);
};

enum SkyObjectType : uint {
	BSSM_SKY_TEXTURE,
	BSSM_SKY_SUNGLARE,
	BSSM_SKY,
	BSSM_SKY_CLOUDS,
	BSSM_SKY_STARS = 5,
	BSSM_SKY_MOON_STARS_MASK = 7
};

class SkyShaderProperty : public BSShaderLightingProperty {
private:
	NiString fileName;
	SkyObjectType skyObjectType = BSSM_SKY_TEXTURE;

public:
	static constexpr const char* BlockName = "SkyShaderProperty";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);

	SkyShaderProperty* Clone() { return new SkyShaderProperty(*this); }
};

class TileShaderProperty : public BSShaderLightingProperty {
private:
	NiString fileName;

public:
	static constexpr const char* BlockName = "TileShaderProperty";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);

	TileShaderProperty* Clone() { return new TileShaderProperty(*this); }
};

class BSShaderNoLightingProperty : public BSShaderLightingProperty {
public:
	NiString baseTexture;
	float falloffStartAngle = 1.0f;			// User Version 2 > 26
	float falloffStopAngle = 0.0f;			// User Version 2 > 26
	float falloffStartOpacity = 1.0f;		// User Version 2 > 26
	float falloffStopOpacity = 1.0f;		// User Version 2 > 26

	static constexpr const char* BlockName = "BSShaderNoLightingProperty";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	BSShaderNoLightingProperty* Clone() { return new BSShaderNoLightingProperty(*this); }

	bool IsSkinned();
	void SetSkinned(const bool enable);
};

class BSShaderPPLightingProperty : public BSShaderLightingProperty {
private:
	BlockRef<BSShaderTextureSet> textureSetRef;

public:
	float refractionStrength = 0.0f;		// User Version == 11 && User Version 2 > 14
	int refractionFirePeriod = 0;			// User Version == 11 && User Version 2 > 14
	float parallaxMaxPasses = 4.0f;			// User Version == 11 && User Version 2 > 24
	float parallaxScale = 1.0f;				// User Version == 11 && User Version 2 > 24
	Color4 emissiveColor;					// User Version >= 12

	static constexpr const char* BlockName = "BSShaderPPLightingProperty";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetChildRefs(std::set<Ref*>& refs);
	BSShaderPPLightingProperty* Clone() { return new BSShaderPPLightingProperty(*this); }

	bool IsSkinned();
	void SetSkinned(const bool enable);
	int GetTextureSetRef();
	void SetTextureSetRef(const int texSetRef);
};

class Lighting30ShaderProperty : public BSShaderPPLightingProperty {
public:
	static constexpr const char* BlockName = "Lighting30ShaderProperty";
	virtual const char* GetBlockName() { return BlockName; }

	Lighting30ShaderProperty* Clone() { return new Lighting30ShaderProperty(*this); }
};

class NiAlphaProperty : public NiProperty {
public:
	ushort flags = 4844;
	byte threshold = 128;

	static constexpr const char* BlockName = "NiAlphaProperty";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	NiAlphaProperty* Clone() { return new NiAlphaProperty(*this); }
};


class NiMaterialProperty : public NiProperty {
private:
	Vector3 colorSpecular;
	Vector3 colorEmissive;
	float glossiness = 1.0f;
	float alpha = 1.0f;
	float emitMulti = 1.0f;					// Version == 20.2.0.7 && User Version >= 11 && User Version 2 > 21

public:
	Vector3 colorAmbient;					// !(Version == 20.2.0.7 && User Version >= 11 && User Version 2 > 21)
	Vector3 colorDiffuse;					// !(Version == 20.2.0.7 && User Version >= 11 && User Version 2 > 21)

	static constexpr const char* BlockName = "NiMaterialProperty";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	NiMaterialProperty* Clone() { return new NiMaterialProperty(*this); }

	bool IsEmissive();
	Vector3 GetSpecularColor();
	void SetSpecularColor(const Vector3& color);
	float GetGlossiness();
	void SetGlossiness(const float gloss);
	Color4 GetEmissiveColor();
	void SetEmissiveColor(const Color4& color);
	float GetEmissiveMultiple();
	void SetEmissiveMultiple(const float emissive);
	float GetAlpha();
};

enum StencilMasks {
	ENABLE_MASK = 0x0001,
	FAIL_MASK = 0x000E,
	FAIL_POS = 1,
	ZFAIL_MASK = 0x0070,
	ZFAIL_POS = 4,
	ZPASS_MASK = 0x0380,
	ZPASS_POS = 7,
	DRAW_MASK = 0x0C00,
	DRAW_POS = 10,
	TEST_MASK = 0x7000,
	TEST_POS = 12
};

enum DrawMode {
	DRAW_CCW_OR_BOTH,
	DRAW_CCW,
	DRAW_CW,
	DRAW_BOTH,
	DRAW_MAX
};

class NiStencilProperty : public NiProperty {
public:
	ushort flags = 19840;
	uint stencilRef = 0;
	uint stencilMask = 0xFFFFFFFF;

	static constexpr const char* BlockName = "NiStencilProperty";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	NiStencilProperty* Clone() { return new NiStencilProperty(*this); }
};
